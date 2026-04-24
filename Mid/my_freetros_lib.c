#include "my_freetros_lib.h"

/**
 * @brief  鉁?FreeRTOS 鍏ㄥ眬璧勬簮鐩戞帶鍣?(浠诲姟缁熻銆佸唴瀛樺揩鐓т笌杩愯鑰楁椂)
 * * @details
 * 1. 鏍稿績鍘熺悊锛氶€氳繃 uxTaskGetSystemState 鎹曡幏鍐呮牳 TCB (Task Control Block) 鍒楄〃鏁版嵁銆?
 * 2. 鏃堕棿鍩哄噯锛氫緷璧?portGET_RUN_TIME_COUNTER_VALUE() 瀹忚繑鍥炵殑楂橀 Tick 鍊笺€?
 * 3. 鎬ц兘褰卞搷锛氭湰鍑芥暟鍖呭惈鍔ㄦ€佸唴瀛樼敵璇蜂笌娴偣杩愮畻锛屽缓璁粎鐢ㄤ簬璋冭瘯闃舵锛屼笉瑕佸湪楂橀涓柇涓皟鐢ㄣ€?
 */
void Show_System_Stats(void)
{
    /* --- 鍙橀噺瀹氫箟涓庤祫婧愬噯澶?--- */
    UBaseType_t uxArraySize;            // 褰撳墠绯荤粺涓瓨鍦ㄧ殑浠诲姟鎬绘暟
    TaskStatus_t *pxTaskStatusArray;    // 鎸囧悜浠诲姟鐘舵€佺粨鏋勪綋鏁扮粍鐨勬寚閽?
    uint32_t ulTotalRunTime;            // 绯荤粺鑷惎鍔ㄤ互鏉ョ殑鎬荤粺璁℃椂闂?(g_task_run_tick)

    /* 1. 鍔ㄦ€佽幏鍙栦换鍔℃暟骞剁敵璇峰唴瀛?(闇€寮€鍚?configSUPPORT_DYNAMIC_ALLOCATION) */
    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

    /* 鍐呭瓨妫€鏌ワ細鑻ュ爢绌洪棿涓嶈冻锛屾墦鍗伴敊璇苟閫€鍑猴紝闃叉鍚庣画闈炴硶璁块棶 */
    if (pxTaskStatusArray == NULL) {
        printf("[Monitor] Error: Heap memory full!\r\n");
        return;
    }

    /* 2. 鎹曡幏绯荤粺鐘舵€佸揩鐓?
       - pxTaskStatusArray: 濉厖姣忎釜浠诲姟鐨勫悕瀛椼€佷紭鍏堢骇銆佹爤娣卞害绛?
       - ulTotalRunTime: 杩斿洖鐢辩‖浠跺畾鏃跺櫒绱姞鐨勬€绘椂闂存埑 */
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

    /* 瀹归敊澶勭悊锛氱‘淇濇€绘椂闂翠笉涓?锛岄伩鍏嶅悗缁绠楃櫨鍒嗘瘮鏃跺彂鐢熼櫎闆跺紓甯?*/
    if (ulTotalRunTime == 0) ulTotalRunTime = 1;

    /* --- 3. 鎵撳嵃瑙嗚鍒嗛殧绗︿笌绯荤粺姒傝 --- */
    printf("\r\n====================== FreeRTOS Monitor ======================\r\n");
    printf("System Total RunTime: %u Ticks (Base: 0.01ms)\r\n", (unsigned int)ulTotalRunTime);
    printf("--------------------------------------------------------------\r\n");

    /* 鎵撳嵃琛ㄥご锛歂ame(鍚嶇О), State(鐘舵€?, Prio(浼樺厛绾?, Stack(鍓╀綑鏍?, AbsTime(缁濆鑰楁椂), CPU% */
    printf("%-12s %-6s %-5s %-8s %-12s %-4s\r\n",
           "Name", "State", "Prio", "Stack", "AbsTime", "CPU%");
    printf("--------------------------------------------------------------\r\n");

    /* 4. 閬嶅巻骞惰В鏋愭瘡涓换鍔＄殑鏁版嵁 */
    for (UBaseType_t i = 0; i < uxArraySize; i++)
    {
        char *pcState;
        /* 灏嗘灇涓剧被鍨嬬殑浠诲姟鐘舵€佽浆鎹负鏄撹鐨勫瓧绗︿覆 */
        switch (pxTaskStatusArray[i].eCurrentState)
        {
            case eRunning:   pcState = "RUN";  break; // 姝ｅ湪鍗犵敤 CPU 鐨勪换鍔?
            case eReady:     pcState = "RDY";  break; // 灏辩华鎬侊紝绛夊緟璋冨害
            case eBlocked:   pcState = "BLK";  break; // 闃诲鎬侊紙濡傛鍦?vTaskDelay锛?
            case eSuspended: pcState = "SUS";  break; // 鎸傝捣鎬?
            case eDeleted:   pcState = "DEL";  break; // 宸插垹闄わ紙绛夊緟鍥炴敹璧勬簮锛?
            default:         pcState = "UNK";  break; // 鏈煡鐘舵€?
        }

        /* 璁＄畻璇ヤ换鍔″崰鐢?CPU 鐨勭櫨鍒嗘瘮 (娴偣杞崲淇濊瘉绮惧害) */
        uint32_t ulStatsAsPercentage = (uint32_t)((float)pxTaskStatusArray[i].ulRunTimeCounter / ulTotalRunTime * 100);

        /* 鎵撳嵃浠诲姟璇︽儏锛?
           - Stack 鍒楁樉绀虹殑鏄€滈珮姘翠綅绾库€濓紝鍗宠浠诲姟鑷惎鍔ㄤ互鏉ユ渶灏忓墿浣欐爤绌洪棿锛堝崟浣嶏細瀛楋級
           - AbsTime 鍒楁樉绀鸿浠诲姟鎬诲叡闇稿崰浜嗗灏戜釜 0.01ms */
        printf("%-12s %-6s %-5u %-8u %-12u %u%%\r\n",
               pxTaskStatusArray[i].pcTaskName,
               pcState,
               (unsigned int)pxTaskStatusArray[i].uxCurrentPriority,
               (unsigned int)pxTaskStatusArray[i].usStackHighWaterMark,
               (unsigned int)pxTaskStatusArray[i].ulRunTimeCounter,
               (unsigned int)ulStatsAsPercentage);
    }

    /* --- 5. 鍫嗗唴瀛樼洃鎺?(Heap Safety Check) --- */
    size_t freeHeap = xPortGetFreeHeapSize();
    size_t minFreeHeap = xPortGetMinimumEverFreeHeapSize(); // 鐩戞帶鈥滄渶鍗遍櫓鏃跺埢鈥濈殑鍓╀綑鍐呭瓨

    printf("--------------------------------------------------------------\r\n");
    printf("Memory Summary:\r\n");
    printf("   > Current Free : %u Bytes\r\n", (unsigned int)freeHeap);
    printf("   > Historic Min : %u Bytes (MinFree)\r\n", (unsigned int)minFreeHeap);

    /* 闃堝€兼姤璀︼細鑻ュ墿浣欏唴瀛樺巻鍙叉渶浣庡€艰繃灏忥紝棰勮绯荤粺鍙兘宕╂簝 */
    if (minFreeHeap < 128) {
        printf("WARNING: System memory is critically low!\r\n");
    }
    printf("==============================================================\r\n");

    /* 6. 閲婃斁涓存椂鐢宠鐨勬暟缁勫唴瀛橈紝闃叉鍐呭瓨娉勬紡 (Memory Leak) */
    vPortFree(pxTaskStatusArray);
}


// FreeRTOS 任务运行统计时间基准，单位 0.01ms
uint32_t g_task_run_tick;

/**
 * @brief 定时器 TIMA0 的中断服务函数，用于递增任务运行统计时间
 *
 * 说明：
 * - 运行时统计依赖高频定时器周期性累加 g_task_run_tick
 * - DriverLib 推荐读取 IIDX 来自动清除挂起中断，避免重复触发
 */
void TASKTIMER_INST_IRQHandler(void)
{
    // 1) 读取并清除挂起中断标志
    // TASKTIMER 为 TIMA0，必须使用 TimerA API
    switch (DL_TimerA_getPendingInterrupt(TASKTIMER_INST)) {

        // 2) 只处理计数到 0 的事件（Zero/Load）
        // 该事件周期性出现，作为运行时统计的时间基准
        case DL_TIMER_IIDX_ZERO:
            // 3) 单调递增统计计数
            g_task_run_tick++;
            break;

        default:
            break;
    }
}

// 任务栈溢出时进入该钩子（需 configCHECK_FOR_STACK_OVERFLOW > 0）
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    // xTask 未必可用（溢出后栈内容可能已损坏），只做最小化使用
    (void)xTask;

    // 禁止中断，防止继续运行引发二次错误
    taskDISABLE_INTERRUPTS();

    // 打印任务名（若 pcTaskName 可用）
    printf("\r\n[StackOverflow] task=%s\r\n", pcTaskName);

    // 停在这里供调试
    for (;;) {
    }
}

// 堆内存分配失败时进入该钩子（需 configUSE_MALLOC_FAILED_HOOK = 1）
void vApplicationMallocFailedHook(void)
{
    // 禁止中断，避免系统继续运行导致不可预期行为
    taskDISABLE_INTERRUPTS();

    // 打印当前可用堆和历史最小可用堆，快速判断是否“堆打满”
    printf("\r\n[MallocFailed] heap=%u min=%u\r\n",
           (unsigned int)xPortGetFreeHeapSize(),
           (unsigned int)xPortGetMinimumEverFreeHeapSize());

    // 停在这里供调试
    for (;;) {
    }
}


