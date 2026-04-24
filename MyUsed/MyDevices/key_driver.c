#include "key_driver.h"

/*
 * @name: Key_ReadLevel
 * @note: 根据Key的active_low配置，读取并转换按键逻辑电平
 * @param {key_t *} me - Key设备实例指针
 * @return {uint8_t} 0表示按下，1表示释放
 * @author cube
 */
uint8_t Key_ReadLevel(key_t *me)
{
    uint8_t level;

    if ((me == 0) || (me->base.is_init == 0U)) {
        return 1U;
    }

    level = Platform_GPIO_Read(&me->cfg.gpio);
    if (me->cfg.active_low != 0U) {
        return level;
    }

    return (uint8_t)!level;
}

/*
 * @name: Key_Scan
 * @note: 执行单个Key对象的状态机扫描，返回低4位事件类型
 * @param {key_t *} me - Key设备实例指针
 * @return {uint8_t} 按键事件类型，0表示无事件
 * @author cube
 */
uint8_t Key_Scan(key_t *me)
{
    uint8_t key_val = 0U;
    uint32_t current_time;
    uint8_t pin_state;

    if ((me == 0) || (me->base.is_init == 0U) || (me->cfg.get_tick == 0)) {
        return 0U;
    }

    current_time = me->cfg.get_tick();
    pin_state = Key_ReadLevel(me);

    switch (me->state) {
        case KEY_STATE_RELEASE:
            if (pin_state == 0U) {
                me->state = KEY_STATE_DEBOUNCE;
                me->last_time = current_time;
            }
            break;

        case KEY_STATE_DEBOUNCE:
            if (pin_state == 0U) {
                if (current_time - me->last_time >= DEBOUNCE_TIME) {
                    me->state = KEY_STATE_PRESSED;
                    me->last_time = current_time;
                    key_val = 0x01;
                }
            } else {
                me->state = KEY_STATE_RELEASE;
            }
            break;

        case KEY_STATE_PRESSED:
            if (pin_state == 1U) {
                me->state = KEY_STATE_RELEASE;
                key_val = 0x02;
            } else {
                if (current_time - me->last_time >= LONG_PRESS_TIME) {
                    me->state = KEY_STATE_LONG_PRESS;
                    me->last_time = current_time;
                    key_val = 0x03;
                }
            }
            break;

        case KEY_STATE_LONG_PRESS:
            if (pin_state == 1U) {
                me->state = KEY_STATE_RELEASE;
                key_val = 0x04;
            } else {
                if (current_time - me->last_time >= CONTINUOUS_START) {
                    me->state = KEY_STATE_CONTINUOUS;
                    me->continuous_time = current_time;
                }
            }
            break;

        case KEY_STATE_CONTINUOUS:
            if (pin_state == 1U) {
                me->state = KEY_STATE_RELEASE;
            } else {
                if (current_time - me->continuous_time >= CONTINUOUS_INTERVAL) {
                    me->continuous_time = current_time;
                    key_val = 0x05;
                }
            }
            break;

        default:
            me->state = KEY_STATE_RELEASE;
            break;
    }

    return key_val;
}

/*
 * @name: Key_Base_Scan/ReadLevel
 * @note: Key基础操作函数，挂载到key_base_ops_t中用于统一调度
 * @param {void *} me - Key设备实例指针
 * @return {uint8_t} 扫描事件或逻辑电平
 * @author cube
 */
static uint8_t Key_Base_Scan(void *me)
{
    return Key_Scan((key_t *) me);
}

static uint8_t Key_Base_ReadLevel(void *me)
{
    return Key_ReadLevel((key_t *) me);
}

/*
 * @name: g_key_base_ops
 * @note: Key基础操作函数集合
 */
static const key_base_ops_t g_key_base_ops = {
    .scan = Key_Base_Scan,
    .read_level = Key_Base_ReadLevel,
};

/*
 * @name: Key_GetBaseOps
 * @note: 获取Key基础操作函数集合的指针
 */
const key_base_ops_t *Key_GetBaseOps(void)
{
    return &g_key_base_ops;
}

/*
 * @name: Key_InitDevice
 * @note: 初始化Key设备对象
 * @param {key_t *} me - Key设备实例指针
 * @param {const key_cfg_t *} cfg - Key配置信息指针
 * @return {void}
 * @author cube
 */
void Key_InitDevice(key_t *me, const key_cfg_t *cfg)
{
    if ((me == 0) || (cfg == 0)) {
        return;
    }

    me->ops = Key_GetBaseOps();
    me->cfg = *cfg;
    me->state = KEY_STATE_RELEASE;
    me->last_time = 0U;
    me->continuous_time = 0U;
    me->base.is_init = 1U;
}
