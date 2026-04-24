#include "key_facade.h"
#include "key_board.h"

/*
 * @name: Key_Init
 * @note: Key业务层初始化入口，内部初始化板级Key对象
 * @return {void}
 * @author cube
 */
void Key_Init(void)
{
    Key_Board_Init();
}

/*
 * @name: Key_GetValue
 * @note: 遍历所有板级Key对象，返回第一个有效Key事件
 * @return {Key_Value} Key事件值，KEY_NONE表示无事件
 * @author cube
 */
Key_Value Key_GetValue(void)
{
    key_t *keys;
    uint8_t key_num;
    Key_Value value = KEY_NONE;

    keys = Key_Board_GetKeys();
    key_num = Key_Board_GetNum();

    if ((keys == 0) || (key_num == 0U)) {
        return value;
    }

    for (uint8_t i = 0U; i < key_num; i++) {
        uint8_t ret;

        if ((keys[i].ops == 0) || (keys[i].ops->scan == 0)) {
            continue;
        }

        ret = keys[i].ops->scan(&keys[i]);
        if (ret != 0U) {
            value = (Key_Value)(ret | (i << 4));
            break;
        }
    }

    return value;
}
