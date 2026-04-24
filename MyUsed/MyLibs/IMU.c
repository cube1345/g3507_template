#include "IMU.h"

#include <math.h>
#include "icm42688_driver.h"
#include "my_system_lib.h"


/* 四元数单位化时用于避免除零的最小模长阈值。*/
#define IMU_QUATERNION_NORM_EPSILON          (1.0e-6f)

/* 三轴向量单位化时用于避免除零的最小模长阈值。*/
#define IMU_VECTOR_NORM_EPSILON              (1.0e-6f)

/* 当前 IMU 算法层输出数据快照，用于集中保存传感器物理量、零偏和姿态结果。*/
static imu_data_t g_imu_data;

/* 当前 IMU 姿态调试信息快照，用于分析漂移时各中间补偿量是否合理。*/
static imu_debug_info_t g_imu_debug_info;
static imu_profile_t g_imu_profile;
static imu_timebase_ops_t g_imu_timebase_ops;
static imu_config_t g_imu_config = {
    2.0f,
    0.05f,
    0.5f,
    1.5f,
    1U,
    0.05f,
    0.20f,
    50U,
    0.01f
};

/* 当前 IMU 算法层绑定的底层传感器标准接口，由应用层在初始化前注入。*/
static imu_sensor_ops_t g_imu_sensor_ops;

/* IMU 算法层初始化完成标志，true 表示底层驱动和算法状态已准备就绪。*/
static bool g_imu_is_initialized = false;

/* IMU 陀螺仪零偏校准完成标志，true 表示 gyro_bias_* 字段已经具备有效校准结果。*/
static bool g_imu_is_gyro_calibrated = false;

xyz_f_t north = {0.0f, 0.0f, 0.0f};
xyz_f_t west = {0.0f, 0.0f, 0.0f};
volatile float yaw[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
volatile uint8_t g_imu_debug_init_ok = 0U;
volatile uint8_t g_imu_debug_comm_ok = 0U;
volatile uint32_t g_imu_debug_sample_count = 0U;
volatile float g_imu_debug_accel[3] = {0.0f, 0.0f, 0.0f};
volatile float g_imu_debug_gyro[3] = {0.0f, 0.0f, 0.0f};
volatile float g_imu_debug_gyro_offset[3] = {0.0f, 0.0f, 0.0f};
volatile float g_imu_debug_q[4] = {1.0f, 0.0f, 0.0f, 0.0f};
volatile float g_imu_debug_ypr[3] = {0.0f, 0.0f, 0.0f};

#define IMU_LEGACY_BIAS_SAMPLE_COUNT         (200U)
#define IMU_LEGACY_BIAS_INTERVAL_MS          (5U)

static icm42688_init_config_t g_imu_legacy_icm_init_config;
static bool g_imu_legacy_icm_config_ready = false;

/* Mahony 积分补偿项状态，用于累计重力方向误差的低频分量。*/
static imu_vector3f_t g_imu_mahony_integral_error;
static uint64_t g_imu_last_update_time_us = 0ULL;
static bool g_imu_has_last_update_time = false;
static uint16_t g_imu_static_detect_count = 0U;
static bool imu_is_timebase_bound(void);

/**
  * @brief  将自动 dt 计算使用的时间基准重新同步到当前时刻。
  * @param  None
  * @retval None
  */
static void imu_sync_auto_update_timebase(void)
{
    if (imu_is_timebase_bound() == false) {
        g_imu_last_update_time_us = 0ULL;
        g_imu_has_last_update_time = false;
        return;
    }

    if (g_imu_timebase_ops.get_time_us != (imu_timebase_get_us_fn_t) 0) {
        g_imu_last_update_time_us =
            g_imu_timebase_ops.get_time_us(g_imu_timebase_ops.user_data);
    } else {
        g_imu_last_update_time_us =
            ((uint64_t) g_imu_timebase_ops.get_time_ms(g_imu_timebase_ops.user_data)) * 1000ULL;
    }

    g_imu_has_last_update_time = true;
}

/**
  * @brief  判断当前 IMU 配置是否有效。
  * @param  config 待检查的 IMU 配置指针。
  * @retval 返回 true 表示有效，返回 false 表示无效。
  */
static bool imu_is_valid_config(const imu_config_t *config)
{
    if (config == (const imu_config_t *) 0) {
        return false;
    }

    if (config->mahony_kp < 0.0f) {
        return false;
    }

    if (config->mahony_ki < 0.0f) {
        return false;
    }

    if ((config->accel_norm_min_g <= 0.0f) ||
        (config->accel_norm_max_g <= config->accel_norm_min_g)) {
        return false;
    }

    if (config->static_accel_tolerance_g < 0.0f) {
        return false;
    }

    if (config->static_gyro_threshold_dps < 0.0f) {
        return false;
    }

    if (config->static_detect_count_threshold == 0U) {
        return false;
    }

    if (config->online_gyro_bias_z_alpha < 0.0f) {
        return false;
    }

    return true;
}

/**
  * @brief  根据绑定状态判断 IMU 底层传感器接口是否可用。
  * @param  None
  * @retval 返回 true 表示已绑定有效传感器接口，返回 false 表示尚未完成绑定。
  */
static bool imu_is_sensor_bound(void)
{
    return ((g_imu_sensor_ops.init != (imu_sensor_init_fn_t) 0) &&
        (g_imu_sensor_ops.read != (imu_sensor_read_fn_t) 0));
}

static bool imu_is_timebase_bound(void)
{
    return ((g_imu_timebase_ops.get_time_us != (imu_timebase_get_us_fn_t) 0) ||
        (g_imu_timebase_ops.get_time_ms != (imu_timebase_get_ms_fn_t) 0));
}

/**
  * @brief  计算三轴向量的欧氏模长。
  * @param  vector：待计算模长的三轴向量指针。
  * @retval 返回向量模长。
  */
static float imu_vector3f_get_norm(const imu_vector3f_t *vector)
{
    if (vector == (const imu_vector3f_t *) 0) {
        return 0.0f;
    }

    return sqrtf((vector->x * vector->x) +
        (vector->y * vector->y) +
        (vector->z * vector->z));
}

/**
  * @brief  对三轴向量进行单位化处理。
  * @param  vector：待单位化的三轴向量指针。
  * @retval 返回 true 表示单位化成功，返回 false 表示向量模长过小不适合单位化。
  */
static bool imu_vector3f_normalize(imu_vector3f_t *vector)
{
    float vector_norm;

    if (vector == (imu_vector3f_t *) 0) {
        return false;
    }

    vector_norm = imu_vector3f_get_norm(vector);
    if (vector_norm < IMU_VECTOR_NORM_EPSILON) {
        return false;
    }

    vector->x /= vector_norm;
    vector->y /= vector_norm;
    vector->z /= vector_norm;

    return true;
}

/**
  * @brief  对当前四元数进行单位化，避免积分误差导致姿态发散。
  * @param  None
  * @retval None
  */
static void imu_normalize_quaternion(void)
{
    float quaternion_norm = sqrtf((g_imu_data.quaternion.w * g_imu_data.quaternion.w) +
        (g_imu_data.quaternion.x * g_imu_data.quaternion.x) +
        (g_imu_data.quaternion.y * g_imu_data.quaternion.y) +
        (g_imu_data.quaternion.z * g_imu_data.quaternion.z));

    if (quaternion_norm < IMU_QUATERNION_NORM_EPSILON) {
        g_imu_data.quaternion.w = 1.0f;
        g_imu_data.quaternion.x = 0.0f;
        g_imu_data.quaternion.y = 0.0f;
        g_imu_data.quaternion.z = 0.0f;
        return;
    }

    g_imu_data.quaternion.w /= quaternion_norm;
    g_imu_data.quaternion.x /= quaternion_norm;
    g_imu_data.quaternion.y /= quaternion_norm;
    g_imu_data.quaternion.z /= quaternion_norm;
}

/**
  * @brief  根据当前四元数更新欧拉角输出，分别给出弧度制和角度制结果。
  * @param  None
  * @retval None
  */
static void imu_update_euler_from_quaternion(void)
{
    float sin_pitch;
    float roll_rad;
    float pitch_rad;
    float yaw_rad;
    float w = g_imu_data.quaternion.w;
    float x = g_imu_data.quaternion.x;
    float y = g_imu_data.quaternion.y;
    float z = g_imu_data.quaternion.z;

    roll_rad = atan2f(2.0f * ((w * x) + (y * z)),
        1.0f - 2.0f * ((x * x) + (y * y)));

    sin_pitch = 2.0f * ((w * y) - (z * x));
    if (sin_pitch > 1.0f) {
        sin_pitch = 1.0f;
    } else if (sin_pitch < -1.0f) {
        sin_pitch = -1.0f;
    }
    pitch_rad = asinf(sin_pitch);

    yaw_rad = atan2f(2.0f * ((w * z) + (x * y)),
        1.0f - 2.0f * ((y * y) + (z * z)));

    g_imu_data.euler_rad.roll = roll_rad;
    g_imu_data.euler_rad.pitch = pitch_rad;
    g_imu_data.euler_rad.yaw = yaw_rad;

    g_imu_data.euler_deg.roll = roll_rad * IMU_RAD_TO_DEG;
    g_imu_data.euler_deg.pitch = pitch_rad * IMU_RAD_TO_DEG;
    g_imu_data.euler_deg.yaw = yaw_rad * IMU_RAD_TO_DEG;
}

/**
  * @brief  根据当前静止加速度方向估算初始 roll/pitch，并据此建立与重力方向对齐的初始四元数。
  * @param  None
  * @retval 返回 true 表示初始姿态对齐成功，返回 false 表示当前加速度不适合用作重力参考。
  */
static bool imu_align_quaternion_with_gravity(void)
{
    imu_vector3f_t accel_unit;
    float accel_norm;
    float roll_rad;
    float pitch_rad;
    float half_roll;
    float half_pitch;
    float cos_half_roll;
    float sin_half_roll;
    float cos_half_pitch;
    float sin_half_pitch;

    accel_unit = g_imu_data.accel_g;
    accel_norm = imu_vector3f_get_norm(&accel_unit);

    if ((accel_norm < g_imu_config.accel_norm_min_g) ||
        (accel_norm > g_imu_config.accel_norm_max_g)) {
        return false;
    }

    if (imu_vector3f_normalize(&accel_unit) == false) {
        return false;
    }

    /*
     * 这里使用静止时的重力方向估算初始横滚角和俯仰角，
     * yaw 在仅有加速度计参考时无法确定，因此初始化阶段先固定为 0。
     */
    roll_rad = atan2f(accel_unit.y, accel_unit.z);
    pitch_rad = atan2f(-accel_unit.x,
        sqrtf((accel_unit.y * accel_unit.y) + (accel_unit.z * accel_unit.z)));

    half_roll = 0.5f * roll_rad;
    half_pitch = 0.5f * pitch_rad;

    cos_half_roll = cosf(half_roll);
    sin_half_roll = sinf(half_roll);
    cos_half_pitch = cosf(half_pitch);
    sin_half_pitch = sinf(half_pitch);

    g_imu_data.quaternion.w = cos_half_roll * cos_half_pitch;
    g_imu_data.quaternion.x = sin_half_roll * cos_half_pitch;
    g_imu_data.quaternion.y = cos_half_roll * sin_half_pitch;
    g_imu_data.quaternion.z = -sin_half_roll * sin_half_pitch;

    imu_normalize_quaternion();
    imu_update_euler_from_quaternion();

    return true;
}

/**
  * @brief  从 ICM42688 驱动层读取一帧换算后的物理量数据，并更新 IMU 当前传感器输出缓存。
  * @param  None
  * @retval 返回驱动状态码。
  */
static imu_status_t imu_refresh_sensor_data(void)
{
    imu_sensor_data_t sensor_data;
    imu_status_t imu_status;

    if (imu_is_sensor_bound() == false) {
        return IMU_STATUS_ERROR_UNBOUND;
    }

    imu_status = g_imu_sensor_ops.read(&sensor_data, g_imu_sensor_ops.user_data);
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    g_imu_data.temperature_c = sensor_data.temperature_c;
    g_imu_data.accel_g = sensor_data.accel_g;
    g_imu_data.gyro_dps = sensor_data.gyro_dps;
    g_imu_data.gyro_rad_s = sensor_data.gyro_rad_s;

    return IMU_STATUS_OK;
}

/**
  * @brief  将当前陀螺仪数据减去零偏，并输出为本次姿态积分使用的角速度。
  * @param  gyro_rad_s：角速度输出向量指针，单位为 rad/s。
  * @retval None
  */
static void imu_get_bias_compensated_gyro(imu_vector3f_t *gyro_rad_s)
{
    if (gyro_rad_s == (imu_vector3f_t *) 0) {
        return;
    }

    gyro_rad_s->x = g_imu_data.gyro_rad_s.x - g_imu_data.gyro_bias_rad_s.x;
    gyro_rad_s->y = g_imu_data.gyro_rad_s.y - g_imu_data.gyro_bias_rad_s.y;
    gyro_rad_s->z = g_imu_data.gyro_rad_s.z - g_imu_data.gyro_bias_rad_s.z;

    g_imu_debug_info.gyro_bias_compensated_rad_s = *gyro_rad_s;
    g_imu_debug_info.gyro_bias_compensated_dps.x = gyro_rad_s->x * IMU_RAD_TO_DEG;
    g_imu_debug_info.gyro_bias_compensated_dps.y = gyro_rad_s->y * IMU_RAD_TO_DEG;
    g_imu_debug_info.gyro_bias_compensated_dps.z = gyro_rad_s->z * IMU_RAD_TO_DEG;
}

/**
  * @brief  在启用宏开关后，仅当设备满足静止判定条件时，缓慢在线微调 Z 轴陀螺仪零偏。
  * @param  gyro_rad_s：当前去零偏后的陀螺仪角速度向量指针，单位为 rad/s。
  * @retval None
  */
static void imu_update_online_gyro_bias_trim(const imu_vector3f_t *gyro_rad_s)
{
    float accel_error_g;
    bool is_static;

    g_imu_debug_info.gyro_bias_trim_enabled =
        (g_imu_config.enable_online_gyro_bias_trim != 0U);
    g_imu_debug_info.gyro_bias_trim_active = false;
    g_imu_debug_info.static_detect_count = g_imu_static_detect_count;
    if (gyro_rad_s == (const imu_vector3f_t *) 0) {
        return;
    }

    if (g_imu_config.enable_online_gyro_bias_trim == 0U) {
        return;
    }

    accel_error_g = fabsf(g_imu_debug_info.accel_norm_g - 1.0f);
    is_static = ((accel_error_g <= g_imu_config.static_accel_tolerance_g) &&
        (fabsf(g_imu_debug_info.gyro_bias_compensated_dps.x) <= g_imu_config.static_gyro_threshold_dps) &&
        (fabsf(g_imu_debug_info.gyro_bias_compensated_dps.y) <= g_imu_config.static_gyro_threshold_dps) &&
        (fabsf(g_imu_debug_info.gyro_bias_compensated_dps.z) <= g_imu_config.static_gyro_threshold_dps));

    if (is_static == true) {
        if (g_imu_static_detect_count < g_imu_config.static_detect_count_threshold) {
            g_imu_static_detect_count++;
        }
    } else {
        g_imu_static_detect_count = 0U;
    }

    g_imu_debug_info.static_detect_count = g_imu_static_detect_count;

    if (g_imu_static_detect_count < g_imu_config.static_detect_count_threshold) {
        return;
    }

    g_imu_data.gyro_bias_rad_s.z += g_imu_config.online_gyro_bias_z_alpha * gyro_rad_s->z;
    g_imu_data.gyro_bias_dps.z = g_imu_data.gyro_bias_rad_s.z * IMU_RAD_TO_DEG;
    g_imu_debug_info.gyro_bias_trim_active = true;
}

/**
  * @brief  使用加速度方向对当前角速度做一阶姿态修正，降低纯积分带来的漂移。
  * @param  gyro_rad_s：待修正的角速度向量指针，单位为 rad/s。
  * @retval None
  */
static void imu_apply_mahony_feedback(imu_vector3f_t *gyro_rad_s, float dt_s)
{
    imu_vector3f_t accel_unit;
    float accel_norm;
    float half_vx;
    float half_vy;
    float half_vz;
    float half_ex;
    float half_ey;
    float half_ez;
    float q0 = g_imu_data.quaternion.w;
    float q1 = g_imu_data.quaternion.x;
    float q2 = g_imu_data.quaternion.y;
    float q3 = g_imu_data.quaternion.z;

    if ((gyro_rad_s == (imu_vector3f_t *) 0) || (dt_s <= 0.0f)) {
        return;
    }

    accel_unit = g_imu_data.accel_g;
    accel_norm = imu_vector3f_get_norm(&accel_unit);
    g_imu_debug_info.accel_norm_g = accel_norm;
    g_imu_debug_info.accel_feedback_valid = false;
    g_imu_debug_info.accel_unit.x = 0.0f;
    g_imu_debug_info.accel_unit.y = 0.0f;
    g_imu_debug_info.accel_unit.z = 0.0f;
    g_imu_debug_info.gravity_estimate.x = 0.0f;
    g_imu_debug_info.gravity_estimate.y = 0.0f;
    g_imu_debug_info.gravity_estimate.z = 0.0f;
    g_imu_debug_info.mahony_half_error.x = 0.0f;
    g_imu_debug_info.mahony_half_error.y = 0.0f;
    g_imu_debug_info.mahony_half_error.z = 0.0f;

    /*
     * 只有当加速度模长接近 1g 时，才认为其主要反映的是重力方向，
     * 这样可以避免剧烈线性加速度对姿态修正造成明显干扰。
     */
    if ((accel_norm < g_imu_config.accel_norm_min_g) ||
        (accel_norm > g_imu_config.accel_norm_max_g)) {
        return;
    }

    if (imu_vector3f_normalize(&accel_unit) == false) {
        return;
    }

    g_imu_debug_info.accel_feedback_valid = true;
    g_imu_debug_info.accel_unit = accel_unit;

    /* 根据当前四元数估计机体坐标系下的重力方向。*/
    half_vx = (q1 * q3) - (q0 * q2);
    half_vy = (q0 * q1) + (q2 * q3);
    half_vz = (q0 * q0) - 0.5f + (q3 * q3);
    g_imu_debug_info.gravity_estimate.x = 2.0f * half_vx;
    g_imu_debug_info.gravity_estimate.y = 2.0f * half_vy;
    g_imu_debug_info.gravity_estimate.z = 2.0f * half_vz;

    /* 使用测量重力方向与估计重力方向的叉乘误差构造 Mahony 反馈项。*/
    half_ex = (accel_unit.y * half_vz) - (accel_unit.z * half_vy);
    half_ey = (accel_unit.z * half_vx) - (accel_unit.x * half_vz);
    half_ez = (accel_unit.x * half_vy) - (accel_unit.y * half_vx);
    g_imu_debug_info.mahony_half_error.x = half_ex;
    g_imu_debug_info.mahony_half_error.y = half_ey;
    g_imu_debug_info.mahony_half_error.z = half_ez;

    /*
     * Mahony 的积分项用于补偿长期存在的系统性零偏，
     * 比例项用于快速抑制当前姿态与重力方向之间的瞬时误差。
     */
    if (g_imu_config.mahony_ki > 0.0f) {
        g_imu_mahony_integral_error.x += g_imu_config.mahony_ki * half_ex * dt_s;
        g_imu_mahony_integral_error.y += g_imu_config.mahony_ki * half_ey * dt_s;
        g_imu_mahony_integral_error.z += g_imu_config.mahony_ki * half_ez * dt_s;
    } else {
        g_imu_mahony_integral_error.x = 0.0f;
        g_imu_mahony_integral_error.y = 0.0f;
        g_imu_mahony_integral_error.z = 0.0f;
    }

    g_imu_debug_info.mahony_integral_error = g_imu_mahony_integral_error;

    gyro_rad_s->x += g_imu_mahony_integral_error.x + (g_imu_config.mahony_kp * half_ex);
    gyro_rad_s->y += g_imu_mahony_integral_error.y + (g_imu_config.mahony_kp * half_ey);
    gyro_rad_s->z += g_imu_mahony_integral_error.z + (g_imu_config.mahony_kp * half_ez);
}

/**
  * @brief  使用当前修正后的角速度对四元数进行一步积分更新。
  * @param  gyro_rad_s：本次积分使用的角速度向量指针，单位为 rad/s。
  * @param  dt_s：时间步长，单位为秒。
  * @retval None
  */
static void imu_integrate_quaternion(const imu_vector3f_t *gyro_rad_s, float dt_s)
{
    float half_dt;
    float q0;
    float q1;
    float q2;
    float q3;

    if ((gyro_rad_s == (const imu_vector3f_t *) 0) || (dt_s <= 0.0f)) {
        return;
    }

    half_dt = 0.5f * dt_s;

    q0 = g_imu_data.quaternion.w;
    q1 = g_imu_data.quaternion.x;
    q2 = g_imu_data.quaternion.y;
    q3 = g_imu_data.quaternion.z;

    /*
     * 使用标准四元数运动学方程进行一阶积分，
     * 当前版本优先保证结构清晰和可调试性，后续若需要可替换为更高阶积分。
     */
    g_imu_data.quaternion.w += (-q1 * gyro_rad_s->x - q2 * gyro_rad_s->y - q3 * gyro_rad_s->z) * half_dt;
    g_imu_data.quaternion.x += (q0 * gyro_rad_s->x + q2 * gyro_rad_s->z - q3 * gyro_rad_s->y) * half_dt;
    g_imu_data.quaternion.y += (q0 * gyro_rad_s->y - q1 * gyro_rad_s->z + q3 * gyro_rad_s->x) * half_dt;
    g_imu_data.quaternion.z += (q0 * gyro_rad_s->z + q1 * gyro_rad_s->y - q2 * gyro_rad_s->x) * half_dt;

    imu_normalize_quaternion();
}

/**
  * @brief  初始化 IMU 算法层，并使用当前配置完成底层 ICM42688 初始化。
  * @param  None
  * @retval 返回驱动状态码。
  */
imu_status_t imu_get_default_config(imu_config_t *config)
{
    if (config == (imu_config_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    config->mahony_kp = 2.0f;
    config->mahony_ki = 0.05f;
    config->accel_norm_min_g = 0.5f;
    config->accel_norm_max_g = 1.5f;
    config->enable_online_gyro_bias_trim = 0U;
    config->static_accel_tolerance_g = 0.05f;
    config->static_gyro_threshold_dps = 0.20f;
    config->static_detect_count_threshold = 50U;
    config->online_gyro_bias_z_alpha = 0.01f;

    return IMU_STATUS_OK;
}

imu_status_t imu_set_config(const imu_config_t *config)
{
    if (imu_is_valid_config(config) == false) {
        return IMU_STATUS_ERROR_PARAM;
    }

    g_imu_config = *config;

    return IMU_STATUS_OK;
}

imu_status_t imu_get_config(imu_config_t *config)
{
    if (config == (imu_config_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    *config = g_imu_config;

    return IMU_STATUS_OK;
}

imu_status_t imu_bind_sensor(const imu_sensor_ops_t *sensor_ops)
{
    if (sensor_ops == (const imu_sensor_ops_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    if ((sensor_ops->init == (imu_sensor_init_fn_t) 0) ||
        (sensor_ops->read == (imu_sensor_read_fn_t) 0)) {
        return IMU_STATUS_ERROR_PARAM;
    }

    g_imu_sensor_ops = *sensor_ops;

    return IMU_STATUS_OK;
}

imu_status_t imu_bind_timebase(const imu_timebase_ops_t *timebase_ops)
{
    if (timebase_ops == (const imu_timebase_ops_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    if ((timebase_ops->get_time_us == (imu_timebase_get_us_fn_t) 0) &&
        (timebase_ops->get_time_ms == (imu_timebase_get_ms_fn_t) 0)) {
        return IMU_STATUS_ERROR_PARAM;
    }

    g_imu_timebase_ops = *timebase_ops;
    g_imu_last_update_time_us = 0ULL;
    g_imu_has_last_update_time = false;

    return IMU_STATUS_OK;
}

/**
  * @brief  初始化 IMU 算法层，并使用当前绑定的底层传感器接口完成器件初始化。
  * @param  None
  * @retval 返回 IMU 算法层统一状态码。
  */
imu_status_t imu_init(void)
{
    imu_status_t imu_status;

    if (imu_is_sensor_bound() == false) {
        return IMU_STATUS_ERROR_UNBOUND;
    }

    imu_status = g_imu_sensor_ops.init(g_imu_sensor_ops.user_data);
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    imu_reset();
    g_imu_is_initialized = true;
    imu_sync_auto_update_timebase();

    imu_status = imu_refresh_sensor_data();
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    (void) imu_align_quaternion_with_gravity();

    return IMU_STATUS_OK;
}

/**
  * @brief  重置 IMU 算法层状态，但不修改当前底层配置。
  * @param  None
  * @retval None
  */
void imu_reset(void)
{
    g_imu_data.temperature_c = 0.0f;

    g_imu_data.accel_g.x = 0.0f;
    g_imu_data.accel_g.y = 0.0f;
    g_imu_data.accel_g.z = 0.0f;

    g_imu_data.gyro_dps.x = 0.0f;
    g_imu_data.gyro_dps.y = 0.0f;
    g_imu_data.gyro_dps.z = 0.0f;

    g_imu_data.gyro_rad_s.x = 0.0f;
    g_imu_data.gyro_rad_s.y = 0.0f;
    g_imu_data.gyro_rad_s.z = 0.0f;

    g_imu_data.gyro_bias_dps.x = 0.0f;
    g_imu_data.gyro_bias_dps.y = 0.0f;
    g_imu_data.gyro_bias_dps.z = 0.0f;

    g_imu_data.gyro_bias_rad_s.x = 0.0f;
    g_imu_data.gyro_bias_rad_s.y = 0.0f;
    g_imu_data.gyro_bias_rad_s.z = 0.0f;

    g_imu_data.quaternion.w = 1.0f;
    g_imu_data.quaternion.x = 0.0f;
    g_imu_data.quaternion.y = 0.0f;
    g_imu_data.quaternion.z = 0.0f;

    g_imu_data.euler_deg.roll = 0.0f;
    g_imu_data.euler_deg.pitch = 0.0f;
    g_imu_data.euler_deg.yaw = 0.0f;

    g_imu_data.euler_rad.roll = 0.0f;
    g_imu_data.euler_rad.pitch = 0.0f;
    g_imu_data.euler_rad.yaw = 0.0f;

    g_imu_mahony_integral_error.x = 0.0f;
    g_imu_mahony_integral_error.y = 0.0f;
    g_imu_mahony_integral_error.z = 0.0f;

    g_imu_debug_info.accel_norm_g = 0.0f;
    g_imu_debug_info.update_dt_s = 0.0f;
    g_imu_debug_info.update_dt_ms = 0U;
    g_imu_debug_info.update_dt_us = 0U;
    g_imu_debug_info.accel_feedback_valid = false;
    g_imu_debug_info.gyro_bias_trim_enabled =
        (g_imu_config.enable_online_gyro_bias_trim != 0U);
    g_imu_debug_info.gyro_bias_trim_active = false;
    g_imu_debug_info.static_detect_count = 0U;
    g_imu_debug_info.accel_unit.x = 0.0f;
    g_imu_debug_info.accel_unit.y = 0.0f;
    g_imu_debug_info.accel_unit.z = 0.0f;
    g_imu_debug_info.gravity_estimate.x = 0.0f;
    g_imu_debug_info.gravity_estimate.y = 0.0f;
    g_imu_debug_info.gravity_estimate.z = 0.0f;
    g_imu_debug_info.gyro_bias_compensated_dps.x = 0.0f;
    g_imu_debug_info.gyro_bias_compensated_dps.y = 0.0f;
    g_imu_debug_info.gyro_bias_compensated_dps.z = 0.0f;
    g_imu_debug_info.gyro_bias_compensated_rad_s.x = 0.0f;
    g_imu_debug_info.gyro_bias_compensated_rad_s.y = 0.0f;
    g_imu_debug_info.gyro_bias_compensated_rad_s.z = 0.0f;
    g_imu_debug_info.mahony_half_error.x = 0.0f;
    g_imu_debug_info.mahony_half_error.y = 0.0f;
    g_imu_debug_info.mahony_half_error.z = 0.0f;
    g_imu_debug_info.mahony_integral_error.x = 0.0f;
    g_imu_debug_info.mahony_integral_error.y = 0.0f;
    g_imu_debug_info.mahony_integral_error.z = 0.0f;
    g_imu_debug_info.gyro_after_mahony_rad_s.x = 0.0f;
    g_imu_debug_info.gyro_after_mahony_rad_s.y = 0.0f;
    g_imu_debug_info.gyro_after_mahony_rad_s.z = 0.0f;
    g_imu_debug_info.gyro_after_mahony_dps.x = 0.0f;
    g_imu_debug_info.gyro_after_mahony_dps.y = 0.0f;
    g_imu_debug_info.gyro_after_mahony_dps.z = 0.0f;

    g_imu_is_gyro_calibrated = false;
    g_imu_last_update_time_us = 0ULL;
    g_imu_has_last_update_time = false;
    g_imu_static_detect_count = 0U;
}

/**
  * @brief  设置 IMU 算法层后续要使用的 ICM42688 初始化配置。
  * @param  init_config：初始化配置结构体指针。
  * @retval 返回驱动状态码。
  */
imu_status_t imu_calibrate_gyro_bias(uint16_t sample_count, uint32_t interval_ms)
{
    imu_status_t imu_status;
    float sum_gyro_x_dps = 0.0f;
    float sum_gyro_y_dps = 0.0f;
    float sum_gyro_z_dps = 0.0f;
    float sum_gyro_x_rad_s = 0.0f;
    float sum_gyro_y_rad_s = 0.0f;
    float sum_gyro_z_rad_s = 0.0f;
    uint16_t sample_index;

    if (sample_count == 0U) {
        return IMU_STATUS_ERROR_PARAM;
    }

    if (g_imu_is_initialized == false) {
        imu_status = imu_init();
        if (imu_status != IMU_STATUS_OK) {
            return imu_status;
        }
    }

    /*
     * 零偏校准默认假定当前传感器处于静止状态，
     * 因此这里直接对多帧角速度求平均作为后续姿态更新的偏置补偿量。
     */
    for (sample_index = 0U; sample_index < sample_count; sample_index++) {
        imu_status = imu_refresh_sensor_data();
        if (imu_status != IMU_STATUS_OK) {
            return imu_status;
        }

        sum_gyro_x_dps += g_imu_data.gyro_dps.x;
        sum_gyro_y_dps += g_imu_data.gyro_dps.y;
        sum_gyro_z_dps += g_imu_data.gyro_dps.z;

        sum_gyro_x_rad_s += g_imu_data.gyro_rad_s.x;
        sum_gyro_y_rad_s += g_imu_data.gyro_rad_s.y;
        sum_gyro_z_rad_s += g_imu_data.gyro_rad_s.z;

        if (interval_ms > 0U) {
            delay_ms(interval_ms);
        }
    }

    g_imu_data.gyro_bias_dps.x = sum_gyro_x_dps / (float) sample_count;
    g_imu_data.gyro_bias_dps.y = sum_gyro_y_dps / (float) sample_count;
    g_imu_data.gyro_bias_dps.z = sum_gyro_z_dps / (float) sample_count;

    g_imu_data.gyro_bias_rad_s.x = sum_gyro_x_rad_s / (float) sample_count;
    g_imu_data.gyro_bias_rad_s.y = sum_gyro_y_rad_s / (float) sample_count;
    g_imu_data.gyro_bias_rad_s.z = sum_gyro_z_rad_s / (float) sample_count;

    g_imu_is_gyro_calibrated = true;
    imu_sync_auto_update_timebase();

    return IMU_STATUS_OK;
}

/**
  * @brief  使用一帧新的传感器数据更新 IMU 姿态状态。
  * @param  dt_s：本次更新对应的时间步长，单位为秒。
  * @retval 返回驱动状态码。
  */
imu_status_t imu_update(float dt_s)
{
    imu_status_t imu_status;
    imu_vector3f_t gyro_rad_s;
    uint64_t stage_start_us;
    uint64_t update_start_us;
    uint64_t update_end_us;

    if (dt_s <= 0.0f) {
        return IMU_STATUS_ERROR_PARAM;
    }

    if (g_imu_is_initialized == false) {
        imu_status = imu_init();
        if (imu_status != IMU_STATUS_OK) {
            return imu_status;
        }
    }

    update_start_us = My_GetTimeUs();

    stage_start_us = update_start_us;
    imu_status = imu_refresh_sensor_data();
    g_imu_profile.sensor_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
    if (imu_status != IMU_STATUS_OK) {
        update_end_us = My_GetTimeUs();
        g_imu_profile.update_total_us = (uint32_t) (update_end_us - update_start_us);
        return imu_status;
    }

    g_imu_debug_info.update_dt_s = dt_s;
    g_imu_debug_info.update_dt_ms = (uint32_t) (dt_s * 1000.0f);
    g_imu_debug_info.update_dt_us = (uint32_t) (dt_s * 1000000.0f);

    stage_start_us = My_GetTimeUs();
    imu_get_bias_compensated_gyro(&gyro_rad_s);
    g_imu_profile.bias_comp_1_us = (uint32_t) (My_GetTimeUs() - stage_start_us);

    stage_start_us = My_GetTimeUs();
    imu_update_online_gyro_bias_trim(&gyro_rad_s);
    g_imu_profile.bias_trim_us = (uint32_t) (My_GetTimeUs() - stage_start_us);

    stage_start_us = My_GetTimeUs();
    imu_get_bias_compensated_gyro(&gyro_rad_s);
    g_imu_profile.bias_comp_2_us = (uint32_t) (My_GetTimeUs() - stage_start_us);

    stage_start_us = My_GetTimeUs();
    imu_apply_mahony_feedback(&gyro_rad_s, dt_s);
    g_imu_profile.mahony_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
    g_imu_debug_info.gyro_after_mahony_rad_s = gyro_rad_s;
    g_imu_debug_info.gyro_after_mahony_dps.x = gyro_rad_s.x * IMU_RAD_TO_DEG;
    g_imu_debug_info.gyro_after_mahony_dps.y = gyro_rad_s.y * IMU_RAD_TO_DEG;
    g_imu_debug_info.gyro_after_mahony_dps.z = gyro_rad_s.z * IMU_RAD_TO_DEG;

    stage_start_us = My_GetTimeUs();
    imu_integrate_quaternion(&gyro_rad_s, dt_s);
    g_imu_profile.integrate_us = (uint32_t) (My_GetTimeUs() - stage_start_us);

    stage_start_us = My_GetTimeUs();
    imu_update_euler_from_quaternion();
    g_imu_profile.euler_us = (uint32_t) (My_GetTimeUs() - stage_start_us);

    update_end_us = My_GetTimeUs();
    g_imu_profile.update_total_us = (uint32_t) (update_end_us - update_start_us);

    return IMU_STATUS_OK;
}

imu_status_t imu_update_auto(void)
{
    uint64_t current_time_us;
    uint64_t dt_us;
    imu_status_t imu_status;
    uint64_t auto_start_us;
    uint64_t auto_before_update_us;
    uint64_t auto_end_us;


    auto_start_us = My_GetTimeUs();
    if (imu_is_timebase_bound() == false) {
        return IMU_STATUS_ERROR_UNBOUND;
    }

    if (g_imu_timebase_ops.get_time_us != (imu_timebase_get_us_fn_t) 0) {
        current_time_us = g_imu_timebase_ops.get_time_us(g_imu_timebase_ops.user_data);
    } else {
        current_time_us =
            ((uint64_t) g_imu_timebase_ops.get_time_ms(g_imu_timebase_ops.user_data)) * 1000ULL;
    }

    if (g_imu_has_last_update_time == false) {
        g_imu_last_update_time_us = current_time_us;
        g_imu_has_last_update_time = true;
        g_imu_debug_info.update_dt_s = 0.0f;
        g_imu_debug_info.update_dt_ms = 0U;
        g_imu_debug_info.update_dt_us = 0U;
        g_imu_profile.auto_timebase_us = (uint32_t) (My_GetTimeUs() - auto_start_us);
        g_imu_profile.auto_update_call_us = 0U;
        g_imu_profile.auto_total_us = g_imu_profile.auto_timebase_us;
        return IMU_STATUS_OK;
    }

    dt_us = current_time_us - g_imu_last_update_time_us;
    if (dt_us == 0ULL) {
        g_imu_debug_info.update_dt_s = 0.0f;
        g_imu_debug_info.update_dt_ms = 0U;
        g_imu_debug_info.update_dt_us = 0U;
        g_imu_profile.auto_timebase_us = (uint32_t) (My_GetTimeUs() - auto_start_us);
        g_imu_profile.auto_update_call_us = 0U;
        g_imu_profile.auto_total_us = g_imu_profile.auto_timebase_us;
        return IMU_STATUS_OK;
    }

    g_imu_last_update_time_us = current_time_us;

    auto_before_update_us = My_GetTimeUs();
    g_imu_profile.auto_timebase_us = (uint32_t) (auto_before_update_us - auto_start_us);
    imu_status = imu_update((float) dt_us / 1000000.0f);
    auto_end_us = My_GetTimeUs();
    g_imu_profile.auto_update_call_us = (uint32_t) (auto_end_us - auto_before_update_us);
    g_imu_profile.auto_total_us = (uint32_t) (auto_end_us - auto_start_us);

    return imu_status;
}

/**
  * @brief  获取 IMU 算法层当前输出的数据快照指针。
  * @param  None
  * @retval 返回只读的 IMU 数据结构体指针。
  */
const imu_data_t *imu_get_data(void)
{
    return &g_imu_data;
}

/**
  * @brief  获取 IMU 算法层最近一次姿态更新保存的调试信息快照。
  * @param  None
  * @retval 返回只读的 IMU 调试信息结构体指针。
  */
const imu_debug_info_t *imu_get_debug_info(void)
{
    return &g_imu_debug_info;
}

const imu_profile_t *imu_get_profile(void)
{
    return &g_imu_profile;
}


/**
  * @brief  判断 IMU 算法层是否已经完成初始化。
  * @param  None
  * @retval 返回 true 表示已初始化，返回 false 表示尚未初始化。
  */
bool imu_is_initialized(void)
{
    return g_imu_is_initialized;
}

/**
  * @brief  判断 IMU 算法层是否已经完成陀螺仪零偏校准。
  * @param  None
  * @retval 返回 true 表示已完成校准，返回 false 表示尚未完成校准。
  */
bool imu_is_gyro_calibrated(void)
{
    return g_imu_is_gyro_calibrated;
}

static imu_status_t imu_legacy_convert_driver_status(icm42688_status_t driver_status)
{
    switch (driver_status) {
        case ICM42688_STATUS_OK:
            return IMU_STATUS_OK;

        case ICM42688_STATUS_ERROR_PARAM:
            return IMU_STATUS_ERROR_PARAM;

        case ICM42688_STATUS_ERROR_COMM:
        case ICM42688_STATUS_ERROR_ID:
        default:
            return IMU_STATUS_ERROR_SENSOR;
    }
}

static uint32_t imu_legacy_get_time_ms(void *user_data)
{
    (void) user_data;
    return My_GetTick();
}

static uint64_t imu_legacy_get_time_us(void *user_data)
{
    (void) user_data;
    return My_GetTimeUs();
}

static imu_status_t imu_legacy_sensor_init(void *user_data)
{
    icm42688_status_t driver_status;

    (void) user_data;

    if (g_imu_legacy_icm_config_ready == false) {
        driver_status = icm42688_get_default_init_config(&g_imu_legacy_icm_init_config);
        if (driver_status != ICM42688_STATUS_OK) {
            return imu_legacy_convert_driver_status(driver_status);
        }
        g_imu_legacy_icm_config_ready = true;
    }

    driver_status = icm42688_init_with_config(&g_imu_legacy_icm_init_config);
    return imu_legacy_convert_driver_status(driver_status);
}

static imu_status_t imu_legacy_sensor_read(
    imu_sensor_data_t *sensor_data, void *user_data)
{
    icm42688_raw_data_t raw_data;
    icm42688_scaled_data_t scaled_data;
    icm42688_status_t driver_status;

    (void) user_data;

    if (sensor_data == (imu_sensor_data_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    driver_status = icm42688_read_raw_data(&raw_data);
    if (driver_status != ICM42688_STATUS_OK) {
        return imu_legacy_convert_driver_status(driver_status);
    }

    driver_status = icm42688_get_current_config(&g_imu_legacy_icm_init_config);
    if (driver_status != ICM42688_STATUS_OK) {
        return imu_legacy_convert_driver_status(driver_status);
    }

    driver_status =
        icm42688_convert_raw_data(&g_imu_legacy_icm_init_config, &raw_data, &scaled_data);
    if (driver_status != ICM42688_STATUS_OK) {
        return imu_legacy_convert_driver_status(driver_status);
    }

    sensor_data->temperature_c = scaled_data.temperature_c;
    sensor_data->accel_g.x = scaled_data.accel_x_g;
    sensor_data->accel_g.y = scaled_data.accel_y_g;
    sensor_data->accel_g.z = scaled_data.accel_z_g;
    sensor_data->gyro_dps.x = scaled_data.gyro_x_dps;
    sensor_data->gyro_dps.y = scaled_data.gyro_y_dps;
    sensor_data->gyro_dps.z = scaled_data.gyro_z_dps;
    sensor_data->gyro_rad_s.x = scaled_data.gyro_x_rad_s;
    sensor_data->gyro_rad_s.y = scaled_data.gyro_y_rad_s;
    sensor_data->gyro_rad_s.z = scaled_data.gyro_z_rad_s;

    return IMU_STATUS_OK;
}

static void imu_legacy_refresh_debug_cache(void)
{
    const imu_data_t *imuData = imu_get_data();
    const imu_debug_info_t *debugInfo = imu_get_debug_info();

    if (imuData == (const imu_data_t *) 0) {
        return;
    }

    g_imu_debug_accel[0] = imuData->accel_g.x;
    g_imu_debug_accel[1] = imuData->accel_g.y;
    g_imu_debug_accel[2] = imuData->accel_g.z;

    g_imu_debug_gyro[0] = imuData->gyro_dps.x;
    g_imu_debug_gyro[1] = imuData->gyro_dps.y;
    g_imu_debug_gyro[2] = imuData->gyro_dps.z;

    g_imu_debug_gyro_offset[0] = imuData->gyro_bias_dps.x;
    g_imu_debug_gyro_offset[1] = imuData->gyro_bias_dps.y;
    g_imu_debug_gyro_offset[2] = imuData->gyro_bias_dps.z;

    g_imu_debug_q[0] = imuData->quaternion.w;
    g_imu_debug_q[1] = imuData->quaternion.x;
    g_imu_debug_q[2] = imuData->quaternion.y;
    g_imu_debug_q[3] = imuData->quaternion.z;

    g_imu_debug_ypr[0] = imuData->euler_deg.yaw;
    g_imu_debug_ypr[1] = imuData->euler_deg.pitch;
    g_imu_debug_ypr[2] = imuData->euler_deg.roll;
    yaw[0] = g_imu_debug_ypr[0];

    if (debugInfo != (const imu_debug_info_t *) 0) {
        north.x = debugInfo->gravity_estimate.x;
        north.y = debugInfo->gravity_estimate.y;
        north.z = debugInfo->gravity_estimate.z;
        west.x = debugInfo->accel_unit.x;
        west.y = debugInfo->accel_unit.y;
        west.z = debugInfo->accel_unit.z;
    }
}

void IMU_init(void)
{
    imu_sensor_ops_t sensor_ops;
    imu_timebase_ops_t timebase_ops;
    imu_config_t default_config;
    imu_status_t imu_status;
    icm42688_status_t driver_status;

    driver_status = icm42688_get_default_init_config(&g_imu_legacy_icm_init_config);
    if (driver_status != ICM42688_STATUS_OK) {
        g_imu_debug_init_ok = 0U;
        g_imu_debug_comm_ok = 0U;
        return;
    }

    g_imu_legacy_icm_config_ready = true;

    sensor_ops.init = imu_legacy_sensor_init;
    sensor_ops.read = imu_legacy_sensor_read;
    sensor_ops.user_data = (void *) 0;
    timebase_ops.get_time_ms = imu_legacy_get_time_ms;
    timebase_ops.get_time_us = imu_legacy_get_time_us;
    timebase_ops.user_data = (void *) 0;

    imu_reset();
    (void) imu_bind_sensor(&sensor_ops);
    (void) imu_bind_timebase(&timebase_ops);

    imu_status = imu_get_default_config(&default_config);
    if (imu_status != IMU_STATUS_OK) {
        g_imu_debug_init_ok = 0U;
        g_imu_debug_comm_ok = 0U;
        return;
    }

    imu_status = imu_set_config(&default_config);
    if (imu_status != IMU_STATUS_OK) {
        g_imu_debug_init_ok = 0U;
        g_imu_debug_comm_ok = 0U;
        return;
    }

    imu_status = imu_init();
    if (imu_status != IMU_STATUS_OK) {
        g_imu_debug_init_ok = 0U;
        g_imu_debug_comm_ok = 0U;
        return;
    }

    imu_status =
        imu_calibrate_gyro_bias(IMU_LEGACY_BIAS_SAMPLE_COUNT, IMU_LEGACY_BIAS_INTERVAL_MS);
    if (imu_status != IMU_STATUS_OK) {
        g_imu_debug_init_ok = 0U;
        g_imu_debug_comm_ok = 0U;
        return;
    }

    g_imu_debug_init_ok = 1U;
    g_imu_debug_comm_ok = 1U;
    g_imu_debug_sample_count = 0U;
    imu_legacy_refresh_debug_cache();
}

uint8_t IMU_IsReady(void)
{
    return (uint8_t) ((g_imu_debug_init_ok != 0U) && (g_imu_debug_comm_ok != 0U));
}

/*
 * 获取 yaw, pitch, roll 角度
 */
void IMU_getYawPitchRoll(float *angles)
{
    imu_status_t imu_status;
    const imu_data_t *imuData;

    if (angles == (float *) 0) { // 参数检查：确保传入的指针有效
        return;
    }

    imu_status = imu_update_auto(); // imu_update_auto 内部会检查是否已初始化，并在必要时进行初始化
    if (imu_status != IMU_STATUS_OK) { // 更新失败，可能是因为传感器通信问题或其他错误
        g_imu_debug_comm_ok = 0U; // 标记通信异常
        angles[0] = g_imu_debug_ypr[0];
        angles[1] = g_imu_debug_ypr[1];
        angles[2] = g_imu_debug_ypr[2];
        return;
    }

    g_imu_debug_comm_ok = 1U;
    imuData = imu_get_data(); // 获取最新的 IMU 数据快照
    if (imuData == (const imu_data_t *) 0) {
        // 获取数据失败，可能是因为内部状态异常或其他不可预见的错误，保持通信异常状态并返回上次的角度数据
        angles[0] = g_imu_debug_ypr[0];
        angles[1] = g_imu_debug_ypr[1];
        angles[2] = g_imu_debug_ypr[2];
        return;
    }

    // 成功获取数据，更新角度输出并刷新调试缓存
    angles[0] = imuData->euler_deg.yaw;
    angles[1] = imuData->euler_deg.pitch;
    angles[2] = imuData->euler_deg.roll;
    
    // for debug
    // 刷新调试缓存以确保后续调用 IMU_TT_getgyro 时能够获取到最新的传感器数据快照
    imu_legacy_refresh_debug_cache();
    // 增加样本计数，便于调试和性能分析
    g_imu_debug_sample_count++;
}

void IMU_TT_getgyro(float *zsjganda)
{
    const imu_data_t *imuData = imu_get_data();

    if ((zsjganda == (float *) 0) || (imuData == (const imu_data_t *) 0)) {
        return;
    }

    zsjganda[0] = imuData->accel_g.x;
    zsjganda[1] = imuData->accel_g.y;
    zsjganda[2] = imuData->accel_g.z;
    zsjganda[3] = imuData->gyro_dps.x;
    zsjganda[4] = imuData->gyro_dps.y;
    zsjganda[5] = imuData->gyro_dps.z;
    zsjganda[6] = imuData->temperature_c;
}

void MPU6050_InitAng_Offset(void)
{
}
