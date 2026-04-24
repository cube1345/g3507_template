#include "led_driver.h"

/*
 * @name: LED_WriteLevel
 * @note: 根据LED的active_low配置，计算实际输出电平并写入GPIO
 * @param {led_t *} me - LED设备实例指针
 * @param {uint8_t} is_on - 是否点亮LED（1为点亮，0为熄灭）
 * @return {void}
 * @author cube
 */
static void LED_WriteLevel(led_t *me, uint8_t is_on)
{
    uint8_t level;

    if ((me == 0) || (me->base.is_init == 0U)) {
        return;
    }

    level = (me->cfg.active_low != 0U) ? (is_on ? 0U : 1U) : (is_on ? 1U : 0U);
    Platform_GPIO_Write(&me->cfg.hw.gpio, level);
}

/*
 * @name: LED_Base_On/Off/Toggle
 * @note: LED的基础操作函数，直接调用LED_WriteLevel或GPIO切换函数实现开关功能
 * @param {void *} me - LED设备实例指针
 * @return {void}
 * @author cube
 */
static void LED_Base_On(void *me)
{
    LED_WriteLevel((led_t *) me, 1U);
}

static void LED_Base_Off(void *me)
{
    LED_WriteLevel((led_t *) me, 0U);
}

static void LED_Base_Toggle(void *me)
{
    led_t *led;

    led = (led_t *) me;
    if ((led == 0) || (led->base.is_init == 0U)) {
        return;
    }

    Platform_GPIO_Toggle(&led->cfg.hw.gpio);
}

/*
 * @name: g_led_base_ops
 * @note: LED基础操作函数集合
 */
static const led_base_ops_t g_led_base_ops = {
    .on = LED_Base_On,
    .off = LED_Base_Off,
    .toggle = LED_Base_Toggle,
};

/*
 * @name: LED_GetBaseOps
 * @note: 获取LED基础操作函数集合的指针
 */
const led_base_ops_t *LED_GetBaseOps(void)
{
    return &g_led_base_ops;
}

/*
 * @name: LED_Init
 * @note: 初始化LED设备
 * @param {led_t *} me - LED设备实例指针
 * @param {const led_cfg_t *} cfg - LED配置信息指针
 * @return {void}
 * @author cube
 */
void LED_Init(led_t *me, const led_cfg_t *cfg)
{
    if ((me == 0) || (cfg == 0)) {
        return;
    }

    me->ops = LED_GetBaseOps();
    me->cfg = *cfg;
    me->base.is_init = 1U;
    LED_WriteLevel(me, me->cfg.default_on);
}

/*
 * @name: LED_On
 * @note: 点亮LED，比如说我需要闪烁LED，
 * 可以通过修改工具注册表的方式将LED的开函数换成闪烁函数，
 * 这样就可以实现闪烁了
 * @param {led_t *} me - LED设备实例指针
 * @return {void}
 * @author cube
 */
void LED_On(led_t *me)
{
    if ((me == 0) || (me->ops == 0) || (me->ops->on == 0)) {
        return;
    }

    me->ops->on(me); // 通过函数指针调用具体的操作函数
}

/*
 * @name: LED_Off
 * @note: 熄灭LED（对外接口）
 * @param {led_t *} me - LED设备实例指针
 * @return {void}
 * @author cube
 */
void LED_Off(led_t *me)
{
    if ((me == 0) || (me->ops == 0) || (me->ops->off == 0)) {
        return;
    }

    me->ops->off(me); // 本质上都是调用opus注册的工具函数
}

/*
 * @name: LED_Toggle
 * @note: 切换LED状态（对外接口）
 * @param {led_t *} me - LED设备实例指针
 * @return {void}
 * @author cube
 */
void LED_Toggle(led_t *me)
{
    if ((me == 0) || (me->ops == 0) || (me->ops->toggle == 0)) {
        return;
    }

    me->ops->toggle(me);
}
