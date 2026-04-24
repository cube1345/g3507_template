#include "ti_msp_dl_config.h"
#include "stdio.h"
#include "my_system_lib.h"
#include "IMU.h"
#include "FreeRTOS_demo.h"
#include "yuntai_app.h"

#define YUNTAI_STARTUP_USE_DIRECTION_CAL 0U


int main(void)
{
	SYSCFG_DL_init();

	/* 当前仅保留 IMU 与二维云台自稳所需初始化。 */
	IMU_init();
#if (YUNTAI_STARTUP_USE_DIRECTION_CAL == 1U)
	Yuntai_DirectionCal_Init();
#else
	Yuntai_SelfStab_Init();
#endif

	/* 以下外设当前没有对应 RTOS 任务，先全部注释，避免干扰排障。 */
	// USART_App_Init();
	// OLED_Init();
	// Key_Init();
	// Menu_Init();
	// Xunji_Init();
	// LED_Init();
	// Motor_Init();
	// Encoder_Init();
	// Buzzer_Init();


	NVIC_EnableIRQ(SYSTIMER_INST_INT_IRQN);
	/* 当前不使用摄像头串口接收，先关闭 UART0 中断。 */
	// NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
	// NVIC_EnableIRQ(UART_2_INST_INT_IRQN);


	for (int i = 0; i < 3; i++)
	{
		printf("INIT_OK\r\n");
		/* 启动阶段不要依赖 SYSTIMER 中断节拍，直接用阻塞延时更稳妥。 */
		delay_ms(50);
	}


	// Set_PWM(300,300);

	FreeRTOS_Start();


   while (1)
	{

   		printf("error\r\n");
   		delay_ms(1000);
	}
}


/**
 * @brief UART2 接收中断处理函数
 */
void UART_2_INST_IRQHandler(void)
{
	switch (DL_UART_Main_getPendingInterrupt(UART_2_INST)) {
		case DL_UART_MAIN_IIDX_RX:
		{
			uint8_t res = DL_UART_Main_receiveData(UART_2_INST);
			printf("Received: 0x%02X\r\n", res);
			break;
		}
		default:
			break;
	}
}











