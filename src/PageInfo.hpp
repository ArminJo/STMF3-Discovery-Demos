/*
 * PageInfo.cpp
 *
 * for STM32F3_DISCO it consists of multiple pages,
 * for others it consists only of the system info page
 *
 *
 *  Copyright (C) 2013-2023  Armin Joachimsmeyer
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

#ifndef _PAGE_INFO_HPP
#define _PAGE_INFO_HPP

#include <locale.h> // for localeconv

extern "C" {
#include "diskio.h"
#include "usbd_misc.h"
}

/*
 * first the code for system info page
 */
void printSystemInfo() {
    /*
     * first draw GUI
     */
    clearDisplayAndDisableButtonsAndSliders (COLOR16_WHITE);
    TouchButtonBack.drawButton();

    BlueDisplay1.setWriteStringSizeAndColorAndFlag(TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR16_WHITE, true);
    BlueDisplay1.setWriteStringPosition(0, 0);

    /*
     * then print constant values
     */
    extern char *sLowestStackPointer; // from syscalls.c
    extern char estack asm("_estack");
    // Bss, Heap, Ram
    extern char BssStart asm("_sbss");
    extern char BssEnd asm("_ebss");

    extern char HeapStart asm("_sccmram");

    extern char DataStart asm("_sdata");
    extern char DataEnd asm("_edata");

    unsigned int tControlRegisterContent = __get_CONTROL();
    printf("Control=0x%X xPSR=0x%lX\n", tControlRegisterContent, __get_xPSR());

    // DATA
    printf("DATA=0x%X e=0x%X l=%d\n", (uintptr_t) &DataStart, ((uintptr_t) &DataEnd) & 0xFFFF, &DataEnd - &DataStart);

    // BSS
    printf("BSS=0x%X e=0x%X l=%d\n", (uintptr_t) &BssStart, ((uintptr_t) &BssEnd) & 0xFFFF, &BssEnd - &BssStart);

    // HEAP
    char *tHeapEnd = (char*) malloc(16); // + 8 Bytes for each malloc
    printf("HEAP=0x%X e=0x%X l=%d\n", (uintptr_t) &HeapStart, ((uintptr_t) tHeapEnd) & 0xFFFF, (tHeapEnd - &HeapStart));
    free(tHeapEnd);

    // STACK show lowest stackpointer
    if ((char*) __get_MSP() < sLowestStackPointer) {
        sLowestStackPointer = (char*) __get_MSP();
    }
    printf("STACK=%#X low=%#X / %ld\n", (uintptr_t) &estack, ((uintptr_t) sLowestStackPointer) & 0xFFFF,
            (uint32_t) (&estack - sLowestStackPointer));

    // Clocks
    printf("SYSCLK=%ldMHz HCLK=%ldMHz\n", HAL_RCC_GetSysClockFreq() / 1000000, HAL_RCC_GetHCLKFreq() / 1000000);

    // ADC's + RTC
    const char *tRTCClockSource = "HSE";
    if ( __HAL_RCC_GET_RTC_SOURCE() == RCC_RTCCLKSOURCE_LSE) {
        tRTCClockSource = "LSE";
    }

    // TODO use new HAL lib
#ifdef USE_STM32F3_DISCO
    printf("ADC12,ADC34=%ldMHz CLK=%s\n", HAL_RCC_GetHCLKFreq() / 1000000, tRTCClockSource);
#else
    printf("ADC12,ADC34=%ldMHz RTC=%ldHz CLK=%s\n", HAL_RCC_GetHCLKFreq() / 1000000, HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_RTC),
            tRTCClockSource);
#endif

    // PCLK1
#ifdef STM32F30X
    printf("PCLK1=%ldMHz - Tim2-4,I2C1+2,USART2-3\n", HAL_RCC_GetPCLK1Freq() / 1000000);
#else
    printf("PCLK1=%ldMHz I2C1,USART2-4\n", HAL_RCC_GetPCLK1Freq() / 1000000);
#endif

    // PCLK2
    printf("PCLK2=%ldMHz USART1,TIM1+8  %ld NVIC lines\n", HAL_RCC_GetPCLK2Freq() / 1000000,
            ((SCnSCB->ICTR & SCnSCB_ICTR_INTLINESNUM_Msk) + 1) * 32);

    // Priorities
#ifdef STM32F30X
    printf("Prios: ADC=%ld UART3=%ld WWDG=%ld User=%ld\n", NVIC_GetPriority(ADC1_2_IRQn), NVIC_GetPriority(USART3_IRQn),
            NVIC_GetPriority(WWDG_IRQn), NVIC_GetPriority(EXTI0_IRQn));
    printf("SysTic=%ld USB=%ld Touch=%ld DMA1=%ld\n", NVIC_GetPriority(SysTick_IRQn), NVIC_GetPriority(USB_LP_CAN_RX0_IRQn),
            NVIC_GetPriority(EXTI1_IRQn), NVIC_GetPriority(DMA1_Channel1_IRQn));
#else
    printf("Prios: ADC=%ld UART1=%ld WWDG=%ld\n", NVIC_GetPriority(ADC1_2_IRQn), NVIC_GetPriority(USART1_IRQn),
            NVIC_GetPriority(WWDG_IRQn));
    printf("SysTic=%ld USB=%ld DMA1=%ld\n", NVIC_GetPriority(SysTick_IRQn), NVIC_GetPriority(USB_LP_CAN1_RX0_IRQn),
            NVIC_GetPriority(DMA1_Channel1_IRQn));
#endif

    // Compile time
    printf("Compiled at: %s %s\n", __DATE__, __TIME__);

    // Locale
    struct lconv *tLconfPtr = localeconv();
    // decimal point is set to comma by main.c
    printf("Locale=%s decimalpoint=%s thousands=%s\n", setlocale(LC_ALL, NULL), tLconfPtr->decimal_point, tLconfPtr->thousands_sep);

#if defined(SUPPORT_LOCAL_DISPLAY)
    // Lock info
    printf("LOCK count=%d\n", sLockCount);
#endif

    // Debug Info - just in case
    printf("Dbg: 1=%#X, 1=%d 2=%#X\n", DebugValue1, DebugValue1, DebugValue2);
    //        printf("Dbg: 3=%#X 4=%#X 5=%#X", DebugValue3, DebugValue4, DebugValue5);
}

void initSystemInfoPage(void) {
}

/**
 * allocate and position all BUTTONS
 */
void startSystemInfoPage(void) {
    printSystemInfo();
    /*
     * save state
     */
    sLastRedrawCallback = getRedrawCallback();
    registerRedrawCallback(&printSystemInfo);
}

/*
 * print only values that may have changed
 */
void loopSystemInfoPage(void) {
    /*
     * show temperature
     */
    int tTemperatureFiltered = ADC1_getTemperatureMilligrades() / 10;
    int tTemperatureFiltered2 = tTemperatureFiltered;

    int tTemp = ADC1_getTemperatureMilligrades() / 10;
// filter 16 samples
    tTemperatureFiltered = (tTemperatureFiltered * 15 + tTemp + 8) / 16;
// filter 32 samples
    tTemperatureFiltered2 = (tTemperatureFiltered2 * 31 + tTemp + 16) / 32;
    snprintf(sStringBuffer, sizeof sStringBuffer, "%2d.%02d\xB0" "C %2d.%02d %2d.%02d", tTemp / 100, tTemp % 100,
            tTemperatureFiltered / 100, tTemperatureFiltered % 100, tTemperatureFiltered2 / 100, tTemperatureFiltered2 % 100);
    BlueDisplay1.drawText(2, REMOTE_DISPLAY_HEIGHT - 2 * TEXT_SIZE_11_HEIGHT + TEXT_SIZE_11_ASCEND, sStringBuffer, TEXT_SIZE_11,
            COLOR_PAGE_INFO, COLOR16_WHITE);

    ADC_setRawToVoltFactor();
#ifdef STM32F30X
    float tVBatValue = ADC1_getChannelValue(ADC_CHANNEL_VBAT, 4);
    tVBatValue *= 2 * sADCToVoltFactor;
// display VDDA voltage
    snprintf(sStringBuffer, sizeof sStringBuffer, "VCC=%5.3fV VBat=%5.3fV 3V=%#X|%d", sVDDA, tVBatValue, sReading3Volt,
            sReading3Volt);
#else
    // display VDDA voltage
    snprintf(sStringBuffer, sizeof sStringBuffer, "VCC=%5.3fV 3V=%#X|%d", sVDDA, sReading3Volt, sReading3Volt);
#endif
    BlueDisplay1.drawText(2, REMOTE_DISPLAY_HEIGHT - TEXT_SIZE_11_HEIGHT + TEXT_SIZE_11_ASCEND, sStringBuffer, TEXT_SIZE_11,
            COLOR_PAGE_INFO, COLOR16_WHITE);

    delayMillisWithCheckAndHandleEvents(500L);
}

/**
 * cleanup on leaving this page
 */
void stopSystemInfoPage(void) {
    registerRedrawCallback (sLastRedrawCallback);
}

#if defined(SUPPORT_LOCAL_DISPLAY)
/*
 * Info only needed for local display
 */
BDButton TouchButtonInfoFont1;
BDButton TouchButtonInfoFont2;
BDButton TouchButtonInfoColors;
// Buttons for a single info page
BDButton TouchButtonSettingsGamma1;
BDButton TouchButtonSettingsGamma2;

void pickColorPeriodicCallbackHandler(struct TouchEvent *const aActualPositionPtr);

#endif

/*
 * Info only implemented (yet) for STM32F3_DISCO
 */
#ifdef USE_STM32F3_DISCO
BDButton TouchButtonInfoMMC;
BDButton TouchButtonInfoUSB;
BDButton TouchButtonSystemInfo; // another button for showing systemInfoPage

#define INFO_BUTTONS_NUMBER_TO_DISPLAY 6 // Number of buttons on main info page, without back button
BDButton *const TouchButtonsInfoPage[] = { &TouchButtonInfoFont1, &TouchButtonInfoFont2, &TouchButtonInfoColors,
        &TouchButtonInfoMMC, &TouchButtonSystemInfo, &TouchButtonInfoUSB, &TouchButtonBack, &TouchButtonSettingsGamma1,
        &TouchButtonSettingsGamma2 };

/* Private function prototypes -----------------------------------------------*/

void drawInfoPage(void);

void doInfoButtons(BDButton *aTheTouchedButton, int16_t aValue) {
    BlueDisplay1.clearDisplay(BACKGROUND_COLOR);
    BDButton::deactivateAll();
    TouchButtonBack.drawButton();
    sBackButtonPressed = false;

    if (aTheTouchedButton->mButtonHandle == TouchButtonInfoFont1.mButtonHandle) {
        /*
         * Replace page content with printable ascii chars of current font  and return
         */
        uint16_t tXPos;
        uint16_t tYPos = 6 * TEXT_SIZE_11_HEIGHT;
        // printable ascii chars
        unsigned char tChar = FONT_START;
        for (int i = 6; i != 0; i--) {
            tXPos = 0;
            for (uint8_t j = 16; j != 0; j--) {
                tXPos = BlueDisplay1.drawChar(tXPos, tYPos, tChar, TEXT_SIZE_22, COLOR16_BLACK, COLOR16_YELLOW) + 4;
                tChar++;
            }
            tYPos += TEXT_SIZE_22_HEIGHT + 4;
        }

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonInfoFont2.mButtonHandle) {
        uint16_t tXPos;
        uint16_t tYPos = TEXT_SIZE_22_HEIGHT + 4;
        // special chars
        unsigned char tChar = 0x80;
        for (int i = 8; i != 0; i--) {
            tXPos = 0;
            for (uint8_t j = 16; j != 0; j--) {
                tXPos = BlueDisplay1.drawChar(tXPos, tYPos, tChar, TEXT_SIZE_22, COLOR16_BLACK, COLOR16_YELLOW) + 4;
                tChar++;
            }
            tYPos += TEXT_SIZE_22_HEIGHT + 2;
        }

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonInfoColors.mButtonHandle) {
        /*
         * Display color spectrum (very slow if remote display is connected) and show color of point which is touched
         */
        BlueDisplay1.generateColorSpectrum();
        TouchButtonSettingsGamma1.activate();
        TouchButtonSettingsGamma2.activate();
        registerTouchDownCallback (&pickColorPeriodicCallbackHandler);
        registerTouchMoveCallback(&pickColorPeriodicCallbackHandler);

        /*
         * Show color of point which is touched (by event callback handler) as long as back button is not pressed
         */
        while (!sBackButtonPressed) {
            checkAndHandleEvents();
        }
        // reset handler
        registerTouchDownCallback(NULL);
        registerTouchMoveCallback(NULL);

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonInfoMMC.mButtonHandle) {
        /*
         * MMC / micro SD card info
         */
        int tRes = getCardInfo(sStringBuffer, sizeof(sStringBuffer));
        if (tRes == 0 || tRes > RES_PARERR) {
            BlueDisplay1.drawMLText(0, 10, sStringBuffer, TEXT_SIZE_11, COLOR16_RED, BACKGROUND_COLOR);
        } else {
            testAttachMMC();
        }
        if (tRes > RES_PARERR) {
            tRes = getFSInfo(sStringBuffer, sizeof(sStringBuffer));
            if (tRes == 0 || tRes > FR_INVALID_PARAMETER) {
                BlueDisplay1.drawMLText(0, 20 + 4 * TEXT_SIZE_11_HEIGHT, sStringBuffer, TEXT_SIZE_11, COLOR16_RED,
                        BACKGROUND_COLOR);
            }
        }
        // was overwritten by MLText
        TouchButtonBack.drawButton();

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonInfoUSB.mButtonHandle) {
        /*
         * USB info
         */
        getUSB_StaticInfos(sStringBuffer, sizeof sStringBuffer);
        // 3 lines
        BlueDisplay1.drawMLText(10, BUTTON_HEIGHT_4_LINE_2 + TEXT_SIZE_11_ASCEND, sStringBuffer, TEXT_SIZE_11, COLOR16_RED,
                BACKGROUND_COLOR);
//        USB_ChangeToCDC();
//        do {
//            uint8_t * tRecData = CDC_Loopback();
//            if (tRecData != NULL) {
//                myPrint((const char*) tRecData, CDC_RX_BUFFER_SIZE);
//            }
//            BlueDisplay1.drawText(10, BUTTON_HEIGHT_4_LINE_2 - TEXT_SIZE_11_DECEND, getUSBDeviceState(), TEXT_SIZE_11, COLOR16_RED,
//            BACKGROUND_COLOR);
//            delay(WithCheckAndHandleEvents(500);
//        } while (!sBackButtonPressed);

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonSystemInfo.mButtonHandle) {
        /*
         * System info
         */
        startSystemInfoPage();
        /*
         * Refresh info content as long as back button is not pressed
         */
        do {
            loopSystemInfoPage();
        } while (!sBackButtonPressed);
        stopSystemInfoPage();
    }
}

void pickColorPeriodicCallbackHandler(struct TouchEvent *const aActualPositionPtr) {
    uint16_t tColor = LocalDisplay.readPixel(aActualPositionPtr->TouchPosition.PositionX,
            aActualPositionPtr->TouchPosition.PositionY);
    snprintf(sStringBuffer, sizeof sStringBuffer, "X=%3d Y=%3d R=0x%02X G=0x%02X B=0x%02X",
            aActualPositionPtr->TouchPosition.PositionX, aActualPositionPtr->TouchPosition.PositionY, (tColor >> 11) << 3,
            ((tColor >> 5) & 0x3F) << 2, (tColor & 0x1F) << 3);
    BlueDisplay1.drawText(2, BlueDisplay1.getDisplayHeight() - TEXT_SIZE_11_DECEND, sStringBuffer, TEXT_SIZE_11, COLOR16_WHITE,
            COLOR16_BLACK);
// show color
    BlueDisplay1.fillRect(BlueDisplay1.getDisplayWidth() - 30, BlueDisplay1.getDisplayHeight() - 30,
            BlueDisplay1.getDisplayWidth() - 1, BlueDisplay1.getDisplayHeight() - 1, tColor);
}

#if defined(SUPPORT_LOCAL_DISPLAY)
/**
 * for gamma change in color page
 * @param aTheTouchedButton
 * @param aValue
 */
void doSetGamma(BDButton *aTheTouchedButton, int16_t aValue) {
    setGamma(aValue);
}
#endif

/**
 * switch back from sub page to info menu
 */
void doInfoBackButton(BDButton *aTheTouchedButton, int16_t aValue) {
    drawInfoPage();
}

void drawInfoPage(void) {
    BlueDisplay1.clearDisplay(BACKGROUND_COLOR);
    for (unsigned int i = 0; i < INFO_BUTTONS_NUMBER_TO_DISPLAY; ++i) {
        TouchButtonsInfoPage[i]->drawButton();
    }
    TouchButtonMainHome.drawButton();
}

void initInfoPage(void) {
}

/**
 * allocate and position all BUTTONS
 */
void startInfoPage(void) {
//    printSetPositionColumnLine(0, 0);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"

    int tPosY = 0;
//1. row
#if defined(SUPPORT_LOCAL_DISPLAY)
    TouchButtonInfoFont1.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_GREEN, "Font 1",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

    TouchButtonInfoFont2.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR16_GREEN, "Font 2", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);
#endif

    TouchButtonBack.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_RED, "Back", TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH, -1, &doInfoBackButton);

// 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonInfoMMC.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_GREEN, "MMC\nInfos", TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

    TouchButtonSystemInfo.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_GREEN, "System\ninfo",
            TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

    TouchButtonInfoColors.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_GREEN, "Colors", TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

// 3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonInfoUSB.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_GREEN, "USB\nInfos", TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

// 4. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;

#if defined(SUPPORT_LOCAL_DISPLAY)
// button for sub pages
    TouchButtonSettingsGamma1.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_GREEN, "", 0, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doSetGamma);
    TouchButtonSettingsGamma2.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_GREEN, "", 0, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 1, &doSetGamma);
#endif

#pragma GCC diagnostic pop

    drawInfoPage();
    registerRedrawCallback(&drawInfoPage);
}

void loopInfoPage(void) {
    loopSystemInfoPage();
}

/**
 * cleanup on leaving this page
 */
void stopInfoPage(void) {
#  if defined(SUPPORT_LOCAL_DISPLAY)
// free buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsInfoPage) / sizeof(TouchButtonsInfoPage[0]); ++i) {
        TouchButtonsInfoPage[i]->deinit();
    }
#  endif
}

#endif

#endif // _PAGE_INFO_HPP
