#ifndef __KEY_INT_H
#define __KEY_INT_H

// 接口层初始化：
// 负责绑定板级按键读引脚函数与时间函数到 driver 层。
// 该层不承载按键状态机逻辑。
void Key_Int_Init(void);

#endif
