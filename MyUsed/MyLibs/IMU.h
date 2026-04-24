#ifndef __IMU_H
#define __IMU_H

#include <stdbool.h>
#include <stdint.h>

/* Radian-to-degree conversion factor, unit: deg/rad. */
#define IMU_RAD_TO_DEG                       (57.2957795130823208768f)

typedef struct
{
    float x;
    float y;
    float z;
} imu_vector3f_t;

typedef struct
{
    float w;
    float x;
    float y;
    float z;
} imu_quaternion_t;

typedef struct
{
    float roll;
    float pitch;
    float yaw;
} imu_euler_t;

typedef enum
{
    IMU_STATUS_OK = 0U,
    IMU_STATUS_ERROR_PARAM,
    IMU_STATUS_ERROR_SENSOR,
    IMU_STATUS_ERROR_UNBOUND
} imu_status_t;

typedef struct
{
    float temperature_c;
    imu_vector3f_t accel_g;
    imu_vector3f_t gyro_dps;
    imu_vector3f_t gyro_rad_s;
} imu_sensor_data_t;

typedef struct
{
    float temperature_c;
    imu_vector3f_t accel_g;
    imu_vector3f_t gyro_dps;
    imu_vector3f_t gyro_rad_s;
    imu_vector3f_t gyro_bias_dps;
    imu_vector3f_t gyro_bias_rad_s;
    imu_quaternion_t quaternion;
    imu_euler_t euler_deg;
    imu_euler_t euler_rad;
} imu_data_t;

typedef struct
{
    float mahony_kp;
    float mahony_ki;
    float accel_norm_min_g;
    float accel_norm_max_g;
    uint8_t enable_online_gyro_bias_trim;
    float static_accel_tolerance_g;
    float static_gyro_threshold_dps;
    uint16_t static_detect_count_threshold;
    float online_gyro_bias_z_alpha;
} imu_config_t;

typedef imu_status_t (*imu_sensor_init_fn_t)(void *user_data);
typedef imu_status_t (*imu_sensor_read_fn_t)(imu_sensor_data_t *sensor_data, void *user_data);

typedef struct
{
    imu_sensor_init_fn_t init;
    imu_sensor_read_fn_t read;
    void *user_data;
} imu_sensor_ops_t;

typedef uint32_t (*imu_timebase_get_ms_fn_t)(void *user_data);
typedef uint64_t (*imu_timebase_get_us_fn_t)(void *user_data);

typedef struct
{
    imu_timebase_get_ms_fn_t get_time_ms;
    imu_timebase_get_us_fn_t get_time_us;
    void *user_data;
} imu_timebase_ops_t;

typedef struct
{
    float accel_norm_g;
    float update_dt_s;
    uint32_t update_dt_ms;
    uint32_t update_dt_us;
    bool accel_feedback_valid;
    bool gyro_bias_trim_enabled;
    bool gyro_bias_trim_active;
    uint16_t static_detect_count;
    imu_vector3f_t accel_unit;
    imu_vector3f_t gravity_estimate;
    imu_vector3f_t gyro_bias_compensated_dps;
    imu_vector3f_t gyro_bias_compensated_rad_s;
    imu_vector3f_t mahony_half_error;
    imu_vector3f_t mahony_integral_error;
    imu_vector3f_t gyro_after_mahony_rad_s;
    imu_vector3f_t gyro_after_mahony_dps;
} imu_debug_info_t;

typedef struct
{
    uint32_t auto_timebase_us;
    uint32_t auto_update_call_us;
    uint32_t auto_total_us;
    uint32_t sensor_us;
    uint32_t bias_comp_1_us;
    uint32_t bias_trim_us;
    uint32_t bias_comp_2_us;
    uint32_t mahony_us;
    uint32_t integrate_us;
    uint32_t euler_us;
    uint32_t update_total_us;
} imu_profile_t;

typedef struct
{
    float x;
    float y;
    float z;
} xyz_f_t;

extern xyz_f_t north;
extern xyz_f_t west;
extern volatile float yaw[5];
extern volatile uint8_t g_imu_debug_init_ok;
extern volatile uint8_t g_imu_debug_comm_ok;
extern volatile uint32_t g_imu_debug_sample_count;
extern volatile float g_imu_debug_accel[3];
extern volatile float g_imu_debug_gyro[3];
extern volatile float g_imu_debug_gyro_offset[3];
extern volatile float g_imu_debug_q[4];
extern volatile float g_imu_debug_ypr[3];

imu_status_t imu_bind_sensor(const imu_sensor_ops_t *sensor_ops);
imu_status_t imu_bind_timebase(const imu_timebase_ops_t *timebase_ops);
imu_status_t imu_get_default_config(imu_config_t *config);
imu_status_t imu_set_config(const imu_config_t *config);
imu_status_t imu_get_config(imu_config_t *config);
imu_status_t imu_init(void);
void imu_reset(void);
imu_status_t imu_calibrate_gyro_bias(uint16_t sample_count, uint32_t interval_ms);
imu_status_t imu_update(float dt_s);
imu_status_t imu_update_auto(void);
const imu_data_t *imu_get_data(void);
const imu_debug_info_t *imu_get_debug_info(void);
const imu_profile_t *imu_get_profile(void);
bool imu_is_initialized(void);
bool imu_is_gyro_calibrated(void);

void IMU_init(void);
uint8_t IMU_IsReady(void);
void IMU_getYawPitchRoll(float *ypr);
void IMU_TT_getgyro(float *zsjganda);
void MPU6050_InitAng_Offset(void);

#endif
