/**
 * @file yuntai_app.c
 * @brief 云台双轴步进电机应用层实现
 * @author cube
 * @date 2026-04-11
 */

#include "yuntai_app.h"
#include <math.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "my_system_lib.h"

// just for test
#define YUNTAI_TEST_TARGET_ANGLE_DEG         (10U)
#define YUNTAI_TEST_DRIVER_MICROSTEP         (32U)
#define YUNTAI_TEST_BASE_STEPS_PER_REV       (200U)
#define YUNTAI_TEST_STEP_HIGH_US             (20U)
#define YUNTAI_TEST_STEP_PERIOD_US           (4000U)
#define YUNTAI_TEST_SWITCH_GAP_MS            (300U)
#define YUNTAI_TEST_DIR_SETUP_US             (100U)
#define YUNTAI_TEST_STEP_PERIOD_MS           ((YUNTAI_TEST_STEP_PERIOD_US + 999U) / 1000U)
#define YUNTAI_TEST_SWING_PULSES             \
    (((YUNTAI_TEST_TARGET_ANGLE_DEG * YUNTAI_TEST_BASE_STEPS_PER_REV * \
       YUNTAI_TEST_DRIVER_MICROSTEP) + 180U) / 360U)

#define YUNTAI_STAB_DEFAULT_YAW_DEADBAND_DEG        (1.0f)
#define YUNTAI_STAB_DEFAULT_PITCH_DEADBAND_DEG      (0.8f)
#define YUNTAI_STAB_DEFAULT_YAW_GAIN_HZ_PER_DEG     (45.0f)
#define YUNTAI_STAB_DEFAULT_PITCH_GAIN_HZ_PER_DEG   (55.0f)
#define YUNTAI_STAB_DEFAULT_YAW_MIN_FREQ_HZ         (120U)
#define YUNTAI_STAB_DEFAULT_YAW_MAX_FREQ_HZ         (900U)
#define YUNTAI_STAB_DEFAULT_PITCH_MIN_FREQ_HZ       (120U)
#define YUNTAI_STAB_DEFAULT_PITCH_MAX_FREQ_HZ       (1200U)
#define YUNTAI_STAB_DEFAULT_REF_CAPTURE_SAMPLES     (1U)
#define YUNTAI_DIRCAL_DEFAULT_MOVE_ANGLE_DEG        (5U)

/** @brief 默认云台应用配置 */
static const Yuntai_AppConfig_t g_yuntaiDefaultConfig = {
    .yawFrequencyHz = YUNTAI_YAW_DEFAULT_FREQUENCY_HZ,
    .pitchFrequencyHz = YUNTAI_PITCH_DEFAULT_FREQUENCY_HZ,
    .yawPulseWidthUs = YUNTAI_DEFAULT_PULSE_WIDTH_US,
    .pitchPulseWidthUs = YUNTAI_DEFAULT_PULSE_WIDTH_US,
    .syncReversePeriodMs = YUNTAI_DEFAULT_SYNC_REVERSE_PERIOD_MS,
    .yawEnablePolarity = STEPPER_ENABLE_ACTIVE_HIGH,
    .pitchEnablePolarity = STEPPER_ENABLE_ACTIVE_HIGH,
    .yawForwardLevelHigh = false,
    .pitchForwardLevelHigh = false,
};

/** @brief 当前生效的云台应用配置 */
static Yuntai_AppConfig_t g_yuntaiConfig = {
    .yawFrequencyHz = YUNTAI_YAW_DEFAULT_FREQUENCY_HZ,
    .pitchFrequencyHz = YUNTAI_PITCH_DEFAULT_FREQUENCY_HZ,
    .yawPulseWidthUs = YUNTAI_DEFAULT_PULSE_WIDTH_US,
    .pitchPulseWidthUs = YUNTAI_DEFAULT_PULSE_WIDTH_US,
    .syncReversePeriodMs = YUNTAI_DEFAULT_SYNC_REVERSE_PERIOD_MS,
    .yawEnablePolarity = STEPPER_ENABLE_ACTIVE_HIGH,
    .pitchEnablePolarity = STEPPER_ENABLE_ACTIVE_HIGH,
    .yawForwardLevelHigh = false,
    .pitchForwardLevelHigh = false,
};

/** @brief 云台应用运行状态缓存 */
static Yuntai_AppState_t g_yuntaiState = {
    .initialized = false,
    .syncReverseEnabled = false,
    .yawEnabled = false,
    .pitchEnabled = false,
    .yawRunning = false,
    .pitchRunning = false,
    .yawDirection = STEPPER_DIR_FORWARD,
    .pitchDirection = STEPPER_DIR_FORWARD,
    .yawFrequencyHz = YUNTAI_YAW_DEFAULT_FREQUENCY_HZ,
    .pitchFrequencyHz = YUNTAI_PITCH_DEFAULT_FREQUENCY_HZ,
};

/** @brief 默认云台自稳参数 */
// 初始化时将默认参数带入当前参数
static const Yuntai_StabilizeConfig_t g_yuntaiDefaultStabilizeConfig = {
    .yawDeadbandDeg = YUNTAI_STAB_DEFAULT_YAW_DEADBAND_DEG,
    .pitchDeadbandDeg = YUNTAI_STAB_DEFAULT_PITCH_DEADBAND_DEG,
    .yawGainHzPerDeg = YUNTAI_STAB_DEFAULT_YAW_GAIN_HZ_PER_DEG,
    .pitchGainHzPerDeg = YUNTAI_STAB_DEFAULT_PITCH_GAIN_HZ_PER_DEG,
    .yawMinFrequencyHz = YUNTAI_STAB_DEFAULT_YAW_MIN_FREQ_HZ,
    .yawMaxFrequencyHz = YUNTAI_STAB_DEFAULT_YAW_MAX_FREQ_HZ,
    .pitchMinFrequencyHz = YUNTAI_STAB_DEFAULT_PITCH_MIN_FREQ_HZ,
    .pitchMaxFrequencyHz = YUNTAI_STAB_DEFAULT_PITCH_MAX_FREQ_HZ,
    .yawPositiveErrorForward = true,
    .pitchPositiveErrorForward = true,
    .referenceCaptureSampleCount = YUNTAI_STAB_DEFAULT_REF_CAPTURE_SAMPLES,
};

/** @brief 当前云台自稳参数 */
static Yuntai_StabilizeConfig_t g_yuntaiStabilizeConfig = {
    .yawDeadbandDeg = YUNTAI_STAB_DEFAULT_YAW_DEADBAND_DEG,
    .pitchDeadbandDeg = YUNTAI_STAB_DEFAULT_PITCH_DEADBAND_DEG,
    .yawGainHzPerDeg = YUNTAI_STAB_DEFAULT_YAW_GAIN_HZ_PER_DEG,
    .pitchGainHzPerDeg = YUNTAI_STAB_DEFAULT_PITCH_GAIN_HZ_PER_DEG,
    .yawMinFrequencyHz = YUNTAI_STAB_DEFAULT_YAW_MIN_FREQ_HZ,
    .yawMaxFrequencyHz = YUNTAI_STAB_DEFAULT_YAW_MAX_FREQ_HZ,
    .pitchMinFrequencyHz = YUNTAI_STAB_DEFAULT_PITCH_MIN_FREQ_HZ,
    .pitchMaxFrequencyHz = YUNTAI_STAB_DEFAULT_PITCH_MAX_FREQ_HZ,
    .yawPositiveErrorForward = true,
    .pitchPositiveErrorForward = true,
    .referenceCaptureSampleCount = YUNTAI_STAB_DEFAULT_REF_CAPTURE_SAMPLES,
};

/** @brief 云台自稳运行状态 */
static Yuntai_StabilizeState_t g_yuntaiStabilizeState = {
    .enabled = false,
    .imuReady = false,
    .referenceCaptured = false,
    .lastImuSampleCount = 0U,
    .referenceYawDeg = 0.0f,
    .referencePitchDeg = 0.0f,
    .referenceRollDeg = 0.0f,
    .currentYawDeg = 0.0f,
    .currentPitchDeg = 0.0f,
    .currentRollDeg = 0.0f, 
    .yawOffsetDeg = 0.0f,
    .pitchOffsetDeg = 0.0f,
    .yawErrorDeg = 0.0f,
    .pitchErrorDeg = 0.0f,
    .yawCommandFrequencyHz = 0U,
    .pitchCommandFrequencyHz = 0U,
};

/** @brief 默认方向校准参数 */
static const Yuntai_DirectionCalConfig_t g_yuntaiDefaultDirectionCalConfig = {
    .moveAngleDeg = YUNTAI_DIRCAL_DEFAULT_MOVE_ANGLE_DEG,
    .driverMicrostep = YUNTAI_TEST_DRIVER_MICROSTEP,
    .baseStepsPerRev = YUNTAI_TEST_BASE_STEPS_PER_REV,
    .stepHighUs = YUNTAI_TEST_STEP_HIGH_US,
    .stepPeriodUs = YUNTAI_TEST_STEP_PERIOD_US,
    .switchGapMs = YUNTAI_TEST_SWITCH_GAP_MS,
    .dirSetupUs = YUNTAI_TEST_DIR_SETUP_US,
};

/** @brief 当前方向校准参数 */
static Yuntai_DirectionCalConfig_t g_yuntaiDirectionCalConfig = {
    .moveAngleDeg = YUNTAI_DIRCAL_DEFAULT_MOVE_ANGLE_DEG,
    .driverMicrostep = YUNTAI_TEST_DRIVER_MICROSTEP,
    .baseStepsPerRev = YUNTAI_TEST_BASE_STEPS_PER_REV,
    .stepHighUs = YUNTAI_TEST_STEP_HIGH_US,
    .stepPeriodUs = YUNTAI_TEST_STEP_PERIOD_US,
    .switchGapMs = YUNTAI_TEST_SWITCH_GAP_MS,
    .dirSetupUs = YUNTAI_TEST_DIR_SETUP_US,
};

/** @brief 当前云台控制模式 */
static Yuntai_ControlMode_t g_yuntaiControlMode = YUNTAI_CONTROL_MODE_SELF_STAB;

/** @brief 云台方向校准状态 */
static Yuntai_DirectionCalState_t g_yuntaiDirectionCalState = {
    .active = false,
    .yaw = {
        .enabled = true,
        .pulseCount = 0U,
        .cycleCount = 0U,
        .lastDirection = STEPPER_DIR_FORWARD,
        .lastDirLevelHigh = false,
        .beforeForwardDeg = 0.0f,
        .afterForwardDeg = 0.0f,
        .forwardDeltaDeg = 0.0f,
        .beforeReverseDeg = 0.0f,
        .afterReverseDeg = 0.0f,
        .reverseDeltaDeg = 0.0f,
    },
    .pitch = {
        .enabled = true,
        .pulseCount = 0U,
        .cycleCount = 0U,
        .lastDirection = STEPPER_DIR_FORWARD,
        .lastDirLevelHigh = false,
        .beforeForwardDeg = 0.0f,
        .afterForwardDeg = 0.0f,
        .forwardDeltaDeg = 0.0f,
        .beforeReverseDeg = 0.0f,
        .afterReverseDeg = 0.0f,
        .reverseDeltaDeg = 0.0f,
    },
};

static void Stepmotor_ConfigStepPinsAsGpio(void);
static void Stepmotor_TestSingleMotor(Stepper_MotorId_t motorId,
                                      Stepper_Direction_t direction,
                                      uint32_t pulseCount);
static void Yuntai_Test_RunMotorTaskCycle(Stepper_MotorId_t motorId);
static void Yuntai_Test_DelayMs(uint32_t delayMs);
static void Yuntai_Test_DelayStepPeriod(void);
static void Yuntai_SanitizeStabilizeConfig(Yuntai_StabilizeConfig_t *config);
static float Yuntai_NormalizeAngleError(float targetDeg, float currentDeg);
static uint32_t Yuntai_ComputeCommandFrequency(float absErrorDeg,
                                               float deadbandDeg,
                                               uint32_t minFrequencyHz,
                                               uint32_t maxFrequencyHz,
                                               float gainHzPerDeg);
static Stepper_Direction_t Yuntai_SelectDirectionByError(float errorDeg,
                                                         bool positiveErrorForward);
static void Yuntai_SelfStab_StopAxisIfRunning(Yuntai_Axis_t axis);
static void Yuntai_SelfStab_ServiceAxis(Yuntai_Axis_t axis);
static void Yuntai_SanitizeDirectionCalConfig(Yuntai_DirectionCalConfig_t *config);
static uint32_t Yuntai_ComputeCalibrationPulseCount(const Yuntai_DirectionCalConfig_t *config);
static float Yuntai_GetAxisCurrentAngleDeg(Yuntai_Axis_t axis);
static bool Yuntai_GetDirectionOutputLevel(Yuntai_Axis_t axis, Stepper_Direction_t direction);
static Yuntai_AxisDirectionCalState_t *Yuntai_GetDirectionCalAxisState(Yuntai_Axis_t axis);
static void Yuntai_DirectionCal_ServiceAxis(Yuntai_Axis_t axis);

/**
 * @brief 将云台轴编号转换为底层电机编号
 * @param[in] axis 云台轴编号
 * @return 对应的底层电机编号
 */
static Stepper_MotorId_t Yuntai_AxisToMotorId(Yuntai_Axis_t axis)
{
    return (axis == YUNTAI_AXIS_PITCH) ? STEPPER_MOTOR_2 : STEPPER_MOTOR_1;
}

/**
 * @brief 检查云台轴编号是否合法
 * @param[in] axis 云台轴编号
 * @retval true 合法
 * @retval false 非法
 */
static bool Yuntai_AxisIsValid(Yuntai_Axis_t axis)
{
    return (axis == YUNTAI_AXIS_YAW) || (axis == YUNTAI_AXIS_PITCH);
}

/**
 * @brief 按底层驱动当前状态刷新应用层状态缓存
 */
static void Yuntai_UpdateAppState(void)
{
    const Stepper_MotorState_t *yawState;
    const Stepper_MotorState_t *pitchState;
    const Stepper_SyncReverse_t *syncState;

    yawState = Stepper_GetState(Yuntai_AxisToMotorId(YUNTAI_AXIS_YAW));
    pitchState = Stepper_GetState(Yuntai_AxisToMotorId(YUNTAI_AXIS_PITCH));
    syncState = Stepper_GetSyncReverseState();

    if ((yawState == 0) || (pitchState == 0) || (syncState == 0)) {
        return;
    }

    g_yuntaiState.yawEnabled = yawState->enabled;
    g_yuntaiState.pitchEnabled = pitchState->enabled;
    g_yuntaiState.yawRunning = yawState->running;
    g_yuntaiState.pitchRunning = pitchState->running;
    g_yuntaiState.yawDirection = yawState->direction;
    g_yuntaiState.pitchDirection = pitchState->direction;
    g_yuntaiState.yawFrequencyHz = yawState->frequencyHz;
    g_yuntaiState.pitchFrequencyHz = pitchState->frequencyHz;
    g_yuntaiState.syncReverseEnabled = syncState->enabled;
}

/**
 * @brief 对单个轴应用初始化配置
 * @param[in] axis 云台轴编号
 * @param[in] frequencyHz 频率，单位 Hz
 * @param[in] pulseWidthUs 脉宽，单位 us
 * @param[in] enablePolarity EN 有效电平
 * @param[in] forwardLevelHigh 逻辑正向对应的 DIR 电平
 */
static void Yuntai_ApplyAxisConfig(Yuntai_Axis_t axis,
                                   uint32_t frequencyHz,
                                   uint32_t pulseWidthUs,
                                   Stepper_EnablePolarity_t enablePolarity,
                                   bool forwardLevelHigh)
{
    Stepper_MotorId_t motorId;

    motorId = Yuntai_AxisToMotorId(axis);

    Stepper_SetEnablePolarity(motorId, enablePolarity);
    Stepper_SetForwardLevel(motorId, forwardLevelHigh);
    Stepper_SetDirection(motorId, STEPPER_DIR_FORWARD);
    (void) Stepper_SetFrequency(motorId, frequencyHz);
    (void) Stepper_SetPulseWidthUs(motorId, pulseWidthUs);
    Stepper_Stop(motorId);
}

/**
 * @brief 修正自稳参数中的非法组合
 * @param[in,out] config 自稳参数
 */
static void Yuntai_SanitizeStabilizeConfig(Yuntai_StabilizeConfig_t *config)
{
    if (config == 0) {
        return;
    }

    if (config->yawDeadbandDeg < 0.0f) {
        config->yawDeadbandDeg = 0.0f;
    }
    if (config->pitchDeadbandDeg < 0.0f) {
        config->pitchDeadbandDeg = 0.0f;
    }
    if (config->yawGainHzPerDeg < 0.0f) {
        config->yawGainHzPerDeg = 0.0f;
    }
    if (config->pitchGainHzPerDeg < 0.0f) {
        config->pitchGainHzPerDeg = 0.0f;
    }
    if (config->yawMinFrequencyHz == 0U) {
        config->yawMinFrequencyHz = 1U;
    }
    if (config->pitchMinFrequencyHz == 0U) {
        config->pitchMinFrequencyHz = 1U;
    }
    if (config->yawMaxFrequencyHz < config->yawMinFrequencyHz) {
        config->yawMaxFrequencyHz = config->yawMinFrequencyHz;
    }
    if (config->pitchMaxFrequencyHz < config->pitchMinFrequencyHz) {
        config->pitchMaxFrequencyHz = config->pitchMinFrequencyHz;
    }
    if (config->referenceCaptureSampleCount == 0U) {
        config->referenceCaptureSampleCount = YUNTAI_STAB_DEFAULT_REF_CAPTURE_SAMPLES;
    }
}

/**
 * @brief 计算角度归一化误差
 * @param[in] targetDeg 目标角度
 * @param[in] currentDeg 当前角度
 * @return 归一化到 [-180, 180] 的误差
 */
static float Yuntai_NormalizeAngleError(float targetDeg, float currentDeg)
{
    float errorDeg;

    errorDeg = targetDeg - currentDeg;
    while (errorDeg > 180.0f) {
        errorDeg -= 360.0f;
    }
    while (errorDeg < -180.0f) {
        errorDeg += 360.0f;
    }

    return errorDeg;
}

/**
 * @brief 按误差幅值计算步进补偿频率
 * @param[in] absErrorDeg 误差绝对值
 * @param[in] deadbandDeg 停止死区
 * @param[in] minFrequencyHz 最小输出频率
 * @param[in] maxFrequencyHz 最大输出频率
 * @param[in] gainHzPerDeg 比例增益
 * @return 频率命令，返回 0 表示停止
 */
static uint32_t Yuntai_ComputeCommandFrequency(float absErrorDeg,
                                               float deadbandDeg,
                                               uint32_t minFrequencyHz,
                                               uint32_t maxFrequencyHz,
                                               float gainHzPerDeg)
{
    float activeErrorDeg;
    float frequencyHz;

    if (absErrorDeg <= deadbandDeg) {
        return 0U;
    }

    activeErrorDeg = absErrorDeg - deadbandDeg;
    frequencyHz = (float) minFrequencyHz + activeErrorDeg * gainHzPerDeg;
    if (frequencyHz > (float) maxFrequencyHz) {
        frequencyHz = (float) maxFrequencyHz;
    }
    if (frequencyHz < (float) minFrequencyHz) {
        frequencyHz = (float) minFrequencyHz;
    }

    return (uint32_t) (frequencyHz + 0.5f);
}

/**
 * @brief 根据误差符号选择逻辑方向
 * @param[in] errorDeg 当前闭环误差
 * @param[in] positiveErrorForward 正误差是否对应逻辑正向
 * @return 逻辑方向
 */
static Stepper_Direction_t Yuntai_SelectDirectionByError(float errorDeg,
                                                         bool positiveErrorForward)
{
    if (errorDeg >= 0.0f) {
        return positiveErrorForward ? STEPPER_DIR_FORWARD : STEPPER_DIR_REVERSE;
    }

    return positiveErrorForward ? STEPPER_DIR_REVERSE : STEPPER_DIR_FORWARD;
}

/**
 * @brief 停止指定轴的脉冲输出
 * @param[in] axis 云台轴编号
 */
static void Yuntai_SelfStab_StopAxisIfRunning(Yuntai_Axis_t axis)
{
    Stepper_MotorId_t motorId;
    const Stepper_MotorState_t *motorState;

    if (!Yuntai_AxisIsValid(axis)) {
        return;
    }

    motorId = Yuntai_AxisToMotorId(axis);
    motorState = Stepper_GetState(motorId);
    if ((motorState != 0) && motorState->running) {
        Stepper_Stop(motorId);
        Yuntai_UpdateAppState();
    }
}

/**
 * @brief 修正方向校准参数中的非法组合
 * @param[in,out] config 校准参数
 */
static void Yuntai_SanitizeDirectionCalConfig(Yuntai_DirectionCalConfig_t *config)
{
    if (config == 0) {
        return;
    }

    if (config->moveAngleDeg == 0U) {
        config->moveAngleDeg = YUNTAI_DIRCAL_DEFAULT_MOVE_ANGLE_DEG;
    }
    if (config->driverMicrostep == 0U) {
        config->driverMicrostep = YUNTAI_TEST_DRIVER_MICROSTEP;
    }
    if (config->baseStepsPerRev == 0U) {
        config->baseStepsPerRev = YUNTAI_TEST_BASE_STEPS_PER_REV;
    }
    if (config->stepHighUs == 0U) {
        config->stepHighUs = YUNTAI_TEST_STEP_HIGH_US;
    }
    if (config->stepPeriodUs <= config->stepHighUs) {
        config->stepPeriodUs = YUNTAI_TEST_STEP_PERIOD_US;
    }
    if (config->switchGapMs == 0U) {
        config->switchGapMs = YUNTAI_TEST_SWITCH_GAP_MS;
    }
    if (config->dirSetupUs == 0U) {
        config->dirSetupUs = YUNTAI_TEST_DIR_SETUP_US;
    }
}

/**
 * @brief 根据方向校准参数计算单次动作脉冲数
 * @param[in] config 校准参数
 * @return 单次动作脉冲数
 */
static uint32_t Yuntai_ComputeCalibrationPulseCount(const Yuntai_DirectionCalConfig_t *config)
{
    uint32_t numerator;

    if (config == 0) {
        return 0U;
    }

    numerator = (uint32_t)config->moveAngleDeg *
                (uint32_t)config->baseStepsPerRev *
                (uint32_t)config->driverMicrostep;

    return (numerator + 180U) / 360U;
}

/**
 * @brief 获取指定轴当前 IMU 角度
 * @param[in] axis 云台轴编号
 * @return 当前角度
 */
static float Yuntai_GetAxisCurrentAngleDeg(Yuntai_Axis_t axis)
{
    float angleDeg;

    taskENTER_CRITICAL();
    angleDeg = (axis == YUNTAI_AXIS_YAW) ?
        g_yuntaiStabilizeState.currentYawDeg : g_yuntaiStabilizeState.currentPitchDeg;
    taskEXIT_CRITICAL();

    return angleDeg;
}

/**
 * @brief 获取当前方向命令对应的实际 DIR 电平
 * @param[in] axis 云台轴编号
 * @param[in] direction 逻辑方向
 * @retval true DIR 高电平
 * @retval false DIR 低电平
 */
static bool Yuntai_GetDirectionOutputLevel(Yuntai_Axis_t axis, Stepper_Direction_t direction)
{
    bool forwardLevelHigh;

    forwardLevelHigh = (axis == YUNTAI_AXIS_YAW) ?
        g_yuntaiConfig.yawForwardLevelHigh : g_yuntaiConfig.pitchForwardLevelHigh;

    return (direction == STEPPER_DIR_FORWARD) ? forwardLevelHigh : !forwardLevelHigh;
}

/**
 * @brief 根据轴编号获取方向校准状态对象
 * @param[in] axis 云台轴编号
 * @return 对应状态对象
 */
static Yuntai_AxisDirectionCalState_t *Yuntai_GetDirectionCalAxisState(Yuntai_Axis_t axis)
{
    if (axis == YUNTAI_AXIS_YAW) {
        return &g_yuntaiDirectionCalState.yaw;
    }

    return &g_yuntaiDirectionCalState.pitch;
}

void Yuntai_GetDefaultConfig(Yuntai_AppConfig_t *config)
{
    if (config == 0) {
        return;
    }

    *config = g_yuntaiDefaultConfig;
}

void Yuntai_GetDefaultStabilizeConfig(Yuntai_StabilizeConfig_t *config)
{
    if (config == 0) {
        return;
    }

    *config = g_yuntaiDefaultStabilizeConfig;
}

void Yuntai_GetDefaultDirectionCalConfig(Yuntai_DirectionCalConfig_t *config)
{
    if (config == 0) {
        return;
    }

    *config = g_yuntaiDefaultDirectionCalConfig;
}

void Yuntai_App_Init(void)
{
    Yuntai_App_InitWithConfig(&g_yuntaiDefaultConfig);
}

void Yuntai_App_InitWithConfig(const Yuntai_AppConfig_t *config)
{
    const Yuntai_AppConfig_t *activeConfig;

    activeConfig = (config == 0) ? &g_yuntaiDefaultConfig : config;
    g_yuntaiConfig = *activeConfig;

    if (g_yuntaiConfig.syncReversePeriodMs == 0U) {
        g_yuntaiConfig.syncReversePeriodMs = YUNTAI_DEFAULT_SYNC_REVERSE_PERIOD_MS;
    }

    Stepper_Init();

    Yuntai_ApplyAxisConfig(YUNTAI_AXIS_YAW,
                           g_yuntaiConfig.yawFrequencyHz,
                           g_yuntaiConfig.yawPulseWidthUs,
                           g_yuntaiConfig.yawEnablePolarity,
                           g_yuntaiConfig.yawForwardLevelHigh);

    Yuntai_ApplyAxisConfig(YUNTAI_AXIS_PITCH,
                           g_yuntaiConfig.pitchFrequencyHz,
                           g_yuntaiConfig.pitchPulseWidthUs,
                           g_yuntaiConfig.pitchEnablePolarity,
                           g_yuntaiConfig.pitchForwardLevelHigh);

    Stepper_ConfigSyncReverse(false, g_yuntaiConfig.syncReversePeriodMs);
    Stepper_EnableAll(false);
    Stepper_StopAll();

    g_yuntaiState.initialized = true;
    Yuntai_UpdateAppState();
}

void Yuntai_SelfStab_Init(void)
{
    Yuntai_SelfStab_InitWithConfig(&g_yuntaiDefaultStabilizeConfig);
}

/**
 * @brief 初始化云台自稳功能，使用指定的自稳参数配置
 * @param[in] config 自稳参数配置指针，传入 NULL 将使用默认配置
 * @details 本函数会初始化云台应用层并根据配置参数设置步进电机的初始状态，同时重置自稳状态以准备捕获参考角度
 * @author cube
 */
void Yuntai_SelfStab_InitWithConfig(const Yuntai_StabilizeConfig_t *config)
{
    const Yuntai_StabilizeConfig_t *activeConfig;

    activeConfig = (config == 0) ? &g_yuntaiDefaultStabilizeConfig : config;
    g_yuntaiStabilizeConfig = *activeConfig;
    Yuntai_SanitizeStabilizeConfig(&g_yuntaiStabilizeConfig);

    Yuntai_App_Init();
    Stepper_ConfigSyncReverse(false, g_yuntaiConfig.syncReversePeriodMs);
    Stepper_StopAll();
    Stepper_EnableAll(false);

    taskENTER_CRITICAL();
    g_yuntaiControlMode = YUNTAI_CONTROL_MODE_SELF_STAB;
    g_yuntaiDirectionCalState.active = false;
    g_yuntaiStabilizeState.enabled = true;
    g_yuntaiStabilizeState.imuReady = false;
    g_yuntaiStabilizeState.referenceCaptured = false;
    g_yuntaiStabilizeState.lastImuSampleCount = 0U;
    g_yuntaiStabilizeState.referenceYawDeg = 0.0f;
    g_yuntaiStabilizeState.referencePitchDeg = 0.0f;
    g_yuntaiStabilizeState.referenceRollDeg = 0.0f;
    g_yuntaiStabilizeState.currentYawDeg = 0.0f;
    g_yuntaiStabilizeState.currentPitchDeg = 0.0f;
    g_yuntaiStabilizeState.currentRollDeg = 0.0f;
    g_yuntaiStabilizeState.yawOffsetDeg = 0.0f;
    g_yuntaiStabilizeState.pitchOffsetDeg = 0.0f;
    g_yuntaiStabilizeState.yawErrorDeg = 0.0f;
    g_yuntaiStabilizeState.pitchErrorDeg = 0.0f;
    g_yuntaiStabilizeState.yawCommandFrequencyHz = 0U;
    g_yuntaiStabilizeState.pitchCommandFrequencyHz = 0U;
    taskEXIT_CRITICAL();

    Yuntai_UpdateAppState();
}

void Yuntai_SelfStab_Enable(bool enable)
{
    taskENTER_CRITICAL();
    g_yuntaiStabilizeState.enabled = enable;
    taskEXIT_CRITICAL();

    if (!enable) {
        Stepper_StopAll();
        Yuntai_UpdateAppState();
    } else {
        Stepper_EnableAll(true);
        Yuntai_UpdateAppState();
    }
}

void Yuntai_SelfStab_ResetReference(void)
{
    taskENTER_CRITICAL();
    g_yuntaiStabilizeState.referenceCaptured = false;
    g_yuntaiStabilizeState.referenceYawDeg = 0.0f;
    g_yuntaiStabilizeState.referencePitchDeg = 0.0f;
    g_yuntaiStabilizeState.referenceRollDeg = 0.0f;
    g_yuntaiStabilizeState.yawErrorDeg = 0.0f;
    g_yuntaiStabilizeState.pitchErrorDeg = 0.0f;
    g_yuntaiStabilizeState.yawCommandFrequencyHz = 0U;
    g_yuntaiStabilizeState.pitchCommandFrequencyHz = 0U;
    taskEXIT_CRITICAL();

    Stepper_StopAll();
    Yuntai_UpdateAppState();
}
/*
 * @brief 更新当前姿态角度和 IMU 状态，并在满足条件时捕获参考角度
 * @param[in] yawDeg 当前偏航角度，单位为度
 * @param[in] pitchDeg 当前俯仰角度，单位为度
 * @param[in] rollDeg 当前横滚角度，单位为度
 * @param[in] imuReady IMU 数据是否准备好
 * @param[in] sampleCount IMU 数据采样计数，用于判断是否达到参考捕获条件
 * @author cube
*/
void Yuntai_SelfStab_UpdateImuAngles(float yawDeg,
                                     float pitchDeg,
                                     float rollDeg,
                                     bool imuReady,
                                     uint32_t sampleCount)
{
    taskENTER_CRITICAL(); // 这个函数的含义是进入临界区，禁止中断，以保护对共享资源（这里是 g_yuntaiStabilizeState）的访问，防止数据竞争和不一致

    // g_yuntaiStabilizeState 用于存储云台数据和状态的全局变量
    g_yuntaiStabilizeState.currentYawDeg = yawDeg;
    g_yuntaiStabilizeState.currentPitchDeg = pitchDeg;
    g_yuntaiStabilizeState.currentRollDeg = rollDeg;
    g_yuntaiStabilizeState.imuReady = imuReady;
    g_yuntaiStabilizeState.lastImuSampleCount = sampleCount;

    if (g_yuntaiStabilizeState.enabled &&
        imuReady &&
        !g_yuntaiStabilizeState.referenceCaptured &&
        (sampleCount >= g_yuntaiStabilizeConfig.referenceCaptureSampleCount)) {
        g_yuntaiStabilizeState.referenceYawDeg = yawDeg;
        g_yuntaiStabilizeState.referencePitchDeg = pitchDeg;
        g_yuntaiStabilizeState.referenceRollDeg = rollDeg;
        g_yuntaiStabilizeState.referenceCaptured = true;
        g_yuntaiStabilizeState.yawErrorDeg = 0.0f;
        g_yuntaiStabilizeState.pitchErrorDeg = 0.0f;
        g_yuntaiStabilizeState.yawCommandFrequencyHz = 0U;
        g_yuntaiStabilizeState.pitchCommandFrequencyHz = 0U;
    }
    taskEXIT_CRITICAL();
}

void Yuntai_SelfStab_SetYawOffsetDeg(float offsetDeg)
{
    Yuntai_SelfStab_SetAxisOffsetDeg(YUNTAI_AXIS_YAW, offsetDeg);
}

void Yuntai_SelfStab_SetPitchOffsetDeg(float offsetDeg)
{
    Yuntai_SelfStab_SetAxisOffsetDeg(YUNTAI_AXIS_PITCH, offsetDeg);
}

void Yuntai_SelfStab_SetAxisOffsetDeg(Yuntai_Axis_t axis, float offsetDeg)
{
    if (!Yuntai_AxisIsValid(axis)) {
        return;
    }

    taskENTER_CRITICAL();
    if (axis == YUNTAI_AXIS_YAW) {
        g_yuntaiStabilizeState.yawOffsetDeg = offsetDeg;
    } else {
        g_yuntaiStabilizeState.pitchOffsetDeg = offsetDeg;
    }
    taskEXIT_CRITICAL();
}

void Yuntai_DirectionCal_Init(void)
{
    Yuntai_DirectionCal_InitWithConfig(&g_yuntaiDefaultDirectionCalConfig);
}

void Yuntai_DirectionCal_InitWithConfig(const Yuntai_DirectionCalConfig_t *config)
{
    const Yuntai_DirectionCalConfig_t *activeConfig;
    uint32_t pulseCount;

    activeConfig = (config == 0) ? &g_yuntaiDefaultDirectionCalConfig : config;
    g_yuntaiDirectionCalConfig = *activeConfig;
    Yuntai_SanitizeDirectionCalConfig(&g_yuntaiDirectionCalConfig);
    pulseCount = Yuntai_ComputeCalibrationPulseCount(&g_yuntaiDirectionCalConfig);

    Yuntai_App_Init();
    Yuntai_StopSyncSweep();
    Stepmotor_ConfigStepPinsAsGpio();
    Stepper_StopAll();
    Stepper_EnableAll(false);

    taskENTER_CRITICAL();
    g_yuntaiControlMode = YUNTAI_CONTROL_MODE_DIRECTION_CAL;
    g_yuntaiDirectionCalState.active = true;
    g_yuntaiDirectionCalState.yaw.enabled = true;
    g_yuntaiDirectionCalState.yaw.pulseCount = pulseCount;
    g_yuntaiDirectionCalState.yaw.cycleCount = 0U;
    g_yuntaiDirectionCalState.yaw.lastDirection = STEPPER_DIR_FORWARD;
    g_yuntaiDirectionCalState.yaw.lastDirLevelHigh = false;
    g_yuntaiDirectionCalState.yaw.beforeForwardDeg = 0.0f;
    g_yuntaiDirectionCalState.yaw.afterForwardDeg = 0.0f;
    g_yuntaiDirectionCalState.yaw.forwardDeltaDeg = 0.0f;
    g_yuntaiDirectionCalState.yaw.beforeReverseDeg = 0.0f;
    g_yuntaiDirectionCalState.yaw.afterReverseDeg = 0.0f;
    g_yuntaiDirectionCalState.yaw.reverseDeltaDeg = 0.0f;
    g_yuntaiDirectionCalState.pitch.enabled = true;
    g_yuntaiDirectionCalState.pitch.pulseCount = pulseCount;
    g_yuntaiDirectionCalState.pitch.cycleCount = 0U;
    g_yuntaiDirectionCalState.pitch.lastDirection = STEPPER_DIR_FORWARD;
    g_yuntaiDirectionCalState.pitch.lastDirLevelHigh = false;
    g_yuntaiDirectionCalState.pitch.beforeForwardDeg = 0.0f;
    g_yuntaiDirectionCalState.pitch.afterForwardDeg = 0.0f;
    g_yuntaiDirectionCalState.pitch.forwardDeltaDeg = 0.0f;
    g_yuntaiDirectionCalState.pitch.beforeReverseDeg = 0.0f;
    g_yuntaiDirectionCalState.pitch.afterReverseDeg = 0.0f;
    g_yuntaiDirectionCalState.pitch.reverseDeltaDeg = 0.0f;
    g_yuntaiStabilizeState.enabled = false;
    taskEXIT_CRITICAL();

    Yuntai_UpdateAppState();
}

void Yuntai_SetControlMode(Yuntai_ControlMode_t mode)
{
    taskENTER_CRITICAL();
    g_yuntaiControlMode = mode;
    g_yuntaiDirectionCalState.active =
        (mode == YUNTAI_CONTROL_MODE_DIRECTION_CAL);
    if (mode != YUNTAI_CONTROL_MODE_DIRECTION_CAL) {
        g_yuntaiStabilizeState.enabled = true;
    }
    taskEXIT_CRITICAL();
}

Yuntai_ControlMode_t Yuntai_GetControlMode(void)
{
    return g_yuntaiControlMode;
}

void Yuntai_ServiceYawTask(void)
{
    if (g_yuntaiControlMode == YUNTAI_CONTROL_MODE_DIRECTION_CAL) {
        Yuntai_DirectionCal_ServiceAxis(YUNTAI_AXIS_YAW);
        return;
    }

    Yuntai_SelfStab_ServiceYawTask();
}

void Yuntai_ServicePitchTask(void)
{
    if (g_yuntaiControlMode == YUNTAI_CONTROL_MODE_DIRECTION_CAL) {
        Yuntai_DirectionCal_ServiceAxis(YUNTAI_AXIS_PITCH);
        return;
    }

    Yuntai_SelfStab_ServicePitchTask();
}

void Yuntai_Enable(bool enable)
{
    Stepper_EnableAll(enable);
    Yuntai_UpdateAppState();
}

void Yuntai_EnableAxis(Yuntai_Axis_t axis, bool enable)
{
    if (!Yuntai_AxisIsValid(axis)) {
        return;
    }

    Stepper_Enable(Yuntai_AxisToMotorId(axis), enable);
    Yuntai_UpdateAppState();
}

void Yuntai_Start(void)
{
    Stepper_ConfigSyncReverse(false, g_yuntaiConfig.syncReversePeriodMs);
    Stepper_EnableAll(true);
    Stepper_StartAll();
    Yuntai_UpdateAppState();
}

void Yuntai_Stop(void)
{
    Stepper_ConfigSyncReverse(false, g_yuntaiConfig.syncReversePeriodMs);
    Stepper_StopAll();
    Yuntai_UpdateAppState();
}

void Yuntai_StartAxis(Yuntai_Axis_t axis)
{
    if (!Yuntai_AxisIsValid(axis)) {
        return;
    }

    Stepper_Enable(Yuntai_AxisToMotorId(axis), true);
    Stepper_Start(Yuntai_AxisToMotorId(axis));
    Yuntai_UpdateAppState();
}

void Yuntai_StopAxis(Yuntai_Axis_t axis)
{
    if (!Yuntai_AxisIsValid(axis)) {
        return;
    }

    Stepper_Stop(Yuntai_AxisToMotorId(axis));
    Yuntai_UpdateAppState();
}

void Yuntai_SetYawDirection(Stepper_Direction_t direction)
{
    Yuntai_SetAxisDirection(YUNTAI_AXIS_YAW, direction);
}

void Yuntai_SetPitchDirection(Stepper_Direction_t direction)
{
    Yuntai_SetAxisDirection(YUNTAI_AXIS_PITCH, direction);
}

void Yuntai_SetAxisDirection(Yuntai_Axis_t axis, Stepper_Direction_t direction)
{
    if (!Yuntai_AxisIsValid(axis)) {
        return;
    }

    Stepper_SetDirection(Yuntai_AxisToMotorId(axis), direction);
    Yuntai_UpdateAppState();
}

bool Yuntai_SetYawFrequency(uint32_t frequencyHz)
{
    return Yuntai_SetAxisFrequency(YUNTAI_AXIS_YAW, frequencyHz);
}

bool Yuntai_SetPitchFrequency(uint32_t frequencyHz)
{
    return Yuntai_SetAxisFrequency(YUNTAI_AXIS_PITCH, frequencyHz);
}

bool Yuntai_SetAxisFrequency(Yuntai_Axis_t axis, uint32_t frequencyHz)
{
    Stepper_MotorId_t motorId;
    bool result;

    if (!Yuntai_AxisIsValid(axis)) {
        return false;
    }

    motorId = Yuntai_AxisToMotorId(axis);
    result = Stepper_SetFrequency(motorId, frequencyHz);
    if (!result) {
        return false;
    }

    if (axis == YUNTAI_AXIS_YAW) {
        g_yuntaiConfig.yawFrequencyHz = frequencyHz;
    } else {
        g_yuntaiConfig.pitchFrequencyHz = frequencyHz;
    }

    Yuntai_UpdateAppState();
    return true;
}

bool Yuntai_SetYawPulseWidthUs(uint32_t pulseWidthUs)
{
    return Yuntai_SetAxisPulseWidthUs(YUNTAI_AXIS_YAW, pulseWidthUs);
}

bool Yuntai_SetPitchPulseWidthUs(uint32_t pulseWidthUs)
{
    return Yuntai_SetAxisPulseWidthUs(YUNTAI_AXIS_PITCH, pulseWidthUs);
}

bool Yuntai_SetAxisPulseWidthUs(Yuntai_Axis_t axis, uint32_t pulseWidthUs)
{
    Stepper_MotorId_t motorId;
    bool result;

    if (!Yuntai_AxisIsValid(axis)) {
        return false;
    }

    motorId = Yuntai_AxisToMotorId(axis);
    result = Stepper_SetPulseWidthUs(motorId, pulseWidthUs);
    if (!result) {
        return false;
    }

    if (axis == YUNTAI_AXIS_YAW) {
        g_yuntaiConfig.yawPulseWidthUs = pulseWidthUs;
    } else {
        g_yuntaiConfig.pitchPulseWidthUs = pulseWidthUs;
    }

    Yuntai_UpdateAppState();
    return true;
}

bool Yuntai_StartSyncSweep(uint32_t yawFrequencyHz,
                           uint32_t pitchFrequencyHz,
                           uint32_t reversePeriodMs)
{
    uint32_t activeReversePeriodMs;
    uint32_t previousYawFrequencyHz;
    uint32_t previousPitchFrequencyHz;

    activeReversePeriodMs = (reversePeriodMs == 0U) ?
        g_yuntaiConfig.syncReversePeriodMs : reversePeriodMs;
    previousYawFrequencyHz = g_yuntaiConfig.yawFrequencyHz;
    previousPitchFrequencyHz = g_yuntaiConfig.pitchFrequencyHz;

    if (!Yuntai_SetYawFrequency(yawFrequencyHz)) {
        return false;
    }

    if (!Yuntai_SetPitchFrequency(pitchFrequencyHz)) {
        (void) Yuntai_SetYawFrequency(previousYawFrequencyHz);
        (void) Yuntai_SetPitchFrequency(previousPitchFrequencyHz);
        return false;
    }

    g_yuntaiConfig.syncReversePeriodMs = activeReversePeriodMs;

    Stepper_SetDirection(Yuntai_AxisToMotorId(YUNTAI_AXIS_YAW), STEPPER_DIR_FORWARD);
    Stepper_SetDirection(Yuntai_AxisToMotorId(YUNTAI_AXIS_PITCH), STEPPER_DIR_FORWARD);
    Stepper_EnableAll(true);
    Stepper_ConfigSyncReverse(true, activeReversePeriodMs);
    Stepper_StartAll();

    Yuntai_UpdateAppState();
    return true;
}

bool Yuntai_StartReferenceDemo(void)
{
    return Yuntai_StartSyncSweep(YUNTAI_YAW_DEFAULT_FREQUENCY_HZ,
                                 YUNTAI_PITCH_DEFAULT_FREQUENCY_HZ,
                                 YUNTAI_DEFAULT_SYNC_REVERSE_PERIOD_MS);
}

void Yuntai_StopSyncSweep(void)
{
    Stepper_ConfigSyncReverse(false, g_yuntaiConfig.syncReversePeriodMs);
    Stepper_StopAll();
    Yuntai_UpdateAppState();
}

void Yuntai_Test_TaskMode_Init(void)
{
    Yuntai_Test_Init();
}

void Yuntai_Test_Init(void)
{
    const Yuntai_AppState_t *appState;

    appState = Yuntai_GetAppState();
    if ((appState == 0) || !appState->initialized) {
        Yuntai_App_Init();
    }

    Yuntai_StopSyncSweep();
    Stepmotor_ConfigStepPinsAsGpio();
    Yuntai_Enable(false);
}

void Yuntai_Test_ServiceYawTask(void)
{
    Yuntai_Test_ServiceM1Task();
}

void Yuntai_Test_ServicePitchTask(void)
{
    Yuntai_Test_ServiceM2Task();
}

void Yuntai_Test_ServiceM1Task(void)
{
    Yuntai_Test_RunMotorTaskCycle(STEPPER_MOTOR_1);
}

void Yuntai_Test_ServiceM2Task(void)
{
    Yuntai_Test_RunMotorTaskCycle(STEPPER_MOTOR_2);
}

void Yuntai_Test_RunCycle(void)
{
    const Yuntai_AppState_t *appState;

    appState = Yuntai_GetAppState();
    if ((appState == 0) || !appState->initialized) {
        return;
    }

    Yuntai_Test_RunMotorTaskCycle(STEPPER_MOTOR_1);
    Yuntai_Test_RunMotorTaskCycle(STEPPER_MOTOR_2);
}

const Yuntai_AppState_t *Yuntai_GetAppState(void)
{
    return &g_yuntaiState;
}

const Stepper_MotorState_t *Yuntai_GetYawState(void)
{
    return Stepper_GetState(Yuntai_AxisToMotorId(YUNTAI_AXIS_YAW));
}

const Stepper_MotorState_t *Yuntai_GetPitchState(void)
{
    return Stepper_GetState(Yuntai_AxisToMotorId(YUNTAI_AXIS_PITCH));
}

const Yuntai_StabilizeState_t *Yuntai_GetStabilizeState(void)
{
    return &g_yuntaiStabilizeState;
}

const Yuntai_DirectionCalState_t *Yuntai_GetDirectionCalState(void)
{
    return &g_yuntaiDirectionCalState;
}

/**
 * @brief 单轴方向校准服务
 * @param[in] axis 云台轴编号
 */
static void Yuntai_DirectionCal_ServiceAxis(Yuntai_Axis_t axis)
{
    Yuntai_AxisDirectionCalState_t *axisState;
    Stepper_MotorId_t motorId;

    if (!Yuntai_AxisIsValid(axis) ||
        !g_yuntaiState.initialized ||
        !g_yuntaiDirectionCalState.active) {
        return;
    }

    axisState = Yuntai_GetDirectionCalAxisState(axis);
    if ((axisState == 0) || !axisState->enabled || (axisState->pulseCount == 0U)) {
        return;
    }

    motorId = Yuntai_AxisToMotorId(axis);

    axisState->lastDirection = STEPPER_DIR_FORWARD;
    axisState->lastDirLevelHigh =
        Yuntai_GetDirectionOutputLevel(axis, STEPPER_DIR_FORWARD);
    axisState->beforeForwardDeg = Yuntai_GetAxisCurrentAngleDeg(axis);
    Stepmotor_TestSingleMotor(motorId, STEPPER_DIR_FORWARD, axisState->pulseCount);
    axisState->afterForwardDeg = Yuntai_GetAxisCurrentAngleDeg(axis);
    axisState->forwardDeltaDeg = Yuntai_NormalizeAngleError(axisState->afterForwardDeg,
                                                            axisState->beforeForwardDeg);
    Yuntai_Test_DelayMs(g_yuntaiDirectionCalConfig.switchGapMs);

    axisState->lastDirection = STEPPER_DIR_REVERSE;
    axisState->lastDirLevelHigh =
        Yuntai_GetDirectionOutputLevel(axis, STEPPER_DIR_REVERSE);
    axisState->beforeReverseDeg = Yuntai_GetAxisCurrentAngleDeg(axis);
    Stepmotor_TestSingleMotor(motorId, STEPPER_DIR_REVERSE, axisState->pulseCount);
    axisState->afterReverseDeg = Yuntai_GetAxisCurrentAngleDeg(axis);
    axisState->reverseDeltaDeg = Yuntai_NormalizeAngleError(axisState->afterReverseDeg,
                                                            axisState->beforeReverseDeg);
    axisState->cycleCount++;
    Yuntai_Test_DelayMs(g_yuntaiDirectionCalConfig.switchGapMs);
}

/**
 * @brief 单轴自稳控制服务
 * @param[in] axis 云台轴编号
 */
static void Yuntai_SelfStab_ServiceAxis(Yuntai_Axis_t axis)
{
    Yuntai_StabilizeState_t stabilizeState;
    Yuntai_StabilizeConfig_t stabilizeConfig;
    Stepper_MotorId_t motorId;
    const Stepper_MotorState_t *motorState;
    Stepper_Direction_t targetDirection;
    float targetAngleDeg;
    float currentAngleDeg;
    float errorDeg;
    float absErrorDeg;
    float deadbandDeg;
    float gainHzPerDeg;
    bool positiveErrorForward;
    uint32_t minFrequencyHz;
    uint32_t maxFrequencyHz;
    uint32_t commandFrequencyHz;
    bool stateChanged;

    if (!Yuntai_AxisIsValid(axis) || !g_yuntaiState.initialized) {
        return;
    }

    taskENTER_CRITICAL();
    stabilizeState = g_yuntaiStabilizeState;
    stabilizeConfig = g_yuntaiStabilizeConfig;
    taskEXIT_CRITICAL();

    motorId = Yuntai_AxisToMotorId(axis);
    stateChanged = false;

    if (!stabilizeState.enabled ||
        !stabilizeState.imuReady ||
        !stabilizeState.referenceCaptured) {
        taskENTER_CRITICAL();
        if (axis == YUNTAI_AXIS_YAW) {
            g_yuntaiStabilizeState.yawErrorDeg = 0.0f;
            g_yuntaiStabilizeState.yawCommandFrequencyHz = 0U;
        } else {
            g_yuntaiStabilizeState.pitchErrorDeg = 0.0f;
            g_yuntaiStabilizeState.pitchCommandFrequencyHz = 0U;
        }
        taskEXIT_CRITICAL();

        Yuntai_SelfStab_StopAxisIfRunning(axis);
        Stepper_Enable(motorId, false);
        Yuntai_UpdateAppState();
        return;
    }

    motorState = Stepper_GetState(motorId);
    if ((motorState != 0) && !motorState->enabled) {
        Stepper_Enable(motorId, true);
        Yuntai_UpdateAppState();
    }

    if (axis == YUNTAI_AXIS_YAW) {
        targetAngleDeg = stabilizeState.referenceYawDeg + stabilizeState.yawOffsetDeg;
        currentAngleDeg = stabilizeState.currentYawDeg;
        deadbandDeg = stabilizeConfig.yawDeadbandDeg;
        gainHzPerDeg = stabilizeConfig.yawGainHzPerDeg;
        minFrequencyHz = stabilizeConfig.yawMinFrequencyHz;
        maxFrequencyHz = stabilizeConfig.yawMaxFrequencyHz;
        positiveErrorForward = stabilizeConfig.yawPositiveErrorForward;
    } else {
        targetAngleDeg = stabilizeState.referencePitchDeg + stabilizeState.pitchOffsetDeg;
        currentAngleDeg = stabilizeState.currentPitchDeg;
        deadbandDeg = stabilizeConfig.pitchDeadbandDeg;
        gainHzPerDeg = stabilizeConfig.pitchGainHzPerDeg;
        minFrequencyHz = stabilizeConfig.pitchMinFrequencyHz;
        maxFrequencyHz = stabilizeConfig.pitchMaxFrequencyHz;
        positiveErrorForward = stabilizeConfig.pitchPositiveErrorForward;
    }

    errorDeg = Yuntai_NormalizeAngleError(targetAngleDeg, currentAngleDeg);
    absErrorDeg = fabsf(errorDeg);
    commandFrequencyHz = Yuntai_ComputeCommandFrequency(absErrorDeg,
                                                       deadbandDeg,
                                                       minFrequencyHz,
                                                       maxFrequencyHz,
                                                       gainHzPerDeg);

    taskENTER_CRITICAL();
    if (axis == YUNTAI_AXIS_YAW) {
        g_yuntaiStabilizeState.yawErrorDeg = errorDeg;
        g_yuntaiStabilizeState.yawCommandFrequencyHz = commandFrequencyHz;
    } else {
        g_yuntaiStabilizeState.pitchErrorDeg = errorDeg;
        g_yuntaiStabilizeState.pitchCommandFrequencyHz = commandFrequencyHz;
    }
    taskEXIT_CRITICAL();

    motorState = Stepper_GetState(motorId);
    if (commandFrequencyHz == 0U) {
        if ((motorState != 0) && motorState->running) {
            Stepper_Stop(motorId);
            Yuntai_UpdateAppState();
        }
        return;
    }

    targetDirection = Yuntai_SelectDirectionByError(errorDeg, positiveErrorForward);

    if ((motorState == 0) || (motorState->direction != targetDirection)) {
        Stepper_SetDirection(motorId, targetDirection);
        stateChanged = true;
    }

    motorState = Stepper_GetState(motorId);
    if ((motorState == 0) || (motorState->frequencyHz != commandFrequencyHz)) {
        if (Stepper_SetFrequency(motorId, commandFrequencyHz)) {
            stateChanged = true;
        }
    }

    motorState = Stepper_GetState(motorId);
    if ((motorState == 0) || !motorState->running) {
        Stepper_Start(motorId);
        stateChanged = true;
    }

    if (stateChanged) {
        Yuntai_UpdateAppState();
    }
}

void Yuntai_SelfStab_ServiceYawTask(void)
{
    Yuntai_SelfStab_ServiceAxis(YUNTAI_AXIS_YAW);
}

void Yuntai_SelfStab_ServicePitchTask(void)
{
    Yuntai_SelfStab_ServiceAxis(YUNTAI_AXIS_PITCH);
}


static void Stepmotor_ConfigStepPinsAsGpio(void)
{
    DL_GPIO_initDigitalOutput(GPIO_STEP_M1_PWM_C0_IOMUX);
    DL_GPIO_initDigitalOutput(GPIO_STEP_M2_PWM_C0_IOMUX);

    DL_GPIO_clearPins(GPIO_STEP_M1_PWM_C0_PORT, GPIO_STEP_M1_PWM_C0_PIN);
    DL_GPIO_clearPins(GPIO_STEP_M2_PWM_C0_PORT, GPIO_STEP_M2_PWM_C0_PIN);

    DL_GPIO_enableOutput(GPIO_STEP_M1_PWM_C0_PORT, GPIO_STEP_M1_PWM_C0_PIN);
    DL_GPIO_enableOutput(GPIO_STEP_M2_PWM_C0_PORT, GPIO_STEP_M2_PWM_C0_PIN);
}



// 测试代码
static void Stepmotor_TestSingleMotor(Stepper_MotorId_t motorId,
                                      Stepper_Direction_t direction,
                                      uint32_t pulseCount)
{
    const char *motorName;
    const char *dirName;
    GPIO_Regs *stepPort;
    uint32_t stepPin;
    uint32_t stepHighUs;
    uint32_t stepPeriodUs;
    uint32_t dirSetupUs;
    uint32_t i;

    if (pulseCount == 0U) {
        return;
    }

    motorName = (motorId == STEPPER_MOTOR_1) ? "M1" : "M2";
    dirName = (direction == STEPPER_DIR_FORWARD) ? "FWD" : "REV";
    stepPort = (motorId == STEPPER_MOTOR_1) ? GPIO_STEP_M1_PWM_C0_PORT : GPIO_STEP_M2_PWM_C0_PORT;
    stepPin = (motorId == STEPPER_MOTOR_1) ? GPIO_STEP_M1_PWM_C0_PIN : GPIO_STEP_M2_PWM_C0_PIN;
    if (g_yuntaiControlMode == YUNTAI_CONTROL_MODE_DIRECTION_CAL) {
        stepHighUs = g_yuntaiDirectionCalConfig.stepHighUs;
        stepPeriodUs = g_yuntaiDirectionCalConfig.stepPeriodUs;
        dirSetupUs = g_yuntaiDirectionCalConfig.dirSetupUs;
    } else {
        stepHighUs = YUNTAI_TEST_STEP_HIGH_US;
        stepPeriodUs = YUNTAI_TEST_STEP_PERIOD_US;
        dirSetupUs = YUNTAI_TEST_DIR_SETUP_US;
    }

    printf("%s %s: pulse=%u stepPeriod=%uus\r\n",
           motorName,
           dirName,
           pulseCount,
           stepPeriodUs);

    /*
     * 这里改为软件直接翻转 STEP 引脚，先把“PWM 定时器/引脚复用”这层变量排除掉。
     * 45 度仅在驱动器为整步模式时成立；若驱动器开了细分，需要把
     * YUNTAI_TEST_DRIVER_MICROSTEP 改成对应细分数。
     */
    Stepper_Enable(motorId, true);
    Stepper_SetDirection(motorId, direction);
    delay_us(dirSetupUs);

    for (i = 0U; i < pulseCount; ++i) {
        DL_GPIO_setPins(stepPort, stepPin);
        delay_us(stepHighUs);
        DL_GPIO_clearPins(stepPort, stepPin);
        Yuntai_Test_DelayStepPeriod();
    }

    Stepper_Enable(motorId, false);
}

static void Yuntai_Test_RunMotorTaskCycle(Stepper_MotorId_t motorId)
{
    const Yuntai_AppState_t *appState;

    appState = Yuntai_GetAppState();
    if ((appState == 0) || !appState->initialized) {
        return;
    }

    Stepmotor_TestSingleMotor(motorId, STEPPER_DIR_FORWARD, YUNTAI_TEST_SWING_PULSES);
    Yuntai_Test_DelayMs(YUNTAI_TEST_SWITCH_GAP_MS);

    Stepmotor_TestSingleMotor(motorId, STEPPER_DIR_REVERSE, YUNTAI_TEST_SWING_PULSES);
    Yuntai_Test_DelayMs(YUNTAI_TEST_SWITCH_GAP_MS);
}

static void Yuntai_Test_DelayMs(uint32_t delayMs)
{
    if (delayMs == 0U) {
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(delayMs));
}

static void Yuntai_Test_DelayStepPeriod(void)
{
    uint32_t stepPeriodUs;
    uint32_t stepHighUs;
    uint32_t stepPeriodMs;

    if (g_yuntaiControlMode == YUNTAI_CONTROL_MODE_DIRECTION_CAL) {
        stepPeriodUs = g_yuntaiDirectionCalConfig.stepPeriodUs;
        stepHighUs = g_yuntaiDirectionCalConfig.stepHighUs;
    } else {
        stepPeriodUs = YUNTAI_TEST_STEP_PERIOD_US;
        stepHighUs = YUNTAI_TEST_STEP_HIGH_US;
    }

    stepPeriodMs = (stepPeriodUs + 999U) / 1000U;

    if ((xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) &&
        (__get_IPSR() == 0U)) {
        vTaskDelay(pdMS_TO_TICKS(stepPeriodMs));
        return;
    }

    delay_us(stepPeriodUs - stepHighUs);
}
