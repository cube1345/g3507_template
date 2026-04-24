#include "motor_driver.h"

#define MOTOR_ABS(a) ((a) > 0 ? (a) : (-(a)))

float Velcity_Kp = 10.0f, Velcity_Ki = 2.0f, Velcity_Kd = 1.0f;

static motor_t g_default_motor;

static inline int32_t Motor_LimitValue(int32_t value, int32_t low, int32_t high)
{
    if (value > high) {
        return high;
    }
    if (value < low) {
        return low;
    }
    return value;
}

static inline int32_t Motor_ApplyDeadzoneValue(int32_t pwm, int32_t deadzone)
{
    if (pwm == 0) {
        return 0;
    }
    if ((pwm > 0) && (pwm < deadzone)) {
        return deadzone;
    }
    if ((pwm < 0) && (pwm > -deadzone)) {
        return -deadzone;
    }
    return pwm;
}

static void Motor_WriteChannel(const motor_channel_hw_t *hw, int32_t pwm, int32_t period)
{
    if ((hw == 0) || (hw->timer == 0)) {
        return;
    }

    if (hw->reverse_logic != 0U) {
        pwm = -pwm;
    }

    if (pwm >= 0) {
        DL_Timer_setCaptureCompareValue(hw->timer, (uint32_t)MOTOR_ABS(period - pwm), hw->cc_forward);
        DL_Timer_setCaptureCompareValue(hw->timer, (uint32_t)MOTOR_ABS(period), hw->cc_reverse);
    } else {
        DL_Timer_setCaptureCompareValue(hw->timer, (uint32_t)MOTOR_ABS(period), hw->cc_forward);
        DL_Timer_setCaptureCompareValue(hw->timer, (uint32_t)MOTOR_ABS(period + pwm), hw->cc_reverse);
    }
}

static void Motor_Base_Start(void *me)
{
    Motor_StartDevice((motor_t *)me);
}

static void Motor_Base_Stop(void *me)
{
    Motor_StopDevice((motor_t *)me);
}

static void Motor_Base_SetPwm(void *me, int32_t pwm_a, int32_t pwm_b)
{
    Motor_SetPwmDevice((motor_t *)me, pwm_a, pwm_b);
}

static int32_t Motor_Base_VelocityCalc(void *me, uint8_t channel, int32_t target, int32_t current)
{
    return Motor_VelocityCalcDevice((motor_t *)me, channel, target, current);
}

static const motor_base_ops_t g_motor_base_ops = {
    .start = Motor_Base_Start,
    .stop = Motor_Base_Stop,
    .set_pwm = Motor_Base_SetPwm,
    .velocity_calc = Motor_Base_VelocityCalc,
};

const motor_base_ops_t *Motor_GetBaseOps(void)
{
    return &g_motor_base_ops;
}

void Motor_InitDevice(motor_t *me, const motor_cfg_t *cfg)
{
    if ((me == 0) || (cfg == 0)) {
        return;
    }

    me->ops = Motor_GetBaseOps();
    me->cfg = *cfg;
    me->integral_a = 0;
    me->integral_b = 0;
    me->base.is_init = 1U;
}

void Motor_StartDevice(motor_t *me)
{
    if ((me == 0) || (me->base.is_init == 0U)) {
        return;
    }

    DL_Timer_startCounter(me->cfg.left.timer);
    DL_Timer_startCounter(me->cfg.right.timer);
}

void Motor_StopDevice(motor_t *me)
{
    if ((me == 0) || (me->base.is_init == 0U)) {
        return;
    }

    Motor_SetPwmDevice(me, 0, 0);
    DL_Timer_stopCounter(me->cfg.left.timer);
    DL_Timer_stopCounter(me->cfg.right.timer);
}

void Motor_SetPwmDevice(motor_t *me, int32_t pwm_left, int32_t pwm_right)
{
    if ((me == 0) || (me->base.is_init == 0U)) {
        return;
    }

    pwm_left = Motor_LimitValue(pwm_left, -me->cfg.pwm_limit, me->cfg.pwm_limit);
    pwm_right = Motor_LimitValue(pwm_right, -me->cfg.pwm_limit, me->cfg.pwm_limit);
    pwm_left = Motor_ApplyDeadzoneValue(pwm_left, me->cfg.deadzone_pwm);
    pwm_right = Motor_ApplyDeadzoneValue(pwm_right, me->cfg.deadzone_pwm);

    Motor_WriteChannel(&me->cfg.right, pwm_right, me->cfg.period_ticks);
    Motor_WriteChannel(&me->cfg.left, pwm_left, me->cfg.period_ticks);
}

int32_t Motor_VelocityCalcDevice(motor_t *me, uint8_t channel, int32_t target, int32_t current)
{
    int32_t bias;
    int32_t *integral;
    int32_t output;

    if ((me == 0) || (me->base.is_init == 0U)) {
        return 0;
    }

    integral = (channel == MOTOR_CHANNEL_B) ? &me->integral_b : &me->integral_a;
    bias = target - current;
    *integral += bias;
    *integral = Motor_LimitValue(*integral, -me->cfg.integral_limit, me->cfg.integral_limit);

    output = (int32_t)((me->cfg.kp * (float)bias) + (me->cfg.ki * (float)(*integral)));
    return Motor_LimitValue(output, -me->cfg.pwm_limit, me->cfg.pwm_limit);
}

void Motor_Init(void)
{
    const motor_cfg_t cfg = {
        .left = {
            .timer = PWM_1_INST,
            .cc_forward = GPIO_PWM_1_C0_IDX,
            .cc_reverse = GPIO_PWM_1_C1_IDX,
            .reverse_logic = 1U,
        },
        .right = {
            .timer = PWM_0_INST,
            .cc_forward = GPIO_PWM_0_C0_IDX,
            .cc_reverse = GPIO_PWM_0_C1_IDX,
            .reverse_logic = 0U,
        },
        .period_ticks = MOTOR_DEFAULT_PERIOD_TICKS,
        .deadzone_pwm = MOTOR_DEFAULT_DEADZONE_PWM,
        .pwm_limit = MOTOR_DEFAULT_PWM_LIMIT,
        .integral_limit = MOTOR_DEFAULT_INTEGRAL_LIMIT,
        .kp = Velcity_Kp,
        .ki = Velcity_Ki,
        .kd = Velcity_Kd,
    };

    Motor_InitDevice(&g_default_motor, &cfg);
    Motor_StartDevice(&g_default_motor);
}

int Velocity_A(int TargetVelocity, int CurrentVelocity)
{
    return (int)Motor_VelocityCalcDevice(&g_default_motor, MOTOR_CHANNEL_A, TargetVelocity, CurrentVelocity);
}

int Velocity_B(int TargetVelocity, int CurrentVelocity)
{
    return (int)Motor_VelocityCalcDevice(&g_default_motor, MOTOR_CHANNEL_B, TargetVelocity, CurrentVelocity);
}

void Set_PWM(int pwma, int pwmb)
{
    Motor_SetPwmDevice(&g_default_motor, pwma, pwmb);
}

int limit_PWM(int value, int low, int high)
{
    return (int)Motor_LimitValue(value, low, high);
}

int motor_apply_deadzone(int pwm)
{
    return (int)Motor_ApplyDeadzoneValue(pwm, MOTOR_DEFAULT_DEADZONE_PWM);
}
