# MyUsed 外设五层架构说明

## 1. 文档目的

本文档用于系统说明当前 `MyUsed` 目录下外设驱动的五层架构设计，包括：

- 五层架构分别承担什么职责；
- 为什么要将外设拆分为多层；
- 各类外设在当前架构中的功能划分；
- `Core`、`Platform/BSP`、`Device`、`Board`、`Facade/Compat` 等层的意义；
- 后续新增外设时应遵守的设计原则。

当前工程已经将 LED、Key、Encoder、Buzzer、OLED、ICM42688、Xunji、Motor、Stepper 等模块逐步整理为类似的分层结构。

---

## 2. 总体架构概览

当前外设驱动采用五层思想组织：

```text
Application / App / Libs
        |
        v
Facade / Compat 层
        |
        v
Board 层
        |
        v
Device 层
        |
        v
Core + Platform/BSP 层
        |
        v
MCU 外设寄存器 / SysConfig / DriverLib
```

实际目录大致对应如下：

| 层级 | 目录 | 职责 |
| --- | --- | --- |
| Core 层 | `MyUsed/MyCore` | 通用设备基类、通用操作接口定义 |
| Platform/BSP 层 | `MyUsed/MyPlatform`、`MyUsed/MyBSP` | GPIO/I2C/SPI 等平台适配与底层硬件访问 |
| Device 层 | `MyUsed/MyDevices` | 外设对象、配置结构体、默认行为、ops 分发 |
| Board 层 | `MyUsed/MyBoards` | 绑定具体板级资源、SysConfig 宏、具体实例 |
| Facade 层 | `MyUsed/MyFacades` | 面向业务层的兼容 API、简化接口 |
| Compat 层 | `MyUsed/MyDrivers`、部分 `MyUsed/MyInts` | 保持旧 include 路径和旧 API 兼容 |

> 注：严格来说 `Compat` 可以看作 Facade 的一部分，也可以看作迁移期的第六类辅助层。本文为了说明迁移过程，会单独介绍。

---

## 3. 为什么需要五层架构

### 3.1 避免驱动文件职责混乱

早期单文件驱动常常同时包含：

- 硬件寄存器操作；
- GPIO/I2C/SPI 绑定；
- 设备状态变量；
- 业务接口；
- 中断处理；
- 全局兼容变量。

这样会导致：

1. 一个文件越来越大；
2. 外设逻辑和板级资源强耦合；
3. 换引脚、换定时器、换总线时容易改坏设备算法；
4. 业务层直接依赖底层实现；
5. 很难复用同一类设备的通用逻辑。

五层架构的核心目标就是将这些职责拆开。

### 3.2 提高复用能力

例如 `LED`、`Buzzer` 本质上都是开关型设备，它们可以复用 `device_switch_ops_t` 的思想；`Key` 是输入设备；`Encoder` 是计数设备；`Motor` 是执行器；`ICM42688` 是传感器。

通过抽象 `ops`，可以让不同设备拥有统一的对象调用方式，同时保留各自的具体行为。

### 3.3 降低板级迁移成本

Device 层不应该关心：

- 当前 LED 接在哪个 GPIO；
- 当前 PWM 使用哪个 Timer；
- 当前 I2C 使用哪个 BSP 端口；
- 当前中断号是什么。

这些都属于 Board 层职责。

当 PCB 或 SysConfig 变化时，优先修改 Board 层，而不是修改 Device 层算法。

### 3.4 兼容旧业务代码

当前项目中已经存在不少旧接口，例如：

- `LED_B_On()`；
- `Buzzer_Start()`；
- `Key_GetValue()`；
- `Encoder_Init()`；
- `OLED_Init()`；
- `Xunji_Get()`；
- `Set_PWM()`；
- `Stepper_Start()`；
- `bsp_Icm42688Init()`。

如果直接删除旧接口，会造成业务层大面积修改。因此引入 Facade/Compat 层，让旧代码继续可用，同时内部逐步转向新架构。

---

## 4. Core 层

### 4.1 位置

核心文件：

- `MyUsed/MyCore/device_base.h`
- `MyUsed/MyCore/device_ops.h`

### 4.2 `device_base_t`

`device_base_t` 是所有设备对象的最小公共基类，目前主要保存初始化状态：

```c
typedef struct {
    uint8_t is_init;
} device_base_t;
```

每个 Device 对象都应包含：

```c
device_base_t base;
```

这样每个设备都可以统一判断是否已经初始化。

### 4.3 `device_ops.h`

`device_ops.h` 定义不同设备类型的通用操作集合。

当前包括：

| ops 类型 | 适用设备 | 典型操作 |
| --- | --- | --- |
| `device_switch_ops_t` | LED、Buzzer | `on`、`off`、`toggle` |
| `device_input_ops_t` | Key | `scan`、`read_level` |
| `device_counter_ops_t` | Encoder | `get_count`、`reset_count`、`update_from_ab` |
| `device_display_ops_t` | OLED | `init`、`update`、`clear` |
| `device_sensor_ops_t` | ICM42688 | `init`、`read_raw`、`read_scaled` |
| `device_array_input_ops_t` | Xunji | `read_all` |
| `device_motor_ops_t` | Motor | `start`、`stop`、`set_pwm`、`velocity_calc` |
| `device_stepper_ops_t` | Stepper | `enable`、`set_direction`、`set_frequency`、`start`、`stop`、`service` |

### 4.4 Core 层意义

Core 层不依赖具体硬件，不依赖 SysConfig，不依赖某个具体外设。

它提供的是：

- 统一对象模型；
- 统一初始化状态；
- 统一操作接口风格；
- 后续扩展新设备时的基础约束。

---

## 5. Platform/BSP 层

### 5.1 位置

主要目录：

- `MyUsed/MyPlatform`
- `MyUsed/MyBSP`

典型文件：

- `platform_gpio.h/.c`
- `bsp_gpio.h/.c`
- `bsp_iic.h/.c`
- `bsp_spi.h/.c`

### 5.2 BSP 层职责

BSP 层负责直接对接 MCU DriverLib、寄存器或 SysConfig 生成宏。

例如：

- GPIO 读写；
- I2C 读写寄存器；
- SPI 传输；
- Timer/PWM 控制。

### 5.3 Platform 层职责

Platform 层比 BSP 更接近通用设备抽象。

例如 `platform_gpio_t` 将 GPIO 端口和引脚包装为结构体，让 Device 层可以通过统一接口读写 GPIO，而不直接依赖 `DL_GPIO_setPins()` 等底层函数。

### 5.4 Platform/BSP 层意义

这一层隔离 MCU 厂商库和设备逻辑。

如果将来从 MSPM0G3507 迁移到其他 MCU，理论上优先改 BSP/Platform，而 Device 层逻辑可以尽量保持稳定。

---

## 6. Device 层

### 6.1 位置

目录：

- `MyUsed/MyDevices`

### 6.2 Device 层职责

Device 层是当前架构中最核心的外设类实现层。

每个外设通常包含：

1. 对象结构体；
2. 配置结构体；
3. ops 类型别名；
4. 默认 ops 实现；
5. 初始化函数；
6. 对象方法；
7. 必要的状态变量。

典型形式：

```c
typedef struct xxx_s xxx_t;
typedef device_xxx_ops_t xxx_base_ops_t;

typedef struct {
    // 外设配置
} xxx_cfg_t;

struct xxx_s {
    device_base_t base;
    const xxx_base_ops_t *ops;
    xxx_cfg_t cfg;
    // 运行时状态
};
```

### 6.3 Device 层不应该做什么

Device 层原则上不应该强依赖：

- 某一个具体板子的引脚编号；
- 某一个具体定时器实例；
- 某一个业务场景；
- 某一个 App 的全局变量。

如果必须使用硬件资源，应尽量通过 `cfg` 或回调注入。

---

## 7. Board 层

### 7.1 位置

目录：

- `MyUsed/MyBoards`

### 7.2 Board 层职责

Board 层负责把 Device 层的通用设备类绑定到当前硬件板。

典型职责包括：

- 创建具体对象实例；
- 填写 GPIO 配置；
- 填写 Timer/PWM 配置；
- 绑定 I2C/SPI 端口；
- 绑定 SysConfig 生成的宏；
- 配置 NVIC；
- 放置强板级 ISR。

例如 Encoder 的板级层负责：

- 实例化 Encoder A/B；
- 绑定 A/B 相 GPIO；
- 在 `GROUP1_IRQHandler` 中读取中断状态和引脚电平；
- 调用 `Encoder_UpdateFromAB()` 更新计数。

### 7.3 Board 层意义

Board 层是“硬件差异集中区”。

当硬件引脚变化、PWM 通道变化、I2C 地址变化时，应优先修改 Board 层。

---

## 8. Facade 层

### 8.1 位置

目录：

- `MyUsed/MyFacades`

### 8.2 Facade 层职责

Facade 层面向业务代码，提供简单稳定的接口。

它隐藏对象指针、Board 实例、ops 调用细节，让业务层只调用熟悉的函数。

例如：

```c
LED_B_On();
Key_GetValue();
Encoder_GetCountA();
Buzzer_Start();
```

Facade 内部再转调 Board 层对象和 Device 层方法。

### 8.3 Facade 层意义

Facade 层的价值包括：

1. 业务层接口稳定；
2. 降低业务层认知成本；
3. 隐藏对象化实现；
4. 便于兼容旧 API；
5. 后续重构 Device/Board 时减少业务层改动。

---

## 9. Compat 层

### 9.1 位置

主要是：

- `MyUsed/MyDrivers`
- 部分 `MyUsed/MyInts`

### 9.2 Compat 层职责

Compat 层是迁移期保留的兼容壳。

旧代码可能仍然包含：

```c
#include "motor_driver.h"
#include "OLED_driver.h"
#include "xunji_driver.h"
```

如果直接移动文件，会导致旧 include 失效。

因此旧 `MyDrivers/*.h` 现在主要转发到 `MyFacades/*.h`，旧 `.c` 则保留为空翻译单元，避免重复定义。

### 9.3 Compat 层意义

Compat 层让架构重构可以渐进完成，而不是一次性修改所有业务代码。

长期来看，推荐新代码逐步直接 include Facade 或 Device/Board 头文件，而不是继续依赖旧 Drivers 路径。

---

## 10. 当前各外设功能划分

## 10.1 LED

### Device 层

文件：

- `MyUsed/MyDevices/led_driver.h`
- `MyUsed/MyDevices/led_driver.c`

职责：

- 定义 `led_t`；
- 定义 `led_cfg_t`；
- 继承 `device_switch_ops_t`；
- 实现 `LED_Init()`、`LED_On()`、`LED_Off()`、`LED_Toggle()`；
- 根据 `active_low` 处理实际输出电平。

### Board 层

文件：

- `MyUsed/MyBoards/led_board.h`
- `MyUsed/MyBoards/led_board.c`

职责：

- 实例化蓝灯、红灯；
- 绑定 `LED_BLUE_PORT`、`LED_BLUE_PIN` 等 SysConfig 宏；
- 提供 `LED_Board_GetBlue()`、`LED_Board_GetRed()`。

### Facade 层

文件：

- `MyUsed/MyFacades/led_facade.h`
- `MyUsed/MyFacades/led_facade.c`

职责：

- 保留 `LED_B_On()`、`LED_R_On()` 等业务接口。

---

## 10.2 Buzzer

### Device 层

文件：

- `MyUsed/MyDevices/buzzer_driver.h`
- `MyUsed/MyDevices/buzzer_driver.c`

职责：

- 定义 `buzzer_t`；
- 定义 `buzzer_cfg_t`；
- 继承 `device_switch_ops_t`；
- 实现 `Buzzer_InitDevice()`、`Buzzer_On()`、`Buzzer_Off()`；
- 根据 `active_high` 处理蜂鸣器实际电平。

### Board 层

文件：

- `MyUsed/MyBoards/buzzer_board.h`
- `MyUsed/MyBoards/buzzer_board.c`

职责：

- 实例化蜂鸣器对象；
- 绑定蜂鸣器 GPIO。

### Facade 层

文件：

- `MyUsed/MyFacades/buzzer_facade.h`
- `MyUsed/MyFacades/buzzer_facade.c`

职责：

- 保留 `Buzzer_Start()`、`Buzzer_Stop()`、`Buzzer_Init()` 等接口。

---

## 10.3 Key

### Device 层

文件：

- `MyUsed/MyDevices/key_driver.h`
- `MyUsed/MyDevices/key_driver.c`

职责：

- 定义 `key_t`；
- 定义 `key_cfg_t`；
- 继承 `device_input_ops_t`；
- 实现按键状态机；
- 支持消抖、短按、长按、连发；
- 通过 `get_tick` 回调获取系统时间。

### Board 层

文件：

- `MyUsed/MyBoards/key_board.h`
- `MyUsed/MyBoards/key_board.c`

职责：

- 实例化多个按键；
- 绑定 GPIO；
- 绑定系统 tick。

### Facade 层

文件：

- `MyUsed/MyFacades/key_facade.h`
- `MyUsed/MyFacades/key_facade.c`

职责：

- 保留 `Key_Init()`、`Key_GetValue()`。

---

## 10.4 Encoder

### Device 层

文件：

- `MyUsed/MyDevices/encoder_driver.h`
- `MyUsed/MyDevices/encoder_driver.c`

职责：

- 定义 `encoder_t`；
- 定义 `encoder_cfg_t`；
- 继承 `device_counter_ops_t`；
- 实现 `Encoder_GetCount()`、`Encoder_ResetCount()`、`Encoder_UpdateFromAB()`；
- 通过 A/B 相变化更新计数。

### Board 层

文件：

- `MyUsed/MyBoards/encoder_board.h`
- `MyUsed/MyBoards/encoder_board.c`

职责：

- 实例化 Encoder A/B；
- 绑定 A/B 相 GPIO；
- 配置并响应 GPIO 中断；
- 在 ISR 中调用 Device 层更新计数。

### Facade 层

文件：

- `MyUsed/MyFacades/encoder_facade.h`
- `MyUsed/MyFacades/encoder_facade.c`

职责：

- 保留 `Encoder_Init()`、`Encoder_GetCountA()`、`Encoder_GetCountB()` 等接口；
- 兼容旧全局计数变量。

---

## 10.5 OLED

### Device 层

文件：

- `MyUsed/MyDevices/OLED_driver.h`
- `MyUsed/MyDevices/OLED_driver.c`

职责：

- 保留 OLED 显存和绘图函数；
- 定义 `oled_t`、`oled_cfg_t`；
- 继承 `device_display_ops_t`；
- 封装 `OLED_InitDevice()`、`OLED_GetBaseOps()`；
- 通过 `OLED_SCL`、`OLED_SDA` 回调绑定软件 I2C 线控制。

### Board 层

文件：

- `MyUsed/MyBoards/oled_board.h`
- `MyUsed/MyBoards/oled_board.c`

职责：

- 提供 OLED 板级初始化入口；
- 后续可进一步迁移 `OLED_int.c` 中的 SCL/SDA GPIO 写函数。

### Facade/Compat 层

文件：

- `MyUsed/MyFacades/oled_facade.h`
- `MyUsed/MyDrivers/OLED_driver.h`

职责：

- 保持 `OLED_Init()`、`OLED_Update()`、`OLED_Clear()`、`OLED_ShowString()` 等原 API 可用。

---

## 10.6 ICM42688

### Device 层

文件：

- `MyUsed/MyDevices/icm42688_driver.h`
- `MyUsed/MyDevices/icm42688_driver.c`

职责：

- 保留寄存器读写、配置、数据换算算法；
- 定义 `icm42688_t`、`icm42688_cfg_t`；
- 继承 `device_sensor_ops_t`；
- 封装 `ICM42688_InitDevice()`；
- 封装 `ICM42688_ReadRawDevice()`；
- 封装 `ICM42688_ReadScaledDevice()`。

### Board 层

文件：

- `MyUsed/MyBoards/icm42688_board.h`
- `MyUsed/MyBoards/icm42688_board.c`

职责：

- 提供板级初始化入口；
- 后续可进一步将 I2C 端口、器件地址等从 `icm42688_int` 迁入 Board 层。

### Facade/Compat 层

文件：

- `MyUsed/MyFacades/icm42688_facade.h`
- `MyUsed/MyDrivers/icm42688_driver.h`

职责：

- 保留 `icm42688_init()`、`bsp_Icm42688Init()`、`bsp_IcmGetAccelerometer()` 等旧接口。

---

## 10.7 Xunji 循迹模块

### Device 层

文件：

- `MyUsed/MyDevices/xunji_driver.h`
- `MyUsed/MyDevices/xunji_driver.c`

职责：

- 定义 `xunji_t`；
- 定义 `xunji_cfg_t`；
- 继承 `device_array_input_ops_t`；
- 封装多路输入读取；
- 保留 `Xunji_Driver_Bind()`、`Xunji_Get()`、`Xunji_Init()`。

### Board/Int 层

文件：

- `MyUsed/MyBoards/xunji_board.h`
- `MyUsed/MyBoards/xunji_board.c`
- `MyUsed/MyInts/xunji_int.h`
- `MyUsed/MyInts/xunji_int.c`

职责：

- 绑定五路循迹传感器 GPIO；
- 将每一路 GPIO 读函数注册给 Device 层。

---

## 10.8 Motor 直流电机

### Device 层

文件：

- `MyUsed/MyDevices/motor_driver.h`
- `MyUsed/MyDevices/motor_driver.c`

职责：

- 定义 `motor_t`；
- 定义 `motor_cfg_t`；
- 继承 `device_motor_ops_t`；
- 封装 PWM 输出；
- 封装启动死区处理；
- 封装 PWM 限幅；
- 封装速度 PI 计算；
- 保留 `Motor_Init()`、`Set_PWM()`、`Velocity_A()`、`Velocity_B()` 等旧接口。

### Board 层

文件：

- `MyUsed/MyBoards/motor_board.h`
- `MyUsed/MyBoards/motor_board.c`

职责：

- 提供电机板级初始化入口；
- 后续可进一步将 PWM Timer/Channel 绑定从 Device 默认配置中迁入 Board 层。

---

## 10.9 Stepper 步进电机

### Device 层

文件：

- `MyUsed/MyDevices/stepper_driver.h`
- `MyUsed/MyDevices/stepper_driver.c`

职责：

- 保留双步进电机底层状态表；
- 定义 `stepper_t`；
- 继承 `device_stepper_ops_t`；
- 封装使能、方向、频率、启动、停止、服务节拍；
- 保留 `Stepper_Init()`、`Stepper_Start()`、`Stepper_Stop()` 等旧接口；
- 保留 `STEP_CTRL_TIMER_INST_IRQHandler()`。

### Board 层

文件：

- `MyUsed/MyBoards/stepper_board.h`
- `MyUsed/MyBoards/stepper_board.c`

职责：

- 提供步进电机板级初始化入口；
- 后续建议将定时器实例、GPIO、ISR 进一步迁移到 Board 层。

---

## 11. 当前架构的调用链示例

### 11.1 LED 调用链

```text
业务层 LED_B_On()
    -> Facade: led_facade.c
        -> Board: LED_Board_GetBlue()
            -> Device: LED_On(led_t *)
                -> ops->on()
                    -> Platform_GPIO_Write()
                        -> BSP / DriverLib
```

### 11.2 Encoder 调用链

```text
GPIO 中断 GROUP1_IRQHandler
    -> Board: 读取中断状态和 A/B 相电平
        -> Device: Encoder_UpdateFromAB()
            -> 更新 encoder_t.count

业务层 Encoder_GetCountA()
    -> Facade
        -> Board: Encoder_Board_GetA()
            -> Device: Encoder_GetCount()
```

### 11.3 Motor 调用链

```text
业务层 Set_PWM(left, right)
    -> Device 兼容 API
        -> Motor_SetPwmDevice(&g_default_motor, left, right)
            -> deadzone / limit
                -> DL_Timer_setCaptureCompareValue()
```

---

## 12. 新增外设推荐模板

新增一个外设时，推荐按以下步骤：

1. 判断外设类型：
   - 开关类；
   - 输入类；
   - 计数类；
   - 传感器类；
   - 显示类；
   - 执行器类。

2. 在 `device_ops.h` 中复用或新增合适的 ops。

3. 在 `MyDevices` 中创建：

```text
xxx_driver.h
xxx_driver.c
```

4. 在 Device 头文件中定义：

```c
typedef struct xxx_s xxx_t;
typedef device_xxx_ops_t xxx_base_ops_t;

typedef struct {
    // 配置项
} xxx_cfg_t;

struct xxx_s {
    device_base_t base;
    const xxx_base_ops_t *ops;
    xxx_cfg_t cfg;
};
```

5. 在 `MyBoards` 中创建：

```text
xxx_board.h
xxx_board.c
```

用于绑定当前板子的 GPIO、Timer、I2C、SPI、中断等。

6. 在 `MyFacades` 中创建：

```text
xxx_facade.h
xxx_facade.c
```

用于对业务层提供稳定 API。

7. 如果旧代码已经使用 `MyDrivers/xxx_driver.h`，则在 `MyDrivers` 保留兼容壳。

---

## 13. 当前架构的优点

### 13.1 可维护性更高

每一层职责明确，修改板级硬件不容易影响设备算法。

### 13.2 复用性更强

Device 层对象可以复用到多个实例，例如多个 LED、多个 Key、多个 Encoder。

### 13.3 业务接口稳定

Facade 和 Compat 层保留旧 API，减少业务层大面积修改。

### 13.4 便于测试

Device 层可以通过 mock cfg、mock 回调进行测试，而不一定依赖真实硬件。

### 13.5 便于扩展

新增外设可以照着已有模板实现，统一 `base + ops + cfg + board + facade` 模式。

---

## 14. 后续优化建议

当前架构已经完成第一轮迁移，但仍有一些可以继续优化的方向：

1. 将 OLED 的 SCL/SDA GPIO 写函数从 `OLED_int.c` 进一步迁移到 `oled_board.c`。
2. 将 ICM42688 的 I2C 端口和器件地址从 `icm42688_int.h` 进一步迁移到 `icm42688_board.h/.c`。
3. 将 Motor 的 PWM Timer/Channel 默认配置进一步迁移到 `motor_board.c`，让 Device 层更纯粹。
4. 将 Stepper 的 SysConfig 宏、ISR 和双电机状态表进一步拆分到 Board 层。
5. 新业务代码优先 include `MyFacades/*.h` 或 `MyBoards/*.h`，减少对旧 `MyDrivers` 兼容路径的依赖。
6. 逐步减少全局变量兼容接口，改为函数访问或对象访问。
7. 为每个 Device 层补齐统一风格的 Doxygen 注释。

---

## 15. 总结

当前五层架构的核心思想是：

```text
Core 定义通用规则；
Platform/BSP 屏蔽 MCU 底层差异；
Device 实现外设类本体；
Board 绑定当前硬件实例；
Facade/Compat 稳定业务接口并兼容旧代码。
```

这种结构适合嵌入式工程长期演进，尤其适合当前这种外设数量逐渐增加、业务层逐渐复杂、同时又需要兼容历史代码的项目。

最终目标不是为了“分层而分层”，而是为了让每个文件只承担清晰职责，让外设驱动更容易维护、复用、迁移和扩展。
