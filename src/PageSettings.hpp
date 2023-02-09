/*
 * PageSettins.hpp
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

#ifndef _PAGE_SETTINGS_HPP
#define _PAGE_SETTINGS_HPP

#include <string.h>

extern "C" {
//#include "usb_misc.h"
//#include "usb_pwr.h"
#include "stm32f30x_it.h"
//#include "stm32f30x_rtc.h"
}

// date strings
const char StringClock[] = "clock";
const char StringYear[] = "year";
const char StringMonth[] = "month";
const char StringDay[] = "day";
const char StringHour[] = "hour";
const char StringMinute[] = "minute";
const char String_min[] = "min";
const char StringSecond[] = "second";
const char *const DateStrings[] = { StringClock, StringSecond, StringMinute, StringHour, StringDay, StringMonth, StringYear };

/* Public variables ---------------------------------------------------------*/
unsigned long LastMillis = 0;
unsigned long LoopMillis = 0;

/* Private define ------------------------------------------------------------*/

#define MENU_TOP 15
#define MENU_LEFT 30
#define COLOR_BACKGROUND_FREQ COLOR16_WHITE

#define PRINT_MODE_USB_DISABLED 0 //false
#define PRINT_MODE_USB_ENABLED 1 //true
static uint8_t sPrintMode = PRINT_MODE_USB_DISABLED;

#define SET_DATE_STRING_INDEX 4 // index of variable part in StringSetDateCaption
/* Private variables ---------------------------------------------------------*/
char StringSetDateCaption[] = "Set\nclock  "; // spaces needed for string "minute"

#if defined(SUPPORT_LOCAL_DISPLAY)
void doTPCalibration(BDButton *aTheTouchedButton, int16_t aValue);
#endif

BDButton TouchButtonTogglePrintMode;
BDButton TouchButtonToggleTouchXYDisplay;
BDButton TouchButtonSetDate;

// for misc testing purposes

void doTogglePrintEnable(BDButton *aTheTouchedButton, int16_t aValue);
void doToggleTouchXYDisplay(BDButton *aTheTouchedButton, int16_t aValue);

BDButton TouchButtonAutorepeatDate_Plus;
BDButton TouchButtonAutorepeatDate_Minus;

/*********************************************
 * Print stuff
 *********************************************/

/**
 * @param aTheTouchedButton
 * @param aValue 0,1,2=USB Mode
 */
void doTogglePrintEnable(BDButton *aTheTouchedButton, int16_t aValue) {
    sPrintMode = !sPrintMode;
    if (sPrintMode) {
        //USB_ChangeToCDC();
    } else {
        //USB_PowerOff();
    }
    TouchButtonTogglePrintMode.setValueAndDraw(sPrintMode);
}

void drawSettingsPage(void) {
    BlueDisplay1.clearDisplay(BACKGROUND_COLOR);
    drawClockSettingElements();
#if defined(SUPPORT_LOCAL_DISPLAY)
    drawBacklightElements();
    TouchButtonTPCalibration.drawButton();
#endif
    TouchButtonTogglePrintMode.drawButton();
    TouchButtonToggleTouchXYDisplay.drawButton();
    TouchButtonMainHome.drawButton();
}

void startSettingsPage(void) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    createBacklightGUI();
#endif
    initClockSettingElements();

    //1. row
    int tPosY = 0;
#if defined(SUPPORT_LOCAL_DISPLAY)
    TouchButtonToggleTouchXYDisplay.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Touch\nX Y",
            TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, isDisplayXYValuesEnabled(),
            &doToggleTouchXYDisplay);
#endif
    //2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonTogglePrintMode.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "USB\nPrint", TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, 0, &doTogglePrintEnable);

#if defined(SUPPORT_LOCAL_DISPLAY)
    TouchButtonTPCalibration.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_RED, "TP-Calibr.",
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTPCalibration);
#endif

    ADC_setRawToVoltFactor();
    registerRedrawCallback(&drawSettingsPage);
    drawSettingsPage();
}

void loopSettingsPage(void) {
    showRTCTimeEverySecond(2, BlueDisplay1.getDisplayHeight() - TEXT_SIZE_11_DECEND - 1, COLOR16_RED, BACKGROUND_COLOR);
    checkAndHandleEvents();
}

void stopSettingsPage(void) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    deinitBacklightElements();
#endif
#if defined(SUPPORT_LOCAL_DISPLAY)
    TouchButtonTPCalibration.deinit();
    TouchButtonTogglePrintMode.deinit();
    TouchButtonToggleTouchXYDisplay.deinit();
    deinitClockSettingElements();
#endif
}

/**
 *
 * @param aTheTouchedButton
 * @param aValue assume as boolean here
 */
void doToggleTouchXYDisplay(BDButton *aTheTouchedButton, int16_t aValue) {
    setDisplayXYValuesFlag(aValue);
}

/*************************************************************
 * RTC and clock setting stuff
 *************************************************************/
uint8_t sSetDateMode;
void doSetDateMode(BDButton *aTheTouchedButton, int16_t aValue);
void doSetDate(BDButton *aTheTouchedButton, int16_t aValue);

void initClockSettingElements(void) {
    strncpy(&StringSetDateCaption[SET_DATE_STRING_INDEX], DateStrings[0], sizeof StringSecond);

    TouchButtonSetDate.init(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_3, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR16_RED, StringSetDateCaption, TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 1, &doSetDateMode);
    // for RTC setting
    TouchButtonAutorepeatDate_Plus.init(BUTTON_WIDTH_6_POS_4, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_6,
    BUTTON_HEIGHT_5, COLOR16_RED, "+", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, 1, &doSetDate);

    TouchButtonAutorepeatDate_Minus.init(BUTTON_WIDTH_6_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_6,
    BUTTON_HEIGHT_5, COLOR16_RED, "-", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, -1, &doSetDate);

    TouchButtonAutorepeatDate_Plus.setButtonAutorepeatTiming(500, 300, 5, 100);
    TouchButtonAutorepeatDate_Minus.setButtonAutorepeatTiming(500, 300, 5, 100);
    sSetDateMode = 0;
}

void deinitClockSettingElements(void) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    TouchButtonSetDate.deinit();
    TouchButtonAutorepeatDate_Plus.deinit();
    TouchButtonAutorepeatDate_Minus.deinit();
#endif
}

void drawClockSettingElements(void) {
    TouchButtonSetDate.drawButton();
    sSetDateMode = 0;
}

void doSetDateMode(BDButton *aTheTouchedButton, int16_t aValue) {
    sSetDateMode++;
    if (sSetDateMode == 1) {
        TouchButtonAutorepeatDate_Plus.drawButton();
        TouchButtonAutorepeatDate_Minus.drawButton();
    } else if (sSetDateMode == sizeof DateStrings / sizeof DateStrings[0]) {
        TouchButtonAutorepeatDate_Plus.removeButton(BACKGROUND_COLOR);
        TouchButtonAutorepeatDate_Minus.removeButton(BACKGROUND_COLOR);
        sSetDateMode = 0;
    }

    strncpy(&StringSetDateCaption[SET_DATE_STRING_INDEX], DateStrings[sSetDateMode], sizeof StringSecond);
    aTheTouchedButton->setCaption(StringSetDateCaption);
    aTheTouchedButton->drawButton();
}

uint8_t sDateTimeMaxValues[] = { 59, 59, 23, 31, 12, 99 };
void checkAndSetDateTimeElement(uint8_t *aElementPointer, uint8_t SetDateMode, int16_t aValue) {
    SetDateMode--;
    uint8_t tNewValue = *aElementPointer + aValue;
    uint8_t tMaxValue = sDateTimeMaxValues[SetDateMode];
    if (tNewValue == __UINT8_MAX__) {
        // underflow
        tNewValue = tMaxValue;
    } else if (tNewValue > tMaxValue) {
        //overflow
        tNewValue = 0;
    }
    if ((SetDateMode == 3 || SetDateMode == 4)) {
        if (tNewValue == 0) {
            // no 0th day and month
            if (aValue > 0) {
                // increment
                tNewValue++;
            } else {
                // decrement from 1 to zero -> set to max
                tNewValue = tMaxValue;
            }
        }

    }
    *aElementPointer = tNewValue;
}

void doSetDate(BDButton *aTheTouchedButton, int16_t aValue) {
    assertParamMessage((sSetDateMode != 0), sSetDateMode, "Impossible mode");
    HAL_PWR_EnableBkUpAccess();
    uint8_t *tElementPointer;
    if (sSetDateMode < 4) {
        //set time
        RTC_TimeTypeDef RTC_TimeStructure;
        HAL_RTC_GetTime(&RTCHandle, &RTC_TimeStructure, FORMAT_BIN);
        if (sSetDateMode == 1) {
            tElementPointer = &RTC_TimeStructure.Seconds;
        } else if (sSetDateMode == 2) {
            tElementPointer = &RTC_TimeStructure.Minutes;
        } else {
            tElementPointer = &RTC_TimeStructure.Hours;
        }
        RTC_TimeStructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
        RTC_TimeStructure.StoreOperation = RTC_STOREOPERATION_RESET;
        checkAndSetDateTimeElement(tElementPointer, sSetDateMode, aValue);
        HAL_RTC_SetTime(&RTCHandle, &RTC_TimeStructure, FORMAT_BIN);
    } else {
        RTC_DateTypeDef RTC_DateStructure;
        HAL_RTC_GetDate(&RTCHandle, &RTC_DateStructure, FORMAT_BIN);
        if (sSetDateMode == 4) {
            tElementPointer = &RTC_DateStructure.Date;
        } else if (sSetDateMode == 5) {
            tElementPointer = &RTC_DateStructure.Month;
        } else {
            tElementPointer = &RTC_DateStructure.Year;
        }
        checkAndSetDateTimeElement(tElementPointer, sSetDateMode, aValue);
        HAL_RTC_SetDate(&RTCHandle, &RTC_DateStructure, FORMAT_BIN);
    }
    RTC_PWRDisableBkUpAccess();
    showRTCTime(2, BlueDisplay1.getDisplayHeight() - TEXT_SIZE_11_DECEND - 1, COLOR16_RED, BACKGROUND_COLOR,
    true);
}

#if defined(SUPPORT_LOCAL_DISPLAY)
void doTPCalibration(BDButton *aTheTouchedButton, int16_t aValue) {
    //Calibration Button pressed -> calibrate touch panel
    TouchPanel.doCalibration(false);
    drawSettingsPage();
}

#endif

#endif // _PAGE_SETTINGS_HPP
