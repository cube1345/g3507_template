#include "motor_driver.h"

#define ABS(a)      (a>0 ? a:(-a))
#define MOTOR_DEADZONE_PWM   250

float Velcity_Kp=10.0f,  Velcity_Ki=2.0f,  Velcity_Kd=1.0f; //相关速度PID参数

int limit_PWM(int value,int low,int high)
{
	if(value>high) return high;
	else if(value<low) return low;
	else return value;
}

/*
 * 电机存在启动死区。
 * 当 PWM 非 0 但绝对值小于死区值时，提升到最小启动 PWM。
 */
int motor_apply_deadzone(int pwm)
{
    if (pwm == 0)
    {
        return 0;
    }

    if ((pwm > 0) && (pwm < MOTOR_DEADZONE_PWM))
    {
        return MOTOR_DEADZONE_PWM;
    }

    if ((pwm < 0) && (pwm > (-MOTOR_DEADZONE_PWM)))
    {
        return -MOTOR_DEADZONE_PWM;
    }

    return pwm;
}


void Set_PWM(int pwmL,int pwmR)
{
	 pwmL = motor_apply_deadzone(pwmL);
	 pwmR = motor_apply_deadzone(pwmR);

	 if(pwmR>=0)
    {
		DL_Timer_setCaptureCompareValue(PWM_0_INST,ABS(8000-pwmR),GPIO_PWM_0_C0_IDX);
		DL_Timer_setCaptureCompareValue(PWM_0_INST,ABS(8000),GPIO_PWM_0_C1_IDX);

    }
    else
    {
		DL_Timer_setCaptureCompareValue(PWM_0_INST,ABS(8000),GPIO_PWM_0_C0_IDX);
		DL_Timer_setCaptureCompareValue(PWM_0_INST,ABS(8000+pwmR),GPIO_PWM_0_C1_IDX);

    }

	 if(pwmL>=0)
    {
					DL_Timer_setCaptureCompareValue(PWM_1_INST,ABS(8000),GPIO_PWM_1_C0_IDX);
		DL_Timer_setCaptureCompareValue(PWM_1_INST,ABS(8000-pwmL),GPIO_PWM_1_C1_IDX);

    }
    else
    {
		DL_Timer_setCaptureCompareValue(PWM_1_INST,ABS(8000+pwmL),GPIO_PWM_1_C0_IDX);
		DL_Timer_setCaptureCompareValue(PWM_1_INST,ABS(8000),GPIO_PWM_1_C1_IDX);

    }


}

void Motor_Init()
{
	DL_Timer_startCounter(PWM_0_INST);
	DL_Timer_startCounter(PWM_1_INST);
}

/***************************************************************************
函数功能：A电机的位置式PI闭环控制
入口参数：目标速度、当前速度
返 回 值：电机PWM
***************************************************************************/
int Velocity_A(int TargetVelocity, int CurrentVelocity)
{
	int Bias;
	static int Integral_biasA = 0;   // 误差积分
	int ControlVelocityA;

	Bias = TargetVelocity - CurrentVelocity;   // 当前误差

	Integral_biasA += Bias;                    // 误差累加

	// 积分限幅，防止积分饱和
	if (Integral_biasA > 3000) Integral_biasA = 3000;
	else if (Integral_biasA < -3000) Integral_biasA = -3000;

	// 位置式PI
	ControlVelocityA = Velcity_Kp * Bias + Velcity_Ki * Integral_biasA;

	// 输出限幅
	if (ControlVelocityA > 7000) ControlVelocityA = 7000;
	else if (ControlVelocityA < -7000) ControlVelocityA = -7000;

	return ControlVelocityA;
}

/***************************************************************************
函数功能：B电机的位置式PI闭环控制
入口参数：目标速度、当前速度
返 回 值：电机PWM
***************************************************************************/
int Velocity_B(int TargetVelocity, int CurrentVelocity)
{
	int Bias;
	static int Integral_biasB = 0;   // 误差积分
	int ControlVelocityB;

	Bias = TargetVelocity - CurrentVelocity;   // 当前误差

	Integral_biasB += Bias;                    // 误差累加

	// 积分限幅，防止积分饱和
	if (Integral_biasB > 3000) Integral_biasB = 3000;
	else if (Integral_biasB < -3000) Integral_biasB = -3000;

	// 位置式PI
	ControlVelocityB = Velcity_Kp * Bias + Velcity_Ki * Integral_biasB;

	// 输出限幅
	if (ControlVelocityB > 7000) ControlVelocityB = 7000;
	else if (ControlVelocityB < -7000) ControlVelocityB = -7000;

	return ControlVelocityB;
}
