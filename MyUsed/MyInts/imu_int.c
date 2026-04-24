#include "imu_int.h"

#include "my_system_lib.h"

/* IMU 接口层内部保存的 ICM42688 初始化配置，用于适配层初始化具体器件。*/
static icm42688_init_config_t g_imu_int_icm42688_init_config;
static imu_config_t g_imu_int_imu_config;
static imu_int_profile_t g_imu_int_profile;

/**
  * @brief  IMU 接口层默认使用的毫秒时间基准获取函数，统一复用系统节拍计数。
  * @param  user_data：用户私有上下文指针，当前未使用。
  * @retval 返回当前毫秒时间戳。
  */
static uint32_t imu_int_get_system_time_ms(void *user_data)
{
    (void) user_data;

    return My_GetTick();
}

static uint64_t imu_int_get_system_time_us(void *user_data)
{
    (void) user_data;

    return My_GetTimeUs();
}

/**
  * @brief  将 ICM42688 驱动状态码转换为 IMU 算法层统一状态码。
  * @param  driver_status：待转换的 ICM42688 驱动状态码。
  * @retval 返回 IMU 算法层统一状态码。
  */
static imu_status_t imu_int_convert_driver_status(icm42688_status_t driver_status)
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

/**
  * @brief  IMU 层绑定用的 ICM42688 初始化适配函数。
  * @param  user_data：用户私有上下文指针，当前未使用。
  * @retval 返回 IMU 算法层统一状态码。
  */
static imu_status_t imu_int_icm42688_sensor_init(void *user_data)
{
    icm42688_status_t driver_status;

    (void) user_data;

    if (g_imu_int_icm42688_init_config.gyro_odr == 0U) {
        driver_status = icm42688_get_default_init_config(&g_imu_int_icm42688_init_config);
        if (driver_status != ICM42688_STATUS_OK) {
            return imu_int_convert_driver_status(driver_status);
        }
    }

    driver_status = icm42688_init_with_config(&g_imu_int_icm42688_init_config);

    return imu_int_convert_driver_status(driver_status);
}

/**
  * @brief  IMU 层绑定用的 ICM42688 读数适配函数，把器件驱动输出转换成 IMU 标准物理量格式。
  * @param  sensor_data：标准化传感器物理量输出结构体指针。
  * @param  user_data：用户私有上下文指针，当前未使用。
  * @retval 返回 IMU 算法层统一状态码。
  */
static imu_status_t imu_int_icm42688_sensor_read(imu_sensor_data_t *sensor_data, void *user_data)
{
    icm42688_raw_data_t raw_data;
    icm42688_scaled_data_t scaled_data;
    icm42688_status_t driver_status;
    uint64_t total_start_us;
    uint64_t stage_start_us;

    (void) user_data;

    if (sensor_data == (imu_sensor_data_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }


    total_start_us = My_GetTimeUs();

    stage_start_us = total_start_us;
    driver_status = icm42688_read_raw_data(&raw_data);
    g_imu_int_profile.raw_read_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
    if (driver_status != ICM42688_STATUS_OK) {
        g_imu_int_profile.total_us = (uint32_t) (My_GetTimeUs() - total_start_us);
        return imu_int_convert_driver_status(driver_status);
    }

    stage_start_us = My_GetTimeUs();
    driver_status = icm42688_convert_raw_data(&g_imu_int_icm42688_init_config, &raw_data, &scaled_data);
    g_imu_int_profile.convert_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
    if (driver_status != ICM42688_STATUS_OK) {
        g_imu_int_profile.total_us = (uint32_t) (My_GetTimeUs() - total_start_us);
        return imu_int_convert_driver_status(driver_status);
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
    g_imu_int_profile.total_us = (uint32_t) (My_GetTimeUs() - total_start_us);

    return IMU_STATUS_OK;
}

/**
  * @brief  绑定 IMU 算法层当前使用的 ICM42688 适配接口。
  * @param  None
  * @retval 返回 IMU 算法层统一状态码。
  */
static imu_status_t imu_int_bind_icm42688_sensor(void)
{
    imu_sensor_ops_t sensor_ops;

    sensor_ops.init = imu_int_icm42688_sensor_init;
    sensor_ops.read = imu_int_icm42688_sensor_read;
    sensor_ops.user_data = (void *) 0;

    return imu_bind_sensor(&sensor_ops);
}

/**
  * @brief  绑定 IMU 算法层默认使用的系统毫秒时间基准。
  * @param  None
  * @retval 返回 IMU 算法层统一状态码。
  */
static imu_status_t imu_int_bind_timebase(void)
{
    imu_timebase_ops_t timebase_ops;

    timebase_ops.get_time_ms = imu_int_get_system_time_ms;
    timebase_ops.get_time_us = imu_int_get_system_time_us;
    timebase_ops.user_data = (void *) 0;

    return imu_bind_timebase(&timebase_ops);
}

imu_status_t imu_int_set_icm42688_init_config(const icm42688_init_config_t *init_config)
{
    if (init_config == (const icm42688_init_config_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    g_imu_int_icm42688_init_config = *init_config;

    return IMU_STATUS_OK;
}

imu_status_t imu_int_get_icm42688_init_config(icm42688_init_config_t *init_config)
{
    if (init_config == (icm42688_init_config_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    *init_config = g_imu_int_icm42688_init_config;

    return IMU_STATUS_OK;
}

imu_status_t imu_int_set_imu_config(const imu_config_t *imu_config)
{
    imu_status_t imu_status;

    if (imu_config == (const imu_config_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    imu_status = imu_set_config(imu_config);
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    g_imu_int_imu_config = *imu_config;

    return IMU_STATUS_OK;
}

imu_status_t imu_int_get_imu_config(imu_config_t *imu_config)
{
    if (imu_config == (imu_config_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    return imu_get_config(imu_config);
}

imu_status_t imu_int_init(uint16_t bias_sample_count, uint32_t bias_interval_ms)
{
    imu_status_t imu_status;

    imu_status = imu_int_bind_icm42688_sensor();
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    imu_status = imu_int_bind_timebase();
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    if ((g_imu_int_imu_config.accel_norm_max_g == 0.0f) &&
        (g_imu_int_imu_config.accel_norm_min_g == 0.0f)) {
        imu_status = imu_get_default_config(&g_imu_int_imu_config);
        if (imu_status != IMU_STATUS_OK) {
            return imu_status;
        }
    }

    imu_status = imu_set_config(&g_imu_int_imu_config);
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    imu_status = imu_init();
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    return imu_calibrate_gyro_bias(bias_sample_count, bias_interval_ms);
}

const imu_int_profile_t *imu_int_get_profile(void)
{
    return &g_imu_int_profile;
}

