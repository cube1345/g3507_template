#ifndef __BSP_IIC_H
#define __BSP_IIC_H

#include <stdbool.h>
#include <stdint.h>

/* BSP IIC 传输模式定义值：轮询模式。 */
#define BSP_IIC_TRANSFER_MODE_POLL      (0U)

/* BSP IIC 传输模式定义值：中断模式。 */
#define BSP_IIC_TRANSFER_MODE_IRQ       (1U)

/*
 * BSP IIC 当前传输模式选择宏。
 * 如需切换为轮询模式，请将该宏改为 BSP_IIC_TRANSFER_MODE_POLL。
 */
#ifndef BSP_IIC_TRANSFER_MODE
#define BSP_IIC_TRANSFER_MODE           BSP_IIC_TRANSFER_MODE_IRQ
#endif

/*
 * 寄存器读链路强制轮询开关。
 * 置 1 后，即便当前全局传输模式为 IRQ，bsp_iic_read_reg8s() 也会改走轮询路径，
 * 便于对比判断 IMU 当前 33ms 耗时是否由 IRQ 读状态机引入。
 */
#ifndef BSP_IIC_FORCE_POLL_REG_READ
#define BSP_IIC_FORCE_POLL_REG_READ     (0U)
#endif

/*
 * When IRQ mode is selected for the current IMU project, only the following
 * controller interrupts are required in SysConfig:
 * RX Done, TX Done, RX FIFO trigger, Addr/Data NACK and Arbitration lost.
 * Register reads use two IRQ transactions:
 * first write the register address, then start a separate IRQ read.
 */

/**
  * @brief  BSP IIC 端口编号定义，用于统一选择 SysConfig 已初始化的硬件 IIC 控制器。
  */
typedef enum
{
    BSP_IIC_PORT_0 = 0U,
    BSP_IIC_PORT_1 = 1U
} bsp_iic_port_t;

/**
  * @brief  BSP IIC 接口返回状态定义，用于上层统一判断传输结果。
  */
typedef enum
{
    BSP_IIC_STATUS_OK = 0U,
    BSP_IIC_STATUS_ERROR_PARAM,
    BSP_IIC_STATUS_ERROR_TIMEOUT,
    BSP_IIC_STATUS_ERROR_BUSY,
    BSP_IIC_STATUS_ERROR_NACK,
    BSP_IIC_STATUS_ERROR_ARB_LOST,
    BSP_IIC_STATUS_ERROR_TRANSFER
} bsp_iic_status_t;

typedef struct
{
    uint16_t transfer_length;
    uint8_t last_stage;
    uint8_t finish_stage;
    uint32_t last_irq_iidx;
    uint32_t last_controller_status;
    uint32_t finish_controller_status;
    uint32_t last_raw_interrupt_status;
    uint32_t finish_raw_interrupt_status;
    uint32_t rxfifo_trigger_count;
    uint32_t rxdone_count;
    uint32_t wait_notify_value;
    uint32_t wait_rtos_ticks;
    uint32_t finish_rtos_tick;
    uint32_t wake_lag_rtos_ticks;
    uint32_t result_code;
    uint8_t completed_by_timeout;
    uint32_t wait_bus_us;
    uint32_t wait_idle_us;
    uint32_t clear_us;
    uint32_t prepare_us;
    uint32_t wait_us;
    uint32_t poll_tx_us;
    uint32_t poll_rx_us;
    uint32_t poll_tail_us;
    uint32_t total_us;
} bsp_iic_profile_t;

/* BSP IIC 初始化与状态接口。 */
void bsp_iic_init(void);
uint32_t bsp_iic_get_bus_speed_hz(bsp_iic_port_t iic_port);
const bsp_iic_profile_t *bsp_iic_get_profile(bsp_iic_port_t iic_port);

/* BSP IIC 基础设备访问接口。 */
bsp_iic_status_t bsp_iic_check_device(bsp_iic_port_t iic_port, uint8_t device_addr);
bsp_iic_status_t bsp_iic_write_bytes(bsp_iic_port_t iic_port, uint8_t device_addr,
    const uint8_t *write_data, uint16_t write_length);
bsp_iic_status_t bsp_iic_read_bytes(bsp_iic_port_t iic_port, uint8_t device_addr,
    uint8_t *read_data, uint16_t read_length);

/* BSP IIC 8 位寄存器访问接口。 */
bsp_iic_status_t bsp_iic_write_reg8(bsp_iic_port_t iic_port, uint8_t device_addr,
    uint8_t reg_addr, uint8_t reg_data);
bsp_iic_status_t bsp_iic_read_reg8(bsp_iic_port_t iic_port, uint8_t device_addr,
    uint8_t reg_addr, uint8_t *reg_data);
bsp_iic_status_t bsp_iic_write_reg8s(bsp_iic_port_t iic_port, uint8_t device_addr,
    uint8_t reg_addr, const uint8_t *write_data, uint16_t write_length);
bsp_iic_status_t bsp_iic_read_reg8s(bsp_iic_port_t iic_port, uint8_t device_addr,
    uint8_t reg_addr, uint8_t *read_data, uint16_t read_length);

#endif
