#include "my_system_lib.h"
#include "FreeRTOS.h"
#include "task.h"

#ifndef MY_PRINTF_ENABLE
#define MY_PRINTF_ENABLE 0U
#endif

#ifndef MY_UART2_TX_RETRY_MAX
#define MY_UART2_TX_RETRY_MAX 100000U
#endif

uint32_t SystemCoreClock = 80000000;


// 定义全局微秒计数变量，务必加 volatile 防止被编译器优化
volatile uint32_t g_my_tick_ms = 0;

/**
 * @brief 定时器中断服务函数
 * 函数名必须与 SysConfig 中生成的宏 xxxxxxIRQHandler 一致
 */
void SYSTIMER_INST_IRQHandler(void)  //递增毫秒级系统时间
{
    // 获取当前待处理的中断并自动清除标志位
    // 对于 Timer A，通常使用 DL_TimerA_getPendingInterrupt
    switch (DL_TimerA_getPendingInterrupt(SYSTIMER_INST)) {

        // 当计数器达到 0（Zero Event）时触发
        case DL_TIMER_IIDX_ZERO:
            g_my_tick_ms++;  // 毫秒计数自增
            //
            break;

            // 如果你后续配置了其他事件（如下溢或捕获），可以在这里添加 case
        default:
            break;
    }
}

/**
 * @brief 获取当前系统启动后的毫秒数
 * 类似于 HAL_GetTick()
 */
uint32_t My_GetTick(void)
{
    return g_my_tick_ms;
}

uint64_t My_GetTimeUs(void)
{
    uint32_t tickMsBefore;
    uint32_t tickMsAfter;
    uint32_t timerCount;
    uint32_t elapsedUsInMs;

    do {
        tickMsBefore = g_my_tick_ms;
        timerCount = DL_TimerA_getTimerCount(SYSTIMER_INST);
        tickMsAfter = g_my_tick_ms;
    } while (tickMsBefore != tickMsAfter);

    if (timerCount > SYSTIMER_INST_LOAD_VALUE) {
        timerCount = SYSTIMER_INST_LOAD_VALUE;
    }

    elapsedUsInMs = SYSTIMER_INST_LOAD_VALUE - timerCount;

    return ((uint64_t) tickMsBefore * 1000ULL) + (uint64_t) elapsedUsInMs;
}


/**
 * @brief 阻塞式毫秒延时（类似 HAL_Delay）
 * @param ms 要延时的毫秒数
 */
void My_Delay_ms(uint32_t ms)
{
    uint32_t start_time = g_my_tick_ms;

    // 这种写法考虑了溢出情况，非常鲁棒
    while ((g_my_tick_ms - start_time) < ms) {
        // 等待定时器中断不断累加 g_my_tick_ms
        // __asm("nop");
    }
}



void delay_us(uint32_t us) { delay_cycles( (SystemCoreClock / 1000 / 1000)*us); }
void delay_ns(uint32_t ns)
{
    uint64_t cycles = ((uint64_t)SystemCoreClock * (uint64_t)ns + 999999999ULL) /
        1000000000ULL;
    if (cycles == 0U) {
        cycles = 1U;
    }
    delay_cycles((uint32_t)cycles);
}

void delay_ms(uint32_t ms)
{
    if (ms == 0U) {
        return;
    }

    /* In task context after scheduler start, yield CPU instead of busy-looping. */
    if ((xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) &&
        (__get_IPSR() == 0U)) {
        vTaskDelay(pdMS_TO_TICKS(ms));
        return;
    }

    delay_cycles((SystemCoreClock / 1000U) * ms);
}

bool DebugUart2_WriteChar(uint8_t ch)
{
    uint32_t retry = MY_UART2_TX_RETRY_MAX;

    while (!DL_UART_Main_transmitDataCheck(UART_2_INST, ch)) {
        if (retry-- == 0U) {
            return false;
        }
    }

    return true;
}

bool DebugUart2_WriteBuffer(const uint8_t *data, uint32_t length)
{
    const uint8_t *txPtr;
    uint32_t remain;
    uint32_t sent;
    uint32_t retry = MY_UART2_TX_RETRY_MAX;

    if ((data == NULL) || (length == 0U)) {
        return false;
    }

    txPtr = data;
    remain = length;

    while (remain > 0U) {
        sent = DL_UART_Main_fillTXFIFO(UART_2_INST, (uint8_t *)txPtr, remain);
        if (sent == 0U) {
            if (retry-- == 0U) {
                return false;
            }
        } else {
            retry = MY_UART2_TX_RETRY_MAX;
            txPtr += sent;
            remain -= sent;
        }
    }

    retry = MY_UART2_TX_RETRY_MAX;
    while (DL_UART_Main_isBusy(UART_2_INST)) {
        if (retry-- == 0U) {
            return false;
        }
    }

    return true;
}

void DebugUart2_WriteString(const char *str)
{
    uint32_t length = 0U;

    if (str == NULL) {
        return;
    }

    while (str[length] != '\0') {
        length++;
    }

    (void)DebugUart2_WriteBuffer((const uint8_t *)str, length);
}

void DebugUart2_WriteJustFloatFrame(const float *values, uint32_t count)
{
    uint32_t i;
    union {
        float floatValue;
        uint8_t bytes[sizeof(float)];
    } valueBytes;
    static const uint8_t justFloatTail[4] = {0x00U, 0x00U, 0x80U, 0x7FU};

    if ((values == NULL) || (count == 0U)) {
        return;
    }

    for (i = 0U; i < count; i++) {
        valueBytes.floatValue = values[i];
        if (!DebugUart2_WriteBuffer(valueBytes.bytes, (uint32_t)sizeof(float))) {
            return;
        }
    }

    (void)DebugUart2_WriteBuffer(justFloatTail, sizeof(justFloatTail));
}




/**
  * @brief 重定向 printf 到串口
  * @note  适用于 Keil MDK (Compiler V5 和 V6)
  */
int fputc(int ch, FILE *f)
{
    (void)f;

#if (MY_PRINTF_ENABLE == 0U)
    return ch;
#else
    (void)DebugUart2_WriteChar((uint8_t)ch);
#endif
    return ch;
}


// HardFault 现场寄存器快照（volatile 防止编译器优化掉，便于 Watch 查看）
volatile uint32_t g_hf_r0;
volatile uint32_t g_hf_r1;
volatile uint32_t g_hf_r2;
volatile uint32_t g_hf_r3;
volatile uint32_t g_hf_r12;
volatile uint32_t g_hf_lr;
volatile uint32_t g_hf_pc;
volatile uint32_t g_hf_xpsr;
volatile uint32_t g_hf_cfsr;
volatile uint32_t g_hf_hfsr;
volatile uint32_t g_hf_mmar;
volatile uint32_t g_hf_bfar;

// 从异常堆栈中解析寄存器，并读取系统故障状态寄存器
static void hardfault_capture(uint32_t *sp)
{
    // Cortex-M 异常自动入栈顺序：R0,R1,R2,R3,R12,LR,PC,xPSR
    g_hf_r0 = sp[0];
    g_hf_r1 = sp[1];
    g_hf_r2 = sp[2];
    g_hf_r3 = sp[3];
    g_hf_r12 = sp[4];
    g_hf_lr = sp[5];
    g_hf_pc = sp[6];
    g_hf_xpsr = sp[7];

    // 故障状态寄存器地址（SCB）
    // CFSR: 可配置故障状态（含 Usage/Bus/Memory）
    // HFSR: HardFault 状态
    // MMAR/BFAR: 访问违规地址（若有效）
    g_hf_cfsr = *((volatile uint32_t *)0xE000ED28U);
    g_hf_hfsr = *((volatile uint32_t *)0xE000ED2CU);
    g_hf_mmar = *((volatile uint32_t *)0xE000ED34U);
    g_hf_bfar = *((volatile uint32_t *)0xE000ED38U);
}

// C 级 HardFault 处理：保存现场 + 打印关键寄存器
void HardFault_HandlerC(uint32_t *sp)
{
    hardfault_capture(sp);

    // 打印 PC/LR 便于快速定位出错点
    printf("\r\n[HardFault] PC=0x%08X LR=0x%08X CFSR=0x%08X HFSR=0x%08X\r\n",
           (unsigned int)g_hf_pc, (unsigned int)g_hf_lr,
           (unsigned int)g_hf_cfsr, (unsigned int)g_hf_hfsr);

    // 停在这里供调试
    for (;;) {
    }
}

// 汇编入口（naked）：判断使用 MSP 还是 PSP，并把栈指针传给 C 处理函数
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile(
        // LR bit2 = 0 -> MSP, bit2 = 1 -> PSP
        "movs r1, #4           \n"
        "mov r2, lr            \n"
        "tst r2, r1            \n"
        "beq 1f                \n"
        // 使用 PSP 的情况下，将 PSP 传给 HardFault_HandlerC
        "mrs r0, psp           \n"
        "ldr r3, =HardFault_HandlerC \n"
        "bx r3                 \n"
        "1:                    \n"
        // 使用 MSP 的情况下，将 MSP 传给 HardFault_HandlerC
        "mrs r0, msp           \n"
        "ldr r3, =HardFault_HandlerC \n"
        "bx r3                 \n");
}



