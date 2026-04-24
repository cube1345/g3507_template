#include "key_driver.h"
#include "key_int.h"

// 当前绑定的按键对象数组指针（由接口层提供）
static Key_Driver_t *g_keys = 0;
// 当前绑定的按键数量
static uint8_t g_key_num = 0;

// 绑定并初始化按键状态机对象
void Key_Driver_Bind(Key_Driver_t *keys, uint8_t num)
{
    g_keys = keys;
    g_key_num = num;

    // 空指针或数量为 0 时不做初始化
    if (keys == 0 || num == 0U) {
        return;
    }

    // 初始化每个按键状态机字段
    for (uint8_t i = 0; i < num; i++) {
        keys[i].State = KEY_STATE_RELEASE;
        keys[i].LastTime = 0;
        keys[i].ContinuousTime = 0;
    }
}

// 单个按键状态机扫描
// 注意：这里保持你原有逻辑分支与阈值不变
uint8_t Key_Scan(Key_Driver_t *key)
{
    uint8_t key_val = 0;
    uint32_t current_time = key->GetTick();
    uint8_t pin_state = key->ReadPin();

    switch (key->State) {
        case KEY_STATE_RELEASE:
            // 空闲态检测到按下（低电平）进入去抖
            if (pin_state == 0) {
                key->State = KEY_STATE_DEBOUNCE;
                key->LastTime = current_time;
            }
            break;

        case KEY_STATE_DEBOUNCE:
            // 去抖阶段仍保持按下，且持续超过阈值，确认为短按触发
            if (pin_state == 0) {
                if (current_time - key->LastTime >= DEBOUNCE_TIME) {
                    key->State = KEY_STATE_PRESSED;
                    key->LastTime = current_time;
                    key_val = 0x01;
                }
            } else {
                // 去抖阶段松开，回到空闲态
                key->State = KEY_STATE_RELEASE;
            }
            break;

        case KEY_STATE_PRESSED:
            if (pin_state == 1) {
                // 已确认按下后松开，触发短按释放事件
                key->State = KEY_STATE_RELEASE;
                key_val = 0x02;
            } else {
                // 持续按下超过长按阈值，进入长按态并触发长按事件
                if (current_time - key->LastTime >= LONG_PRESS_TIME) {
                    key->State = KEY_STATE_LONG_PRESS;
                    key->LastTime = current_time;
                    key_val = 0x03;
                }
            }
            break;

        case KEY_STATE_LONG_PRESS:
            if (pin_state == 1) {
                // 长按阶段松开，触发长按释放事件
                key->State = KEY_STATE_RELEASE;
                key_val = 0x04;
            } else {
                // 继续保持，达到连发起始阈值则进入连发态
                if (current_time - key->LastTime >= CONTINUOUS_START) {
                    key->State = KEY_STATE_CONTINUOUS;
                    key->ContinuousTime = current_time;
                }
            }
            break;

        case KEY_STATE_CONTINUOUS:
            if (pin_state == 1) {
                // 连发阶段松开，回空闲态（保持你原先不额外发释放事件的行为）
                key->State = KEY_STATE_RELEASE;
            } else {
                // 连发阶段每个周期触发一次连发事件
                if (current_time - key->ContinuousTime >= CONTINUOUS_INTERVAL) {
                    key->ContinuousTime = current_time;
                    key_val = 0x05;
                }
            }
            break;

        default:
            key->State = KEY_STATE_RELEASE;
            break;
    }

    return key_val;
}

// 对外初始化入口
// 当前设计：driver 层统一入口，内部转调接口层完成板级绑定
void Key_Init(void)
{
    Key_Int_Init();
}

// 对外获取一次按键事件
// 轮询所有已绑定按键，返回第一个非 0 事件
Key_Value Key_GetValue(void)
{
    Key_Value value = KEY_NONE;

    if (g_keys == 0 || g_key_num == 0U) {
        return value;
    }

    for (uint8_t i = 0; i < g_key_num; i++) {
        uint8_t ret = Key_Scan(&g_keys[i]);
        if (ret != 0U) {
            value = (Key_Value)(ret | (i << 4));
            break;
        }
    }

    return value;
}
