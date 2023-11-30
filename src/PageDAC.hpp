/*
 * PageDAC.hpp
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

#ifndef _PAGE_DAC_HPP
#define _PAGE_DAC_HPP

/* Private define ------------------------------------------------------------*/
#define DAC_DHR12R2_ADDRESS      0x40007414
#define DAC_DHR8R1_ADDRESS       0x40007410

#define COLOR_BACKGROUND_FREQ COLOR16_WHITE

#define SET_DATE_STRING_INDEX 4 // index of variable part in StringSetDateCaption
#define VERTICAL_SLIDER_MAX_VALUE 180
#define DAC_SLIDER_SIZE 10

#define DAC_START_AMPLITUDE 2048
#define DAC_START_FREQ_SLIDER_VALUE (8 * 20) // 8 = log 2(256)
/* Private variables ---------------------------------------------------------*/
const uint16_t Sine12bit[32] = { 2047, 2447, 2831, 3185, 3498, 3750, 3939, 4056, 4095, 4056, 3939, 3750, 3495, 3185, 2831, 2447,
        2047, 1647, 1263, 909, 599, 344, 155, 38, 0, 38, 155, 344, 599, 909, 1263, 1647 };

BDButton TouchButtonStartStop;
BDButton TouchButtonSetWaveform;

static BDButton TouchButtonAutorepeatFrequencyPlus;
static BDButton TouchButtonAutorepeatFrequencyMinus;

const char StringTriangle[] = "Triangle";
const char StringNoise[] = "Noise";
const char StringSine[] = "Sine";
const char *const WaveformStrings[] = { StringTriangle, StringNoise, StringSine };

static uint16_t sAmplitude;
static uint32_t sFrequency;
static uint16_t sLastFrequencySliderValue;
static uint32_t sPeriodMicros;
static uint32_t sTimerReloadValue;
static uint16_t sOffset = 0;
static uint16_t sLastOffsetSliderValue = 0;
static uint16_t sLastAmplitudeSliderValue = VERTICAL_SLIDER_MAX_VALUE - 20;

#define DAC_WAVEFORM_TRIANGLE   0
#define DAC_WAVEFORM_NOISE      1
#define DAC_WAVEFORM_SINE       2
#define DAC_WAVEFORM_MAX        DAC_WAVEFORM_NOISE

#define MAX_SLIDER_VALUE        240

#define FREQUENCY_SLIDER_START_X 37
BDSlider TouchSliderDACFrequency;
static BDSlider TouchSliderAmplitude;
static BDSlider TouchSliderOffset;

static uint8_t sActualWaveform = DAC_WAVEFORM_TRIANGLE;

// shifted to get better resolution
uint32_t actualTimerAutoreload = 0x100;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

static void doChangeDACWaveform(BDButton *aTheTouchedButton, int16_t aValue) {
    sActualWaveform++;
    if (sActualWaveform == DAC_WAVEFORM_NOISE) {
        DAC_ModeNoise();
        DAC_Start();
    } else if (sActualWaveform > DAC_WAVEFORM_MAX) {
        sActualWaveform = DAC_WAVEFORM_TRIANGLE;
        DAC_ModeTriangle();
        DAC_Start();
    }
    aTheTouchedButton->setCaption(WaveformStrings[sActualWaveform]);
    aTheTouchedButton->drawButton();
}

/**
 * Computes ComputeReloadValue value for DAC timer, minimum is 4
 * @param aSliderValue
 */
static unsigned int ComputeReloadValue(uint16_t aSliderValue) {
    // highest frequency right
    aSliderValue = MAX_SLIDER_VALUE - aSliderValue;
    // logarithmic scale starting with 4
    unsigned int tValue = 1 << ((aSliderValue / 20) + 2);
    // between two logarithmic values we have a linear scale ;-)
    tValue += (tValue * (aSliderValue % 20)) / 20;
    return tValue;
}

/**
 * Computes Autoreload value for DAC timer, minimum is 4
 * @param aSliderValue
 * @param aSetTimer
 */
static void ComputeFrequencyAndSetTimer(uint16_t aReloadValue, bool aSetTimer) {
    if (aSetTimer) {
        sTimerReloadValue = aReloadValue;
        DAC_Timer_SetReloadValue(aReloadValue);
    }
    // Frequency is 72Mhz / Divider
    sFrequency = 72000000 / (2 * sAmplitude * aReloadValue);
    sPeriodMicros = (2 * sAmplitude * aReloadValue) / 72;
    /*
     * Print values
     */
    snprintf(sStringBuffer, sizeof sStringBuffer, "%7luHz", sFrequency);
    TouchSliderDACFrequency.printValue(sStringBuffer);

    if (sPeriodMicros > 100000) {
        snprintf(sStringBuffer, sizeof sStringBuffer, "%5lums", sPeriodMicros / 1000);
    } else {
        snprintf(sStringBuffer, sizeof sStringBuffer, "%5lu\xB5s", sPeriodMicros);
    }
    BlueDisplay1.drawText(FREQUENCY_SLIDER_START_X + MAX_SLIDER_VALUE - 8 * TEXT_SIZE_11_WIDTH,
            BUTTON_HEIGHT_4_LINE_2 + (3 * DAC_SLIDER_SIZE) + SLIDER_DEFAULT_VALUE_MARGIN + getTextAscend(TEXT_SIZE_11),
            sStringBuffer, TEXT_SIZE_11, COLOR16_BLUE, COLOR_BACKGROUND_FREQ);
}

void doChangeDACFrequency(BDButton *aTheTouchedButton, int16_t aValue) {

    uint32_t tTimerReloadValue = sTimerReloadValue + aValue;
    if (tTimerReloadValue < DAC_TIMER_MIN_RELOAD_VALUE) {
        tTimerReloadValue = DAC_TIMER_MIN_RELOAD_VALUE;
        BDButton::playFeedbackTone(true);
        BDButton::disableAutorepeatUntilEndOfTouch();
        return;
    }

    BDButton::playFeedbackTone();
    ComputeFrequencyAndSetTimer(tTimerReloadValue, true);
    int tNewSliderValue = sLastFrequencySliderValue;
    // check if slider value must be updated
    if (aValue > 0) {
        // increase period, decrease frequency
        tNewSliderValue--;
        if (ComputeReloadValue(tNewSliderValue) <= sTimerReloadValue) {
            do {
                tNewSliderValue--;
            } while (ComputeReloadValue(tNewSliderValue) <= sTimerReloadValue);
            tNewSliderValue++;
            sLastFrequencySliderValue = tNewSliderValue;
            TouchSliderDACFrequency.setValueAndDrawBar(tNewSliderValue);
        }
    } else {
        tNewSliderValue++;
        if (ComputeReloadValue(tNewSliderValue) >= sTimerReloadValue) {
            do {
                tNewSliderValue++;
            } while (ComputeReloadValue(tNewSliderValue) >= sTimerReloadValue);
            tNewSliderValue--;
            sLastFrequencySliderValue = tNewSliderValue;
            TouchSliderDACFrequency.setValueAndDrawBar(tNewSliderValue);
        }
    }
}

void doDACVolumeSlider(BDSlider *aTheTouchedSlider, uint16_t aAmplitude) {
    DAC_TriangleAmplitude(aAmplitude / 16);
    sLastAmplitudeSliderValue = (aAmplitude / 16) * 16;

    // compute new slider values
    unsigned int tValue = (0x01 << ((aAmplitude / 16) + 1)) - 1;
    sAmplitude = tValue + 1;
// update frequency
    ComputeFrequencyAndSetTimer(ComputeReloadValue(sLastFrequencySliderValue), false);
    snprintf(sStringBuffer, sizeof sStringBuffer, "%4u", tValue);
    aTheTouchedSlider->printValue(sStringBuffer);
}

void doDACOffsetSlider(BDSlider *aTheTouchedSlider, uint16_t aOffset) {
    sLastOffsetSliderValue = aOffset;
//	if (sActualWaveform == DAC_WAVEFORM_TRIANGLE) {
    sOffset = aOffset * (4096 / VERTICAL_SLIDER_MAX_VALUE);
    DAC_SetOutputValue(sOffset);
    float tOffsetVoltage = sVDDA * aOffset / VERTICAL_SLIDER_MAX_VALUE;
    snprintf(sStringBuffer, sizeof sStringBuffer, "%0.2fV", tOffsetVoltage);
    aTheTouchedSlider->printValue(sStringBuffer);

//	}
}

void doDACFrequencySlider(BDSlider *aTheTouchedSlider, uint16_t aFrequency) {
    sLastFrequencySliderValue = aFrequency;
    ComputeFrequencyAndSetTimer(ComputeReloadValue(aFrequency), true);
}

void doDACStop(BDButton *aTheTouchedButton, int16_t aValue) {
    if (aValue) {
        DAC_Stop();
    } else {
        DAC_Start();
    }
}

void drawDACPage(void) {
    BlueDisplay1.clearDisplay(BACKGROUND_COLOR);

    TouchButtonMainHome.drawButton();
    TouchButtonSetWaveform.drawButton();
    TouchButtonStartStop.drawButton();
    TouchButtonAutorepeatFrequencyPlus.drawButton();
    TouchButtonAutorepeatFrequencyMinus.drawButton();

    TouchSliderAmplitude.drawSlider();
    TouchSliderOffset.drawSlider();
    TouchSliderDACFrequency.drawSlider();
}

void initDACPage(void) {
    DAC_init(); // sets mode triangle
    DAC_TriangleAmplitude(10); //log2(DAC_START_AMPLITUDE)
    sAmplitude = DAC_START_AMPLITUDE;
    sLastFrequencySliderValue = DAC_START_FREQ_SLIDER_VALUE;
}

void startDACPage(void) {
    //1. row
    int tPosY = 0;
    TouchButtonStartStop.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Start", TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, true, &doDACStop);
    TouchButtonStartStop.setCaptionForValueTrue("Stop");

    TouchButtonMainHome.setPosition(BUTTON_WIDTH_3_POS_3, 0);

    //2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    // Frequency slider
    TouchSliderDACFrequency.init(FREQUENCY_SLIDER_START_X, tPosY, DAC_SLIDER_SIZE, MAX_SLIDER_VALUE, MAX_SLIDER_VALUE,
            sLastFrequencySliderValue, COLOR16_BLUE, COLOR16_GREEN, FLAG_SLIDER_SHOW_BORDER | FLAG_SLIDER_IS_HORIZONTAL,
            &doDACFrequencySlider);
    TouchSliderDACFrequency.setCaptionProperties(TEXT_SIZE_11, FLAG_SLIDER_VALUE_CAPTION_ALIGN_MIDDLE, SLIDER_DEFAULT_VALUE_MARGIN,
            COLOR16_RED, BACKGROUND_COLOR);
    TouchSliderDACFrequency.setCaption("Frequency");
    TouchSliderDACFrequency.setPrintValueProperties(TEXT_SIZE_11, FLAG_SLIDER_VALUE_CAPTION_ALIGN_LEFT, SLIDER_DEFAULT_VALUE_MARGIN,
            COLOR16_BLUE, BACKGROUND_COLOR);

    // 3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonSetWaveform.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_RED, StringTriangle,
            TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 1, &doChangeDACWaveform);

    TouchButtonAutorepeatFrequencyMinus.init( FREQUENCY_SLIDER_START_X + 6, tPosY, BUTTON_WIDTH_5, BUTTON_HEIGHT_4, COLOR16_RED,
            "-", TEXT_SIZE_22, FLAG_BUTTON_NO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, 1, &doChangeDACFrequency);
    TouchButtonAutorepeatFrequencyPlus.init(230, tPosY, BUTTON_WIDTH_5, BUTTON_HEIGHT_4, COLOR16_RED, "+", TEXT_SIZE_22,
            FLAG_BUTTON_NO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, -1, &doChangeDACFrequency);
    TouchButtonAutorepeatFrequencyMinus.setButtonAutorepeatTiming(600, 100, 10, 20);
    TouchButtonAutorepeatFrequencyPlus.setButtonAutorepeatTiming(600, 100, 10, 20);

//Amplitude slider
    TouchSliderAmplitude.init(5, 20, DAC_SLIDER_SIZE, VERTICAL_SLIDER_MAX_VALUE, VERTICAL_SLIDER_MAX_VALUE,
            sLastAmplitudeSliderValue, COLOR16_BLUE, COLOR16_GREEN, FLAG_SLIDER_SHOW_BORDER, &doDACVolumeSlider);
    TouchSliderAmplitude.setCaptionProperties(TEXT_SIZE_11,
            FLAG_SLIDER_VALUE_CAPTION_ALIGN_LEFT | FLAG_SLIDER_VALUE_CAPTION_TAKE_DEFAULT_MARGIN, 0, COLOR16_RED, BACKGROUND_COLOR);
    TouchSliderAmplitude.setCaption("Amplitude");
    TouchSliderAmplitude.setPrintValueProperties(TEXT_SIZE_11,
            FLAG_SLIDER_VALUE_CAPTION_ALIGN_MIDDLE | FLAG_SLIDER_VALUE_CAPTION_TAKE_DEFAULT_MARGIN, 0, COLOR16_BLUE, BACKGROUND_COLOR);

//Offset slider
    TouchSliderOffset.init(BlueDisplay1.getDisplayWidth() - (3 * DAC_SLIDER_SIZE) - 1, 20, DAC_SLIDER_SIZE,
    VERTICAL_SLIDER_MAX_VALUE, VERTICAL_SLIDER_MAX_VALUE / 2, sLastOffsetSliderValue, COLOR16_BLUE, COLOR16_GREEN,
            FLAG_SLIDER_SHOW_BORDER, &doDACOffsetSlider);
    TouchSliderOffset.setCaptionProperties(TEXT_SIZE_11, FLAG_SLIDER_VALUE_CAPTION_ALIGN_RIGHT | FLAG_SLIDER_VALUE_CAPTION_TAKE_DEFAULT_MARGIN, 0,
            COLOR16_RED, BACKGROUND_COLOR);
    TouchSliderOffset.setCaption("Offset");
    TouchSliderOffset.setPrintValueProperties(TEXT_SIZE_11,
            FLAG_SLIDER_VALUE_CAPTION_ALIGN_MIDDLE | FLAG_SLIDER_VALUE_CAPTION_TAKE_DEFAULT_MARGIN, 0, COLOR16_BLUE, BACKGROUND_COLOR);
    // for local slider
#if defined(SUPPORT_LOCAL_DISPLAY)
    TouchSliderOffset.mLocalSliderPointer->setXOffsetValue(-(2 * TEXT_SIZE_11_WIDTH));
#endif

// draw buttons and slider
    drawDACPage();
    registerRedrawCallback(&drawDACPage);

    ComputeFrequencyAndSetTimer(ComputeReloadValue(sLastFrequencySliderValue), true);
    DAC_Start();
    // to avoid counting touch move/up for Amplitude slider.
    sDisableMoveEventsUntilTouchUpIsDone = true;
}

void loopDACPage(void) {
    checkAndHandleEvents();
}

void stopDACPage(void) {
    TouchButtonMainHome.setPosition(BUTTON_WIDTH_5_POS_5, 0);

#if defined(SUPPORT_LOCAL_DISPLAY)
    TouchButtonStartStop.deinit();
    TouchButtonSetWaveform.deinit();
    TouchButtonAutorepeatFrequencyPlus.deinit();
    TouchButtonAutorepeatFrequencyMinus.deinit();
    TouchSliderDACFrequency.deinit();
    TouchSliderAmplitude.deinit();
    TouchSliderOffset.deinit();
#endif
}

#endif //_PAGE_DAC_HPP
