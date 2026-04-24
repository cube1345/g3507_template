#ifndef OLED_MENU_H
#define OLED_MENU_H

#include "OLED_int.h"

void Menu_Key_Handler(void);
void Menu_Init(void);

//使用方法
//第一步 OLED_Init();
//	    Key_Init();
//      Menu_Init();
//第二步 在OLED_menu.c里编辑主菜单和参数表
//第三步 在循环里调用Menu_Key_Handler();

#endif
