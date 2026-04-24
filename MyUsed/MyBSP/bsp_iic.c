#include "bsp_iic.h"

#include "FreeRTOS.h"
#include "my_system_lib.h"
#include "task.h"
#include "ti_msp_dl_config.h"

static bsp_iic_profile_t g_bsp_iic0_profile;
#if defined(I2C_1_INST)
#define BSP_IIC_HAS_PORT1                    (1U)
static bsp_iic_profile_t g_bsp_iic1_profile;
#else
#define BSP_IIC_HAS_PORT1                    (0U)
#endif

/* IIC 轮询或中断等待的统一超时计数，用于限制异常情况下的最长阻塞时间。 */
#define BSP_IIC_TIMEOUT_COUNT                (200000U)

/* IIC 中断模式下任务通知等待的超时时间，单位为 ms。*/
#define BSP_IIC_IRQ_WAIT_TIMEOUT_MS          (50U)

/* IIC 事务中需要重点关注的错误中断掩码，用于统一识别 NACK 和仲裁丢失异常。 */
#define BSP_IIC_ERROR_INT_MASK               (DL_I2C_INTERRUPT_CONTROLLER_NACK | \
                                              DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST)

/* IIC 控制器事务完成相关中断掩码，用于判断一次收发流程是否结束。 */
#define BSP_IIC_DONE_INT_MASK                (DL_I2C_INTERRUPT_CONTROLLER_TX_DONE | \
                                              DL_I2C_INTERRUPT_CONTROLLER_RX_DONE)

/* IIC 中断模式下需要开启的控制器中断集合，用于驱动收发状态机。 */
#define BSP_IIC_CONTROLLER_IRQ_MASK          (DL_I2C_INTERRUPT_CONTROLLER_RX_DONE | \
                                              DL_I2C_INTERRUPT_CONTROLLER_TX_DONE | \
                                              DL_I2C_INTERRUPT_CONTROLLER_RXFIFO_TRIGGER | \
                                              DL_I2C_INTERRUPT_CONTROLLER_NACK | \
                                              DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST)

/* IIC 完成事件测试引脚置高辅助宏，用于逻辑分析仪观察 finish 时刻。 */

/* IIC 完成事件测试引脚置低辅助宏，用于逻辑分析仪观察 finish 时刻。 */


/**
  * @brief  BSP IIC 中断事务阶段定义，用于描述一次收发流程当前推进到哪一步。
  */
typedef enum
{
    BSP_IIC_IRQ_STAGE_IDLE = 0U,
    BSP_IIC_IRQ_STAGE_WRITE,
    BSP_IIC_IRQ_STAGE_READ
} bsp_iic_irq_stage_t;

#if (BSP_IIC_TRANSFER_MODE == BSP_IIC_TRANSFER_MODE_IRQ)

/**
  * @brief  单个 IIC 端口的中断事务上下文，用于保存一次阻塞式中断收发所需的状态。
  */
typedef struct
{
    volatile bool is_busy;
    volatile bool is_done;
    volatile bsp_iic_status_t result;
    volatile bsp_iic_irq_stage_t stage;
    TaskHandle_t waiting_task;
    uint8_t device_addr;
    uint8_t reg_addr;
    const uint8_t *tx_buffer;
    uint16_t tx_length;
    volatile uint16_t tx_index;
    uint8_t *rx_buffer;
    uint16_t rx_length;
    volatile uint16_t rx_index;
} bsp_iic_irq_context_t;

/* IIC0 的中断事务上下文，用于保存当前一次基于中断的 IIC0 访问状态。 */
static bsp_iic_irq_context_t g_bsp_iic0_irq_context;

/* IIC1 的中断事务上下文，用于保存当前一次基于中断的 IIC1 访问状态。 */
#if (BSP_IIC_HAS_PORT1 == 1U)
static bsp_iic_irq_context_t g_bsp_iic1_irq_context;
#endif

#endif

/**
  * @brief  根据逻辑端口编号获取对应的硬件 IIC 实例。
  * @param  iic_port：目标 IIC 端口编号。
  * @retval 返回对应的硬件 IIC 实例指针；若端口非法，则返回空指针。
  */
static I2C_Regs *bsp_iic_get_instance(bsp_iic_port_t iic_port)
{
    I2C_Regs *iic_instance = (I2C_Regs *) 0;

    switch (iic_port) {
        case BSP_IIC_PORT_0:
            iic_instance = I2C_0_INST;
            break;

        case BSP_IIC_PORT_1:
#if (BSP_IIC_HAS_PORT1 == 1U)
            iic_instance = I2C_1_INST;
#else
            iic_instance = (I2C_Regs *) 0;
#endif
            break;

        default:
            iic_instance = (I2C_Regs *) 0;
            break;
    }

    return iic_instance;
}

/**
  * @brief  获取指定端口对应的总线速率，用于给上层提供调试和确认信息。
  * @param  iic_port：目标 IIC 端口编号。
  * @retval 返回总线速率，单位为 Hz；若端口非法，则返回 0。
  */
static uint32_t bsp_iic_get_instance_speed_hz(bsp_iic_port_t iic_port)
{
    uint32_t bus_speed_hz = 0U;

    switch (iic_port) {
        case BSP_IIC_PORT_0:
            bus_speed_hz = I2C_0_BUS_SPEED_HZ;
            break;

        case BSP_IIC_PORT_1:
#if (BSP_IIC_HAS_PORT1 == 1U)
            bus_speed_hz = I2C_1_BUS_SPEED_HZ;
#else
            bus_speed_hz = 0U;
#endif
            break;

        default:
            bus_speed_hz = 0U;
            break;
    }

    return bus_speed_hz;
}

static bsp_iic_profile_t *bsp_iic_get_profile_slot(bsp_iic_port_t iic_port)
{
    switch (iic_port) {
        case BSP_IIC_PORT_0:
            return &g_bsp_iic0_profile;

        case BSP_IIC_PORT_1:
#if (BSP_IIC_HAS_PORT1 == 1U)
            return &g_bsp_iic1_profile;
#else
            return (bsp_iic_profile_t *) 0;
#endif

        default:
            return (bsp_iic_profile_t *) 0;
    }
}

static bsp_iic_profile_t *bsp_iic_get_profile_slot_by_instance(I2C_Regs *iic_instance)
{
    if (iic_instance == I2C_0_INST) {
        return &g_bsp_iic0_profile;
    }

#if (BSP_IIC_HAS_PORT1 == 1U)
    if (iic_instance == I2C_1_INST) {
        return &g_bsp_iic1_profile;
    }
#endif

    return (bsp_iic_profile_t *) 0;
}

static void bsp_iic_disable_irq_transaction_interrupts(I2C_Regs *iic_instance)
{
    DL_I2C_disableInterrupt(iic_instance, BSP_IIC_CONTROLLER_IRQ_MASK);
    DL_I2C_clearInterruptStatus(iic_instance, BSP_IIC_CONTROLLER_IRQ_MASK);
}

static void bsp_iic_enable_irq_transaction_interrupts(I2C_Regs *iic_instance, uint32_t interrupt_mask)
{
    DL_I2C_clearInterruptStatus(iic_instance, interrupt_mask);
    DL_I2C_enableInterrupt(iic_instance, interrupt_mask);
}

static DL_I2C_RX_FIFO_LEVEL bsp_iic_get_rx_fifo_threshold(uint16_t read_length)
{
    (void) read_length;

    return DL_I2C_RX_FIFO_LEVEL_BYTES_1;
}

/**
  * @brief  清理控制器上的残留传输状态，避免上一次异常传输影响本次事务。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @retval None
  */
static void bsp_iic_clear_controller_state(I2C_Regs *iic_instance)
{
    DL_I2C_resetControllerTransfer(iic_instance);
    DL_I2C_flushControllerTXFIFO(iic_instance);
    DL_I2C_flushControllerRXFIFO(iic_instance);
    DL_I2C_clearInterruptStatus(iic_instance, 0xFFFFFFFFU);
}

/**
  * @brief  解析当前 IIC 控制器错误来源，并转换为统一的 BSP 错误码。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @retval 返回统一的 BSP IIC 状态码。
  */
static bsp_iic_status_t bsp_iic_get_error_status(I2C_Regs *iic_instance)
{
    uint32_t interrupt_status = DL_I2C_getRawInterruptStatus(
        iic_instance, BSP_IIC_ERROR_INT_MASK);
    uint32_t controller_status = DL_I2C_getControllerStatus(iic_instance);

    if ((interrupt_status & DL_I2C_INTERRUPT_CONTROLLER_NACK) != 0U) {
        return BSP_IIC_STATUS_ERROR_NACK;
    }

    if ((interrupt_status & DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST) != 0U) {
        return BSP_IIC_STATUS_ERROR_ARB_LOST;
    }

    if ((controller_status & DL_I2C_CONTROLLER_STATUS_ARBITRATION_LOST) != 0U) {
        return BSP_IIC_STATUS_ERROR_ARB_LOST;
    }

    if ((controller_status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) {
        return BSP_IIC_STATUS_ERROR_TRANSFER;
    }

    return BSP_IIC_STATUS_OK;
}

/**
  * @brief  轮询等待控制器进入空闲状态，确保新的事务可以安全启动。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @retval 返回等待结果；超时或检测到错误时返回对应错误码。
  */
static bsp_iic_status_t bsp_iic_wait_controller_idle(I2C_Regs *iic_instance)
{
    uint32_t timeout_count = BSP_IIC_TIMEOUT_COUNT;

    while ((DL_I2C_getControllerStatus(iic_instance) & DL_I2C_CONTROLLER_STATUS_IDLE) == 0U) {
        bsp_iic_status_t error_status = bsp_iic_get_error_status(iic_instance);

        if (error_status != BSP_IIC_STATUS_OK) {
            return error_status;
        }

        if (timeout_count == 0U) {
            return BSP_IIC_STATUS_ERROR_TIMEOUT;
        }

        timeout_count--;
    }

    return BSP_IIC_STATUS_OK;
}

/**
  * @brief  轮询等待总线释放，用于保证上一笔事务的 STOP 已经完成。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @retval 返回等待结果；超时或检测到错误时返回对应错误码。
  */
static bsp_iic_status_t bsp_iic_wait_bus_release(I2C_Regs *iic_instance)
{
    uint32_t timeout_count = BSP_IIC_TIMEOUT_COUNT;

    /*
     * 单纯等待 BUSY_BUS 清零在当前 IRQ 事务链路下会稳定卡住约 31ms，
     * 这说明它并不适合作为“上一笔本控制器事务已经可安全启动下一笔”的判据。
     * 这里改用 TI 官方驱动同类逻辑：只要控制器本身不再 busy，
     * 或者已经进入 idle，就认为可以继续启动下一笔事务。
     */
    while (((DL_I2C_getControllerStatus(iic_instance) & DL_I2C_CONTROLLER_STATUS_BUSY) != 0U) &&
           ((DL_I2C_getControllerStatus(iic_instance) & DL_I2C_CONTROLLER_STATUS_IDLE) == 0U)) {
        bsp_iic_status_t error_status = bsp_iic_get_error_status(iic_instance);

        if (error_status != BSP_IIC_STATUS_OK) {
            return error_status;
        }

        if (timeout_count == 0U) {
            return BSP_IIC_STATUS_ERROR_TIMEOUT;
        }

        timeout_count--;
    }

    return BSP_IIC_STATUS_OK;
}

/**
  * @brief  等待发送 FIFO 出现空位，确保后续字节可以继续送入控制器。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @retval 返回等待结果；超时或检测到错误时返回对应错误码。
  */
static bsp_iic_status_t bsp_iic_wait_tx_fifo_ready(I2C_Regs *iic_instance)
{
    uint32_t timeout_count = BSP_IIC_TIMEOUT_COUNT;

    while (DL_I2C_isControllerTXFIFOFull(iic_instance) == true) {
        bsp_iic_status_t error_status = bsp_iic_get_error_status(iic_instance);

        if (error_status != BSP_IIC_STATUS_OK) {
            return error_status;
        }

        if (timeout_count == 0U) {
            return BSP_IIC_STATUS_ERROR_TIMEOUT;
        }

        timeout_count--;
    }

    return BSP_IIC_STATUS_OK;
}

/**
  * @brief  等待发送 FIFO 彻底清空，作为 repeated-start 前地址阶段发完的判据。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @retval 返回等待结果；超时或检测到错误时返回对应错误码。
  */
static bsp_iic_status_t bsp_iic_wait_tx_fifo_empty(I2C_Regs *iic_instance)
{
    uint32_t timeout_count = BSP_IIC_TIMEOUT_COUNT;

    while (DL_I2C_isControllerTXFIFOEmpty(iic_instance) == false) {
        bsp_iic_status_t error_status = bsp_iic_get_error_status(iic_instance);

        if (error_status != BSP_IIC_STATUS_OK) {
            return error_status;
        }

        if (timeout_count == 0U) {
            return BSP_IIC_STATUS_ERROR_TIMEOUT;
        }

        timeout_count--;
    }

    return BSP_IIC_STATUS_OK;
}

/**
  * @brief  等待接收 FIFO 中出现新数据，避免读取到无效内容。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @retval 返回等待结果；超时或检测到错误时返回对应错误码。
  */
static bsp_iic_status_t bsp_iic_wait_rx_fifo_ready(I2C_Regs *iic_instance)
{
    uint32_t timeout_count = BSP_IIC_TIMEOUT_COUNT;

    while (DL_I2C_isControllerRXFIFOEmpty(iic_instance) == true) {
        bsp_iic_status_t error_status = bsp_iic_get_error_status(iic_instance);

        if (error_status != BSP_IIC_STATUS_OK) {
            return error_status;
        }

        if (timeout_count == 0U) {
            return BSP_IIC_STATUS_ERROR_TIMEOUT;
        }

        timeout_count--;
    }

    return BSP_IIC_STATUS_OK;
}

/**
  * @brief  等待指定事务完成中断置位，用于区分无 STOP 场景和普通场景的完成时机。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @param  interrupt_mask：需要等待的完成中断标志。
  * @retval 返回等待结果；超时或检测到错误时返回对应错误码。
  */
static bsp_iic_status_t bsp_iic_wait_transfer_done(
    I2C_Regs *iic_instance, uint32_t interrupt_mask)
{
    uint32_t timeout_count = BSP_IIC_TIMEOUT_COUNT;

    while (DL_I2C_getRawInterruptStatus(iic_instance, interrupt_mask) == 0U) {
        bsp_iic_status_t error_status = bsp_iic_get_error_status(iic_instance);

        if (error_status != BSP_IIC_STATUS_OK) {
            return error_status;
        }

        if (timeout_count == 0U) {
            return BSP_IIC_STATUS_ERROR_TIMEOUT;
        }

        timeout_count--;
    }

    DL_I2C_clearInterruptStatus(iic_instance, interrupt_mask);

    return BSP_IIC_STATUS_OK;
}

/**
  * @brief  通过纯地址写事务检测器件是否在线。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @retval 返回检测结果，若器件应答则返回 BSP_IIC_STATUS_OK。
  */
static bsp_iic_status_t bsp_iic_probe_device(I2C_Regs *iic_instance, uint8_t device_addr)
{
    bsp_iic_status_t status;

    status = bsp_iic_wait_bus_release(iic_instance);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    status = bsp_iic_wait_controller_idle(iic_instance);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    bsp_iic_clear_controller_state(iic_instance);

    DL_I2C_startControllerTransferAdvanced(iic_instance, device_addr,
        DL_I2C_CONTROLLER_DIRECTION_TX, 0U, DL_I2C_CONTROLLER_START_ENABLE,
        DL_I2C_CONTROLLER_STOP_ENABLE, DL_I2C_CONTROLLER_ACK_ENABLE);

    status = bsp_iic_wait_bus_release(iic_instance);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    return bsp_iic_wait_controller_idle(iic_instance);
}

/**
  * @brief  以轮询方式发送一组连续字节，用于封装所有原始写事务的公共流程。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @param  write_data：待发送数据缓冲区首地址。
  * @param  write_length：待发送数据长度，单位为字节。
  * @retval 返回事务执行结果。
  */
static bsp_iic_status_t bsp_iic_poll_write_packet(I2C_Regs *iic_instance, uint8_t device_addr,
    const uint8_t *write_data, uint16_t write_length)
{
    bsp_iic_status_t status;
    uint16_t write_index = 0U;

    status = bsp_iic_wait_bus_release(iic_instance);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    status = bsp_iic_wait_controller_idle(iic_instance);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    bsp_iic_clear_controller_state(iic_instance);

    /*
     * 先尽量预装一部分数据到发送 FIFO，可减少事务启动后的首字节等待时间。
     * 后续若数据长度超过 FIFO 深度，再在事务过程中继续补装剩余数据。
     */
    while ((write_index < write_length) &&
           (DL_I2C_isControllerTXFIFOFull(iic_instance) == false)) {
        DL_I2C_transmitControllerData(iic_instance, write_data[write_index]);
        write_index++;
    }

    DL_I2C_startControllerTransfer(iic_instance, device_addr,
        DL_I2C_CONTROLLER_DIRECTION_TX, write_length);

    while (write_index < write_length) {
        status = bsp_iic_wait_tx_fifo_ready(iic_instance);
        if (status != BSP_IIC_STATUS_OK) {
            return status;
        }

        DL_I2C_transmitControllerData(iic_instance, write_data[write_index]);
        write_index++;
    }

    status = bsp_iic_wait_transfer_done(iic_instance, DL_I2C_INTERRUPT_CONTROLLER_TX_DONE);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    status = bsp_iic_wait_bus_release(iic_instance);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    return bsp_iic_wait_controller_idle(iic_instance);
}

/**
  * @brief  以轮询方式读取一组连续字节，用于封装所有原始读事务的公共流程。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @param  read_data：接收数据缓冲区首地址。
  * @param  read_length：待读取数据长度，单位为字节。
  * @retval 返回事务执行结果。
  */
static bsp_iic_status_t bsp_iic_poll_read_packet(I2C_Regs *iic_instance, uint8_t device_addr,
    uint8_t *read_data, uint16_t read_length)
{
    bsp_iic_status_t status;
    uint16_t read_index = 0U;

    status = bsp_iic_wait_bus_release(iic_instance);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    status = bsp_iic_wait_controller_idle(iic_instance);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    bsp_iic_clear_controller_state(iic_instance);

    DL_I2C_startControllerTransfer(iic_instance, device_addr,
        DL_I2C_CONTROLLER_DIRECTION_RX, read_length);

    while (read_index < read_length) {
        status = bsp_iic_wait_rx_fifo_ready(iic_instance);
        if (status != BSP_IIC_STATUS_OK) {
            return status;
        }

        read_data[read_index] = DL_I2C_receiveControllerData(iic_instance);
        read_index++;
    }

    status = bsp_iic_wait_bus_release(iic_instance);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    return bsp_iic_wait_controller_idle(iic_instance);
}

/**
  * @brief  将 8 位寄存器地址和后续数据拼成一个连续写事务。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @param  reg_addr：8 位寄存器地址。
  * @param  write_data：待写入数据缓冲区首地址。
  * @param  write_length：待写入数据长度，单位为字节。
  * @retval 返回事务执行结果。
  */
static bsp_iic_status_t bsp_iic_poll_write_reg_packet(I2C_Regs *iic_instance, uint8_t device_addr,
    uint8_t reg_addr, const uint8_t *write_data, uint16_t write_length)
{
    bsp_iic_status_t status;
    uint16_t write_index = 0U;

    status = bsp_iic_wait_bus_release(iic_instance);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    status = bsp_iic_wait_controller_idle(iic_instance);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    bsp_iic_clear_controller_state(iic_instance);

    /* 寄存器访问必须先发送寄存器地址，再发送真正的数据负载。 */
    DL_I2C_transmitControllerData(iic_instance, reg_addr);

    while ((write_index < write_length) &&
           (DL_I2C_isControllerTXFIFOFull(iic_instance) == false)) {
        DL_I2C_transmitControllerData(iic_instance, write_data[write_index]);
        write_index++;
    }

    DL_I2C_startControllerTransfer(iic_instance, device_addr,
        DL_I2C_CONTROLLER_DIRECTION_TX, (uint16_t) (write_length + 1U));

    while (write_index < write_length) {
        status = bsp_iic_wait_tx_fifo_ready(iic_instance);
        if (status != BSP_IIC_STATUS_OK) {
            return status;
        }

        DL_I2C_transmitControllerData(iic_instance, write_data[write_index]);
        write_index++;
    }

    status = bsp_iic_wait_transfer_done(iic_instance, DL_I2C_INTERRUPT_CONTROLLER_TX_DONE);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    status = bsp_iic_wait_bus_release(iic_instance);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    return bsp_iic_wait_controller_idle(iic_instance);
}

/**
  * @brief  通过“寄存器地址写入 + 重启读”流程读取连续寄存器数据。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @param  reg_addr：起始 8 位寄存器地址。
  * @param  read_data：接收数据缓冲区首地址。
  * @param  read_length：待读取数据长度，单位为字节。
  * @retval 返回事务执行结果。
  */
static bsp_iic_status_t bsp_iic_poll_read_reg_packet(I2C_Regs *iic_instance, uint8_t device_addr,
    uint8_t reg_addr, uint8_t *read_data, uint16_t read_length)
{
    bsp_iic_profile_t *profile = bsp_iic_get_profile_slot_by_instance(iic_instance);
    bsp_iic_status_t status;
    uint16_t read_index = 0U;
    uint64_t total_start_us;
    uint64_t stage_start_us;

    total_start_us = My_GetTimeUs();
    if (profile != (bsp_iic_profile_t *) 0) {
        profile->transfer_length = read_length;
        profile->wait_bus_us = 0U;
        profile->wait_idle_us = 0U;
        profile->clear_us = 0U;
        profile->prepare_us = 0U;
        profile->wait_us = 0U;
        profile->poll_tx_us = 0U;
        profile->poll_rx_us = 0U;
        profile->poll_tail_us = 0U;
        profile->total_us = 0U;
        profile->result_code = BSP_IIC_STATUS_OK;
    }

    stage_start_us = My_GetTimeUs();
    status = bsp_iic_wait_bus_release(iic_instance);
    if (profile != (bsp_iic_profile_t *) 0) {
        profile->wait_bus_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
    }
    if (status != BSP_IIC_STATUS_OK) {
        if (profile != (bsp_iic_profile_t *) 0) {
            profile->result_code = (uint32_t) status;
            profile->total_us = (uint32_t) (My_GetTimeUs() - total_start_us);
        }
        return status;
    }

    stage_start_us = My_GetTimeUs();
    status = bsp_iic_wait_controller_idle(iic_instance);
    if (profile != (bsp_iic_profile_t *) 0) {
        profile->wait_idle_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
    }
    if (status != BSP_IIC_STATUS_OK) {
        if (profile != (bsp_iic_profile_t *) 0) {
            profile->result_code = (uint32_t) status;
            profile->total_us = (uint32_t) (My_GetTimeUs() - total_start_us);
        }
        return status;
    }

    stage_start_us = My_GetTimeUs();
    bsp_iic_clear_controller_state(iic_instance);
    if (profile != (bsp_iic_profile_t *) 0) {
        profile->clear_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
    }

    /*
     * 第一步只发送寄存器地址且不发 STOP，这样第二步读取时会形成标准的重复起始条件。
     * 这是绝大多数寄存器型 IIC 器件读取寄存器的通用时序。
     */
    stage_start_us = My_GetTimeUs();
    DL_I2C_transmitControllerData(iic_instance, reg_addr);
    DL_I2C_startControllerTransferAdvanced(iic_instance, device_addr,
        DL_I2C_CONTROLLER_DIRECTION_TX, 1U, DL_I2C_CONTROLLER_START_ENABLE,
        DL_I2C_CONTROLLER_STOP_DISABLE, DL_I2C_CONTROLLER_ACK_ENABLE);

    status = bsp_iic_wait_tx_fifo_empty(iic_instance);
    if (profile != (bsp_iic_profile_t *) 0) {
        profile->poll_tx_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
    }
    if (status != BSP_IIC_STATUS_OK) {
        if (profile != (bsp_iic_profile_t *) 0) {
            profile->result_code = (uint32_t) status;
            profile->total_us = (uint32_t) (My_GetTimeUs() - total_start_us);
        }
        return status;
    }

    DL_I2C_startControllerTransfer(iic_instance, device_addr,
        DL_I2C_CONTROLLER_DIRECTION_RX, read_length);

    stage_start_us = My_GetTimeUs();
    while (read_index < read_length) {
        status = bsp_iic_wait_rx_fifo_ready(iic_instance);
        if (status != BSP_IIC_STATUS_OK) {
            if (profile != (bsp_iic_profile_t *) 0) {
                profile->poll_rx_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
                profile->result_code = (uint32_t) status;
                profile->total_us = (uint32_t) (My_GetTimeUs() - total_start_us);
            }
            return status;
        }

        read_data[read_index] = DL_I2C_receiveControllerData(iic_instance);
        read_index++;
    }
    if (profile != (bsp_iic_profile_t *) 0) {
        profile->poll_rx_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
    }

    stage_start_us = My_GetTimeUs();
    status = bsp_iic_wait_bus_release(iic_instance);
    if (status != BSP_IIC_STATUS_OK) {
        if (profile != (bsp_iic_profile_t *) 0) {
            profile->poll_tail_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
            profile->result_code = (uint32_t) status;
            profile->total_us = (uint32_t) (My_GetTimeUs() - total_start_us);
        }
        return status;
    }

    status = bsp_iic_wait_controller_idle(iic_instance);
    if (profile != (bsp_iic_profile_t *) 0) {
        profile->poll_tail_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
        profile->result_code = (uint32_t) status;
        profile->total_us = (uint32_t) (My_GetTimeUs() - total_start_us);
    }

    return status;
}

#if (BSP_IIC_TRANSFER_MODE == BSP_IIC_TRANSFER_MODE_IRQ)

/**
  * @brief  根据 IIC 端口编号获取对应的中断事务上下文。
  * @param  iic_port：目标 IIC 端口编号。
  * @retval 返回对应端口的中断事务上下文指针；若端口非法，则返回空指针。
  */
static bsp_iic_irq_context_t *bsp_iic_get_irq_context(bsp_iic_port_t iic_port)
{
    bsp_iic_irq_context_t *irq_context = (bsp_iic_irq_context_t *) 0;

    switch (iic_port) {
        case BSP_IIC_PORT_0:
            irq_context = &g_bsp_iic0_irq_context;
            break;

        case BSP_IIC_PORT_1:
#if (BSP_IIC_HAS_PORT1 == 1U)
            irq_context = &g_bsp_iic1_irq_context;
#else
            irq_context = (bsp_iic_irq_context_t *) 0;
#endif
            break;

        default:
            irq_context = (bsp_iic_irq_context_t *) 0;
            break;
    }

    return irq_context;
}

/**
  * @brief  根据硬件 IIC 实例反查中断事务上下文。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @retval 返回对应实例的中断事务上下文指针；若实例非法，则返回空指针。
  */
static bsp_iic_irq_context_t *bsp_iic_get_irq_context_by_instance(I2C_Regs *iic_instance)
{
    if (iic_instance == I2C_0_INST) {
        return &g_bsp_iic0_irq_context;
    }

#if (BSP_IIC_HAS_PORT1 == 1U)
    if (iic_instance == I2C_1_INST) {
        return &g_bsp_iic1_irq_context;
    }
#endif

    return (bsp_iic_irq_context_t *) 0;
}

static I2C_Regs *bsp_iic_get_instance_by_irq_context(
    const bsp_iic_irq_context_t *irq_context)
{
    if (irq_context == &g_bsp_iic0_irq_context) {
        return I2C_0_INST;
    }

#if (BSP_IIC_HAS_PORT1 == 1U)
    if (irq_context == &g_bsp_iic1_irq_context) {
        return I2C_1_INST;
    }
#endif

    return (I2C_Regs *) 0;
}

/**
  * @brief  将中断事务上下文恢复到空闲状态，避免上一次状态残留影响下一次收发。
  * @param  irq_context：目标中断事务上下文指针。
  * @retval None
  */
static void bsp_iic_reset_irq_context(bsp_iic_irq_context_t *irq_context)
{
    irq_context->is_busy = false;
    irq_context->is_done = false;
    irq_context->result = BSP_IIC_STATUS_OK;
    irq_context->stage = BSP_IIC_IRQ_STAGE_IDLE;
    irq_context->waiting_task = (TaskHandle_t) 0;
    irq_context->device_addr = 0U;
    irq_context->reg_addr = 0U;
    irq_context->tx_buffer = (const uint8_t *) 0;
    irq_context->tx_length = 0U;
    irq_context->tx_index = 0U;
    irq_context->rx_buffer = (uint8_t *) 0;
    irq_context->rx_length = 0U;
    irq_context->rx_index = 0U;
}

/**
  * @brief  结束一次中断事务，并统一回填执行结果。
  * @param  irq_context：目标中断事务上下文指针。
  * @param  result：本次事务最终结果。
  * @retval None
  */
static void bsp_iic_finish_irq_transfer(
    bsp_iic_irq_context_t *irq_context, bsp_iic_status_t result)
{
    TaskHandle_t waiting_task = irq_context->waiting_task;
    bsp_iic_profile_t *profile = (bsp_iic_profile_t *) 0;
    uint32_t finish_tick = 0U;
    I2C_Regs *iic_instance;

    if (irq_context == &g_bsp_iic0_irq_context) {
        profile = &g_bsp_iic0_profile;
    }
#if (BSP_IIC_HAS_PORT1 == 1U)
    else if (irq_context == &g_bsp_iic1_irq_context) {
        profile = &g_bsp_iic1_profile;
    }
#endif
    else {
        profile = (bsp_iic_profile_t *) 0;
    }

    iic_instance = bsp_iic_get_instance_by_irq_context(irq_context);

    if (__get_IPSR() == 0U) {
        finish_tick = (uint32_t) xTaskGetTickCount();
    } else {
        finish_tick = (uint32_t) xTaskGetTickCountFromISR();
    }

    if ((profile != (bsp_iic_profile_t *) 0) && (iic_instance != (I2C_Regs *) 0)) {
        profile->finish_stage = (uint8_t) irq_context->stage;
        profile->finish_rtos_tick = finish_tick;
        profile->finish_controller_status =
            DL_I2C_getControllerStatus(iic_instance);
        profile->finish_raw_interrupt_status =
            DL_I2C_getRawInterruptStatus(iic_instance, BSP_IIC_CONTROLLER_IRQ_MASK);
    }
    irq_context->result = result;
    irq_context->is_busy = false;
    irq_context->is_done = true;
    irq_context->stage = BSP_IIC_IRQ_STAGE_IDLE;
    /* 输出一个窄脉冲，标记事务完成判定进入 finish 阶段。 */
    if (waiting_task != (TaskHandle_t) 0) {
        if (__get_IPSR() == 0U) {
            xTaskNotifyGive(waiting_task);
        } else {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;

            vTaskNotifyGiveFromISR(waiting_task, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

/**
  * @brief  判断当前是否处于可使用 FreeRTOS 任务通知等待的任务上下文。
  * @param  None
  * @retval 返回 true 表示当前可以使用任务通知等待。
  */
static bool bsp_iic_is_task_notify_wait_available(void)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        return false;
    }

    if (__get_IPSR() != 0U) {
        return false;
    }

    return true;
}

/**
  * @brief  在中断模式下等待当前事务完成，用于对外保持阻塞式统一接口。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @param  irq_context：目标中断事务上下文指针。
  * @retval 返回事务最终执行结果。
  */
static bsp_iic_status_t bsp_iic_wait_irq_transfer_done(
    I2C_Regs *iic_instance, bsp_iic_irq_context_t *irq_context)
{
    uint32_t timeout_count = BSP_IIC_TIMEOUT_COUNT;
    TickType_t wait_ticks;

    if (bsp_iic_is_task_notify_wait_available() == true) {
        uint32_t notify_value = 0U;
        TickType_t tick_before_wait = 0U;
        TickType_t tick_after_wait = 0U;

        if (irq_context->is_done == false) {
            wait_ticks = pdMS_TO_TICKS(BSP_IIC_IRQ_WAIT_TIMEOUT_MS);
            if (wait_ticks == 0U) {
                wait_ticks = 1U;
            }

            tick_before_wait = xTaskGetTickCount();
            notify_value = (uint32_t) ulTaskNotifyTake(pdTRUE, wait_ticks);
            tick_after_wait = xTaskGetTickCount();
            if (notify_value == 0U) {
                /*
                 * 某些边界场景下，事务可能已经在超时点附近由 IRQ 完成，
                 * 但当前任务由于调度或日志打印抖动尚未来得及取到通知。
                 * 这里在判定超时前再检查一次完成标志，避免把“已完成但通知晚到”
                 * 误判成真正的 IIC 超时。
                 */
                if (irq_context->is_done != false) {
                    irq_context->waiting_task = (TaskHandle_t) 0;
                    {
                        bsp_iic_profile_t *profile = bsp_iic_get_profile_slot_by_instance(iic_instance);
                        if (profile != (bsp_iic_profile_t *) 0) {
                            profile->wait_notify_value = 0U;
                            profile->wait_rtos_ticks =
                                (uint32_t) (tick_after_wait - tick_before_wait);
                            profile->wake_lag_rtos_ticks =
                                (uint32_t) (tick_after_wait - profile->finish_rtos_tick);
                            profile->completed_by_timeout = 0U;
                            profile->result_code = (uint32_t) irq_context->result;
                        }
                    }
                    return irq_context->result;
                }

                bsp_iic_finish_irq_transfer(irq_context, BSP_IIC_STATUS_ERROR_TIMEOUT);
                bsp_iic_disable_irq_transaction_interrupts(iic_instance);
                bsp_iic_clear_controller_state(iic_instance);
                irq_context->waiting_task = (TaskHandle_t) 0;
                {
                    bsp_iic_profile_t *profile = bsp_iic_get_profile_slot_by_instance(iic_instance);
                    if (profile != (bsp_iic_profile_t *) 0) {
                        profile->wait_notify_value = 0U;
                        profile->wait_rtos_ticks =
                            (uint32_t) (tick_after_wait - tick_before_wait);
                        profile->wake_lag_rtos_ticks =
                            (uint32_t) (tick_after_wait - profile->finish_rtos_tick);
                        profile->completed_by_timeout = 1U;
                        profile->result_code = (uint32_t) BSP_IIC_STATUS_ERROR_TIMEOUT;
                    }
                }
                return BSP_IIC_STATUS_ERROR_TIMEOUT;
            }
        }

        irq_context->waiting_task = (TaskHandle_t) 0;
        {
            bsp_iic_profile_t *profile = bsp_iic_get_profile_slot_by_instance(iic_instance);
            if (profile != (bsp_iic_profile_t *) 0) {
                profile->wait_notify_value = notify_value;
                profile->wait_rtos_ticks =
                    (uint32_t) (tick_after_wait - tick_before_wait);
                profile->wake_lag_rtos_ticks =
                    (uint32_t) (tick_after_wait - profile->finish_rtos_tick);
                profile->completed_by_timeout = 0U;
                profile->result_code = (uint32_t) irq_context->result;
            }
        }

        return irq_context->result;
    }

    while (irq_context->is_done == false) {
        if (timeout_count == 0U) {
            bsp_iic_finish_irq_transfer(irq_context, BSP_IIC_STATUS_ERROR_TIMEOUT);
            bsp_iic_disable_irq_transaction_interrupts(iic_instance);
            bsp_iic_clear_controller_state(iic_instance);
            return BSP_IIC_STATUS_ERROR_TIMEOUT;
        }

        timeout_count--;
    }

    return irq_context->result;
}

/**
  * @brief  启动一次中断模式下的连续写事务。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @param  irq_context：目标中断事务上下文指针。
  * @retval None
  */
static void bsp_iic_start_irq_write(I2C_Regs *iic_instance, bsp_iic_irq_context_t *irq_context)
{
    uint32_t interrupt_mask = DL_I2C_INTERRUPT_CONTROLLER_TX_DONE |
                              DL_I2C_INTERRUPT_CONTROLLER_NACK |
                              DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST;

    while ((irq_context->tx_index < irq_context->tx_length) &&
           (DL_I2C_isControllerTXFIFOFull(iic_instance) == false)) {
        DL_I2C_transmitControllerData(iic_instance, irq_context->tx_buffer[irq_context->tx_index]);
        irq_context->tx_index++;
    }

    bsp_iic_disable_irq_transaction_interrupts(iic_instance);
    bsp_iic_enable_irq_transaction_interrupts(iic_instance, interrupt_mask);
    DL_I2C_startControllerTransfer(iic_instance, irq_context->device_addr,
        DL_I2C_CONTROLLER_DIRECTION_TX, irq_context->tx_length);
}

/**
  * @brief  启动一次中断模式下的寄存器写事务。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @param  irq_context：目标中断事务上下文指针。
  * @retval None
  */
/**
  * @brief  启动一次中断模式下的连续读事务。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @param  irq_context：目标中断事务上下文指针。
  * @retval None
  */
static void bsp_iic_start_irq_read(I2C_Regs *iic_instance, bsp_iic_irq_context_t *irq_context)
{
    bsp_iic_disable_irq_transaction_interrupts(iic_instance);
    DL_I2C_setControllerRXFIFOThreshold(
        iic_instance, bsp_iic_get_rx_fifo_threshold(irq_context->rx_length));
    bsp_iic_enable_irq_transaction_interrupts(iic_instance,
        DL_I2C_INTERRUPT_CONTROLLER_RXFIFO_TRIGGER |
            DL_I2C_INTERRUPT_CONTROLLER_RX_DONE |
            DL_I2C_INTERRUPT_CONTROLLER_NACK |
            DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST);
    DL_I2C_startControllerTransfer(iic_instance, irq_context->device_addr,
        DL_I2C_CONTROLLER_DIRECTION_RX, irq_context->rx_length);
}

/**
  * @brief  启动一次中断模式下的寄存器读地址阶段事务。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @param  irq_context：目标中断事务上下文指针。
  * @retval None
  */
/**
  * @brief  根据当前阶段启动中断模式事务，用于统一调度写、读和寄存器访问流程。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @param  irq_context：目标中断事务上下文指针。
  * @retval None
  */
static void bsp_iic_start_irq_stage(I2C_Regs *iic_instance, bsp_iic_irq_context_t *irq_context)
{
    switch (irq_context->stage) {
        case BSP_IIC_IRQ_STAGE_WRITE:
            bsp_iic_start_irq_write(iic_instance, irq_context);
            break;

        case BSP_IIC_IRQ_STAGE_READ:
            bsp_iic_start_irq_read(iic_instance, irq_context);
            break;

        default:
            bsp_iic_finish_irq_transfer(irq_context, BSP_IIC_STATUS_ERROR_TRANSFER);
            break;
    }
}

/**
  * @brief  准备一次中断事务的公共前置流程，确保总线和上下文都进入可用状态。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @param  irq_context：目标中断事务上下文指针。
  * @retval 返回准备结果；若端口正忙、总线异常或超时，则返回对应错误码。
  */
static bsp_iic_status_t bsp_iic_prepare_irq_transfer(
    I2C_Regs *iic_instance, bsp_iic_irq_context_t *irq_context)
{
    bsp_iic_profile_t *profile = bsp_iic_get_profile_slot_by_instance(iic_instance);

    if (irq_context->is_busy == true) {
        return BSP_IIC_STATUS_ERROR_BUSY;
    }

    /*
     * IRQ 事务完成后，控制器内部的传输状态有可能仍然保留上一笔事务的 busy 痕迹，
     * 尤其是在任务被 RX_DONE/TX_DONE 唤醒后立刻发起下一笔事务时更明显。
     * 这里先主动 reset/flush 一次控制器状态，再进入 BUSY/IDLE 判定，
     * 避免下一笔事务白白卡在陈旧 BUSY_BUS 状态上。
     */
    bsp_iic_disable_irq_transaction_interrupts(iic_instance);
    bsp_iic_clear_controller_state(iic_instance);
    bsp_iic_reset_irq_context(irq_context);

    irq_context->is_busy = true;
    irq_context->is_done = false;
    irq_context->result = BSP_IIC_STATUS_OK;
    if (bsp_iic_is_task_notify_wait_available() == true) {
        irq_context->waiting_task = xTaskGetCurrentTaskHandle();
        (void) ulTaskNotifyTake(pdTRUE, 0U);
    }
    if (profile != (bsp_iic_profile_t *) 0) {
        profile->last_stage = 0U;
        profile->finish_stage = 0U;
        profile->last_irq_iidx = 0U;
        profile->last_controller_status = 0U;
        profile->finish_controller_status = 0U;
        profile->last_raw_interrupt_status = 0U;
        profile->finish_raw_interrupt_status = 0U;
        profile->rxfifo_trigger_count = 0U;
        profile->rxdone_count = 0U;
        profile->wait_notify_value = 0U;
        profile->wait_rtos_ticks = 0U;
        profile->finish_rtos_tick = 0U;
        profile->wake_lag_rtos_ticks = 0U;
        profile->result_code = 0U;
        profile->completed_by_timeout = 0U;
    }

    return BSP_IIC_STATUS_OK;
}

/**
  * @brief  以中断方式发送一组连续字节，对外仍保持阻塞式返回风格。
  * @param  iic_port：目标 IIC 端口编号。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @param  write_data：待发送数据缓冲区首地址。
  * @param  write_length：待发送数据长度，单位为字节。
  * @retval 返回事务执行结果。
  */
static bsp_iic_status_t bsp_iic_irq_write_packet(bsp_iic_port_t iic_port, uint8_t device_addr,
    const uint8_t *write_data, uint16_t write_length)
{
    I2C_Regs *iic_instance = bsp_iic_get_instance(iic_port);
    bsp_iic_irq_context_t *irq_context = bsp_iic_get_irq_context(iic_port);
    bsp_iic_status_t status;

    status = bsp_iic_prepare_irq_transfer(iic_instance, irq_context);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    irq_context->device_addr = device_addr;
    irq_context->tx_buffer = write_data;
    irq_context->tx_length = write_length;
    irq_context->tx_index = 0U;
    irq_context->stage = BSP_IIC_IRQ_STAGE_WRITE;

    bsp_iic_start_irq_stage(iic_instance, irq_context);

    return bsp_iic_wait_irq_transfer_done(iic_instance, irq_context);
}

/**
  * @brief  以中断方式读取一组连续字节，对外仍保持阻塞式返回风格。
  * @param  iic_port：目标 IIC 端口编号。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @param  read_data：接收数据缓冲区首地址。
  * @param  read_length：待读取数据长度，单位为字节。
  * @retval 返回事务执行结果。
  */
static bsp_iic_status_t bsp_iic_irq_read_packet(bsp_iic_port_t iic_port, uint8_t device_addr,
    uint8_t *read_data, uint16_t read_length)
{
    I2C_Regs *iic_instance = bsp_iic_get_instance(iic_port);
    bsp_iic_irq_context_t *irq_context = bsp_iic_get_irq_context(iic_port);
    bsp_iic_status_t status;

    status = bsp_iic_prepare_irq_transfer(iic_instance, irq_context);
    if (status != BSP_IIC_STATUS_OK) {
        return status;
    }

    irq_context->device_addr = device_addr;
    irq_context->rx_buffer = read_data;
    irq_context->rx_length = read_length;
    irq_context->rx_index = 0U;
    irq_context->stage = BSP_IIC_IRQ_STAGE_READ;

    bsp_iic_start_irq_stage(iic_instance, irq_context);

    return bsp_iic_wait_irq_transfer_done(iic_instance, irq_context);
}

/**
  * @brief  以中断方式向寄存器写入连续数据，对外仍保持阻塞式返回风格。
  * @param  iic_port：目标 IIC 端口编号。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @param  reg_addr：起始 8 位寄存器地址。
  * @param  write_data：待写入数据缓冲区首地址。
  * @param  write_length：待写入数据长度，单位为字节。
  * @retval 返回事务执行结果。
  */
static bsp_iic_status_t bsp_iic_irq_write_reg_packet(bsp_iic_port_t iic_port, uint8_t device_addr,
    uint8_t reg_addr, const uint8_t *write_data, uint16_t write_length)
{
    I2C_Regs *iic_instance = bsp_iic_get_instance(iic_port);

    return bsp_iic_poll_write_reg_packet(
        iic_instance, device_addr, reg_addr, write_data, write_length);
}

/**
  * @brief  以中断方式读取连续寄存器数据，对外仍保持阻塞式返回风格。
  * @param  iic_port：目标 IIC 端口编号。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @param  reg_addr：起始 8 位寄存器地址。
  * @param  read_data：接收数据缓冲区首地址。
  * @param  read_length：待读取数据长度，单位为字节。
  * @retval 返回事务执行结果。
  */
static bsp_iic_status_t bsp_iic_irq_read_reg_packet(bsp_iic_port_t iic_port, uint8_t device_addr,
    uint8_t reg_addr, uint8_t *read_data, uint16_t read_length)
{
    bsp_iic_profile_t *profile = bsp_iic_get_profile_slot(iic_port);
    bsp_iic_status_t status;
    uint64_t total_start_us;
    uint64_t stage_start_us;

    total_start_us = My_GetTimeUs();
    if (profile != (bsp_iic_profile_t *) 0) {
        profile->transfer_length = read_length;
        profile->last_stage = 0U;
        profile->finish_stage = 0U;
        profile->rxfifo_trigger_count = 0U;
        profile->rxdone_count = 0U;
        profile->last_controller_status = 0U;
        profile->finish_controller_status = 0U;
        profile->last_raw_interrupt_status = 0U;
        profile->finish_raw_interrupt_status = 0U;
        profile->wait_bus_us = 0U;
        profile->wait_idle_us = 0U;
        profile->clear_us = 0U;
        profile->prepare_us = 0U;
        profile->wait_us = 0U;
        profile->wait_rtos_ticks = 0U;
        profile->finish_rtos_tick = 0U;
        profile->wake_lag_rtos_ticks = 0U;
        profile->poll_tx_us = 0U;
        profile->poll_rx_us = 0U;
        profile->poll_tail_us = 0U;
        profile->total_us = 0U;
    }

    stage_start_us = My_GetTimeUs();
    status = bsp_iic_irq_write_packet(iic_port, device_addr, &reg_addr, 1U);
    if (profile != (bsp_iic_profile_t *) 0) {
        profile->poll_tx_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
    }

    if (status == BSP_IIC_STATUS_OK) {
        stage_start_us = My_GetTimeUs();
        status = bsp_iic_irq_read_packet(iic_port, device_addr, read_data, read_length);
        if (profile != (bsp_iic_profile_t *) 0) {
            profile->poll_rx_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
        }
    }

    if (profile != (bsp_iic_profile_t *) 0) {
        profile->poll_tail_us = 0U;
        profile->wait_us = profile->poll_tx_us + profile->poll_rx_us;
        profile->result_code = (uint32_t) status;
        profile->total_us = (uint32_t) (My_GetTimeUs() - total_start_us);
    }

    return status;
}

/**
  * @brief  向发送 FIFO 继续补充待发送数据，确保长帧事务可以在中断中持续推进。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @param  irq_context：目标中断事务上下文指针。
  * @retval None
  */
static void bsp_iic_irq_fill_tx_fifo(I2C_Regs *iic_instance, bsp_iic_irq_context_t *irq_context)
{
    while ((irq_context->tx_index < irq_context->tx_length) &&
           (DL_I2C_isControllerTXFIFOFull(iic_instance) == false)) {
        DL_I2C_transmitControllerData(iic_instance, irq_context->tx_buffer[irq_context->tx_index]);
        irq_context->tx_index++;
    }
}

/**
  * @brief  从接收 FIFO 中提取已到达的数据，确保中断模式下接收缓冲区及时更新。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @param  irq_context：目标中断事务上下文指针。
  * @retval None
  */
static void bsp_iic_irq_drain_rx_fifo(I2C_Regs *iic_instance, bsp_iic_irq_context_t *irq_context)
{
    while (DL_I2C_isControllerRXFIFOEmpty(iic_instance) == false) {
        uint8_t receive_data = DL_I2C_receiveControllerData(iic_instance);

        if (irq_context->rx_index < irq_context->rx_length) {
            irq_context->rx_buffer[irq_context->rx_index] = receive_data;
            irq_context->rx_index++;
        }
    }
}

/**
  * @brief  通用 IIC 中断处理核心，用于根据当前事务阶段驱动状态机继续执行。
  * @param  iic_instance：目标硬件 IIC 实例指针。
  * @retval None
  */
static void bsp_iic_irq_handler_common(I2C_Regs *iic_instance)
{
    bsp_iic_irq_context_t *irq_context = bsp_iic_get_irq_context_by_instance(iic_instance);
    bsp_iic_profile_t *profile = bsp_iic_get_profile_slot_by_instance(iic_instance);
    uint32_t interrupt_status;
    uint32_t controller_status;

    if (irq_context == (bsp_iic_irq_context_t *) 0) {
        return;
    }

    interrupt_status = DL_I2C_getRawInterruptStatus(iic_instance, BSP_IIC_CONTROLLER_IRQ_MASK);
    controller_status = DL_I2C_getControllerStatus(iic_instance);
    if (profile != (bsp_iic_profile_t *) 0) {
        profile->last_stage = (uint8_t) irq_context->stage;
        profile->last_controller_status = controller_status;
        profile->last_raw_interrupt_status = interrupt_status;
    }
    while (interrupt_status != 0U) {
        if ((interrupt_status & DL_I2C_INTERRUPT_CONTROLLER_NACK) != 0U) {
            if (profile != (bsp_iic_profile_t *) 0) {
                profile->last_irq_iidx = (uint32_t) DL_I2C_IIDX_CONTROLLER_NACK;
            }
            bsp_iic_disable_irq_transaction_interrupts(iic_instance);
            bsp_iic_finish_irq_transfer(irq_context, BSP_IIC_STATUS_ERROR_NACK);
            break;
        }

        if ((interrupt_status & DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST) != 0U) {
            if (profile != (bsp_iic_profile_t *) 0) {
                profile->last_irq_iidx = (uint32_t) DL_I2C_IIDX_CONTROLLER_ARBITRATION_LOST;
            }
            bsp_iic_disable_irq_transaction_interrupts(iic_instance);
            bsp_iic_finish_irq_transfer(irq_context, BSP_IIC_STATUS_ERROR_ARB_LOST);
            break;
        }

        if ((interrupt_status & DL_I2C_INTERRUPT_CONTROLLER_RXFIFO_TRIGGER) != 0U) {
            if (profile != (bsp_iic_profile_t *) 0) {
                profile->last_irq_iidx = (uint32_t) DL_I2C_IIDX_CONTROLLER_RXFIFO_TRIGGER;
                profile->rxfifo_trigger_count++;
            }
            if (irq_context->stage == BSP_IIC_IRQ_STAGE_READ) {
                bsp_iic_irq_drain_rx_fifo(iic_instance, irq_context);
                if (irq_context->rx_index >= irq_context->rx_length) {
                    DL_I2C_clearInterruptStatus(
                        iic_instance, DL_I2C_INTERRUPT_CONTROLLER_RXFIFO_TRIGGER);
                    bsp_iic_disable_irq_transaction_interrupts(iic_instance);
                    bsp_iic_finish_irq_transfer(irq_context, BSP_IIC_STATUS_OK);
                    break;
                }
            }
            DL_I2C_clearInterruptStatus(iic_instance, DL_I2C_INTERRUPT_CONTROLLER_RXFIFO_TRIGGER);
            interrupt_status = DL_I2C_getRawInterruptStatus(iic_instance, BSP_IIC_CONTROLLER_IRQ_MASK);
            continue;
        }

        if ((interrupt_status & DL_I2C_INTERRUPT_CONTROLLER_TX_DONE) != 0U) {
            if (profile != (bsp_iic_profile_t *) 0) {
                profile->last_irq_iidx = (uint32_t) DL_I2C_IIDX_CONTROLLER_TX_DONE;
            }
            DL_I2C_clearInterruptStatus(iic_instance, DL_I2C_INTERRUPT_CONTROLLER_TX_DONE);
            bsp_iic_disable_irq_transaction_interrupts(iic_instance);
            bsp_iic_finish_irq_transfer(irq_context, BSP_IIC_STATUS_OK);
            break;
        }

        if ((interrupt_status & DL_I2C_INTERRUPT_CONTROLLER_RX_DONE) != 0U) {
            if (profile != (bsp_iic_profile_t *) 0) {
                profile->last_irq_iidx = (uint32_t) DL_I2C_IIDX_CONTROLLER_RX_DONE;
                profile->rxdone_count++;
            }
            DL_I2C_clearInterruptStatus(iic_instance, DL_I2C_INTERRUPT_CONTROLLER_RX_DONE);
            bsp_iic_irq_drain_rx_fifo(iic_instance, irq_context);
            bsp_iic_disable_irq_transaction_interrupts(iic_instance);
            bsp_iic_finish_irq_transfer(irq_context, BSP_IIC_STATUS_OK);
            break;
        }

        break;
    }
}

/**
  * @brief  I2C0 中断服务函数，用于驱动 I2C0 的中断收发状态机。
  * @param  None
  * @retval None
  */
void I2C0_IRQHandler(void)
{
    bsp_iic_irq_handler_common(I2C_0_INST);
}

/**
  * @brief  I2C1 中断服务函数，用于驱动 I2C1 的中断收发状态机。
  * @param  None
  * @retval None
  */
#if (BSP_IIC_HAS_PORT1 == 1U)
void I2C1_IRQHandler(void)
{
    bsp_iic_irq_handler_common(I2C_1_INST);
}
#endif

#endif

/**
  * @brief  初始化 BSP IIC 软件封装层。
  * @param  None
  * @retval None
  */
/* ==================== BSP IIC 对外接口实现 ==================== */

void bsp_iic_init(void)
{
    /*
     * 硬件外设的真正初始化已由 SysConfig 生成代码完成。
     * 这里负责统一清空控制器残留状态，并按当前编译模式配置 IIC 中断行为。
     */
    bsp_iic_clear_controller_state(I2C_0_INST);
#if (BSP_IIC_HAS_PORT1 == 1U)
    bsp_iic_clear_controller_state(I2C_1_INST);
#endif

#if (BSP_IIC_TRANSFER_MODE == BSP_IIC_TRANSFER_MODE_IRQ)
    bsp_iic_reset_irq_context(&g_bsp_iic0_irq_context);
#if (BSP_IIC_HAS_PORT1 == 1U)
    bsp_iic_reset_irq_context(&g_bsp_iic1_irq_context);
#endif

    /*
     * I2C IRQ 中会调用 FreeRTOS 的 FromISR API，因此其中断优先级必须满足
      * FreeRTOS 的最大系统调用中断优先级约束。若保持默认更高优先级，
      * 可能出现 IRQ 能进来但任务通知无法可靠唤醒等待任务的现象。
      */
    NVIC_SetPriority(I2C_0_INST_INT_IRQN, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
#if (BSP_IIC_HAS_PORT1 == 1U)
    NVIC_SetPriority(I2C_1_INST_INT_IRQN, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
#endif

    bsp_iic_disable_irq_transaction_interrupts(I2C_0_INST);
#if (BSP_IIC_HAS_PORT1 == 1U)
    bsp_iic_disable_irq_transaction_interrupts(I2C_1_INST);
#endif

    NVIC_ClearPendingIRQ(I2C_0_INST_INT_IRQN);
    NVIC_EnableIRQ(I2C_0_INST_INT_IRQN);
#if (BSP_IIC_HAS_PORT1 == 1U)
    NVIC_ClearPendingIRQ(I2C_1_INST_INT_IRQN);
    NVIC_EnableIRQ(I2C_1_INST_INT_IRQN);
#endif
#else
    /*
     * 轮询模式下主动关闭 NVIC 和 IIC 控制器中断，避免中断标志被异步清除后影响轮询判断。
     * 这样可以确保同一套寄存器访问流程完全由轮询逻辑接管。
     */
    NVIC_DisableIRQ(I2C_0_INST_INT_IRQN);
    DL_I2C_disableInterrupt(I2C_0_INST, BSP_IIC_CONTROLLER_IRQ_MASK);
#if (BSP_IIC_HAS_PORT1 == 1U)
    NVIC_DisableIRQ(I2C_1_INST_INT_IRQN);
    DL_I2C_disableInterrupt(I2C_1_INST, BSP_IIC_CONTROLLER_IRQ_MASK);
#endif
#endif
}

/**
  * @brief  获取指定 IIC 端口的总线速率。
  * @param  iic_port：目标 IIC 端口编号。
  * @retval 返回总线速率，单位为 Hz；若端口非法，则返回 0。
  */
uint32_t bsp_iic_get_bus_speed_hz(bsp_iic_port_t iic_port)
{
    return bsp_iic_get_instance_speed_hz(iic_port);
}

const bsp_iic_profile_t *bsp_iic_get_profile(bsp_iic_port_t iic_port)
{
    return bsp_iic_get_profile_slot(iic_port);
}

/**
  * @brief  判断指定 IIC 端口上的器件是否应答。
  * @param  iic_port：目标 IIC 端口编号。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @retval 返回 IIC 操作状态，返回 BSP_IIC_STATUS_OK 表示器件存在且应答正常。
  */
bsp_iic_status_t bsp_iic_check_device(bsp_iic_port_t iic_port, uint8_t device_addr)
{
    I2C_Regs *iic_instance = bsp_iic_get_instance(iic_port);

    if (iic_instance == (I2C_Regs *) 0) {
        return BSP_IIC_STATUS_ERROR_PARAM;
    }

    return bsp_iic_probe_device(iic_instance, device_addr);
}

/**
  * @brief  向指定器件连续写入原始数据。
  * @param  iic_port：目标 IIC 端口编号。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @param  write_data：待发送数据缓冲区首地址。
  * @param  write_length：待发送数据长度，单位为字节。
  * @retval 返回 IIC 操作状态。
  */
bsp_iic_status_t bsp_iic_write_bytes(bsp_iic_port_t iic_port, uint8_t device_addr,
    const uint8_t *write_data, uint16_t write_length)
{
    I2C_Regs *iic_instance = bsp_iic_get_instance(iic_port);

    if ((iic_instance == (I2C_Regs *) 0) || (write_data == (const uint8_t *) 0) ||
        (write_length == 0U)) {
        return BSP_IIC_STATUS_ERROR_PARAM;
    }

#if (BSP_IIC_TRANSFER_MODE == BSP_IIC_TRANSFER_MODE_IRQ)
    return bsp_iic_irq_write_packet(iic_port, device_addr, write_data, write_length);
#else
    return bsp_iic_poll_write_packet(iic_instance, device_addr, write_data, write_length);
#endif
}

/**
  * @brief  从指定器件连续读取原始数据。
  * @param  iic_port：目标 IIC 端口编号。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @param  read_data：接收数据缓冲区首地址。
  * @param  read_length：待读取数据长度，单位为字节。
  * @retval 返回 IIC 操作状态。
  */
bsp_iic_status_t bsp_iic_read_bytes(bsp_iic_port_t iic_port, uint8_t device_addr,
    uint8_t *read_data, uint16_t read_length)
{
    I2C_Regs *iic_instance = bsp_iic_get_instance(iic_port);

    if ((iic_instance == (I2C_Regs *) 0) || (read_data == (uint8_t *) 0) ||
        (read_length == 0U)) {
        return BSP_IIC_STATUS_ERROR_PARAM;
    }

#if (BSP_IIC_TRANSFER_MODE == BSP_IIC_TRANSFER_MODE_IRQ)
    return bsp_iic_irq_read_packet(iic_port, device_addr, read_data, read_length);
#else
    return bsp_iic_poll_read_packet(iic_instance, device_addr, read_data, read_length);
#endif
}

/**
  * @brief  向指定器件的 8 位寄存器写入 1 个字节数据。
  * @param  iic_port：目标 IIC 端口编号。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @param  reg_addr：8 位寄存器地址。
  * @param  reg_data：待写入的寄存器数据。
  * @retval 返回 IIC 操作状态。
  */
bsp_iic_status_t bsp_iic_write_reg8(bsp_iic_port_t iic_port, uint8_t device_addr,
    uint8_t reg_addr, uint8_t reg_data)
{
    return bsp_iic_write_reg8s(iic_port, device_addr, reg_addr, &reg_data, 1U);
}

/**
  * @brief  从指定器件的 8 位寄存器读取 1 个字节数据。
  * @param  iic_port：目标 IIC 端口编号。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @param  reg_addr：8 位寄存器地址。
  * @param  reg_data：寄存器数据输出指针。
  * @retval 返回 IIC 操作状态。
  */
bsp_iic_status_t bsp_iic_read_reg8(bsp_iic_port_t iic_port, uint8_t device_addr,
    uint8_t reg_addr, uint8_t *reg_data)
{
    return bsp_iic_read_reg8s(iic_port, device_addr, reg_addr, reg_data, 1U);
}

/**
  * @brief  向指定器件的连续 8 位寄存器写入多个字节数据。
  * @param  iic_port：目标 IIC 端口编号。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @param  reg_addr：起始 8 位寄存器地址。
  * @param  write_data：待写入数据缓冲区首地址。
  * @param  write_length：待写入数据长度，单位为字节。
  * @retval 返回 IIC 操作状态。
  */
bsp_iic_status_t bsp_iic_write_reg8s(bsp_iic_port_t iic_port, uint8_t device_addr,
    uint8_t reg_addr, const uint8_t *write_data, uint16_t write_length)
{
    I2C_Regs *iic_instance = bsp_iic_get_instance(iic_port);

    if ((iic_instance == (I2C_Regs *) 0) || (write_data == (const uint8_t *) 0) ||
        (write_length == 0U)) {
        return BSP_IIC_STATUS_ERROR_PARAM;
    }

#if (BSP_IIC_TRANSFER_MODE == BSP_IIC_TRANSFER_MODE_IRQ)
    return bsp_iic_irq_write_reg_packet(iic_port, device_addr, reg_addr, write_data, write_length);
#else
    return bsp_iic_poll_write_reg_packet(
        iic_instance, device_addr, reg_addr, write_data, write_length);
#endif
}

/**
  * @brief  从指定器件的连续 8 位寄存器读取多个字节数据。
  * @param  iic_port：目标 IIC 端口编号。
  * @param  device_addr：7 位器件地址，不需要左移。
  * @param  reg_addr：起始 8 位寄存器地址。
  * @param  read_data：接收数据缓冲区首地址。
  * @param  read_length：待读取数据长度，单位为字节。
  * @retval 返回 IIC 操作状态。
  */
bsp_iic_status_t bsp_iic_read_reg8s(bsp_iic_port_t iic_port, uint8_t device_addr,
    uint8_t reg_addr, uint8_t *read_data, uint16_t read_length)
{
    I2C_Regs *iic_instance = bsp_iic_get_instance(iic_port);
    bsp_iic_status_t status;

    if ((iic_instance == (I2C_Regs *) 0) || (read_data == (uint8_t *) 0) ||
        (read_length == 0U)) {
        return BSP_IIC_STATUS_ERROR_PARAM;
    }


#if (BSP_IIC_FORCE_POLL_REG_READ == 1U)
    {
        bsp_iic_profile_t *profile = bsp_iic_get_profile_slot(iic_port);
        uint64_t total_start_us = My_GetTimeUs();
        uint64_t stage_start_us;

        if (profile != (bsp_iic_profile_t *) 0) {
            profile->transfer_length = read_length;
            profile->poll_tx_us = 0U;
            profile->poll_rx_us = 0U;
            profile->poll_tail_us = 0U;
            profile->total_us = 0U;
            profile->result_code = BSP_IIC_STATUS_OK;
        }

        stage_start_us = My_GetTimeUs();
        status = bsp_iic_poll_write_packet(iic_instance, device_addr, &reg_addr, 1U);
        if (profile != (bsp_iic_profile_t *) 0) {
            profile->poll_tx_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
        }

        if (status == BSP_IIC_STATUS_OK) {
            stage_start_us = My_GetTimeUs();
            status = bsp_iic_poll_read_packet(iic_instance, device_addr, read_data, read_length);
            if (profile != (bsp_iic_profile_t *) 0) {
                profile->poll_rx_us = (uint32_t) (My_GetTimeUs() - stage_start_us);
            }
        }

        if (profile != (bsp_iic_profile_t *) 0) {
            profile->result_code = (uint32_t) status;
            profile->total_us = (uint32_t) (My_GetTimeUs() - total_start_us);
        }
    }
#elif (BSP_IIC_TRANSFER_MODE == BSP_IIC_TRANSFER_MODE_IRQ)
    status = bsp_iic_irq_read_reg_packet(iic_port, device_addr, reg_addr, read_data, read_length);
#else
    status = bsp_iic_poll_read_reg_packet(
        iic_instance, device_addr, reg_addr, read_data, read_length);
#endif
    return status;
}
