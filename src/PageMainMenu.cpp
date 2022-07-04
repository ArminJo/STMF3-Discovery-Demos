/*
 * PageMainMenu.cpp
 *
 * @date 17.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 *
 * While exploring the capabilities of the STM32F3 Discovery coupled with a 3.2 HY32D touch screen I build several apps:
 -DSO
 -Frequency synthesizer
 -DAC triangle generator
 -Visualization app for accelerometer, gyroscope and compass data
 -Simple drawing app
 -Screenshot to MicroSd card
 -Color picker
 -Game of life
 -System info and test app

 And a C library for drawing thick lines (extension of Bresenham algorithm)
 */

#include "Pages.h"
#include "FrequencyGeneratorPage.hpp" // include sources for pages

#include "myStrings.h" // for StringHomeChar

#include "TouchButton.h"
#include "AccuCapacity.h"
#include "GuiDemo.h"
#include "stm32f3DiscoveryLedsButtons.h"

extern "C" {
#include "diskio.h"
#include "ff.h"
}

/* Private define ------------------------------------------------------------*/
#define MENU_TOP 15
#define MENU_LEFT 30
#define MAIN_MENU_COLOR COLOR_RED

/* Public variables ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
BDButton TouchButtonDSO;
BDButton TouchButtonBob;
BDButton TouchButtonDemo;
BDButton TouchButtonAccelerometer;
BDButton TouchButtonDraw;
BDButton TouchButtonDatalogger;
BDButton TouchButtonDac;
BDButton TouchButtonFrequencyGenerator;
BDButton TouchButtonTest;
BDButton TouchButtonInfo;
BDButton TouchButtonIR;
BDButton TouchButtonMainSettings;
BDButton TouchButtonMainHome; // for other pages to get back
BDButton *const TouchButtonsMainMenu[] = { &TouchButtonDSO, &TouchButtonMainSettings, &TouchButtonDemo, &TouchButtonIR,
        &TouchButtonAccelerometer, &TouchButtonDraw, &TouchButtonDac, &TouchButtonDatalogger, &TouchButtonBob,
        &TouchButtonFrequencyGenerator, &TouchButtonInfo, &TouchButtonTest, &TouchButtonMainHome }; // TouchButtonMainHome must be last element of array

uint32_t MillisLastLoop;
unsigned int sMillisSinceLastInfoOutput;

void drawMainMenuPage(void);

/* Private functions ---------------------------------------------------------*/
void doMainMenuButtons(BDButton *aTheTouchedButton, int16_t aValue) {
    BDButton::deactivateAllButtons();
    if (aTheTouchedButton->mButtonHandle == TouchButtonDSO.mButtonHandle) {
        startDSOPage();
        while (!sBackButtonPressed) {
            loopDSOPage();
        }
        stopDSOPage();

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonMainSettings.mButtonHandle) {
        // Settings button pressed
        startSettingsPage();
        while (!sBackButtonPressed) {
            loopSettingsPage();
        }
        stopSettingsPage();

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonDac.mButtonHandle) {
        startDACPage();
        while (!sBackButtonPressed) {
            loopDACPage();
        }
        stopDACPage();

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonFrequencyGenerator.mButtonHandle) {
        startFrequencyGeneratorPage();
        while (!sBackButtonPressed) {
            loopFrequencyGeneratorPage();
        }
        stopFrequencyGeneratorPage();

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonInfo.mButtonHandle) {
        startInfoPage();
        while (!sBackButtonPressed) {
            loopInfoPage();
        }
        stopInfoPage();

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonTest.mButtonHandle) {
        startTestsPage();
        while (!sBackButtonPressed) {
            loopTestsPage();
        }
        stopTestsPage();

#ifndef DEBUG_MINIMAL_VERSION
    } else if (aTheTouchedButton->mButtonHandle == TouchButtonDemo.mButtonHandle) {
        startGuiDemo();
        while (!sBackButtonPressed) {
            loopGuiDemo();
        }
        stopGuiDemo();

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonAccelerometer.mButtonHandle) {
        startAccelerometerCompassPage();
        while (!sBackButtonPressed) {
            loopAccelerometerGyroCompassPage();
        }
        stopAccelerometerCompassPage();

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonDatalogger.mButtonHandle) {
        startAccuCapacity();
        while (!sBackButtonPressed) {
            loopAccuCapacity();
        }
        stopAccuCapacity();

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonDraw.mButtonHandle) {
        startDrawPage();
        while (!sBackButtonPressed) {
            loopDrawPage();
        }
        stopDrawPage();

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonBob.mButtonHandle) {
        SPI1_setPrescaler(SPI_BAUDRATEPRESCALER_8);
        initBobsDemo();
        startBobsDemo();
        while (!sBackButtonPressed) {
            loopBobsDemo();
        }
        stopBobsDemo();

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonIR.mButtonHandle) {
        startIRPage();
        while (!sBackButtonPressed) {
            loopIRPage();
        }
        stopIRPage();
    }
#endif
    // for all cases
    sBackButtonPressed = false;
    // if no page available just show main menu again which in turn re-activates all buttons
    drawMainMenuPage();
}

/**
 * misc tests
 */

//FIL File1, File2; /* File objects */
void doTestButton(BDButton *aTheTouchedButton, int16_t aValue) {
    BDButton::deactivateAllButtons();
    BlueDisplay1.clearDisplay(COLOR_WHITE);
    BlueDisplay1.drawLine(10, 10, 20, 16, COLOR_BLACK);
    BlueDisplay1.drawLine(10, 11, 20, 17, COLOR_BLACK);
    BlueDisplay1.drawLine(9, 11, 19, 17, COLOR_BLACK);

    BlueDisplay1.drawLine(10, 80, 80, 17, COLOR_BLACK);
    BlueDisplay1.drawLine(10, 79, 80, 16, COLOR_BLACK);
    BlueDisplay1.drawLine(11, 80, 81, 17, COLOR_BLACK);

//	 BlueDisplay1.clearDisplay(COLOR_WHITE);
//	BYTE b1 = f_open(&File1, "test.asc", FA_OPEN_EXISTING | FA_READ);
//	snprintf(StringBuffer, sizeof StringBuffer, "Open Result of test.asc: %u\n", b1);
//	BlueDisplay1.drawText(0, 0, StringBuffer, 1, COLOR_RED, COLOR_WHITE);
//
//	b1 = f_open(&File2, "test2.asc", FA_CREATE_ALWAYS | FA_WRITE);
//	snprintf(StringBuffer, sizeof StringBuffer, "Open Result of test2.asc: %u\n", b1);
//	BlueDisplay1.drawText(0, TEXT_SIZE_11_HEIGHT, StringBuffer, 1, COLOR_RED, COLOR_WHITE);
//
//	f_close(&File1);
//	f_close(&File2);

//	FeedbackToneOK();
    TouchButtonMainHome.drawButton();
    return;
}

/**
 * clear display handler
 */
void doClearScreen(BDButton *aTheTouchedButton, int16_t aValue) {
    BlueDisplay1.clearDisplay(aValue);
}

/* Public functions ---------------------------------------------------------*/

void drawMainMenuPage(void) {
    registerRedrawCallback(&drawMainMenuPage);
    registerTouchDownCallback(&simpleTouchDownHandler);
    // Overwrite old values with the one used at most sub pages
    registerTouchMoveCallback(&simpleTouchMoveHandlerForSlider);

    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    // draw all buttons except last
    for (unsigned int i = 0; i < (sizeof(TouchButtonsMainMenu) / sizeof(TouchButtonsMainMenu[0]) - 1); ++i) {
        (*TouchButtonsMainMenu[i]).drawButton();
    }
}

void initMainMenuPage(void) {
}

void startMainMenuPage(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
    /**
     * create and position all buttons initially
     */
    // 1. row
    int tPosY = 0;
    TouchButtonDSO.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, MAIN_MENU_COLOR, "DSO", TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doMainMenuButtons);

    TouchButtonMainSettings.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, MAIN_MENU_COLOR, "Settings",
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doMainMenuButtons);

    TouchButtonDemo.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, MAIN_MENU_COLOR, "Demo",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doMainMenuButtons);

    // not visible on this page
    TouchButtonMainHome.init(BUTTON_WIDTH_5_POS_5, tPosY, BUTTON_WIDTH_5, BUTTON_HEIGHT_4,
    MAIN_MENU_COLOR, StringHomeChar, TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doDefaultBackButton);

    // 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonIR.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, MAIN_MENU_COLOR, "IR", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH,
            0, &doMainMenuButtons);

    TouchButtonAccelerometer.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, MAIN_MENU_COLOR, "Accelero.",
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doMainMenuButtons);

    TouchButtonDraw.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, MAIN_MENU_COLOR, "Draw",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doMainMenuButtons);

    //3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonDac.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, MAIN_MENU_COLOR, "DAC", TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doMainMenuButtons);

    TouchButtonDatalogger.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, MAIN_MENU_COLOR, "AccuCapacity",
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doMainMenuButtons);

    TouchButtonBob.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, MAIN_MENU_COLOR, "Bob",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doMainMenuButtons);

    //4. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonFrequencyGenerator.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, MAIN_MENU_COLOR, "Frequency",
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doMainMenuButtons);

    TouchButtonInfo.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, MAIN_MENU_COLOR, "Info",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doMainMenuButtons);

    TouchButtonTest.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, MAIN_MENU_COLOR, "Test",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doMainMenuButtons);

#pragma GCC diagnostic pop

    drawMainMenuPage();
}

void loopMainMenuPage(void) {
    showRTCTimeEverySecond(0, BUTTON_HEIGHT_4_LINE_4 - TEXT_SIZE_11_DECEND, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
    checkAndHandleEvents();
}

void stopMainMenuPage(void) {
    registerRedrawCallback(NULL);
    registerTouchDownCallback(NULL);
    registerTouchMoveCallback(NULL);
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    // free buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsMainMenu) / sizeof(TouchButtonsMainMenu[0]); ++i) {
        (*TouchButtonsMainMenu[i]).deinit();
    }
#endif
}

