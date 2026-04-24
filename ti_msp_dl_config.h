/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define GPIO_HFXT_PORT                                                     GPIOA
#define GPIO_HFXIN_PIN                                             DL_GPIO_PIN_5
#define GPIO_HFXIN_IOMUX                                         (IOMUX_PINCM10)
#define GPIO_HFXOUT_PIN                                            DL_GPIO_PIN_6
#define GPIO_HFXOUT_IOMUX                                        (IOMUX_PINCM11)
#define CPUCLK_FREQ                                                     80000000



/* Defines for PWM_0 */
#define PWM_0_INST                                                         TIMG8
#define PWM_0_INST_IRQHandler                                   TIMG8_IRQHandler
#define PWM_0_INST_INT_IRQN                                     (TIMG8_INT_IRQn)
#define PWM_0_INST_CLK_FREQ                                             40000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_0_C0_PORT                                                 GPIOB
#define GPIO_PWM_0_C0_PIN                                          DL_GPIO_PIN_6
#define GPIO_PWM_0_C0_IOMUX                                      (IOMUX_PINCM23)
#define GPIO_PWM_0_C0_IOMUX_FUNC                     IOMUX_PINCM23_PF_TIMG8_CCP0
#define GPIO_PWM_0_C0_IDX                                    DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_0_C1_PORT                                                 GPIOB
#define GPIO_PWM_0_C1_PIN                                          DL_GPIO_PIN_7
#define GPIO_PWM_0_C1_IOMUX                                      (IOMUX_PINCM24)
#define GPIO_PWM_0_C1_IOMUX_FUNC                     IOMUX_PINCM24_PF_TIMG8_CCP1
#define GPIO_PWM_0_C1_IDX                                    DL_TIMER_CC_1_INDEX

/* Defines for PWM_1 */
#define PWM_1_INST                                                         TIMG7
#define PWM_1_INST_IRQHandler                                   TIMG7_IRQHandler
#define PWM_1_INST_INT_IRQN                                     (TIMG7_INT_IRQn)
#define PWM_1_INST_CLK_FREQ                                             80000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_1_C0_PORT                                                 GPIOA
#define GPIO_PWM_1_C0_PIN                                         DL_GPIO_PIN_17
#define GPIO_PWM_1_C0_IOMUX                                      (IOMUX_PINCM39)
#define GPIO_PWM_1_C0_IOMUX_FUNC                     IOMUX_PINCM39_PF_TIMG7_CCP0
#define GPIO_PWM_1_C0_IDX                                    DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_1_C1_PORT                                                 GPIOA
#define GPIO_PWM_1_C1_PIN                                          DL_GPIO_PIN_7
#define GPIO_PWM_1_C1_IOMUX                                      (IOMUX_PINCM14)
#define GPIO_PWM_1_C1_IOMUX_FUNC                     IOMUX_PINCM14_PF_TIMG7_CCP1
#define GPIO_PWM_1_C1_IDX                                    DL_TIMER_CC_1_INDEX

/* Defines for STEP_M1_PWM */
#define STEP_M1_PWM_INST                                                   TIMG0
#define STEP_M1_PWM_INST_IRQHandler                             TIMG0_IRQHandler
#define STEP_M1_PWM_INST_INT_IRQN                               (TIMG0_INT_IRQn)
#define STEP_M1_PWM_INST_CLK_FREQ                                       40000000
/* GPIO defines for channel 0 */
#define GPIO_STEP_M1_PWM_C0_PORT                                           GPIOA
#define GPIO_STEP_M1_PWM_C0_PIN                                   DL_GPIO_PIN_23
#define GPIO_STEP_M1_PWM_C0_IOMUX                                (IOMUX_PINCM53)
#define GPIO_STEP_M1_PWM_C0_IOMUX_FUNC               IOMUX_PINCM53_PF_TIMG0_CCP0
#define GPIO_STEP_M1_PWM_C0_IDX                              DL_TIMER_CC_0_INDEX

/* Defines for STEP_M2_PWM */
#define STEP_M2_PWM_INST                                                   TIMG6
#define STEP_M2_PWM_INST_IRQHandler                             TIMG6_IRQHandler
#define STEP_M2_PWM_INST_INT_IRQN                               (TIMG6_INT_IRQn)
#define STEP_M2_PWM_INST_CLK_FREQ                                       80000000
/* GPIO defines for channel 0 */
#define GPIO_STEP_M2_PWM_C0_PORT                                           GPIOA
#define GPIO_STEP_M2_PWM_C0_PIN                                   DL_GPIO_PIN_29
#define GPIO_STEP_M2_PWM_C0_IOMUX                                 (IOMUX_PINCM4)
#define GPIO_STEP_M2_PWM_C0_IOMUX_FUNC                IOMUX_PINCM4_PF_TIMG6_CCP0
#define GPIO_STEP_M2_PWM_C0_IDX                              DL_TIMER_CC_0_INDEX



/* Defines for SYSTIMER */
#define SYSTIMER_INST                                                    (TIMA1)
#define SYSTIMER_INST_IRQHandler                                TIMA1_IRQHandler
#define SYSTIMER_INST_INT_IRQN                                  (TIMA1_INT_IRQn)
#define SYSTIMER_INST_LOAD_VALUE                                          (999U)
/* Defines for TASKTIMER */
#define TASKTIMER_INST                                                   (TIMA0)
#define TASKTIMER_INST_IRQHandler                               TIMA0_IRQHandler
#define TASKTIMER_INST_INT_IRQN                                 (TIMA0_INT_IRQn)
#define TASKTIMER_INST_LOAD_VALUE                                           (9U)
/* Defines for STEP_CTRL_TIMER */
#define STEP_CTRL_TIMER_INST                                            (TIMG12)
#define STEP_CTRL_TIMER_INST_IRQHandler                        TIMG12_IRQHandler
#define STEP_CTRL_TIMER_INST_INT_IRQN                          (TIMG12_INT_IRQn)
#define STEP_CTRL_TIMER_INST_LOAD_VALUE                                 (79999U)




/* Defines for I2C_0 */
#define I2C_0_INST                                                          I2C0
#define I2C_0_INST_IRQHandler                                    I2C0_IRQHandler
#define I2C_0_INST_INT_IRQN                                        I2C0_INT_IRQn
#define I2C_0_BUS_SPEED_HZ                                                100000
#define GPIO_I2C_0_SDA_PORT                                                GPIOA
#define GPIO_I2C_0_SDA_PIN                                         DL_GPIO_PIN_0
#define GPIO_I2C_0_IOMUX_SDA                                      (IOMUX_PINCM1)
#define GPIO_I2C_0_IOMUX_SDA_FUNC                       IOMUX_PINCM1_PF_I2C0_SDA
#define GPIO_I2C_0_SCL_PORT                                                GPIOA
#define GPIO_I2C_0_SCL_PIN                                         DL_GPIO_PIN_1
#define GPIO_I2C_0_IOMUX_SCL                                      (IOMUX_PINCM2)
#define GPIO_I2C_0_IOMUX_SCL_FUNC                       IOMUX_PINCM2_PF_I2C0_SCL


/* Defines for UART_0 */
#define UART_0_INST                                                        UART0
#define UART_0_INST_IRQHandler                                  UART0_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART0_INT_IRQn
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                        DL_GPIO_PIN_11
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_10
#define GPIO_UART_0_IOMUX_RX                                     (IOMUX_PINCM22)
#define GPIO_UART_0_IOMUX_TX                                     (IOMUX_PINCM21)
#define GPIO_UART_0_IOMUX_RX_FUNC                      IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                      IOMUX_PINCM21_PF_UART0_TX
#define UART_0_BAUD_RATE                                                (115200)
#define UART_0_IBRD_40_MHZ_115200_BAUD                                      (21)
#define UART_0_FBRD_40_MHZ_115200_BAUD                                      (45)
/* Defines for UART_2 */
#define UART_2_INST                                                        UART2
#define UART_2_INST_IRQHandler                                  UART2_IRQHandler
#define UART_2_INST_INT_IRQN                                      UART2_INT_IRQn
#define GPIO_UART_2_RX_PORT                                                GPIOB
#define GPIO_UART_2_TX_PORT                                                GPIOB
#define GPIO_UART_2_RX_PIN                                        DL_GPIO_PIN_16
#define GPIO_UART_2_TX_PIN                                        DL_GPIO_PIN_17
#define GPIO_UART_2_IOMUX_RX                                     (IOMUX_PINCM33)
#define GPIO_UART_2_IOMUX_TX                                     (IOMUX_PINCM43)
#define GPIO_UART_2_IOMUX_RX_FUNC                      IOMUX_PINCM33_PF_UART2_RX
#define GPIO_UART_2_IOMUX_TX_FUNC                      IOMUX_PINCM43_PF_UART2_TX
#define UART_2_BAUD_RATE                                                (115200)
#define UART_2_IBRD_40_MHZ_115200_BAUD                                      (21)
#define UART_2_FBRD_40_MHZ_115200_BAUD                                      (45)
/* Defines for UART_1 */
#define UART_1_INST                                                        UART1
#define UART_1_INST_IRQHandler                                  UART1_IRQHandler
#define UART_1_INST_INT_IRQN                                      UART1_INT_IRQn
#define GPIO_UART_1_RX_PORT                                                GPIOA
#define GPIO_UART_1_TX_PORT                                                GPIOA
#define GPIO_UART_1_RX_PIN                                         DL_GPIO_PIN_9
#define GPIO_UART_1_TX_PIN                                         DL_GPIO_PIN_8
#define GPIO_UART_1_IOMUX_RX                                     (IOMUX_PINCM20)
#define GPIO_UART_1_IOMUX_TX                                     (IOMUX_PINCM19)
#define GPIO_UART_1_IOMUX_RX_FUNC                      IOMUX_PINCM20_PF_UART1_RX
#define GPIO_UART_1_IOMUX_TX_FUNC                      IOMUX_PINCM19_PF_UART1_TX
#define UART_1_BAUD_RATE                                                (115200)
#define UART_1_IBRD_40_MHZ_115200_BAUD                                      (21)
#define UART_1_FBRD_40_MHZ_115200_BAUD                                      (45)





/* Port definition for Pin Group BUZZER */
#define BUZZER_PORT                                                      (GPIOA)

/* Defines for BUZ: GPIOA.22 with pinCMx 47 on package pin 18 */
#define BUZZER_BUZ_PIN                                          (DL_GPIO_PIN_22)
#define BUZZER_BUZ_IOMUX                                         (IOMUX_PINCM47)
/* Defines for BLUE: GPIOB.9 with pinCMx 26 on package pin 61 */
#define LED_BLUE_PORT                                                    (GPIOB)
#define LED_BLUE_PIN                                             (DL_GPIO_PIN_9)
#define LED_BLUE_IOMUX                                           (IOMUX_PINCM26)
/* Defines for RED: GPIOA.15 with pinCMx 37 on package pin 8 */
#define LED_RED_PORT                                                     (GPIOA)
#define LED_RED_PIN                                             (DL_GPIO_PIN_15)
#define LED_RED_IOMUX                                            (IOMUX_PINCM37)
/* Defines for NUM0: GPIOB.8 with pinCMx 25 on package pin 60 */
#define KEY_NUM0_PORT                                                    (GPIOB)
#define KEY_NUM0_PIN                                             (DL_GPIO_PIN_8)
#define KEY_NUM0_IOMUX                                           (IOMUX_PINCM25)
/* Defines for NUM1: GPIOA.18 with pinCMx 40 on package pin 11 */
#define KEY_NUM1_PORT                                                    (GPIOA)
#define KEY_NUM1_PIN                                            (DL_GPIO_PIN_18)
#define KEY_NUM1_IOMUX                                           (IOMUX_PINCM40)
/* Defines for NUM2: GPIOA.13 with pinCMx 35 on package pin 6 */
#define KEY_NUM2_PORT                                                    (GPIOA)
#define KEY_NUM2_PIN                                            (DL_GPIO_PIN_13)
#define KEY_NUM2_IOMUX                                           (IOMUX_PINCM35)
/* Port definition for Pin Group OLED */
#define OLED_PORT                                                        (GPIOA)

/* Defines for SCL0: GPIOA.28 with pinCMx 3 on package pin 35 */
#define OLED_SCL0_PIN                                           (DL_GPIO_PIN_28)
#define OLED_SCL0_IOMUX                                           (IOMUX_PINCM3)
/* Defines for SDA0: GPIOA.31 with pinCMx 6 on package pin 39 */
#define OLED_SDA0_PIN                                           (DL_GPIO_PIN_31)
#define OLED_SDA0_IOMUX                                           (IOMUX_PINCM6)
/* Port definition for Pin Group ENCODERA */
#define ENCODERA_PORT                                                    (GPIOA)

/* Defines for E1A: GPIOA.16 with pinCMx 38 on package pin 9 */
// pins affected by this interrupt request:["E1A","E1B"]
#define ENCODERA_INT_IRQN                                       (GPIOA_INT_IRQn)
#define ENCODERA_INT_IIDX                       (DL_INTERRUPT_GROUP1_IIDX_GPIOA)
#define ENCODERA_E1A_IIDX                                   (DL_GPIO_IIDX_DIO16)
#define ENCODERA_E1A_PIN                                        (DL_GPIO_PIN_16)
#define ENCODERA_E1A_IOMUX                                       (IOMUX_PINCM38)
/* Defines for E1B: GPIOA.14 with pinCMx 36 on package pin 7 */
#define ENCODERA_E1B_IIDX                                   (DL_GPIO_IIDX_DIO14)
#define ENCODERA_E1B_PIN                                        (DL_GPIO_PIN_14)
#define ENCODERA_E1B_IOMUX                                       (IOMUX_PINCM36)
/* Port definition for Pin Group XUN */
#define XUN_PORT                                                         (GPIOA)

/* Defines for JI0: GPIOA.26 with pinCMx 59 on package pin 30 */
#define XUN_JI0_PIN                                             (DL_GPIO_PIN_26)
#define XUN_JI0_IOMUX                                            (IOMUX_PINCM59)
/* Defines for JI1: GPIOA.25 with pinCMx 55 on package pin 26 */
#define XUN_JI1_PIN                                             (DL_GPIO_PIN_25)
#define XUN_JI1_IOMUX                                            (IOMUX_PINCM55)
/* Defines for JI2: GPIOA.12 with pinCMx 34 on package pin 5 */
#define XUN_JI2_PIN                                             (DL_GPIO_PIN_12)
#define XUN_JI2_IOMUX                                            (IOMUX_PINCM34)
/* Defines for JI3: GPIOA.27 with pinCMx 60 on package pin 31 */
#define XUN_JI3_PIN                                             (DL_GPIO_PIN_27)
#define XUN_JI3_IOMUX                                            (IOMUX_PINCM60)
/* Defines for JI4: GPIOA.24 with pinCMx 54 on package pin 25 */
#define XUN_JI4_PIN                                             (DL_GPIO_PIN_24)
#define XUN_JI4_IOMUX                                            (IOMUX_PINCM54)
/* Port definition for Pin Group ENCODERB */
#define ENCODERB_PORT                                                    (GPIOB)

/* Defines for E2A: GPIOB.20 with pinCMx 48 on package pin 19 */
// pins affected by this interrupt request:["E2A","E2B"]
#define ENCODERB_INT_IRQN                                       (GPIOB_INT_IRQn)
#define ENCODERB_INT_IIDX                       (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define ENCODERB_E2A_IIDX                                   (DL_GPIO_IIDX_DIO20)
#define ENCODERB_E2A_PIN                                        (DL_GPIO_PIN_20)
#define ENCODERB_E2A_IOMUX                                       (IOMUX_PINCM48)
/* Defines for E2B: GPIOB.24 with pinCMx 52 on package pin 23 */
#define ENCODERB_E2B_IIDX                                   (DL_GPIO_IIDX_DIO24)
#define ENCODERB_E2B_PIN                                        (DL_GPIO_PIN_24)
#define ENCODERB_E2B_IOMUX                                       (IOMUX_PINCM52)
/* Port definition for Pin Group STEP_EN */
#define STEP_EN_PORT                                                     (GPIOA)

/* Defines for M1_EN: GPIOA.21 with pinCMx 46 on package pin 17 */
#define STEP_EN_M1_EN_PIN                                       (DL_GPIO_PIN_21)
#define STEP_EN_M1_EN_IOMUX                                      (IOMUX_PINCM46)
/* Defines for M2_EN: GPIOA.30 with pinCMx 5 on package pin 37 */
#define STEP_EN_M2_EN_PIN                                       (DL_GPIO_PIN_30)
#define STEP_EN_M2_EN_IOMUX                                       (IOMUX_PINCM5)
/* Port definition for Pin Group STEP_DIR */
#define STEP_DIR_PORT                                                    (GPIOB)

/* Defines for M1_DIR: GPIOB.2 with pinCMx 15 on package pin 50 */
#define STEP_DIR_M1_DIR_PIN                                      (DL_GPIO_PIN_2)
#define STEP_DIR_M1_DIR_IOMUX                                    (IOMUX_PINCM15)
/* Defines for M2_DIR: GPIOB.3 with pinCMx 16 on package pin 51 */
#define STEP_DIR_M2_DIR_PIN                                      (DL_GPIO_PIN_3)
#define STEP_DIR_M2_DIR_IOMUX                                    (IOMUX_PINCM16)

/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWM_0_init(void);
void SYSCFG_DL_PWM_1_init(void);
void SYSCFG_DL_STEP_M1_PWM_init(void);
void SYSCFG_DL_STEP_M2_PWM_init(void);
void SYSCFG_DL_SYSTIMER_init(void);
void SYSCFG_DL_TASKTIMER_init(void);
void SYSCFG_DL_STEP_CTRL_TIMER_init(void);
void SYSCFG_DL_I2C_0_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_UART_2_init(void);
void SYSCFG_DL_UART_1_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
