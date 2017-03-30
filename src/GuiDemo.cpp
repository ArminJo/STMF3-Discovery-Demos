/**
 * @file GuiDemo.cpp
 *
 * @date 31.01.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 *
 *      Demo of the libs:
 *      TouchButton
 *      TouchSlider
 *      Chart
 *
 *      and the "Apps" ;-)
 *      Draw lines
 *      Game of life
 *      Show ADS7846 A/D channels
 *      Display font
 *
 *      For STM32F3 Discovery
 *
 */

#include "Pages.h"
#ifdef LOCAL_DISPLAY_EXISTS
#include "ADS7846.h"
#endif

#include "GameOfLife.h"
#include "Chart.h"

/** @addtogroup Gui_Demo
 * @{
 */

/*
 * LCD and touch panel stuff
 */

#define COLOR_DEMO_BACKGROUND COLOR_WHITE

void createDemoButtonsAndSliders(void);

// Global button handler
void doGuiDemoButtons(BDButton * aTheTochedButton, int16_t aValue);

void doGolSpeed(BDSlider * aTheTochedSlider, uint16_t aSliderValue);

BDButton TouchButtonContinue;

/*
 * Game of life stuff
 */
BDButton TouchButtonDemoSettings;
void showGolSettings(void);

BDButton TouchButtonGolDying;
void doGolDying(BDButton * aTheTouchedButton, int16_t aValue);

BDButton TouchButtonNew;
void doNew(BDButton * aTheTouchedButton, int16_t aValue);
void doContinue(BDButton * aTheTouchedButton, int16_t aValue);
void initNewGameOfLife(void);

static BDSlider TouchSliderGolSpeed;

const char StringGOL[] = "GameOfLife";
// to slow down game of life
unsigned long GolDelay = 0;
bool GolShowDying = true;
bool GolRunning = false;
bool GolInitialized = false;
#define GOL_DIE_MAX 20

/**
 * Menu stuff
 */
void showGuiDemoMenu(void);

BDButton TouchButtonChartDemo;
void showCharts(void);

// space between buttons
#define APPLICATION_MENU 0
#define APPLICATION_SETTINGS 1
#define APPLICATION_DRAW 2
#define APPLICATION_GAME_OF_LIFE 3
#define APPLICATION_CHART 4
#define APPLICATION_ADS7846_CHANNELS 5

int mActualApplication = APPLICATION_MENU;

/*
 * Settings stuff
 */
void showSettings(void);

BDButton TouchButtonGameOfLife;
const char StringTPCalibration[] = "TP-Calibration";

static BDSlider TouchSliderDemo1;
static BDSlider TouchSliderAction;
static BDSlider TouchSliderActionWithoutBorder;
unsigned int ActionSliderValue = 0;
#define ACTION_SLIDER_MAX 100
bool ActionSliderUp = true;

#ifdef LOCAL_DISPLAY_EXISTS
BDButton TouchButtonCalibration;

/*
 * ADS7846 channels stuff
 */
BDButton TouchButtonADS7846Channels;
void ADS7846DisplayChannels(void);
void doADS7846Channels(BDButton * aTheTouchedButton, int16_t aValue);
#endif

void initGuiDemo(void) {
}

BDButton * TouchButtonsGuiDemo[] = { &TouchButtonDemoSettings, &TouchButtonChartDemo, &TouchButtonGameOfLife,
        &TouchButtonBack, &TouchButtonGolDying, &TouchButtonNew, &TouchButtonContinue
#ifdef LOCAL_DISPLAY_EXISTS
        , &TouchButtonCalibration, &TouchButtonADS7846Channels
#endif
        };

void startGuiDemo(void) {
#ifdef LOCAL_DISPLAY_EXISTS
    initBacklightElements();
#endif
    createDemoButtonsAndSliders();
    registerRedrawCallback(&showGuiDemoMenu);

    showGuiDemoMenu();
    frame = new uint8_t[GOL_X_SIZE][GOL_Y_SIZE];
    MillisLastLoop = getMillisSinceBoot();
}

void loopGuiDemo(void) {
    // count milliseconds for loop control
    uint32_t tMillis = getMillisSinceBoot();
    sMillisSinceLastInfoOutput += tMillis - MillisLastLoop;
    MillisLastLoop = tMillis;

    if (sNothingTouched) {
        /*
         * switch for different "apps"
         */
        switch (mActualApplication) {
        case APPLICATION_GAME_OF_LIFE:
            if (GolRunning) {
                // switch back to settings page
                showGolSettings();
                FeedbackToneOK();
            }
            break;
        }
    }

    /*
     * switch for different "apps"
     */
    switch (mActualApplication) {
    case APPLICATION_SETTINGS:
        // Moving slider bar :-)
        if (sMillisSinceLastInfoOutput >= 20) {
            sMillisSinceLastInfoOutput = 0;
            if (ActionSliderUp) {
                ActionSliderValue++;
                if (ActionSliderValue == ACTION_SLIDER_MAX) {
                    ActionSliderUp = false;
                }
            } else {
                ActionSliderValue--;
                if (ActionSliderValue == 0) {
                    ActionSliderUp = true;
                }
            }
            TouchSliderAction.setActualValueAndDrawBar(ActionSliderValue);
            TouchSliderActionWithoutBorder.setActualValueAndDrawBar(ACTION_SLIDER_MAX - ActionSliderValue);
        }
        break;

    case APPLICATION_GAME_OF_LIFE:
        if (GolRunning && sMillisSinceLastInfoOutput >= GolDelay) {
            //game of live "app"
            play_gol();
            drawGenerationText();
            draw_gol();
            sMillisSinceLastInfoOutput = 0;
        }
        break;

    case APPLICATION_MENU:
        break;
    }

    /*
     * Actions independent of touch
     */
#ifdef LOCAL_DISPLAY_EXISTS
    if (mActualApplication == APPLICATION_ADS7846_CHANNELS) {
        ADS7846DisplayChannels();
    }
#endif
    checkAndHandleEvents();
}

void stopGuiDemo(void) {
    delete[] frame;

    // free buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsGuiDemo) / sizeof(TouchButtonsGuiDemo[0]); ++i) {
        TouchButtonsGuiDemo[i]->deinit();
    }
    TouchSliderGolSpeed.deinit();
    TouchSliderActionWithoutBorder.deinit();
    TouchSliderAction.deinit();
#ifdef LOCAL_DISPLAY_EXISTS
    deinitBacklightElements();
#endif
}

/*
 * buttons are positioned relative to sliders and vice versa
 */
void createDemoButtonsAndSliders(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
    /*
     * create and position all BUTTONS initially
     */
    //1. row
    int tPosY = 0;
    TouchButtonChartDemo.init(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR_RED, "Chart",
    TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);

    // Back text button for sub pages
    TouchButtonBack.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_RED, "Back",
    TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);

    // 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonGameOfLife.init(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR_RED, StringGOL,
    TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);

    // 3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonDemoSettings.init(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR_RED, "Settings",
    TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);

#ifdef LOCAL_DISPLAY_EXISTS
    // 4. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonADS7846Channels.init(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR_RED, "ADS7846",
    TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doADS7846Channels);

    // sub pages
    TouchButtonCalibration.init(BUTTON_WIDTH_2_POS_2, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR_RED,
            StringTPCalibration, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);
#endif
    TouchButtonNew.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_RED, "New", TEXT_SIZE_22,
            BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doNew);

    TouchButtonContinue.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_RED, "Continue",
    TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doContinue);

    /*
     * Slider
     */
    TouchSliderGolSpeed.init(70, BUTTON_HEIGHT_4 + BUTTON_DEFAULT_SPACING_HALF, 10, 75, 75, 75, COLOR_BLUE,
    COLOR_GREEN,
            FLAG_SLIDER_SHOW_BORDER | FLAG_SLIDER_IS_HORIZONTAL | TOUCHSLIDER_HORIZONTAL_VALUE_BELOW_TITLE
                    | FLAG_SLIDER_VALUE_BY_CALLBACK, &doGolSpeed);
    TouchSliderGolSpeed.setCaptionProperties(TEXT_SIZE_11, FLAG_SLIDER_CAPTION_ALIGN_MIDDLE, 2, COLOR_RED,
    COLOR_BACKGROUND_DEFAULT);
    TouchSliderGolSpeed.setCaption("Gol-Speed");
    TouchSliderGolSpeed.setPrintValueProperties(TEXT_SIZE_11, FLAG_SLIDER_CAPTION_ALIGN_MIDDLE, 4 + TEXT_SIZE_11,
    COLOR_BLUE, COLOR_BACKGROUND_DEFAULT);

// ON OFF button relative to slider
    TouchButtonGolDying.init(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_3, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
    COLOR_RED, "Show dying", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH | BUTTON_FLAG_TYPE_AUTO_RED_GREEN, false,
            &doGolDying);

// self moving sliders
    TouchSliderActionWithoutBorder.init(180, BUTTON_HEIGHT_4_LINE_2 - 10, 20, ACTION_SLIDER_MAX, ACTION_SLIDER_MAX, 0,
    COLOR_BLUE, COLOR_YELLOW, FLAG_SLIDER_SHOW_VALUE | FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    TouchSliderActionWithoutBorder.setPrintValueProperties(TEXT_SIZE_11, FLAG_SLIDER_CAPTION_ALIGN_MIDDLE, 4,
    COLOR_BLUE, COLOR_BACKGROUND_DEFAULT);

    TouchSliderAction.init(180 + 2 * 20 + BUTTON_DEFAULT_SPACING, BUTTON_HEIGHT_4_LINE_2 - 10, 20, ACTION_SLIDER_MAX,
    ACTION_SLIDER_MAX, 0, COLOR_BLUE, COLOR_YELLOW, FLAG_SLIDER_SHOW_BORDER | FLAG_SLIDER_SHOW_VALUE | FLAG_SLIDER_IS_ONLY_OUTPUT,
    NULL);
    TouchSliderAction.setPrintValueProperties(TEXT_SIZE_11, FLAG_SLIDER_CAPTION_ALIGN_MIDDLE, 4, COLOR_BLUE,
    COLOR_BACKGROUND_DEFAULT);

#pragma GCC diagnostic pop

}

const char StringSlowest[] = "slowest";
const char StringSlow[] = "slow   ";
const char StringNormal[] = "normal ";
const char StringFast[] = "fast   ";

void doGolSpeed(BDSlider * aTheTouchedSlider, uint16_t aSliderValue) {
    aSliderValue = aSliderValue / 25;
    const char * tValueString = "";
    switch (aSliderValue) {
    case 0:
        GolDelay = 8000;
        tValueString = StringSlowest;
        break;
    case 1:
        GolDelay = 2000;
        tValueString = StringSlow;
        break;
    case 2:
        GolDelay = 500;
        tValueString = StringNormal;
        break;
    case 3:
        GolDelay = 0;
        tValueString = StringFast;
        break;
    }
    aTheTouchedSlider->printValue(tValueString);
    aTheTouchedSlider->setActualValueAndDrawBar(aSliderValue * 25);
}

void doGuiDemoButtons(BDButton * aTheTouchedButton, int16_t aValue) {
    BDButton::deactivateAllButtons();
    BDSlider::deactivateAllSliders();
    if (aTheTouchedButton->mButtonHandle == TouchButtonChartDemo.mButtonHandle) {
        showCharts();
        return;
    }

#ifdef LOCAL_DISPLAY_EXISTS

    if (aTheTouchedButton->mButtonHandle == TouchButtonCalibration.mButtonHandle) {
        //Calibration Button pressed -> calibrate touch panel
        TouchPanel.doCalibration(false);
        showSettings();
        return;
    }
#endif

    if (aTheTouchedButton->mButtonHandle == TouchButtonGameOfLife.mButtonHandle) {
        // Game of Life button pressed
        showGolSettings();
        mActualApplication = APPLICATION_GAME_OF_LIFE;
        return;
    }
    if (aTheTouchedButton->mButtonHandle == TouchButtonDemoSettings.mButtonHandle) {
        // Settings button pressed
        showSettings();
        return;
    }
    if (aTheTouchedButton->mButtonHandle == TouchButtonBack.mButtonHandle) {
        // Home button pressed
        showGuiDemoMenu();
        return;
    }
}

void doGolDying(BDButton * aTheTouchedButton, int16_t aValue) {
    GolShowDying = !aValue;
    TouchButtonGolDying.setValueAndDraw(!aValue);
}

void showGolSettings(void) {
    /*
     * Settings button pressed
     */
    GolRunning = false;
    BlueDisplay1.clearDisplay(COLOR_DEMO_BACKGROUND);
    TouchButtonBack.drawButton();
    TouchSliderGolSpeed.drawSlider();
// redefine Buttons
    TouchButtonNew.drawButton();
    TouchButtonContinue.drawButton();
}

void doNew(BDButton * aTheTouchedButton, int16_t aValue) {
// deactivate gui elements
    BDButton::deactivateAllButtons();
    BDSlider::deactivateAllSliders();
    initNewGameOfLife();
    sMillisSinceLastInfoOutput = GolDelay;
    GolRunning = true;
}

void doContinue(BDButton * aTheTouchedButton, int16_t aValue) {
// deactivate gui elements
    BDButton::deactivateAllButtons();
    BDSlider::deactivateAllSliders();
    if (!GolInitialized) {
        initNewGameOfLife();
    } else {
        ClearScreenAndDrawGameOfLifeGrid();
    }
    sMillisSinceLastInfoOutput = GolDelay;
    GolRunning = true;
}

void initNewGameOfLife(void) {
    GolInitialized = true;
    init_gol();
}

void showSettings(void) {
    /*
     * Settings button pressed
     */
    BlueDisplay1.clearDisplay(COLOR_DEMO_BACKGROUND);
    TouchButtonBack.drawButton();
#ifdef LOCAL_DISPLAY_EXISTS
    TouchButtonCalibration.drawButton();
    drawBacklightElements();
#endif
    TouchSliderGolSpeed.drawSlider();
    TouchSliderAction.drawSlider();
    TouchSliderActionWithoutBorder.drawSlider();
    mActualApplication = APPLICATION_SETTINGS;
}

void showGuiDemoMenu(void) {
    TouchButtonBack.deactivate();

    BlueDisplay1.clearDisplay(COLOR_DEMO_BACKGROUND);
    TouchButtonMainHome.drawButton();
    TouchButtonDemoSettings.drawButton();
    TouchButtonChartDemo.drawButton();
#ifdef LOCAL_DISPLAY_EXISTS
    TouchButtonADS7846Channels.drawButton();
#endif
    TouchButtonGameOfLife.drawButton();
    GolInitialized = false;
    mActualApplication = APPLICATION_MENU;
}

void showFont(void) {
    BlueDisplay1.clearDisplay(COLOR_DEMO_BACKGROUND);
    TouchButtonBack.drawButton();

    uint16_t tXPos;
    uint16_t tYPos = 10;
    unsigned char tChar = 0x20;
    for (uint8_t i = 14; i != 0; i--) {
        tXPos = 10;
        for (uint8_t j = 16; j != 0; j--) {
            tXPos = BlueDisplay1.drawChar(tXPos, tYPos, tChar, TEXT_SIZE_11, COLOR_BLACK, COLOR_YELLOW) + 4;
            tChar++;
        }
        tYPos += TEXT_SIZE_11_HEIGHT + 4;
    }
}

/*
 * Show charts features
 */
void showCharts(void) {
    BlueDisplay1.clearDisplay(COLOR_DEMO_BACKGROUND);
    TouchButtonBack.drawButton();
    showChartDemo();
    mActualApplication = APPLICATION_CHART;
}

#ifdef LOCAL_DISPLAY_EXISTS
void doADS7846Channels(BDButton * aTheTouchedButton, int16_t aValue) {
    BDButton::deactivateAllButtons();
    mActualApplication = APPLICATION_ADS7846_CHANNELS;
    BlueDisplay1.clearDisplay(COLOR_DEMO_BACKGROUND);
    uint16_t tPosY = 30;
    // draw text
    for (uint8_t i = 0; i < 8; ++i) {
        BlueDisplay1.drawText(90, tPosY, (char *) ADS7846ChannelStrings[i], TEXT_SIZE_22, COLOR_RED,
        COLOR_DEMO_BACKGROUND);
        tPosY += TEXT_SIZE_22_HEIGHT;
    }
    TouchButtonBack.drawButton();
}

#define BUTTON_CHECK_INTERVAL 20
void ADS7846DisplayChannels(void) {
    static uint8_t aButtonCheckInterval = 0;
    uint16_t tPosY = 30;
    int16_t tTemp;
    bool tUseDiffMode = true;
    bool tUse12BitMode = false;

    for (uint8_t i = 0; i < 8; ++i) {
        if (i == 4) {
            tUseDiffMode = false;
        }
        if (i == 2) {
            tUse12BitMode = true;
        }
        tTemp = TouchPanel.readChannel(ADS7846ChannelMapping[i], tUse12BitMode, tUseDiffMode, 2);
        snprintf(sStringBuffer, sizeof sStringBuffer, "%04u", tTemp);
        BlueDisplay1.drawText(15, tPosY, sStringBuffer, TEXT_SIZE_22, COLOR_RED, COLOR_DEMO_BACKGROUND);
        tPosY += TEXT_SIZE_22_HEIGHT;
    }
    aButtonCheckInterval++;
    if (aButtonCheckInterval >= BUTTON_CHECK_INTERVAL) {
        TouchPanel.rd_data();
        if (TouchPanel.mPressure > MIN_REASONABLE_PRESSURE) {
            // TODO use Callback
            //TouchButtonBack->checkButton(TouchPanel.getXActual(), TouchPanel.getYActual(), true);
        }

    }
}
#endif

/**
 * @}
 */
