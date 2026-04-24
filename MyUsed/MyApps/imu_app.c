#include "imu_app.h"

#include "imu_int.h"

/* Default gyro bias calibration sample count used by the application layer. */
#define IMU_APP_DEFAULT_BIAS_SAMPLE_COUNT    (200U)

/* Default interval between neighboring bias calibration samples, unit: ms. */
#define IMU_APP_DEFAULT_BIAS_INTERVAL_MS     (5U)

/* Current IMU application-layer runtime configuration. */
static imu_app_config_t g_imu_app_config;

/* Marks whether the application-layer configuration has been prepared. */
static bool g_imu_app_config_ready = false;

/* Marks whether the IMU application layer has completed initialization. */
static bool g_imu_app_is_ready = false;

/**
  * @brief  Ensure application layer always has a valid configuration snapshot.
  * @retval Returns IMU status code.
  */
static imu_status_t imu_app_prepare_config(void)
{
    imu_status_t imu_status;

    if (g_imu_app_config_ready == true) {
        return IMU_STATUS_OK;
    }

    imu_status = imu_app_get_default_config(&g_imu_app_config);
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    g_imu_app_config_ready = true;

    return IMU_STATUS_OK;
}

imu_status_t imu_app_get_default_config(imu_app_config_t *app_config)
{
    icm42688_status_t driver_status;
    imu_status_t imu_status;

    if (app_config == (imu_app_config_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    driver_status = icm42688_get_default_init_config(&app_config->icm42688_init_config);
    if (driver_status != ICM42688_STATUS_OK) {
        return IMU_STATUS_ERROR_SENSOR;
    }

    imu_status = imu_get_default_config(&app_config->imu_config);
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    app_config->bias_sample_count = IMU_APP_DEFAULT_BIAS_SAMPLE_COUNT;
    app_config->bias_interval_ms = IMU_APP_DEFAULT_BIAS_INTERVAL_MS;

    return IMU_STATUS_OK;
}

imu_status_t imu_app_set_config(const imu_app_config_t *app_config)
{
    if (app_config == (const imu_app_config_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    if (app_config->bias_sample_count == 0U) {
        return IMU_STATUS_ERROR_PARAM;
    }

    g_imu_app_config = *app_config;
    g_imu_app_config_ready = true;
    g_imu_app_is_ready = false;

    return IMU_STATUS_OK;
}

imu_status_t imu_app_get_config(imu_app_config_t *app_config)
{
    imu_status_t imu_status;

    if (app_config == (imu_app_config_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    imu_status = imu_app_prepare_config();
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    *app_config = g_imu_app_config;

    return IMU_STATUS_OK;
}

imu_status_t imu_app_init(void)
{
    imu_status_t imu_status;

    imu_status = imu_app_prepare_config();
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    imu_status = imu_int_set_icm42688_init_config(&g_imu_app_config.icm42688_init_config);
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    imu_status = imu_int_set_imu_config(&g_imu_app_config.imu_config);
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    imu_status = imu_int_init(
        g_imu_app_config.bias_sample_count,
        g_imu_app_config.bias_interval_ms);
    if (imu_status != IMU_STATUS_OK) {
        g_imu_app_is_ready = false;
        return imu_status;
    }

    g_imu_app_is_ready = true;

    return IMU_STATUS_OK;
}

imu_status_t imu_app_init_with_config(const imu_app_config_t *app_config)
{
    imu_status_t imu_status;

    imu_status = imu_app_set_config(app_config);
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    return imu_app_init();
}

imu_status_t imu_app_update(void)
{
    if (g_imu_app_is_ready == false) {
        return IMU_STATUS_ERROR_UNBOUND;
    }

    return imu_update_auto();
}

imu_status_t imu_app_recalibrate(void)
{
    imu_status_t imu_status;

    imu_status = imu_app_prepare_config();
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    if (imu_is_initialized() == false) {
        return imu_app_init();
    }

    imu_status = imu_calibrate_gyro_bias(
        g_imu_app_config.bias_sample_count,
        g_imu_app_config.bias_interval_ms);
    if (imu_status != IMU_STATUS_OK) {
        return imu_status;
    }

    g_imu_app_is_ready = true;

    return IMU_STATUS_OK;
}

const imu_data_t *imu_app_get_data(void)
{
    return imu_get_data();
}

imu_status_t imu_app_get_data_snapshot(imu_data_t *imu_data)
{
    const imu_data_t *imu_data_ptr;

    if (imu_data == (imu_data_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    if (g_imu_app_is_ready == false) {
        return IMU_STATUS_ERROR_UNBOUND;
    }

    imu_data_ptr = imu_get_data();
    *imu_data = *imu_data_ptr;

    return IMU_STATUS_OK;
}

const imu_debug_info_t *imu_app_get_debug_info(void)
{
    return imu_get_debug_info();
}

imu_status_t imu_app_get_debug_snapshot(imu_debug_info_t *debug_info)
{
    const imu_debug_info_t *debug_info_ptr;

    if (debug_info == (imu_debug_info_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    if (g_imu_app_is_ready == false) {
        return IMU_STATUS_ERROR_UNBOUND;
    }

    debug_info_ptr = imu_get_debug_info();
    *debug_info = *debug_info_ptr;

    return IMU_STATUS_OK;
}

imu_status_t imu_app_get_euler_deg(imu_euler_t *euler_deg)
{
    const imu_data_t *imu_data;

    if (euler_deg == (imu_euler_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    if (g_imu_app_is_ready == false) {
        return IMU_STATUS_ERROR_UNBOUND;
    }

    imu_data = imu_get_data();
    *euler_deg = imu_data->euler_deg;

    return IMU_STATUS_OK;
}

imu_status_t imu_app_get_euler_rad(imu_euler_t *euler_rad)
{
    const imu_data_t *imu_data;

    if (euler_rad == (imu_euler_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    if (g_imu_app_is_ready == false) {
        return IMU_STATUS_ERROR_UNBOUND;
    }

    imu_data = imu_get_data();
    *euler_rad = imu_data->euler_rad;

    return IMU_STATUS_OK;
}

imu_status_t imu_app_get_quaternion(imu_quaternion_t *quaternion)
{
    const imu_data_t *imu_data;

    if (quaternion == (imu_quaternion_t *) 0) {
        return IMU_STATUS_ERROR_PARAM;
    }

    if (g_imu_app_is_ready == false) {
        return IMU_STATUS_ERROR_UNBOUND;
    }

    imu_data = imu_get_data();
    *quaternion = imu_data->quaternion;

    return IMU_STATUS_OK;
}

bool imu_app_is_ready(void)
{
    return (g_imu_app_is_ready && imu_is_initialized() && imu_is_gyro_calibrated());
}
