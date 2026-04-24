#include "key_board.h"
#include "ti_msp_dl_config.h"
#include "my_system_lib.h"

#define KEY_BOARD_NUM 3U

static key_t g_keys[KEY_BOARD_NUM];

/*
 * @name: Key_Board_GetTick
 * @note: 板级tick适配函数，将系统tick绑定给Key对象使用
 * @return {uint32_t} 当前系统tick，单位ms
 * @author cube
 */
static uint32_t Key_Board_GetTick(void)
{
    return My_GetTick();
}

/*
 * @name: Key_Board_GetKeys
 * @note: 获取板级Key对象数组指针
 * @return {key_t *} Key对象数组指针
 * @author cube
 */
key_t *Key_Board_GetKeys(void)
{
    return g_keys;
}

/*
 * @name: Key_Board_GetNum
 * @note: 获取板级Key对象数量
 * @return {uint8_t} Key对象数量
 * @author cube
 */
uint8_t Key_Board_GetNum(void)
{
    return KEY_BOARD_NUM;
}

/*
 * @name: Key_Board_Init
 * @note: 初始化板级Key对象，填充GPIO和极性配置
 * @return {void}
 * @author cube
 */
void Key_Board_Init(void)
{
    const key_cfg_t key_cfgs[KEY_BOARD_NUM] = {
        {
            .gpio = {
                .port = KEY_NUM0_PORT,
                .pin = KEY_NUM0_PIN,
            },
            .active_low = 1U,
            .get_tick = Key_Board_GetTick,
        },
        {
            .gpio = {
                .port = KEY_NUM1_PORT,
                .pin = KEY_NUM1_PIN,
            },
            .active_low = 0U,
            .get_tick = Key_Board_GetTick,
        },
        {
            .gpio = {
                .port = KEY_NUM2_PORT,
                .pin = KEY_NUM2_PIN,
            },
            .active_low = 1U,
            .get_tick = Key_Board_GetTick,
        },
    };

    for (uint8_t i = 0U; i < KEY_BOARD_NUM; i++) {
        Key_InitDevice(&g_keys[i], &key_cfgs[i]);
    }
}
