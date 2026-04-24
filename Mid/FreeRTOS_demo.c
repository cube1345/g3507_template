#include "FreeRTOS_demo.h"
#include "icm42688_driver.h"
#include "my_system_lib.h"
#include "yuntai_app.h"
#include "event_groups.h"
#include <stdio.h>

#define USE_CAMERA   1
#define PRINT_OUTPUT_MODE_TEXT_DEBUG      0U
#define PRINT_OUTPUT_MODE_VOFA_JUSTFLOAT  1U
#define PRINT_OUTPUT_MODE                 PRINT_OUTPUT_MODE_VOFA_JUSTFLOAT
#define IMU_EVENT_BIT_YAW                 (1U << 0)
#define IMU_EVENT_BIT_PITCH               (1U << 1)
#define IMU_EVENT_WAIT_TIMEOUT_MS         (10U)

//测试任务
void testtask(void *pvParam);
#define TESTTASK_STACK_SIZE 128
#define TESTTASK_PRIORITY 3
TaskHandle_t testtask_handle;

//循迹任务
void xunjitask(void *pvParam);
#define XUNJITASK_STACK_SIZE 64
#define XUNJITASK_PRIORITY 2
TaskHandle_t xunjitask_handle;

//陀螺仪任务
void icmtask(void *pvParam);
#define ICMTASK_STACK_SIZE 384
#define ICMTASK_PRIORITY 2
TaskHandle_t icmtask_handle;


//底盘控制任务
void cartask(void *pvParam);
#define CARTASK_STACK_SIZE 128
#define CARTASK_PRIORITY 2
TaskHandle_t cartask_handle;

void yaw_step_task(void *pvParam);
#define YAW_STEP_TASK_STACK_SIZE 512
#define YAW_STEP_TASK_PRIORITY 2
TaskHandle_t yaw_step_task_handle;

void pitch_step_task(void *pvParam);
#define PITCH_STEP_TASK_STACK_SIZE 512
#define PITCH_STEP_TASK_PRIORITY 2
TaskHandle_t pitch_step_task_handle;

volatile uint32_t g_icmtask_heartbeat = 0U;
volatile uint32_t g_yaw_step_task_heartbeat = 0U;
volatile uint32_t g_pitch_step_task_heartbeat = 0U;
volatile UBaseType_t g_icmtask_stack_hwm = 0U;
volatile UBaseType_t g_yaw_step_task_stack_hwm = 0U;
volatile UBaseType_t g_pitch_step_task_stack_hwm = 0U;
volatile TickType_t g_icmtask_last_tick = 0U;
volatile TickType_t g_yaw_step_task_last_tick = 0U;
volatile TickType_t g_pitch_step_task_last_tick = 0U;

/* IMU latest-snapshot update event source for yaw/pitch service tasks. */
static StaticEventGroup_t g_imu_event_group_buffer;
static EventGroupHandle_t g_imu_event_group = NULL;

//oled任务
void oledTask(void *pvParam);
#define OLEDTASK_STACK_SIZE 512
#define OLEDTASK_PRIORITY 1
TaskHandle_t oledtask_handle;

//声光任务
void soundlighttask(void *pvParam);
#define SOUNDLIGHTTASK_STACK_SIZE 128
#define SOUNDLIGHTTASK_PRIORITY 1
TaskHandle_t soundlighttask_handle;

//串口打印任务
void printttask(void *pvParam);
/* printf with float needs more stack than simple tasks */
#define PRINTTASK_STACK_SIZE 512
#define PRINTTASK_PRIORITY 1
TaskHandle_t printttask_handle;

//任务状况报告任务
void reporttask(void *pvParam);
#define REPORTTASK_STACK_SIZE 128
#define REPORTTASK_PRIORITY 2
TaskHandle_t reporttask_handle;

//总状态机任务
void totaltask(void *pvParam);
#define TOTALTASK_STACK_SIZE 128
#define TOTALTASK_PRIORITY 2
TaskHandle_t totaltask_handle;

static void FreeRTOS_MenuVar_Register(void);
static void Debug_PrintIcmStatus(void);
static void Debug_PrintVofaAngleTable(void);
static float Debug_NormalizeAngleDeg(float angleDeg);


#define SPEED 165


//freeRTOS启动函数
void FreeRTOS_Start(void)
{
    BaseType_t taskCreateResult;

    // FreeRTOS_MenuVar_Register();
    // g_imu_event_group = xEventGroupCreateStatic(&g_imu_event_group_buffer);
    // configASSERT(g_imu_event_group != NULL);

    // // xTaskCreate(
    // //     testtask, "testtask", TESTTASK_STACK_SIZE, NULL, TESTTASK_PRIORITY,&testtask_handle);

    // taskCreateResult = xTaskCreate(
    //     icmtask, "icmtask", ICMTASK_STACK_SIZE, NULL, ICMTASK_PRIORITY,
    //     &icmtask_handle);
    // configASSERT(taskCreateResult == pdPASS);

    // xTaskCreate(
    //     oledTask, "oledTask", OLEDTASK_STACK_SIZE, NULL,OLEDTASK_PRIORITY, &oledtask_handle);

    // xTaskCreate(
    //     soundlighttask, "soundlighttask", SOUNDLIGHTTASK_STACK_SIZE, NULL,SOUNDLIGHTTASK_PRIORITY, &soundlighttask_handle);

    // xTaskCreate(
    //     xunjitask, "xunjitask", XUNJITASK_STACK_SIZE, NULL,XUNJITASK_PRIORITY, &xunjitask_handle);

    // xTaskCreate(
    //     cartask, "cartask", CARTASK_STACK_SIZE, NULL,CARTASK_PRIORITY, &cartask_handle);

    // xTaskCreate(
    //     totaltask, "totaltask", TOTALTASK_STACK_SIZE, NULL,TOTALTASK_PRIORITY, &totaltask_handle);


    // taskCreateResult = xTaskCreate(
    //     printttask, "printttask", PRINTTASK_STACK_SIZE, NULL,
    //     PRINTTASK_PRIORITY, &printttask_handle);
    // configASSERT(taskCreateResult == pdPASS);

    // taskCreateResult = xTaskCreate(
    //     yaw_step_task, "yaw_step_task", YAW_STEP_TASK_STACK_SIZE, NULL,
    //     YAW_STEP_TASK_PRIORITY, &yaw_step_task_handle);
    // configASSERT(taskCreateResult == pdPASS);

    // taskCreateResult = xTaskCreate(
    //     pitch_step_task, "pitch_step_task", PITCH_STEP_TASK_STACK_SIZE, NULL,
    //     PITCH_STEP_TASK_PRIORITY, &pitch_step_task_handle);
    // configASSERT(taskCreateResult == pdPASS);
    // xTaskCreate(reporttask, "reporttask", REPORTTASK_STACK_SIZE, NULL,
    // REPORTTASK_PRIORITY, &reporttask_handle);

    printf("task start\r\n");
    vTaskStartScheduler();
}


//总任务状态枚举
typedef enum {
    TASK_CHOSE,
    TASK1ING,
    TASK2ING,
    TASK3ING,
    TASK4ING,
    TASK_END,
} Total_Task_State_t;

#if 1
//任务状态枚举
typedef enum {
    A2B,
    C2D,

    A2C,
    B2D,

    D2A,

    A,
    B,
    C,
    D,

    STRAIGHT_XUNJI,
    ARC_XUNJI,

    RIGHT_ANGLE_TURN,
    GO_STRAIGHT,
}TASK_STATE_t;
#endif

const char *task_state_str[] =
{
    "A2B",
    "C2D",
    "A2C",
    "B2D",
    "D2A",
    "A",
    "B",
    "C",
    "D",
    "STRAIGHT_XUNJI",
    "ARC_XUNJI",
    "RIGHT_ANGLE_TURN",
    "GO_STRAIGHT"
};

//编码器值 即左右轮速度
extern int Get_Encoder_countA;
extern int Get_Encoder_countB;

//左右轮目标速度
int g_target_speedA = 0;
int g_target_speedB = 0;



// 任务选择标志位 由 OLED 菜单选择
extern uint8_t g_task_choose_flag;

//无赛道标志位
uint8_t g_no_road_flag = 0;

//循迹标志位数组
uint8_t g_xunji[XUNJI_COUNT];

//陀螺仪数组
float g_ypr[3];

// PID参数
float yaw_kp = 0.01f;
float yaw_ki = 0.002f;
float yaw_kd = 0.0f;

// PID变量
float yaw_err = 0;
float yaw_last_err = 0;
float yaw_integral = 0;

// 输出
float yaw_pid_out = 0;

#define A2B_YAW_TARGET 0.0f
#define C2D_YAW_TARGET 160.0f
#define A2C_YAW_TARGET 35.0f
#define B2D_YAW_TARGET 165.0f

Total_Task_State_t g_total_state = TASK_CHOSE;
TASK_STATE_t g_task_state = A;

static void FreeRTOS_MenuVar_Register(void)
{
    /*
     * 当前 OLED_menu 模块使用内部静态参数表，不提供动态注册 API。
     * 因此这里保持为空，避免调用不存在的 Menu_VarView/Menu_Pid 接口。
     */
}

//角度误差归一
float Angle_Error(float target, float current)
{
    float err = target - current;

    while (err > 180.0f)  err -= 360.0f;
    while (err < -180.0f) err += 360.0f;

    return err;
}


//陀螺仪走直线pid
float Yaw_Pid(float target_yaw)
{
    // 当前误差
    // yaw_err = target_yaw - g_ypr[0];

   yaw_err = Angle_Error(target_yaw, g_ypr[0]);


    // 积分
    yaw_integral += yaw_err;

    //积分限幅
    yaw_integral = fXianfu(yaw_integral,-20,20);

    // 微分
    float yaw_derivative = yaw_err - yaw_last_err;

    // PID计算
    yaw_pid_out = yaw_kp * yaw_err +
              yaw_ki * yaw_integral +
              yaw_kd * yaw_derivative;

    // 保存上次误差
    yaw_last_err = yaw_err;

    return yaw_pid_out;
}


//无赛道持续时间
#define NO_ROAD_TIME 500

//判断是否无赛道一段时间
bool Check_No_Road_A_Long_Time(uint8_t *xunji)
{
    static TickType_t last_have_road_tick = 0;
    static bool init_flag = false;

    if (!init_flag)
    {
        last_have_road_tick = xTaskGetTickCount();
        init_flag = true;
    }

    if (!Xunji_No_Road(xunji))
    {
        // 当前检测到赛道
        last_have_road_tick = xTaskGetTickCount();
        return false;
    }

    // 当前没检测到赛道，判断持续时间是否超过200ms
    if ((xTaskGetTickCount() - last_have_road_tick) >= pdMS_TO_TICKS(NO_ROAD_TIME))
    {
        return true;
    }

    return false;
}

uint8_t g_round_cnt = 0;

#define ANGLE_STRAIGHT_TIME  1200
#define ANGLE_TIME  1000

void Car_Right_Angle_Turn(void)
{
    static uint8_t turning = 0;
    static uint8_t turn_stage = 0;
    static TickType_t start_tick = 0;

    /* 每次新进入直角转弯状态时，重新记录本次动作的起始时刻。 */
    if (turning == 0)
    {
        turning = 1;
        turn_stage = 0;
        start_tick = xTaskGetTickCount();
    }

    if (turn_stage == 0)
    {
        if ((xTaskGetTickCount() - start_tick) < pdMS_TO_TICKS(ANGLE_STRAIGHT_TIME))
        {
            Car_Run(SPEED,1.0);
            return;
        }

        turn_stage = 1;
        start_tick = xTaskGetTickCount();
    }

    if ((xTaskGetTickCount() - start_tick) < pdMS_TO_TICKS(ANGLE_TIME))
    {
        Car_Run(SPEED,1.8);   // 持续转弯
    }
    else
    {
        /* 本次直角动作结束，复位内部状态，方便下一次重新计时。 */
        turning = 0;
        turn_stage = 0;
        g_task_state = D2A;

    }
}

#define STRAIGHT_TIME  1000

void Car_Go_Straight_Time(void)
{
    static uint8_t running = 0;
    static TickType_t start_tick = 0;

    if (running == 0)
    {
        running = 1;
        start_tick = xTaskGetTickCount();
    }

    if ((xTaskGetTickCount() - start_tick) < pdMS_TO_TICKS(STRAIGHT_TIME))
    {
        Car_Run(SPEED,1.0);
    }
    else
    {
        running = 0;
        xTaskNotifyGive(soundlighttask_handle);
        g_round_cnt--;
        if (g_round_cnt == 0) {
            g_total_state = TASK_END;
        }
        else {
            g_task_state = A;
            LED_R_On();
        }
    }
}




//任务状态打印
void Print_Task_State(void)
{
    printf("TASK_STATE: %s\r\n", task_state_str[g_task_state]);
}


const char *total_state_str[] =
{
    "TASK_CHOSE",
    "TASK1ING",
    "TASK2ING",
    "TASK3ING",
    "TASK4ING",
    "TASK_END"
};

//总状态打印
void Print_Total_State(void)
{
    printf("TOTAL_STATE: %s\r\n", total_state_str[g_total_state]);
}

void Car_Yaw_Pid(float target_yaw)
{
    yaw_pid_out = 1+Yaw_Pid(target_yaw);
    yaw_pid_out = fXianfu(yaw_pid_out,0,2);
    Car_Run(SPEED,yaw_pid_out);
}


//任务1 
void Task1(void)
{
     
}

//任务2 
void Task2(void)
{

}

//任务3 
void Task3(void)
{

}



// //任务4
void Task4(void)
{

}

//任务选择函数
void Task_Choose(void) {
    //手动控制
    switch (g_task_choose_flag)
    {
        case 1:
            g_total_state = TASK1ING;
            g_task_choose_flag = 0;
            g_round_cnt =1;
            printf("task1 choose\r\n");
            break;
        case 2:
            g_total_state = TASK2ING;
            g_task_choose_flag = 0;
            g_round_cnt =1;
            printf("task2 choose\r\n");
            break;
        case 3:
            g_total_state = TASK3ING;
            g_task_choose_flag = 0;
            g_round_cnt =1;
            printf("task3 choose\r\n");
            break;
        case 4:
            g_total_state = TASK4ING;
            g_task_choose_flag = 0;
            g_round_cnt =3;
            printf("task4 choose\r\n");
            break;
        default:
            break;
    }

    //摄像头指令控制
    if (USART_App_IsQrCode2()) {
        g_total_state = TASK2ING;
        printf("task2 choose by camera\r\n");
    }
    else if (USART_App_IsQrCode3()) {
        g_total_state = TASK3ING;
        printf("task3 choose by camera\r\n");
    }
}

//总状态机任务
void totaltask(void *pvParam)
{
    (void) pvParam;
    for (;;)
    {
        // Print_Total_State();
        switch (g_total_state)
        {
            case TASK_CHOSE:
                Task_Choose();
                break;
            case TASK1ING:
                Task1();
                break;
            case TASK2ING:
                Task2();
                break;
            case TASK3ING:
                Task3();
                break;
            case TASK4ING:
                Task4();
                break;
            case TASK_END:
                Car_Run(0,1);
                // printf("task end\r\n");
                break;
            default:
                g_total_state = TASK_CHOSE;
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}




//巡迹任务
void xunjitask(void *pvParam)
{
    (void) pvParam;
    for (;;) {
        Xunji_Get(g_xunji, XUNJI_COUNT);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}


//陀螺仪任务
void icmtask(void *pvParam)
{
    TickType_t xLastWakeTime;
    (void) pvParam;

    xLastWakeTime = xTaskGetTickCount(); // 记录任务开始的时刻，用于 vTaskDelayUntil 的周期性调用
    for (;;) {
        g_icmtask_heartbeat++;  // 检查任务还在跑
        g_icmtask_last_tick = xTaskGetTickCount(); // 记录任务最后一次运行的时间
        g_icmtask_stack_hwm = uxTaskGetStackHighWaterMark(NULL); // 记录任务当前的栈高水位，监测是否有栈溢出风险
        IMU_getYawPitchRoll(g_ypr); // 获取最新的 IMU 角度数据，更新全局变量 g_ypr
        Yuntai_SelfStab_UpdateImuAngles(g_ypr[0],
                                        g_ypr[1],
                                        g_ypr[2],
                                        (IMU_IsReady() != 0U),
                                        g_imu_debug_sample_count); // 将最新的 IMU 角度和状态传递给云台自稳模块，供后续控制算法使用
        if (g_imu_event_group != NULL) { 
            (void) xEventGroupSetBits( // 通知云台控制任务有新的 IMU 数据可用，触发相应的控制更新
                g_imu_event_group, IMU_EVENT_BIT_YAW | IMU_EVENT_BIT_PITCH);
        }
        /* Fixed-rate sampling prevents jitter accumulation */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10)); // 10ms调用一次这个函数
    }
}

int g_pwmA = 0;
int g_pwmB = 0;

int g_encoderA_cnt = 0;
int g_encoderB_cnt = 0;

//底盘控制任务
void cartask(void *pvParam)
{
    (void) pvParam;

    for (;;) {


        Encoder_SyncLegacyCounts();
        g_pwmA = Velocity_A(g_target_speedA, Get_Encoder_countA);
        g_pwmB = Velocity_B(g_target_speedB, Get_Encoder_countB);
        Set_PWM(g_pwmA, g_pwmB);

        g_encoderA_cnt += Get_Encoder_countA;
        g_encoderB_cnt += Get_Encoder_countB;

        // printf("EncoderA:%d,EncoderB:%d,PWM_A:%d,PWM_B:%d\r\n",
        //     Get_Encoder_countA,Get_Encoder_countB,g_pwmA,g_pwmB);

        Encoder_ResetLegacyCounts();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void yaw_step_task(void *pvParam)
{
    const TickType_t waitTicks = pdMS_TO_TICKS(IMU_EVENT_WAIT_TIMEOUT_MS);
    (void) pvParam;

    for (;;) {
        g_yaw_step_task_heartbeat++;
        g_yaw_step_task_last_tick = xTaskGetTickCount();
        g_yaw_step_task_stack_hwm = uxTaskGetStackHighWaterMark(NULL);
        /*
         * Prefer waking on fresh IMU updates. Keep a short timeout so direction
         * calibration mode and control-mode transitions still progress even when
         * no new IMU sample arrives.
         */
        if (g_imu_event_group != NULL) {
            (void) xEventGroupWaitBits(g_imu_event_group,
                IMU_EVENT_BIT_YAW, pdTRUE, pdFALSE, waitTicks);
        } else {
            vTaskDelay(waitTicks);
        }
        Yuntai_ServiceYawTask();
    }
}

void pitch_step_task(void *pvParam)
{
    const TickType_t waitTicks = pdMS_TO_TICKS(IMU_EVENT_WAIT_TIMEOUT_MS);
    (void) pvParam;

    for (;;) {
        g_pitch_step_task_heartbeat++;
        g_pitch_step_task_last_tick = xTaskGetTickCount();
        g_pitch_step_task_stack_hwm = uxTaskGetStackHighWaterMark(NULL);
        if (g_imu_event_group != NULL) {
            (void) xEventGroupWaitBits(g_imu_event_group,
                IMU_EVENT_BIT_PITCH, pdTRUE, pdFALSE, waitTicks);
        } else {
            vTaskDelay(waitTicks);
        }
        Yuntai_ServicePitchTask();
    }
}

//声光任务
void soundlighttask(void *pvParam)
{
    (void) pvParam;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        LED_B_On();
        Buzzer_Start();
        vTaskDelay(pdMS_TO_TICKS(300));
        LED_B_Off();
        Buzzer_Stop();
    }
}

//oled任务
void oledTask(void *pvParam)
{
    TickType_t xLastWakeTime;
    (void) pvParam;

    xLastWakeTime = xTaskGetTickCount();
    for (;;) {
        Menu_Key_Handler();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
    }
}


//串口打印任务
static void Debug_PrintIcmStatus(void)
{
    char txBuf[512];
    int len;
    const Yuntai_StabilizeState_t *stabilizeState;
    const Yuntai_DirectionCalState_t *directionCalState;
    const Stepper_MotorState_t *yawMotorState;
    const Stepper_MotorState_t *pitchMotorState;
    unsigned int controlMode;
    long yaw_x100;
    long pitch_x100;
    long roll_x100;
    long acc_x1000;
    long acc_y1000;
    long acc_z1000;
    long gyro_x100;
    long gyro_y100;
    long gyro_z100;
    long ref_yaw_x100;
    long ref_pitch_x100;
    long yaw_err_x100;
    long pitch_err_x100;
    unsigned long yaw_cmd_hz;
    unsigned long pitch_cmd_hz;
    unsigned int yaw_run;
    unsigned int pitch_run;
    unsigned int yaw_en;
    unsigned int pitch_en;
    unsigned long yaw_freq;
    unsigned long pitch_freq;
    long yaw_fwd_delta_x100;
    long yaw_rev_delta_x100;
    long pitch_fwd_delta_x100;
    long pitch_rev_delta_x100;
    unsigned int yaw_dir_high;
    unsigned int pitch_dir_high;
    unsigned long yaw_cal_pulses;
    unsigned long pitch_cal_pulses;
    unsigned long yaw_cal_cycles;
    unsigned long pitch_cal_cycles;

    yaw_x100 = (long)(g_imu_debug_ypr[0] * 100.0f);
    pitch_x100 = (long)(g_imu_debug_ypr[1] * 100.0f);
    roll_x100 = (long)(g_imu_debug_ypr[2] * 100.0f);

    acc_x1000 = (long)(g_imu_debug_accel[0] * 1000.0f);
    acc_y1000 = (long)(g_imu_debug_accel[1] * 1000.0f);
    acc_z1000 = (long)(g_imu_debug_accel[2] * 1000.0f);

    gyro_x100 = (long)(g_imu_debug_gyro[0] * 100.0f);
    gyro_y100 = (long)(g_imu_debug_gyro[1] * 100.0f);
    gyro_z100 = (long)(g_imu_debug_gyro[2] * 100.0f);

    stabilizeState = Yuntai_GetStabilizeState();
    directionCalState = Yuntai_GetDirectionCalState();
    yawMotorState = Yuntai_GetYawState();
    pitchMotorState = Yuntai_GetPitchState();
    controlMode = (unsigned int)Yuntai_GetControlMode();
    if (stabilizeState != NULL) {
        ref_yaw_x100 = (long)(stabilizeState->referenceYawDeg * 100.0f);
        ref_pitch_x100 = (long)(stabilizeState->referencePitchDeg * 100.0f);
        yaw_err_x100 = (long)(stabilizeState->yawErrorDeg * 100.0f);
        pitch_err_x100 = (long)(stabilizeState->pitchErrorDeg * 100.0f);
        yaw_cmd_hz = (unsigned long)stabilizeState->yawCommandFrequencyHz;
        pitch_cmd_hz = (unsigned long)stabilizeState->pitchCommandFrequencyHz;
    } else {
        ref_yaw_x100 = 0L;
        ref_pitch_x100 = 0L;
        yaw_err_x100 = 0L;
        pitch_err_x100 = 0L;
        yaw_cmd_hz = 0UL;
        pitch_cmd_hz = 0UL;
    }

    yaw_run = (unsigned int)((yawMotorState != NULL) ? yawMotorState->running : 0U);
    pitch_run = (unsigned int)((pitchMotorState != NULL) ? pitchMotorState->running : 0U);
    yaw_en = (unsigned int)((yawMotorState != NULL) ? yawMotorState->enabled : 0U);
    pitch_en = (unsigned int)((pitchMotorState != NULL) ? pitchMotorState->enabled : 0U);
    yaw_freq = (unsigned long)((yawMotorState != NULL) ? yawMotorState->frequencyHz : 0U);
    pitch_freq = (unsigned long)((pitchMotorState != NULL) ? pitchMotorState->frequencyHz : 0U);

    if (directionCalState != NULL) {
        yaw_fwd_delta_x100 = (long)(directionCalState->yaw.forwardDeltaDeg * 100.0f);
        yaw_rev_delta_x100 = (long)(directionCalState->yaw.reverseDeltaDeg * 100.0f);
        pitch_fwd_delta_x100 = (long)(directionCalState->pitch.forwardDeltaDeg * 100.0f);
        pitch_rev_delta_x100 = (long)(directionCalState->pitch.reverseDeltaDeg * 100.0f);
        yaw_dir_high = (unsigned int)directionCalState->yaw.lastDirLevelHigh;
        pitch_dir_high = (unsigned int)directionCalState->pitch.lastDirLevelHigh;
        yaw_cal_pulses = (unsigned long)directionCalState->yaw.pulseCount;
        pitch_cal_pulses = (unsigned long)directionCalState->pitch.pulseCount;
        yaw_cal_cycles = (unsigned long)directionCalState->yaw.cycleCount;
        pitch_cal_cycles = (unsigned long)directionCalState->pitch.cycleCount;
    } else {
        yaw_fwd_delta_x100 = 0L;
        yaw_rev_delta_x100 = 0L;
        pitch_fwd_delta_x100 = 0L;
        pitch_rev_delta_x100 = 0L;
        yaw_dir_high = 0U;
        pitch_dir_high = 0U;
        yaw_cal_pulses = 0UL;
        pitch_cal_pulses = 0UL;
        yaw_cal_cycles = 0UL;
        pitch_cal_cycles = 0UL;
    }

    len = snprintf(txBuf, sizeof(txBuf),
        "ICM init=%u initRet=%ld who=0x%02X rs=%u ws=%u reg=0x%02X len=%u rc=%lu wc=%lu sc=%lu ypr_x100=%ld,%ld,%ld acc_x1000=%ld,%ld,%ld gyro_x100=%ld,%ld,%ld mode=%u stab=%u ref=%u ref_x100=%ld,%ld err_x100=%ld,%ld cmdHz=%lu,%lu m1_en=%u m1_run=%u m1_freq=%lu m2_en=%u m2_run=%u m2_freq=%lu cal_pulse=%lu,%lu cal_cycle=%lu,%lu cal_dirH=%u,%u cal_dlt_x100=%ld,%ld,%ld,%ld\r\n",
        (unsigned int)g_imu_debug_init_ok,
        (long)g_icm_debug_init_result,
        (unsigned int)g_icm_debug_last_who_am_i,
        (unsigned int)g_icm_debug_last_read_status,
        (unsigned int)g_icm_debug_last_write_status,
        (unsigned int)g_icm_debug_last_reg,
        (unsigned int)g_icm_debug_last_len,
        (unsigned long)g_icm_debug_read_count,
        (unsigned long)g_icm_debug_write_count,
        (unsigned long)g_imu_debug_sample_count,
        yaw_x100, pitch_x100, roll_x100,
        acc_x1000, acc_y1000, acc_z1000,
        gyro_x100, gyro_y100, gyro_z100,
        controlMode,
        (unsigned int)((stabilizeState != NULL) ? stabilizeState->enabled : 0U),
        (unsigned int)((stabilizeState != NULL) ? stabilizeState->referenceCaptured : 0U),
        ref_yaw_x100, ref_pitch_x100,
        yaw_err_x100, pitch_err_x100,
        yaw_cmd_hz, pitch_cmd_hz,
        yaw_en, yaw_run, yaw_freq,
        pitch_en, pitch_run, pitch_freq,
        yaw_cal_pulses, pitch_cal_pulses,
        yaw_cal_cycles, pitch_cal_cycles,
        yaw_dir_high, pitch_dir_high,
        yaw_fwd_delta_x100, yaw_rev_delta_x100,
        pitch_fwd_delta_x100, pitch_rev_delta_x100);

    if (len > 0) {
        DebugUart2_WriteString(txBuf);
    }
}

static float Debug_NormalizeAngleDeg(float angleDeg)
{
    while (angleDeg > 180.0f) {
        angleDeg -= 360.0f;
    }
    while (angleDeg < -180.0f) {
        angleDeg += 360.0f;
    }

    return angleDeg;
}

static void Debug_PrintVofaAngleTable(void)
{
    float angleChannels[6];
    const Yuntai_StabilizeState_t *stabilizeState;

    stabilizeState = Yuntai_GetStabilizeState();
    if ((stabilizeState != NULL) && stabilizeState->referenceCaptured) {
        angleChannels[0] = Debug_NormalizeAngleDeg(
            stabilizeState->currentYawDeg - stabilizeState->referenceYawDeg);
        angleChannels[1] = Debug_NormalizeAngleDeg(
            stabilizeState->currentPitchDeg - stabilizeState->referencePitchDeg);
        angleChannels[2] = Debug_NormalizeAngleDeg(
            stabilizeState->currentRollDeg - stabilizeState->referenceRollDeg);
    } else {
        angleChannels[0] = 0.0f;
        angleChannels[1] = 0.0f;
        angleChannels[2] = 0.0f;
    }

    /*
     * VOFA JustFloat channel order:
     * 0: yaw relative angle (deg)
     * 1: pitch relative angle (deg)
     * 2: roll relative angle (deg)
     * 3: ICM I2C device address (decimal, e.g. 105 means 0x69)
     * 4: last accessed register address (decimal)
     * 5: WHO_AM_I value (decimal, e.g. 71 means 0x47)
     */
    angleChannels[3] = (float)icm42688_int_get_device_addr();
    angleChannels[4] = (float)g_icm_debug_last_reg;
    angleChannels[5] = (float)g_icm_debug_last_who_am_i;

    DebugUart2_WriteJustFloatFrame(angleChannels, 6U);
}

void printttask(void *pvParam)
{
    TickType_t xLastWakeTime;

    (void) pvParam;

    xLastWakeTime = xTaskGetTickCount();
    for (;;) {
#if (PRINT_OUTPUT_MODE == PRINT_OUTPUT_MODE_VOFA_JUSTFLOAT)
        Debug_PrintVofaAngleTable();
#else
        Debug_PrintIcmStatus();
#endif
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200));
    }
}


//状态机任务
void reporttask(void *pvParam)
{
    TickType_t xLastWakeTime;
    (void) pvParam;

    xLastWakeTime = xTaskGetTickCount();
    for (;;) {
        Show_System_Stats();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}

void testtask(void *pvParam)
{
    (void) pvParam;
    for (;;)
    {
        // yaw_pid_out = 1+Yaw_Pid(0);
        // yaw_pid_out = fXianfu(yaw_pid_out,0,2);
        // printf("yaw:%f\r\n", g_ypr[0]);
        //
        // printf("yaw_pid_out:%f\r\n",yaw_pid_out);
        // Car_Run(20,yaw_pid_out);
        Car_Right_Angle_Turn();



        vTaskDelay(pdMS_TO_TICKS(20));
    }
}



/*
 *                     /\     /\
 *                    {  `---'  }
 *                    {  O   O  }
 *                    ~~>  V  <~~
 *                     \  \|/  /
 *                      `-----'____
 *                      /     \    \_
 *                     {       }\  )_\_   _
 *                     |  \_/  |/ /  \_\_/ )
 *                      \__/  /(_/     \__/
 *                        (__/
 */

