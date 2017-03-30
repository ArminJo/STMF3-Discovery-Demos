/*
 *  PageFrequencyGenerator.cpp
 *
 *  Sine frequency is lowered when DSO is running since then not all overflow interrupts can be handled
 *
 *  Created on: 01.01.2015
 *      Author: Armin Joachimsmeyer
 *      Email: armin.joachimsmeyer@gmail.com
 *      License: GPL v3 (http://www.gnu.org/licenses/gpl.html)
 *
 *      Sources: http://code.google.com/p/simple-touch-screen-dso-software/
 *
 *      Features:
 *      Frequency output from 119 mHz (8.388 second) to 8MHz on Arduino
 *
 */

#ifdef AVR
#include "PageFrequencyGenerator.h"
#include "SinePWM.h"
#include "BlueDisplay.h"

#include "SimpleTouchScreenDSO.h"

#include <stdlib.h> // for dtostrf

#else
#include "Pages.h"
#include "main.h" // for StringBuffer
#ifndef LOCAL_DISPLAY_EXISTS
#include "TouchDSO.h"
#endif
#endif

#include <stdio.h>   // for printf
#include <math.h>   // for pow and log10f

static void (*sLastRedrawCallback)(void);

#define COLOR_BACKGROUND_FREQ COLOR_WHITE

#ifdef AVR
#define TIMER_PRESCALER_64 0x03
#define TIMER_PRESCALER_MASK 0x07
#endif

#define NUMBER_OF_FIXED_FREQUENCY_BUTTONS 10
#define NUMBER_OF_FREQUENCY_RANGE_BUTTONS 5

/*
 * Position + size
 */
#define FREQ_SLIDER_SIZE 10 // width of bar / border
#define FREQ_SLIDER_MAX_VALUE 300 // (BlueDisplay1.getDisplayWidth() - 20) = 300 length of bar
#define FREQ_SLIDER_X 5
#define FREQ_SLIDER_Y (4 * TEXT_SIZE_11_HEIGHT + 4)

/*
 * Direct frequency + range buttons
 */
#ifdef AVR
const uint16_t FixedFrequencyButtonCaptions[NUMBER_OF_FIXED_FREQUENCY_BUTTONS] PROGMEM
= { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000 };

// the compiler cannot optimize 2 occurrences of the same PROGMEM string
const char StringmHz[] PROGMEM = "mHz";
const char StringHz[] PROGMEM = "Hz";
const char String10Hz[] PROGMEM = "10Hz";
const char StringkHz[] PROGMEM = "kHz";
const char StringMHz[] PROGMEM = "MHz";

const char* RangeButtonStrings[5] = { StringmHz, StringHz, String10Hz, StringkHz, StringMHz };
#else
const uint16_t FixedFrequencyButtonCaptions[NUMBER_OF_FIXED_FREQUENCY_BUTTONS] = {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000};
const char* const RangeButtonStrings[5] = {"mHz", "Hz", "10Hz", "kHz", "MHz"};
#endif

const char FrequencyFactorChars[4] = { 'm', ' ', 'k', 'M' };
#define INDEX_OF_10HZ 2

// Start values for 2 kHz
static float sFrequency = 2000;
static uint32_t sPeriodInt;

static uint8_t sFrequencyFactorIndex = 1; // 0->mHz, 1->Hz, 2->kHz, 3->MHz

// factor for mHz/Hz/kHz/MHz - times 1000 because of mHz handling
// 1 -> 1 mHz, 1000 -> 1 Hz, 1000000 -> 1 kHz
static uint32_t sFrequencyFactorTimes1000;

static const int BUTTON_INDEX_SELECTED_INITIAL = 2; // select 10Hz Button
static bool is10HzRange = true;

/*
 * GUI
 */
BDButton TouchButtonFrequencyRanges[NUMBER_OF_FREQUENCY_RANGE_BUTTONS];
BDButton ActiveTouchButtonFrequencyRange; // Used to determine which range button is active

BDButton TouchButtonFrequencyStartStop;
BDButton TouchButtonGetFrequency;
BDButton TouchButtonSine;
bool isSineOutput = false;
bool isOutputEnabled = true;

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
BDButton * const TouchButtonFixedFrequency[] = {&TouchButton1, &TouchButton2, &TouchButton5, &TouchButton10, &TouchButton20,
    &TouchButton50, &TouchButton100, &TouchButton200, &TouchButton500, &TouchButton1k};
#else
BDButton TouchButtonFirstFixedFrequency;
#endif

BDSlider TouchSliderFrequency;

void initFrequencyGeneratorPageGui(void);

void doFrequencySlider(BDSlider * aTheTouchedSlider, uint16_t aValue);

void doToggleSineOutput(BDButton * aTheTouchedButton, int16_t aValue);
void doSetFixedFrequency(BDButton * aTheTouchedButton, int16_t aValue);
void doChangeFrequencyRange(BDButton * aTheTouchedButton, int16_t aValue);
void doFrequencyGeneratorStartStop(BDButton * aTheTouchedButton, int16_t aValue);
void doGetFrequency(BDButton * aTheTouchedButton, int16_t aValue);
bool ComputePeriodAndSetTimer(bool aSetSlider);
void setFrequencyFactor(int aIndexValue);
void printFrequencyPeriod(bool aSetSlider);
#ifdef AVR
void initTimer1ForCTC(void);
#else
#endif

/***********************
 * Code starts here
 ***********************/

void initFrequencyGenerator(void) {
#ifdef AVR
    initTimer1ForCTC();
#else
    // set frequency to 2 kHz
    Synth_Timer_initialize(36000);
#endif
}

void initFrequencyGeneratorPage(void) {
#ifndef LOCAL_DISPLAY_EXISTS
    initFrequencyGeneratorPageGui();
#endif
    /*
     * Initialize other variables
     */
    setFrequencyFactor(sFrequencyFactorIndex);
}

void startFrequencyGeneratorPage(void) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_FREQ);

#ifdef LOCAL_DISPLAY_EXISTS
    // do it here to enable freeing of button resources in stopFrequencyGeneratorPage()
    initFrequencyGeneratorPageGui();
#endif

    drawFrequencyGeneratorPage();
    ComputePeriodAndSetTimer(true);
    /*
     * save state
     */
    sLastRedrawCallback = getRedrawCallback();
    registerRedrawCallback(&drawFrequencyGeneratorPage);

#ifndef AVR
    Synth_Timer_Start();
#endif
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
#endif
    /*
     * restore previous state
     */
    registerRedrawCallback(sLastRedrawCallback);
}

void initFrequencyGeneratorPageGui() {
    // Frequency slider for 1 to 1000 at top of screen
    TouchSliderFrequency.init(FREQ_SLIDER_X, FREQ_SLIDER_Y, FREQ_SLIDER_SIZE, FREQ_SLIDER_MAX_VALUE,
    FREQ_SLIDER_MAX_VALUE, 0, COLOR_BLUE, COLOR_GREEN, FLAG_SLIDER_SHOW_BORDER | FLAG_SLIDER_IS_HORIZONTAL, &doFrequencySlider);

    /*
     * Fixed frequency buttons next. Example of button handling without button objects
     */
    uint16_t tXPos = 0;
    uint16_t tFrequency;
#ifdef AVR
    // captions are in PGMSPACE
    const uint16_t * tFrequencyCaptionPtr = &FixedFrequencyButtonCaptions[0];
    for (uint8_t i = 0; i < NUMBER_OF_FIXED_FREQUENCY_BUTTONS; ++i) {
        tFrequency = pgm_read_word(tFrequencyCaptionPtr);
        sprintf_P(sStringBuffer, PSTR("%u"), tFrequency);
#else
        for (uint8_t i = 0; i < NUMBER_OF_FIXED_FREQUENCY_BUTTONS; ++i) {
            tFrequency = FixedFrequencyButtonCaptions[i];
            sprintf(sStringBuffer, "%u", tFrequency);
#endif

#ifdef LOCAL_DISPLAY_EXISTS
        TouchButtonFixedFrequency[i]->init(tXPos,
                REMOTE_DISPLAY_HEIGHT - BUTTON_HEIGHT_4 - BUTTON_HEIGHT_5 - BUTTON_HEIGHT_6 - 2 * BUTTON_DEFAULT_SPACING,
                BUTTON_WIDTH_10, BUTTON_HEIGHT_6, COLOR_BLUE, sStringBuffer, TEXT_SIZE_11, 0, tFrequency, &doSetFixedFrequency);
#else
        TouchButtonFirstFixedFrequency.init(tXPos,
                REMOTE_DISPLAY_HEIGHT - BUTTON_HEIGHT_4 - BUTTON_HEIGHT_5 - BUTTON_HEIGHT_6 - 2 * BUTTON_DEFAULT_SPACING,
                BUTTON_WIDTH_10, BUTTON_HEIGHT_6, COLOR_BLUE, sStringBuffer, TEXT_SIZE_11, 0, tFrequency, &doSetFixedFrequency);
#endif

        tXPos += BUTTON_WIDTH_10 + BUTTON_DEFAULT_SPACING_QUARTER;
#ifdef AVR
        tFrequencyCaptionPtr++;
#endif
    }
#ifndef LOCAL_DISPLAY_EXISTS
    TouchButtonFirstFixedFrequency.mButtonHandle -= NUMBER_OF_FIXED_FREQUENCY_BUTTONS - 1;
#endif

    // Range next
    tXPos = 0;
    int tYPos = REMOTE_DISPLAY_HEIGHT - BUTTON_HEIGHT_4 - BUTTON_HEIGHT_5 - BUTTON_DEFAULT_SPACING;
    for (int i = 0; i < NUMBER_OF_FREQUENCY_RANGE_BUTTONS; ++i) {
        uint16_t tButtonColor = BUTTON_AUTO_RED_GREEN_FALSE_COLOR;
        if (i == BUTTON_INDEX_SELECTED_INITIAL) {
            tButtonColor = BUTTON_AUTO_RED_GREEN_TRUE_COLOR;
        }
#ifdef AVR
        TouchButtonFrequencyRanges[i].initPGM(tXPos, tYPos, BUTTON_WIDTH_5 + BUTTON_DEFAULT_SPACING_HALF,
        BUTTON_HEIGHT_5, tButtonColor, RangeButtonStrings[i], TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, i,
                &doChangeFrequencyRange);
#else
        TouchButtonFrequencyRanges[i].init(tXPos, tYPos, BUTTON_WIDTH_5 + BUTTON_DEFAULT_SPACING_HALF,
                BUTTON_HEIGHT_5, tButtonColor, RangeButtonStrings[i], TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, i,
                &doChangeFrequencyRange);
#endif
        tXPos += BUTTON_WIDTH_5 + BUTTON_DEFAULT_SPACING - 2;
    }

    ActiveTouchButtonFrequencyRange = TouchButtonFrequencyRanges[BUTTON_INDEX_SELECTED_INITIAL];

#ifdef AVR
    TouchButtonFrequencyStartStop.initPGM(0, LAYOUT_256_HEIGHT - BUTTON_HEIGHT_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
    COLOR_GREEN, PSTR("Start"), TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH | BUTTON_FLAG_TYPE_TOGGLE_RED_GREEN, isOutputEnabled,
            &doFrequencyGeneratorStartStop);
    TouchButtonFrequencyStartStop.setCaptionPGMForValueTrue(PSTR("Stop"));

    TouchButtonGetFrequency.initPGM(BUTTON_WIDTH_3_POS_2, LAYOUT_256_HEIGHT - BUTTON_HEIGHT_4, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_BLUE, PSTR("Hz..."), TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, true, &doGetFrequency);

    TouchButtonSine.initPGM(BUTTON_WIDTH_3_POS_3, LAYOUT_256_HEIGHT - BUTTON_HEIGHT_4, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_BLUE, PSTR("Sine"), TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH | BUTTON_FLAG_TYPE_TOGGLE_RED_GREEN,
            isSineOutput, &doToggleSineOutput);
#else
    TouchButtonFrequencyStartStop.init(0, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Start",
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH | BUTTON_FLAG_TYPE_TOGGLE_RED_GREEN, true, &doFrequencyGeneratorStartStop);
    TouchButtonFrequencyStartStop.setCaptionForValueTrue("Stop");

    TouchButtonGetFrequency.init(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
            BUTTON_HEIGHT_4, COLOR_BLUE, ("Hz..."), TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, true, &doGetFrequency);
#endif
}

void drawFrequencyGeneratorPage(void) {
    // do not clear screen here since gui is refreshed periodically while DSO is running
    BDButton::deactivateAllButtons();
    BDSlider::deactivateAllSliders();

    TouchButtonBack.drawButton();

    TouchSliderFrequency.drawSlider();
#ifdef AVR
    BlueDisplay1.drawTextPGM(TEXT_SIZE_11_WIDTH, FREQ_SLIDER_Y + 3 * FREQ_SLIDER_SIZE + TEXT_SIZE_11_HEIGHT, PSTR("1"),
    TEXT_SIZE_11, COLOR_BLUE, COLOR_BACKGROUND_FREQ);
    BlueDisplay1.drawTextPGM(REMOTE_DISPLAY_WIDTH - 5 * TEXT_SIZE_11_WIDTH,
    FREQ_SLIDER_Y + 3 * FREQ_SLIDER_SIZE + TEXT_SIZE_11_HEIGHT, PSTR("1000"), TEXT_SIZE_11, COLOR_BLUE,
    COLOR_BACKGROUND_FREQ);
#else
    BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, FREQ_SLIDER_Y + 3 * FREQ_SLIDER_SIZE + TEXT_SIZE_11_HEIGHT, ("1"),
            TEXT_SIZE_11, COLOR_BLUE, COLOR_BACKGROUND_FREQ);
    BlueDisplay1.drawText(BlueDisplay1.getDisplayWidth() - 5 * TEXT_SIZE_11_WIDTH,
            FREQ_SLIDER_Y + 3 * FREQ_SLIDER_SIZE + TEXT_SIZE_11_HEIGHT, ("1000"), TEXT_SIZE_11, COLOR_BLUE,
            COLOR_BACKGROUND_FREQ);
#endif

    // fixed frequency buttons
    // we know that the buttons handles are increasing numbers
#ifndef LOCAL_DISPLAY_EXISTS
    BDButton tButton(TouchButtonFirstFixedFrequency);
#endif
#ifdef LOCAL_DISPLAY_EXISTS
    for (uint8_t i = 0; i < NUMBER_OF_FIXED_FREQUENCY_BUTTONS - 1; ++i) {
        // Generate strings each time buttons are drawn since only the pointer to caption is stored in button
        sprintf(sStringBuffer, "%u", FixedFrequencyButtonCaptions[i]);
        TouchButtonFixedFrequency[i]->setCaption(sStringBuffer);
        TouchButtonFixedFrequency[i]->drawButton();
    }
    // label last button 1k instead of 1000 which is too long
    TouchButtonFixedFrequency[NUMBER_OF_FIXED_FREQUENCY_BUTTONS - 1]->setCaption("1k");
    TouchButtonFixedFrequency[NUMBER_OF_FIXED_FREQUENCY_BUTTONS - 1]->drawButton();
#else
    for (uint8_t i = 0; i < NUMBER_OF_FIXED_FREQUENCY_BUTTONS; ++i) {
        tButton.drawButton();
        tButton.mButtonHandle++;
    }
#endif

    for (uint8_t i = 0; i < NUMBER_OF_FREQUENCY_RANGE_BUTTONS; ++i) {
        TouchButtonFrequencyRanges[i].drawButton();
    }

    TouchButtonFrequencyStartStop.drawButton();
    TouchButtonGetFrequency.drawButton();
    TouchButtonSine.drawButton();

    // show values
    printFrequencyPeriod(true);
}

void setFrequencyFactor(int aIndexValue) {
    sFrequencyFactorIndex = aIndexValue;
    uint32_t tFactor = 1;
    while (aIndexValue > 0) {
        tFactor *= 1000;
        aIndexValue--;
    }
    sFrequencyFactorTimes1000 = tFactor;
}

/*
 * Slider handlers
 */
void doFrequencySlider(BDSlider * aTheTouchedSlider, uint16_t aValue) {
    float tValue = aValue;
    tValue = tValue / (FREQ_SLIDER_MAX_VALUE / 3); // gives 0-3
    // 950 byte program space needed for pow() and log10f()
    if (is10HzRange) {
        tValue += 1;
    }
    sFrequency = pow(10, tValue);
    ComputePeriodAndSetTimer(false);
}

/*
 * Button handlers
 */

/*
 *
 */
#ifdef AVR
void doToggleSineOutput(BDButton * aTheTouchedButton, int16_t aValue) {
    isSineOutput = aValue;
    // started here
    if (isSineOutput) {
        initTimer1For8BitPWM();
    } else {
        initTimer1ForCTC();
    }
    ComputePeriodAndSetTimer(false);
}
#endif

/**
 * Set frequency to fixed value 1,2,5,10...,1000
 */
void doSetFixedFrequency(BDButton * aTheTouchedButton, int16_t aValue) {
    if (is10HzRange) {
        aValue *= 10;
    }
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
#ifdef AVR
    BlueDisplay1.getNumberWithShortPromptPGM(&doSetFrequency, PSTR("frequency [Hz]"));
#else
    BlueDisplay1.getNumberWithShortPrompt(&doSetFrequency, ("frequency [Hz]"));
#endif
}
#endif

void doFrequencyGeneratorStartStop(BDButton * aTheTouchedButton, int16_t aValue) {
    isOutputEnabled = aValue;
    if (isOutputEnabled) {
        // Start timer
#ifndef AVR
        Synth_Timer_Start();
#endif
        ComputePeriodAndSetTimer(true);
    } else {
        // Stop timer
#ifdef AVR
        TCCR1B &= ~TIMER_PRESCALER_MASK;
#else
        Synth_Timer_Stop();
#endif
    }
}

/*
 * uses global value sFrequency and sPeriodInt
 */
void printFrequencyPeriod(bool aSetSlider) {
    float tPeriodMicros;

#ifdef AVR
    dtostrf(sFrequency, 9, 3, &sStringBuffer[20]);
    sprintf_P(sStringBuffer, PSTR("%s%cHz"), &sStringBuffer[20], FrequencyFactorChars[sFrequencyFactorIndex]);
    tPeriodMicros = sPeriodInt;
    tPeriodMicros /= 8;
#else
// recompute exact frequency for given integer period
    float tPeriodFloat = sPeriodInt;
    float tFrequency = (36000000000 / sFrequencyFactorTimes1000) / tPeriodFloat;
    snprintf(sStringBuffer, sizeof sStringBuffer, "%9.3f%cHz", tFrequency, FrequencyFactorChars[sFrequencyFactorIndex]);
    tPeriodMicros = tPeriodFloat;
    tPeriodMicros /= 36;
#endif
// print frequency
    BlueDisplay1.drawText(FREQ_SLIDER_X + 2 * TEXT_SIZE_22_WIDTH, TEXT_SIZE_22_HEIGHT, sStringBuffer, TEXT_SIZE_22,
    COLOR_RED, COLOR_BACKGROUND_FREQ);

// output period
    char tUnitChar = '\xB5';    // micro
    if (tPeriodMicros > 10000) {
        tPeriodMicros /= 1000;
        tUnitChar = 'm';
    }
#ifdef AVR
    dtostrf(tPeriodMicros, 10, 3, &sStringBuffer[20]);
    sprintf_P(sStringBuffer, PSTR("%s%cs"), &sStringBuffer[20], tUnitChar);
#else
    snprintf(sStringBuffer, sizeof sStringBuffer, "%10.3f%cs", tPeriodMicros, tUnitChar);
#endif
    BlueDisplay1.drawText(FREQ_SLIDER_X, TEXT_SIZE_22_HEIGHT + 4 + TEXT_SIZE_22_ASCEND, sStringBuffer, TEXT_SIZE_22,
    COLOR_BLUE, COLOR_BACKGROUND_FREQ);

    if (aSetSlider) {
        // 950 byte program space needed for pow() and log10f()
        uint16_t tSliderValue = log10f(sFrequency) * 100;
        if (is10HzRange) {
            tSliderValue -= 100;
        }
        TouchSliderFrequency.setActualValueAndDrawBar(tSliderValue);
    }
}

/**
 * Computes Autoreload value for synthesizer from 8,381 mHz (0xFFFFFFFF) to 18MHz (0x02) and prints frequency value
 * @param aSetSlider
 * @param global variable sFrequency
 * @return true if error happened
 */
bool ComputePeriodAndSetTimer(bool aSetSlider) {
    if (!isOutputEnabled) {
        return true;
    }
    bool tIsError = false;
#ifdef AVR
    if (isSineOutput) {
        sPeriodInt = setSineFrequency(sFrequency) * 8;
    } else {
        /*
         * Times 500 would be correct because timer runs in toggle mode and has 8 MHz
         * But F_CPU * 500 does not fit in a 32 bit integer so use half of it which fits
         */
        uint32_t tDividerInt = ((F_CPU * 250) / sFrequencyFactorTimes1000);
        float tFrequency = sFrequency;
        if (tDividerInt > 0x7FFFFFFF) {
            tFrequency /= 2;
        } else {
            tDividerInt *= 2;
        }
        uint32_t tDividerSave = tDividerInt;
        tDividerInt /= tFrequency;

        if (tDividerInt == 0) {
            // 8 Mhz / 0.125 us is Max
            tIsError = true;
            tDividerInt = 1;
            tFrequency = 8;
        }

        // determine prescaler
        uint8_t tPrescalerHWValue = 1;    // direct clock
        uint16_t tPrescaler = 1;
        if (tDividerInt > 0x10000) {
            tDividerInt >>= 3;
            if (tDividerInt <= 0x10000) {
                tPrescaler = 8;
                tPrescalerHWValue = 2;
            } else {
                tDividerInt >>= 3;
                if (tDividerInt <= 0x10000) {
                    tPrescaler = 64;
                    tPrescalerHWValue = 3;
                } else {
                    tDividerInt >>= 2;
                    if (tDividerInt <= 0x10000) {
                        tPrescaler = 256;
                        tPrescalerHWValue = 4;
                    } else {
                        tDividerInt >>= 2;
                        tPrescaler = 1024;
                        tPrescalerHWValue = 5;
                        if (tDividerInt > 0x10000) {
                            // clip to 16 bit value
                            tDividerInt = 0x10000;
                        }
                    }
                }
            }
        }
        TCCR1B &= ~TIMER_PRESCALER_MASK;
        TCCR1B |= tPrescalerHWValue;
        OCR1A = tDividerInt - 1; // set compare match register

        // recompute exact period and frequency for given integer period
        tDividerInt *= tPrescaler;

        // output frequency
        tFrequency = tDividerSave;
        tFrequency /= tDividerInt;
        if (tDividerSave > 0x7FFFFFFF) {
            tFrequency *= 2;
        }
        sFrequency = tFrequency;
        sPeriodInt = tDividerInt;
    }

#else
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
    sPeriodInt = tPeriodInt;
#endif

    printFrequencyPeriod(aSetSlider);
    return tIsError;
}

#ifdef AVR
void initTimer1ForCTC(void) {
    DDRB |= _BV(DDB2); // set pin OC1B = PortB2 -> PIN 10 to output direction

    TIMSK1 = 0; // no interrupts

    TCCR1A = _BV(COM1B0); // Toggle OC1B on compare match / CTC mode
    TCCR1B = _BV(WGM12); // CTC with OCR1A - no clock->timer disabled
    OCR1A = 125 - 1; // set compare match register for 1kHz
    TCNT1 = 0; // init counter
}
#endif

