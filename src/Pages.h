/*
 * Pages.h
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

#ifndef PAGES_H_
#define PAGES_H_

//#define DEBUG_MINIMAL_VERSION // ???since debugging is not possible if Flash is almost fully occupied ???
// Landscape format
#define REMOTE_DISPLAY_HEIGHT   240
#define REMOTE_DISPLAY_WIDTH    320

#include "BlueDisplay.h"
#include "main.h" // for StringBuffer
#include "timing.h"
#include "utils.h" // for showRTCTime

#if defined(SUPPORT_LOCAL_DISPLAY)
#include "LocalDisplay/LocalDisplayInterface.h"
#include "LocalDisplay/ADS7846.h"
#include "LocalGUI/LocalTouchSlider.h"
#include "LocalGUI/LocalTouchButton.h"
#include "LocalGUI/LocalTouchButtonAutorepeat.h"
#include "LocalGUI/LocalTinyPrint.h"
#include "Chart.h"
#endif

#include "stm32fx0xPeripherals.h"

#include <stdio.h> // for sprintf
#include <stdlib.h> // for abs()

/*
 * GUI used by multiple pages
 */
extern BDSlider TouchSliderBacklight;
extern BDButton TouchButtonBack;
// global flag for page control. Is evaluated by calling loop or page and set by buttonBack handler
extern bool sBackButtonPressed;
extern BDButton TouchButtonTPCalibration;
extern BDButton TouchButtonDraw; // Used by main menu and Demo


/**
 * From TouchDSOGui page
 */
void initDSOPage(void);
void startDSOPage(void);
void loopDSOPage(void);
void stopDSOPage(void);

void doDefaultBackButton(BDButton *aTheTouchedButton, int16_t aValue); // Default handler for TouchButtonMainHome

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
#ifdef __cplusplus
#if defined(SUPPORT_LOCAL_DISPLAY)
extern BDButton TouchButtonMainHome;

extern LocalTouchButtonAutorepeat TouchButtonAutorepeatPlus;
extern LocalTouchButtonAutorepeat TouchButtonAutorepeatMinus;

extern LocalTouchSlider TouchSliderVertical2;
extern LocalTouchSlider TouchSliderHorizontal2;

void doMainMenuHomeButton(BDButton *aTheTouchedButton, int16_t aValue);
void doClearScreen(BDButton *aTheTouchedButton, int16_t aValue);
void printTPData(void);
#endif // SUPPORT_LOCAL_DISPLAY

// for loop timings
extern uint32_t sMillisOfLastLoop;
extern unsigned int sMillisSinceLastInfoOutput;

/**
 * From MainMenuPage
 */
void initMainMenuPage(void);
void startMainMenuPage(void);
void loopMainMenuPage(void);
void stopMainMenuPage(void);
void backToMainMenu(void);

#define COLOR_PAGE_INFO COLOR16_RED

#if defined(SUPPORT_LOCAL_DISPLAY)
/**
 * from SettingsPage
 */
#define BACKLIGHT_CONTROL_X 30
#define BACKLIGHT_CONTROL_Y 4

void createBacklightGUI(void);
void deinitBacklightElements(void);
void drawBacklightElements(void);
void doBacklightSlider(BDSlider *aTheTouchedSlider, uint16_t aBrightness);
#endif // SUPPORT_LOCAL_DISPLAY

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
