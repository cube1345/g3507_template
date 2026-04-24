#ifndef __IMU_APP_H
#define __IMU_APP_H

#include <stdbool.h>
#include <stdint.h>

#include "IMU.h"
#include "icm42688_driver.h"

/**
  * @brief  IMU application-layer unified configuration.
  * @note   This layer is intended for bare-metal loops or FreeRTOS tasks.
  */
typedef struct
{
    icm42688_init_config_t icm42688_init_config;
    imu_config_t imu_config;
    uint16_t bias_sample_count;
    uint32_t bias_interval_ms;
} imu_app_config_t;

/**
  * @brief  Get default IMU application-layer configuration.
  * @param  app_config Default configuration output pointer.
  * @retval Returns IMU status code.
  */
imu_status_t imu_app_get_default_config(imu_app_config_t *app_config);

/**
  * @brief  Save current IMU application-layer configuration.
  * @param  app_config Configuration input pointer.
  * @retval Returns IMU status code.
  */
imu_status_t imu_app_set_config(const imu_app_config_t *app_config);

/**
  * @brief  Get current IMU application-layer configuration.
  * @param  app_config Current configuration output pointer.
  * @retval Returns IMU status code.
  */
imu_status_t imu_app_get_config(imu_app_config_t *app_config);

/**
  * @brief  Initialize IMU application layer using current saved configuration.
  * @param  None
  * @retval Returns IMU status code.
  */
imu_status_t imu_app_init(void);

/**
  * @brief  Initialize IMU application layer using explicit configuration.
  * @param  app_config Configuration input pointer.
  * @retval Returns IMU status code.
  */
imu_status_t imu_app_init_with_config(const imu_app_config_t *app_config);

/**
  * @brief  Update IMU attitude once using the bound automatic timebase.
  * @param  None
  * @retval Returns IMU status code.
  */
imu_status_t imu_app_update(void);

/**
  * @brief  Recalibrate gyro bias using current saved calibration parameters.
  * @param  None
  * @retval Returns IMU status code.
  */
imu_status_t imu_app_recalibrate(void);

/**
  * @brief  Get current IMU full data snapshot pointer.
  * @param  None
  * @retval Returns read-only IMU data pointer.
  */
const imu_data_t *imu_app_get_data(void);

/**
  * @brief  Copy current IMU full data snapshot to user buffer.
  * @param  imu_data IMU data snapshot output pointer.
  * @retval Returns IMU status code.
  */
imu_status_t imu_app_get_data_snapshot(imu_data_t *imu_data);

/**
  * @brief  Get current IMU debug snapshot pointer.
  * @param  None
  * @retval Returns read-only IMU debug pointer.
  */
const imu_debug_info_t *imu_app_get_debug_info(void);

/**
  * @brief  Copy current IMU debug snapshot to user buffer.
  * @param  debug_info IMU debug snapshot output pointer.
  * @retval Returns IMU status code.
  */
imu_status_t imu_app_get_debug_snapshot(imu_debug_info_t *debug_info);

/**
  * @brief  Get current Euler angle output in degrees.
  * @param  euler_deg Euler angle output pointer.
  * @retval Returns IMU status code.
  */
imu_status_t imu_app_get_euler_deg(imu_euler_t *euler_deg);

/**
  * @brief  Get current Euler angle output in radians.
  * @param  euler_rad Euler angle output pointer.
  * @retval Returns IMU status code.
  */
imu_status_t imu_app_get_euler_rad(imu_euler_t *euler_rad);

/**
  * @brief  Get current quaternion output.
  * @param  quaternion Quaternion output pointer.
  * @retval Returns IMU status code.
  */
imu_status_t imu_app_get_quaternion(imu_quaternion_t *quaternion);

/**
  * @brief  Check whether IMU application layer has completed initialization.
  * @param  None
  * @retval Returns true when IMU app layer is ready.
  */
bool imu_app_is_ready(void);

#endif
