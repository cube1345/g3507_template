# 文件说明
ICM42688.*: ICM42688的读写操作

IMU.*: IMU算法操作

I2C_communication.*:硬件I2C通信驱动

# syscfg配置
见HW_I2C.syscfg/ti_msp_dl_config.*

注：在MSPM0G3507中，PA0/PA1是5V耐压开漏输出

# 项目结构说明
ICM42688 (Project Root)
├── empty.c (应用主入口/Main Loop)
├── HW_I2C.syscfg (硬件资源配置 - 定义引脚/速率)
├── readme.md
├──  IMU (应用逻辑层)
│   ├── IMU.c
│   └── IMU.h
├──  ICM42688 (芯片驱动层)
│   ├── icm42688.c
│   └── icm42688.h
└──  IIC (硬件抽象层/驱动接口)
    ├── I2C_communication.c
    └── I2C_communication.h