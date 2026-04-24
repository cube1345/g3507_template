/**
 * @file stepper_driver.h
 * @brief MSPM0G3507 双步进电机底层驱动接口
 * @author cube
 * @date 2026-04-11
 */

#ifndef __STEPPER_DRIVER_H
#define __STEPPER_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup STEPPER_DRIVER Stepper Driver
 * @brief 基于 PUL/DIR/EN 接口的双步进电机底层驱动
 * @{
 */

/** @brief 当前工程支持的步进电机数量 */
#define STEPPER_MOTOR_COUNT                    (2U)
/** @brief 默认同步反向周期，单位 ms */
#define STEPPER_DEFAULT_SYNC_REVERSE_PERIOD_MS (5000U)
/** @brief 默认 STEP 高电平脉宽，单位 us */
#define STEPPER_DEFAULT_PULSE_WIDTH_US         (5U)

/**
 * @brief 步进电机编号
 */
typedef enum {
    STEPPER_MOTOR_1 = 0U, /**< 电机 1 */
    STEPPER_MOTOR_2 = 1U, /**< 电机 2 */
} Stepper_MotorId_t;

/**
 * @brief 步进电机逻辑方向
 */
typedef enum {
    STEPPER_DIR_FORWARD = 0U, /**< 正向 */
    STEPPER_DIR_REVERSE = 1U, /**< 反向 */
} Stepper_Direction_t;

/**
 * @brief 使能引脚的有效电平定义
 */
typedef enum {
    STEPPER_ENABLE_ACTIVE_LOW = 0U,  /**< 低电平有效 */
    STEPPER_ENABLE_ACTIVE_HIGH = 1U, /**< 高电平有效 */
} Stepper_EnablePolarity_t;

/**
 * @brief 单个步进电机的软件状态
 *
 * 该结构体既保存当前已经生效的底层参数，也预留了独立调速与简易加减速
 * 所需的目标速度与斜坡参数，方便后续在 1ms 控制节拍内扩展速度规划。
 */
typedef struct {
    GPTIMER_Regs *pwmInst;                    /**< STEP 脉冲对应的 PWM 定时器实例 */
    uint32_t pwmClockHz;                     /**< PWM 定时器输入时钟，单位 Hz */
    DL_TIMER_CC_INDEX pwmCcIndex;            /**< STEP 输出使用的比较通道 */
    GPIO_Regs *stepPort;                     /**< STEP GPIO 端口 */
    uint32_t stepPin;                        /**< STEP GPIO 引脚掩码 */
    uint32_t stepIomux;                      /**< STEP 引脚 IOMUX 配置寄存器 */
    uint32_t stepIomuxFunc;                  /**< STEP 引脚外设复用功能 */
    GPIO_Regs *dirPort;                      /**< DIR GPIO 端口 */
    uint32_t dirPin;                         /**< DIR GPIO 引脚掩码 */
    GPIO_Regs *enPort;                       /**< EN GPIO 端口 */
    uint32_t enPin;                          /**< EN GPIO 引脚掩码 */
    Stepper_EnablePolarity_t enPolarity;     /**< EN 有效电平 */
    bool forwardLevelHigh;                   /**< true 表示 DIR 高电平为正向 */
    bool enabled;                            /**< 当前逻辑使能状态 */
    bool running;                            /**< 当前是否输出 STEP 脉冲 */
    Stepper_Direction_t direction;           /**< 当前逻辑方向 */
    uint32_t frequencyHz;                    /**< 当前 STEP 频率 */
    uint32_t pulseWidthUs;                   /**< 当前 STEP 高电平脉宽 */
    uint32_t periodTicks;                    /**< 当前 PWM 周期计数值 */
    uint32_t pulseTicks;                     /**< 当前 PWM 高电平计数值 */
    uint32_t targetFrequencyHz;              /**< 预留给加减速的目标频率 */
    uint32_t accelHzPerMs;                   /**< 预留给加速规划的斜率 */
    uint32_t decelHzPerMs;                   /**< 预留给减速规划的斜率 */
} Stepper_MotorState_t;

/**
 * @brief 两电机同步反向控制状态
 */
typedef struct {
    bool enabled;        /**< 是否启用同步反向功能 */
    uint32_t periodMs;   /**< 反向周期，单位 ms */
    uint32_t elapsedMs;  /**< 已累计的控制节拍，单位 ms */
} Stepper_SyncReverse_t;

/**
 * @brief 初始化步进底层驱动
 *
 * @note 需在 @ref SYSCFG_DL_init 调用之后执行。
 * @note 该函数会按当前参考例程的语义将两个 EN 输出到有效电平，
 *       但不会自动启动 STEP 脉冲。
 */
void Stepper_Init(void);

/**
 * @brief 设置单个电机的使能状态
 * @param[in] motorId 电机编号
 * @param[in] enable true 为使能，false 为失能
 */
void Stepper_Enable(Stepper_MotorId_t motorId, bool enable);

/**
 * @brief 同时设置两个电机的使能状态
 * @param[in] enable true 为使能，false 为失能
 */
void Stepper_EnableAll(bool enable);

/**
 * @brief 设置单个电机 EN 引脚的有效电平
 * @param[in] motorId 电机编号
 * @param[in] polarity EN 有效电平定义
 */
void Stepper_SetEnablePolarity(Stepper_MotorId_t motorId,
                               Stepper_EnablePolarity_t polarity);

/**
 * @brief 设置单个电机逻辑正向对应的 DIR 电平
 * @param[in] motorId 电机编号
 * @param[in] forwardLevelHigh true 表示高电平为正向
 */
void Stepper_SetForwardLevel(Stepper_MotorId_t motorId, bool forwardLevelHigh);

/**
 * @brief 设置单个电机方向
 * @param[in] motorId 电机编号
 * @param[in] direction 目标方向
 *
 * @note 若电机正在运行，驱动层会先停脉冲，再改 DIR，再恢复输出，
 *       以减少换向瞬间的时序风险。
 */
void Stepper_SetDirection(Stepper_MotorId_t motorId,
                          Stepper_Direction_t direction);

/**
 * @brief 翻转单个电机方向
 * @param[in] motorId 电机编号
 */
void Stepper_ToggleDirection(Stepper_MotorId_t motorId);

/**
 * @brief 设置单个电机 STEP 频率
 * @param[in] motorId 电机编号
 * @param[in] frequencyHz 目标频率，单位 Hz
 * @retval true 设置成功
 * @retval false 频率参数非法或超过当前定时器能力
 */
bool Stepper_SetFrequency(Stepper_MotorId_t motorId, uint32_t frequencyHz);

/**
 * @brief 设置单个电机 STEP 高电平脉宽
 * @param[in] motorId 电机编号
 * @param[in] pulseWidthUs 目标脉宽，单位 us
 * @retval true 设置成功
 * @retval false 脉宽参数非法或与当前频率冲突
 */
bool Stepper_SetPulseWidthUs(Stepper_MotorId_t motorId, uint32_t pulseWidthUs);

/**
 * @brief 启动单个电机 STEP 脉冲输出
 * @param[in] motorId 电机编号
 */
void Stepper_Start(Stepper_MotorId_t motorId);

/**
 * @brief 停止单个电机 STEP 脉冲输出
 * @param[in] motorId 电机编号
 */
void Stepper_Stop(Stepper_MotorId_t motorId);

/**
 * @brief 同时启动两个电机 STEP 脉冲输出
 */
void Stepper_StartAll(void);

/**
 * @brief 同时停止两个电机 STEP 脉冲输出
 */
void Stepper_StopAll(void);

/**
 * @brief 保存单个电机后续速度规划所需的目标频率与斜坡参数
 * @param[in] motorId 电机编号
 * @param[in] targetFrequencyHz 目标频率
 * @param[in] accelHzPerMs 加速斜率
 * @param[in] decelHzPerMs 减速斜率
 *
 * @note 当前基础版仅保存参数，不执行自动加减速。
 */
void Stepper_SetRampProfile(Stepper_MotorId_t motorId,
                            uint32_t targetFrequencyHz,
                            uint32_t accelHzPerMs,
                            uint32_t decelHzPerMs);

/**
 * @brief 配置两电机同步反向功能
 * @param[in] enable true 启用，false 关闭
 * @param[in] periodMs 同步反向周期，单位 ms
 *
 * @note 若 periodMs 为 0，则内部回退到默认的 5000ms。
 */
void Stepper_ConfigSyncReverse(bool enable, uint32_t periodMs);

/**
 * @brief 1ms 控制节拍服务函数
 *
 * @note 当前用于处理“两个电机同频运行、每 5 秒同时反向”的基础逻辑，
 *       同时为后续扩展独立速度规划预留统一调度入口。
 */
void Stepper_Service1ms(void);

/**
 * @brief 获取单个电机当前状态
 * @param[in] motorId 电机编号
 * @return 只读状态指针，非法编号时返回 NULL
 */
const Stepper_MotorState_t *Stepper_GetState(Stepper_MotorId_t motorId);

/**
 * @brief 获取同步反向控制状态
 * @return 只读状态指针
 */
const Stepper_SyncReverse_t *Stepper_GetSyncReverseState(void);

/**
 * @brief STEP 控制定时器中断服务函数
 *
 * @note 函数名与 SysConfig 生成的 @ref STEP_CTRL_TIMER_INST_IRQHandler 保持一致。
 */
void STEP_CTRL_TIMER_INST_IRQHandler(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __STEPPER_DRIVER_H */
