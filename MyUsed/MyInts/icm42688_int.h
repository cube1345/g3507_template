#ifndef __ICM42688_INT_H
#define __ICM42688_INT_H

#include <stdint.h>

#include "bsp_iic.h"

/* ICM42688 当前挂载的 BSP IIC 端口，依据当前硬件连接固定为 IIC0。 */
#define ICM42688_INT_BSP_IIC_PORT          (BSP_IIC_PORT_0)

/* ICM42688 当前使用的 7 位 IIC 器件地址，依据原理图中 AD0 上拉配置固定为 0x69。 */
#define ICM42688_INT_DEVICE_ADDR           (0x69U)

/**
  * @brief  初始化 ICM42688 接口绑定层。
  * @param  None
  * @retval None
  */
void icm42688_int_init(void);

/**
  * @brief  获取 ICM42688 当前绑定的 7 位 IIC 器件地址。
  * @param  None
  * @retval 返回当前 ICM42688 使用的 7 位 IIC 器件地址。
  */
uint8_t icm42688_int_get_device_addr(void);

/**
  * @brief  检测 ICM42688 是否在当前绑定的 IIC 总线上正常应答。
  * @param  None
  * @retval 返回底层 BSP IIC 状态码，返回 BSP_IIC_STATUS_OK 表示器件应答正常。
  */
bsp_iic_status_t icm42688_int_check_device(void);

/**
  * @brief  向 ICM42688 的指定 8 位寄存器写入 1 个字节数据。
  * @param  reg_addr：目标 8 位寄存器地址。
  * @param  reg_data：待写入的寄存器数据。
  * @retval 返回底层 BSP IIC 状态码。
  */
bsp_iic_status_t icm42688_int_write_reg(uint8_t reg_addr, uint8_t reg_data);

/**
  * @brief  从 ICM42688 的指定 8 位寄存器读取 1 个字节数据。
  * @param  reg_addr：目标 8 位寄存器地址。
  * @param  reg_data：寄存器数据输出指针。
  * @retval 返回底层 BSP IIC 状态码。
  */
bsp_iic_status_t icm42688_int_read_reg(uint8_t reg_addr, uint8_t *reg_data);

/**
  * @brief  向 ICM42688 的连续 8 位寄存器写入多个字节数据。
  * @param  reg_addr：起始 8 位寄存器地址。
  * @param  write_data：待写入数据缓冲区首地址。
  * @param  write_length：待写入数据长度，单位为字节。
  * @retval 返回底层 BSP IIC 状态码。
  */
bsp_iic_status_t icm42688_int_write_regs(uint8_t reg_addr,
    const uint8_t *write_data, uint16_t write_length);

/**
  * @brief  从 ICM42688 的连续 8 位寄存器读取多个字节数据。
  * @param  reg_addr：起始 8 位寄存器地址。
  * @param  read_data：接收数据缓冲区首地址。
  * @param  read_length：待读取数据长度，单位为字节。
  * @retval 返回底层 BSP IIC 状态码。
  */
bsp_iic_status_t icm42688_int_read_regs(uint8_t reg_addr,
    uint8_t *read_data, uint16_t read_length);

#endif
