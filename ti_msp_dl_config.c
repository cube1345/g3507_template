/*
 * Copyright (c) 2023, Texas Instruments Incorporated
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
 *  ============ ti_msp_dl_config.c =============
 *  Configured MSPM0 DriverLib module definitions
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */

#include "ti_msp_dl_config.h"

DL_TimerG_backupConfig gPWM_1Backup;
DL_TimerG_backupConfig gSTEP_M2_PWMBackup;
DL_TimerA_backupConfig gSYSTIMERBackup;
DL_TimerA_backupConfig gTASKTIMERBackup;

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform any initialization needed before using any board APIs
 */
SYSCONFIG_WEAK void SYSCFG_DL_init(void)
{
    SYSCFG_DL_initPower();
    SYSCFG_DL_GPIO_init();
    /* Module-Specific Initializations*/
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_PWM_0_init();
    SYSCFG_DL_PWM_1_init();
    SYSCFG_DL_STEP_M1_PWM_init();
    SYSCFG_DL_STEP_M2_PWM_init();
    SYSCFG_DL_SYSTIMER_init();
    SYSCFG_DL_TASKTIMER_init();
    SYSCFG_DL_STEP_CTRL_TIMER_init();
    SYSCFG_DL_I2C_0_init();
    SYSCFG_DL_UART_0_init();
    SYSCFG_DL_UART_2_init();
    SYSCFG_DL_UART_1_init();
    /* Ensure backup structures have no valid state */
	gPWM_1Backup.backupRdy 	= false;
	gSTEP_M2_PWMBackup.backupRdy 	= false;
	gSYSTIMERBackup.backupRdy 	= false;
	gTASKTIMERBackup.backupRdy 	= false;


}
/*
 * User should take care to save and restore register configuration in application.
 * See Retention Configuration section for more details.
 */
SYSCONFIG_WEAK bool SYSCFG_DL_saveConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerG_saveConfiguration(PWM_1_INST, &gPWM_1Backup);
	retStatus &= DL_TimerG_saveConfiguration(STEP_M2_PWM_INST, &gSTEP_M2_PWMBackup);
	retStatus &= DL_TimerA_saveConfiguration(SYSTIMER_INST, &gSYSTIMERBackup);
	retStatus &= DL_TimerA_saveConfiguration(TASKTIMER_INST, &gTASKTIMERBackup);

    return retStatus;
}


SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerG_restoreConfiguration(PWM_1_INST, &gPWM_1Backup, false);
	retStatus &= DL_TimerG_restoreConfiguration(STEP_M2_PWM_INST, &gSTEP_M2_PWMBackup, false);
	retStatus &= DL_TimerA_restoreConfiguration(SYSTIMER_INST, &gSYSTIMERBackup, false);
	retStatus &= DL_TimerA_restoreConfiguration(TASKTIMER_INST, &gTASKTIMERBackup, false);

    return retStatus;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerG_reset(PWM_0_INST);
    DL_TimerG_reset(PWM_1_INST);
    DL_TimerG_reset(STEP_M1_PWM_INST);
    DL_TimerG_reset(STEP_M2_PWM_INST);
    DL_TimerA_reset(SYSTIMER_INST);
    DL_TimerA_reset(TASKTIMER_INST);
    DL_TimerG_reset(STEP_CTRL_TIMER_INST);
    DL_I2C_reset(I2C_0_INST);
    DL_UART_Main_reset(UART_0_INST);
    DL_UART_Main_reset(UART_2_INST);
    DL_UART_Main_reset(UART_1_INST);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerG_enablePower(PWM_0_INST);
    DL_TimerG_enablePower(PWM_1_INST);
    DL_TimerG_enablePower(STEP_M1_PWM_INST);
    DL_TimerG_enablePower(STEP_M2_PWM_INST);
    DL_TimerA_enablePower(SYSTIMER_INST);
    DL_TimerA_enablePower(TASKTIMER_INST);
    DL_TimerG_enablePower(STEP_CTRL_TIMER_INST);
    DL_I2C_enablePower(I2C_0_INST);
    DL_UART_Main_enablePower(UART_0_INST);
    DL_UART_Main_enablePower(UART_2_INST);
    DL_UART_Main_enablePower(UART_1_INST);
    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{

    DL_GPIO_initPeripheralAnalogFunction(GPIO_HFXIN_IOMUX);
    DL_GPIO_initPeripheralAnalogFunction(GPIO_HFXOUT_IOMUX);

    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_0_C0_IOMUX,GPIO_PWM_0_C0_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_0_C0_PORT, GPIO_PWM_0_C0_PIN);
    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_0_C1_IOMUX,GPIO_PWM_0_C1_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_0_C1_PORT, GPIO_PWM_0_C1_PIN);
    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_1_C0_IOMUX,GPIO_PWM_1_C0_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_1_C0_PORT, GPIO_PWM_1_C0_PIN);
    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_1_C1_IOMUX,GPIO_PWM_1_C1_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_1_C1_PORT, GPIO_PWM_1_C1_PIN);
    DL_GPIO_initPeripheralOutputFunction(GPIO_STEP_M1_PWM_C0_IOMUX,GPIO_STEP_M1_PWM_C0_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_STEP_M1_PWM_C0_PORT, GPIO_STEP_M1_PWM_C0_PIN);
    DL_GPIO_initPeripheralOutputFunction(GPIO_STEP_M2_PWM_C0_IOMUX,GPIO_STEP_M2_PWM_C0_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_STEP_M2_PWM_C0_PORT, GPIO_STEP_M2_PWM_C0_PIN);

    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_I2C_0_IOMUX_SDA,
        GPIO_I2C_0_IOMUX_SDA_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_I2C_0_IOMUX_SCL,
        GPIO_I2C_0_IOMUX_SCL_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_enableHiZ(GPIO_I2C_0_IOMUX_SDA);
    DL_GPIO_enableHiZ(GPIO_I2C_0_IOMUX_SCL);

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_0_IOMUX_TX, GPIO_UART_0_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_0_IOMUX_RX, GPIO_UART_0_IOMUX_RX_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_2_IOMUX_TX, GPIO_UART_2_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_2_IOMUX_RX, GPIO_UART_2_IOMUX_RX_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_1_IOMUX_TX, GPIO_UART_1_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_1_IOMUX_RX, GPIO_UART_1_IOMUX_RX_FUNC);

    DL_GPIO_initDigitalOutputFeatures(BUZZER_BUZ_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
		 DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);

    DL_GPIO_initDigitalOutputFeatures(LED_BLUE_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);

    DL_GPIO_initDigitalOutputFeatures(LED_RED_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
		 DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);

    DL_GPIO_initDigitalInputFeatures(KEY_NUM0_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(KEY_NUM1_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(KEY_NUM2_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalOutputFeatures(OLED_SCL0_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);

    DL_GPIO_initDigitalOutputFeatures(OLED_SDA0_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);

    DL_GPIO_initDigitalInput(ENCODERA_E1A_IOMUX);

    DL_GPIO_initDigitalInput(ENCODERA_E1B_IOMUX);

    DL_GPIO_initDigitalInputFeatures(XUN_JI0_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(XUN_JI1_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(XUN_JI2_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(XUN_JI3_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(XUN_JI4_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInput(ENCODERB_E2A_IOMUX);

    DL_GPIO_initDigitalInput(ENCODERB_E2B_IOMUX);

    DL_GPIO_initDigitalOutput(STEP_EN_M1_EN_IOMUX);

    DL_GPIO_initDigitalOutput(STEP_EN_M2_EN_IOMUX);

    DL_GPIO_initDigitalOutput(STEP_DIR_M1_DIR_IOMUX);

    DL_GPIO_initDigitalOutput(STEP_DIR_M2_DIR_IOMUX);

    DL_GPIO_clearPins(GPIOA, BUZZER_BUZ_PIN |
		LED_RED_PIN);
    DL_GPIO_setPins(GPIOA, OLED_SCL0_PIN |
		OLED_SDA0_PIN |
		STEP_EN_M1_EN_PIN |
		STEP_EN_M2_EN_PIN);
    DL_GPIO_enableOutput(GPIOA, BUZZER_BUZ_PIN |
		LED_RED_PIN |
		OLED_SCL0_PIN |
		OLED_SDA0_PIN |
		STEP_EN_M1_EN_PIN |
		STEP_EN_M2_EN_PIN);
    DL_GPIO_setLowerPinsPolarity(GPIOA, DL_GPIO_PIN_14_EDGE_RISE);
    DL_GPIO_setUpperPinsPolarity(GPIOA, DL_GPIO_PIN_16_EDGE_RISE);
    DL_GPIO_clearInterruptStatus(GPIOA, ENCODERA_E1A_PIN |
		ENCODERA_E1B_PIN);
    DL_GPIO_enableInterrupt(GPIOA, ENCODERA_E1A_PIN |
		ENCODERA_E1B_PIN);
    DL_GPIO_clearPins(GPIOB, STEP_DIR_M1_DIR_PIN |
		STEP_DIR_M2_DIR_PIN);
    DL_GPIO_setPins(GPIOB, LED_BLUE_PIN);
    DL_GPIO_enableOutput(GPIOB, LED_BLUE_PIN |
		STEP_DIR_M1_DIR_PIN |
		STEP_DIR_M2_DIR_PIN);
    DL_GPIO_setUpperPinsPolarity(GPIOB, DL_GPIO_PIN_20_EDGE_RISE |
		DL_GPIO_PIN_24_EDGE_RISE);
    DL_GPIO_clearInterruptStatus(GPIOB, ENCODERB_E2A_PIN |
		ENCODERB_E2B_PIN);
    DL_GPIO_enableInterrupt(GPIOB, ENCODERB_E2A_PIN |
		ENCODERB_E2B_PIN);

}


static const DL_SYSCTL_SYSPLLConfig gSYSPLLConfig = {
    .inputFreq              = DL_SYSCTL_SYSPLL_INPUT_FREQ_8_16_MHZ,
	.rDivClk2x              = 3,
	.rDivClk1               = 0,
	.rDivClk0               = 0,
	.enableCLK2x            = DL_SYSCTL_SYSPLL_CLK2X_ENABLE,
	.enableCLK1             = DL_SYSCTL_SYSPLL_CLK1_DISABLE,
	.enableCLK0             = DL_SYSCTL_SYSPLL_CLK0_DISABLE,
	.sysPLLMCLK             = DL_SYSCTL_SYSPLL_MCLK_CLK2X,
	.sysPLLRef              = DL_SYSCTL_SYSPLL_REF_HFCLK,
	.qDiv                   = 15,
	.pDiv                   = DL_SYSCTL_SYSPLL_PDIV_4
};
SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{

	//Low Power Mode is configured to be SLEEP0
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);
    DL_SYSCTL_setFlashWaitState(DL_SYSCTL_FLASH_WAIT_STATE_2);

    
	DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
	/* Set default configuration */
	DL_SYSCTL_disableHFXT();
	DL_SYSCTL_disableSYSPLL();
    DL_SYSCTL_setHFCLKSourceHFXTParams(DL_SYSCTL_HFXT_RANGE_32_48_MHZ,255, true);
    DL_SYSCTL_configSYSPLL((DL_SYSCTL_SYSPLLConfig *) &gSYSPLLConfig);
    DL_SYSCTL_setULPCLKDivider(DL_SYSCTL_ULPCLK_DIV_2);
    DL_SYSCTL_setMCLKSource(SYSOSC, HSCLK, DL_SYSCTL_HSCLK_SOURCE_SYSPLL);

}


/*
 * Timer clock configuration to be sourced by  / 1 (40000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   40000000 Hz = 40000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gPWM_0ClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale = 0U
};

static const DL_TimerG_PWMConfig gPWM_0Config = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP,
    .period = 8000,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_PWM_0_init(void) {

    DL_TimerG_setClockConfig(
        PWM_0_INST, (DL_TimerG_ClockConfig *) &gPWM_0ClockConfig);

    DL_TimerG_initPWMMode(
        PWM_0_INST, (DL_TimerG_PWMConfig *) &gPWM_0Config);

    DL_TimerG_setCaptureCompareOutCtl(PWM_0_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_0_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(PWM_0_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_0_INDEX);
    DL_TimerG_setCaptureCompareValue(PWM_0_INST, 0, DL_TIMER_CC_0_INDEX);

    DL_TimerG_setCaptureCompareOutCtl(PWM_0_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_1_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(PWM_0_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_1_INDEX);
    DL_TimerG_setCaptureCompareValue(PWM_0_INST, 0, DL_TIMER_CC_1_INDEX);

    DL_TimerG_enableClock(PWM_0_INST);


    
    DL_TimerG_setCCPDirection(PWM_0_INST , DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT );


}
/*
 * Timer clock configuration to be sourced by  / 1 (80000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   80000000 Hz = 80000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gPWM_1ClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale = 0U
};

static const DL_TimerG_PWMConfig gPWM_1Config = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP,
    .period = 8000,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_PWM_1_init(void) {

    DL_TimerG_setClockConfig(
        PWM_1_INST, (DL_TimerG_ClockConfig *) &gPWM_1ClockConfig);

    DL_TimerG_initPWMMode(
        PWM_1_INST, (DL_TimerG_PWMConfig *) &gPWM_1Config);

    DL_TimerG_setCaptureCompareOutCtl(PWM_1_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_0_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(PWM_1_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_0_INDEX);
    DL_TimerG_setCaptureCompareValue(PWM_1_INST, 0, DL_TIMER_CC_0_INDEX);

    DL_TimerG_setCaptureCompareOutCtl(PWM_1_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_1_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(PWM_1_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_1_INDEX);
    DL_TimerG_setCaptureCompareValue(PWM_1_INST, 0, DL_TIMER_CC_1_INDEX);

    DL_TimerG_enableClock(PWM_1_INST);


    
    DL_TimerG_setCCPDirection(PWM_1_INST , DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT );


}
/*
 * Timer clock configuration to be sourced by  / 1 (40000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   40000000 Hz = 40000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gSTEP_M1_PWMClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale = 0U
};

static const DL_TimerG_PWMConfig gSTEP_M1_PWMConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
    .period = 28000,
    .startTimer = DL_TIMER_STOP,
};

SYSCONFIG_WEAK void SYSCFG_DL_STEP_M1_PWM_init(void) {

    DL_TimerG_setClockConfig(
        STEP_M1_PWM_INST, (DL_TimerG_ClockConfig *) &gSTEP_M1_PWMClockConfig);

    DL_TimerG_initPWMMode(
        STEP_M1_PWM_INST, (DL_TimerG_PWMConfig *) &gSTEP_M1_PWMConfig);

    DL_TimerG_setCaptureCompareOutCtl(STEP_M1_PWM_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_0_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(STEP_M1_PWM_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_0_INDEX);
    DL_TimerG_setCaptureCompareValue(STEP_M1_PWM_INST, 194, DL_TIMER_CC_0_INDEX);

    DL_TimerG_enableClock(STEP_M1_PWM_INST);


    
    DL_TimerG_setCCPDirection(STEP_M1_PWM_INST , DL_TIMER_CC0_OUTPUT );


}
/*
 * Timer clock configuration to be sourced by  / 1 (80000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   80000000 Hz = 80000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gSTEP_M2_PWMClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale = 0U
};

static const DL_TimerG_PWMConfig gSTEP_M2_PWMConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
    .period = 56000,
    .startTimer = DL_TIMER_STOP,
};

SYSCONFIG_WEAK void SYSCFG_DL_STEP_M2_PWM_init(void) {

    DL_TimerG_setClockConfig(
        STEP_M2_PWM_INST, (DL_TimerG_ClockConfig *) &gSTEP_M2_PWMClockConfig);

    DL_TimerG_initPWMMode(
        STEP_M2_PWM_INST, (DL_TimerG_PWMConfig *) &gSTEP_M2_PWMConfig);

    DL_TimerG_setCaptureCompareOutCtl(STEP_M2_PWM_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_0_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(STEP_M2_PWM_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_0_INDEX);
    DL_TimerG_setCaptureCompareValue(STEP_M2_PWM_INST, 389, DL_TIMER_CC_0_INDEX);

    DL_TimerG_enableClock(STEP_M2_PWM_INST);


    
    DL_TimerG_setCCPDirection(STEP_M2_PWM_INST , DL_TIMER_CC0_OUTPUT );


}



/*
 * Timer clock configuration to be sourced by BUSCLK /  (80000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   1000000 Hz = 80000000 Hz / (1 * (79 + 1))
 */
static const DL_TimerA_ClockConfig gSYSTIMERClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 79U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * SYSTIMER_INST_LOAD_VALUE = (1ms * 1000000 Hz) - 1
 */
static const DL_TimerA_TimerConfig gSYSTIMERTimerConfig = {
    .period     = SYSTIMER_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_SYSTIMER_init(void) {

    DL_TimerA_setClockConfig(SYSTIMER_INST,
        (DL_TimerA_ClockConfig *) &gSYSTIMERClockConfig);

    DL_TimerA_initTimerMode(SYSTIMER_INST,
        (DL_TimerA_TimerConfig *) &gSYSTIMERTimerConfig);
    DL_TimerA_enableInterrupt(SYSTIMER_INST , DL_TIMERA_INTERRUPT_ZERO_EVENT);
    DL_TimerA_enableClock(SYSTIMER_INST);





}

/*
 * Timer clock configuration to be sourced by BUSCLK /  (80000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   1012658.2278481013 Hz = 80000000 Hz / (1 * (78 + 1))
 */
static const DL_TimerA_ClockConfig gTASKTIMERClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 78U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TASKTIMER_INST_LOAD_VALUE = (0.01ms * 1012658.2278481013 Hz) - 1
 */
static const DL_TimerA_TimerConfig gTASKTIMERTimerConfig = {
    .period     = TASKTIMER_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_TASKTIMER_init(void) {

    DL_TimerA_setClockConfig(TASKTIMER_INST,
        (DL_TimerA_ClockConfig *) &gTASKTIMERClockConfig);

    DL_TimerA_initTimerMode(TASKTIMER_INST,
        (DL_TimerA_TimerConfig *) &gTASKTIMERTimerConfig);
    DL_TimerA_enableInterrupt(TASKTIMER_INST , DL_TIMERA_INTERRUPT_ZERO_EVENT);
    DL_TimerA_enableClock(TASKTIMER_INST);





}

/*
 * Timer clock configuration to be sourced by BUSCLK /  (80000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   80000000 Hz = 80000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gSTEP_CTRL_TIMERClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 0U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * STEP_CTRL_TIMER_INST_LOAD_VALUE = (1 ms * 80000000 Hz) - 1
 */
static const DL_TimerG_TimerConfig gSTEP_CTRL_TIMERTimerConfig = {
    .period     = STEP_CTRL_TIMER_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC_UP,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_STEP_CTRL_TIMER_init(void) {

    DL_TimerG_setClockConfig(STEP_CTRL_TIMER_INST,
        (DL_TimerG_ClockConfig *) &gSTEP_CTRL_TIMERClockConfig);

    DL_TimerG_initTimerMode(STEP_CTRL_TIMER_INST,
        (DL_TimerG_TimerConfig *) &gSTEP_CTRL_TIMERTimerConfig);
    DL_TimerG_enableInterrupt(STEP_CTRL_TIMER_INST , DL_TIMERG_INTERRUPT_ZERO_EVENT);
    DL_TimerG_enableClock(STEP_CTRL_TIMER_INST);





}


static const DL_I2C_ClockConfig gI2C_0ClockConfig = {
    .clockSel = DL_I2C_CLOCK_BUSCLK,
    .divideRatio = DL_I2C_CLOCK_DIVIDE_1,
};

SYSCONFIG_WEAK void SYSCFG_DL_I2C_0_init(void) {

    DL_I2C_setClockConfig(I2C_0_INST,
        (DL_I2C_ClockConfig *) &gI2C_0ClockConfig);
    DL_I2C_setAnalogGlitchFilterPulseWidth(I2C_0_INST,
        DL_I2C_ANALOG_GLITCH_FILTER_WIDTH_50NS);
    DL_I2C_enableAnalogGlitchFilter(I2C_0_INST);

    /* Configure Controller Mode */
    DL_I2C_resetControllerTransfer(I2C_0_INST);
    /* Set frequency to 100000 Hz*/
    DL_I2C_setTimerPeriod(I2C_0_INST, 39);
    DL_I2C_setControllerTXFIFOThreshold(I2C_0_INST, DL_I2C_TX_FIFO_LEVEL_EMPTY);
    DL_I2C_setControllerRXFIFOThreshold(I2C_0_INST, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
    DL_I2C_enableControllerClockStretching(I2C_0_INST);

    /* Configure Interrupts */
    DL_I2C_enableInterrupt(I2C_0_INST,
                           DL_I2C_INTERRUPT_CONTROLLER_RXFIFO_TRIGGER |
                           DL_I2C_INTERRUPT_CONTROLLER_RX_DONE |
                           DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER |
                           DL_I2C_INTERRUPT_CONTROLLER_TX_DONE);


    /* Enable module */
    DL_I2C_enableController(I2C_0_INST);


}


static const DL_UART_Main_ClockConfig gUART_0ClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_0Config = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_0_init(void)
{
    DL_UART_Main_setClockConfig(UART_0_INST, (DL_UART_Main_ClockConfig *) &gUART_0ClockConfig);

    DL_UART_Main_init(UART_0_INST, (DL_UART_Main_Config *) &gUART_0Config);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115190.78
     */
    DL_UART_Main_setOversampling(UART_0_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_0_INST, UART_0_IBRD_40_MHZ_115200_BAUD, UART_0_FBRD_40_MHZ_115200_BAUD);


    /* Configure Interrupts */
    DL_UART_Main_enableInterrupt(UART_0_INST,
                                 DL_UART_MAIN_INTERRUPT_RX);


    DL_UART_Main_enable(UART_0_INST);
}

static const DL_UART_Main_ClockConfig gUART_2ClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_2Config = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_2_init(void)
{
    DL_UART_Main_setClockConfig(UART_2_INST, (DL_UART_Main_ClockConfig *) &gUART_2ClockConfig);

    DL_UART_Main_init(UART_2_INST, (DL_UART_Main_Config *) &gUART_2Config);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115190.78
     */
    DL_UART_Main_setOversampling(UART_2_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_2_INST, UART_2_IBRD_40_MHZ_115200_BAUD, UART_2_FBRD_40_MHZ_115200_BAUD);


    /* Configure Interrupts */
    DL_UART_Main_enableInterrupt(UART_2_INST,
                                 DL_UART_MAIN_INTERRUPT_RX);


    DL_UART_Main_enable(UART_2_INST);
}

static const DL_UART_Main_ClockConfig gUART_1ClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_1Config = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_1_init(void)
{
    DL_UART_Main_setClockConfig(UART_1_INST, (DL_UART_Main_ClockConfig *) &gUART_1ClockConfig);

    DL_UART_Main_init(UART_1_INST, (DL_UART_Main_Config *) &gUART_1Config);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115190.78
     */
    DL_UART_Main_setOversampling(UART_1_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_1_INST, UART_1_IBRD_40_MHZ_115200_BAUD, UART_1_FBRD_40_MHZ_115200_BAUD);



    DL_UART_Main_enable(UART_1_INST);
}

