/*
 * PageTests.cpp
 *
 * @date 16.03.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#if defined(USE_HY32D)
#include "SSD1289.h"
#else
#include "MI0283QT2.h"
#endif
#include "Pages.h"
#include "Chart.h"

#include "tinyPrint.h"
#include "main.h"
#include <string.h>
#include <locale.h>
#include <stdlib.h> // for srand
#include <stm32f3DiscoveryLedsButtons.h> // for allLedsOff()
extern "C" {
#include "stm32f3_discovery.h"
#include "l3gdc20_lsm303dlhc_utils.h"
#include "diskio.h"
#include "ff.h"
#include "BlueSerial.h"
}

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
            BlueDisplay1.drawPixel(x, y, RGB(i * 8, i * 18, i * 13));
        }
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
    BlueDisplay1.drawText(0, BUTTON_HEIGHT_4_LINE_4 - TEXT_SIZE_11_DECEND, sStringBuffer, TEXT_SIZE_11, COLOR_BLUE,
    COLOR_WHITE);
}

void doSetTestvalue(BDButton * aTheTouchedButton, int16_t aValue) {
    int tFeedbackType = FEEDBACK_TONE_OK;
    int tTestValue = testValueIntern + aValue;
    if (tTestValue < 0) {
        tFeedbackType = FEEDBACK_TONE_ERROR;
        tTestValue = 0;
    } else if (tTestValue > MAX_TEST_VALUE) {
        tFeedbackType = FEEDBACK_TONE_ERROR;
        tTestValue = MAX_TEST_VALUE;
    }
    FeedbackTone(tFeedbackType);
    testValueIntern = tTestValue;
    testValueForExtern = testValueIntern << 3;
    drawTestvalue();
}

void drawBaudrate(uint32_t aBaudRate) {
    snprintf(sStringBuffer, sizeof sStringBuffer, "BR=%6lu", aBaudRate);
    BlueDisplay1.drawText(220, BUTTON_HEIGHT_4_LINE_4 - TEXT_SIZE_11_DECEND, sStringBuffer, TEXT_SIZE_11, COLOR_BLUE,
    COLOR_WHITE);
}

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
        setUART_BD_BaudRate(230400);
        drawBaudrate(230400);
        return;
    } else if (aTheTouchedButton->mButtonHandle == TouchButtonTestFunction2.mButtonHandle) {
        allLedsOff(); // GREEN RIGHT
        return;
    }

    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    BDButton::deactivateAllButtons();
    TouchButtonBack.drawButton();
    sBackButtonPressed = false;

// Exceptions
    if (aTheTouchedButton->mButtonHandle == TouchButtonTestExceptions.mButtonHandle) {
        assertParamMessage((false), 0x12345678, "Error Message");
        delayMillis(1000);
        delayMillis(*((volatile uint32_t *) (0xFFFFFFFF)));
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
            //delayMillisWithCheckAndHandleEvents(1000);
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
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    // do not draw back button
    for (unsigned int i = 0; i < (sizeof(TouchButtonsTestPage) / sizeof(TouchButtonsTestPage[0])) - 1; ++i) {
        TouchButtonsTestPage[i]->drawButton();
    }
    TouchButtonMainHome.drawButton();
    drawTestvalue();
    drawBaudrate(getUSART_BD_BaudRate());
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
    TouchButtonTestReset.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_GREEN, "Reset",
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonTestMandelbrot.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_GREEN, "Mandelbrot",
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonBack.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_RED, "Back", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, -1, &doTestsBackButton);

    // 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonTestExceptions.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_GREEN, "Exceptions",
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonTestMisc.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_GREEN, "Misc Test", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonTestGraphics.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_GREEN, "Graph. Test", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    // 3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonTestFunction1.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_GREEN, "Baud",
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonTestFunction2.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_GREEN, "LED reset", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonTestFunction3.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_GREEN, "Func 3", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    // 4. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonSPIPrescalerPlus.init(0, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_5, COLOR_BLUE, "+", TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH, 1, &doSetTestvalue);

    TouchButtonSPIPrescalerMinus.init(BUTTON_WIDTH_6_POS_2, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_5, COLOR_BLUE, "-",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, -1, &doSetTestvalue);

    TouchButtonAutorepeatBaudPlus.init(BUTTON_WIDTH_6_POS_5, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_5, COLOR_GREEN, "+",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, 1, &doChangeBaudrate);
    TouchButtonAutorepeatBaudPlus.setButtonAutorepeatTiming(600, 100, 10, 20);

    TouchButtonAutorepeatBaudMinus.init(BUTTON_WIDTH_6_POS_6, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_5, COLOR_GREEN,
            "-",
            TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, -1, &doChangeBaudrate);
    TouchButtonAutorepeatBaudMinus.setButtonAutorepeatTiming(600, 100, 10, 40);

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
    // free buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsTestPage) / sizeof(TouchButtonsTestPage[0]); ++i) {
        TouchButtonsTestPage[i]->deinit();
    }
}
