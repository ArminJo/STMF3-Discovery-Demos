/**
 * PageFrequencyGenerator.cpp
 *
 * @date 26.10.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */

#include "Pages.h"
#ifndef LOCAL_DISPLAY_EXISTS
#include "TouchDSO.h"
#include "EventHandler.h"
#endif

#include <math.h>   // for pow and log10f

static void (*sOldRedrawCallback)(void);

#define COLOR_BACKGROUND_FREQ COLOR_WHITE

#define NUMBER_OF_FIXED_FREQUENCY_BUTTONS 10
#define NUMBER_OF_FREQUENCY_RANGE_BUTTONS 5

/*
 * Position + size
 */
#define FREQ_SLIDER_SIZE 10 // width of bar / border
#define FREQ_SLIDER_MAX_VALUE (BlueDisplay1.getDisplayWidth() - 20) /* 300 length of bar */
#define FREQ_SLIDER_X 5
#define FREQ_SLIDER_Y (4 * TEXT_SIZE_11_HEIGHT + 4)

/*
 * Direct frequency buttons
 */
const uint16_t Frequency[NUMBER_OF_FIXED_FREQUENCY_BUTTONS] = { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000 };

/*
 * Range buttons
 */
const char* const FrequencyButtonStrings[5] = { "mHz", "Hz", "10Hz", "kHz", "MHz" };
const char FrequencyFactorChars[4] = { 'm', ' ', 'k', 'M' };
#define INDEX_OF_10HZ 2

// Start values for 10 kHz
static float sFrequency = 10;
static uint8_t sFrequencyFactorIndex; //0->mHz, 1->Hz, 2->kHz, 3->MHz
static bool is10HzRange = false;

// factor for mHz/Hz/kHz/MHz - times 100 because of mHz handling
// 1 -> 1 mHz, 1000 -> 1 Hz, 1000000 -> 1 kHz
static uint32_t sFrequencyFactorTimes1000;

/*
 * GUI
 */
#ifndef LOCAL_DISPLAY_EXISTS
BDButton TouchButtonFrequencyPage;
#endif

static BDSlider TouchSliderFrequency;

BDButton TouchButtonFrequencyStartStop;
BDButton TouchButtonGetFrequency;
#ifdef LOCAL_DISPLAY_EXISTS
BDButton TouchButton1;
BDButton TouchButton2;
BDButton TouchButton5;
BDButton TouchButton10;
BDButton TouchButton20;
BDButton TouchButton50;
BDButton TouchButton100;
BDButton TouchButton200;
BDButton TouchButton500;
BDButton TouchButton1k;
BDButton * const TouchButtonFixedFrequency[] = {&TouchButton1, &TouchButton2, &TouchButton5, &TouchButton10,
    &TouchButton20, &TouchButton50, &TouchButton100, &TouchButton200, &TouchButton500, &TouchButton1k};
#else
BDButton TouchButtonFirstFixedFrequency;
#endif

BDButton TouchButtonFrequencyRanges[NUMBER_OF_FREQUENCY_RANGE_BUTTONS];
BDButton ActiveTouchButtonFrequencyRange; // Used to determine which range button is active

void drawFrequencyGeneratorPage(void);

/***********************
 * Code starts here
 ***********************/
void setFrequencyFactor(int aIndexValue) {
    sFrequencyFactorIndex = aIndexValue;
    uint tFactor = 1;
    while (aIndexValue > 0) {
        tFactor *= 1000;
        aIndexValue--;
    }
    sFrequencyFactorTimes1000 = tFactor;
}

/**
 * Computes Autoreload value for synthesizer from 8,381 mHz (0xFFFFFFFF) to 18MHz (0x02) and prints frequency value
 * @param aSetSlider
 * @return true if error happened
 */
bool ComputePeriodAndSetTimer(bool aSetSlider) {
    bool tIsError = false;

    float tPeriod = (36000000000 / sFrequencyFactorTimes1000) / sFrequency;
    uint32_t tPeriodInt = tPeriod;
    if (tPeriodInt < 2) {
        tIsError = true;
        tPeriodInt = 2;
    }

#ifdef STM32F30X
    Synth_Timer32_SetReloadValue(tPeriodInt);
#else
    uint32_t tPrescalerValue = (tPeriodInt >> 16) + 1; // +1 since at least divider by 1
    if (tPrescalerValue > 1) {
        //we have prescaler > 1 -> adjust reload value to be less than 0x10001
        tPeriodInt /= tPrescalerValue;
    }
    Synth_Timer16_SetReloadValue(tPeriodInt, tPrescalerValue);
    tPeriodInt *= tPrescalerValue;
#endif

    // recompute exact frequency for given integer period
    tPeriod = tPeriodInt;
    float tFrequency = (36000000000 / sFrequencyFactorTimes1000) / tPeriod;
    snprintf(StringBuffer, sizeof StringBuffer, "%9.3f%cHz", tFrequency, FrequencyFactorChars[sFrequencyFactorIndex]);
    BlueDisplay1.drawText(FREQ_SLIDER_X + 2 * TEXT_SIZE_22_WIDTH, TEXT_SIZE_22_HEIGHT, StringBuffer, TEXT_SIZE_22,
    COLOR_RED, COLOR_BACKGROUND_FREQ);

    // output period
    tPeriod /= 36;
    char tUnitChar = '\xB5'; // micro
    if (tPeriod > 10000) {
        tPeriod /= 1000;
        tUnitChar = 'm';
    }
    snprintf(StringBuffer, sizeof StringBuffer, "%10.3f%cs", tPeriod, tUnitChar);
    BlueDisplay1.drawText(FREQ_SLIDER_X, TEXT_SIZE_22_HEIGHT + 4 + TEXT_SIZE_22_ASCEND, StringBuffer, TEXT_SIZE_22,
    COLOR_BLUE, COLOR_BACKGROUND_FREQ);

    if (aSetSlider) {
        uint16_t tSliderValue = log10f(sFrequency) * 100;
#ifdef LOCAL_DISPLAY_EXISTS
        TouchSliderFrequency.setActualValueAndDrawBar(tSliderValue);
#else
        TouchSliderFrequency.setActualValueAndDrawBar(tSliderValue);
#endif
    }
    return tIsError;
}

void initFrequencyGenerator(void) {
    // set frequency to 10 kHz
    Synth_Timer_initialize(7200);
}

/*
 * Slider handlers
 */
void doFrequencySlider(BDSlider * aTheTouchedSlider, uint16_t aValue) {
    float tValue = aValue;
    tValue = tValue / (FREQ_SLIDER_MAX_VALUE / 3); // gives 0-3
    // 950 byte program space needed for pow() and log10f()
    sFrequency = pow(10, tValue);
    if (is10HzRange) {
        sFrequency *= 10;
    }
    ComputePeriodAndSetTimer(false);
}

/*
 * Button handlers
 */
/**
 * Set frequency to fixed value 1,2,5,10...,1000
 */
void doSetFixedFrequency(BDButton * aTheTouchedButton, int16_t aValue) {
    sFrequency = aValue;
#ifdef LOCAL_DISPLAY_EXISTS
    FeedbackTone(ComputePeriodAndSetTimer(true));
#else
    BlueDisplay1.playFeedbackTone(ComputePeriodAndSetTimer(true));
#endif
}

/**
 * changes the unit (mHz - MHz)
 * set color for old and new button
 */
void doChangeFrequencyRange(BDButton * aTheTouchedButton, int16_t aValue) {
    if (ActiveTouchButtonFrequencyRange != *aTheTouchedButton) {
        ActiveTouchButtonFrequencyRange.setButtonColorAndDraw( BUTTON_AUTO_RED_GREEN_FALSE_COLOR);
        ActiveTouchButtonFrequencyRange = *aTheTouchedButton;
        aTheTouchedButton->setButtonColorAndDraw( BUTTON_AUTO_RED_GREEN_TRUE_COLOR);
        // Handling of 10 Hz button
        if (aValue == INDEX_OF_10HZ) {
            is10HzRange = true;
        } else {
            is10HzRange = false;
        }
        if (aValue >= INDEX_OF_10HZ) {
            aValue--;
        }

        setFrequencyFactor(aValue);
        ComputePeriodAndSetTimer(true);
    }
}

#ifdef LOCAL_DISPLAY_EXISTS
/**
 * gets frequency value from numberpad
 * @param aTheTouchedButton
 * @param aValue
 */
void doGetFrequency(BDButton * aTheTouchedButton, int16_t aValue) {
    TouchSliderFrequency.deactivate();
    float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, COLOR_BLUE);
    // check for cancel
    if (!isnan(tNumber)) {
        sFrequency = tNumber;
    }
    drawFrequencyGeneratorPage();
    ComputePeriodAndSetTimer(true);
}
#else
/**
 * show gui of settings screen
 */
void doShowFrequencyPage(BDButton * aTheTouchedButton, int16_t aValue) {
    startFrequencyGeneratorPage();
}

/**
 * Handler for number receive event - set frequency to value
 */
void doSetFrequency(float aValue) {
    uint8_t tIndex = 1;
    while (aValue > 1000) {
        aValue /= 1000;
        tIndex++;
    }
    if (aValue < 1) {
        tIndex = 0; //mHz
        aValue *= 1000;
    }
    setFrequencyFactor(tIndex);
    sFrequency = aValue;
    BlueDisplay1.playFeedbackTone(ComputePeriodAndSetTimer(true));
}

/**
 * Request frequency numerical
 */
void doGetFrequency(BDButton * aTheTouchedButton, int16_t aValue) {
    BlueDisplay1.getNumberWithShortPrompt(&doSetFrequency, ("frequency [Hz]"));
}
#endif

void doFrequencyGeneratorStartStop(BDButton * aTheTouchedButton, int16_t aValue) {
    if (aValue) {
        // Stop timer and print red start button
        aTheTouchedButton->setCaption("Start");
        Synth_Timer_Stop();
    } else {
        // Start timer and print green stop button
        aTheTouchedButton->setCaption("Stop");
        Synth_Timer_Start();
        ComputePeriodAndSetTimer(true);
    }
    aTheTouchedButton->setValueAndDraw(!aValue);
}

void createGui() {
#ifndef LOCAL_DISPLAY_EXISTS
    TouchButtonFrequencyPage.init(0, REMOTE_DISPLAY_HEIGHT - (BUTTON_HEIGHT_4 + BUTTON_DEFAULT_SPACING),
    BUTTON_WIDTH_3, BUTTON_HEIGHT_4 + BUTTON_DEFAULT_SPACING, COLOR_RED, ("Frequency"), TEXT_SIZE_11,
            BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doShowFrequencyPage);
#endif

    // Frequency slider for 1 to 1000 at top of screen
    TouchSliderFrequency.init(FREQ_SLIDER_X, FREQ_SLIDER_Y, FREQ_SLIDER_SIZE, FREQ_SLIDER_MAX_VALUE,
    FREQ_SLIDER_MAX_VALUE, 0, COLOR_BLUE, COLOR_GREEN, FLAG_SLIDER_SHOW_BORDER | FLAG_SLIDER_IS_HORIZONTAL, &doFrequencySlider);

    /*
     * Fixed frequency buttons next. Example of button handling without button objects
     */
    uint16_t tXPos = 0;
    for (int i = 0; i < NUMBER_OF_FIXED_FREQUENCY_BUTTONS; ++i) {
        sprintf(StringBuffer, "%u", Frequency[i]);
#ifdef LOCAL_DISPLAY_EXISTS
        TouchButtonFixedFrequency[i]->init(tXPos,
                BlueDisplay1.getDisplayHeight() - BUTTON_HEIGHT_4 - BUTTON_HEIGHT_5 - BUTTON_HEIGHT_6 - 2 * BUTTON_DEFAULT_SPACING,
                BUTTON_WIDTH_10, BUTTON_HEIGHT_6, COLOR_BLUE, StringBuffer, TEXT_SIZE_11, 0, Frequency[i],
                &doSetFixedFrequency);
#else
        TouchButtonFirstFixedFrequency.init(tXPos,
                BlueDisplay1.getDisplayHeight() - BUTTON_HEIGHT_4 - BUTTON_HEIGHT_5 - BUTTON_HEIGHT_6 - 2 * BUTTON_DEFAULT_SPACING,
                BUTTON_WIDTH_10, BUTTON_HEIGHT_6, COLOR_BLUE, StringBuffer, TEXT_SIZE_11, 0, Frequency[i],
                &doSetFixedFrequency);
#endif
        tXPos += BUTTON_WIDTH_10 + BUTTON_DEFAULT_SPACING_QUARTER;
    }
#ifndef LOCAL_DISPLAY_EXISTS
    TouchButtonFirstFixedFrequency.mButtonHandle -= NUMBER_OF_FIXED_FREQUENCY_BUTTONS - 1;
#endif

    // Range next
    tXPos = 0;
    int tSelectedButtonIndex =
            (sFrequencyFactorIndex >= INDEX_OF_10HZ) ? sFrequencyFactorIndex + 1 : sFrequencyFactorIndex;
    int tYPos = BlueDisplay1.getDisplayHeight() - BUTTON_HEIGHT_4 - BUTTON_HEIGHT_5 - BUTTON_DEFAULT_SPACING;
    for (int i = 0; i < NUMBER_OF_FREQUENCY_RANGE_BUTTONS; ++i) {
        uint16_t tButtonColor = BUTTON_AUTO_RED_GREEN_FALSE_COLOR;
        if (i == tSelectedButtonIndex) {
            tButtonColor = BUTTON_AUTO_RED_GREEN_TRUE_COLOR;
        }
        TouchButtonFrequencyRanges[i].init(tXPos, tYPos, BUTTON_WIDTH_5 + BUTTON_DEFAULT_SPACING_HALF,
        BUTTON_HEIGHT_5, tButtonColor, FrequencyButtonStrings[i], TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, i,
                &doChangeFrequencyRange);
        if (i == tSelectedButtonIndex) {
            ActiveTouchButtonFrequencyRange = TouchButtonFrequencyRanges[i];
        }
        tXPos += BUTTON_WIDTH_5 + BUTTON_DEFAULT_SPACING - 2;
    }

    TouchButtonFrequencyStartStop.init(0, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Stop",
    TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH | BUTTON_FLAG_TYPE_AUTO_RED_GREEN, true,
            &doFrequencyGeneratorStartStop);

    TouchButtonGetFrequency.init(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_BLUE, ("Hz..."), TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, true, &doGetFrequency);
}

/**
 * draws slider, text and buttons
 */
void drawFrequencyGeneratorPage(void) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_FREQ);

    BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, FREQ_SLIDER_Y + 3 * FREQ_SLIDER_SIZE + TEXT_SIZE_11_HEIGHT, ("1"),
    TEXT_SIZE_11, COLOR_BLUE, COLOR_BACKGROUND_FREQ);
    BlueDisplay1.drawText(BlueDisplay1.getDisplayWidth() - 5 * TEXT_SIZE_11_WIDTH,
    FREQ_SLIDER_Y + 3 * FREQ_SLIDER_SIZE + TEXT_SIZE_11_HEIGHT, ("1000"), TEXT_SIZE_11, COLOR_BLUE,
    COLOR_BACKGROUND_FREQ);

#ifdef LOCAL_DISPLAY_EXISTS
    TouchButtonMainHome.drawButton();

#else
    BDButton::deactivateAllButtons();
    BDSlider::deactivateAllSliders();
    TouchButtonBackDSO.drawButton();

#endif
    TouchSliderFrequency.drawSlider();

    // fixed frequency buttons
    // we know that the buttons handles are increasing numbers
#ifdef LOCAL_DISPLAY_EXISTS
#else
    BDButton tButton(TouchButtonFirstFixedFrequency);
#endif
    for (unsigned int i = 0; i < NUMBER_OF_FIXED_FREQUENCY_BUTTONS - 1; ++i) {
#ifdef LOCAL_DISPLAY_EXISTS
        // Generate strings each time buttons are drawn since only the pointer to caption is stored in button
        sprintf(StringBuffer, "%u", Frequency[i]);
        TouchButtonFixedFrequency[i]->setCaption(StringBuffer);
        TouchButtonFixedFrequency[i]->drawButton();
#else
        tButton.drawButton();
        tButton.mButtonHandle++;
#endif
    }
    // label last button 1k instead of 1000 which is too long
#ifdef LOCAL_DISPLAY_EXISTS
    TouchButtonFixedFrequency[NUMBER_OF_FIXED_FREQUENCY_BUTTONS-1]->setCaption("1k");
    TouchButtonFixedFrequency[NUMBER_OF_FIXED_FREQUENCY_BUTTONS-1]->drawButton();
#else
    tButton.setCaption("1k");
    tButton.drawButton();
#endif

    for (int i = 0; i < NUMBER_OF_FREQUENCY_RANGE_BUTTONS; ++i) {
        TouchButtonFrequencyRanges[i].drawButton();
    }

    TouchButtonFrequencyStartStop.drawButton();
    TouchButtonGetFrequency.drawButton();

    // show values
    ComputePeriodAndSetTimer(true);
}

void initFrequencyGeneratorPage(void) {
    setFrequencyFactor(2); // for kHz range
#ifndef LOCAL_DISPLAY_EXISTS
    createGui();
#endif
}

void startFrequencyGeneratorPage(void) {
#ifdef LOCAL_DISPLAY_EXISTS
    createGui();
    setFrequencyFactor(sFrequencyFactorIndex);
#endif
    drawFrequencyGeneratorPage();

    sOldRedrawCallback = getRedrawCallback();
    registerRedrawCallback(&drawFrequencyGeneratorPage);
    Synth_Timer_Start();
}

void loopFrequencyGeneratorPage(void) {
    checkAndHandleEvents();
}

void stopFrequencyGeneratorPage(void) {
#ifdef LOCAL_DISPLAY_EXISTS
    // free buttons
    for (unsigned int i = 0; i < NUMBER_OF_FIXED_FREQUENCY_BUTTONS; ++i) {
        TouchButtonFixedFrequency[i]->deinit();
    }

    for (int i = 0; i < NUMBER_OF_FREQUENCY_RANGE_BUTTONS; ++i) {
        TouchButtonFrequencyRanges[i].deinit();
    }
    TouchButtonFrequencyStartStop.deinit();
    TouchButtonGetFrequency.deinit();
    TouchSliderFrequency.deinit();
#else
    TouchSliderFrequency.deactivate();
#endif
    registerRedrawCallback(sOldRedrawCallback);
}

