/**
 * TouchDSO.h
 *
 * @date 20.12.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 *      Features:
 *      No dedicated hardware, just off the shelf components + c software
 *      --- 3 MSamples per second
 *      Automatic or manual range, trigger and offset value selection
 *      --- currently 10 screens data buffer
 *      Min, max, peak to peak and average display
 *      All settings can be changed during measurement by touching the (invisible) buttons
 *      --- 3 external + 3 internal channels (VBatt/2, VRefint + Temp) selectable
 *      Single shot function
 *      Display of pre trigger values
 *
 *      Build on:
 *      STM32F3-DISCOVERY http://www.watterott.com/de/STM32F3DISCOVERY      16 EUR
 *      HY32D LCD Display http://www.ebay.de/itm/250906574590               12 EUR
 *      Total                                                               28 EUR
 *
 */

#ifndef SIMPLETOUCHSCREENDSO_H_
#define SIMPLETOUCHSCREENDSO_H_

#include "BlueDisplay.h"
#include "BDButton.h"
#ifdef LOCAL_DISPLAY_EXISTS
#include "ADS7846.h"
#endif

#include <stdint.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <arm_math.h> // for float32_t
#pragma GCC diagnostic pop

/*
 * COLORS
 */
#define COLOR_BACKGROUND_DSO COLOR_WHITE

// Button colors
#define COLOR_GUI_CONTROL COLOR_RED
#define COLOR_GUI_TRIGGER COLOR_BLUE
#define COLOR_GUI_DISPLAY_CONTROL COLOR_YELLOW
#define COLOR_GUI_SOURCE_TIMEBASE RGB(0x00,0x90,0x00)

// Data colors
#define COLOR_DATA_RUN COLOR_BLUE
#define COLOR_DATA_RUN_CLIPPING COLOR_RED
#define COLOR_DATA_TRIGGER COLOR_BLACK      // color for trigger status line
#define COLOR_DATA_PRETRIGGER COLOR_GREEN   // color for pre trigger data in draw while acquire mode
#define COLOR_FFT_DATA COLOR_BLUE
#define COLOR_DATA_HOLD COLOR_RED
#define COLOR_GRID_LINES RGB(0x00,0x98,0x00)
#define COLOR_INFO_BACKGROUND RGB(0xC8,0xC8,0x00)

// Data history colors
#define COLOR_DATA_ERASE_LOW RGB(0xC0,0xFF,0xC0)
#define COLOR_DATA_ERASE_MID RGB(0x80,0xE0,0x80)
#define COLOR_DATA_ERASE_HIGH RGB(0x40,0xC0,0x40)

//Line colors
#define COLOR_VOLTAGE_PICKER COLOR_YELLOW
#define COLOR_VOLTAGE_PICKER_SLIDER RGB(0xFF,0XFF,0xE0) // Light Yellow
#define COLOR_TRIGGER_LINE COLOR_MAGENTA
#define COLOR_TRIGGER_SLIDER RGB(0xFF,0XF0,0xFF) // light Magenta

#define COLOR_MAX_MIN_LINE COLOR_GREEN
#define COLOR_HOR_GRID_LINE_LABEL COLOR_BLUE
#define COLOR_HOR_GRID_LINE_LABEL_NEGATIVE COLOR_RED

/*
 * DISPLAY LAYOUT
 */
#define DSO_DISPLAY_HEIGHT 240
#define DSO_DISPLAY_WIDTH 320
#define HORIZONTAL_GRID_COUNT 6
#define HORIZONTAL_GRID_HEIGHT (DSO_DISPLAY_HEIGHT / HORIZONTAL_GRID_COUNT) // 40
#define TIMING_GRID_WIDTH (DSO_DISPLAY_WIDTH / 10) // 32
#define DISPLAY_AC_ZERO_OFFSET_GRID_COUNT (-3)
#define DISPLAY_VALUE_FOR_ZERO (DSO_DISPLAY_HEIGHT - 2) // Zero line is not exactly at bottom of display to improve readability
#define PIXEL_AFTER_LABEL 2 // Space between end of label and display border

/*
 * POSITIONS + SIZES
 */
#ifdef LOCAL_DISPLAY_EXISTS
#define FONT_SIZE_INFO_SHORT        TEXT_SIZE_11    // for 1 line info
#define FONT_SIZE_INFO_LONG         TEXT_SIZE_11    // for 3 lines info
#define FONT_SIZE_INFO_SHORT_ASC    TEXT_SIZE_11_ASCEND    // for 3 lines info
#define FONT_SIZE_INFO_LONG_ASC     TEXT_SIZE_11_ASCEND    // for 3 lines info
#define FONT_SIZE_INFO_LONG_WIDTH   TEXT_SIZE_11_WIDTH    // for 3 lines info
#else
#define FONT_SIZE_INFO_SHORT        TEXT_SIZE_16    // for 1 line info
#define FONT_SIZE_INFO_LONG         TEXT_SIZE_14    // for 3 lines info
#define FONT_SIZE_INFO_SHORT_ASC    TEXT_SIZE_16_ASCEND    // for 3 lines info
#define FONT_SIZE_INFO_LONG_ASC     TEXT_SIZE_14_ASCEND    // for 3 lines info
#define FONT_SIZE_INFO_LONG_WIDTH   TEXT_SIZE_14_WIDTH    // for 3 lines info
#endif

#define SLIDER_SIZE 24
#define SLIDER_VPICKER_POS_X        0 // Position of slider
#define SLIDER_VPICKER_INFO_X       (SLIDER_VPICKER_POS_X + SLIDER_SIZE)
#define SLIDER_VPICKER_INFO_SHORT_Y (FONT_SIZE_INFO_SHORT + FONT_SIZE_INFO_SHORT_ASC)
#define SLIDER_VPICKER_INFO_LONG_Y  (3 * FONT_SIZE_INFO_LONG + FONT_SIZE_INFO_SHORT_ASC) // since font size is always 18

#define SLIDER_TLEVEL_POS_X         (14 * FONT_SIZE_INFO_LONG_WIDTH) // Position of slider
#define TRIGGER_LEVEL_INFO_SHORT_X  (SLIDER_TLEVEL_POS_X  + SLIDER_SIZE)
#ifdef LOCAL_DISPLAY_EXISTS
#define TRIGGER_LEVEL_INFO_LONG_X   (11 * FONT_SIZE_INFO_LONG_WIDTH)
#else
#define TRIGGER_LEVEL_INFO_LONG_X   (11 * FONT_SIZE_INFO_LONG_WIDTH +1) // +1 since we have a special character in the string before
#endif
#define TRIGGER_LEVEL_INFO_SHORT_Y  (FONT_SIZE_INFO_SHORT + FONT_SIZE_INFO_SHORT_ASC)
#define TRIGGER_LEVEL_INFO_LONG_Y   (2 * FONT_SIZE_INFO_LONG  + FONT_SIZE_INFO_LONG_ASC)

#define TRIGGER_HIGH_DISPLAY_OFFSET 7 // for trigger state line

/*
 * CONTROL Definitions
 */
/*
 * DATA BUFFER
 */
#ifdef STM32F303xC
#define DATABUFFER_SIZE_FACTOR 10
#else
#define DATABUFFER_SIZE_FACTOR 7
#endif
#define DATABUFFER_DISPLAY_RESOLUTION_FACTOR 10
#define DATABUFFER_DISPLAY_RESOLUTION (DSO_DISPLAY_WIDTH / DATABUFFER_DISPLAY_RESOLUTION_FACTOR)     // Base value for other (32)
#define DATABUFFER_DISPLAY_INCREMENT DATABUFFER_DISPLAY_RESOLUTION // increment value for display scroll
#define DATABUFFER_SIZE (DSO_DISPLAY_WIDTH * DATABUFFER_SIZE_FACTOR) // * 4 = bytes RAM needed
#define DATABUFFER_MIN_OFFSET DATABUFFER_SIZE // DataBufferMinValues[0] - DataBuffer[0]
#define DATABUFFER_PRE_TRIGGER_SIZE (5 * DATABUFFER_DISPLAY_RESOLUTION)
extern unsigned int sDatabufferPreDisplaySize;
#define DATABUFFER_DISPLAY_START (DATABUFFER_PRE_TRIGGER_SIZE - DisplayControl.DatabufferPreTriggerDisplaySize)
#define DATABUFFER_DISPLAY_END (DATABUFFER_DISPLAY_START + DSO_DISPLAY_WIDTH - 1)
#define DATABUFFER_POST_TRIGGER_START (&DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE])
#define DATABUFFER_POST_TRIGGER_SIZE (DATABUFFER_SIZE - DATABUFFER_PRE_TRIGGER_SIZE)
#define DATABUFFER_INVISIBLE_RAW_VALUE 0x1000 // Value for invalid data in/from pretrigger area
#define DISPLAYBUFFER_INVISIBLE_VALUE 0xFF // Value for invisible data in display buffer. Used if raw value was DATABUFFER_INVISIBLE_RAW_VALUE
#define DMA_TEMP_BUFFER_MAX_SIZE 1000

#define DRAW_MODE_REGULAR 0 // DataBuffer is drawn result is in DisplayBuffer (+ DisplayBufferMin)
#define DRAW_MODE_CLEAR_OLD 1 // DisplayBuffer is taken and cleared
#define DRAW_MODE_CLEAR_OLD_MIN 2 // DisplayBufferMin is taken and cleared

/*
 * TIMEBASE
 */
#define CHANGE_REQUESTED_TIMEBASE 0x01
#define TIMEBASE_FAST_MODES 7 // first modes are fast DMA modes
#define TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE 17 // min index where chart is drawn while buffer is filled
#define TIMEBASE_INDEX_CAN_USE_OVERSAMPLING 11 // min index where Min/Max oversampling is enabled
#define TIMEBASE_NUMBER_OF_ENTRIES 21 // the number of different timebase provided - 1. entry is not uses until interleaved acquisition is implemented
#define TIMEBASE_NUMBER_OF_EXCACT_ENTRIES 8 // the number of exact float value for timebase because of granularity of clock division
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
extern const uint8_t xScaleForTimebase[TIMEBASE_NUMBER_OF_XSCALE_CORRECTION];
extern const uint16_t TimebaseDivValues[TIMEBASE_NUMBER_OF_ENTRIES];
extern const float TimebaseExactDivValuesMicros[TIMEBASE_NUMBER_OF_EXCACT_ENTRIES];
extern const uint16_t TimebaseTimerDividerValues[TIMEBASE_NUMBER_OF_ENTRIES];
extern const uint16_t TimebaseTimerPrescalerDividerValues[TIMEBASE_NUMBER_OF_ENTRIES];
extern const uint16_t TimebaseOversampleCountForMinMaxMode[TIMEBASE_NUMBER_OF_ENTRIES];
extern const uint16_t TimebaseOversampleIndexForMinMaxMode[TIMEBASE_NUMBER_OF_ENTRIES];
extern const uint16_t ADCClockPrescalerValues[TIMEBASE_NUMBER_OF_ENTRIES];

/*
 * TRIGGER
 */
// Enum of TriggerMode
#define TRIGGER_MODE_AUTOMATIC 0
#define TRIGGER_MODE_MANUAL 1
#define TRIGGER_MODE_OFF 2

#define TRIGGER_HYSTERESIS_MANUAL 2 // value for effective trigger hysteresis in manual trigger mode

// States of tTriggerStatus
#define TRIGGER_START 0 // No trigger condition met
#define TRIGGER_BEFORE_THRESHOLD 1 // slope condition met, wait to go beyond threshold hysteresis
#define TRIGGER_OK 2 // Trigger condition met
#define PHASE_PRE_TRIGGER 0 // load pre trigger values
#define PHASE_SEARCH_TRIGGER 1 // wait for trigger condition
#define PHASE_POST_TRIGGER 2 // trigger found -> acquire data
#define TRIGGER_TIMEOUT_MILLIS 200 // Milliseconds to wait for trigger
#define TRIGGER_TIMEOUT_MIN_SAMPLES (6 * TIMING_GRID_WIDTH) // take at least this amount of samples to find trigger

/*
 * OFFSET
 */
#define OFFSET_MODE_0_VOLT 0
#define OFFSET_MODE_AUTOMATIC 1
#define OFFSET_MODE_MANUAL 2    // Implies range mode manual.

/*
 * RANGE (+ attenuator)
 */
#define NUMBER_OF_DISPLAY_RANGES_WITHOUT_ATTENUATOR 6 // from 0.06V total - 0,02V to 3V total - 0.5V/div (div=40 pixel)
#define NO_ATTENUATOR_MIN_DISPLAY_RANGE_INDEX 1
#define NO_ATTENUATOR_MAX_DISPLAY_RANGE_INDEX 5

#define NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR (NUMBER_OF_DISPLAY_RANGES_WITHOUT_ATTENUATOR + 6)  // up to 120V total - 40V/div
//#define NUMBER_OF_HARDWARE_RANGES 5  // different hardware attenuator levels = *2 , *1 , /4, /40, /100

/*
 * EXTERNAL ATTENUATOR
 */
#define ATTENUATOR_TYPE_NO_ATTENUATOR 0
#define ATTENUATOR_TYPE_FIXED_ATTENUATOR 1  // assume manual AC/DC switch
#define NUMBER_OF_CHANNEL_WITH_FIXED_ATTENUATOR 3 // Channel0 = /1, Ch1= /10, Ch2= /100

#define ATTENUATOR_TYPE_ACTIVE_ATTENUATOR 2 // and 3
#define NUMBER_OF_CHANNEL_WITH_ACTIVE_ATTENUATOR 2

//#define DSO_ATTENUATOR_BASE_GAIN 2 // Gain if attenuator is at level 1
#define ACTIVE_ATTENUATOR_INFINITE_VALUE 0 // value for which attenuator shortens the input

// the maximum raw value if attenuator output amplifier cannot reach VRefint (3V)
//
// 2,13V TL082 with USB 4.0V
// 2.93V TL062 with USB 4.2V
//  external 4.75V
// 2,74V TL082
// 2.99V TL062
#ifdef STM32F303xC
#define ATTENUATOR_MAX_CONVERSION_VALUE 3950 // 3754 = 2.75V 3959 = 2.9V
#else
#define ATTENUATOR_MAX_CONVERSION_VALUE ADC_MAX_CONVERSION_VALUE
#endif

// Range declarations
extern const float ScaleVoltagePerDiv[NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR];
extern const uint8_t RangePrecision[NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR];
#define DSO_SCALE_FACTOR_SHIFT ADC_SCALE_FACTOR_SHIFT  // =18 - 2**18 = 0x40000 or 262144
extern int ScaleFactorRawToDisplayShift18[NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR];
extern float actualDSORawToVoltFactor;
// Attenuator declarations
extern float RawAttenuationFactor[NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR];
extern float FixedAttenuationFactor[NUMBER_OF_CHANNEL_WITH_FIXED_ATTENUATOR];
extern const uint8_t AttenuatorHardwareValue[NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR];
extern uint16_t RawDSOReadingACZero;

/*
 * CHANNEL
 */
#define START_ADC_CHANNEL_INDEX 0  // see also ChannelSelectButtonString
#ifdef STM32F303xC
#define ADC_CHANNEL_COUNT 6 // The number of ADC channel
#else
#define ADC_CHANNEL_COUNT 6 // The number of ADC channel
#endif
extern const char * const ADCInputMUXChannelStrings[ADC_CHANNEL_COUNT];
extern char ADCInputMUXChannelChars[ADC_CHANNEL_COUNT];
extern uint8_t const ADCInputMUXChannels[ADC_CHANNEL_COUNT];

/*
 * FFT
 */
#define FFT_SIZE 256
extern uint8_t DisplayBufferFFT[FFT_SIZE / 2];

/*
 * STRUCTURES
 */
struct MeasurementControlStruct {
    bool isRunning;
    volatile uint8_t ChangeRequestedFlags; // GUI (Event) -> Thread (main loop) - change of clock prescaler requested from GUI
    volatile bool StopRequested; // GUI -> Thread
    volatile bool StopAcknowledged; // true if DMA made its last acquisition before stop

    // Input select
#ifdef LOCAL_DISPLAY_EXISTS
    bool ADS7846ChannelsAsDatasource;
#endif

    // Read phase for ISR and single shot mode see SEGMENT_...
    volatile uint8_t TriggerActualPhase; // ADC-ISR internal and -> Thread
    volatile bool isSingleShotMode; // GUI
    volatile bool doPretriggerCopyForDisplay; // signal from loop to DMA ISR to copy the pre trigger area for display - useful for single shot

    // Trigger
    volatile bool TriggerPhaseJustEnded; // ADC-ISR -> Thread - signal for draw while acquire
    volatile bool TriggerSlopeRising; // GUI -> ADC-ISR
    volatile uint16_t RawTriggerLevel; // GUI -> ADC-ISR
    uint16_t RawTriggerLevelHysteresis; // ADC-ISR internal

    uint8_t TriggerMode; // GUI -> ADC-ISR - TRIGGER_MODE_AUTOMATIC, MANUAL, OFF
    uint8_t TriggerStatus; // Set by ISR: see TRIGGER_START etc.
    uint16_t TriggerSampleCount; // ISR: for checking trigger timeout
    uint16_t TriggerTimeoutSampleOrLoopCount; // ISR max samples / DMA max number of loops before trigger timeout
    uint16_t RawValueBeforeTrigger; // only single shot mode: to show actual value during wait for trigger

    bool isMinMaxMode;          // DMA oversampling
    bool isEffectiveMinMaxMode; // =(isMinMaxMode && TimebaseEffectiveIndex >= TIMEBASE_INDEX_CAN_USE_OVERSAMPLING)
    uint16_t MinMaxModeTempValuesSize;  // Number of oversample for one display value
    uint16_t MinMaxModeMaxValue;
    uint16_t MinMaxModeMinValue;

    // computed values from display buffer
    float PeriodMicros;
    uint32_t FrequencyHertz;
    float FrequencyHertzAtMaxFFTBin;
    float MaxFFTValue;

    // Statistics (for auto range/offset/trigger)
    uint16_t RawValueMin;
    uint16_t RawValueMax;
    uint16_t RawValueAverage;

    // Timebase
    bool TimebaseFastDMAMode;
    int8_t TimebaseNewIndex; // set by touch handler
    int8_t TimebaseEffectiveIndex;  // = (TimebaseADCIndex * Oversample count) if Min/Max oversampling enabled
    int8_t TimebaseADCIndex; // Timebase for ADC

    // Channel
    uint8_t ADCInputMUXChannelIndex;

    // Range
    bool RangeAutomatic; // [RANGE_MODE_AUTOMATIC, MANUAL]
    int DisplayRangeIndex; // index including attenuator ranges  [0 to NUMBER_OF_RANGES_WITH_ACTIVE_ATTENUATOR]
    int DisplayRangeIndexForPrint; // Only for ATTENUATOR_TYPE_FIXED_ATTENUATOR other (+3,+6) other than DisplayRangeIndex.
    float actualDSORawToVoltFactor; // used for getFloatFromRawValue() and for changeInputRange()
    uint32_t TimestampLastRangeChange;

    // Attenuator
    uint8_t AttenuatorType; // ATTENUATOR_TYPE_NO_ATTENUATOR, ATTENUATOR_TYPE_FIXED_ATTENUATOR, ATTENUATOR_TYPE_ACTIVE_ATTENUATOR
    uint8_t FirstChannelIndexWithoutAttenuator; // ATTENUATOR_TYPE_NO_ATTENUATOR -> 0, ATTENUATOR_TYPE_SIMPLE_ATTENUATOR -> 2, ...
    bool ChannelHasActiveAttenuator; // actual channel has active attenuator attached

    // AC / DC Switch
    bool ChannelHasACDCSwitch; // has AC / DC switch - only for channels with active or passive attenuators
    bool ChannelIsACMode; // actual AC Mode for actual channel
    bool isACMode; // user AC mode setting false: unipolar mode => 0V probe input -> 0V ADC input  - true: AC range => 0V probe input -> 1.5V ADC input
    uint16_t RawDSOReadingACZero;

    // Offset
    // Offset in ADC Reading is multiple of reading / div
    uint8_t OffsetMode; //OFFSET_MODE_0_VOLT, OFFSET_MODE_AUTOMATIC, OFFSET_MODE_MANUAL
    signed short RawOffsetValueForDisplayRange; // to be subtracted from adjusted (by FactorFromInputToDisplayRange) raw value
    uint16_t RawValueOffsetClippingLower; // ADC raw lower clipping value for offset mode
    uint16_t RawValueOffsetClippingUpper; // ADC raw upper clipping value for offset mode

    int16_t OffsetGridCount; // number of lowest horizontal grid to display for auto offset
};
extern struct MeasurementControlStruct MeasurementControl;

/*
 * Data buffer
 */
struct DataBufferStruct {
    volatile bool DataBufferFull; // ISR -> main loop
    bool DrawWhileAcquire;
    volatile bool DataBufferPreTriggerAreaWrapAround; // ISR -> draw-while-acquire mode

    uint16_t * DataBufferPreTriggerNextPointer; // pointer to next pre trigger value in DataBuffer - set only once at end of search trigger phase
    uint16_t * DataBufferNextInPointer; // used by ISR as main databuffer pointer - also read by draw-while-acquire mode
    volatile uint16_t * DataBufferNextDrawPointer; // for draw-while-acquire mode
    uint16_t NextDrawXValue; // for draw-while-acquire mode
    // to detect end of acquisition in interrupt service routine
    volatile uint16_t * DataBufferEndPointer; // pointer to last valid data in databuffer | Thread -> ISR (for stop)

    // Pointer for horizontal scrolling - use value 2 divs before trigger point to show pre trigger values
    uint16_t * DataBufferDisplayStart;
    /**
     * consists of 2 regions - first pre trigger region, second data region
     * display region starts in pre trigger region
     */
    uint16_t DataBuffer[DATABUFFER_SIZE];
    uint16_t DataBufferMinValues[DATABUFFER_SIZE];
    uint16_t DataBufferTempDMAValues[DMA_TEMP_BUFFER_MAX_SIZE];
};
extern struct DataBufferStruct DataBufferControl;
extern void * TempBufferForPreTriggerAdjustAndFFT;

/*
 * Display control
 * while running switch between upper info line on/off
 * while stopped switch between chart / t+info line and gui
 */

enum DisplayPageEnum {
    START, CHART, SETTINGS, MORE_SETTINGS
#ifndef STM32F303xC
    , FREQ_SYNTH, SYST_INFO
#endif
};

#define SCALE_CHANGE_DELAY_MILLIS 2000
#define DRAW_HISTORY_LEVELS 4 // 0 = No history, 3 = history high
// modes for showInfoMode
#define INFO_MODE_NO_INFO 0
#define INFO_MODE_SHORT_INFO 1
#define INFO_MODE_LONG_INFO 2
struct DisplayControlStruct {
    uint8_t TriggerLevelDisplayValue; // For clearing old line of manual trigger level setting
#ifdef LOCAL_DISPLAY_EXISTS
    bool drawPixelMode;
#endif
    bool showTriggerInfoLine;
    uint8_t showInfoMode;
    DisplayPageEnum DisplayPage; // START, CHART, SETTINGS, MORE_SETTINGS
    bool ShowFFT;

    /**
     * XScale > 1 : expansion by factor XScale
     * XScale == 1 : expansion by 1.5
     * XScale == 0 : identity
     * XScale == -1 : compression by 1.5
     * XScale < -1 : compression by factor -XScale
     */
    int8_t XScale; // Factor for X Data expansion(>0) or compression(<0). 2->display 1 value 2 times -2->display average of 2 values etc.
    uint16_t DisplayIncrementPixel; // corresponds to XScale

    unsigned int DatabufferPreTriggerDisplaySize;

    int LastDisplayRangeIndex;

    // for recognizing that MeasurementControl values changed => clear old grid labels
    int16_t LastOffsetGridCount;

    Color_t EraseColors[DRAW_HISTORY_LEVELS];
    Color_t EraseColor;
    uint8_t EraseColorIndex;
};
extern DisplayControlStruct DisplayControl;

struct FFTInfoStruct {
    float MaxValue; // max bin value for y scaling
    int MaxIndex;   // index of MaxValue
    uint32_t TimeElapsedMillis; // milliseconds of computing last fft
};
extern FFTInfoStruct FFTInfo;
extern uint8_t DisplayBuffer[DSO_DISPLAY_WIDTH];
extern uint8_t DisplayBufferMin[DSO_DISPLAY_WIDTH];

/*
 * BSS memory usage total: 12064 byte
 *   60 2 Slider
 *   48 ScaleFactorRawToDisplayShift18[]
 *   48 MaxPeakToPeakValue[]
 *   88 MeasurementControl
 * 1088 4 Display Buffer
 *10468 DataBufferControl
 */
/*******************************************************************************************
 * Function declaration section
 *******************************************************************************************/
void initDSOPage(void);
void startDSOPage(void);
void loopDSOPage(void);
void stopDSOPage(void);

void autoACZeroCalibration(void);
void resetAcquisition(void);
void initAcquisition(void);
void startAcquisition(void);
void readADS7846Channels(void);

void changeTimeBase(void);

void initRawToDisplayFactorsAndMaxPeakToPeakValues(void);
void setOffsetGridCount(int aOffsetGridCount);
void computeAutoTrigger(void);
void computeAutoRangeAndAutoOffset(void);

void computePeriodFrequency(void);

void setTriggerLevelAndHysteresis(int aRawTriggerValue, int aRawTriggerHysteresis);
bool changeInputRange(int aValue);
int changeDisplayRange(int aValue);
bool setDisplayRange(int aNewDisplayRangeIndex, bool aClipToIndexInputRange);
void adjustPreTriggerBuffer(void);
uint16_t computeNumberOfSamplesToTimeout(int8_t aTimebaseIndex);
void computeMinMaxAverageAndPeriodFrequency(void);
bool setDisplayRange(int aNewRangeIndex);
void setOffsetGridCountAccordingToACMode(void);
void setACMode(bool aACRangeEnable);
void setChannel(int aChannel);

void drawGridLinesWithHorizLabelsAndTriggerLine(Color_t aColor);
void drawMinMaxLines(void);

void drawTriggerLine(void);
void clearTriggerLine(uint8_t aTriggerLevelDisplayValue);
void printTriggerInfo(void);

void printInfo(void);
void clearInfo(void);
void drawDataBuffer(uint16_t *aDataBufferPointer, int aLength, Color_t aColor, Color_t aClearBeforeColor, int aDrawMode, bool aDrawAlsoMin);
void drawRemainingDataBufferValues(Color_t aDrawColor);

void initScaleValuesForDisplay(void);
void testDSOConversions(void);
int getDisplayFrowRawInputValue(int aAdcValue);
int getDisplayFrowMultipleRawValues(uint16_t * aAdcValuePtr, int aCount);

void initRawToDisplayFactors(void);
int getRawOffsetValueFromGridCount(int aCount);
int getInputRawFromDisplayValue(int aValue);
float getFloatFromRawValue(int aValue);
float getFloatFromDisplayValue(uint8_t aValue);

float getDataBufferTimebaseExactValueMicros(int8_t aTimebaseIndex);

float32_t * computeFFT(uint16_t * aDataBufferPointer);
void draw128FFTValuesFast(Color_t aColor);
void clearFFTValuesOnDisplay(void);
void drawFFT(void);

#endif /* SIMPLETOUCHSCREENDSO_H_ */
