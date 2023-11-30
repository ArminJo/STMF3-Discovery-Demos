/*
 * main.cpp
 *
 *
 *  Copyright (C) 2012-2023  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of STMF3-Discovery-Demos https://github.com/ArminJo/STMF3-Discovery-Demos.
 *
 *  STMF3-Discovery-Demos is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 */

// file:///E:/WORKSPACE_STM32/TouchScreenApps/html/index.html
/* Includes ------------------------------------------------------------------*/
/*
 * Enabling program features dependent on display configuration
 */
/*
 * Settings to configure the BlueDisplay library and to reduce its size
 */
//#define BLUETOOTH_BAUD_RATE BAUD_115200  // Activate this, if you have reprogrammed the HC05 module for 115200, otherwise 9600 is used as baud rate
//#define DO_NOT_NEED_BASIC_TOUCH_EVENTS // Disables basic touch events like down, move and up. Saves 620 bytes program memory and 36 bytes RAM
//#define USE_SIMPLE_SERIAL // Do not use the Serial object. Saves up to 1250 bytes program memory and 185 bytes RAM, if Serial is not used otherwise
//#define DISABLE_REMOTE_DISPLAY    // Suppress drawing to Bluetooth connected display. Allow only drawing on the locally attached display
#define USE_TIMER_FOR_PERIODIC_LOCAL_TOUCH_CHECKS // Use registerDelayCallback() and changeDelayCallback() for periodic touch checks
#define SUPPORT_LOCAL_LONG_TOUCH_DOWN_DETECTION
#define LOCAL_DISPLAY_GENERATES_BD_EVENTS
#define SUPPORT_LOCAL_DISPLAY       // Supports simultaneously drawing on the locally attached display. Not (yet) implemented for all commands!
#define FONT_8X12                   // Font size used here
#include "LocalSSD1289Display.hpp"  // The implementation of the local display must be included before BlueDisplay.hpp
#define DISPLAY_HEIGHT LOCAL_DISPLAY_HEIGHT // Use local size for remote too
#define DISPLAY_WIDTH  LOCAL_DISPLAY_WIDTH
#include "BlueDisplay.hpp"          // Is used to access the local display too

#include "PageMainMenu.hpp"

#include "stm32f3DiscoveryLedsButtons.h"
#include "stm32f3DiscoPeripherals.h"

extern "C" {
#include "stm32f3_discovery.h"
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include "ff.h"
#include "diskio.h"
#include "usbd_misc.h" // for USBDDeviceHandle
#include "usbd_hid.h"
#include "stm32f3_discovery_accelerometer.h"
#include "stm32f3_discovery_gyroscope.h"
}

#include "TouchDSO.h"
#include "AccuCapacity.h"
#include "GuiDemo.h"

/*
 * buffer for any purpose...
 */
char sStringBuffer[SIZEOF_STRINGBUFFER];

FATFS Fatfs[1];

void SystemClock_Config(void);

// global flag for page control. Is evaluated by calling loop or page and set by buttonBack handler
bool sBackButtonPressed;

void initDisplay(void) {
    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE | BD_FLAG_LONG_TOUCH_ENABLE, 320, 240);
    BlueDisplay1.setCharacterMapping(0x81, 0x03A9); // Omega in UTF16
    BlueDisplay1.setCharacterMapping(0xD6, 0x21B2); // Enter in UTF16
    BlueDisplay1.setCharacterMapping(0xD1, 0x21E7); // Ascending in UTF16 - for printInfo()
    BlueDisplay1.setCharacterMapping(0xD2, 0x21E9); // Descending in UTF16 - for printInfo()
    BlueDisplay1.setCharacterMapping(0xD3, 0x2302); // Home in UTF16
    BlueDisplay1.setCharacterMapping(0xD4, 0x2227); // UP (logical AND) in UTF16
    BlueDisplay1.setCharacterMapping(0xD5, 0x2228); // Down (logical OR) in UTF16
    BlueDisplay1.setCharacterMapping(0xE0, 0x2195); // UP/Down in UTF16
    BlueDisplay1.setCharacterMapping(0xF8, 0x2103); // Degree Celsius in UTF16
}

int main(void) {

    /* SysTick end of count event each ms */
    /* calls HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4)*/
    // sets Prio for Systick Interrupt to 0x0F, which is lowest possible prio
    if (HAL_Init() != HAL_OK) {
        Error_Handler();
    }

    /* Configure the system clock to 72 Mhz */
    SystemClock_Config();

    // sets systic prio to 8
    NVIC_SetPriority(SysTick_IRQn, SYS_TICK_INTERRUPT_PRIO);

    initializeLEDs();
    BSP_LED_On(LED_RED);

    /**
     * User Button
     */
    BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
    NVIC_SetPriority(USER_BUTTON_EXTI_IRQn, 6);

    Misc_IO_initalize(); // Init the Debug pin E7 / B0

    /*
     * Watchdog
     */
    Watchdog_init();

    initalizeTone();
    // signal that we got a watchdog reset and do not enable it any further
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != RESET) {
        /* Clear reset flags */
        __HAL_RCC_CLEAR_RESET_FLAGS();
        tone(1000, 600);
    } else {
        tone(2000, 100);
        Watchdog_start();
    }
    BSP_LED_On(LED_BLUE);

    RTC_InitLSEOn();
    // if LSE is already oscillating everything is fine else try again in 2 seconds
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET) {
        // wait more than 1500 milli seconds for transient oscillation after power up, then try again
        registerDelayCallback(&RTC_initialize, 2000);
    }

#if !defined(DISABLE_REMOTE_DISPLAY)
    // is required for remote display
    UART_BD_initialize(115200);
#endif
    BSP_LED_On(LED_GREEN);

#if defined(SUPPORT_LOCAL_DISPLAY)    // init display early because assertions and timeouts want to be displayed :-)
    // sets isLocalDisplayAvailable
    LocalDisplay.init();
    setDimdelay(BACKLIGHT_DIM_DEFAULT_DELAY_MILLIS);
#endif
    BSP_LED_On(LED_ORANGE_2);

    // initializes SPI and uses SPI_BAUDRATEPRESCALER_16;
    if (BSP_GYRO_Init() != HAL_OK) {
        /* Initialization Error */
        Error_Handler();
    }

    // initializes I2C
    if (BSP_ACCELERO_Init() != HAL_OK) {
        /* Initialization Error */
        Error_Handler();
    }
    BSP_LED_On(LED_RED_2);

    MICROSD_IO_initalize(); // this redefine user button prio (EXTI4) to 15
    // Setup ADC default values
    ADC12_init();
    ADC1_init();
    ADC2_init();

    // USB init must be done in main.cpp since otherwise no device is recognized after boot
    /**
     * sets USB_LP_CAN1_RX0_IRQn Prio 2 of 0-3
     * USBWakeUp_IRQn Prio 1 of 0-3
     * USER_BUTTON_EXTI_IRQn 0 of 0-3
     */
    /* Init USB Device Library */
    USBD_Init(&USBDDeviceHandle, &HID_Desc, 0);
    // needed only once for each handle!
    USBD_RegisterClass(&USBDDeviceHandle, &USBD_HID);

    // sets the 1.5 K pullup if implemented
    USBD_Start(&USBDDeviceHandle);

    BSP_LED_On(LED_BLUE_2);

    registerConnectCallback(&initDisplay);
    // do initialize also on reset
    initDisplay();
    BlueDisplay1.clearDisplay(BACKGROUND_COLOR);
#if defined(SUPPORT_LOCAL_DISPLAY)
    if (isLocalDisplayAvailable) {

        //init touch controller
        TouchPanel.init();

        /**
         * Touch-panel calibration
         */
        TouchPanel.readData();
        // check if panel connected
        if (TouchPanel.getPressure() > MIN_REASONABLE_PRESSURE && TouchPanel.getPressure() < MAX_REASONABLE_PRESSURE) {
            // panel pressed
            TouchPanel.doCalibration(false); //don't check EEPROM for calibration data
        } else {
            TouchPanel.doCalibration(true); //check EEPROM for calibration data
        }
    }
#endif
    BSP_LED_On(LED_GREEN_2);
    if (isLocalDisplayAvailable || USART_isBluetoothPaired()) {

        /*
         * init pages
         */
        initMainMenuPage();
        initGuiDemo();
        initAccelerometerCompassPage();
        // init ADCs late since I have seen timeouts especially for ADC_enableAndWait()
        initDSOPage();
        initAccuCapacity();
        initInfoPage();
        initSystemInfoPage();
        initDACPage();
        initFrequencyGeneratorPage();
        //setZeroAccelerometerGyroValue();

        if (MICROSD_isCardInserted()) {
            // delayed execution of MMC test
            registerDelayCallback(&testAttachMMC, 200);
        }

        struct lconv *tLconfPtr = localeconv();
        //*tLconfPtr->decimal_point = ','; does not work because string is constant and in flash rom
        tLconfPtr->decimal_point = (char*) ",";
        //tLconfPtr->thousands_sep = (char*) "."; // does not affect sprintf()
        BSP_LED_On(LED_ORANGE);

        /*
         * Infinite loop
         */
        while (1) {
            startMainMenuPage();
            allLedsOff();
            while (1) {
                loopMainMenuPage();
            }
            stopMainMenuPage();
        }
    } else {
        delay(1000);
        allLedsOff();
        while (1) {
            // no display attached
            CycleLEDs();
#ifdef HAL_WWDG_MODULE_ENABLED
            Watchdog_reload();
#endif
        }
    }
    return 1;
}

/**
 * @brief  System Clock Configuration
 *         The system Clock is configured as follow :
 *            System Clock source            = PLL (HSE)
 *            SYSCLK(Hz)                     = 72000000
 *            HCLK(Hz)                       = 72000000
 *            AHB Prescaler                  = 1
 *            APB1 Prescaler                 = 2
 *            APB2 Prescaler                 = 1
 *            HSE Frequency(Hz)              = 8000000
 *            HSE PREDIV                     = 1
 *            PLLMUL                         = 9
 *            Flash Latency(WS)              = 2
 * @param  None
 * @retval None
 */
void SystemClock_Config(void) {
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
//    RCC_OscInitTypeDef RCC_OscInitStruct;
//    RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;
//
    /* Enable HSE Oscillator and activate PLL with HSE as source */
//    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
//    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
//    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
//    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
//    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
//    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
//    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
//        Error_Handler();
//    }
    // saves 1712 Bytes
    /*
     * Timeouts are not possible since systick is not running
     */
    // Reset HSEON and HSEBYP bits before configuring the HSE, seems that it is needed to have clear conditions
    __HAL_RCC_HSE_CONFIG(RCC_HSE_OFF);
    /* Wait till HSE is bypassed or disabled */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) != RESET) {
        ;
    }

    __HAL_RCC_HSE_CONFIG(RCC_HSE_ON);
    /* Check the HSE State */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) == RESET) {
        ;
    }

    __HAL_RCC_HSE_PREDIV_CONFIG(RCC_HSE_PREDIV_DIV1);

    /* Configure the main PLL clock source and multiplication factors. */
    __HAL_RCC_PLL_CONFIG(RCC_PLLSOURCE_HSE, RCC_PLL_MUL9);
    /* Enable the main PLL. */
    __HAL_RCC_PLL_ENABLE();

    /* Wait till PLL is ready */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY) == RESET) {
        ;
    }

    /* Configures the USB clock */
//    RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
//    RCC_PeriphClkInit.USBClockSelection = RCC_USBPLLCLK_DIV1_5;
//    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);
    // saves 416 bytes
    /* Configure the USB clock source */
    __HAL_RCC_USB_CONFIG(RCC_USBPLLCLK_DIV1_5);

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }

}

