/*
 * PageSettins.cpp
 *
 * @date 16.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */

#include "Pages.h"

#ifdef LOCAL_DISPLAY_EXISTS
#include "ADS7846.h"
#  if defined(USE_HY32D)
#include "SSD1289.h"
#  else
#include "MI0283QT2.h"
#  endif
#endif

#include "tinyPrint.h"

#include <string.h>

extern "C" {
//#include "usb_misc.h"
//#include "usb_pwr.h"
#include "stm32f30x_it.h"
//#include "stm32f30x_rtc.h"
}

const char StringUSBPrint[] = "USB Print";

// date strings
const char StringClock[] = "clock";
const char StringYear[] = "year";
const char StringMonth[] = "month";
const char StringDay[] = "day";
const char StringHour[] = "hour";
const char StringMinute[] = "minute";
const char String_min[] = "min";
const char StringSecond[] = "second";
const char * const DateStrings[] = { StringClock, StringSecond, StringMinute, StringHour, StringDay, StringMonth,
        StringYear };

/* Public variables ---------------------------------------------------------*/
unsigned long LastMillis = 0;
unsigned long LoopMillis = 0;

/* Private define ------------------------------------------------------------*/

#define MENU_TOP 15
#define MENU_LEFT 30
#define COLOR_BACKGROUND_FREQ COLOR_WHITE

#define PRINT_MODE_USB_DISABLED 0 //false
#define PRINT_MODE_USB_ENABLED 1 //true
static uint8_t sPrintMode = PRINT_MODE_USB_DISABLED;

#define SET_DATE_STRING_INDEX 4 // index of variable part in StringSetDateCaption
/* Private variables ---------------------------------------------------------*/
char StringSetDateCaption[] = "Set clock  "; // spaces needed for string "minute"

#ifdef LOCAL_DISPLAY_EXISTS
static BDSlider TouchSliderBacklight;
BDButton TouchButtonTPCalibration;
void doTPCalibration(BDButton * aTheTouchedButton, int16_t aValue);
BDButton TouchButtonAutorepeatBacklight_Plus;
BDButton TouchButtonAutorepeatBacklight_Minus;
#endif
BDButton TouchButtonTogglePrintMode;
BDButton TouchButtonToggleTouchXYDisplay;
BDButton TouchButtonSetDate;

// for misc testing purposes

void doTogglePrintEnable(BDButton * aTheTouchedButton, int16_t aValue);
void doToggleTouchXYDisplay(BDButton * aTheTouchedButton, int16_t aValue);

BDButton TouchButtonAutorepeatDate_Plus;
BDButton TouchButtonAutorepeatDate_Minus;

char StringTPCal[] = "TP-Calibr.";

/*********************************************
 * Print stuff
 *********************************************/

/**
 * @param aTheTouchedButton
 * @param aValue 0,1,2=USB Mode
 */
void doTogglePrintEnable(BDButton * aTheTouchedButton, int16_t aValue) {
    sPrintMode = !sPrintMode;
    if (sPrintMode) {
        //USB_ChangeToCDC();
    } else {
        //USB_PowerOff();
    }
    TouchButtonTogglePrintMode.setValueAndDraw(sPrintMode);
}

void drawSettingsPage(void) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    drawClockSettingElements();
#ifdef LOCAL_DISPLAY_EXISTS
    drawBacklightElements();
    TouchButtonTPCalibration.drawButton();
#endif
    TouchButtonTogglePrintMode.drawButton();
    TouchButtonToggleTouchXYDisplay.drawButton();
}

void startSettingsPage(void) {
#ifdef LOCAL_DISPLAY_EXISTS
    initBacklightElements();
#endif
    initClockSettingElements();

    //1. row
    int tPosY = 0;
#ifdef LOCAL_DISPLAY_EXISTS
    TouchButtonToggleTouchXYDisplay.init(BUTTON_WIDTH_3_POS_2, tPosY,
    BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Touch X Y", TEXT_SIZE_11,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, getDisplayXYValuesFlag(),
            &doToggleTouchXYDisplay);
#endif
    //2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonTogglePrintMode.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, 0, StringUSBPrint, TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN,
            0, &doTogglePrintEnable);

#ifdef LOCAL_DISPLAY_EXISTS
    TouchButtonTPCalibration.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4,
    COLOR_RED, StringTPCal, TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTPCalibration);
#endif

    ADC_setRawToVoltFactor();
    registerRedrawCallback(&drawSettingsPage);
    drawSettingsPage();
}

void loopSettingsPage(void) {
    showRTCTimeEverySecond(2, BlueDisplay1.getDisplayHeight() - TEXT_SIZE_11_DECEND - 1, COLOR_RED,
    COLOR_BACKGROUND_DEFAULT);
    checkAndHandleEvents();
}

void stopSettingsPage(void) {
#ifdef LOCAL_DISPLAY_EXISTS
    deinitBacklightElements();
#endif
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
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
void doToggleTouchXYDisplay(BDButton * aTheTouchedButton, int16_t aValue) {
    setDisplayXYValuesFlag(!aValue);
    aTheTouchedButton->setValueAndDraw(!aValue);
}

/*************************************************************
 * RTC and clock setting stuff
 *************************************************************/
uint8_t sSetDateMode;
void doSetDateMode(BDButton * aTheTouchedButton, int16_t aValue);
void doSetDate(BDButton * aTheTouchedButton, int16_t aValue);

void initClockSettingElements(void) {
    strncpy(&StringSetDateCaption[SET_DATE_STRING_INDEX], DateStrings[0], sizeof StringSecond);

    TouchButtonSetDate.init(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_3,
    BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_RED, StringSetDateCaption, TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 1, &doSetDateMode);
    // for RTC setting
    TouchButtonAutorepeatDate_Plus.init(BUTTON_WIDTH_6_POS_4, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_6,
    BUTTON_HEIGHT_5, COLOR_RED, "+", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, 1,
            &doSetDate);

    TouchButtonAutorepeatDate_Minus.init(BUTTON_WIDTH_6_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_6,
    BUTTON_HEIGHT_5, COLOR_RED, "-", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, -1,
            &doSetDate);

    TouchButtonAutorepeatDate_Plus.setButtonAutorepeatTiming(500, 300, 5, 100);
    TouchButtonAutorepeatDate_Minus.setButtonAutorepeatTiming(500, 300, 5, 100);
    sSetDateMode = 0;
}

void deinitClockSettingElements(void) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    TouchButtonSetDate.deinit();
    TouchButtonAutorepeatDate_Plus.deinit();
    TouchButtonAutorepeatDate_Minus.deinit();
#endif
}

void drawClockSettingElements(void) {
    TouchButtonSetDate.drawButton();
    sSetDateMode = 0;
}

void doSetDateMode(BDButton * aTheTouchedButton, int16_t aValue) {
    sSetDateMode++;
    if (sSetDateMode == 1) {
        TouchButtonAutorepeatDate_Plus.drawButton();
        TouchButtonAutorepeatDate_Minus.drawButton();
    } else if (sSetDateMode == sizeof DateStrings / sizeof DateStrings[0]) {
        TouchButtonAutorepeatDate_Plus.removeButton(COLOR_BACKGROUND_DEFAULT);
        TouchButtonAutorepeatDate_Minus.removeButton(COLOR_BACKGROUND_DEFAULT);
        sSetDateMode = 0;
    }

    strncpy(&StringSetDateCaption[SET_DATE_STRING_INDEX], DateStrings[sSetDateMode], sizeof StringSecond);
    aTheTouchedButton->setCaption(StringSetDateCaption);
    aTheTouchedButton->drawButton();
}

uint8_t sDateTimeMaxValues[] = { 59, 59, 23, 31, 12, 99 };
void checkAndSetDateTimeElement(uint8_t * aElementPointer, uint8_t SetDateMode, int16_t aValue) {
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

void doSetDate(BDButton * aTheTouchedButton, int16_t aValue) {
    assertParamMessage((sSetDateMode != 0), sSetDateMode, "Impossible mode");
    HAL_PWR_EnableBkUpAccess();
    uint8_t * tElementPointer;
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
    showRTCTime(2, BlueDisplay1.getDisplayHeight() - TEXT_SIZE_11_DECEND - 1, COLOR_RED, COLOR_BACKGROUND_DEFAULT,
    true);
}

#ifdef LOCAL_DISPLAY_EXISTS
void doTPCalibration(BDButton * aTheTouchedButton, int16_t aValue) {
    //Calibration Button pressed -> calibrate touch panel
    TouchPanel.doCalibration(false);
    drawSettingsPage();
}
/*************************************************************
 * Backlight stuff
 *************************************************************/
void doChangeBacklight(BDButton * aTheTouchedButton, int16_t aValue);

/**
 * create backlight slider and autorepeat buttons
 */
void initBacklightElements(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"

    TouchButtonAutorepeatBacklight_Plus.init(BACKLIGHT_CONTROL_X, BACKLIGHT_CONTROL_Y, BUTTON_WIDTH_10,
    BUTTON_HEIGHT_6, COLOR_RED, "+", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, 1,
            &doChangeBacklight);
    /*
     * Backlight slider
     */
    TouchSliderBacklight.init(BACKLIGHT_CONTROL_X, BACKLIGHT_CONTROL_Y + BUTTON_HEIGHT_6 + 4,
    SLIDER_DEFAULT_BAR_WIDTH, BACKLIGHT_MAX_VALUE, BACKLIGHT_MAX_VALUE, getBacklightValue(), COLOR_BLUE,
    COLOR_GREEN, FLAG_SLIDER_SHOW_BORDER| FLAG_SLIDER_SHOW_VALUE, &doBacklightSlider);
    TouchSliderBacklight.setCaptionProperties(TEXT_SIZE_11, FLAG_SLIDER_CAPTION_ALIGN_MIDDLE, 4, COLOR_RED,
    COLOR_BACKGROUND_DEFAULT);
    TouchSliderBacklight.setCaption("Backlight");
    TouchSliderBacklight.setPrintValueProperties(TEXT_SIZE_11, FLAG_SLIDER_CAPTION_ALIGN_MIDDLE, 4 + TEXT_SIZE_11,
    COLOR_BLUE, COLOR_BACKGROUND_DEFAULT);

    TouchButtonAutorepeatBacklight_Minus.init(BACKLIGHT_CONTROL_X, TouchSliderBacklight.getPositionYBottom() + 30,
    BUTTON_WIDTH_10, BUTTON_HEIGHT_6, COLOR_RED, "-", TEXT_SIZE_11,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, -1, &doChangeBacklight);

    TouchButtonAutorepeatBacklight_Plus.setButtonAutorepeatTiming(600, 100, 10, 20);
    TouchButtonAutorepeatBacklight_Minus.setButtonAutorepeatTiming(600, 100, 10, 20);

#pragma GCC diagnostic pop
}

void deinitBacklightElements(void) {
    TouchButtonAutorepeatBacklight_Plus.deinit();
    TouchButtonAutorepeatBacklight_Minus.deinit();
    TouchSliderBacklight.deinit();
}

void drawBacklightElements(void) {
    TouchButtonAutorepeatBacklight_Plus.drawButton();
    TouchSliderBacklight.drawSlider();
    TouchButtonAutorepeatBacklight_Minus.drawButton();
}

void doBacklightSlider(BDSlider * aTheTouchedSlider, uint16_t aValue) {
    setBacklightValue(aValue);
}

void doChangeBacklight(BDButton * aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    setBacklightValue(getBacklightValue() + aValue);
    TouchSliderBacklight.setValueAndDrawBar(getBacklightValue());
}
#endif

