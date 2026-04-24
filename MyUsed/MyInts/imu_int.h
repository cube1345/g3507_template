#ifndef __IMU_INT_H
#define __IMU_INT_H

#include <stdint.h>

#include "IMU.h"
#include "icm42688_driver.h"

typedef struct
{
    uint32_t raw_read_us;
    uint32_t convert_us;
    uint32_t total_us;
} imu_int_profile_t;

imu_status_t imu_int_set_icm42688_init_config(const icm42688_init_config_t *init_config);
imu_status_t imu_int_get_icm42688_init_config(icm42688_init_config_t *init_config);
imu_status_t imu_int_set_imu_config(const imu_config_t *imu_config);
imu_status_t imu_int_get_imu_config(imu_config_t *imu_config);
imu_status_t imu_int_init(uint16_t bias_sample_count, uint32_t bias_interval_ms);
const imu_int_profile_t *imu_int_get_profile(void);

#endif
