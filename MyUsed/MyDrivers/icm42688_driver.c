#include "icm42688_driver.h"

#include "my_system_lib.h"

/* 驱动层缓存的当前初始化配置，用于运行时查询和原始数据换算。 */
static icm42688_init_config_t g_icm42688_current_config;

/* 当前配置缓存是否已经具备有效内容。 */
static bool g_icm42688_has_current_config = false;

volatile uint8_t g_icm_debug_last_who_am_i = 0U;
volatile int32_t g_icm_debug_init_result = -2;
volatile uint8_t g_icm_debug_last_read_status = 0xFFU;
volatile uint8_t g_icm_debug_last_write_status = 0xFFU;
volatile uint8_t g_icm_debug_last_reg = 0U;
volatile uint8_t g_icm_debug_last_len = 0U;
volatile uint32_t g_icm_debug_read_count = 0U;
volatile uint32_t g_icm_debug_write_count = 0U;
volatile int32_t g_icm_debug_last_io_result = -2;

static int8_t icm42688_status_to_legacy_ret(icm42688_status_t driver_status)
{
    switch (driver_status) {
        case ICM42688_STATUS_OK:
            return 0;

        case ICM42688_STATUS_ERROR_ID:
            return -1;

        case ICM42688_STATUS_ERROR_PARAM:
            return -2;

        case ICM42688_STATUS_ERROR_COMM:
        default:
            return -3;
    }
}

/**
  * @brief  将底层 BSP IIC 状态码转换为 ICM42688 驱动层状态码。
  * @param  int_status：接口绑定层返回的底层状态码。
  * @retval 返回驱动层统一状态码。
  */
static icm42688_status_t icm42688_convert_int_status(bsp_iic_status_t int_status)
{
    if (int_status == BSP_IIC_STATUS_OK) {
        return ICM42688_STATUS_OK;
    }

    if (int_status == BSP_IIC_STATUS_ERROR_PARAM) {
        return ICM42688_STATUS_ERROR_PARAM;
    }

    return ICM42688_STATUS_ERROR_COMM;
}

/**
  * @brief  将连续读取出的两个高低字节拼接为一个有符号 16 位原始值。
  * @param  high_byte：高字节数据。
  * @param  low_byte：低字节数据。
  * @retval 返回拼接后的有符号 16 位数据。
  */
static int16_t icm42688_merge_bytes(uint8_t high_byte, uint8_t low_byte)
{
    return (int16_t) (((uint16_t) high_byte << 8) | (uint16_t) low_byte);
}

/**
  * @brief  将陀螺仪量程与输出数据率枚举拼接为 GYRO_CONFIG0 寄存器配置值。
  * @param  gyro_fs：陀螺仪量程枚举值。
  * @param  gyro_odr：陀螺仪输出数据率枚举值。
  * @retval 返回 GYRO_CONFIG0 寄存器配置值。
  */
static uint8_t icm42688_build_gyro_config0(
    icm42688_gyro_fs_t gyro_fs, icm42688_gyro_odr_t gyro_odr)
{
    return (uint8_t) ((((uint8_t) gyro_fs) << 5) | ((uint8_t) gyro_odr & 0x0FU));
}

/**
  * @brief  将加速度计量程与输出数据率枚举拼接为 ACCEL_CONFIG0 寄存器配置值。
  * @param  accel_fs：加速度计量程枚举值。
  * @param  accel_odr：加速度计输出数据率枚举值。
  * @retval 返回 ACCEL_CONFIG0 寄存器配置值。
  */
static uint8_t icm42688_build_accel_config0(
    icm42688_accel_fs_t accel_fs, icm42688_accel_odr_t accel_odr)
{
    return (uint8_t) ((((uint8_t) accel_fs) << 5) | ((uint8_t) accel_odr & 0x0FU));
}

/**
  * @brief  判断陀螺仪量程枚举值是否合法。
  * @param  gyro_fs 陀螺仪量程枚举值。
  * @retval 返回 true 表示合法，返回 false 表示非法。
  */
static bool icm42688_is_valid_gyro_fs(icm42688_gyro_fs_t gyro_fs)
{
    return ((uint8_t) gyro_fs <= (uint8_t) ICM42688_GYRO_FS_15DPS625);
}

/**
  * @brief  判断加速度计量程枚举值是否合法。
  * @param  accel_fs 加速度计量程枚举值。
  * @retval 返回 true 表示合法，返回 false 表示非法。
  */
static bool icm42688_is_valid_accel_fs(icm42688_accel_fs_t accel_fs)
{
    return ((uint8_t) accel_fs <= (uint8_t) ICM42688_ACCEL_FS_2G);
}

/**
  * @brief  判断陀螺仪 ODR 枚举值是否合法。
  * @param  gyro_odr 陀螺仪 ODR 枚举值。
  * @retval 返回 true 表示合法，返回 false 表示非法。
  */
static bool icm42688_is_valid_gyro_odr(icm42688_gyro_odr_t gyro_odr)
{
    switch (gyro_odr) {
        case ICM42688_GYRO_ODR_32KHZ:
        case ICM42688_GYRO_ODR_16KHZ:
        case ICM42688_GYRO_ODR_8KHZ:
        case ICM42688_GYRO_ODR_4KHZ:
        case ICM42688_GYRO_ODR_2KHZ:
        case ICM42688_GYRO_ODR_1KHZ:
        case ICM42688_GYRO_ODR_200HZ:
        case ICM42688_GYRO_ODR_100HZ:
        case ICM42688_GYRO_ODR_50HZ:
        case ICM42688_GYRO_ODR_25HZ:
        case ICM42688_GYRO_ODR_12HZ5:
        case ICM42688_GYRO_ODR_500HZ:
            return true;

        default:
            return false;
    }
}

/**
  * @brief  判断加速度计 ODR 枚举值是否合法。
  * @param  accel_odr 加速度计 ODR 枚举值。
  * @retval 返回 true 表示合法，返回 false 表示非法。
  */
static bool icm42688_is_valid_accel_odr(icm42688_accel_odr_t accel_odr)
{
    switch (accel_odr) {
        case ICM42688_ACCEL_ODR_32KHZ:
        case ICM42688_ACCEL_ODR_16KHZ:
        case ICM42688_ACCEL_ODR_8KHZ:
        case ICM42688_ACCEL_ODR_4KHZ:
        case ICM42688_ACCEL_ODR_2KHZ:
        case ICM42688_ACCEL_ODR_1KHZ:
        case ICM42688_ACCEL_ODR_200HZ:
        case ICM42688_ACCEL_ODR_100HZ:
        case ICM42688_ACCEL_ODR_50HZ:
        case ICM42688_ACCEL_ODR_25HZ:
        case ICM42688_ACCEL_ODR_12HZ5:
        case ICM42688_ACCEL_ODR_6HZ25:
        case ICM42688_ACCEL_ODR_3HZ125:
        case ICM42688_ACCEL_ODR_1HZ5625:
        case ICM42688_ACCEL_ODR_500HZ:
            return true;

        default:
            return false;
    }
}

/**
  * @brief  判断完整初始化配置是否合法。
  * @param  init_config 待检查的初始化配置结构体指针。
  * @retval 返回 true 表示合法，返回 false 表示非法。
  */
static bool icm42688_is_valid_init_config(const icm42688_init_config_t *init_config)
{
    if (init_config == (const icm42688_init_config_t *) 0) {
        return false;
    }

    if (icm42688_is_valid_gyro_fs(init_config->gyro_fs) == false) {
        return false;
    }

    if (icm42688_is_valid_gyro_odr(init_config->gyro_odr) == false) {
        return false;
    }

    if (icm42688_is_valid_accel_fs(init_config->accel_fs) == false) {
        return false;
    }

    if (icm42688_is_valid_accel_odr(init_config->accel_odr) == false) {
        return false;
    }

    return true;
}

/**
  * @brief  更新驱动层缓存的当前初始化配置。
  * @param  init_config 已经确认合法的初始化配置结构体指针。
  * @retval None
  */
static void icm42688_store_current_config(const icm42688_init_config_t *init_config)
{
    if (init_config == (const icm42688_init_config_t *) 0) {
        return;
    }

    g_icm42688_current_config = *init_config;
    g_icm42688_has_current_config = true;
}

/**
  * @brief  向 ICM42688 的指定寄存器写入 1 个字节数据。
  * @param  reg_addr：目标 8 位寄存器地址。
  * @param  reg_data：待写入的寄存器数据。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_write_reg(uint8_t reg_addr, uint8_t reg_data)
{
    bsp_iic_status_t int_status = icm42688_int_write_reg(reg_addr, reg_data);
    icm42688_status_t driver_status = icm42688_convert_int_status(int_status);

    g_icm_debug_last_reg = reg_addr;
    g_icm_debug_last_len = 1U;
    g_icm_debug_last_write_status = (uint8_t) int_status;
    g_icm_debug_last_io_result = (int32_t) driver_status;
    g_icm_debug_write_count++;

    return driver_status;
}

/**
  * @brief  从 ICM42688 的指定寄存器读取 1 个字节数据。
  * @param  reg_addr：目标 8 位寄存器地址。
  * @param  reg_data：寄存器数据输出指针。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_read_reg(uint8_t reg_addr, uint8_t *reg_data)
{
    bsp_iic_status_t int_status;
    icm42688_status_t driver_status;

    if (reg_data == (uint8_t *) 0) {
        g_icm_debug_last_io_result = (int32_t) ICM42688_STATUS_ERROR_PARAM;
        return ICM42688_STATUS_ERROR_PARAM;
    }

    int_status = icm42688_int_read_reg(reg_addr, reg_data);
    driver_status = icm42688_convert_int_status(int_status);

    g_icm_debug_last_reg = reg_addr;
    g_icm_debug_last_len = 1U;
    g_icm_debug_last_read_status = (uint8_t) int_status;
    g_icm_debug_last_io_result = (int32_t) driver_status;
    g_icm_debug_read_count++;

    return driver_status;
}

/**
  * @brief  向 ICM42688 的连续寄存器写入多个字节数据。
  * @param  reg_addr：起始 8 位寄存器地址。
  * @param  write_data：待写入数据缓冲区首地址。
  * @param  write_length：待写入数据长度，单位为字节。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_write_regs(uint8_t reg_addr,
    const uint8_t *write_data, uint16_t write_length)
{
    bsp_iic_status_t int_status;
    icm42688_status_t driver_status;

    if ((write_data == (const uint8_t *) 0) || (write_length == 0U)) {
        g_icm_debug_last_io_result = (int32_t) ICM42688_STATUS_ERROR_PARAM;
        return ICM42688_STATUS_ERROR_PARAM;
    }

    int_status = icm42688_int_write_regs(reg_addr, write_data, write_length);
    driver_status = icm42688_convert_int_status(int_status);

    g_icm_debug_last_reg = reg_addr;
    g_icm_debug_last_len = (uint8_t) write_length;
    g_icm_debug_last_write_status = (uint8_t) int_status;
    g_icm_debug_last_io_result = (int32_t) driver_status;
    g_icm_debug_write_count++;

    return driver_status;
}

/**
  * @brief  从 ICM42688 的连续寄存器读取多个字节数据。
  * @param  reg_addr：起始 8 位寄存器地址。
  * @param  read_data：数据输出缓冲区首地址。
  * @param  read_length：待读取数据长度，单位为字节。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_read_regs(uint8_t reg_addr,
    uint8_t *read_data, uint16_t read_length)
{
    bsp_iic_status_t int_status;
    icm42688_status_t driver_status;

    if ((read_data == (uint8_t *) 0) || (read_length == 0U)) {
        g_icm_debug_last_io_result = (int32_t) ICM42688_STATUS_ERROR_PARAM;
        return ICM42688_STATUS_ERROR_PARAM;
    }

    int_status = icm42688_int_read_regs(reg_addr, read_data, read_length);
    driver_status = icm42688_convert_int_status(int_status);

    g_icm_debug_last_reg = reg_addr;
    g_icm_debug_last_len = (uint8_t) read_length;
    g_icm_debug_last_read_status = (uint8_t) int_status;
    g_icm_debug_last_io_result = (int32_t) driver_status;
    g_icm_debug_read_count++;

    return driver_status;
}

/**
  * @brief  从 ICM42688 的 WHO_AM_I 寄存器读取器件标识值。
  * @param  who_am_i：器件标识输出指针。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_get_who_am_i(uint8_t *who_am_i)
{
    icm42688_status_t driver_status;

    if (who_am_i == (uint8_t *) 0) {
        g_icm_debug_last_io_result = (int32_t) ICM42688_STATUS_ERROR_PARAM;
        return ICM42688_STATUS_ERROR_PARAM;
    }

    driver_status = icm42688_read_reg(ICM42688_REG_WHO_AM_I, who_am_i);
    if (driver_status == ICM42688_STATUS_OK) {
        g_icm_debug_last_who_am_i = *who_am_i;
    }

    return driver_status;
}

/**
  * @brief  检查 ICM42688 的 WHO_AM_I 是否与预期器件标识一致。
  * @param  None
  * @retval 返回驱动状态码，返回 ICM42688_STATUS_OK 表示器件 ID 正确。
  */
icm42688_status_t icm42688_check_id(void)
{
    icm42688_status_t driver_status;
    uint8_t who_am_i = 0U;

    /*
     * 先确认器件地址存在应答，再读取 WHO_AM_I，可更清楚地区分“总线不通”
     * 和“器件 ID 错误”这两类问题。
     */
    driver_status = icm42688_convert_int_status(icm42688_int_check_device());
    if (driver_status != ICM42688_STATUS_OK) {
        return driver_status;
    }

    driver_status = icm42688_get_who_am_i(&who_am_i);
    if (driver_status != ICM42688_STATUS_OK) {
        return driver_status;
    }

    if (who_am_i != ICM42688_WHO_AM_I_VALUE) {
        return ICM42688_STATUS_ERROR_ID;
    }

    return ICM42688_STATUS_OK;
}

/**
  * @brief  对 ICM42688 执行软件复位。
  * @param  None
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_soft_reset(void)
{
    return icm42688_write_reg(ICM42688_REG_DEVICE_CONFIG, ICM42688_SOFT_RESET_MASK);
}

/**
  * @brief  获取 ICM42688 的默认初始化配置。
  * @param  init_config：默认配置输出结构体指针。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_get_default_init_config(icm42688_init_config_t *init_config)
{
    if (init_config == (icm42688_init_config_t *) 0) {
        return ICM42688_STATUS_ERROR_PARAM;
    }

    init_config->gyro_fs = ICM42688_GYRO_FS_2000DPS;
    init_config->gyro_odr = ICM42688_GYRO_ODR_1KHZ;
    init_config->accel_fs = ICM42688_ACCEL_FS_4G;
    init_config->accel_odr = ICM42688_ACCEL_ODR_1KHZ;

    return ICM42688_STATUS_OK;
}

icm42688_status_t icm42688_get_current_config(icm42688_init_config_t *init_config)
{
    if (init_config == (icm42688_init_config_t *) 0) {
        return ICM42688_STATUS_ERROR_PARAM;
    }

    if (g_icm42688_has_current_config == false) {
        return icm42688_get_default_init_config(init_config);
    }

    *init_config = g_icm42688_current_config;

    return ICM42688_STATUS_OK;
}

/**
 * @brief  根据用户传入的配置参数显式初始化 ICM42688。
 * @param  init_config：初始化配置结构体指针。
 * @retval 返回驱动状态码。
 */
icm42688_status_t icm42688_init_with_config(const icm42688_init_config_t *init_config)
{
    icm42688_status_t driver_status;
    uint8_t gyro_config0;
    uint8_t accel_config0;

    if (icm42688_is_valid_init_config(init_config) == false) {
        return ICM42688_STATUS_ERROR_PARAM;
    }

    gyro_config0 = icm42688_build_gyro_config0(
        init_config->gyro_fs, init_config->gyro_odr);
    accel_config0 = icm42688_build_accel_config0(
        init_config->accel_fs, init_config->accel_odr);

    /*
     * 当前采用不带软复位的显式配置流程，优先保证通信稳定。
     * 先完成底层接口初始化与器件识别，再写入量程与 ODR 配置，最后唤醒 accel 和 gyro。
     */
    icm42688_int_init();

    driver_status = icm42688_check_id();
    if (driver_status != ICM42688_STATUS_OK) {
        g_icm_debug_init_result = (int32_t) icm42688_status_to_legacy_ret(driver_status);
        return driver_status;
    }

    driver_status = icm42688_write_reg(ICM42688_REG_GYRO_CONFIG0, gyro_config0);
    if (driver_status != ICM42688_STATUS_OK) {
        g_icm_debug_init_result = (int32_t) icm42688_status_to_legacy_ret(driver_status);
        return driver_status;
    }

    driver_status = icm42688_write_reg(ICM42688_REG_ACCEL_CONFIG0, accel_config0);
    if (driver_status != ICM42688_STATUS_OK) {
        g_icm_debug_init_result = (int32_t) icm42688_status_to_legacy_ret(driver_status);
        return driver_status;
    }

    driver_status = icm42688_write_reg(
        ICM42688_REG_PWR_MGMT0, ICM42688_PWR_MGMT0_ACCEL_GYRO_LN);
    if (driver_status != ICM42688_STATUS_OK) {
        g_icm_debug_init_result = (int32_t) icm42688_status_to_legacy_ret(driver_status);
        return driver_status;
    }

    delay_ms(ICM42688_SENSOR_START_DELAY_MS);
    icm42688_store_current_config(init_config);
    g_icm_debug_init_result = 0;

    return ICM42688_STATUS_OK;
}

/**
  * @brief  使用默认配置完整初始化 ICM42688。
  * @param  None
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_init(void)
{
    icm42688_init_config_t init_config;
    icm42688_status_t driver_status;

    driver_status = icm42688_get_default_init_config(&init_config);
    if (driver_status != ICM42688_STATUS_OK) {
        return driver_status;
    }

    return icm42688_init_with_config(&init_config);
}

/**
  * @brief  使用默认配置启动 ICM42688 的加速度计和陀螺仪。
  * @param  None
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_init_default(void)
{
    return icm42688_init();
}

icm42688_status_t icm42688_set_gyro_config(
    icm42688_gyro_fs_t gyro_fs, icm42688_gyro_odr_t gyro_odr)
{
    icm42688_init_config_t init_config;
    icm42688_status_t driver_status;
    uint8_t gyro_config0;

    if ((icm42688_is_valid_gyro_fs(gyro_fs) == false) ||
        (icm42688_is_valid_gyro_odr(gyro_odr) == false)) {
        return ICM42688_STATUS_ERROR_PARAM;
    }

    driver_status = icm42688_get_current_config(&init_config);
    if (driver_status != ICM42688_STATUS_OK) {
        return driver_status;
    }

    gyro_config0 = icm42688_build_gyro_config0(gyro_fs, gyro_odr);
    driver_status = icm42688_write_reg(ICM42688_REG_GYRO_CONFIG0, gyro_config0);
    if (driver_status != ICM42688_STATUS_OK) {
        return driver_status;
    }

    init_config.gyro_fs = gyro_fs;
    init_config.gyro_odr = gyro_odr;
    icm42688_store_current_config(&init_config);

    return ICM42688_STATUS_OK;
}

icm42688_status_t icm42688_set_accel_config(
    icm42688_accel_fs_t accel_fs, icm42688_accel_odr_t accel_odr)
{
    icm42688_init_config_t init_config;
    icm42688_status_t driver_status;
    uint8_t accel_config0;

    if ((icm42688_is_valid_accel_fs(accel_fs) == false) ||
        (icm42688_is_valid_accel_odr(accel_odr) == false)) {
        return ICM42688_STATUS_ERROR_PARAM;
    }

    driver_status = icm42688_get_current_config(&init_config);
    if (driver_status != ICM42688_STATUS_OK) {
        return driver_status;
    }

    accel_config0 = icm42688_build_accel_config0(accel_fs, accel_odr);
    driver_status = icm42688_write_reg(ICM42688_REG_ACCEL_CONFIG0, accel_config0);
    if (driver_status != ICM42688_STATUS_OK) {
        return driver_status;
    }

    init_config.accel_fs = accel_fs;
    init_config.accel_odr = accel_odr;
    icm42688_store_current_config(&init_config);

    return ICM42688_STATUS_OK;
}

/**
  * @brief  连续读取 ICM42688 的温度、三轴加速度和三轴陀螺仪原始数据。
  * @param  raw_data：原始数据输出结构体指针。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_read_raw_data(icm42688_raw_data_t *raw_data)
{
    icm42688_status_t driver_status;
    uint8_t raw_buffer[ICM42688_RAW_DATA_LENGTH];

    if (raw_data == (icm42688_raw_data_t *) 0) {
        return ICM42688_STATUS_ERROR_PARAM;
    }

    driver_status = icm42688_read_regs(
        ICM42688_REG_TEMP_DATA1, raw_buffer, ICM42688_RAW_DATA_LENGTH);
    if (driver_status != ICM42688_STATUS_OK) {
        return driver_status;
    }

    raw_data->temperature = icm42688_merge_bytes(raw_buffer[0], raw_buffer[1]);
    raw_data->accel_x = icm42688_merge_bytes(raw_buffer[2], raw_buffer[3]);
    raw_data->accel_y = icm42688_merge_bytes(raw_buffer[4], raw_buffer[5]);
    raw_data->accel_z = icm42688_merge_bytes(raw_buffer[6], raw_buffer[7]);
    raw_data->gyro_x = icm42688_merge_bytes(raw_buffer[8], raw_buffer[9]);
    raw_data->gyro_y = icm42688_merge_bytes(raw_buffer[10], raw_buffer[11]);
    raw_data->gyro_z = icm42688_merge_bytes(raw_buffer[12], raw_buffer[13]);

    return ICM42688_STATUS_OK;
}

/**
  * @brief  将陀螺仪量程枚举转换为可读字符串。
  * @param  gyro_fs：陀螺仪量程枚举值。
  * @retval 返回对应的字符串常量。
  */
const char *icm42688_get_gyro_fs_string(icm42688_gyro_fs_t gyro_fs)
{
    switch (gyro_fs) {
        case ICM42688_GYRO_FS_2000DPS:
            return "+/-2000dps";
        case ICM42688_GYRO_FS_1000DPS:
            return "+/-1000dps";
        case ICM42688_GYRO_FS_500DPS:
            return "+/-500dps";
        case ICM42688_GYRO_FS_250DPS:
            return "+/-250dps";
        case ICM42688_GYRO_FS_125DPS:
            return "+/-125dps";
        case ICM42688_GYRO_FS_62DPS5:
            return "+/-62.5dps";
        case ICM42688_GYRO_FS_31DPS25:
            return "+/-31.25dps";
        case ICM42688_GYRO_FS_15DPS625:
            return "+/-15.625dps";
        default:
            return "UNKNOWN";
    }
}

/**
  * @brief  将加速度计量程枚举转换为可读字符串。
  * @param  accel_fs：加速度计量程枚举值。
  * @retval 返回对应的字符串常量。
  */
const char *icm42688_get_accel_fs_string(icm42688_accel_fs_t accel_fs)
{
    switch (accel_fs) {
        case ICM42688_ACCEL_FS_16G:
            return "+/-16g";
        case ICM42688_ACCEL_FS_8G:
            return "+/-8g";
        case ICM42688_ACCEL_FS_4G:
            return "+/-4g";
        case ICM42688_ACCEL_FS_2G:
            return "+/-2g";
        default:
            return "UNKNOWN";
    }
}

/**
  * @brief  将陀螺仪输出数据率枚举转换为可读字符串。
  * @param  gyro_odr：陀螺仪输出数据率枚举值。
  * @retval 返回对应的字符串常量。
  */
const char *icm42688_get_gyro_odr_string(icm42688_gyro_odr_t gyro_odr)
{
    switch (gyro_odr) {
        case ICM42688_GYRO_ODR_32KHZ:
            return "32kHz";
        case ICM42688_GYRO_ODR_16KHZ:
            return "16kHz";
        case ICM42688_GYRO_ODR_8KHZ:
            return "8kHz";
        case ICM42688_GYRO_ODR_4KHZ:
            return "4kHz";
        case ICM42688_GYRO_ODR_2KHZ:
            return "2kHz";
        case ICM42688_GYRO_ODR_1KHZ:
            return "1kHz";
        case ICM42688_GYRO_ODR_200HZ:
            return "200Hz";
        case ICM42688_GYRO_ODR_100HZ:
            return "100Hz";
        case ICM42688_GYRO_ODR_50HZ:
            return "50Hz";
        case ICM42688_GYRO_ODR_25HZ:
            return "25Hz";
        case ICM42688_GYRO_ODR_12HZ5:
            return "12.5Hz";
        case ICM42688_GYRO_ODR_500HZ:
            return "500Hz";
        default:
            return "UNKNOWN";
    }
}

/**
  * @brief  将加速度计输出数据率枚举转换为可读字符串。
  * @param  accel_odr：加速度计输出数据率枚举值。
  * @retval 返回对应的字符串常量。
  */
const char *icm42688_get_accel_odr_string(icm42688_accel_odr_t accel_odr)
{
    switch (accel_odr) {
        case ICM42688_ACCEL_ODR_32KHZ:
            return "32kHz";
        case ICM42688_ACCEL_ODR_16KHZ:
            return "16kHz";
        case ICM42688_ACCEL_ODR_8KHZ:
            return "8kHz";
        case ICM42688_ACCEL_ODR_4KHZ:
            return "4kHz";
        case ICM42688_ACCEL_ODR_2KHZ:
            return "2kHz";
        case ICM42688_ACCEL_ODR_1KHZ:
            return "1kHz";
        case ICM42688_ACCEL_ODR_200HZ:
            return "200Hz";
        case ICM42688_ACCEL_ODR_100HZ:
            return "100Hz";
        case ICM42688_ACCEL_ODR_50HZ:
            return "50Hz";
        case ICM42688_ACCEL_ODR_25HZ:
            return "25Hz";
        case ICM42688_ACCEL_ODR_12HZ5:
            return "12.5Hz";
        case ICM42688_ACCEL_ODR_6HZ25:
            return "6.25Hz";
        case ICM42688_ACCEL_ODR_3HZ125:
            return "3.125Hz";
        case ICM42688_ACCEL_ODR_1HZ5625:
            return "1.5625Hz";
        case ICM42688_ACCEL_ODR_500HZ:
            return "500Hz";
        default:
            return "UNKNOWN";
    }
}

/**
  * @brief  获取当前加速度计量程下每 1g 对应的原始 LSB 数。
  * @param  accel_fs：加速度计量程枚举值。
  * @retval 返回对应的灵敏度，单位为 LSB/g。
  */
float icm42688_get_accel_lsb_per_g(icm42688_accel_fs_t accel_fs)
{
    switch (accel_fs) {
        case ICM42688_ACCEL_FS_16G:
            return 2048.0f;
        case ICM42688_ACCEL_FS_8G:
            return 4096.0f;
        case ICM42688_ACCEL_FS_4G:
            return 8192.0f;
        case ICM42688_ACCEL_FS_2G:
            return 16384.0f;
        default:
            return 0.0f;
    }
}

/**
  * @brief  获取当前陀螺仪量程下每 1dps 对应的原始 LSB 数。
  * @param  gyro_fs：陀螺仪量程枚举值。
  * @retval 返回对应的灵敏度，单位为 LSB/dps。
  */
float icm42688_get_gyro_lsb_per_dps(icm42688_gyro_fs_t gyro_fs)
{
    switch (gyro_fs) {
        case ICM42688_GYRO_FS_2000DPS:
            return 16.4f;
        case ICM42688_GYRO_FS_1000DPS:
            return 32.8f;
        case ICM42688_GYRO_FS_500DPS:
            return 65.5f;
        case ICM42688_GYRO_FS_250DPS:
            return 131.0f;
        case ICM42688_GYRO_FS_125DPS:
            return 262.0f;
        case ICM42688_GYRO_FS_62DPS5:
            return 524.3f;
        case ICM42688_GYRO_FS_31DPS25:
            return 1048.6f;
        case ICM42688_GYRO_FS_15DPS625:
            return 2097.2f;
        default:
            return 0.0f;
    }
}

/**
 * @brief  将原始数据按指定初始化配置换算为摄氏度、g、dps 和 rad/s 单位。
  * @param  init_config：初始化配置结构体指针。
  * @param  raw_data：原始数据输入结构体指针。
  * @param  scaled_data：换算后数据输出结构体指针。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_convert_raw_data(
    const icm42688_init_config_t *init_config,
    const icm42688_raw_data_t *raw_data,
    icm42688_scaled_data_t *scaled_data)
{
    float accel_lsb_per_g;
    float gyro_lsb_per_dps;

    if ((init_config == (const icm42688_init_config_t *) 0) ||
        (raw_data == (const icm42688_raw_data_t *) 0) ||
        (scaled_data == (icm42688_scaled_data_t *) 0)) {
        return ICM42688_STATUS_ERROR_PARAM;
    }

    accel_lsb_per_g = icm42688_get_accel_lsb_per_g(init_config->accel_fs);
    gyro_lsb_per_dps = icm42688_get_gyro_lsb_per_dps(init_config->gyro_fs);

    if ((accel_lsb_per_g <= 0.0f) || (gyro_lsb_per_dps <= 0.0f)) {
        return ICM42688_STATUS_ERROR_PARAM;
    }

    /*
     * 温度换算公式来自 ICM42688 数据手册：
     * Temperature(degC) = (TEMP_DATA / 132.48) + 25
     */
    scaled_data->temperature_c = ((float) raw_data->temperature / 132.48f) + 25.0f;
    scaled_data->accel_x_g = (float) raw_data->accel_x / accel_lsb_per_g;
    scaled_data->accel_y_g = (float) raw_data->accel_y / accel_lsb_per_g;
    scaled_data->accel_z_g = (float) raw_data->accel_z / accel_lsb_per_g;
    scaled_data->gyro_x_dps = (float) raw_data->gyro_x / gyro_lsb_per_dps;
    scaled_data->gyro_y_dps = (float) raw_data->gyro_y / gyro_lsb_per_dps;
    scaled_data->gyro_z_dps = (float) raw_data->gyro_z / gyro_lsb_per_dps;
    scaled_data->gyro_x_rad_s = scaled_data->gyro_x_dps * ICM42688_DEG_TO_RAD;
    scaled_data->gyro_y_rad_s = scaled_data->gyro_y_dps * ICM42688_DEG_TO_RAD;
    scaled_data->gyro_z_rad_s = scaled_data->gyro_z_dps * ICM42688_DEG_TO_RAD;

    return ICM42688_STATUS_OK;
}

int8_t bsp_Icm42688Init(void)
{
    return icm42688_status_to_legacy_ret(icm42688_init());
}

int8_t bsp_IcmGetTemperature(int16_t *pTemp)
{
    icm42688_raw_data_t raw_data;
    icm42688_scaled_data_t scaled_data;
    icm42688_init_config_t init_config;
    icm42688_status_t driver_status;

    if (pTemp == (int16_t *) 0) {
        return -2;
    }

    driver_status = icm42688_read_raw_data(&raw_data);
    if (driver_status != ICM42688_STATUS_OK) {
        return icm42688_status_to_legacy_ret(driver_status);
    }

    driver_status = icm42688_get_current_config(&init_config);
    if (driver_status != ICM42688_STATUS_OK) {
        return icm42688_status_to_legacy_ret(driver_status);
    }

    driver_status = icm42688_convert_raw_data(&init_config, &raw_data, &scaled_data);
    if (driver_status != ICM42688_STATUS_OK) {
        return icm42688_status_to_legacy_ret(driver_status);
    }

    *pTemp = (int16_t) scaled_data.temperature_c;
    return 0;
}

int8_t bsp_IcmGetAccelerometer(icm42688RawData_t *accData)
{
    icm42688_raw_data_t raw_data;
    icm42688_status_t driver_status;

    if (accData == (icm42688RawData_t *) 0) {
        return -2;
    }

    driver_status = icm42688_read_raw_data(&raw_data);
    if (driver_status != ICM42688_STATUS_OK) {
        return icm42688_status_to_legacy_ret(driver_status);
    }

    accData->x = raw_data.accel_x;
    accData->y = raw_data.accel_y;
    accData->z = raw_data.accel_z;

    return 0;
}

int8_t bsp_IcmGetGyroscope(icm42688RawData_t *gyroData)
{
    icm42688_raw_data_t raw_data;
    icm42688_status_t driver_status;

    if (gyroData == (icm42688RawData_t *) 0) {
        return -2;
    }

    driver_status = icm42688_read_raw_data(&raw_data);
    if (driver_status != ICM42688_STATUS_OK) {
        return icm42688_status_to_legacy_ret(driver_status);
    }

    gyroData->x = raw_data.gyro_x;
    gyroData->y = raw_data.gyro_y;
    gyroData->z = raw_data.gyro_z;

    return 0;
}

int8_t bsp_IcmGetAccelerometerReal(icm42688RealData_t *accData)
{
    icm42688_init_config_t init_config;
    icm42688_raw_data_t raw_data;
    icm42688_scaled_data_t scaled_data;
    icm42688_status_t driver_status;

    if (accData == (icm42688RealData_t *) 0) {
        return -2;
    }

    driver_status = icm42688_read_raw_data(&raw_data);
    if (driver_status != ICM42688_STATUS_OK) {
        return icm42688_status_to_legacy_ret(driver_status);
    }

    driver_status = icm42688_get_current_config(&init_config);
    if (driver_status != ICM42688_STATUS_OK) {
        return icm42688_status_to_legacy_ret(driver_status);
    }

    driver_status = icm42688_convert_raw_data(&init_config, &raw_data, &scaled_data);
    if (driver_status != ICM42688_STATUS_OK) {
        return icm42688_status_to_legacy_ret(driver_status);
    }

    accData->x = scaled_data.accel_x_g;
    accData->y = scaled_data.accel_y_g;
    accData->z = scaled_data.accel_z_g;

    return 0;
}

int8_t bsp_IcmGetGyroscopeReal(icm42688RealData_t *gyroData)
{
    icm42688_init_config_t init_config;
    icm42688_raw_data_t raw_data;
    icm42688_scaled_data_t scaled_data;
    icm42688_status_t driver_status;

    if (gyroData == (icm42688RealData_t *) 0) {
        return -2;
    }

    driver_status = icm42688_read_raw_data(&raw_data);
    if (driver_status != ICM42688_STATUS_OK) {
        return icm42688_status_to_legacy_ret(driver_status);
    }

    driver_status = icm42688_get_current_config(&init_config);
    if (driver_status != ICM42688_STATUS_OK) {
        return icm42688_status_to_legacy_ret(driver_status);
    }

    driver_status = icm42688_convert_raw_data(&init_config, &raw_data, &scaled_data);
    if (driver_status != ICM42688_STATUS_OK) {
        return icm42688_status_to_legacy_ret(driver_status);
    }

    gyroData->x = scaled_data.gyro_x_dps;
    gyroData->y = scaled_data.gyro_y_dps;
    gyroData->z = scaled_data.gyro_z_dps;

    return 0;
}

int8_t bsp_IcmGetRawData(icm42688RealData_t *accData, icm42688RealData_t *gyroData)
{
    icm42688_init_config_t init_config;
    icm42688_raw_data_t raw_data;
    icm42688_scaled_data_t scaled_data;
    icm42688_status_t driver_status;

    if ((accData == (icm42688RealData_t *) 0) ||
        (gyroData == (icm42688RealData_t *) 0)) {
        return -2;
    }

    driver_status = icm42688_read_raw_data(&raw_data);
    if (driver_status != ICM42688_STATUS_OK) {
        return icm42688_status_to_legacy_ret(driver_status);
    }

    driver_status = icm42688_get_current_config(&init_config);
    if (driver_status != ICM42688_STATUS_OK) {
        return icm42688_status_to_legacy_ret(driver_status);
    }

    driver_status = icm42688_convert_raw_data(&init_config, &raw_data, &scaled_data);
    if (driver_status != ICM42688_STATUS_OK) {
        return icm42688_status_to_legacy_ret(driver_status);
    }

    accData->x = scaled_data.accel_x_g;
    accData->y = scaled_data.accel_y_g;
    accData->z = scaled_data.accel_z_g;
    gyroData->x = scaled_data.gyro_x_dps;
    gyroData->y = scaled_data.gyro_y_dps;
    gyroData->z = scaled_data.gyro_z_dps;

    return 0;
}
