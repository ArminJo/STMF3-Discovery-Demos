/*
 * PageTests.hpp
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

#ifndef _PAGE_TESTS_HPP
#define _PAGE_TESTS_HPP

#include <stm32f3DiscoveryLedsButtons.h> // for allLedsOff()


/* Private variables ---------------------------------------------------------*/
BDButton TouchButtonTestGraphics;

BDButton TouchButtonTestMisc;
BDButton TouchButtonTestReset;

BDButton TouchButtonTestExceptions;
// to call test functions
BDButton TouchButtonTestFunction1;
BDButton TouchButtonTestFunction2;
BDButton TouchButtonTestFunction3;
BDButton TouchButtonTestMandelbrot;

BDButton TouchButtonSPIPrescalerPlus;
BDButton TouchButtonSPIPrescalerMinus;

BDButton TouchButtonAutorepeatBaudPlus;
BDButton TouchButtonAutorepeatBaudMinus;

BDButton* const TouchButtonsTestPage[] =
        { &TouchButtonTestMandelbrot, &TouchButtonTestExceptions, &TouchButtonTestMisc, &TouchButtonTestReset,
                &TouchButtonTestGraphics, &TouchButtonTestFunction1, &TouchButtonTestFunction2,
                &TouchButtonTestFunction3, &TouchButtonAutorepeatBaudPlus, &TouchButtonAutorepeatBaudMinus,
                &TouchButtonSPIPrescalerPlus, &TouchButtonSPIPrescalerMinus, &TouchButtonBack };

/* Private function prototypes -----------------------------------------------*/

void drawTestsPage(void);

/* Private functions ---------------------------------------------------------*/
void Mandel(uint16_t size_x, uint16_t size_y, uint16_t offset_x, uint16_t offset_y, uint16_t zoom) {
    float tmp1, tmp2;
    float num_real, num_img;
    float radius;
    uint8_t i;
    uint16_t x, y;
    for (y = 0; y < size_y; y++) {
        for (x = 0; x < size_x; x++) {
            num_real = y - offset_y;
            num_real = num_real / zoom;
            num_img = x - offset_x;
            num_img = num_img / zoom;
            i = 0;
            radius = 0;
            while ((i < 256 - 1) && (radius < 4)) {
                tmp1 = num_real * num_real;
                tmp2 = num_img * num_img;
                num_img = 2 * num_real * num_img + 0.6;
                num_real = tmp1 - tmp2 - 0.4;
                radius = tmp1 + tmp2;
                i++;
            }
            BlueDisplay1.drawPixel(x, y, COLOR16(i * 8, i * 18, i * 13));
        }
#ifdef HAL_WWDG_MODULE_ENABLED
        Watchdog_reload();
#endif
    }
}

/*********************************************
 * Test stuff
 *********************************************/
static int testValueIntern = 0;
int testValueForExtern = 0x0000;
#define MAX_TEST_VALUE 7
void drawTestvalue(void) {
    snprintf(sStringBuffer, sizeof sStringBuffer, "MMC SPI_BRPrescaler=%3d", 0x01 << (testValueIntern + 1));
    BlueDisplay1.drawText(0, BUTTON_HEIGHT_4_LINE_4 - TEXT_SIZE_11_DECEND, sStringBuffer, TEXT_SIZE_11, COLOR16_BLUE,
    COLOR16_WHITE);
}

void doSetTestvalue(BDButton * aTheTouchedButton, int16_t aValue) {
    bool tIsError = false;
    int tTestValue = testValueIntern + aValue;
    if (tTestValue < 0) {
        tIsError = true;
        tTestValue = 0;
    } else if (tTestValue > MAX_TEST_VALUE) {
        tIsError = true;
        tTestValue = MAX_TEST_VALUE;
    }
    BDButton::playFeedbackTone(tIsError);
    testValueIntern = tTestValue;
    testValueForExtern = testValueIntern << 3;
    drawTestvalue();
}

void drawBaudrate(uint32_t aBaudRate) {
    snprintf(sStringBuffer, sizeof sStringBuffer, "BR=%6lu", aBaudRate);
    BlueDisplay1.drawText(220, BUTTON_HEIGHT_4_LINE_4 - TEXT_SIZE_11_DECEND, sStringBuffer, TEXT_SIZE_11, COLOR16_BLUE,
    COLOR16_WHITE);
}

#if !defined(DISABLE_REMOTE_DISPLAY)
void doChangeBaudrate(BDButton * aTheTouchedButton, int16_t aValue) {
    uint32_t tBaudRate = getUSART_BD_BaudRate();
    uint32_t tBaudRateDelta = tBaudRate / 256;
    if (aValue < 0) {
        // decrease baud rate
        tBaudRate -= tBaudRateDelta;
    } else {
        // increase baud rate
        tBaudRate += tBaudRateDelta;
    }
    setUART_BD_BaudRate(tBaudRate);
    drawBaudrate(tBaudRate);
}
#endif

void showPrintfExamples() {
    BlueDisplay1.setWriteStringPosition(0, BUTTON_HEIGHT_4_LINE_2);
    float tTest = 1.23456789;
    printf("5.2f=%5.2f 0.3f=%0.3f 4f=%4f\n", tTest, tTest, tTest);
    tTest = -1.23456789;
    printf("5.2f=%5.2f 0.3f=%0.3f 4f=%4f\n", tTest, tTest, tTest);
    tTest = 12345.6789;
    printf("7.0f=%7.0f 7f=%7f\n", tTest, tTest);
    tTest = 0.0123456789;
    printf("3.2f=%3.2f 0.4f=%0.4f 4f=%4f\n", tTest, tTest, tTest);
    uint16_t t16 = 0x02CF;
    printf("#x=%#x X=%X 5x=%5x 0#8X=%0#8X\n", t16, t16, t16, t16);
    printf("p=%p\n", &t16);
    displayTimings( BUTTON_HEIGHT_4_LINE_2 + 6 * TEXT_SIZE_11_HEIGHT);
}

void doTestButtons(BDButton * aTheTouchedButton, int16_t aValue) {
    // Function which does not need a new screen
    if (aTheTouchedButton->mButtonHandle == TouchButtonTestFunction1.mButtonHandle) {
#if !defined(DISABLE_REMOTE_DISPLAY)
        setUART_BD_BaudRate(230400);
        drawBaudrate(230400);
#endif
        return;
    } else if (aTheTouchedButton->mButtonHandle == TouchButtonTestFunction2.mButtonHandle) {
        allLedsOff(); // GREEN RIGHT
        return;
    }

    BlueDisplay1.clearDisplay(BACKGROUND_COLOR);
    BDButton::deactivateAll();
    TouchButtonBack.drawButton();
    sBackButtonPressed = false;

// Exceptions
    if (aTheTouchedButton->mButtonHandle == TouchButtonTestExceptions.mButtonHandle) {
        assertParamMessage((false), 0x12345678, "Error Message");
        delay(1000);
        delay(*((volatile uint32_t *) (0xFFFFFFFF)));
//        snprintf(StringBuffer, sizeof StringBuffer, "%#lx", *((volatile uint32_t *) (0xFFFFFFFF)));

// Miscellaneous tests
    } else if (aTheTouchedButton->mButtonHandle == TouchButtonTestMisc.mButtonHandle) {
        showPrintfExamples();
        do {
            testTimingsLoop(1);
            checkAndHandleEvents();
        } while (!sBackButtonPressed);

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonTestReset.mButtonHandle) {
        reset();

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonTestMandelbrot.mButtonHandle) {
        int i = 1;
        do {
            Mandel(320, 240, 160, 120, i);
            i += 10;
            if (i > 1000) {
                i = 0;
            }
            checkAndHandleEvents();
        } while (!sBackButtonPressed);
    } else if (aTheTouchedButton->mButtonHandle == TouchButtonTestGraphics.mButtonHandle) {
        TouchButtonBack.activate();
        BlueDisplay1.testDisplay();
        TouchButtonBack.drawButton();
        /**
         * Test functions which needs a new screen
         */

    } else if (aTheTouchedButton->mButtonHandle == TouchButtonTestFunction3.mButtonHandle) {
        do {
            //delay(WithCheckAndHandleEvents(1000);
        } while (true); // while (!sBackButtonPressed);
    }
}

/**
 * switch back to tests menu
 */
void doTestsBackButton(BDButton * aTheTouchedButton, int16_t aValue) {
    drawTestsPage();
    sBackButtonPressed = true;
}

void drawTestsPage(void) {
    BlueDisplay1.clearDisplay(BACKGROUND_COLOR);
    // do not draw back button
    for (unsigned int i = 0; i < (sizeof(TouchButtonsTestPage) / sizeof(TouchButtonsTestPage[0])) - 1; ++i) {
        TouchButtonsTestPage[i]->drawButton();
    }
    TouchButtonMainHome.drawButton();
    drawTestvalue();
#if !defined(DISABLE_REMOTE_DISPLAY)
    drawBaudrate(getUSART_BD_BaudRate());
#endif
}

void initTestPage(void) {
}

/**
 * allocate and position all BUTTONS
 */
void startTestsPage(void) {
    printSetPositionColumnLine(0, 0);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"

    int tPosY = 0;
    //1. row
    TouchButtonTestReset.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_GREEN, "Reset",
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonTestMandelbrot.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR16_GREEN, "Mandelbrot",
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonBack.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR16_RED, "Back", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, -1, &doTestsBackButton);

    // 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonTestExceptions.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_GREEN, "Exceptions",
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonTestMisc.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR16_GREEN, "Misc Test", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonTestGraphics.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR16_GREEN, "Graph. Test", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    // 3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonTestFunction1.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_GREEN, "Baud",
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonTestFunction2.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR16_GREEN, "LED reset", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonTestFunction3.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR16_GREEN, "Func 3", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    // 4. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonSPIPrescalerPlus.init(0, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_5, COLOR16_BLUE, "+", TEXT_SIZE_22,
            FLAG_BUTTON_NO_BEEP_ON_TOUCH, 1, &doSetTestvalue);

    TouchButtonSPIPrescalerMinus.init(BUTTON_WIDTH_6_POS_2, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_5, COLOR16_BLUE, "-",
    TEXT_SIZE_22, FLAG_BUTTON_NO_BEEP_ON_TOUCH, -1, &doSetTestvalue);

#if !defined(DISABLE_REMOTE_DISPLAY)
    TouchButtonAutorepeatBaudPlus.init(BUTTON_WIDTH_6_POS_5, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_5, COLOR16_GREEN, "+",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, 1, &doChangeBaudrate);
    TouchButtonAutorepeatBaudPlus.setButtonAutorepeatTiming(600, 100, 10, 20);

    TouchButtonAutorepeatBaudMinus.init(BUTTON_WIDTH_6_POS_6, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_5, COLOR16_GREEN,
            "-",
            TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, -1, &doChangeBaudrate);
    TouchButtonAutorepeatBaudMinus.setButtonAutorepeatTiming(600, 100, 10, 40);
#endif

#pragma GCC diagnostic pop

    registerRedrawCallback(&drawTestsPage);
    drawTestsPage();
}

void loopTestsPage(void) {
    checkAndHandleEvents();
}

/**
 * cleanup on leaving this page
 */
void stopTestsPage(void) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    // free buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsTestPage) / sizeof(TouchButtonsTestPage[0]); ++i) {
        TouchButtonsTestPage[i]->deinit();
    }
#endif
}

#endif // _PAGE_TESTS_HPP
