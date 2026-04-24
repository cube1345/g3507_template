/**
 * @file yuntai_app.h
 * @brief 云台双轴步进电机应用层接口
 * @author cube
 * @date 2026-04-11
 */

#ifndef __YUNTAI_APP_H
#define __YUNTAI_APP_H

#include <stdbool.h>
#include <stdint.h>

#include "stepper_driver.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @defgroup YUNTAI_APP Yuntai App
 * @brief 云台双轴步进电机应用层
 *
 * 约定:
 * - M1 对应云台偏航轴 Yaw
 * - M2 对应云台俯仰轴 Pitch
 * - EN 默认按参考 STM32 工程使用高电平有效
 *
 * @{
 */

/** @brief 偏航轴默认频率，单位 Hz */
#define YUNTAI_YAW_DEFAULT_FREQUENCY_HZ       (1428U)
/** @brief 俯仰轴默认频率，单位 Hz */
#define YUNTAI_PITCH_DEFAULT_FREQUENCY_HZ     (1428U)
/** @brief 默认 STEP 高电平脉宽，单位 us */
#define YUNTAI_DEFAULT_PULSE_WIDTH_US         (5U)
/** @brief 参考例程同步反向周期，单位 ms */
#define YUNTAI_DEFAULT_SYNC_REVERSE_PERIOD_MS (5000U)

/**
 * @brief 云台轴编号
 */
typedef enum {
    YUNTAI_AXIS_YAW = 0U,   /**< 偏航轴，对应 M1 */
    YUNTAI_AXIS_PITCH = 1U, /**< 俯仰轴，对应 M2 */
} Yuntai_Axis_t;

/**
 * @brief 云台应用初始化参数
 *
 * @note `yawForwardLevelHigh` 与 `pitchForwardLevelHigh` 只描述“逻辑正向”
 *       对应的 DIR 电平，不直接等价于“左转/右转”或“抬头/低头”。
 *       机械正方向需要你上电试转后再最终确认。
 */
typedef struct {
    uint32_t yawFrequencyHz;                    /**< 偏航轴默认频率 */
    uint32_t pitchFrequencyHz;                  /**< 俯仰轴默认频率 */
    uint32_t yawPulseWidthUs;                   /**< 偏航轴 STEP 高电平脉宽 */
    uint32_t pitchPulseWidthUs;                 /**< 俯仰轴 STEP 高电平脉宽 */
    uint32_t syncReversePeriodMs;               /**< 同步反向周期 */
    Stepper_EnablePolarity_t yawEnablePolarity; /**< 偏航轴 EN 有效电平 */
    Stepper_EnablePolarity_t pitchEnablePolarity; /**< 俯仰轴 EN 有效电平 */
    bool yawForwardLevelHigh;                   /**< 偏航轴正向是否为 DIR 高电平 */
    bool pitchForwardLevelHigh;                 /**< 俯仰轴正向是否为 DIR 高电平 */
} Yuntai_AppConfig_t;

/**
 * @brief 云台应用运行状态
 */
typedef struct {
    bool initialized;                   /**< 应用层是否已初始化 */
    bool syncReverseEnabled;            /**< 当前是否启用同步往返扫动 */
    bool yawEnabled;                    /**< 偏航轴逻辑使能状态 */
    bool pitchEnabled;                  /**< 俯仰轴逻辑使能状态 */
    bool yawRunning;                    /**< 偏航轴是否在输出 STEP */
    bool pitchRunning;                  /**< 俯仰轴是否在输出 STEP */
    Stepper_Direction_t yawDirection;   /**< 偏航轴当前方向 */
    Stepper_Direction_t pitchDirection; /**< 俯仰轴当前方向 */
    uint32_t yawFrequencyHz;            /**< 偏航轴当前频率 */
    uint32_t pitchFrequencyHz;          /**< 俯仰轴当前频率 */
} Yuntai_AppState_t;

/**
 * @brief 云台自稳控制参数
 *
 * @note 该控制器本质上是“角度误差 -> 步进频率”的开环步进、闭环姿态控制。
 *       没有使用编码器，因此机械传动方向仍需按实物试转确认。
 */
typedef struct {
    float yawDeadbandDeg;               /**< 偏航轴停止死区，单位 deg */
    float pitchDeadbandDeg;             /**< 俯仰轴停止死区，单位 deg */
    float yawGainHzPerDeg;              /**< 偏航轴误差到频率的比例系数 */
    float pitchGainHzPerDeg;            /**< 俯仰轴误差到频率的比例系数 */
    uint32_t yawMinFrequencyHz;         /**< 偏航轴最小补偿频率 */
    uint32_t yawMaxFrequencyHz;         /**< 偏航轴最大补偿频率 */
    uint32_t pitchMinFrequencyHz;       /**< 俯仰轴最小补偿频率 */
    uint32_t pitchMaxFrequencyHz;       /**< 俯仰轴最大补偿频率 */
    bool yawPositiveErrorForward;       /**< 偏航正误差是否对应逻辑正向 */
    bool pitchPositiveErrorForward;     /**< 俯仰正误差是否对应逻辑正向 */
    uint32_t referenceCaptureSampleCount; /**< 采集多少帧 IMU 后锁定参考姿态 */
} Yuntai_StabilizeConfig_t;

/**
 * @brief 云台自稳运行状态
 */
typedef struct {
    bool enabled;                       /**< 自稳控制是否使能 */
    bool imuReady;                      /**< 最近一次 IMU 状态是否有效 */
    bool referenceCaptured;             /**< 是否已经锁定上电参考姿态 */
    uint32_t lastImuSampleCount;        /**< 最近一次 IMU 样本计数 */
    float referenceYawDeg;              /**< 上电锁定的偏航参考角 */
    float referencePitchDeg;            /**< 上电锁定的俯仰参考角 */
    float referenceRollDeg;             /**< 上电锁定的横滚参考角 */
    float currentYawDeg;                /**< 当前偏航角 */
    float currentPitchDeg;              /**< 当前俯仰角 */
    float currentRollDeg;               /**< 当前横滚角 */
    float yawOffsetDeg;                 /**< 偏航目标偏置，后续可用于指令角 */
    float pitchOffsetDeg;               /**< 俯仰目标偏置，后续可用于指令角 */
    float yawErrorDeg;                  /**< 偏航闭环误差 */
    float pitchErrorDeg;                /**< 俯仰闭环误差 */
    uint32_t yawCommandFrequencyHz;     /**< 偏航轴当前命令频率 */
    uint32_t pitchCommandFrequencyHz;   /**< 俯仰轴当前命令频率 */
} Yuntai_StabilizeState_t;

/**
 * @brief 云台控制模式
 */
typedef enum {
    YUNTAI_CONTROL_MODE_SELF_STAB = 0U,      /**< 自稳模式 */
    YUNTAI_CONTROL_MODE_DIRECTION_CAL = 1U,  /**< 极性/方向校准模式 */
} Yuntai_ControlMode_t;

/**
 * @brief 单轴方向校准参数
 */
typedef struct {
    uint16_t moveAngleDeg;               /**< 每次校准动作的目标角度 */
    uint16_t driverMicrostep;            /**< 驱动器细分数 */
    uint16_t baseStepsPerRev;            /**< 电机整步步数，常见为 200 */
    uint32_t stepHighUs;                 /**< STEP 高电平脉宽 */
    uint32_t stepPeriodUs;               /**< STEP 周期 */
    uint32_t switchGapMs;                /**< 正反向动作之间的等待时间 */
    uint32_t dirSetupUs;                 /**< DIR 建立时间 */
} Yuntai_DirectionCalConfig_t;

/**
 * @brief 单轴方向校准状态
 */
typedef struct {
    bool enabled;                        /**< 当前轴是否参与方向校准 */
    uint32_t pulseCount;                 /**< 本轮校准输出脉冲数 */
    uint32_t cycleCount;                 /**< 已完成的校准循环次数 */
    Stepper_Direction_t lastDirection;   /**< 最近一次命令方向 */
    bool lastDirLevelHigh;               /**< 最近一次命令时 DIR 实际电平 */
    float beforeForwardDeg;              /**< 正向动作前角度 */
    float afterForwardDeg;               /**< 正向动作后角度 */
    float forwardDeltaDeg;               /**< 正向动作引起的角度变化 */
    float beforeReverseDeg;              /**< 反向动作前角度 */
    float afterReverseDeg;               /**< 反向动作后角度 */
    float reverseDeltaDeg;               /**< 反向动作引起的角度变化 */
} Yuntai_AxisDirectionCalState_t;

/**
 * @brief 云台方向校准状态
 */
typedef struct {
    bool active;                                 /**< 是否处于方向校准模式 */
    Yuntai_AxisDirectionCalState_t yaw;          /**< 偏航轴校准状态 */
    Yuntai_AxisDirectionCalState_t pitch;        /**< 俯仰轴校准状态 */
} Yuntai_DirectionCalState_t;

/**
 * @brief 获取一份默认初始化参数
 * @param[out] config 输出配置结构体指针
 */
void Yuntai_GetDefaultConfig(Yuntai_AppConfig_t *config);

/**
 * @brief 获取一份默认自稳控制参数
 * @param[out] config 输出配置结构体指针
 */
void Yuntai_GetDefaultStabilizeConfig(Yuntai_StabilizeConfig_t *config);

/**
 * @brief 获取一份默认方向校准参数
 * @param[out] config 输出配置结构体指针
 */
void Yuntai_GetDefaultDirectionCalConfig(Yuntai_DirectionCalConfig_t *config);

/**
 * @brief 使用默认参数初始化云台应用层
 *
 * @note 该函数会调用 @ref Stepper_Init ，随后将两轴保持在“停止 + 失能”状态，
 *       不会在初始化阶段自动输出脉冲。
 */
void Yuntai_App_Init(void);

/**
 * @brief 使用自定义参数初始化云台应用层
 * @param[in] config 自定义配置，为 NULL 时回退到默认配置
 */
void Yuntai_App_InitWithConfig(const Yuntai_AppConfig_t *config);

/**
 * @brief 初始化二维云台自稳控制
 *
 * @note 该接口会保留底层 PWM 步进输出能力，不会进入 GPIO 测试模式。
 *       上电后先等待 IMU 样本达到设定数量，再自动锁定当前姿态为参考值。
 */
void Yuntai_SelfStab_Init(void);

/**
 * @brief 使用自定义自稳控制参数初始化二维云台
 * @param[in] config 自定义参数，为 NULL 时回退到默认值
 */
void Yuntai_SelfStab_InitWithConfig(const Yuntai_StabilizeConfig_t *config);

/**
 * @brief 使能或关闭自稳控制
 * @param[in] enable true 为开启自稳，false 为关闭自稳
 *
 * @note 关闭自稳时会停止 STEP 脉冲输出，但默认不自动失能驱动器。
 */
void Yuntai_SelfStab_Enable(bool enable);

/**
 * @brief 清除已锁定的参考姿态，下次达到样本门限后重新锁定
 */
void Yuntai_SelfStab_ResetReference(void);

/**
 * @brief 更新最新 IMU 姿态数据
 * @param[in] yawDeg 偏航角，单位 deg
 * @param[in] pitchDeg 俯仰角，单位 deg
 * @param[in] rollDeg 横滚角，单位 deg
 * @param[in] imuReady IMU 当前是否有效
 * @param[in] sampleCount 已累计样本数
 */
void Yuntai_SelfStab_UpdateImuAngles(float yawDeg,
                                     float pitchDeg,
                                     float rollDeg,
                                     bool imuReady,
                                     uint32_t sampleCount);

/**
 * @brief 设置偏航轴目标偏置角
 * @param[in] offsetDeg 相对上电参考姿态的偏置，单位 deg
 */
void Yuntai_SelfStab_SetYawOffsetDeg(float offsetDeg);

/**
 * @brief 设置俯仰轴目标偏置角
 * @param[in] offsetDeg 相对上电参考姿态的偏置，单位 deg
 */
void Yuntai_SelfStab_SetPitchOffsetDeg(float offsetDeg);

/**
 * @brief 设置单个轴目标偏置角
 * @param[in] axis 云台轴编号
 * @param[in] offsetDeg 相对上电参考姿态的偏置，单位 deg
 */
void Yuntai_SelfStab_SetAxisOffsetDeg(Yuntai_Axis_t axis, float offsetDeg);

/**
 * @brief 偏航轴自稳任务服务函数
 *
 * @note 适合由独立 RTOS 任务周期调用，仅负责 M1/Yaw 轴。
 */
void Yuntai_SelfStab_ServiceYawTask(void);

/**
 * @brief 俯仰轴自稳任务服务函数
 *
 * @note 适合由独立 RTOS 任务周期调用，仅负责 M2/Pitch 轴。
 */
void Yuntai_SelfStab_ServicePitchTask(void);

/**
 * @brief 初始化云台方向/极性校准模式
 *
 * @note 该模式会将 STEP 引脚切换为 GPIO 软件翻转，用小角度往返动作
 *       校验 EN 极性、DIR 方向和 IMU 角度符号是否对应。
 */
void Yuntai_DirectionCal_Init(void);

/**
 * @brief 使用自定义参数初始化云台方向校准模式
 * @param[in] config 自定义参数，为 NULL 时回退到默认值
 */
void Yuntai_DirectionCal_InitWithConfig(const Yuntai_DirectionCalConfig_t *config);

/**
 * @brief 设置云台控制模式
 * @param[in] mode 目标模式
 */
void Yuntai_SetControlMode(Yuntai_ControlMode_t mode);

/**
 * @brief 获取当前云台控制模式
 * @return 当前控制模式
 */
Yuntai_ControlMode_t Yuntai_GetControlMode(void);

/**
 * @brief 云台偏航任务统一服务入口
 *
 * @note 当前会根据控制模式自动分派到自稳服务或方向校准服务。
 */
void Yuntai_ServiceYawTask(void);

/**
 * @brief 云台俯仰任务统一服务入口
 *
 * @note 当前会根据控制模式自动分派到自稳服务或方向校准服务。
 */
void Yuntai_ServicePitchTask(void);

/**
 * @brief 同时设置两轴使能状态
 * @param[in] enable true 为使能，false 为失能
 */
void Yuntai_Enable(bool enable);

/**
 * @brief 设置单个轴使能状态
 * @param[in] axis 轴编号
 * @param[in] enable true 为使能，false 为失能
 */
void Yuntai_EnableAxis(Yuntai_Axis_t axis, bool enable);

/**
 * @brief 同时启动两轴
 *
 * @note 该接口会先使能驱动器，再启动脉冲输出；
 *       不会自动打开同步反向功能。
 */
void Yuntai_Start(void);

/**
 * @brief 同时停止两轴脉冲输出
 *
 * @note 该接口不会自动失能驱动器，因此步进驱动器仍可保持力矩。
 */
void Yuntai_Stop(void);

/**
 * @brief 启动单个轴
 * @param[in] axis 轴编号
 */
void Yuntai_StartAxis(Yuntai_Axis_t axis);

/**
 * @brief 停止单个轴
 * @param[in] axis 轴编号
 */
void Yuntai_StopAxis(Yuntai_Axis_t axis);

/**
 * @brief 设置偏航轴方向
 * @param[in] direction 目标方向
 */
void Yuntai_SetYawDirection(Stepper_Direction_t direction);

/**
 * @brief 设置俯仰轴方向
 * @param[in] direction 目标方向
 */
void Yuntai_SetPitchDirection(Stepper_Direction_t direction);

/**
 * @brief 设置单个轴方向
 * @param[in] axis 轴编号
 * @param[in] direction 目标方向
 */
void Yuntai_SetAxisDirection(Yuntai_Axis_t axis, Stepper_Direction_t direction);

/**
 * @brief 设置偏航轴频率
 * @param[in] frequencyHz 目标频率，单位 Hz
 * @retval true 设置成功
 * @retval false 参数非法
 */
bool Yuntai_SetYawFrequency(uint32_t frequencyHz);

/**
 * @brief 设置俯仰轴频率
 * @param[in] frequencyHz 目标频率，单位 Hz
 * @retval true 设置成功
 * @retval false 参数非法
 */
bool Yuntai_SetPitchFrequency(uint32_t frequencyHz);

/**
 * @brief 设置单个轴频率
 * @param[in] axis 轴编号
 * @param[in] frequencyHz 目标频率，单位 Hz
 * @retval true 设置成功
 * @retval false 参数非法
 */
bool Yuntai_SetAxisFrequency(Yuntai_Axis_t axis, uint32_t frequencyHz);

/**
 * @brief 设置偏航轴 STEP 高电平脉宽
 * @param[in] pulseWidthUs 目标脉宽，单位 us
 * @retval true 设置成功
 * @retval false 参数非法
 */
bool Yuntai_SetYawPulseWidthUs(uint32_t pulseWidthUs);

/**
 * @brief 设置俯仰轴 STEP 高电平脉宽
 * @param[in] pulseWidthUs 目标脉宽，单位 us
 * @retval true 设置成功
 * @retval false 参数非法
 */
bool Yuntai_SetPitchPulseWidthUs(uint32_t pulseWidthUs);

/**
 * @brief 设置单个轴 STEP 高电平脉宽
 * @param[in] axis 轴编号
 * @param[in] pulseWidthUs 目标脉宽，单位 us
 * @retval true 设置成功
 * @retval false 参数非法
 */
bool Yuntai_SetAxisPulseWidthUs(Yuntai_Axis_t axis, uint32_t pulseWidthUs);

/**
 * @brief 启动“两个电机同频运行、按固定周期同步反向”的扫动模式
 * @param[in] yawFrequencyHz 偏航轴频率，单位 Hz
 * @param[in] pitchFrequencyHz 俯仰轴频率，单位 Hz
 * @param[in] reversePeriodMs 同步反向周期，单位 ms，为 0 时使用当前配置值
 * @retval true 启动成功
 * @retval false 参数非法
 */
bool Yuntai_StartSyncSweep(uint32_t yawFrequencyHz,
                           uint32_t pitchFrequencyHz,
                           uint32_t reversePeriodMs);

/**
 * @brief 按参考 STM32 逻辑启动基础演示
 *
 * @note 默认行为为:
 * - 两轴频率均为约 1428Hz
 * - STEP 高电平脉宽约 5us
 * - 每 5 秒同时反向一次
 */
bool Yuntai_StartReferenceDemo(void);

/**
 * @brief 停止同步扫动模式
 */
void Yuntai_StopSyncSweep(void);

/**
 * @brief 初始化云台双轴测试任务模式
 *
 * @note 该接口用于 RTOS 进入调度前的一次性初始化，内部会完成步进测试模式
 *       所需的底层配置，后续偏航/俯仰任务只负责各自轴的测试动作。
 */
void Yuntai_Test_TaskMode_Init(void);

/**
 * @brief 初始化云台双轴测试模式
 *
 * @note 该接口会停止同步扫动、关闭底层 PWM 输出，并将 STEP 引脚切换为
 *       GPIO 软件翻转模式，供应用层测试代码使用。
 */
void Yuntai_Test_Init(void);

/**
 * @brief 偏航轴测试任务服务函数
 *
 * @note 该接口仅服务 M1/Yaw 轴，适合在独立 RTOS 任务中循环调用。
 */
void Yuntai_Test_ServiceYawTask(void);

/**
 * @brief 俯仰轴测试任务服务函数
 *
 * @note 该接口仅服务 M2/Pitch 轴，适合在独立 RTOS 任务中循环调用。
 */
void Yuntai_Test_ServicePitchTask(void);

/**
 * @brief M1 电机测试任务服务函数
 *
 * @note 当前测试模式下，M1 对应物理通道 M1，使用 STEP/DIR/EN 的 GPIO 软件翻转方式。
 */
void Yuntai_Test_ServiceM1Task(void);

/**
 * @brief M2 电机测试任务服务函数
 *
 * @note 当前测试模式下，M2 对应物理通道 M2，使用 STEP/DIR/EN 的 GPIO 软件翻转方式。
 */
void Yuntai_Test_ServiceM2Task(void);

/**
 * @brief 运行一轮云台双轴往返测试
 *
 * @note 当前测试顺序为:
 * - 偏航轴正向转动约 45 度
 * - 偏航轴反向转动约 45 度
 * - 俯仰轴正向转动约 45 度
 * - 俯仰轴反向转动约 45 度
 */
void Yuntai_Test_RunCycle(void);

/**
 * @brief 获取云台应用运行状态
 * @return 只读状态指针
 */
const Yuntai_AppState_t *Yuntai_GetAppState(void);

/**
 * @brief 获取云台自稳状态
 * @return 只读状态指针
 */
const Yuntai_StabilizeState_t *Yuntai_GetStabilizeState(void);

/**
 * @brief 获取云台方向校准状态
 * @return 只读状态指针
 */
const Yuntai_DirectionCalState_t *Yuntai_GetDirectionCalState(void);

/**
 * @brief 获取偏航轴底层状态
 * @return 只读状态指针
 */
const Stepper_MotorState_t *Yuntai_GetYawState(void);

/**
 * @brief 获取俯仰轴底层状态
 * @return 只读状态指针
 */
const Stepper_MotorState_t *Yuntai_GetPitchState(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __YUNTAI_APP_H */
