/*
 * Pages.h
 *
 * @date 16.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#ifndef PAGES_H_
#define PAGES_H_

//#define DEBUG_MINIMAL_VERSION // ???since debugging is not possible if Flash is almost fully occupied ???
// Landscape format
const unsigned int REMOTE_DISPLAY_HEIGHT = 240;
const unsigned int REMOTE_DISPLAY_WIDTH = 320;

#include "BlueDisplay.h"
#include "main.h" // for StringBuffer
#include "timing.h"
#include "utils.h" // for showRTCTime

#ifdef LOCAL_DISPLAY_EXISTS
#include "TouchSlider.h"
#include "TouchButton.h"
#include "TouchButtonAutorepeat.h"
#endif

#include "stm32fx0xPeripherals.h"

#include <stdio.h> // for sprintf

extern BDButton TouchButtonBack;

// global flag for page control. Is evaluated by calling loop or page and set by buttonBack handler
extern bool sBackButtonPressed;

/**
 * From TouchDSOGui page
 */
void initDSOPage(void);
void startDSOPage(void);
void loopDSOPage(void);
void stopDSOPage(void);

// have it here and not at MainMenuPage since it is also used for systems without main menu
void doDefaultBackButton(BDButton * aTheTouchedButton, int16_t aValue);
extern BDButton TouchButtonFrequencyPage;
extern BDButton TouchButtonShowSystemInfo;

/**
 * From FrequencyGenerator page
 */
void initFrequencyGenerator(void);
void initFrequencyGeneratorPage(void);
void drawFrequencyGeneratorPage(void);
void startFrequencyGeneratorPage(void);
void loopFrequencyGeneratorPage(void);
void stopFrequencyGeneratorPage(void);

/**
 * From system info page
 */
void initSystemInfoPage(void);
void startSystemInfoPage(void);
void loopSystemInfoPage(void);
void stopSystemInfoPage(void);

/**
 * from MainMenuPage
 */
#define COLOR_BACKGROUND_DEFAULT COLOR_WHITE

#ifdef __cplusplus
#ifdef LOCAL_DISPLAY_EXISTS
extern BDButton TouchButtonMainHome;

extern TouchButtonAutorepeat TouchButtonAutorepeatPlus;
extern TouchButtonAutorepeat TouchButtonAutorepeatMinus;

extern TouchSlider TouchSliderVertical2;
extern TouchSlider TouchSliderHorizontal2;

void doMainMenuHomeButton(BDButton * aTheTouchedButton, int16_t aValue);
void doClearScreen(BDButton * aTheTouchedButton, int16_t aValue);
#endif

// for loop timings
extern uint32_t MillisLastLoop;
extern unsigned int sMillisSinceLastInfoOutput;

/**
 * From MainMenuPage
 */
void initMainMenuPage(void);
void startMainMenuPage(void);
void loopMainMenuPage(void);
void stopMainMenuPage(void);
void backToMainMenu(void);
void initMainHomeButtonWithPosition(const uint16_t aPositionX, const uint16_t aPositionY, bool doDraw);
void initMainHomeButton(bool doDraw);

#define COLOR_PAGE_INFO COLOR_RED

#ifdef LOCAL_DISPLAY_EXISTS
/**
 * from SettingsPage
 */
#define BACKLIGHT_CONTROL_X 30
#define BACKLIGHT_CONTROL_Y 4

void initBacklightElements(void);
void deinitBacklightElements(void);
void drawBacklightElements(void);
void doBacklightSlider(BDSlider * aTheTouchedSlider, uint16_t aBrightness);
#endif

void initClockSettingElements(void);
void deinitClockSettingElements(void);
void drawClockSettingElements(void);

void startSettingsPage(void);
void loopSettingsPage(void);
void stopSettingsPage(void);

/**
 * from  numberpad page
 */
#include <math.h> // for NAN in numberpad
#define NUMBERPAD_DEFAULT_X 60
float getNumberFromNumberPad(uint16_t aXStart, uint16_t aYStart, uint16_t aButtonColor);

/**
 * From DAC page
 */
void initDACPage(void);
void startDACPage(void);
void loopDACPage(void);
void stopDACPage(void);

/**
 * From IR page
 */
void startIRPage(void);
void loopIRPage(void);
void stopIRPage(void);

/**
 * From PageAccelerometerCompassDemo
 */
void initAccelerometerCompassPage(void);
void startAccelerometerCompassPage(void);
void loopAccelerometerGyroCompassPage(void);
void stopAccelerometerCompassPage(void);

/**
 * From BobsDemo
 */
void initBobsDemo(void);
void startBobsDemo(void);
void loopBobsDemo(void);
void stopBobsDemo(void);

/**
 * From info page
 */
void initInfoPage(void);
void startInfoPage(void);
void loopInfoPage(void);
void stopInfoPage(void);

/**
 * From Tests page
 */
void initTestsPage(void);
void startTestsPage(void);
void loopTestsPage(void);
void stopTestsPage(void);

/**
 * From Draw page
 */
void startDrawPage(void);
void loopDrawPage(void);
void stopDrawPage(void);

#endif /* __cplusplus */
#endif /* PAGES_H_ */
