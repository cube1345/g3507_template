#ifndef __ICM42688_DRIVER_H
#define __ICM42688_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#include "icm42688_int.h"

/* ICM42688 DEVICE_CONFIG 寄存器地址，用于软件复位。 */
#define ICM42688_REG_DEVICE_CONFIG            (0x11U)

/* ICM42688 温度高字节寄存器地址，连续读原始数据从这里开始。 */
#define ICM42688_REG_TEMP_DATA1               (0x1DU)

/* ICM42688 电源管理寄存器地址。 */
#define ICM42688_REG_PWR_MGMT0                (0x4EU)

/* ICM42688 陀螺仪配置寄存器地址。 */
#define ICM42688_REG_GYRO_CONFIG0             (0x4FU)

/* ICM42688 加速度计配置寄存器地址。 */
#define ICM42688_REG_ACCEL_CONFIG0            (0x50U)

/* ICM42688 WHO_AM_I 寄存器地址。 */
#define ICM42688_REG_WHO_AM_I                 (0x75U)

/* ICM42688 正确的 WHO_AM_I 返回值。 */
#define ICM42688_WHO_AM_I_VALUE               (0x47U)

/* ICM42688 软件复位控制位掩码。 */
#define ICM42688_SOFT_RESET_MASK              (0x01U)

/* ICM42688 同时开启 accel 和 gyro 低噪声模式的配置值。 */
#define ICM42688_PWR_MGMT0_ACCEL_GYRO_LN      (0x0FU)

/* 一次连续读取温度、加速度计和陀螺仪原始数据的总字节数。 */
#define ICM42688_RAW_DATA_LENGTH              (14U)

/* 传感器启动后的稳定等待时间，单位毫秒。 */
#define ICM42688_SENSOR_START_DELAY_MS        (50U)

/* 角度转弧度换算系数，单位 rad/deg。 */
#define ICM42688_DEG_TO_RAD                   (0.01745329251994329577f)

/**
  * @brief  ICM42688 驱动层统一状态码定义。
  */
typedef enum
{
    ICM42688_STATUS_OK = 0U,
    ICM42688_STATUS_ERROR_PARAM,
    ICM42688_STATUS_ERROR_COMM,
    ICM42688_STATUS_ERROR_ID
} icm42688_status_t;

/**
  * @brief  ICM42688 陀螺仪量程枚举定义，对应 GYRO_CONFIG0[7:5]。
  */
typedef enum
{
    ICM42688_GYRO_FS_2000DPS = 0U,
    ICM42688_GYRO_FS_1000DPS = 1U,
    ICM42688_GYRO_FS_500DPS = 2U,
    ICM42688_GYRO_FS_250DPS = 3U,
    ICM42688_GYRO_FS_125DPS = 4U,
    ICM42688_GYRO_FS_62DPS5 = 5U,
    ICM42688_GYRO_FS_31DPS25 = 6U,
    ICM42688_GYRO_FS_15DPS625 = 7U
} icm42688_gyro_fs_t;

/**
  * @brief  ICM42688 加速度计量程枚举定义，对应 ACCEL_CONFIG0[7:5]。
  */
typedef enum
{
    ICM42688_ACCEL_FS_16G = 0U,
    ICM42688_ACCEL_FS_8G = 1U,
    ICM42688_ACCEL_FS_4G = 2U,
    ICM42688_ACCEL_FS_2G = 3U
} icm42688_accel_fs_t;

/**
  * @brief  ICM42688 陀螺仪输出数据率枚举定义，对应 GYRO_CONFIG0[3:0]。
  */
typedef enum
{
    ICM42688_GYRO_ODR_32KHZ = 1U,
    ICM42688_GYRO_ODR_16KHZ = 2U,
    ICM42688_GYRO_ODR_8KHZ = 3U,
    ICM42688_GYRO_ODR_4KHZ = 4U,
    ICM42688_GYRO_ODR_2KHZ = 5U,
    ICM42688_GYRO_ODR_1KHZ = 6U,
    ICM42688_GYRO_ODR_200HZ = 7U,
    ICM42688_GYRO_ODR_100HZ = 8U,
    ICM42688_GYRO_ODR_50HZ = 9U,
    ICM42688_GYRO_ODR_25HZ = 10U,
    ICM42688_GYRO_ODR_12HZ5 = 11U,
    ICM42688_GYRO_ODR_500HZ = 15U
} icm42688_gyro_odr_t;

/**
  * @brief  ICM42688 加速度计输出数据率枚举定义，对应 ACCEL_CONFIG0[3:0]。
  */
typedef enum
{
    ICM42688_ACCEL_ODR_32KHZ = 1U,
    ICM42688_ACCEL_ODR_16KHZ = 2U,
    ICM42688_ACCEL_ODR_8KHZ = 3U,
    ICM42688_ACCEL_ODR_4KHZ = 4U,
    ICM42688_ACCEL_ODR_2KHZ = 5U,
    ICM42688_ACCEL_ODR_1KHZ = 6U,
    ICM42688_ACCEL_ODR_200HZ = 7U,
    ICM42688_ACCEL_ODR_100HZ = 8U,
    ICM42688_ACCEL_ODR_50HZ = 9U,
    ICM42688_ACCEL_ODR_25HZ = 10U,
    ICM42688_ACCEL_ODR_12HZ5 = 11U,
    ICM42688_ACCEL_ODR_6HZ25 = 12U,
    ICM42688_ACCEL_ODR_3HZ125 = 13U,
    ICM42688_ACCEL_ODR_1HZ5625 = 14U,
    ICM42688_ACCEL_ODR_500HZ = 15U
} icm42688_accel_odr_t;

/**
  * @brief  ICM42688 初始化配置结构体。
  */
typedef struct
{
    icm42688_gyro_fs_t gyro_fs;
    icm42688_gyro_odr_t gyro_odr;
    icm42688_accel_fs_t accel_fs;
    icm42688_accel_odr_t accel_odr;
} icm42688_init_config_t;

/**
  * @brief  ICM42688 原始数据结构体。
  */
typedef struct
{
    int16_t temperature;
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} icm42688_raw_data_t;

/**
  * @brief  ICM42688 换算后的物理量数据结构体。
  */
typedef struct
{
    float temperature_c;
    float accel_x_g;
    float accel_y_g;
    float accel_z_g;
    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;
    float gyro_x_rad_s;
    float gyro_y_rad_s;
    float gyro_z_rad_s;
} icm42688_scaled_data_t;

typedef struct
{
    int16_t x; /**< Raw LSB value from the x axis. */
    int16_t y; /**< Raw LSB value from the y axis. */
    int16_t z; /**< Raw LSB value from the z axis. */
} icm42688RawData_t;

typedef struct
{
    float x; /**< Scaled physical value from the x axis. */
    float y; /**< Scaled physical value from the y axis. */
    float z; /**< Scaled physical value from the z axis. */
} icm42688RealData_t;

extern volatile uint8_t g_icm_debug_last_who_am_i;
extern volatile int32_t g_icm_debug_init_result;
extern volatile uint8_t g_icm_debug_last_read_status;
extern volatile uint8_t g_icm_debug_last_write_status;
extern volatile uint8_t g_icm_debug_last_reg;
extern volatile uint8_t g_icm_debug_last_len;
extern volatile uint32_t g_icm_debug_read_count;
extern volatile uint32_t g_icm_debug_write_count;
extern volatile int32_t g_icm_debug_last_io_result;

/**
  * @brief  获取 ICM42688 默认初始化配置。
  * @param  init_config 默认配置输出结构体指针。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_get_default_init_config(icm42688_init_config_t *init_config);

/**
  * @brief  获取 ICM42688 当前缓存配置。
  * @param  init_config 当前配置输出结构体指针。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_get_current_config(icm42688_init_config_t *init_config);

/**
  * @brief  使用默认配置完整初始化 ICM42688。
  * @param  None
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_init(void);

/**
  * @brief  使用指定配置完整初始化 ICM42688。
  * @param  init_config 初始化配置结构体指针。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_init_with_config(const icm42688_init_config_t *init_config);

/**
  * @brief  与 icm42688_init 等效的兼容接口。
  * @param  None
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_init_default(void);

/**
  * @brief  单独更新陀螺仪量程和 ODR 配置。
  * @param  gyro_fs 陀螺仪量程枚举值。
  * @param  gyro_odr 陀螺仪输出数据率枚举值。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_set_gyro_config(
    icm42688_gyro_fs_t gyro_fs, icm42688_gyro_odr_t gyro_odr);

/**
  * @brief  单独更新加速度计量程和 ODR 配置。
  * @param  accel_fs 加速度计量程枚举值。
  * @param  accel_odr 加速度计输出数据率枚举值。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_set_accel_config(
    icm42688_accel_fs_t accel_fs, icm42688_accel_odr_t accel_odr);

/**
  * @brief  检查 ICM42688 WHO_AM_I 是否正确。
  * @param  None
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_check_id(void);

/**
  * @brief  读取 ICM42688 WHO_AM_I。
  * @param  who_am_i WHO_AM_I 输出指针。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_get_who_am_i(uint8_t *who_am_i);

/**
  * @brief  执行 ICM42688 软件复位。
  * @param  None
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_soft_reset(void);

/**
  * @brief  向指定寄存器写入 1 字节数据。
  * @param  reg_addr 8 位寄存器地址。
  * @param  reg_data 待写入寄存器数据。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_write_reg(uint8_t reg_addr, uint8_t reg_data);

/**
  * @brief  从指定寄存器读取 1 字节数据。
  * @param  reg_addr 8 位寄存器地址。
  * @param  reg_data 寄存器数据输出指针。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_read_reg(uint8_t reg_addr, uint8_t *reg_data);

/**
  * @brief  向连续寄存器写入多个字节数据。
  * @param  reg_addr 起始 8 位寄存器地址。
  * @param  write_data 写数据缓冲区首地址。
  * @param  write_length 写数据长度，单位字节。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_write_regs(
    uint8_t reg_addr, const uint8_t *write_data, uint16_t write_length);

/**
  * @brief  从连续寄存器读取多个字节数据。
  * @param  reg_addr 起始 8 位寄存器地址。
  * @param  read_data 读数据缓冲区首地址。
  * @param  read_length 读数据长度，单位字节。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_read_regs(
    uint8_t reg_addr, uint8_t *read_data, uint16_t read_length);

/**
  * @brief  连续读取原始温度、加速度计和陀螺仪数据。
  * @param  raw_data 原始数据输出结构体指针。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_read_raw_data(icm42688_raw_data_t *raw_data);

/**
  * @brief  获取陀螺仪量程对应的可读字符串。
  * @param  gyro_fs 陀螺仪量程枚举值。
  * @retval 返回量程字符串。
  */
const char *icm42688_get_gyro_fs_string(icm42688_gyro_fs_t gyro_fs);

/**
  * @brief  获取加速度计量程对应的可读字符串。
  * @param  accel_fs 加速度计量程枚举值。
  * @retval 返回量程字符串。
  */
const char *icm42688_get_accel_fs_string(icm42688_accel_fs_t accel_fs);

/**
  * @brief  获取陀螺仪 ODR 对应的可读字符串。
  * @param  gyro_odr 陀螺仪输出数据率枚举值。
  * @retval 返回 ODR 字符串。
  */
const char *icm42688_get_gyro_odr_string(icm42688_gyro_odr_t gyro_odr);

/**
  * @brief  获取加速度计 ODR 对应的可读字符串。
  * @param  accel_odr 加速度计输出数据率枚举值。
  * @retval 返回 ODR 字符串。
  */
const char *icm42688_get_accel_odr_string(icm42688_accel_odr_t accel_odr);

/**
  * @brief  获取当前加速度计量程下每 1g 对应的原始 LSB 数。
  * @param  accel_fs 加速度计量程枚举值。
  * @retval 返回灵敏度，单位 LSB/g。
  */
float icm42688_get_accel_lsb_per_g(icm42688_accel_fs_t accel_fs);

/**
  * @brief  获取当前陀螺仪量程下每 1dps 对应的原始 LSB 数。
  * @param  gyro_fs 陀螺仪量程枚举值。
  * @retval 返回灵敏度，单位 LSB/dps。
  */
float icm42688_get_gyro_lsb_per_dps(icm42688_gyro_fs_t gyro_fs);

/**
  * @brief  将原始数据按指定配置换算为物理量。
  * @param  init_config 初始化配置结构体指针。
  * @param  raw_data 原始数据输入结构体指针。
  * @param  scaled_data 换算后数据输出结构体指针。
  * @retval 返回驱动状态码。
  */
icm42688_status_t icm42688_convert_raw_data(
    const icm42688_init_config_t *init_config,
    const icm42688_raw_data_t *raw_data,
    icm42688_scaled_data_t *scaled_data);

int8_t bsp_Icm42688Init(void);
int8_t bsp_IcmGetTemperature(int16_t *pTemp);
int8_t bsp_IcmGetAccelerometer(icm42688RawData_t *accData);
int8_t bsp_IcmGetGyroscope(icm42688RawData_t *gyroData);
int8_t bsp_IcmGetAccelerometerReal(icm42688RealData_t *accData);
int8_t bsp_IcmGetGyroscopeReal(icm42688RealData_t *gyroData);
int8_t bsp_IcmGetRawData(icm42688RealData_t *accData, icm42688RealData_t *gyroData);

#endif
