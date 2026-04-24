#include "buzzer_driver.h"

/*
 * @name: Buzzer_WriteLevel
 * @note: 根据Buzzer的active_high配置，计算实际输出电平并写入GPIO
 * @param {buzzer_t *} me - Buzzer设备实例指针
 * @param {uint8_t} is_on - 是否打开Buzzer（1为响，0为不响）
 * @return {void}
 * @author cube
 */
static void Buzzer_WriteLevel(buzzer_t *me, uint8_t is_on)
{
    uint8_t level;

    if ((me == 0) || (me->base.is_init == 0U)) {
        return;
    }

    level = (me->cfg.active_high != 0U) ? (is_on ? 1U : 0U) : (is_on ? 0U : 1U);
    Platform_GPIO_Write(&me->cfg.hw.gpio, level);
}

/*
 * @name: Buzzer_Base_On/Off
 * @note: Buzzer的基础操作函数，直接调用Buzzer_WriteLevel实现开关功能
 * @param {void *} me - Buzzer设备实例指针
 * @return {void}
 * @author cube
 */
static void Buzzer_Base_On(void *me)
{
    Buzzer_WriteLevel((buzzer_t *) me, 1U);
}

static void Buzzer_Base_Off(void *me)
{
    Buzzer_WriteLevel((buzzer_t *) me, 0U);
}

/*
 * @name: g_buzzer_base_ops
 * @note: Buzzer基础操作函数集合
 */
static const buzzer_base_ops_t g_buzzer_base_ops = {
    .on = Buzzer_Base_On,
    .off = Buzzer_Base_Off,
    .toggle = 0,
};

/*
 * @name: Buzzer_GetBaseOps
 * @note: 获取Buzzer基础操作函数集合的指针
 */
const buzzer_base_ops_t *Buzzer_GetBaseOps(void)
{
    return &g_buzzer_base_ops;
}

/*
 * @name: Buzzer_InitDevice
 * @note: 初始化Buzzer设备
 * @param {buzzer_t *} me - Buzzer设备实例指针
 * @param {const buzzer_cfg_t *} cfg - Buzzer配置信息指针
 * @return {void}
 * @author cube
 */
void Buzzer_InitDevice(buzzer_t *me, const buzzer_cfg_t *cfg)
{
    if ((me == 0) || (cfg == 0)) {
        return;
    }

    me->ops = Buzzer_GetBaseOps();
    me->cfg = *cfg;
    me->base.is_init = 1U;
    Buzzer_WriteLevel(me, me->cfg.default_on);
}

/*
 * @name: Buzzer_On
 * @note: 打开Buzzer，比如说我需要让Buzzer播放某种模式，
 * 可以通过修改工具注册表的方式将Buzzer的开函数换成播放函数，
 * 这样就可以实现模式播放了
 * @param {buzzer_t *} me - Buzzer设备实例指针
 * @return {void}
 * @author cube
 */
void Buzzer_On(buzzer_t *me)
{
    if ((me == 0) || (me->ops == 0) || (me->ops->on == 0)) {
        return;
    }

    me->ops->on(me); // 通过函数指针调用具体的操作函数
}

/*
 * @name: Buzzer_Off
 * @note: 关闭Buzzer（对外接口）
 * @param {buzzer_t *} me - Buzzer设备实例指针
 * @return {void}
 * @author cube
 */
void Buzzer_Off(buzzer_t *me)
{
    if ((me == 0) || (me->ops == 0) || (me->ops->off == 0)) {
        return;
    }

    me->ops->off(me); // 本质上都是调用ops注册的工具函数
}
