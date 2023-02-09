/**
 * TouchDSOAcquisition.hpp
 *
 *  Copyright (C) 2012-2023  Armin Joachimsmeyer
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

/*
 *		2 acquisition methods are implemented (all triggered by timer)
 *		1. Fast
 *      	Data is copied by DMA
 *      	    During copy, the filled data buffer is searched for trigger condition.
 *      	    If no trigger condition met, DMA is restarted and search continues.
 *      	    Timeout by TriggerTimeoutSampleOrLoopCount.
 *		2. Normal
 *			Data is acquired by interrupt service routine (ISR)
 *      	Acquisitions has 3 phases
 *      		1. One pre trigger buffer is filled with DATABUFFER_PRE_TRIGGER_SIZE samples (not in draw while acquire display mode!)
 *      		2. Trigger condition is searched and cyclic pre trigger buffer is filled -> Timeout by TriggerTimeoutSampleOrLoopCount
 *      		3. Data buffer is filled with data -> Size is DISPLAY_WIDTH - DATABUFFER_DISPLAY_SHOW_PRE_TRIGGER_SIZE while running
 *      											or DATABUFFER_SIZE - DATABUFFER_PRE_TRIGGER_SIZE if stopped,

 *
 *		2 display methods implemented
 *		1. Normal for fast timebase values
 *			Acquisition stops after buffer full.
 *			Main loop aligns pre trigger area.
 *			New range, trigger, offset and average are computed
 *			Data is displayed.
 *			New acquisition is started.
 *		2. Draw while acquire for slow timebase
 *			As long as data is written to buffer it is displayed on screen by the main loop.
 *			If trigger condition is met, pre trigger area is aligned and redrawn.
 *			If stop is requested, samples are acquired until end of display buffer reached.
 *			If stop is requested twice, sampling is stopped immediately if end of display reached.
 *
 *		Timing of a complete acquisition and display cycle is app. 5-10ms /200-100Hz
 *		depending on display content and with compiler option -Os (optimize size)
 *		or 17 ms / 60Hz unoptimized
 */

#ifndef _TOUCH_DSO_AQUISITION_HPP
#define _TOUCH_DSO_AQUISITION_HPP

#include "arm_common_tables.h" // For FFT

/*****************************
 * Timebase stuff
 *****************************/
/*
 *  TB  Index Timer-   Timer   Resulting   Exact TB[us] for ADCClock-  XScale      Oversample
 *            Prescaler        Divider     divider value    Prescaler  Factor TB-Index Count Enabled
 * 200ns  0     1        14     14         6.2222222          0         31        0     1
 * 500ns  1     1        14     14         6.2222222          0         12        1     1
 *   1us  2     1        14     14         6.2222222          0          6        2     1
 *   2us  3     1        14|90  14|90      6.2222222          0          3|20     3     1
 *   5us  4     1        22|90  22|90      9.7777777|20       0          2|8      4     1
 *  10us  5     1        22|90  22|90      9.7777777|20       0       -(1)|4      5     1
 *  20us  6     1        45|90  45|90      20                 1       -(1)|2      6     1
 *  50us  7     1       112    112         49.777777          2       -(1)        7     1
 * 100us  8     9        25    225                            3                   6     5
 * 200us  9     9        50    500                            4                   6    10
 * 500us 10     9       125   1125                            5                   7    10
 *   1ms 11     9       250   2250                            6                   8    10   *
 *   2ms 12     9       500                                   7                   8    20   *
 *   5ms 13     9      1250                                   8                   8    50   *
 *  10ms 14     9      2500                                   9                   8   100   *
 *  20ms 15     9      5000                                  10                   8   200   *
 *  50ms 16     9     12500                                  11                   8   500   *
 * 100ms 17     9     25000                                  11                   8  1000   *
 * 200ms 18     9     50000                                  11                   9  1000   *
 * 500ms 19  9000       125                                  11                  10  1000   *
 *1000ms 20  9000       250                                  11                  11  1000   *
 */

const uint16_t TimebaseDivValues[TIMEBASE_NUMBER_OF_ENTRIES] = { 200, 500, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1, 2, 5, 10, 20, 50,
        100, 200, 500, 1000 }; // only for Display - in nanoseconds/microseconds/milliseconds

// 1 => 13.8888889 us resolution, 9 => 1/8 us resolution (72/9)
// Prescaler can divide by 1 (no division)
const uint16_t TimebaseTimerPrescalerDividerValues[TIMEBASE_NUMBER_OF_ENTRIES] = { 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9000, 9000 };

#ifdef STM32F30X
// Timer cannot divide by 1!
// ADC needs at last 14 cycles for 12 bit conversion and fastest sample time of 1.5 cycles => 72/14 = 5.14 MSamples
const uint16_t TimebaseTimerDividerValues[TIMEBASE_NUMBER_OF_ENTRIES] = { 14, 14, 14, 14, 22, 22, 45, 112, 25, 50, 125, 250, 500,
        1250, 2500, 5000, 12500, 25000, 50000, 125, 250 };

// first entries are realized by xScale > 1 and not by higher ADC clock
const uint8_t xScaleForTimebase[TIMEBASE_NUMBER_OF_XSCALE_CORRECTION] = { 31, 12, 6, 3, 2 };

// On 103 the is only one clock prescaler value (value 6 -> 12MHz -> 1,1667usec/conversion) possible if running with 72 MHz
// One conversion must be faster than timer.
const uint16_t ADCClockPrescalerValues[TIMEBASE_NUMBER_OF_ENTRIES] = { 0, 0, 0, 0, 0, 0/*72 MHz*/, 1/*36 MHz*/, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 11, 11, 11, 11 };

// (1/72) * TimebaseTimerDividerValues * (GridSize =32) / xScaleForTimebase - for frequency
const float TimebaseExactDivValuesMicros[TIMEBASE_NUMBER_OF_EXCACT_ENTRIES] = { 6.2222222, 6.2222222, 6.2222222, 6.2222222,
        9.7777777, 9.7777777, 20, 49.777777 };
#else
// Timer cannot divide by 1!
// ADC needs at last 14*6=84 cycles for 12 bit conversion and fastest sample time of 1.5 cycles => 72/(14*6) = 0.857 MSamples
const uint16_t TimebaseTimerDividerValues[TIMEBASE_NUMBER_OF_ENTRIES] = {90, 90, 90, 90, 90, 90, 90, 112, 25, 50, 125, 250, 500,
    1250, 2500, 5000, 12500, 25000, 50000, 125, 250};

// first entries are realized by xScale > 1 and not by higher ADC clock
const uint8_t xScaleForTimebase[TIMEBASE_NUMBER_OF_XSCALE_CORRECTION] = {200, 80, 40, 20, 8, 4, 2}; // do not use first three entries :-)

// (1/72) * TimebaseTimerDividerValues * (GridSize =32) / xScaleForTimebase - for frequency
const float TimebaseExactDivValuesMicros[TIMEBASE_NUMBER_OF_EXCACT_ENTRIES] = {20, 20, 20, 20, 20, 20, 20, 49.777777};
#endif

const uint16_t TimebaseOversampleIndexForMinMaxMode[TIMEBASE_NUMBER_OF_ENTRIES] = { 0, 1, 2, 3, 4, 5, 6, 7, 6, 6, 7, 8, 8, 8, 8, 8,
        8, 8, 9, 10, 11 };
// ADC linearity for divider 225 is perfect and for 112 good. For divider 45 it is reasonable.
const uint16_t TimebaseOversampleCountForMinMaxMode[TIMEBASE_NUMBER_OF_ENTRIES] = { 1, 1, 1, 1, 1, 1, 1, 1, 5, 10, 10, 10, 20, 50,
        100, 200, 500, 1000, 1000, 1000, 1000 };

float getDataBufferTimebaseExactValueMicros(int8_t aTimebaseIndex) {
    float tResult = TimebaseDivValues[aTimebaseIndex];
    if (aTimebaseIndex >= TIMEBASE_INDEX_MILLIS) {
        tResult *= 1000;
    } else if (aTimebaseIndex < TIMEBASE_NUMBER_OF_EXCACT_ENTRIES) {
        tResult = TimebaseExactDivValuesMicros[aTimebaseIndex];
    }
    return tResult;
}

/**
 * FFT stuff
 */
/* generated from armBitRevTable [1024] from arm_common_tables.h - saves 2 k*/
const uint16_t armBitRevTable_256[64] = { 0x40, 0x20, 0x60, 0x10, 0x50, 0x30, 0x70, 0x8, 0x48, 0x28, 0x68, 0x18, 0x58, 0x38, 0x78,
        0x4, 0x44, 0x24, 0x64, 0x14, 0x54, 0x34, 0x74, 0xc, 0x4c, 0x2c, 0x6c, 0x1c, 0x5c, 0x3c, 0x7c, 0x2, 0x42, 0x22, 0x62, 0x12,
        0x52, 0x32, 0x72, 0xa, 0x4a, 0x2a, 0x6a, 0x1a, 0x5a, 0x3a, 0x7a, 0x6, 0x46, 0x26, 0x66, 0x16, 0x56, 0x36, 0x76, 0xe, 0x4e,
        0x2e, 0x6e, 0x1e, 0x5e, 0x3e, 0x7e, 0x1 };
extern "C" void arm_radix4_butterfly_f32(float32_t *pSrc, uint16_t fftLen, float32_t *pCoef, uint16_t twidCoefModifier);
extern "C" void arm_bitreversal_f32(float32_t *pSrc, uint16_t fftSize, uint16_t bitRevFactor, uint16_t *pBitRevTab);

/************************
 * Measurement control
 ************************/
struct MeasurementControlStruct MeasurementControl;
#define ADC_TIMEOUT 2

/*
 * Data buffer
 */
struct DataBufferStruct DataBufferControl;

void *TempBufferForPreTriggerAdjustAndFFT;

/*
 * FFT info
 */
FFTInfoStruct FFTInfo;

/**
 * Attenuator (4051 Hardware) related stuff
 */
/*
 * 0    *0   -> 1MOhm - 0 Ohm
 * 1    *2   -> 1MOhm - 2M Ohm
 * 2    *1   -> 1MOhm - 1MOhm
 * 4     %4  -> 1MOhm - 250kOhm
 * 5    %40  -> 1MOhm - 24kOhm
 * 6    %100 -> 1MOhm - 10kOhm
 * 7 -> 1MOhm - NC
 */
/**
 * scale Values for the horizontal scales
 */
const float ScaleVoltagePerDiv[NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR] = { 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0,
        20.0, 50.0 };
// Attenuation values for active attenuator
float RawAttenuationFactor[NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR] = { 0.5, 1, 1, 1, 1, 1, 4.05837, 4.05837, 41.6666, 41.6666,
        41.6666, 100 };
// Attenuation values for fixed attenuator - Channel0 = /1, Ch1= /10, Ch2= /100
float FixedAttenuationFactor[NUMBER_OF_CHANNELS_WITH_FIXED_ATTENUATOR] = { 1, 10, 100 };

const uint8_t RangePrecision[NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR] = { 2, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0 }; // number of digits to be printed after the decimal point

// Mapping for external attenuator hardware control outputs - ATTENUATOR_INFINITE_VALUE = 0
const uint8_t AttenuatorHardwareValue[NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR] = { 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03,
        0x04, 0x04, 0x04, 0x05 };

// minimal display range without changing attenuator
const uint8_t AttenuatorMinimumRange[NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR] = { 0, 1, 1, 1, 1, 1, 6, 6, 8, 8, 8, 11 };

/**
 * Virtual ADC values for full scale (DisplayHeight) - used for SetAutoRange() and computeAutoDisplayRange()
 */
int MaxPeakToPeakValue[NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR]; // for a virtual 18 bit ADC

/**
 * The array with the display scale factor for raw -> display for all ranges
 * actual values are shift by 18
 */
int ScaleFactorRawToDisplayShift18[NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR];

void initRawToDisplayFactorsAndMaxPeakToPeakValues(void) {
// Reading * ScaleValueForDisplay = Display value - only used for getDisplayFrowRawValue()
// use value shifted by 19 to avoid floating point multiplication by retaining correct value
    for (int i = 0; i < NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR; ++i) {
        ScaleFactorRawToDisplayShift18[i] = sADCScaleFactorShift18 * (RawAttenuationFactor[i] / (2 * ScaleVoltagePerDiv[i]));
        MaxPeakToPeakValue[i] = sReading3Volt * 2 * ScaleVoltagePerDiv[i]; // 50V / div
    }
}

void autoACZeroCalibration(void);
extern "C" void ADC1_2_IRQHandler(void);

/*******************************************************************************************
 * Program code starts here
 *******************************************************************************************/

/************************************************************************
 * Measurement section
 ************************************************************************/

/**
 * Called once at boot time
 */
void initAcquisition(void) {

    DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_START];

    // Channel
    uint tStartChannel = START_ADC_CHANNEL_INDEX;
#ifdef STM32F30X
    MeasurementControl.AttenuatorType = ATTENUATOR_TYPE_ACTIVE_ATTENUATOR;
    MeasurementControl.FirstChannelIndexWithoutAttenuator = NUMBER_OF_CHANNEL_WITH_ACTIVE_ATTENUATOR;

    MeasurementControl.ADMUXChannel = tStartChannel;

    // AC mode on uses DisplayRangeIndex
    MeasurementControl.isACMode = true;
#else
    MeasurementControl.FirstChannelIndexWithoutAttenuator = 0;
    /*
     * detect attached attenuator type
     */
    MeasurementControl.AttenuatorType = DSO_detectAttenuatorType();
    if (MeasurementControl.AttenuatorType == ATTENUATOR_TYPE_FIXED_ATTENUATOR) {
        MeasurementControl.FirstChannelIndexWithoutAttenuator = NUMBER_OF_CHANNEL_WITH_FIXED_ATTENUATOR;
        // Start with %10 Channel (channel 1)
        tStartChannel = 1;
    } else if (MeasurementControl.AttenuatorType >= ATTENUATOR_TYPE_ACTIVE_ATTENUATOR) {
        MeasurementControl.FirstChannelIndexWithoutAttenuator = NUMBER_OF_CHANNEL_WITH_FIXED_ATTENUATOR;
        //initTimer2(); // start timer2 for generating VEE (negative Voltage for external hardware)
    }
    /*
     * Values must be set before initGui()
     */
    MeasurementControl.ADMUXChannel = tStartChannel;

    // AC mode off (needs DisplayRangeIndex if true)
    MeasurementControl.isACMode = false;
    MeasurementControl.isMinMaxMode = true;

#endif

    // Timebase
    MeasurementControl.TimebaseEffectiveIndex = TIMEBASE_INDEX_START_VALUE;
}

void clearDataBuffer() {
    memset(DataBufferControl.DataBuffer, 0, sizeof(DataBufferControl.DataBuffer));
}

/**
 * called at every start of application (startDSO())
 */
void resetAcquisition(void) {
    MeasurementControl.isRunning = false;
    MeasurementControl.isSingleShotMode = false;
    MeasurementControl.isMinMaxMode = true;
    MeasurementControl.isEffectiveMinMaxMode = true;

#if defined(SUPPORT_LOCAL_DISPLAY)
    MeasurementControl.ADS7846ChannelsAsDatasource = false;
#endif

    // Calibration
    ADC_setRawToVoltFactor();
    autoACZeroCalibration(); // sets MeasurementControl.DSOReadingACZero
    initRawToDisplayFactorsAndMaxPeakToPeakValues();
    // needs AttenuatorType and sADCToVoltFactor - sets Channel + Range + actualDSORawToVoltFactor values
    setChannel(MeasurementControl.ADMUXChannel);

    // Timebase
    MeasurementControl.TimebaseNewIndex = MeasurementControl.TimebaseEffectiveIndex;
    changeTimeBase();

    // Trigger
    MeasurementControl.TriggerSlopeRising = true;
    MeasurementControl.TriggerMode = TRIGGER_MODE_AUTOMATIC;

    setTriggerLevelAndHysteresis(10, 5); // must be initialized to enable auto triggering

    // Set range control
    MeasurementControl.TimestampLastRangeChange = 0; // enable direct range change at start
    MeasurementControl.RangeAutomatic = true;
    MeasurementControl.OffsetMode = OFFSET_MODE_0_VOLT;

    // init variables etc. uses DisplayRangeIndex
    setACMode(MeasurementControl.isACMode);
}

/**
 * prepares all variables for new acquisition
 * switches between fast an interrupt mode depending on TIMEBASE_FAST_MODES
 * sets ADC status register including prescaler
 * setup new interrupt cycle only if not to be stopped
 */
void startAcquisition(void) {

    // Default setting (regular mode)
    // start with waiting for triggering condition
    MeasurementControl.TriggerActualPhase = PHASE_PRE_TRIGGER;
    MeasurementControl.TriggerSampleCount = 0;
    MeasurementControl.TriggerStatus = TRIGGER_STATUS_START;
    MeasurementControl.doPretriggerCopyForDisplay = false;
    MeasurementControl.TimebaseFastDMAMode = false;

    if (MeasurementControl.TimebaseEffectiveIndex < TIMEBASE_FAST_MODES) {
        // TimebaseFastDMAMode must be set only here at beginning of acquisition
        MeasurementControl.TimebaseFastDMAMode = true;
    } else if (MeasurementControl.TimebaseEffectiveIndex >= TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE) {
        MeasurementControl.TriggerActualPhase = PHASE_SEARCH_TRIGGER;
        DataBufferControl.DataBufferNextDrawPointer = &DataBufferControl.DataBuffer[0];
        DataBufferControl.NextDrawXValue = 0;
        MeasurementControl.TriggerPhaseJustEnded = false;
        DataBufferControl.DataBufferPreTriggerAreaWrapAround = false;
    }

    // start and end pointer
    // end pointer should be computed here because of trigger mode
    DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_START];
    DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_END];
    DataBufferControl.DataBufferNextInPointer = &DataBufferControl.DataBuffer[0];

    if (MeasurementControl.TriggerMode == TRIGGER_MODE_FREE) {
        MeasurementControl.TriggerActualPhase = PHASE_POST_TRIGGER;
        // no trigger => no pretrigger area
        DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[0];
        DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[REMOTE_DISPLAY_WIDTH - 1];
    }

    if (MeasurementControl.isSingleShotMode) {
        // Start and request immediate stop
        MeasurementControl.StopRequested = true;
    }
    if (MeasurementControl.StopRequested) {
        // last acquisition or single shot mode -> use whole data buffer
        DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[DATABUFFER_SIZE - 1];
    }

#if defined(SUPPORT_LOCAL_DISPLAY)
    if (MeasurementControl.ADS7846ChannelsAsDatasource) {
        // to skip pretrigger and avoid pre trigger buffer adjustment
        DataBufferControl.DataBufferNextInPointer = DataBufferControl.DataBufferDisplayStart;
        DataBufferControl.DataBufferPreTriggerNextPointer = &DataBufferControl.DataBuffer[0];
        // return here because of no interrupt handling for ADS7846Channels
        return;
    }
#endif

    MeasurementControl.StopAcknowledged = false;
    DataBufferControl.DataBufferFull = false;

    /*
     * Start ADC + timer
     */
    ADC_DSO_startTimer();
    if (!MeasurementControl.TimebaseFastDMAMode && !MeasurementControl.isEffectiveMinMaxMode) {
        // ADC -> interrupt mode
        __HAL_ADC_ENABLE_IT(&ADC1Handle, ADC_IT_EOC);
        //ADC_enableEOCInterrupt(ADC1 );
        // starts conversion at next timer edge
#ifdef STM32F30X
        ADC1Handle.Instance->CR |= ADC_CR_ADSTART;
#else
        SET_BIT(ADC1Handle.Instance->CR2, ADC_CR2_EXTTRIG);
#endif
    } else {
        // ADC -> DMA -> interrupt mode
        if (MeasurementControl.isEffectiveMinMaxMode) {
            ADC1_DMA_start((uint32_t) & DataBufferControl.DataBufferTempDMAValues[0], MeasurementControl.MinMaxModeTempValuesSize,
                    true);
        } else {
            ADC1_DMA_start((uint32_t) & DataBufferControl.DataBuffer[0], DATABUFFER_SIZE, false);
        }
    }
}

/*
 * Calibration
 * 1. Tune the hardware amplification so that VRef gives a reading of 0x3FE.
 * 2. Attach probe to VRef or other constant voltage source.
 * 3. Set DC range and get data for 10 ms and process auto range.
 * 4. If range is suitable (i.e highest range for attenuation) output tone and wait for 100ms
 * 							otherwise output message go to 3.
 * 5. Acquire data for 100ms (this reduces 50Hz and 60Hz interference)
 * 4. If p2p < threshold then switch range else output message "ripple too high"
 * 5. wait for 100 ms for range switch to settle and acquire data for 100ms in new range.
 * 6. If p2p < threshold compute attenuation factor and store in CMOS Ram
 * 7. Output Message "Complete"
 */

/**
 * compute how many samples are needed for TRIGGER_TIMEOUT_MILLIS (cur. 200ms) but use at least TRIGGER_TIMEOUT_MIN_SAMPLES samples
 * @param aTimebaseIndex
 * @return
 */
uint16_t computeNumberOfSamplesToTimeout(int8_t aTimebaseIndex) {
    uint32_t tCount = REMOTE_DISPLAY_WIDTH * 225 * TRIGGER_TIMEOUT_MILLIS
            / (TimebaseTimerDividerValues[aTimebaseIndex] * TimebaseTimerPrescalerDividerValues[aTimebaseIndex]);
    if (tCount < TRIGGER_TIMEOUT_MIN_SAMPLES) {
        tCount = TRIGGER_TIMEOUT_MIN_SAMPLES;
    }
    if (aTimebaseIndex < TIMEBASE_FAST_MODES) {
        // compute loop count instead of sample count
        tCount /= (DATABUFFER_SIZE - REMOTE_DISPLAY_WIDTH);
    }
    if (tCount > 0xFFFF) {
        tCount = 0xFFFF;
    }
    return tCount;
}

#if defined(SUPPORT_LOCAL_DISPLAY)
/*
 * read ADS7846 channels
 */
void readADS7846Channels(void) {

    uint16_t tValue;
    uint16_t *DataPointer = DataBufferControl.DataBufferNextInPointer;
    tValue = TouchPanel.readChannel(ADS7846ChannelMapping[MeasurementControl.ADMUXChannel], true, false, 0x1);
    while (DataPointer <= DataBufferControl.DataBufferEndPointer) {
        tValue = TouchPanel.readChannel(ADS7846ChannelMapping[MeasurementControl.ADMUXChannel], true,
        false, 0x1);
        *DataPointer++ = tValue;
    }
    DataBufferControl.DataBufferFull = true;
}
#endif

/*
 * called by half transfer interrupt
 */
void DMACheckForTriggerCondition(void) {
    if (MeasurementControl.TriggerMode != TRIGGER_MODE_FREE) {

        uint16_t tValue;
        uint8_t tTriggerStatus = TRIGGER_STATUS_START;
        bool tFalling = !MeasurementControl.TriggerSlopeRising;
        uint16_t tActualCompareValue = MeasurementControl.RawTriggerLevelHysteresis;
        // start after pre trigger values
        uint16_t *tDMAMemoryAddress = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE];
        uint16_t *tEndMemoryAddress = &DataBufferControl.DataBuffer[DATABUFFER_SIZE / 2];
        do {
            clearSystic();
            while (tDMAMemoryAddress < tEndMemoryAddress) { // 37 instructions at -o0 if nothing found
                /*
                 * scan from end of pre trigger to half of buffer for trigger condition
                 */
                tValue = *tDMAMemoryAddress++;
                bool tValueGreaterRef = (tValue > tActualCompareValue);
                tValueGreaterRef = tValueGreaterRef ^ tFalling; // change value if tFalling == true
                if (tTriggerStatus == TRIGGER_STATUS_START) {
                    // rising slope - wait for value below 1. threshold
                    // falling slope - wait for value above 1. threshold
                    if (!tValueGreaterRef) {
                        tTriggerStatus = TRIGGER_STATUS_AFTER_HYSTERESIS;
                        tActualCompareValue = MeasurementControl.RawTriggerLevel;
                    }
                } else {
                    // rising slope - wait for value to rise above 2. threshold
                    // falling slope - wait for value to go below 2. threshold
                    if (tValueGreaterRef) {
                        tTriggerStatus = TRIGGER_OK;
                        MeasurementControl.TriggerStatus = TRIGGER_OK;
                        break;
                    }
                }
            }

            if (tTriggerStatus == TRIGGER_OK) {
                // trigger found -> exit from loop
                break;
            }
            /*
             * New DMA Transfer must be started if no full screen left - see setting below "if (tCount <..."
             */
            if (tEndMemoryAddress
                    >= &DataBufferControl.DataBuffer[DATABUFFER_SIZE
                            - (REMOTE_DISPLAY_WIDTH - DisplayControl.DatabufferPreTriggerDisplaySize) - 1]
                    || (MeasurementControl.isSingleShotMode)) {
                // No trigger found and not enough buffer left for a screen of data
                // or on singleshot mode, less than a half buffer left -> restart DMA
                if (!MeasurementControl.isSingleShotMode) {
                    // No timeout in single shot mode
                    if (MeasurementControl.TriggerSampleCount++ > MeasurementControl.TriggerTimeoutSampleOrLoopCount) {
                        // Trigger condition not met and timeout reached
                        //BSP_LED_Toggle(LED6); // GREEN LEFT
                        // timeout exit from loop
                        break;
                    }
                }
                // copy pretrigger data for display in loop
                if (MeasurementControl.doPretriggerCopyForDisplay) {
                    memcpy(TempBufferForPreTriggerAdjustAndFFT, &DataBufferControl.DataBuffer[0],
                            DATABUFFER_PRE_TRIGGER_SIZE * sizeof(DataBufferControl.DataBuffer[0]));
                    MeasurementControl.doPretriggerCopyForDisplay = false;
                }

                // restart DMA, leave ISR and wait for new interrupt
                ADC1_DMA_start((uint32_t) & DataBufferControl.DataBuffer[0], DATABUFFER_SIZE, false);
                // reset transfer complete status before return since this interrupt may happened and must be re enabled
                __HAL_DMA_CLEAR_FLAG(ADC1Handle.DMA_Handle, DMA_FLAG_TC1);
                //DMA_ClearITPendingBit (DMA1_IT_TC1);
                DataBufferControl.DataBufferFull = false;
                return;
            } else {
                // set new tEndMemoryAddress and check if it is before last possible trigger position
                // appr. 77063 cycles / 1 ms until we get here first time (1440 values checked 53 cycles/value at -o0)
                uint32_t tCount = DSO_DMA_CHANNEL->CNDTR;
                if (tCount < REMOTE_DISPLAY_WIDTH - DisplayControl.DatabufferPreTriggerDisplaySize) {
                    tCount = REMOTE_DISPLAY_WIDTH - DisplayControl.DatabufferPreTriggerDisplaySize;
                }
                tEndMemoryAddress = &DataBufferControl.DataBuffer[DATABUFFER_SIZE - tCount - 1];
            }
        } while (true); // end with break above

        /*
         * set pointer for display of data
         */
        if (tTriggerStatus != TRIGGER_OK) {
            // timeout here
            tDMAMemoryAddress = &DataBufferControl.DataBuffer[DisplayControl.DatabufferPreTriggerDisplaySize];
        }

        // correct start by DisplayControl.XScale - XScale is known to be >=0 here
        DataBufferControl.DataBufferDisplayStart = tDMAMemoryAddress - 1
                - adjustIntWithScaleFactor(DisplayControl.DatabufferPreTriggerDisplaySize, DisplayControl.XScale);
        if (MeasurementControl.StopRequested) {
            MeasurementControl.StopAcknowledged = true;
        } else {
            // set end pointer to end of display for reproducible min max + average findings - XScale is known to be >=0 here
            int tAdjust = adjustIntWithScaleFactor(REMOTE_DISPLAY_WIDTH - DisplayControl.DatabufferPreTriggerDisplaySize - 1,
                    DisplayControl.XScale);
            DataBufferControl.DataBufferEndPointer = tDMAMemoryAddress + tAdjust;
        }
    }
}

/*
 * called by half transfer and transfer complete interrupt with different processFirstHalfOfBuffer flags
 */
extern "C" void DMAProcessMinMax(bool processFirstHalfOfBuffer) {
    uint16_t tValue;
    uint16_t tMinValue;
    uint16_t tMaxValue;
    uint16_t *tDMAMemoryAddress;
    int tLoopCount;
    if (processFirstHalfOfBuffer) {
        tLoopCount = MeasurementControl.MinMaxModeTempValuesSize / 2;
        tDMAMemoryAddress = &DataBufferControl.DataBufferTempDMAValues[0];
        tMaxValue = *tDMAMemoryAddress++;
        tMinValue = tMaxValue;
        tLoopCount--;
    } else {
        // to support MinMaxModeTempValuesSize value of 5
        tLoopCount = (MeasurementControl.MinMaxModeTempValuesSize + 1) / 2;
        tDMAMemoryAddress = &DataBufferControl.DataBufferTempDMAValues[tLoopCount];
        tMaxValue = MeasurementControl.MinMaxModeMaxValue;
        tMinValue = MeasurementControl.MinMaxModeMinValue;
    }

    while (tLoopCount > 0) {
        /*
         * scan half of buffer for min + max
         */
        tValue = *tDMAMemoryAddress++;
        if (tValue > tMaxValue) {
            tMaxValue = tValue;
        } else if (tValue < tMinValue) {
            tMinValue = tValue;
        }
        tLoopCount--;
    }
    MeasurementControl.MinMaxModeMaxValue = tMaxValue;
    MeasurementControl.MinMaxModeMinValue = tMinValue;
    if (!processFirstHalfOfBuffer) {
        // process value
        ADC1_2_IRQHandler();
    }
}

extern "C" void DMA1_Channel1_IRQHandler(void) {

    // Test on DMA Transfer Complete interrupt
    if (__HAL_DMA_GET_FLAG(ADC1Handle.DMA_Handle, DMA_FLAG_TC1)) {
//  if (DMA_GetITStatus (DMA1_IT_TC1)) {
        /* Clear DMA  Transfer Complete interrupt pending bit */
        __HAL_DMA_CLEAR_FLAG(ADC1Handle.DMA_Handle, DMA_FLAG_TC1);
        //DMA_ClearITPendingBit(DMA1_IT_TC1);
        if (MeasurementControl.isEffectiveMinMaxMode) {
            DMAProcessMinMax(false);
        } else {
            // stop conversion if Fast DMA mode
            DataBufferControl.DataBufferFull = true;
#ifdef STM32F30X
            ADC1Handle.Instance->CR |= ADC_CR_ADSTP;
#else
            CLEAR_BIT(ADC1Handle.Instance->CR2, ADC_CR2_EXTTRIG);
#endif
        }
    }
    // Test on DMA Transfer Error interrupt
    if (__HAL_DMA_GET_FLAG(ADC1Handle.DMA_Handle, DMA_FLAG_TE1)) {
        failParamMessage(ADC1Handle.DMA_Handle->Instance->CPAR, "DMA Error");
        __HAL_DMA_CLEAR_FLAG(ADC1Handle.DMA_Handle, DMA_FLAG_TE1);
//        DMA_ClearITPendingBit(DMA1_IT_TE1);
    }
    // Test on DMA Transfer Half interrupt
    if (__HAL_DMA_GET_FLAG(ADC1Handle.DMA_Handle, DMA_FLAG_HT1)) {
        __HAL_DMA_CLEAR_FLAG(ADC1Handle.DMA_Handle, DMA_FLAG_HT1);
        //DMA_ClearITPendingBit(DMA1_IT_HT1);
        if (MeasurementControl.isEffectiveMinMaxMode) {
            DMAProcessMinMax(true);
        } else {
            DMACheckForTriggerCondition();
        }
    }
}

/**
 * Interrupt service routine for adc interrupt
 * app. 3.5 microseconds when compiled with no optimizations.
 * With optimizations it works up to 50us/div
 * first value which meets trigger condition is stored at position DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE]
 */

extern "C" void ADC1_2_IRQHandler(void) {

    uint16_t tValue;
    uint16_t tValueMin;
    if (MeasurementControl.isEffectiveMinMaxMode) {
        tValue = MeasurementControl.MinMaxModeMaxValue;
        tValueMin = MeasurementControl.MinMaxModeMinValue;
    } else {
        tValue = ADC1Handle.Instance->DR;
        tValueMin = tValue;
    }

    uint16_t *tDataBufferPointer = DataBufferControl.DataBufferNextInPointer;

    /*
     * read at least DATABUFFER_PRE_TRIGGER_SIZE values in pre trigger phase
     */
    if (MeasurementControl.TriggerActualPhase == PHASE_PRE_TRIGGER) {
        // store value
        *tDataBufferPointer = tValue;
        *(tDataBufferPointer + DATABUFFER_MIN_OFFSET) = tValueMin;
        tDataBufferPointer++;
        MeasurementControl.TriggerSampleCount++;
        if (MeasurementControl.TriggerSampleCount >= DATABUFFER_PRE_TRIGGER_SIZE) {
            // now we have read at least DATABUFFER_PRE_TRIGGER_SIZE values => start search for trigger
            if (MeasurementControl.TriggerMode == TRIGGER_MODE_FREE) {
                MeasurementControl.TriggerActualPhase = PHASE_POST_TRIGGER;
            } else {
                MeasurementControl.TriggerActualPhase = PHASE_SEARCH_TRIGGER;
                MeasurementControl.TriggerSampleCount = 0;
                tDataBufferPointer = &DataBufferControl.DataBuffer[0];
            }
        }

    } else if (MeasurementControl.TriggerActualPhase == PHASE_SEARCH_TRIGGER) {
        bool tTriggerFound = false;
        /*
         * Trigger detection here
         */
        uint8_t tTriggerStatus = MeasurementControl.TriggerStatus;

        if (MeasurementControl.TriggerSlopeRising) {
            if (tTriggerStatus == TRIGGER_STATUS_START) {
                // rising slope - wait for value below 1. threshold
                if (tValue < MeasurementControl.RawTriggerLevelHysteresis
                        || tValueMin < MeasurementControl.RawTriggerLevelHysteresis) {
                    MeasurementControl.TriggerStatus = TRIGGER_STATUS_AFTER_HYSTERESIS;
                }
            } else {
                // here tTriggerStatus == TRIGGER_BEFORE_THRESHOLD
                // rising slope - wait for value to rise above 2. threshold
                if (tValue > MeasurementControl.RawTriggerLevel || tValueMin > MeasurementControl.RawTriggerLevel) {
                    // start reading into buffer
                    tTriggerFound = true;
                    MeasurementControl.TriggerStatus = TRIGGER_OK;
                }
            }
        } else {
            if (tTriggerStatus == TRIGGER_STATUS_START) {
                // falling slope - wait for value above 1. threshold
                if (tValue > MeasurementControl.RawTriggerLevelHysteresis
                        || tValueMin > MeasurementControl.RawTriggerLevelHysteresis) {
                    MeasurementControl.TriggerStatus = TRIGGER_STATUS_AFTER_HYSTERESIS;
                }
            } else {
                // here tTriggerStatus == TRIGGER_BEFORE_THRESHOLD
                // falling slope - wait for value to go below 2. threshold
                if (tValue < MeasurementControl.RawTriggerLevel || tValueMin < MeasurementControl.RawTriggerLevel) {
                    // start reading into buffer
                    tTriggerFound = true;
                    MeasurementControl.TriggerStatus = TRIGGER_OK;
                }
            }
        }

        if (!tTriggerFound) {
            /*
             * Store value in pre trigger area and check for wrap around and timeout
             */
            // store value
            *tDataBufferPointer = tValue;
            *(tDataBufferPointer + DATABUFFER_MIN_OFFSET) = tValueMin;
            tDataBufferPointer++;
            MeasurementControl.TriggerSampleCount++;
            // detect end of pre trigger buffer
            if (tDataBufferPointer >= &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE]) {
                // wrap around - for draw while acquire
                DataBufferControl.DataBufferPreTriggerAreaWrapAround = true;
                tDataBufferPointer = &DataBufferControl.DataBuffer[0];
            }
            // prepare for next
            DataBufferControl.DataBufferNextInPointer = tDataBufferPointer;

            if (MeasurementControl.isSingleShotMode) {
                // No timeout in single shot mode - store (max) value for display
                MeasurementControl.RawValueBeforeTrigger = tValue;
                return;
            }
            /*
             * Trigger timeout handling
             */
            if (MeasurementControl.TriggerSampleCount < MeasurementControl.TriggerTimeoutSampleOrLoopCount) {
                /*
                 * Trigger condition not met and timeout not reached
                 */
                return;
            }
        }

        /*
         * Here trigger just found or trigger timeout
         * reset trigger flag and initialize max and min and set data buffer
         */
        MeasurementControl.TriggerActualPhase = PHASE_POST_TRIGGER;
        DataBufferControl.DataBufferPreTriggerNextPointer = tDataBufferPointer;
        /*
         * only needed for draw while acquire
         * set flag for main loop to detect end of trigger phase
         */
        MeasurementControl.TriggerPhaseJustEnded = true;
        tDataBufferPointer = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE];
        // store first value of post trigger area
        *tDataBufferPointer = tValue;
        *(tDataBufferPointer + DATABUFFER_MIN_OFFSET) = tValueMin;
        tDataBufferPointer++;

    } else {
        /*
         * Here regular reading to data buffer
         */
        if (tDataBufferPointer <= DataBufferControl.DataBufferEndPointer) {
            // store display value
            *tDataBufferPointer = tValue;
            *(tDataBufferPointer + DATABUFFER_MIN_OFFSET) = tValueMin;
            tDataBufferPointer++;
        } else {
            ADC1_DMA_stop();
            // stop acquisition
            // End of conversion => stop ADC in order to make it reconfigurable (change timebase)
#ifdef STM32F30X
            ADC1Handle.Instance->CR |= ADC_CR_ADSTP;
#else
            CLEAR_BIT(ADC1Handle.Instance->CR2, ADC_CR2_EXTTRIG);
#endif
            __HAL_ADC_DISABLE_IT(&ADC1Handle, ADC_IT_EOC);

            /*
             * signal to main loop or DMA EOT interrupt that acquisition ended
             * Main loop is responsible to start a new acquisition via call of startAcquisition();
             */
            DataBufferControl.DataBufferFull = true;
        }
    }
    // prepare for next
    DataBufferControl.DataBufferNextInPointer = tDataBufferPointer;
}
/**
 * set attenuator to infinite and AC Pin active, read n samples and store it in MeasurementControl.DSOReadingACZero
 */
void autoACZeroCalibration(void) {
    if (MeasurementControl.AttenuatorType != ATTENUATOR_TYPE_ACTIVE_ATTENUATOR) {
        MeasurementControl.RawDSOReadingACZero = ADC_MAX_CONVERSION_VALUE / 2;
    }
    bool tState = DSO_getACMode();
    DSO_setACMode(true);
    DSO_setAttenuator (ACTIVE_ATTENUATOR_INFINITE_VALUE);
    ADC1_getChannelValue(ADCInputMUXChannels[0], 0); // one sample at first channel
    // wait to settle
    delay(10);
    MeasurementControl.RawDSOReadingACZero = ADC1_getChannelValue(ADCInputMUXChannels[0], 4); // 16 times Oversampling at first channel
    if (MeasurementControl.RawDSOReadingACZero > ADC_MAX_CONVERSION_VALUE) {
        // conversion impossible -> set fallback value
        MeasurementControl.RawDSOReadingACZero = ADC_MAX_CONVERSION_VALUE / 2;
        return;
    }
    // restore original AC Mode
    DSO_setACMode(tState);
}

/**
 * Computes a display range in order to fill screen with signal.
 * Only makes sense in conjunction with a display offset value.
 * Therefore calls computeAutoOffset() if display range changed or display clipping occurs.
 */
int computeAutoOffsetRange(int aRangeIndex) {
    /*
     * check if a display range less than input range is more suitable.
     */
    // get relevant peak2peak value
    // increase value by OFFSET_FIT_FACTOR = 10/8 (1.25) to have margin to have minimum starting at first grid line - exact factor for this would be (6/5 = 1.2)
    int tPeakToPeak = MeasurementControl.RawValueMax - MeasurementControl.RawValueMin;
    tPeakToPeak = ((tPeakToPeak * 10) / 8) * RawAttenuationFactor[aRangeIndex];

    /*
     * find new offset range index
     */
    int tNewOffsetRangeIndex;
    for (tNewOffsetRangeIndex = AttenuatorMinimumRange[aRangeIndex]; tNewOffsetRangeIndex < aRangeIndex; ++tNewOffsetRangeIndex) {
        if (tPeakToPeak <= MaxPeakToPeakValue[tNewOffsetRangeIndex]) {
            //actual PeakToPeak fits into display size now
            break;
        }
    }

    return tNewOffsetRangeIndex;
}

/**
 * compute offset based on center value in order display values at center of screen
 */
void computeAutoOffset(void) {
    int tValueMiddleY = MeasurementControl.RawValueMin + ((MeasurementControl.RawValueMax - MeasurementControl.RawValueMin) / 2);
    if (MeasurementControl.ChannelIsACMode) {
        tValueMiddleY -= MeasurementControl.RawDSOReadingACZero;
    }

    // Adjust to multiple of div
    // formula is: RawPerDiv[i] = HORIZONTAL_GRID_HEIGHT / ScaleFactorRawToDisplay[i]);
    // tNumberOfGridLinesToSkip = tValueMiddleY / RawPerDiv[tNewDisplayRangeIndex];
    signed int tNumberOfGridLinesToSkip = tValueMiddleY
            * (ScaleFactorRawToDisplayShift18[MeasurementControl.DisplayRangeIndex] / HORIZONTAL_GRID_HEIGHT);
    tNumberOfGridLinesToSkip = tNumberOfGridLinesToSkip >> DSO_SCALE_FACTOR_SHIFT;

    if (MeasurementControl.ChannelIsACMode) {
        // Adjust start OffsetDivCount for AC mode in order to display values at center of screen
        tNumberOfGridLinesToSkip -= (HORIZONTAL_GRID_COUNT / 2);
    } else {
        tNumberOfGridLinesToSkip -= (HORIZONTAL_GRID_COUNT / 2);
        if (tNumberOfGridLinesToSkip < 0) {
            tNumberOfGridLinesToSkip = 0;
        }
    }

    // avoid jitter by not changing number if its delta is only 1
    if (abs(MeasurementControl.OffsetGridCount - tNumberOfGridLinesToSkip) > 1 || tNumberOfGridLinesToSkip == 0) {
        setOffsetGridCount(tNumberOfGridLinesToSkip);
        drawGridLinesWithHorizLabelsAndTriggerLine();
    }
}

/**
 * Real ADC PeakToPeakValue is multiplied by RealAttenuationFactor[MeasurementControl.TotalRangeIndex]
 * to get values of a virtual 18 bit ADC.
 *
 * First check for clipping (check ADC_MAX_CONVERSION_VALUE and also 0 if AC mode)
 * If no clipping occurred, sets range index so that max (virtual) peek to peek value fits into the new range on display.
 * Delay changing of ranges to avoid flickering.
 * Compute new offset if clipping occurs in OFFSET_MODE_AUTOMATIC
 */
void computeAutoRangeAndAutoOffset(void) {
    if (MeasurementControl.RangeAutomatic) {
        if (MeasurementControl.AttenuatorType >= ATTENUATOR_TYPE_ACTIVE_ATTENUATOR) {
            //First check for clipping (check ATTENUATOR_MAX_CONVERSION_VALUE and also 0 if AC mode)
            if (MeasurementControl.RawValueMax >= ATTENUATOR_MAX_CONVERSION_VALUE
                    || (MeasurementControl.ChannelIsACMode && MeasurementControl.RawValueMin == 0)) {
                int i = MeasurementControl.DisplayRangeIndex + 1;
                // Need to change attenuator, so lets find next range with greater attenuation
                while (i < NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR) {
                    if (RawAttenuationFactor[i - 1] != RawAttenuationFactor[i]) {
                        break;
                    }
                    i++;
                }
                setDisplayRange(i); // also adjusts trigger level if needed
                return;
            }
        }

        /*
         * Compute best fitting range
         */
        // get relevant peak2peak value
        int tPeakToPeak = MeasurementControl.RawValueMax;
        if (MeasurementControl.ChannelIsACMode) {
            if (MeasurementControl.RawDSOReadingACZero - MeasurementControl.RawValueMin
                    > MeasurementControl.RawValueMax - MeasurementControl.RawDSOReadingACZero) {
                //difference between zero and min is greater than difference between zero and max => min determines the range
                tPeakToPeak = MeasurementControl.RawDSOReadingACZero - MeasurementControl.RawValueMin;
            } else {
                tPeakToPeak -= MeasurementControl.RawDSOReadingACZero;
            }
            // since tPeakToPeak must fit in half of display
            tPeakToPeak *= 2;
        }

        // get virtual peak2peak value (simulate an virtual 18 bit ADC)
        tPeakToPeak *= RawAttenuationFactor[MeasurementControl.DisplayRangeIndex];

        /*
         * find new range index for offset = 0 Volt
         */
        int tNewRangeIndex;
        for (tNewRangeIndex = 0; tNewRangeIndex < NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR; ++tNewRangeIndex) {
            if (tPeakToPeak <= MaxPeakToPeakValue[tNewRangeIndex]) {
                //actual PeakToPeak fits into display size now
                break;
            }
        }

        if (!MeasurementControl.ChannelHasActiveAttenuator) {
            // clip tNewRangeIndex to allowed values
            if (tNewRangeIndex > NO_ATTENUATOR_MAX_DISPLAY_RANGE_INDEX) {
                tNewRangeIndex = NO_ATTENUATOR_MAX_DISPLAY_RANGE_INDEX;
            } else if (tNewRangeIndex < NO_ATTENUATOR_MIN_DISPLAY_RANGE_INDEX) {
                tNewRangeIndex = NO_ATTENUATOR_MIN_DISPLAY_RANGE_INDEX;
            }
        } else if (tNewRangeIndex < AttenuatorMinimumRange[MeasurementControl.DisplayRangeIndex] - 1) {
            // Decrease range only by one attenuator step
            tNewRangeIndex = AttenuatorMinimumRange[MeasurementControl.DisplayRangeIndex] - 1;
        }

        /*
         * Now we have the new range for offset 0 Volt which determines the attenuator range.
         * In case of offset != 0 Volt check if a lower range -for the same attenuator settings !!!- is more suitable
         */
        if (MeasurementControl.OffsetMode == OFFSET_MODE_AUTOMATIC) {
            tNewRangeIndex = computeAutoOffsetRange(tNewRangeIndex);
        }

        /*
         * wait SCALE_CHANGE_DELAY_MILLIS milliseconds before switch to higher resolution (lower index)
         */
        uint32_t tActualMillis = millis();
        bool setRangeNow = true;
        int tRangeIndexChange = tNewRangeIndex - MeasurementControl.DisplayRangeIndex;
        if (tRangeIndexChange == 0) {
            setRangeNow = false;
        } else if (tRangeIndexChange < 0) {
            if (tActualMillis - MeasurementControl.TimestampLastRangeChange > SCALE_CHANGE_DELAY_MILLIS) {
                // decrease range only if value is not greater than 80% of new range max value to avoid fast switching back
                // We have tPeakToPeak of Offset-0V mode here but for OFFSET_MODE_AUTOMATIC this is already checked :-)
                if ((MeasurementControl.OffsetMode != OFFSET_MODE_AUTOMATIC) && tRangeIndexChange == -1
                        && (tPeakToPeak * 10) / 8 > MaxPeakToPeakValue[tNewRangeIndex]) {
                    setRangeNow = false;
                }
            } else {
                setRangeNow = false;
            }
        }

        if (setRangeNow) {
            setDisplayRange(tNewRangeIndex);
            if (MeasurementControl.OffsetMode == OFFSET_MODE_AUTOMATIC) {
                computeAutoOffset();
            }
            // reset "delay"
            MeasurementControl.TimestampLastRangeChange = tActualMillis;
        }
    }
    if (MeasurementControl.OffsetMode == OFFSET_MODE_AUTOMATIC
            && (MeasurementControl.RawValueMin < MeasurementControl.RawValueOffsetClippingLower
                    || MeasurementControl.RawValueMax > MeasurementControl.RawValueOffsetClippingUpper)) {
        /*
         * Change offset ONLY if display clipping occurs e.g. actual min < actual offset or actual max > (actual offset + value for full scale)
         */
        computeAutoOffset();
    }
}

/**
 * adjusts trigger level if needed
 * sets exclusively: DisplayRangeIndex, DisplayRangeIndexForPrint, actualDSOReadingACZeroForInputRange and hardware attenuator
 * @param aNewRangeIndex
 * @return true if range has changed false otherwise
 */ //
bool setDisplayRange(int aNewRangeIndex) {
    bool tRetValue = true;

    if (aNewRangeIndex == MeasurementControl.DisplayRangeIndex) {
        // no change just set values
        tRetValue = false;
    } else if (!MeasurementControl.ChannelHasActiveAttenuator) {
        // clip value
        if (aNewRangeIndex > NO_ATTENUATOR_MAX_DISPLAY_RANGE_INDEX) {
            aNewRangeIndex = NO_ATTENUATOR_MAX_DISPLAY_RANGE_INDEX;
            tRetValue = false;
        } else if (aNewRangeIndex < NO_ATTENUATOR_MIN_DISPLAY_RANGE_INDEX) {
            aNewRangeIndex = NO_ATTENUATOR_MIN_DISPLAY_RANGE_INDEX;
            tRetValue = false;
        }
    } else if (aNewRangeIndex > NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR - 1) {
        aNewRangeIndex = NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR - 1;
        tRetValue = false;
    } else if (aNewRangeIndex < 0) {
        aNewRangeIndex = 0;
        tRetValue = false;
    }

    float tOldDSORawToVoltFactor = MeasurementControl.actualDSORawToVoltFactor;
    MeasurementControl.DisplayRangeIndex = aNewRangeIndex;
    MeasurementControl.DisplayRangeIndexForPrint = aNewRangeIndex;
    // set attenuator and conversion factor
    if (MeasurementControl.AttenuatorType >= ATTENUATOR_TYPE_ACTIVE_ATTENUATOR) {
        DSO_setAttenuator(AttenuatorHardwareValue[aNewRangeIndex]);
        MeasurementControl.actualDSORawToVoltFactor = sADCToVoltFactor * RawAttenuationFactor[aNewRangeIndex];
    } else if (MeasurementControl.AttenuatorType == ATTENUATOR_TYPE_FIXED_ATTENUATOR
            && MeasurementControl.ADMUXChannel < NUMBER_OF_CHANNELS_WITH_FIXED_ATTENUATOR) {
        MeasurementControl.actualDSORawToVoltFactor = sADCToVoltFactor * FixedAttenuationFactor[MeasurementControl.ADMUXChannel];
        // for display of grid and for precision use other index for print
        MeasurementControl.DisplayRangeIndexForPrint = aNewRangeIndex + (3 * MeasurementControl.ADMUXChannel);
    } else {
        MeasurementControl.actualDSORawToVoltFactor = sADCToVoltFactor;
    }

    if (MeasurementControl.TriggerMode != TRIGGER_MODE_FREE) {
        // adjust trigger raw level if needed
        if (tOldDSORawToVoltFactor != MeasurementControl.actualDSORawToVoltFactor) {
            int tAcCompensation = 0;
            if (MeasurementControl.ChannelIsACMode) {
                tAcCompensation = MeasurementControl.RawDSOReadingACZero;
            }
            // Reuse variable here
            tOldDSORawToVoltFactor = tOldDSORawToVoltFactor / MeasurementControl.actualDSORawToVoltFactor;
            MeasurementControl.RawTriggerLevel = ((MeasurementControl.RawTriggerLevel - tAcCompensation) * tOldDSORawToVoltFactor)
                    + tAcCompensation;
            MeasurementControl.RawTriggerLevelHysteresis = ((MeasurementControl.RawTriggerLevelHysteresis - tAcCompensation)
                    * tOldDSORawToVoltFactor) + tAcCompensation;
        }
    }

    if (MeasurementControl.OffsetMode == OFFSET_MODE_0_VOLT) {
        // adjust raw offset value for ac
        if (MeasurementControl.ChannelIsACMode) {
            MeasurementControl.RawOffsetValueForDisplayRange = getRawOffsetValueFromGridCount(DISPLAY_AC_ZERO_OFFSET_GRID_COUNT);
        }
        // keep labels up to date
        if (MeasurementControl.isRunning) {
            drawGridLinesWithHorizLabelsAndTriggerLine();
        }
    }

    return tRetValue;
}

/**
 * Adds aValue to existing DisplayRangeIndex and checks for bounds, then just calls setDisplayRange()
 * Additionally adjust OffsetGridCount (for offset != 0) so that bottom line will stay.
 *
 * @param aValue to add to MeasurementControl.IndexDisplayRange
 * @return true if no clipping if index occurs
 */
int changeDisplayRangeAndAdjustOffsetGridCount(int aValue) {
    int tFeedbackType = FEEDBACK_TONE_OK;
    float tOldFactor = ScaleVoltagePerDiv[MeasurementControl.DisplayRangeIndex];

    int tNewDisplayRange = MeasurementControl.DisplayRangeIndex + aValue;
    // check for bounds
    if (tNewDisplayRange < 0) {
        tNewDisplayRange = 0;
        tFeedbackType = FEEDBACK_TONE_ERROR;
    } else if (tNewDisplayRange > NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR) {
        tNewDisplayRange = NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR;
        tFeedbackType = FEEDBACK_TONE_ERROR;
    }
    setDisplayRange(tNewDisplayRange);
    // adjust OffsetGridCount so that bottom line will stay
    setOffsetGridCount(MeasurementControl.OffsetGridCount * tOldFactor / ScaleVoltagePerDiv[MeasurementControl.DisplayRangeIndex]);

    return tFeedbackType;
}

/**
 * Handler for offset up / down
 */
int changeOffsetGridCount(int aValue) {
    setOffsetGridCount(MeasurementControl.OffsetGridCount + aValue);
    drawGridLinesWithHorizLabelsAndTriggerLine();
    return FEEDBACK_TONE_OK;
}

/**
 * Sets grid count and depending values and draws new grid and labels
 * @param aOffsetGridCount
 */
void setOffsetGridCount(int aOffsetGridCount) {
    MeasurementControl.OffsetGridCount = aOffsetGridCount;
    //Formula is: MeasurementControl.ValueOffset = tOffsetDivCount * RawPerDiv[tNewRangeIndex];
    MeasurementControl.RawOffsetValueForDisplayRange = getRawOffsetValueFromGridCount(aOffsetGridCount);
    MeasurementControl.RawValueOffsetClippingLower = getInputRawFromDisplayValue(DISPLAY_VALUE_FOR_ZERO); // value for bottom of display
    MeasurementControl.RawValueOffsetClippingUpper = getInputRawFromDisplayValue(0); // value for top of display
}

/**
 * Changes timebase
 * sometimes interrupts just stops after the first reading if we set ADC_SetClockPrescaler value here
 * so delay it to the start of a new acquisition
 *
 * @param aValue
 * @return
 */
int changeTimeBaseValue(int aChangeValue) {
    uint8_t tFeedbackType = FEEDBACK_TONE_OK;
    int tOldIndex = MeasurementControl.TimebaseEffectiveIndex;
// positive value means increment timebase index!
    int tNewIndex = tOldIndex + aChangeValue;
// do not decrement below TIMEBASE_NUMBER_START or increment above TIMEBASE_NUMBER_OF_ENTRIES -1
// skip 1./3 entry(s) because it makes no sense yet (wait until interleaving acquisition is implemented :-))
    if (tNewIndex < TIMEBASE_NUMBER_START) {
        tNewIndex = TIMEBASE_NUMBER_START;
    }
    if (tNewIndex > TIMEBASE_NUMBER_OF_ENTRIES - 1) {
        tNewIndex = TIMEBASE_NUMBER_OF_ENTRIES - 1;
    }
    if (tNewIndex == tOldIndex) {
        tFeedbackType = FEEDBACK_TONE_ERROR;
    } else {
        // signal change to main loop in thread mode
        MeasurementControl.TimebaseNewIndex = tNewIndex;
        MeasurementControl.ChangeRequestedFlags |= CHANGE_REQUESTED_TIMEBASE_FLAG;
    }
    return tFeedbackType;
}

/**
 * Real timebase change is done here (after an acquisition completed or during draw while acquire)
 * Manages also oversampling rate for Min/Max oversampling
 */
void changeTimeBase(void) {
    int tNewIndex = MeasurementControl.TimebaseNewIndex;
    MeasurementControl.TimebaseEffectiveIndex = tNewIndex;

    // Oversample for Min/Max mode
    int tOversampleIndex = tNewIndex;
    bool tOldMode = MeasurementControl.isEffectiveMinMaxMode;
    if (tNewIndex < TIMEBASE_INDEX_CAN_USE_OVERSAMPLING) {
        MeasurementControl.isEffectiveMinMaxMode = false;
    } else {
        MeasurementControl.isEffectiveMinMaxMode = MeasurementControl.isMinMaxMode;
        if (MeasurementControl.isMinMaxMode) {
            tOversampleIndex = TimebaseOversampleIndexForMinMaxMode[tNewIndex];
            MeasurementControl.MinMaxModeTempValuesSize = TimebaseOversampleCountForMinMaxMode[tNewIndex];
        }
    }

    if (MeasurementControl.isRunning && tOldMode && !MeasurementControl.isEffectiveMinMaxMode) {
        // clear old second (min) chart since only first chart is drawn and cleared from now on
        drawDataBuffer(NULL, REMOTE_DISPLAY_WIDTH, DisplayControl.EraseColor, 0, DRAW_MODE_CLEAR_OLD_MIN,
                MeasurementControl.isEffectiveMinMaxMode);
    }

    ADC_SetTimerPeriod(TimebaseTimerDividerValues[tOversampleIndex], TimebaseTimerPrescalerDividerValues[tOversampleIndex]);
    // so set matching sampling times for channel
    ADC_SelectChannelAndSetSampleTime(&ADC1Handle, ADCInputMUXChannels[MeasurementControl.ADMUXChannel],
            (tOversampleIndex < TIMEBASE_FAST_MODES));

    // stop and start is really needed :-(
    // first disable ADC otherwise sometimes interrupts just stop after the first reading
    ADC_disableAndWait (&ADC1Handle);
#ifdef STM32F30X
    ADC12_SetClockPrescaler(ADCClockPrescalerValues[tOversampleIndex]);
#endif
    ADC_enableAndWait(&ADC1Handle);

    // Xscale - no oversampling for this modes :-)
    if (tNewIndex < TIMEBASE_NUMBER_OF_XSCALE_CORRECTION) {
        DisplayControl.XScale = xScaleForTimebase[tNewIndex];
    } else {
        DisplayControl.XScale = 0;
    }

    DataBufferControl.DrawWhileAcquire = (tNewIndex >= TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE);
    MeasurementControl.TriggerTimeoutSampleOrLoopCount = computeNumberOfSamplesToTimeout(tNewIndex);
    printInfo();
}

void setOffsetGridCountAccordingToACMode(void) {
    if (MeasurementControl.ChannelIsACMode) {
        // Zero line is at grid 3 if ACRange == true
        setOffsetGridCount (DISPLAY_AC_ZERO_OFFSET_GRID_COUNT);
    } else {
        setOffsetGridCount(0);
    }
}
/**
 * Sets all channel as well as all range values and attached attenuator
 * @param aChannelIndex - Index into ADCInputMUXChannels array
 * @return true - If ChannelIsACMode has changed and page must be redrawn
 */
void setChannel(uint8_t aChannelIndex) {

    int tNewRange = NO_ATTENUATOR_MAX_DISPLAY_RANGE_INDEX;
    if (aChannelIndex == 0) {
        /*
         * Set Channel related flags
         */
        MeasurementControl.ChannelHasAC_DCSwitch = true;
        // restore user AC mode
        MeasurementControl.ChannelIsACMode = MeasurementControl.isACMode;
        if (MeasurementControl.AttenuatorType >= ATTENUATOR_TYPE_ACTIVE_ATTENUATOR) {
            MeasurementControl.ChannelHasActiveAttenuator = true;
            // start with high attenuation
            tNewRange = NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR - 1;
        }

    } else if (aChannelIndex >= MeasurementControl.FirstChannelIndexWithoutAttenuator) {
        /*
         * SHandle no-attenuator channels
         */
        MeasurementControl.ChannelHasActiveAttenuator = false;
        MeasurementControl.ChannelHasAC_DCSwitch = false;
        MeasurementControl.ChannelIsACMode = false;

    } else if (MeasurementControl.AttenuatorType == ATTENUATOR_TYPE_FIXED_ATTENUATOR
            && aChannelIndex < NUMBER_OF_CHANNELS_WITH_FIXED_ATTENUATOR) {
        MeasurementControl.ChannelHasAC_DCSwitch = true;
        MeasurementControl.ChannelIsACMode = MeasurementControl.isACMode;
    }
    MeasurementControl.ADMUXChannel = aChannelIndex;
    ADC_SelectChannelAndSetSampleTime(&ADC1Handle, ADCInputMUXChannels[MeasurementControl.ADMUXChannel],
            MeasurementControl.TimebaseFastDMAMode);
    setDisplayRange(tNewRange); // calls in turn DSO_setAttenuator() and needs ADMUXChannel
}

/**
 * adjust (copy around) the cyclic pre trigger buffer (320/2 Values) in order to have them at linear time
 * app. 100 us
 * tTempBuffer size must be at least sizeof(uint16_t)*DATABUFFER_PRE_TRIGGER_SIZE
 */
void adjustPreTriggerBuffer(void) {
// align pre trigger buffer
    int tCount = 0;
    uint16_t *tDestPtr;
    uint16_t *tSrcPtr;
    if ((DataBufferControl.DataBufferPreTriggerNextPointer == &DataBufferControl.DataBuffer[0])
            || MeasurementControl.TriggerMode == TRIGGER_MODE_FREE) {
        return;
    }

    tCount = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE] - DataBufferControl.DataBufferPreTriggerNextPointer;

    /*
     * If Modus is DrawWhileAcquire and pre trigger buffer was only written once,
     * (since trigger condition was met before buffer wrap around)
     * then the tail buffer region from last pre trigger value to end of pre trigger region is invalid.
     */ //
    bool tLastPretriggerRegionInvalid = (DataBufferControl.DrawWhileAcquire
            && MeasurementControl.TriggerSampleCount < DATABUFFER_PRE_TRIGGER_SIZE);
    bool tIsEffectiveMinMaxMode = MeasurementControl.isEffectiveMinMaxMode;
    if (!tLastPretriggerRegionInvalid) {
// 1. shift region from last pre trigger value to end of pre trigger region to start of buffer. Use temp buffer (DisplayLineBuffer)
        tDestPtr = (uint16_t*) TempBufferForPreTriggerAdjustAndFFT;
        tSrcPtr = DataBufferControl.DataBufferPreTriggerNextPointer;
        memcpy(tDestPtr, tSrcPtr, tCount * sizeof(*tDestPtr));
        if (tIsEffectiveMinMaxMode) {
            memcpy(tDestPtr + DATABUFFER_PRE_TRIGGER_SIZE, tSrcPtr + DATABUFFER_MIN_OFFSET, tCount * sizeof(*tDestPtr));
        }
    }
// 2. shift region from beginning to last pre trigger value to end of pre trigger region
    tDestPtr = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE - 1];
    tSrcPtr = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE - tCount - 1];
//	this does not work :-(  memmove(tDestPtr, tSrcPtr, (DATABUFFER_PRE_TRIGGER_SIZE - tCount) * sizeof(*tDestPtr));
    for (int i = DATABUFFER_PRE_TRIGGER_SIZE - tCount; i > 0; --i) {
        *tDestPtr = *tSrcPtr;
        if (tIsEffectiveMinMaxMode) {
            *(tDestPtr + DATABUFFER_MIN_OFFSET) = *(tSrcPtr + DATABUFFER_MIN_OFFSET);
        }
        tDestPtr--;
        tSrcPtr--;
    }
    tDestPtr++;
// 3. copy temp buffer to start of pre trigger region
    tSrcPtr = (uint16_t*) TempBufferForPreTriggerAdjustAndFFT;
    tDestPtr = DataBufferControl.DataBuffer;
    if (tLastPretriggerRegionInvalid) {
// set invalid values in pretrigger area to special value
        for (int i = tCount; i > 0; --i) {
            *tDestPtr = DATABUFFER_INVISIBLE_RAW_VALUE;
            if (tIsEffectiveMinMaxMode) {
                *(tDestPtr + DATABUFFER_MIN_OFFSET) = DATABUFFER_INVISIBLE_RAW_VALUE;
            }
            tDestPtr++;
        }
// memset can only set bytes :-(  memset(tDestPtr, DATABUFFER_INVISIBLE_RAW_VALUE, tCount * sizeof(*tDestPtr));
    } else {
        memcpy(tDestPtr, tSrcPtr, tCount * sizeof(*tDestPtr));
        if (tIsEffectiveMinMaxMode) {
            memcpy(tDestPtr + DATABUFFER_MIN_OFFSET, tSrcPtr + DATABUFFER_PRE_TRIGGER_SIZE, tCount * sizeof(*tDestPtr));
        }
    }
}

/**
 * 3ms for FFT with -OS
 */
float32_t* computeFFT(uint16_t *aDataBufferPointer) {
//    arm_cfft_radix4_instance_f32 tFFTControlStruct;
    int i;

    uint32_t tTime = millis();

// initialize FFT input array
    float32_t *tFFTBufferPointer = (float32_t*) TempBufferForPreTriggerAdjustAndFFT;
    for (i = 0; i < FFT_SIZE; ++i) {
        *tFFTBufferPointer++ = getFloatFromRawValue(*aDataBufferPointer++);
        *tFFTBufferPointer++ = 0.0;
    }

    tFFTBufferPointer = (float32_t*) TempBufferForPreTriggerAdjustAndFFT;

// saves 33848 bytes code
    /* Process the data through the CFFT/CIFFT module */
// arm_cfft_radix4_init_f32(&tFFTControlStruct, FFT_SIZE, 0, 1);
// arm_cfft_radix4_f32(&tFFTControlStruct, tFFTBufferPointer);
    /*  Complex FFT radix-4  */
    arm_radix4_butterfly_f32(tFFTBufferPointer, FFT_SIZE, (float32_t*) twiddleCoef_256, 1);
    /*  Bit Reversal */
//    arm_bitreversal_f32(tFFTBufferPointer, FFT_SIZE, 16u, (uint16_t *) &armBitRevTable[15]);
    arm_bitreversal_f32(tFFTBufferPointer, FFT_SIZE, 1, (uint16_t*) &armBitRevTable_256[0]);

    tFFTBufferPointer = &((float32_t*) TempBufferForPreTriggerAdjustAndFFT)[2]; // skip DC value
    float32_t *tOutBuffer = (float32_t*) TempBufferForPreTriggerAdjustAndFFT;
    float32_t tRealValue, tImaginaryValue;
    float tMaxValue = 0.0;
    int tMaxIndex = 0;

    /*
     * convert to magnitude values and store it in first half of buffer
     */
    *tOutBuffer++ = 0; // set DC to zero so it does not affect the scaling
// use only the first half of bins
    for (i = 1; i < FFT_SIZE / 2; ++i) {
        /*
         * in place conversion
         */
        tRealValue = *tFFTBufferPointer++;
        tImaginaryValue = *tFFTBufferPointer++;
        tRealValue = sqrtf((tRealValue * tRealValue) + (tImaginaryValue * tImaginaryValue));
        // find max bin value for scaling and frequency display
        if (tRealValue > tMaxValue) {
            tMaxValue = tRealValue;
            tMaxIndex = i;
        }
        *tOutBuffer++ = tRealValue;
    }
    FFTInfo.MaxValue = tMaxValue;
    FFTInfo.MaxIndex = tMaxIndex;
    FFTInfo.TimeElapsedMillis = millis() - tTime;

    return (float32_t*) TempBufferForPreTriggerAdjustAndFFT;
}

#endif // _TOUCH_DSO_AQUISITION_HPP
