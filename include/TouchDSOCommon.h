/*
 * TouchDSOCommon.h
 * Declarations from common section
 *
 *  Created on:  01.03.2017
 *      Author:  Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 *      License: GPL v3 (http://www.gnu.org/licenses/gpl.html)
 *      Version: 1.0.0
 */

#ifndef TOUCHDSOCOMMON_H_
#define TOUCHDSOCOMMON_H_

#ifdef AVR
#else
#endif

#include "BDButton.h"

#ifdef AVR
#else
// No PROGMEM on ARM
#define PROGMEM
#endif

/*
 * Trigger values
 */
#define TRIGGER_MODE_AUTOMATIC 0
#define TRIGGER_MODE_MANUAL_TIMEOUT 1
#define TRIGGER_MODE_MANUAL 2 // without timeout
#define TRIGGER_MODE_FREE 3 // waits at least 23 ms (255 samples) for trigger
#define TRIGGER_MODE_EXTERN 4

#ifdef AVR
/*****************************
 * Timebase stuff
 *****************************/
// ADC HW prescaler values
#define PRESCALE4    2 // is noisy
#define PRESCALE8    3 // is reasonable
#define PRESCALE16   4
#define PRESCALE32   5
#define PRESCALE64   6
#define PRESCALE128  7

#define PRESCALE_MIN_VALUE PRESCALE4
#define PRESCALE_MAX_VALUE PRESCALE128
#define PRESCALE_START_VALUE PRESCALE128

/*
 * Don't need to use timer for timebase, since the ADC is also driven by clock and with a few delay times of the ISR
 * one can get reproducible timings just with the ADC conversion timing!
 *
 * Different Acquisition modes depending on Timebase:
 * Mode ultrafast  10us - ADC free running - one loop for read and store 10 bit => needs double buffer space - interrupts blocked for duration of loop
 * Mode fast   20-201us - ADC free running - one loop for read but pre process 10 -> 8 Bit and store - interrupts blocked for duration of loop
 * mode isr without delay 496us   - ADC generates Interrupts - because of ISR initial delay for push just start next conversion immediately
 * mode isr with delay    1,2,5ms - ADC generates Interrupts - busy delay, then start next conversion to match 1,2,5 timebase scale
 * mode isr with multiple read >=10ms - ADC generates Interrupts - to avoid excessive busy delays, start one ore more intermediate conversion just for delay purposes
 */

#define HORIZONTAL_GRID_COUNT 6
/**
 * Formula for Grid Height is:
 * 5V Reference, 10 bit Resolution => 1023/5 = 204.6 Pixel per Volt
 * 1 Volt per Grid -> 204,6 pixel. With scale (shift) 2 => 51.15 pixel.
 * 0.5 Volt -> 102.3 pixel with scale (shift) 1 => 51.15 pixel
 * 0.2 Volt -> 40.96 pixel
 * 1.1V Reference 1023/1.1 = 930 Pixel per Volt
 * 0.2 Volt -> 186 pixel with scale (shift) 2 => 46.5 pixel
 * 0.1 Volt -> 93 pixel with scale (shift) 1 => 46.5 pixel
 * 0.05 Volt -> 46.5 pixel
 */
#define HORIZONTAL_GRID_HEIGHT_1_1V_SHIFT8 11904 // 46.5*256 for 0.05 to 0.2 Volt/div for 6 divs per screen
#define HORIZONTAL_GRID_HEIGHT_2V_SHIFT8 6554 // 25.6*256 for 0.05 to 0.2 Volt/div for 10 divs per screen
#define ADC_CYCLES_PER_CONVERSION 13
#define TIMING_GRID_WIDTH 31 // with 31 instead of 32 the values fit better to 1-2-5 timebase scale
#define TIMEBASE_NUMBER_OF_ENTRIES 15 // the number of different timebases provided
#define TIMEBASE_NUMBER_OF_FAST_PRESCALE 8 // the number of prescale values not equal slowest possible prescale (PRESCALE128)
#define TIMEBASE_INDEX_ULTRAFAST_MODES 0 // first timebase (10) is ultrafast
#define TIMEBASE_INDEX_FAST_MODES 4 // first 5 timebase (10 - 201) are fast free running modes with polling instead of ISR
#define TIMEBASE_NUMBER_OF_XSCALE_CORRECTION 4  // number of timebase which are simulated by display XSale factor
#define TIMEBASE_INDEX_MILLIS 6 // min index to switch to ms instead of us display
#define TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE 11 // min index where chart is drawn while buffer is filled (11 => 50ms)
// the delay between ADC end of conversion and first start of code in ISR
#define ISR_ZERO_DELAY_MICROS 3
#define ISR_DELAY_MICROS_TIMES_4 19 // minimum delay from interrupt to start ADC after delay code - actual 72++ cycles = 4,5++ us
#define ADC_CONVERSION_AS_DELAY_MICROS 112 // only needed for prescaler 128 => 104us per conversion + 8us for 1 clock delay because of manual restarting
#else
/*
 * TIMEBASE
 */
#define CHANGE_REQUESTED_TIMEBASE_FLAG 0x01

#define TIMEBASE_NUMBER_OF_ENTRIES 21 // the number of different timebase provided - 1. entry is not uses until interleaved acquisition is implemented
#define TIMEBASE_NUMBER_OF_EXCACT_ENTRIES 8 // the number of exact float value for timebase because of granularity of clock division
#define TIMEBASE_FAST_MODES 7 // first modes are fast DMA modes
#define TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE 17 // min index where chart is drawn while buffer is filled
#define TIMEBASE_INDEX_CAN_USE_OVERSAMPLING 11 // min index where Min/Max oversampling is enabled
#ifdef STM32F303xC
#define TIMEBASE_NUMBER_START 1  // first reasonable Timebase to display - 0 if interleaving is realized
#define TIMEBASE_NUMBER_OF_XSCALE_CORRECTION 5  // number of timebase which are simulated by display XSale factor
#else
#define TIMEBASE_NUMBER_START 3  // first reasonable Timebase to display - we have only 0.8 MSamples
#define TIMEBASE_NUMBER_OF_XSCALE_CORRECTION 7  // number of timebase which are simulated by display XSale factor
#endif
#define TIMEBASE_INDEX_MILLIS 11 // min index to switch to ms instead of ns display
#define TIMEBASE_INDEX_MICROS 2 // min index to switch to us instead of ns display
#define TIMEBASE_INDEX_START_VALUE 12
#endif

extern const float TimebaseExactDivValuesMicros[] PROGMEM;
#define HORIZONTAL_LINE_LABELS_CAPION_X (REMOTE_DISPLAY_WIDTH - TEXT_SIZE_11_WIDTH * 4)

extern BDButton TouchButtonSlope;
extern char SlopeButtonString[];
// the index of the slope indicator in char array
#define SLOPE_STRING_INDEX 6

extern BDButton TouchButtonTriggerMode;
extern const char TriggerModeButtonStringAuto[] PROGMEM;
extern const char TriggerModeButtonStringManualTimeout[] PROGMEM;
extern const char TriggerModeButtonStringManual[] PROGMEM;
extern const char TriggerModeButtonStringFree[] PROGMEM;
extern const char TriggerModeButtonStringExtern[] PROGMEM;

extern BDButton TouchButtonAutoRangeOnOff;
extern const char AutoRangeButtonStringAuto[] PROGMEM;
extern const char AutoRangeButtonStringManual[] PROGMEM;

void computeMinMax(void);
void computePeriodFrequency(void);
void setTriggerLevelAndHysteresis(int aRawTriggerValue, int aRawTriggerHysteresis);

void redrawDisplay(void);
void drawGridLinesWithHorizLabelsAndTriggerLine();
void drawTriggerLine(void);
void clearTriggerLine(uint8_t aTriggerLevelDisplayValue);

void setSlopeButtonCaption(void);
void setTriggerModeButtonCaption(void);
void setAutoRangeModeAndButtonCaption(bool aNewAutoRangeMode);

void doTriggerSlope(BDButton * aTheTouchedButton, int16_t aValue);
void doTriggerMode(BDButton * aTheTouchedButton, int16_t aValue);
void doRangeMode(BDButton * aTheTouchedButton, int16_t aValue);
void doChartHistory(BDButton * aTheTouchedButton, int16_t aValue);

uint32_t getMicrosFromHorizontalDisplayValue(uint16_t aDisplayValueHorizontal, uint8_t aNumberOfPeriods);

#endif // TOUCHDSOCOMMON_H_
