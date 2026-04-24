#include "OLED_menu.h"
#include "OLED_int.h"
#include "key_driver.h"
#include "FreeRTOS.h"
#include "task.h"

/* ============================================================================== */
/* 核心定义（逻辑层指令与数据结构）                                                  */
/* ============================================================================== */

#define CMD_NONE    0xFF
#define CMD_DOWN    0x00
#define CMD_UP      0x01
#define CMD_ENTER   0x02
#define CMD_BACK    0x03

#define DISP_MAX_LINES        4
#define MENU_MESSAGE_TIME_MS  800
#define MENU_FLOAT_INT_LEN    2
#define MENU_FLOAT_FRA_LEN    3

struct MenuItem_t;

typedef struct MenuItem_t {
    char *String;
    void (*Operation)(void);
    struct MenuItem_t *ChildMenu;
    uint8_t ChildLen;
    struct MenuItem_t *ParentMenu;
    uint8_t ParentLen;
} MenuItem;

typedef enum {
    MENU_UI_LIST = 0,
    MENU_UI_PARAM_LIST,
    MENU_UI_PARAM_EDIT,
    MENU_UI_STEP_EDIT,
    MENU_UI_MESSAGE
} MenuUiMode;

typedef struct {
    char *Name;
    volatile float *Value;
    float Min;
    float Max;
} ParamItem;

#define MENU_LEN(x) ((uint8_t)(sizeof(x) / sizeof((x)[0])))

static void Menu_Refresh(void);

/* ============================================================================== */
/* 全局变量                                                                        */
/* ============================================================================== */

static MenuItem *CurrentMenu;
static uint8_t CurrentLen;
static uint8_t Cursor = 0;
static uint8_t Menu_Scroll_Offset = 0;

static MenuUiMode g_menu_ui_mode = MENU_UI_LIST;

static uint8_t g_param_cursor = 0;
static uint8_t g_param_scroll_offset = 0;
static uint8_t g_param_edit_index = 0;
static uint8_t g_param_step_index = 2;

static const char *g_message_title = 0;
static const char *g_message_desc = 0;
static TickType_t g_message_deadline = 0;

uint8_t g_task_choose_flag = 0;

extern volatile float yaw_kp;
extern volatile float yaw_ki;
extern volatile float yaw_kd;

extern MenuItem MainMenu[];

/* 需要新增在线可调参数时，只需要继续往表里追加一项。 */
static ParamItem g_param_items[] = {
    {"yaw_kp", &yaw_kp, 0.000f, 5.000f},
    {"yaw_ki", &yaw_ki, 0.000f, 5.000f},
    {"yaw_kd", &yaw_kd, 0.000f, 5.000f},
};

static const float g_param_steps[] = {
    0.001f,
    0.005f,
    0.010f,
    0.050f,
    0.100f,
    0.500f,
    1.000f
};

/* ============================================================================== */
/* 硬件按键翻译层                                                                  */
/* ============================================================================== */

static uint8_t Menu_Get_Hardware_Input(void)
{
    Key_Value raw_key = Key_GetValue();

    if (raw_key == KEY_NONE) {
        return CMD_NONE;
    }

    switch (raw_key)
    {
        case KEY0_SHORT_RELEASE:
        case KEY0_CONTINUOUS:
            return CMD_DOWN;

        case KEY1_SHORT_RELEASE:
        case KEY1_CONTINUOUS:
            return CMD_UP;

        case KEY0_LONG_RELEASE:
            return CMD_ENTER;

        case KEY1_LONG_RELEASE:
            return CMD_BACK;

        default:
            return CMD_NONE;
    }
}


/* ============================================================================== */
/* 公共辅助函数                                                                    */
/* ============================================================================== */


static uint8_t Menu_Get_Param_Count(void)
{
    return MENU_LEN(g_param_items);
}

static uint8_t Menu_Get_Param_List_Len(void)
{
    return (uint8_t)(Menu_Get_Param_Count() + 1U);
}

static float Menu_Get_Current_Step(void)
{
    return g_param_steps[g_param_step_index];
}

static float Menu_Clamp_Float(float value, float min, float max)
{
    if (value < min) {
        return min;
    }

    if (value > max) {
        return max;
    }

    return value;
}

static void Menu_Draw_Scroll_Hint(uint8_t scroll_offset, uint8_t total_len)
{
    if (scroll_offset + DISP_MAX_LINES < total_len) {
        OLED_ShowString(120, 48, ".", OLED_8X16);
    }

    if (scroll_offset > 0) {
        OLED_ShowString(120, 0, ".", OLED_8X16);
    }
}

static void Menu_Show_Message(const char *title, const char *desc)
{
    g_message_title = title;
    g_message_desc = desc;
    g_message_deadline = xTaskGetTickCount() + pdMS_TO_TICKS(MENU_MESSAGE_TIME_MS);
    g_menu_ui_mode = MENU_UI_MESSAGE;
    Menu_Refresh();
}

static void Menu_Enter_Param_List(void)
{
    g_menu_ui_mode = MENU_UI_PARAM_LIST;
    g_param_cursor = 0;
    g_param_scroll_offset = 0;
    Menu_Refresh();
}

static void Menu_Return_To_Main_List(void)
{
    g_menu_ui_mode = MENU_UI_LIST;
    Menu_Refresh();
}

static void Menu_Param_Move_Down(void)
{
    uint8_t total_len = Menu_Get_Param_List_Len();

    g_param_cursor++;

    if (g_param_cursor >= total_len) {
        g_param_cursor = 0;
        g_param_scroll_offset = 0;
    }

    if (g_param_cursor >= g_param_scroll_offset + DISP_MAX_LINES) {
        g_param_scroll_offset++;
    }
}

static void Menu_Param_Move_Up(void)
{
    uint8_t total_len = Menu_Get_Param_List_Len();

    if (g_param_cursor == 0U) {
        g_param_cursor = (uint8_t)(total_len - 1U);
        if (total_len > DISP_MAX_LINES) {
            g_param_scroll_offset = (uint8_t)(total_len - DISP_MAX_LINES);
        } else {
            g_param_scroll_offset = 0;
        }
    } else {
        g_param_cursor--;
        if (g_param_cursor < g_param_scroll_offset) {
            g_param_scroll_offset--;
        }
    }
}

static void Menu_Adjust_Step(int8_t direction)
{
    uint8_t step_count = MENU_LEN(g_param_steps);

    if (direction > 0) {
        g_param_step_index++;
        if (g_param_step_index >= step_count) {
            g_param_step_index = 0;
        }
    } else if (direction < 0) {
        if (g_param_step_index == 0U) {
            g_param_step_index = (uint8_t)(step_count - 1U);
        } else {
            g_param_step_index--;
        }
    }
}

static void Menu_Adjust_Param_Value(int8_t direction)
{
    ParamItem *item;
    float value;

    if (g_param_edit_index >= Menu_Get_Param_Count()) {
        return;
    }

    item = &g_param_items[g_param_edit_index];
    value = *(item->Value);

    if (direction > 0) {
        value += Menu_Get_Current_Step();
    } else if (direction < 0) {
        value -= Menu_Get_Current_Step();
    }

    *(item->Value) = Menu_Clamp_Float(value, item->Min, item->Max);
}

/* ============================================================================== */
/* 渲染引擎                                                                        */
/* ============================================================================== */

static void Menu_Refresh_List(void)
{
    uint8_t i;

    OLED_Clear();

    for (i = 0; i < DISP_MAX_LINES; i++)
    {
        uint8_t real_index = (uint8_t)(Menu_Scroll_Offset + i);

        if (real_index >= CurrentLen) {
            break;
        }

        if (real_index == Cursor) {
            OLED_ShowString(0, i * 16, ">", OLED_8X16);
        }

        OLED_ShowString(16, i * 16, CurrentMenu[real_index].String, OLED_8X16);
    }

    Menu_Draw_Scroll_Hint(Menu_Scroll_Offset, CurrentLen);
    OLED_Update();
}

static void Menu_Refresh_Param_List(void)
{
    uint8_t i;
    uint8_t total_len = Menu_Get_Param_List_Len();

    OLED_Clear();

    for (i = 0; i < DISP_MAX_LINES; i++)
    {
        uint8_t real_index = (uint8_t)(g_param_scroll_offset + i);
        uint8_t y = (uint8_t)(i * 16U);

        if (real_index >= total_len) {
            break;
        }

        if (real_index == g_param_cursor) {
            OLED_ShowString(0, y, ">", OLED_8X16);
        }

        if (real_index == 0U) {
            OLED_ShowString(16, y, "step", OLED_8X16);
            OLED_ShowFloatNum(56, y, Menu_Get_Current_Step(), 1, MENU_FLOAT_FRA_LEN, OLED_8X16);
        } else {
            ParamItem *item = &g_param_items[real_index - 1U];
            OLED_ShowString(16, y, item->Name, OLED_8X16);
            OLED_ShowFloatNum(72, y, *(item->Value), MENU_FLOAT_INT_LEN, MENU_FLOAT_FRA_LEN, OLED_8X16);
        }
    }

    Menu_Draw_Scroll_Hint(g_param_scroll_offset, total_len);
    OLED_Update();
}

static void Menu_Refresh_Param_Edit(void)
{
    ParamItem *item = &g_param_items[g_param_edit_index];

    OLED_Clear();
    OLED_ShowString(0, 0, "edit", OLED_8X16);
    OLED_ShowString(40, 0, item->Name, OLED_8X16);
    OLED_ShowString(0, 16, "val", OLED_8X16);
    OLED_ShowFloatNum(40, 16, *(item->Value), MENU_FLOAT_INT_LEN, MENU_FLOAT_FRA_LEN, OLED_8X16);
    OLED_ShowString(0, 32, "step", OLED_8X16);
    OLED_ShowFloatNum(40, 32, Menu_Get_Current_Step(), 1, MENU_FLOAT_FRA_LEN, OLED_8X16);
    OLED_ShowString(0, 48, "K0- K1+ L0/1E", OLED_8X16);
    OLED_Update();
}

static void Menu_Refresh_Step_Edit(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, "step set", OLED_8X16);
    OLED_ShowString(0, 16, "step", OLED_8X16);
    OLED_ShowFloatNum(40, 16, Menu_Get_Current_Step(), 1, MENU_FLOAT_FRA_LEN, OLED_8X16);
    OLED_ShowString(0, 32, "apply live", OLED_8X16);
    OLED_ShowString(0, 48, "K0- K1+ L0/1E", OLED_8X16);
    OLED_Update();
}

static void Menu_Refresh_Message(void)
{
    OLED_Clear();

    if (g_message_title != 0) {
        OLED_ShowString(24, 16, (char *) g_message_title, OLED_8X16);
    }

    if (g_message_desc != 0) {
        OLED_ShowString(24, 40, (char *) g_message_desc, OLED_6X8);
    }

    OLED_Update();
}

static void Menu_Refresh(void)
{
    switch (g_menu_ui_mode)
    {
        case MENU_UI_LIST:
            Menu_Refresh_List();
            break;

        case MENU_UI_PARAM_LIST:
            Menu_Refresh_Param_List();
            break;

        case MENU_UI_PARAM_EDIT:
            Menu_Refresh_Param_Edit();
            break;

        case MENU_UI_STEP_EDIT:
            Menu_Refresh_Step_Edit();
            break;

        case MENU_UI_MESSAGE:
            Menu_Refresh_Message();
            break;

        default:
            g_menu_ui_mode = MENU_UI_LIST;
            Menu_Refresh_List();
            break;
    }
}

/* ============================================================================== */
/* 功能函数                                                                        */
/* ============================================================================== */

static void Func_Task1(void)
{
    g_task_choose_flag = 1;
    Menu_Show_Message("task1", "running");
}

static void Func_Task2(void)
{
    g_task_choose_flag = 2;
    Menu_Show_Message("task2", "running");
}

static void Func_Task3(void)
{
    g_task_choose_flag = 3;
    Menu_Show_Message("task3", "running");
}

static void Func_Task4(void)
{
    g_task_choose_flag = 4;
    Menu_Show_Message("task4", "running");
}

static void Func_Param_Set(void)
{
    Menu_Enter_Param_List();
}

MenuItem MainMenu[] = {
    {"1.task1", Func_Task1, 0, 0, 0, 0},
    {"2.task2", Func_Task2, 0, 0, 0, 0},
    {"3.task3", Func_Task3, 0, 0, 0, 0},
    {"4.task4", Func_Task4, 0, 0, 0, 0},
    {"5.param", Func_Param_Set, 0, 0, 0, 0}
};

/* ============================================================================== */
/* 菜单初始化                                                                      */
/* ============================================================================== */

void Menu_Init(void)
{
    CurrentMenu = MainMenu;
    CurrentLen = MENU_LEN(MainMenu);
    Cursor = 0;
    Menu_Scroll_Offset = 0;

    g_menu_ui_mode = MENU_UI_LIST;
    g_param_cursor = 0;
    g_param_scroll_offset = 0;
    g_param_edit_index = 0;
    g_param_step_index = 2;

    Menu_Refresh();
}

/* ============================================================================== */
/* 核心逻辑处理                                                                    */
/* ============================================================================== */

static void Menu_Handle_List(uint8_t cmd)
{
    if (cmd == CMD_DOWN)
    {
        Cursor++;

        if (Cursor >= CurrentLen)
        {
            Cursor = 0;
            Menu_Scroll_Offset = 0;
        }

        if (Cursor >= Menu_Scroll_Offset + DISP_MAX_LINES)
        {
            Menu_Scroll_Offset++;
        }

        Menu_Refresh();
    }
    else if (cmd == CMD_UP)
    {
        if (Cursor == 0U)
        {
            Cursor = (uint8_t)(CurrentLen - 1U);
            if (CurrentLen > DISP_MAX_LINES) {
                Menu_Scroll_Offset = (uint8_t)(CurrentLen - DISP_MAX_LINES);
            } else {
                Menu_Scroll_Offset = 0;
            }
        }
        else
        {
            Cursor--;
            if (Cursor < Menu_Scroll_Offset)
            {
                Menu_Scroll_Offset--;
            }
        }

        Menu_Refresh();
    }
    else if (cmd == CMD_ENTER)
    {
        MenuItem *item = &CurrentMenu[Cursor];

        if (item->ChildMenu != 0)
        {
            CurrentMenu = item->ChildMenu;
            CurrentLen = item->ChildLen;
            Cursor = 0;
            Menu_Scroll_Offset = 0;
            Menu_Refresh();
        }
        else if (item->Operation != 0)
        {
            item->Operation();
            if (g_menu_ui_mode == MENU_UI_LIST) {
                Menu_Refresh();
            }
        }
    }
    else if (cmd == CMD_BACK)
    {
        if (CurrentMenu[0].ParentMenu != 0)
        {
            MenuItem *current_child_ptr = CurrentMenu;
            MenuItem *next_menu = CurrentMenu[0].ParentMenu;
            uint8_t next_len = CurrentMenu[0].ParentLen;
            uint8_t i;
            uint8_t found = 0;

            CurrentMenu = next_menu;
            CurrentLen = next_len;

            for (i = 0; i < CurrentLen; i++)
            {
                if (CurrentMenu[i].ChildMenu == current_child_ptr)
                {
                    Cursor = i;
                    found = 1;

                    if (Cursor >= DISP_MAX_LINES) {
                        Menu_Scroll_Offset = (uint8_t)(Cursor - (DISP_MAX_LINES - 1U));
                    } else {
                        Menu_Scroll_Offset = 0;
                    }
                    break;
                }
            }

            if (!found) {
                Cursor = 0;
                Menu_Scroll_Offset = 0;
            }

            Menu_Refresh();
        }
    }
}

static void Menu_Handle_Param_List(uint8_t cmd)
{
    if (cmd == CMD_DOWN)
    {
        Menu_Param_Move_Down();
        Menu_Refresh();
    }
    else if (cmd == CMD_UP)
    {
        Menu_Param_Move_Up();
        Menu_Refresh();
    }
    else if (cmd == CMD_ENTER)
    {
        if (g_param_cursor == 0U) {
            g_menu_ui_mode = MENU_UI_STEP_EDIT;
        } else {
            g_param_edit_index = (uint8_t)(g_param_cursor - 1U);
            g_menu_ui_mode = MENU_UI_PARAM_EDIT;
        }
        Menu_Refresh();
    }
    else if (cmd == CMD_BACK)
    {
        Menu_Return_To_Main_List();
    }
}

static void Menu_Handle_Param_Edit(uint8_t cmd)
{
    if (cmd == CMD_DOWN)
    {
        Menu_Adjust_Param_Value(-1);
        Menu_Refresh();
    }
    else if (cmd == CMD_UP)
    {
        Menu_Adjust_Param_Value(1);
        Menu_Refresh();
    }
    else if (cmd == CMD_ENTER || cmd == CMD_BACK)
    {
        g_menu_ui_mode = MENU_UI_PARAM_LIST;
        Menu_Refresh();
    }
}

static void Menu_Handle_Step_Edit(uint8_t cmd)
{
    if (cmd == CMD_DOWN)
    {
        Menu_Adjust_Step(-1);
        Menu_Refresh();
    }
    else if (cmd == CMD_UP)
    {
        Menu_Adjust_Step(1);
        Menu_Refresh();
    }
    else if (cmd == CMD_ENTER || cmd == CMD_BACK)
    {
        g_menu_ui_mode = MENU_UI_PARAM_LIST;
        Menu_Refresh();
    }
}

static void Menu_Handle_Message(uint8_t cmd)
{
    if (cmd != CMD_NONE || xTaskGetTickCount() >= g_message_deadline)
    {
        Menu_Return_To_Main_List();
    }
}

void Menu_Key_Handler(void)
{
    uint8_t cmd = Menu_Get_Hardware_Input();

    if (g_menu_ui_mode == MENU_UI_MESSAGE)
    {
        Menu_Handle_Message(cmd);
        return;
    }

    if (cmd == CMD_NONE) {
        return;
    }

    switch (g_menu_ui_mode)
    {
        case MENU_UI_LIST:
            Menu_Handle_List(cmd);
            break;

        case MENU_UI_PARAM_LIST:
            Menu_Handle_Param_List(cmd);
            break;

        case MENU_UI_PARAM_EDIT:
            Menu_Handle_Param_Edit(cmd);
            break;

        case MENU_UI_STEP_EDIT:
            Menu_Handle_Step_Edit(cmd);
            break;

        default:
            g_menu_ui_mode = MENU_UI_LIST;
            Menu_Refresh();
            break;
    }
}
