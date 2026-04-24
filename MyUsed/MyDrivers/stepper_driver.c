/**
 * @file stepper_driver.c
 * @brief MSPM0G3507 双步进电机底层驱动实现
 * @author cube
 * @date 2026-04-11
 */

#include "stepper_driver.h"
#include <ti_msp_dl_config.h>

#define STEPPER_M1_DEFAULT_PERIOD_TICKS (28000U)
#define STEPPER_M1_DEFAULT_PULSE_TICKS  (194U)
#define STEPPER_M2_DEFAULT_PERIOD_TICKS (56000U)
#define STEPPER_M2_DEFAULT_PULSE_TICKS  (389U)

/**
 * @brief 两个步进电机的软件状态表
 *
 * - M1: TIMG0 / PA23 / PB2 / PA21
 * - M2: TIMG6 / PA29 / PB3 / PA30
 * - EN 默认按参考例程使用高电平有效
 * - DIR 默认低电平表示逻辑正向
 */
static Stepper_MotorState_t g_stepperMotors[STEPPER_MOTOR_COUNT] = {
    {
        .pwmInst = STEP_M1_PWM_INST,
        .pwmClockHz = STEP_M1_PWM_INST_CLK_FREQ,
        .pwmCcIndex = GPIO_STEP_M1_PWM_C0_IDX,
        .stepPort = GPIO_STEP_M1_PWM_C0_PORT,
        .stepPin = GPIO_STEP_M1_PWM_C0_PIN,
        .stepIomux = GPIO_STEP_M1_PWM_C0_IOMUX,
        .stepIomuxFunc = GPIO_STEP_M1_PWM_C0_IOMUX_FUNC,
        .dirPort = STEP_DIR_PORT,
        .dirPin = STEP_DIR_M1_DIR_PIN,
        .enPort = STEP_EN_PORT,
        .enPin = STEP_EN_M1_EN_PIN,
        .enPolarity = STEPPER_ENABLE_ACTIVE_HIGH,
        .forwardLevelHigh = false,
        .enabled = true,
        .running = false,
        .direction = STEPPER_DIR_FORWARD,
        .frequencyHz = STEP_M1_PWM_INST_CLK_FREQ / STEPPER_M1_DEFAULT_PERIOD_TICKS,
        .pulseWidthUs = STEPPER_DEFAULT_PULSE_WIDTH_US,
        .periodTicks = STEPPER_M1_DEFAULT_PERIOD_TICKS,
        .pulseTicks = STEPPER_M1_DEFAULT_PULSE_TICKS,
        .targetFrequencyHz = STEP_M1_PWM_INST_CLK_FREQ / STEPPER_M1_DEFAULT_PERIOD_TICKS,
        .accelHzPerMs = 0U,
        .decelHzPerMs = 0U,
    },
    {
        .pwmInst = STEP_M2_PWM_INST,
        .pwmClockHz = STEP_M2_PWM_INST_CLK_FREQ,
        .pwmCcIndex = GPIO_STEP_M2_PWM_C0_IDX,
        .stepPort = GPIO_STEP_M2_PWM_C0_PORT,
        .stepPin = GPIO_STEP_M2_PWM_C0_PIN,
        .stepIomux = GPIO_STEP_M2_PWM_C0_IOMUX,
        .stepIomuxFunc = GPIO_STEP_M2_PWM_C0_IOMUX_FUNC,
        .dirPort = STEP_DIR_PORT,
        .dirPin = STEP_DIR_M2_DIR_PIN,
        .enPort = STEP_EN_PORT,
        .enPin = STEP_EN_M2_EN_PIN,
        .enPolarity = STEPPER_ENABLE_ACTIVE_HIGH,
        .forwardLevelHigh = false,
        .enabled = true,
        .running = false,
        .direction = STEPPER_DIR_FORWARD,
        .frequencyHz = STEP_M2_PWM_INST_CLK_FREQ / STEPPER_M2_DEFAULT_PERIOD_TICKS,
        .pulseWidthUs = STEPPER_DEFAULT_PULSE_WIDTH_US,
        .periodTicks = STEPPER_M2_DEFAULT_PERIOD_TICKS,
        .pulseTicks = STEPPER_M2_DEFAULT_PULSE_TICKS,
        .targetFrequencyHz = STEP_M2_PWM_INST_CLK_FREQ / STEPPER_M2_DEFAULT_PERIOD_TICKS,
        .accelHzPerMs = 0U,
        .decelHzPerMs = 0U,
    },
};

/** @brief 同步反向控制状态 */
static Stepper_SyncReverse_t g_syncReverse = {
    .enabled = false,
    .periodMs = STEPPER_DEFAULT_SYNC_REVERSE_PERIOD_MS,
    .elapsedMs = 0U,
};

/**
 * @brief 根据电机编号返回内部状态指针
 * @param[in] motorId 电机编号
 * @return 对应状态指针，非法编号时返回 NULL
 */
static Stepper_MotorState_t *Stepper_GetMotor(Stepper_MotorId_t motorId)
{
    if ((uint32_t) motorId >= STEPPER_MOTOR_COUNT) {
        return 0;
    }

    return &g_stepperMotors[(uint32_t) motorId];
}

/**
 * @brief 将逻辑使能状态写入 EN 引脚
 * @param[in,out] motor 电机状态对象
 */
static void Stepper_ApplyEnableLevel(Stepper_MotorState_t *motor)
{
    bool pinHigh;

    if (motor == 0) {
        return;
    }

    pinHigh = (motor->enPolarity == STEPPER_ENABLE_ACTIVE_HIGH) ?
        motor->enabled : !motor->enabled;

    if (pinHigh) {
        DL_GPIO_setPins(motor->enPort, motor->enPin);
    } else {
        DL_GPIO_clearPins(motor->enPort, motor->enPin);
    }
}

/**
 * @brief 将逻辑方向状态写入 DIR 引脚
 * @param[in,out] motor 电机状态对象
 */
static void Stepper_ApplyDirectionLevel(Stepper_MotorState_t *motor)
{
    bool pinHigh;

    if (motor == 0) {
        return;
    }

    pinHigh = (motor->direction == STEPPER_DIR_FORWARD) ?
        motor->forwardLevelHigh : !motor->forwardLevelHigh;

    if (pinHigh) {
        DL_GPIO_setPins(motor->dirPort, motor->dirPin);
    } else {
        DL_GPIO_clearPins(motor->dirPort, motor->dirPin);
    }
}

/**
 * @brief 将 STEP 引脚切回 PWM 外设输出
 * @param[in,out] motor 电机状态对象
 */
static void Stepper_SwitchStepPinToPeripheral(Stepper_MotorState_t *motor)
{
    if (motor == 0) {
        return;
    }

    DL_GPIO_initPeripheralOutputFunction(motor->stepIomux, motor->stepIomuxFunc);
    DL_GPIO_enableOutput(motor->stepPort, motor->stepPin);
}

/**
 * @brief 将 STEP 引脚强制切到 GPIO 低电平
 * @param[in,out] motor 电机状态对象
 */
static void Stepper_SwitchStepPinToGpioLow(Stepper_MotorState_t *motor)
{
    if (motor == 0) {
        return;
    }

    DL_GPIO_initDigitalOutput(motor->stepIomux);
    DL_GPIO_clearPins(motor->stepPort, motor->stepPin);
    DL_GPIO_enableOutput(motor->stepPort, motor->stepPin);
}

/**
 * @brief 将 PWM 周期与比较值写回到底层定时器
 * @param[in,out] motor 电机状态对象
 */
static void Stepper_ApplyPwmRegisters(Stepper_MotorState_t *motor)
{
    uint32_t compareValue;

    if (motor == 0) {
        return;
    }

    DL_TimerG_setLoadValue(motor->pwmInst, motor->periodTicks);

    compareValue = motor->running ? motor->pulseTicks : 0U;
    DL_TimerG_setCaptureCompareValue(motor->pwmInst, compareValue,
                                     motor->pwmCcIndex);
}

/**
 * @brief 停止 STEP 输出并强制比较值为 0
 * @param[in,out] motor 电机状态对象
 */
static void Stepper_StopOutputNow(Stepper_MotorState_t *motor)
{
    if (motor == 0) {
        return;
    }

    DL_TimerG_stopCounter(motor->pwmInst);
    motor->running = false;
    DL_TimerG_setCaptureCompareValue(motor->pwmInst, 0U, motor->pwmCcIndex);
    Stepper_SwitchStepPinToGpioLow(motor);
}

/**
 * @brief 重新计算频率与脉宽对应的计数值
 * @param[in,out] motor 电机状态对象
 * @retval true 计算成功
 * @retval false 参数超出当前定时器能力
 */
static bool Stepper_RecalculateTiming(Stepper_MotorState_t *motor)
{
    uint32_t periodTicks;
    uint64_t pulseTicks;

    if ((motor == 0) || (motor->frequencyHz == 0U) || (motor->pulseWidthUs == 0U)) {
        return false;
    }

    periodTicks = motor->pwmClockHz / motor->frequencyHz;
    if (periodTicks < 2U) {
        return false;
    }

    pulseTicks = ((uint64_t) motor->pwmClockHz * (uint64_t) motor->pulseWidthUs + 999999ULL) /
                 1000000ULL;
    if (pulseTicks == 0ULL) {
        pulseTicks = 1ULL;
    }

    if (pulseTicks >= (uint64_t) periodTicks) {
        pulseTicks = (uint64_t) (periodTicks / 2U);
        if (pulseTicks == 0ULL) {
            pulseTicks = 1ULL;
        }
    }

    motor->periodTicks = periodTicks;
    motor->pulseTicks = (uint32_t) pulseTicks;
    Stepper_ApplyPwmRegisters(motor);

    return true;
}

/**
 * @brief 以同步方式翻转两个电机方向
 *
 * 为尽量避免 DIR 改变时正好落在 STEP 上升沿附近，这里先停脉冲，
 * 再改方向，最后按原状态恢复输出。
 */
static void Stepper_ToggleDirectionBoth(void)
{
    bool restartM1;
    bool restartM2;

    restartM1 = g_stepperMotors[STEPPER_MOTOR_1].running;
    restartM2 = g_stepperMotors[STEPPER_MOTOR_2].running;

    if (restartM1) {
        Stepper_StopOutputNow(&g_stepperMotors[STEPPER_MOTOR_1]);
    }
    if (restartM2) {
        Stepper_StopOutputNow(&g_stepperMotors[STEPPER_MOTOR_2]);
    }

    g_stepperMotors[STEPPER_MOTOR_1].direction =
        (g_stepperMotors[STEPPER_MOTOR_1].direction == STEPPER_DIR_FORWARD) ?
        STEPPER_DIR_REVERSE : STEPPER_DIR_FORWARD;
    g_stepperMotors[STEPPER_MOTOR_2].direction =
        (g_stepperMotors[STEPPER_MOTOR_2].direction == STEPPER_DIR_FORWARD) ?
        STEPPER_DIR_REVERSE : STEPPER_DIR_FORWARD;

    Stepper_ApplyDirectionLevel(&g_stepperMotors[STEPPER_MOTOR_1]);
    Stepper_ApplyDirectionLevel(&g_stepperMotors[STEPPER_MOTOR_2]);

    if (restartM1) {
        Stepper_Start(STEPPER_MOTOR_1);
    }
    if (restartM2) {
        Stepper_Start(STEPPER_MOTOR_2);
    }
}

void Stepper_Init(void)
{
    uint32_t i;

    for (i = 0U; i < STEPPER_MOTOR_COUNT; ++i) {
        Stepper_StopOutputNow(&g_stepperMotors[i]);
        Stepper_ApplyPwmRegisters(&g_stepperMotors[i]);
        Stepper_ApplyDirectionLevel(&g_stepperMotors[i]);
        Stepper_ApplyEnableLevel(&g_stepperMotors[i]);
    }

    g_syncReverse.enabled = false;
    g_syncReverse.periodMs = STEPPER_DEFAULT_SYNC_REVERSE_PERIOD_MS;
    g_syncReverse.elapsedMs = 0U;

    DL_TimerG_stopCounter(STEP_CTRL_TIMER_INST);
    DL_TimerG_setLoadValue(STEP_CTRL_TIMER_INST, STEP_CTRL_TIMER_INST_LOAD_VALUE);
    DL_TimerG_startCounter(STEP_CTRL_TIMER_INST);

    NVIC_ClearPendingIRQ(STEP_CTRL_TIMER_INST_INT_IRQN);
    NVIC_EnableIRQ(STEP_CTRL_TIMER_INST_INT_IRQN);
}

void Stepper_Enable(Stepper_MotorId_t motorId, bool enable)
{
    Stepper_MotorState_t *motor;

    motor = Stepper_GetMotor(motorId);
    if (motor == 0) {
        return;
    }

    motor->enabled = enable;
    Stepper_ApplyEnableLevel(motor);
}

void Stepper_EnableAll(bool enable)
{
    Stepper_Enable(STEPPER_MOTOR_1, enable);
    Stepper_Enable(STEPPER_MOTOR_2, enable);
}

void Stepper_SetEnablePolarity(Stepper_MotorId_t motorId,
                               Stepper_EnablePolarity_t polarity)
{
    Stepper_MotorState_t *motor;

    motor = Stepper_GetMotor(motorId);
    if (motor == 0) {
        return;
    }

    motor->enPolarity = polarity;
    Stepper_ApplyEnableLevel(motor);
}

void Stepper_SetForwardLevel(Stepper_MotorId_t motorId, bool forwardLevelHigh)
{
    Stepper_MotorState_t *motor;

    motor = Stepper_GetMotor(motorId);
    if (motor == 0) {
        return;
    }

    motor->forwardLevelHigh = forwardLevelHigh;
    Stepper_ApplyDirectionLevel(motor);
}

void Stepper_SetDirection(Stepper_MotorId_t motorId,
                          Stepper_Direction_t direction)
{
    Stepper_MotorState_t *motor;
    bool restartAfterChange;

    motor = Stepper_GetMotor(motorId);
    if (motor == 0) {
        return;
    }

    restartAfterChange = motor->running;
    if (restartAfterChange) {
        Stepper_StopOutputNow(motor);
    }

    motor->direction = direction;
    Stepper_ApplyDirectionLevel(motor);

    if (restartAfterChange) {
        Stepper_Start(motorId);
    }
}

void Stepper_ToggleDirection(Stepper_MotorId_t motorId)
{
    Stepper_MotorState_t *motor;

    motor = Stepper_GetMotor(motorId);
    if (motor == 0) {
        return;
    }

    Stepper_SetDirection(
        motorId,
        (motor->direction == STEPPER_DIR_FORWARD) ?
            STEPPER_DIR_REVERSE : STEPPER_DIR_FORWARD);
}

bool Stepper_SetFrequency(Stepper_MotorId_t motorId, uint32_t frequencyHz)
{
    Stepper_MotorState_t *motor;
    uint32_t previousFrequencyHz;
    bool result;

    motor = Stepper_GetMotor(motorId);
    if (motor == 0) {
        return false;
    }

    previousFrequencyHz = motor->frequencyHz;
    motor->frequencyHz = frequencyHz;
    result = Stepper_RecalculateTiming(motor);
    if (!result) {
        motor->frequencyHz = previousFrequencyHz;
    }

    return result;
}

bool Stepper_SetPulseWidthUs(Stepper_MotorId_t motorId, uint32_t pulseWidthUs)
{
    Stepper_MotorState_t *motor;
    uint32_t previousPulseWidthUs;
    bool result;

    motor = Stepper_GetMotor(motorId);
    if (motor == 0) {
        return false;
    }

    previousPulseWidthUs = motor->pulseWidthUs;
    motor->pulseWidthUs = pulseWidthUs;
    result = Stepper_RecalculateTiming(motor);
    if (!result) {
        motor->pulseWidthUs = previousPulseWidthUs;
    }

    return result;
}

void Stepper_Start(Stepper_MotorId_t motorId)
{
    Stepper_MotorState_t *motor;

    motor = Stepper_GetMotor(motorId);
    if (motor == 0) {
        return;
    }

    motor->running = true;
    Stepper_SwitchStepPinToPeripheral(motor);
    Stepper_ApplyPwmRegisters(motor);
    DL_TimerG_startCounter(motor->pwmInst);
}

void Stepper_Stop(Stepper_MotorId_t motorId)
{
    Stepper_MotorState_t *motor;

    motor = Stepper_GetMotor(motorId);
    if (motor == 0) {
        return;
    }

    Stepper_StopOutputNow(motor);
}

void Stepper_StartAll(void)
{
    Stepper_Start(STEPPER_MOTOR_1);
    Stepper_Start(STEPPER_MOTOR_2);
}

void Stepper_StopAll(void)
{
    Stepper_Stop(STEPPER_MOTOR_1);
    Stepper_Stop(STEPPER_MOTOR_2);
}

void Stepper_SetRampProfile(Stepper_MotorId_t motorId,
                            uint32_t targetFrequencyHz,
                            uint32_t accelHzPerMs,
                            uint32_t decelHzPerMs)
{
    Stepper_MotorState_t *motor;

    motor = Stepper_GetMotor(motorId);
    if (motor == 0) {
        return;
    }

    motor->targetFrequencyHz = targetFrequencyHz;
    motor->accelHzPerMs = accelHzPerMs;
    motor->decelHzPerMs = decelHzPerMs;
}

void Stepper_ConfigSyncReverse(bool enable, uint32_t periodMs)
{
    g_syncReverse.enabled = enable;
    g_syncReverse.periodMs = (periodMs == 0U) ?
        STEPPER_DEFAULT_SYNC_REVERSE_PERIOD_MS : periodMs;
    g_syncReverse.elapsedMs = 0U;
}

void Stepper_Service1ms(void)
{
    if (!g_syncReverse.enabled) {
        return;
    }

    g_syncReverse.elapsedMs++;
    if (g_syncReverse.elapsedMs < g_syncReverse.periodMs) {
        return;
    }

    g_syncReverse.elapsedMs = 0U;
    Stepper_ToggleDirectionBoth();
}

const Stepper_MotorState_t *Stepper_GetState(Stepper_MotorId_t motorId)
{
    return Stepper_GetMotor(motorId);
}

const Stepper_SyncReverse_t *Stepper_GetSyncReverseState(void)
{
    return &g_syncReverse;
}

void STEP_CTRL_TIMER_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(STEP_CTRL_TIMER_INST)) {
        case DL_TIMER_IIDX_ZERO:
            Stepper_Service1ms();
            break;

        default:
            break;
    }
}
