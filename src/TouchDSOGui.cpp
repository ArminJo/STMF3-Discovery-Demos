/**
 * TouchDSOGui.cpp
 * @brief contains the gui related and flow control functions for DSO.
 *
 * @date 29.03.2012
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 */

#include "Pages.h" // must be first because of #include <stm32f3xx.h> for arm_math.h
#include "TouchDSO.h"
#ifndef LOCAL_DISPLAY_EXISTS
#include "EventHandler.h"
#include "utils.h" // for showRTCTimeEverySecond()
#endif

#include <string.h>
#include <stdlib.h> // for abs()
#include "Chart.h" // for adjustIntWithScaleFactor()

#ifdef LOCAL_FILESYSTEM_EXISTS
extern "C" {
#include "ff.h"
}
#endif

/**********************
 * Buttons
 *********************/
BDButton TouchButtonBack;
BDButton TouchButtonFrequencyPage;
BDButton TouchButtonShowSystemInfo;

BDButton TouchButtonStartStopDSOMeasurement;

BDButton TouchButtonTriggerMode;
const char TriggerModeButtonStringAuto[] = "Trigger auto";
const char TriggerModeButtonStringManualTimeout[] = "Trigger man t";
const char TriggerModeButtonStringManual[] = "Trigger man";
const char TriggerModeButtonStringFree[] = "Trigger free";
const char TriggerModeButtonStringExtern[] = "Trigger ext";

BDButton TouchButtonAutoRangeOnOff;
const char AutoRangeButtonStringAuto[] = "Range auto";
const char AutoRangeButtonStringManual[] = "Range man";

BDButton TouchButtonAutoOffsetOnOff;
const char AutoOffsetButtonString0[] = "Offset 0V";
const char AutoOffsetButtonStringAuto[] = "Offset auto";
const char AutoOffsetButtonStringMan[] = "Offset man";

// still experimental
BDButton TouchButtonDrawModeTriggerLine;
const char DrawModeTriggerLineButtonString[] = "Trigger\nline";

BDButton TouchButtonSingleshot;

BDButton TouchButtonMinMaxMode;
const char MinMaxModeButtonStringMinMax[] = "Min/Max\nmode";
const char MinMaxModeButtonStringSample[] = "Sample\nmode";

BDButton TouchButtonChartHistoryOnOff;

#ifdef LOCAL_DISPLAY_EXISTS
BDButton TouchButtonDrawModeLinePixel;
const char DrawModeButtonStringLine[] = "Line";
const char DrawModeButtonStringPixel[] = "Pixel";

BDButton TouchButtonADS7846TestOnOff;
#endif

#ifdef LOCAL_FILESYSTEM_EXISTS
BDButton TouchButtonLoad;
BDButton TouchButtonStore;
#define MODE_LOAD 0
#define MODE_STORE 1
#endif
BDButton TouchButtonFFT;

BDButton TouchButtonChannelSelect;
BDButton TouchButtonDSOSettings;
BDButton TouchButtonDSOMoreSettings;
BDButton TouchButtonCalibrateVoltage;
BDButton TouchButtonACRangeOnOff;
BDButton TouchButtonShowPretriggerValuesOnOff;

#ifdef LOCAL_DISPLAY_EXISTS
BDButton * const TouchButtonsDSO[] = {&TouchButtonBack, &TouchButtonStartStopDSOMeasurement, &TouchButtonTriggerMode,
    &TouchButtonAutoRangeOnOff, &TouchButtonAutoOffsetOnOff, &TouchButtonChannelSelect, &TouchButtonDrawModeLinePixel,
    &TouchButtonDrawModeTriggerLine, &TouchButtonDSOSettings, &TouchButtonDSOMoreSettings, &TouchButtonSingleshot,
    &TouchButtonSlope, &TouchButtonADS7846TestOnOff, &TouchButtonMinMaxMode, &TouchButtonChartHistoryOnOff,
#ifdef LOCAL_FILESYSTEM_EXISTS
    &TouchButtonLoad, &TouchButtonStore,
#endif
    &TouchButtonFFT, &TouchButtonCalibrateVoltage, &TouchButtonACRangeOnOff, &TouchButtonShowPretriggerValuesOnOff};
#endif

/*
 * Slider for trigger level and voltage picker
 */
BDSlider TouchSliderTriggerLevel;
BDSlider TouchSliderVoltagePicker;
uint8_t LastPickerValue;

#ifdef LOCAL_DISPLAY_EXISTS
BDSlider TouchSliderBacklight;
#endif

/**********************************
 * Input channels
 **********************************/
BDButton TouchButtonChannels[NUMBER_OF_CHANNEL_WITH_FIXED_ATTENUATOR];

const char StringChannel0[] = "Ch. 0";
const char StringChannel1[] = "Ch. 1";
const char StringChannel2[] = "Ch. 2";
const char StringChannel3[] = "Ch. 3";
const char StringChannel4[] = "Ch. 4";
const char StringTemperature[] = "Temp.";
const char StringVRefint[] = " VRef";
#ifdef STM32F30X
const char StringVBattDiv2[] = "\xBD" "VBatt";
const char * const ADCInputMUXChannelStrings[ADC_CHANNEL_COUNT] = {StringChannel2, StringChannel3, StringChannel4,
    StringTemperature, StringVBattDiv2, StringVRefint};
char ADCInputMUXChannelChars[ADC_CHANNEL_COUNT] = {'1', '2', '3', 'T', 'B', 'R'};
uint8_t const ADCInputMUXChannels[ADC_CHANNEL_COUNT] = {ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_4,
    ADC_CHANNEL_TEMPSENSOR, ADC_CHANNEL_VBAT, ADC_CHANNEL_VREFINT};
#else
const char * const ADCInputMUXChannelStrings[ADC_CHANNEL_COUNT] = { StringChannel0, StringChannel1, StringChannel2, StringChannel3,
        StringTemperature, StringVRefint };
char ADCInputMUXChannelChars[ADC_CHANNEL_COUNT] = { '0', '1', '2', '3', 'T', 'R' };
const uint8_t ADCInputMUXChannels[ADC_CHANNEL_COUNT] = { ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3,
ADC_CHANNEL_TEMPSENSOR, ADC_CHANNEL_VREFINT };
#endif
const char ChannelDivBy1ButtonString[] = "\xF7" "1";
const char ChannelDivBy10ButtonString[] = "\xF7" "10";
const char ChannelDivBy100ButtonString[] = "\xF7" "100";
const char * const ChannelDivByButtonStrings[NUMBER_OF_CHANNEL_WITH_FIXED_ATTENUATOR] = { ChannelDivBy1ButtonString,
        ChannelDivBy10ButtonString, ChannelDivBy100ButtonString };

/***********************
 *   Loop control
 ***********************/
#ifndef LOCAL_DISPLAY_EXISTS
uint32_t MillisLastLoop;
uint32_t sMillisSinceLastInfoOutput;
#endif
#define MILLIS_BETWEEN_INFO_OUTPUT ONE_SECOND

/*******************************************************************************************
 * Function declaration section
 *******************************************************************************************/
#ifdef LOCAL_DISPLAY_EXISTS
void readADS7846Channels(void);
void doADS7846TestOnOff(BDButton * aTheTouchedButton, int16_t aValue);
void doDrawMode(BDButton * aTheTouchedButton, int16_t aValue);
#endif
#ifdef LOCAL_FILESYSTEM_EXISTS
static void doStoreLoadAcquisitionData(BDButton * aTheTouchedButton, int16_t aMode);
#endif
void initDSOGUI(void);
void activateCommonPartOfGui(void);
void activateAnalysisOnlyPartOfGui(void);
void activateRunningOnlyPartOfGui(void);
void drawCommonPartOfGui(void);
void drawAnalysisOnlyPartOfGui(void);
void drawRunningOnlyPartOfGui(void);
void drawDSOSettingsPage(void);
void drawDSOMoreSettingsPage(void);
void startDSOSettingsPage(void);
void startDSOMoreSettingsPage(void);
void redrawDisplay(void);
void longTouchDownHandlerDSO(struct TouchEvent * const);
void swipeEndHandlerDSO(struct Swipe * const aSwipeInfo);
void TouchUpHandler(struct TouchEvent * const aTochPosition);
void doTriggerSlope(BDButton * aTheTouchedButton, int16_t aValue);
void doOffsetMode(BDButton * aTheTouchedButton, int16_t aValue);
void doChannelSelect(BDButton * aTheTouchedButton, int16_t aValue);
void doShowSettingsPage(BDButton * aTheTouchedButton, int16_t aValue);
void doShowMoreSettingsPage(BDButton * aTheTouchedButton, int16_t aValue);
void doStartSingleshot(BDButton * aTheTouchedButton, int16_t aValue);
void doStartStopDSO(BDButton * aTheTouchedButton, int16_t aValue);
void doChartHistory(BDButton * aTheTouchedButton, int16_t aValue);
void doRangeMode(BDButton * aTheTouchedButton, int16_t aValue);

void doTriggerLevel(BDSlider * aTheTouchedSlider, uint16_t aValue);
void doVoltagePicker(BDSlider * aTheTouchedSlider, uint16_t aValue);

/*******************************************************************************************
 * Program code starts here
 * Setup section
 *******************************************************************************************/
void initDSOPage(void) {
// cannot be done statically
    DisplayControl.ShowFFT = false;  // before initDSOGUI()
    DisplayControl.DatabufferPreTriggerDisplaySize = (2 * DATABUFFER_DISPLAY_RESOLUTION);
    initAcquisition();
#ifndef LOCAL_DISPLAY_EXISTS
    initDSOGUI();
#endif
}

void setButtonCaptions(void) {
    if (MeasurementControl.isMinMaxMode) {
        TouchButtonMinMaxMode.setCaption(MinMaxModeButtonStringMinMax);
    } else {
        TouchButtonMinMaxMode.setCaption(MinMaxModeButtonStringSample);
    }
    if (MeasurementControl.ChannelIsACMode) {
        TouchButtonACRangeOnOff.setCaption("AC");
    } else {
        TouchButtonACRangeOnOff.setCaption("DC");
    }
}

void setChannelButtonsCaption(void) {
    for (uint8_t i = 0; i < NUMBER_OF_CHANNEL_WITH_FIXED_ATTENUATOR; ++i) {
        if (MeasurementControl.AttenuatorType == ATTENUATOR_TYPE_FIXED_ATTENUATOR) {
            TouchButtonChannels[i].setCaption(ChannelDivByButtonStrings[i]);
        } else {
            TouchButtonChannels[i].setCaption(ADCInputMUXChannelStrings[i], (DisplayControl.DisplayPage == DISPLAY_PAGE_SETTINGS));
        }
    }
}

void startDSOPage(void) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);
    DisplayControl.DisplayPage = DISPLAY_PAGE_START;

    DisplayControl.showTriggerInfoLine = false;
    DisplayControl.showInfoMode = INFO_MODE_SHORT_INFO;
    DisplayControl.EraseColor = COLOR_BACKGROUND_DSO;
    DisplayControl.XScale = 0;
    DisplayControl.DisplayIncrementPixel = DATABUFFER_DISPLAY_INCREMENT; // == getXScaleCorrectedValue(DATABUFFER_DISPLAY_INCREMENT);
    resetAcquisition(); // sets MinMaxMode

#ifdef LOCAL_DISPLAY_EXISTS
    initDSOGUI();
    DisplayControl.drawPixelMode = false;
#endif

// show start elements
    setButtonCaptions();
    drawCommonPartOfGui();
    drawAnalysisOnlyPartOfGui();

    // use max of DATABUFFER_PRE_TRIGGER_SIZE * sizeof(uint16_t) AND sizeof(float32_t) * 2 * FFT_SIZE
    // 2k
    TempBufferForPreTriggerAdjustAndFFT = malloc(sizeof(float32_t) * 2 * FFT_SIZE);
    if (TempBufferForPreTriggerAdjustAndFFT == NULL) {
        failParamMessage(sizeof(float32_t) * 2 * FFT_SIZE, "malloc() fails");
    }

    registerRedrawCallback(&redrawDisplay);
    registerLongTouchDownCallback(&longTouchDownHandlerDSO, TOUCH_STANDARD_LONG_TOUCH_TIMEOUT_MILLIS);
    registerSwipeEndCallback(&swipeEndHandlerDSO);
// use touch up for buttons in order not to interfere with long touch
    registerTouchUpCallback(&TouchUpHandler);

#ifdef LOCAL_DISPLAY_EXISTS
    setDimDelayMillis(3 * BACKLIGHT_DIM_DEFAULT_DELAY);

// use touch down only for sliders
    registerTouchDownCallback(&simpleTouchDownHandlerOnlyForSlider);
    // to avoid counting touch move/up for slider.
    sDisableUntilTouchUpIsDone = true;
#endif
}

void stopDSOPage(void) {
    DSO_setAttenuator(ACTIVE_ATTENUATOR_INFINITE_VALUE);
    free(TempBufferForPreTriggerAdjustAndFFT);

// only here
    ADC_DSO_stopTimer();

// in case of exiting during a running acquisition
    ADC1_DMA_stop();
    //ADC_StopConversion(ADC1);
#ifdef STM32F30X
    ADC1Handle.Instance->CR |= ADC_CR_ADSTP;
#else
    CLEAR_BIT(ADC1Handle.Instance->CR2, ADC_CR2_EXTTRIG);
#endif
    __HAL_ADC_DISABLE_IT(&ADC1Handle, ADC_IT_EOC);
    //ADC_disableEOCInterrupt(ADC1);

    registerLongTouchDownCallback(NULL, 0);
    registerSwipeEndCallback(NULL);
    registerTouchUpCallback(NULL);

#ifdef LOCAL_DISPLAY_EXISTS
    // free buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsDSO) / sizeof(TouchButtonsDSO[0]); ++i) {
        (*TouchButtonsDSO[i]).deinit();
    }
    for (unsigned int i = 0; i < NUMBER_OF_CHANNEL_WITH_FIXED_ATTENUATOR; ++i) {
        TouchButtonChannels[i].deinit();
    }
    TouchSliderVoltagePicker.deinit();
    TouchSliderTriggerLevel.deinit();
    TouchSliderBacklight.deinit();

    // restore normal behavior
    BlueDisplay1.setButtonsGlobalFlags(FLAG_BUTTON_GLOBAL_USE_DOWN_EVENTS_FOR_BUTTONS);

// restore old touch down handler
    registerTouchDownCallback(&simpleTouchDownHandler);
    // restore flag, since it is not reset by button touch
    sDisableUntilTouchUpIsDone = false;

    setDimDelayMillis(BACKLIGHT_DIM_DEFAULT_DELAY);
#endif
}

/************************************************************************
 * main loop - 32 microseconds
 ************************************************************************/
void loopDSOPage(void) {
    uint32_t tMillis, tMillisOfLoop;
    static bool sDoInfoOutput = true;

    tMillis = getMillisSinceBoot();
    tMillisOfLoop = tMillis - MillisLastLoop;
    MillisLastLoop = tMillis;
    sMillisSinceLastInfoOutput += tMillisOfLoop;
    if (sMillisSinceLastInfoOutput > MILLIS_BETWEEN_INFO_OUTPUT) {
        sMillisSinceLastInfoOutput = 0;
        sDoInfoOutput = true;
    }

    if (MeasurementControl.isRunning) {
#ifdef LOCAL_DISPLAY_EXISTS
        if (MeasurementControl.ADS7846ChannelsAsDatasource) {
            readADS7846Channels();
            // check if button pressed - to process stop and channel buttons here
            TouchPanel.rd_data();
            if (TouchPanel.mPressure > MIN_REASONABLE_PRESSURE) {
                TouchButton::checkAllButtons(TouchPanel.getXActual(), TouchPanel.getYActual());
            }
        }
#endif
        if (DataBufferControl.DataBufferFull) {
            /*
             * Data (from InterruptServiceRoutine or DMA) is ready
             */
            if (sDoInfoOutput) {
                sDoInfoOutput = false;
                if (DisplayControl.DisplayPage == DISPLAY_PAGE_CHART) {
                    drawGridLinesWithHorizLabelsAndTriggerLine();
                    if (DisplayControl.showInfoMode != INFO_MODE_NO_INFO) {
                        computePeriodFrequency(); // compute values only once
                        printInfo();
                    }
                } else if (DisplayControl.DisplayPage == DISPLAY_PAGE_SETTINGS) {
                    // refresh buttons
                    drawDSOSettingsPage();
                } else if (DisplayControl.DisplayPage == DISPLAY_PAGE_FREQUENCY) {
                    // refresh buttons
                    drawFrequencyGeneratorPage();
#ifndef AVR
                } else if (DisplayControl.DisplayPage == DISPLAY_PAGE_MORE_SETTINGS) {
                    // refresh buttons
                    drawDSOMoreSettingsPage();
#endif
                }
            }

            if (!(MeasurementControl.TimebaseFastDMAMode || DataBufferControl.DrawWhileAcquire)) {
                adjustPreTriggerBuffer();
            }
            if (MeasurementControl.StopRequested) {
                if (DataBufferControl.DataBufferEndPointer == &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_END]
                        && !MeasurementControl.StopAcknowledged) {
                    // Stop requested, but DataBufferEndPointer (for ISR) has the value of a regular acquisition or dma has not realized stop
                    // -> start new last acquisition
                    startAcquisition();
                } else {
                    /*
                     * do stop handling here (in thread mode)
                     */
                    MeasurementControl.StopRequested = false;
                    MeasurementControl.isRunning = false;
                    MeasurementControl.isSingleShotMode = false;

                    // delayed tone for stop
#ifdef LOCAL_DISPLAY_EXISTS
                    FeedbackTone(false);
                    DisplayControl.drawPixelMode = false;
#else
                    BlueDisplay1.playFeedbackTone(false);
#endif
                    DisplayControl.ShowFFT = false;
                    // draw grid lines and gui
                    redrawDisplay();
                }
            } else {
                if (MeasurementControl.ChangeRequestedFlags & CHANGE_REQUESTED_TIMEBASE_FLAG) {
                    changeTimeBase();
                    MeasurementControl.ChangeRequestedFlags = 0;
                }
                /*
                 * Normal loop-> process data, draw new chart, and start next acquisition
                 */
                int tLastTriggerDisplayValue = DisplayControl.TriggerLevelDisplayValue;
                computeMinMax();
                computeAutoTrigger();
                computeAutoRangeAndAutoOffset();
                // handle trigger line
                DisplayControl.TriggerLevelDisplayValue = getDisplayFrowRawInputValue(MeasurementControl.RawTriggerLevel);
                if (tLastTriggerDisplayValue
                        != DisplayControl.TriggerLevelDisplayValue&& MeasurementControl . TriggerMode != TRIGGER_MODE_FREE) {
                    clearTriggerLine(tLastTriggerDisplayValue);
                    drawTriggerLine();
                }
                if (!DataBufferControl.DrawWhileAcquire) {    // normal mode => clear old chart and draw new data
                    drawDataBuffer(DataBufferControl.DataBufferDisplayStart, REMOTE_DISPLAY_WIDTH, COLOR_DATA_RUN,
                            DisplayControl.EraseColor, DRAW_MODE_REGULAR, MeasurementControl.isEffectiveMinMaxMode);
                    draw128FFTValuesFast(COLOR_FFT_DATA);
                }
                startAcquisition();
            }
        }
        if (DataBufferControl.DrawWhileAcquire) {
            /*
             * Draw while acquire mode
             */
            if (MeasurementControl.ChangeRequestedFlags & CHANGE_REQUESTED_TIMEBASE_FLAG) {
                // StopConversion here and not at end of display
#ifdef STM32F30X
                ADC1Handle.Instance->CR |= ADC_CR_ADSTP;
#else
                CLEAR_BIT(ADC1Handle.Instance->CR2, ADC_CR2_EXTTRIG);
#endif

                changeTimeBase();
                MeasurementControl.ChangeRequestedFlags = 0;
                //ADC_StartConversion(ADC1);
#ifdef STM32F30X
                ADC1Handle.Instance->CR |= ADC_CR_ADSTART;
#else
                SET_BIT(ADC1Handle.Instance->CR2, ADC_CR2_EXTTRIG);
#endif

            }
            // detect end of pre trigger phase and adjust pre trigger buffer during acquisition
            if (MeasurementControl.TriggerPhaseJustEnded) {
                MeasurementControl.TriggerPhaseJustEnded = false;
                adjustPreTriggerBuffer();
                DataBufferControl.DataBufferNextDrawPointer = &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_START];
                DataBufferControl.NextDrawXValue = 0;
            }
            // check if new data available and draw in corresponding color
            if (MeasurementControl.TriggerActualPhase < PHASE_POST_TRIGGER) {
                if (DataBufferControl.DataBufferPreTriggerAreaWrapAround) {
                    // wrap while searching trigger -> start from beginning of buffer
                    DataBufferControl.DataBufferPreTriggerAreaWrapAround = false;
                    DataBufferControl.NextDrawXValue = 0;
                    DataBufferControl.DataBufferNextDrawPointer = &DataBufferControl.DataBuffer[0];
                }
                drawRemainingDataBufferValues(COLOR_DATA_PRETRIGGER);
            } else {
                drawRemainingDataBufferValues(COLOR_DATA_RUN);
            }
        }
        /*
         * Display pre trigger data for single shot mode
         */
        if (MeasurementControl.isSingleShotMode && MeasurementControl.TriggerActualPhase < PHASE_POST_TRIGGER && sDoInfoOutput) {
            sDoInfoOutput = false;
            /*
             * single shot - output actual values every second
             */
            if (!DataBufferControl.DrawWhileAcquire) {
                if (MeasurementControl.TimebaseFastDMAMode) {
                    // copy data in ISR, otherwise it might be overwritten more than once while copying here
                    MeasurementControl.doPretriggerCopyForDisplay = true;
                    // wait for ISR to finish copy
                    while (MeasurementControl.doPretriggerCopyForDisplay) {
                        ;
                    }
                } else {
                    memcpy(TempBufferForPreTriggerAdjustAndFFT, &DataBufferControl.DataBuffer[0],
                    DATABUFFER_PRE_TRIGGER_SIZE * sizeof(DataBufferControl.DataBuffer[0]));
                }
                drawDataBuffer((uint16_t *) TempBufferForPreTriggerAdjustAndFFT, DATABUFFER_PRE_TRIGGER_SIZE,
                COLOR_DATA_PRETRIGGER, COLOR_BACKGROUND_DSO, DRAW_MODE_REGULAR, false);
            }
            printInfo();
        }
    } else {
        // Analyze mode here
        if (sDoInfoOutput && DisplayControl.DisplayPage == DISPLAY_PAGE_CHART && DisplayControl.showInfoMode != INFO_MODE_NO_INFO) {
            sDoInfoOutput = false;
            /*
             * show time
             */
            showRTCTimeEverySecond(0, FONT_SIZE_INFO_LONG_ASC + 4 * FONT_SIZE_INFO_LONG, COLOR_RED, COLOR_BACKGROUND_DSO);
        }
    }
    /*
     * Handle Sub-Pages
     */
    if (DisplayControl.DisplayPage == DISPLAY_PAGE_SETTINGS) {
        if (sBackButtonPressed) {
            sBackButtonPressed = false;
            DisplayControl.DisplayPage = DISPLAY_PAGE_CHART;
            redrawDisplay();
        }
    } else if (DisplayControl.DisplayPage == DISPLAY_PAGE_FREQUENCY) {
        if (sBackButtonPressed) {
            sBackButtonPressed = false;
            stopFrequencyGeneratorPage();
            DisplayControl.DisplayPage = DISPLAY_PAGE_SETTINGS;
            redrawDisplay();
        } else {
            //not needed here, because is contains only checkAndHandleEvents()
            // loopFrequencyGeneratorPage();
        }
    } else if (DisplayControl.DisplayPage == DISPLAY_PAGE_SYST_INFO) {
        if (sBackButtonPressed) {
            sBackButtonPressed = false;
            stopSystemInfoPage();
            DisplayControl.DisplayPage = DISPLAY_PAGE_SETTINGS;
            redrawDisplay();
        } else {
            // refresh buttons and info
            loopSystemInfoPage();
        }
    } else if (DisplayControl.DisplayPage == DISPLAY_PAGE_MORE_SETTINGS) {
        if (sBackButtonPressed) {
            sBackButtonPressed = false;
            DisplayControl.DisplayPage = DISPLAY_PAGE_SETTINGS;
            redrawDisplay();
        }
    }
    checkAndHandleEvents();
}
/* Main loop end */

/************************************************************************
 * Utils section
 ************************************************************************/
bool checkDatabufferPointerForDrawing(void) {
    bool tReturn = true;
// check for DataBufferDisplayStart
    if (DataBufferControl.DataBufferDisplayStart
            > DataBufferControl.DataBufferEndPointer - (adjustIntWithScaleFactor(REMOTE_DISPLAY_WIDTH, DisplayControl.XScale) - 1)) {
        DataBufferControl.DataBufferDisplayStart = (uint16_t *) DataBufferControl.DataBufferEndPointer
                - (adjustIntWithScaleFactor(REMOTE_DISPLAY_WIDTH, DisplayControl.XScale) - 1);
        // Check begin - if no screen full of data acquired (by forced stop)
        if ((DataBufferControl.DataBufferDisplayStart < &DataBufferControl.DataBuffer[0])) {
            DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[0];
        }
        tReturn = false;
    }
    return tReturn;
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
    uint8_t tFeedbackType = FEEDBACK_TONE_NO_ERROR;
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
        tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
    } else {
        // signal change to main loop in thread mode
        MeasurementControl.TimebaseNewIndex = tNewIndex;
        MeasurementControl.ChangeRequestedFlags |= CHANGE_REQUESTED_TIMEBASE_FLAG;
    }
    return tFeedbackType;
}

/**
 * Handler for offset up / down
 */
int changeOffsetGridCount(int aValue) {
    setOffsetGridCount(MeasurementControl.OffsetGridCount + aValue);
    drawGridLinesWithHorizLabelsAndTriggerLine();
    return FEEDBACK_TONE_NO_ERROR;
}

/*
 * changes x resolution and draws data - only for analyze mode
 */
int changeXScale(int aValue) {
    int tFeedbackType = FEEDBACK_TONE_NO_ERROR;
    DisplayControl.XScale += aValue;

    if (DisplayControl.XScale < -DATABUFFER_DISPLAY_RESOLUTION_FACTOR) {
        tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
        DisplayControl.XScale = -DATABUFFER_DISPLAY_RESOLUTION_FACTOR;
    }
    DisplayControl.DisplayIncrementPixel = adjustIntWithScaleFactor(DATABUFFER_DISPLAY_INCREMENT, DisplayControl.XScale);
    printInfo();

// Check end
    if (!checkDatabufferPointerForDrawing()) {
        tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
    }
// do tone before draw
#ifdef LOCAL_DISPLAY_EXISTS
    FeedbackTone(tFeedbackType);
#else
    BlueDisplay1.playFeedbackTone(tFeedbackType);
#endif
    tFeedbackType = FEEDBACK_TONE_NO_TONE;
    drawDataBuffer(DataBufferControl.DataBufferDisplayStart, REMOTE_DISPLAY_WIDTH, COLOR_DATA_HOLD, COLOR_BACKGROUND_DSO,
    DRAW_MODE_REGULAR, MeasurementControl.isEffectiveMinMaxMode);

    return tFeedbackType;
}

/**
 * set start of display in data buffer and draws data - only for analyze mode
 * @param aValue number of DisplayControl.DisplayIncrementPixel to scroll
 */
int scrollDisplay(int aValue) {
    uint8_t tFeedbackType = FEEDBACK_TONE_NO_ERROR;
    if (DisplayControl.DisplayPage == DISPLAY_PAGE_CHART) {
        DataBufferControl.DataBufferDisplayStart += aValue * DisplayControl.DisplayIncrementPixel;
        // Check begin
        if ((DataBufferControl.DataBufferDisplayStart < &DataBufferControl.DataBuffer[0])) {
            DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[0];
            tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
        }
        // Check end
        if (!checkDatabufferPointerForDrawing()) {
            tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
        }
        // delete old graph and draw new one
        drawDataBuffer(DataBufferControl.DataBufferDisplayStart, REMOTE_DISPLAY_WIDTH, COLOR_DATA_HOLD, COLOR_BACKGROUND_DSO,
        DRAW_MODE_REGULAR, MeasurementControl.isEffectiveMinMaxMode);
    }
    return tFeedbackType;
}

/************************************************************************
 * Callback handler section
 ************************************************************************/
/*
 * Handler for "empty" touch
 * Use touch up in order not to interfere with long touch
 * Switch between upper info line short/long/off
 */
void TouchUpHandler(struct TouchEvent * const aTouchPosition) {
#ifdef LOCAL_DISPLAY_EXISTS
// first check for buttons
    if (!TouchButton::checkAllButtons(aTouchPosition->TouchPosition.PosX, aTouchPosition->TouchPosition.PosY)) {
#endif
    if (DisplayControl.DisplayPage == DISPLAY_PAGE_CHART) {
        // Wrap display mode
        uint8_t tNewMode = DisplayControl.showInfoMode + 1;
        if (tNewMode > INFO_MODE_LONG_INFO) {
            tNewMode = INFO_MODE_NO_INFO;
        }
        DisplayControl.showInfoMode = tNewMode;
        // erase former (short) info line
        clearInfo();
        if (tNewMode != INFO_MODE_NO_INFO) {
            printInfo();
        } else if (!MeasurementControl.isRunning) {
            redrawDisplay();
        }
    }
#ifdef LOCAL_DISPLAY_EXISTS
}
#endif
}

/**
 * Shows gui on long touch
 */
void longTouchDownHandlerDSO(struct TouchEvent * const aTouchPosition) {
    static bool sIsGUIVisible = false;
    if (DisplayControl.DisplayPage == DISPLAY_PAGE_CHART) {
        if (MeasurementControl.isRunning) {
            if (sIsGUIVisible) {
                redrawDisplay();
            } else {
                // Show usable GUI
                drawRunningOnlyPartOfGui();
            }
            sIsGUIVisible = !sIsGUIVisible;
        } else {
            // clear screen and show start gui
            DisplayControl.DisplayPage = DISPLAY_PAGE_START;
            redrawDisplay();
        }
    } else if (DisplayControl.DisplayPage == DISPLAY_PAGE_START) {
        // only chart (and voltage picker)
        DisplayControl.DisplayPage = DISPLAY_PAGE_CHART;
        redrawDisplay();
    }
}

/**
 * Handler for input range up / down
 */
int changeRangeHandler(int aValue) {
    int tFeedbackType = FEEDBACK_TONE_NO_ERROR;
    if (!setDisplayRange(MeasurementControl.DisplayRangeIndex + aValue)) {
        tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
    }
    return tFeedbackType;
}

/**
 * responsible for swipe detection and dispatching
 */
void swipeEndHandlerDSO(struct Swipe * const aSwipeInfo) {
// we have only vertical sliders so allow horizontal swipes even if starting at slider position
    if (aSwipeInfo->TouchDeltaAbsMax < TIMING_GRID_WIDTH) {
        return;
    }
    if (DisplayControl.DisplayPage == DISPLAY_PAGE_CHART && aSwipeInfo->TouchDeltaAbsMax >= TIMING_GRID_WIDTH) {
        bool tDeltaGreaterThanOneGrid = (aSwipeInfo->TouchDeltaAbsMax / (2 * TIMING_GRID_WIDTH)) != 0; // otherwise functions below are called with zero
        int tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
        int tTouchDeltaXGrid = aSwipeInfo->TouchDeltaX / TIMING_GRID_WIDTH;
        if (MeasurementControl.isRunning) {
            /*
             * Running mode
             */
            if (aSwipeInfo->SwipeMainDirectionIsX) {
                // Horizontal swipe -> timebase
                if (tDeltaGreaterThanOneGrid) {
                    tFeedbackType = changeTimeBaseValue(-tTouchDeltaXGrid / 2);
                }
            } else {
                // Vertical swipe
                int tTouchDeltaYGrid = aSwipeInfo->TouchDeltaY / TIMING_GRID_WIDTH;
                if ((!MeasurementControl.RangeAutomatic && MeasurementControl.OffsetMode == OFFSET_MODE_AUTOMATIC)
                        || MeasurementControl.OffsetMode == OFFSET_MODE_MANUAL) {
                    // decide which swipe to perform according to x position of swipe
                    if (aSwipeInfo->TouchStartX > BUTTON_WIDTH_3_POS_2) {
                        //offset
                        tFeedbackType = changeOffsetGridCount(tTouchDeltaYGrid);
                    } else if (tDeltaGreaterThanOneGrid) {
                        //range manual has priority if range manual AND OFFSET_MODE_MANUAL selected
                        if (!MeasurementControl.RangeAutomatic) {
                            tFeedbackType = changeRangeHandler(tTouchDeltaYGrid / 2);
                        } else {
                            tFeedbackType = changeDisplayRange(tTouchDeltaYGrid / 2);
                        }
                    }
                } else if (!MeasurementControl.RangeAutomatic) {
                    //range manual
                    if (tDeltaGreaterThanOneGrid) {
                        tFeedbackType = changeRangeHandler(tTouchDeltaYGrid / 2);
                    }
                } else if (MeasurementControl.OffsetMode != OFFSET_MODE_0_VOLT) {
                    //offset mode != 0 Volt
                    tFeedbackType = changeOffsetGridCount(tTouchDeltaYGrid);
                }
            }
        } else {
            /*
             * Analyze Mode
             */
            if (aSwipeInfo->TouchStartY > BUTTON_HEIGHT_4_LINE_3 + TEXT_SIZE_22) {
                tFeedbackType = scrollDisplay(-tTouchDeltaXGrid);
            } else {
                // scale
                if (tDeltaGreaterThanOneGrid) {
                    tFeedbackType = changeXScale(tTouchDeltaXGrid / 2);
                }
            }
        }
#ifdef LOCAL_DISPLAY_EXISTS
        FeedbackTone(tFeedbackType);
#else
        BlueDisplay1.playFeedbackTone(tFeedbackType);
#endif
    }
}

/************************************************************************
 * Button handler section
 ************************************************************************/

/*
 * step from 0 Volt  to auto to manual offset
 */
void doOffsetMode(BDButton * aTheTouchedButton, int16_t aValue) {
    MeasurementControl.OffsetMode++;
    if (MeasurementControl.OffsetMode > OFFSET_MODE_MANUAL) {
        // switch back from Mode Manual and set range mode to automatic
        MeasurementControl.OffsetMode = OFFSET_MODE_0_VOLT;
        aTheTouchedButton->setCaptionAndDraw(AutoOffsetButtonString0);
        setAutoRangeModeAndButtonCaption(true);
        setOffsetGridCountAccordingToACMode();
    } else if (MeasurementControl.OffsetMode == OFFSET_MODE_AUTOMATIC) {
        aTheTouchedButton->setCaptionAndDraw(AutoOffsetButtonStringAuto);
    } else {
        // Offset mode manual implies range mode manual
        aTheTouchedButton->setCaptionAndDraw(AutoOffsetButtonStringMan);
        setAutoRangeModeAndButtonCaption(false);
        TouchButtonAutoRangeOnOff.deactivate();
    }
}

void doAcDcMode(BDButton * aTheTouchedButton, int16_t aValue) {
    setACMode(!MeasurementControl.isACMode);
    setButtonCaptions();
    aTheTouchedButton->drawButton();
}

/*
 * Cycle through all external and internal adc channels if button value is > 20
 */
void doChannelSelect(BDButton * aTheTouchedButton, int16_t aValue) {
#ifdef LOCAL_DISPLAY_EXISTS
    if (MeasurementControl.ADS7846ChannelsAsDatasource) {
        // ADS7846 channels
        MeasurementControl.ADCInputMUXChannelIndex++;
        if (MeasurementControl.ADCInputMUXChannelIndex >= ADS7846_CHANNEL_COUNT) {
            // wrap to first channel connected to attenuator and restore ACRange
            MeasurementControl.ADCInputMUXChannelIndex = 0;
            MeasurementControl.isACMode = DSO_getACMode();
        }
        aTheTouchedButton->setCaption(ADS7846ChannelStrings[MeasurementControl.ADCInputMUXChannelIndex]);

    } else
#endif
    {
        if (aValue > 20) {
            // channel increment button (CH 3) here
            uint8_t tOldValue = MeasurementControl.ADCInputMUXChannelIndex;
            // increment channel but only if channel 3 is still selected
            if (tOldValue >= NUMBER_OF_CHANNEL_WITH_FIXED_ATTENUATOR) {
                aValue = tOldValue + 1;
            } else {
                aValue = NUMBER_OF_CHANNEL_WITH_FIXED_ATTENUATOR;
            }

        }
        setChannel(aValue);
        aValue = MeasurementControl.ADCInputMUXChannelIndex;
        if (aValue < NUMBER_OF_CHANNEL_WITH_FIXED_ATTENUATOR) {
            //reset caption of 4. button to channel 3
            aValue = 3;
        }
        // set new channel number in caption of channel select button
        TouchButtonChannelSelect.setCaption(ADCInputMUXChannelStrings[aValue]);
    }
    if (DisplayControl.DisplayPage == DISPLAY_PAGE_SETTINGS) {
        redrawDisplay();
    } else {
        if (DisplayControl.showInfoMode == INFO_MODE_LONG_INFO) {
            clearInfo();
            printInfo();
        }
    }
}

/*
 * Switch trigger line mode on and off (for data chart)
 */
void doDrawModeTriggerLine(BDButton * aTheTouchedButton, int16_t aValue) {
// switch mode
    if (!aValue && MeasurementControl.isRunning) {
        // erase old chart in old mode
        drawDataBuffer(NULL, REMOTE_DISPLAY_WIDTH, DisplayControl.EraseColor, 0, DRAW_MODE_CLEAR_OLD,
                MeasurementControl.isEffectiveMinMaxMode);
    }
    DisplayControl.showTriggerInfoLine = aValue;
}

void doShowPretriggerValuesOnOff(BDButton * aTheTouchedButton, int16_t aValue) {
    DisplayControl.DatabufferPreTriggerDisplaySize = 0;
    if (aValue) {
        DisplayControl.DatabufferPreTriggerDisplaySize = (2 * DATABUFFER_DISPLAY_RESOLUTION);
    }
}

/**
 * prepare values for running mode
 */
void prepareForStart(void) {
    MeasurementControl.TimestampLastRangeChange = 0; // enable direct range change at start

// reset xScale to regular value
    if (MeasurementControl.TimebaseEffectiveIndex < TIMEBASE_NUMBER_OF_XSCALE_CORRECTION) {
        DisplayControl.XScale = xScaleForTimebase[MeasurementControl.TimebaseEffectiveIndex];
    } else {
        DisplayControl.XScale = 0;
    }

    startAcquisition();
// must be after startAcquisition()
    MeasurementControl.isRunning = true;

    redrawDisplay();
}

/**
 * start singleshot acquisition
 */
void doStartSingleshot(BDButton * aTheTouchedButton, int16_t aValue) {
// deactivate button
    aTheTouchedButton->deactivate();
    DisplayControl.DisplayPage = DISPLAY_PAGE_CHART;

// prepare info output - at least 1 sec later
    sMillisSinceLastInfoOutput = 0;
    MeasurementControl.RawValueMax = 0;
    MeasurementControl.RawValueMin = 0;
    MeasurementControl.RawValueAverage = 0;
    MeasurementControl.isSingleShotMode = true;
    prepareForStart();
}

void doStartStopDSO(BDButton * aTheTouchedButton, int16_t aValue) {
    if (MeasurementControl.isRunning) {
        /*
         * Stop here
         * for the last measurement read full buffer size
         * Do this asynchronously to the interrupt routine in order to extend a running or started acquisition
         * stop single shot mode
         */
        // first extends end marker for ISR to end of buffer instead of end of display
        DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[DATABUFFER_SIZE - 1];
//		if (MeasurementControl.SingleShotMode) {
//			MeasurementControl.ActualPhase = PHASE_POST_TRIGGER;
//		}
        // in SingleShotMode stop is directly requested
        if (MeasurementControl.StopRequested && !MeasurementControl.isSingleShotMode) {
            // for stop requested 2 times -> stop immediately
            uint16_t * tEndPointer = DataBufferControl.DataBufferNextInPointer;
            DataBufferControl.DataBufferEndPointer = tEndPointer;
            // clear trailing buffer space not used
            memset(tEndPointer, DATABUFFER_INVISIBLE_RAW_VALUE,
                    ((uint8_t*) &DataBufferControl.DataBuffer[DATABUFFER_SIZE]) - ((uint8_t*) tEndPointer));
//            for (int i = &DataBufferControl.DataBuffer[DATABUFFER_SIZE] - tEndPointer; i > 0; --i) {
//                *tEndPointer++ = DATABUFFER_INVISIBLE_RAW_VALUE;
//            }
        }
        // return to continuous  mode
        MeasurementControl.isSingleShotMode = false;
        MeasurementControl.StopRequested = true;
        // set value for horizontal scrolling
        DisplayControl.DisplayIncrementPixel = adjustIntWithScaleFactor(DATABUFFER_DISPLAY_INCREMENT, DisplayControl.XScale);
    } else {
        /*
         *  Start here in normal mode (reset single shot mode)
         */
        MeasurementControl.isSingleShotMode = false; // return to continuous  mode
        DisplayControl.DisplayPage = DISPLAY_PAGE_CHART;
        prepareForStart();
    }
}

/*
 * Sets only the flag and button caption
 */
void doMinMaxMode(BDButton * aTheTouchedButton, int16_t aValue) {
    MeasurementControl.isMinMaxMode = !MeasurementControl.isMinMaxMode;
    setButtonCaptions();
    aTheTouchedButton->drawButton();
    if (MeasurementControl.TimebaseEffectiveIndex >= TIMEBASE_INDEX_CAN_USE_OVERSAMPLING) {
        // changeTimeBase() manages oversampling rate for Min/Max oversampling
        if (MeasurementControl.isRunning) {
            // signal to main loop in thread mode
            MeasurementControl.ChangeRequestedFlags |= CHANGE_REQUESTED_TIMEBASE_FLAG;
        } else {
            changeTimeBase();
        }
    }
}

/**
 *
 * @param aTheTouchedSlider
 * @param aValue range is 0 to DISPLAY_VALUE_FOR_ZERO
 * @return
 */
void doTriggerLevel(BDSlider * aTheTouchedSlider, uint16_t aValue) {
// in auto-trigger mode show only computed value and do not modify it
    if (MeasurementControl.TriggerMode != TRIGGER_MODE_AUTOMATIC) {

        // to get display value take DISPLAY_VALUE_FOR_ZERO - aValue and vice versa
        int tValue = DISPLAY_VALUE_FOR_ZERO - aValue;
        if (DisplayControl.TriggerLevelDisplayValue != tValue) {

            // clear old trigger line
            clearTriggerLine(DisplayControl.TriggerLevelDisplayValue);

            // store actual display value
            DisplayControl.TriggerLevelDisplayValue = tValue;

            if (MeasurementControl.TriggerMode == TRIGGER_MODE_MANUAL) {
                // modify trigger values according to display value
                int tRawLevel = getInputRawFromDisplayValue(tValue);
                setTriggerLevelAndHysteresis(tRawLevel, TRIGGER_HYSTERESIS_MANUAL);
            }
            // draw new line
            drawTriggerLine();
            // print value
            printTriggerInfo();
        }
    }
}

/*
 * The value printed has a resolution of 0,00488 * scale factor
 */
void doVoltagePicker(BDSlider * aTheTouchedSlider, uint16_t aValue) {
    if (LastPickerValue == aValue) {
        return;
    }
    if (LastPickerValue != 0xFF) {
        // clear old line
        int tYpos = DISPLAY_VALUE_FOR_ZERO - LastPickerValue;
        BlueDisplay1.drawLine(0, tYpos, REMOTE_DISPLAY_WIDTH - 1, tYpos, COLOR_BACKGROUND_DSO);
        // restore grid at old y position
        for (unsigned int tXPos = TIMING_GRID_WIDTH - 1; tXPos < REMOTE_DISPLAY_WIDTH - 1; tXPos += TIMING_GRID_WIDTH) {
            BlueDisplay1.drawPixel(tXPos, tYpos, COLOR_GRID_LINES);
        }

        if (!MeasurementControl.isRunning) {
            // restore graph
            uint8_t* ScreenBufferPointer = &DisplayBuffer[0];
            uint8_t* ScreenBufferMinPointer = &DisplayBufferMin[0];
            for (unsigned int i = 0; i < REMOTE_DISPLAY_WIDTH; ++i) {
                int tValueByte = *ScreenBufferPointer++;
                if (tValueByte == tYpos) {
                    BlueDisplay1.drawPixel(i, tValueByte, COLOR_DATA_HOLD);
                }
                if (MeasurementControl.isEffectiveMinMaxMode) {
                    tValueByte = *ScreenBufferMinPointer++;
                    if (tValueByte == tYpos) {
                        BlueDisplay1.drawPixel(i, tValueByte, COLOR_DATA_HOLD);
                    }
                }
            }
        }
    }
// draw new line
    int tValue = DISPLAY_VALUE_FOR_ZERO - aValue;
    BlueDisplay1.drawLine(0, tValue, REMOTE_DISPLAY_WIDTH - 1, tValue, COLOR_VOLTAGE_PICKER);
    LastPickerValue = aValue;

    float tVoltage = getFloatFromDisplayValue(tValue);
    snprintf(sStringBuffer, sizeof sStringBuffer, "%6.*fV", RangePrecision[MeasurementControl.DisplayRangeIndex] + 1, tVoltage);

    int tYPos = SLIDER_VPICKER_INFO_SHORT_Y;
    if (DisplayControl.showInfoMode == INFO_MODE_LONG_INFO) {
        tYPos = SLIDER_VPICKER_INFO_LONG_Y;
    }
// print value
    BlueDisplay1.drawText(SLIDER_VPICKER_INFO_X, tYPos, sStringBuffer, FONT_SIZE_INFO_SHORT, COLOR_BLACK,
    COLOR_INFO_BACKGROUND);
}

/**
 * 3ms for FFT, 9ms complete with -OS
 */
void doShowFFT(BDButton * aTheTouchedButton, int16_t aValue) {
    DisplayControl.ShowFFT = aValue;
    aTheTouchedButton->setValue(aValue);

    if (MeasurementControl.isRunning) {
        if (DisplayControl.DisplayPage == DISPLAY_PAGE_SETTINGS) {
            aTheTouchedButton->drawButton();
        }
        if (aValue) {
            // initialize FFTDisplayBuffer
            memset(&DisplayBufferFFT[0], REMOTE_DISPLAY_HEIGHT - 1, sizeof(DisplayBufferFFT));
        } else {
            clearFFTValuesOnDisplay();
        }
    } else if (DisplayControl.DisplayPage == DISPLAY_PAGE_CHART) {
        if (aValue) {
            // compute and draw FFT
            drawFFT();
        } else {
            // show graph data
            redrawDisplay();
        }
    }
}

void doVoltageCalibration(BDButton * aTheTouchedButton, int16_t aValue) {
#ifdef LOCAL_DISPLAY_EXISTS
// TODO use also BlueDisplay getNumber
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);

    BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND, "Enter actual value",
            TEXT_SIZE_22,
            COLOR_RED, COLOR_BACKGROUND_DSO);
    float tOriginalValue = getFloatFromRawValue(MeasurementControl.RawValueAverage);
    snprintf(sStringBuffer, sizeof sStringBuffer, "Current=%fV", tOriginalValue);
    BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, 4 * TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND, sStringBuffer,
            TEXT_SIZE_22,
            COLOR_BLACK, COLOR_INFO_BACKGROUND);

// wait for touch to become active
    do {
        checkAndHandleEvents();
        delayMillis(10);
    }while (!sTouchIsStillDown);

    FeedbackToneOK();
    float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, COLOR_GUI_SOURCE_TIMEBASE);
// check for cancel
    if (!isnan(tNumber) && tNumber != 0) {
        float tOldValue = RawAttenuationFactor[MeasurementControl.DisplayRangeIndex];
        float tNewValue = tOldValue * (tNumber / tOriginalValue);
        RawAttenuationFactor[MeasurementControl.DisplayRangeIndex] = tNewValue;
        initRawToDisplayFactorsAndMaxPeakToPeakValues();

        BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);
        snprintf(sStringBuffer, sizeof sStringBuffer, "Old value=%f", tOldValue);
        BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND, sStringBuffer,
                TEXT_SIZE_22,
                COLOR_BLACK, COLOR_INFO_BACKGROUND);
        snprintf(sStringBuffer, sizeof sStringBuffer, "New value=%f", tNewValue);
        BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, 4 * TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND, sStringBuffer,
                TEXT_SIZE_22,
                COLOR_BLACK, COLOR_INFO_BACKGROUND);

        // wait for touch to become active
        do {
            delayMillis(10);
            checkAndHandleEvents();
        }while (!sNothingTouched);

        sDisableTouchUpOnce = true;
        FeedbackToneOK();
    }

    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);
    drawDSOMoreSettingsPage();
#endif
}

#ifdef LOCAL_FILESYSTEM_EXISTS
const char sDSODataFileName[] = "DSO-data.bin";
void doStoreLoadAcquisitionData(BDButton * aTheTouchedButton, int16_t aMode) {
    int tFeedbackType = FEEDBACK_TONE_LONG_ERROR;
    if (!MeasurementControl.isRunning && MICROSD_isCardInserted()) {
        FIL tFile;
        FRESULT tOpenResult;
        UINT tCount;

        if (aMode == MODE_LOAD) {
            // Load new data from file
            tOpenResult = f_open(&tFile, sDSODataFileName, FA_OPEN_EXISTING | FA_READ);
            if (tOpenResult == FR_OK) {
                f_read(&tFile, &MeasurementControl, sizeof(MeasurementControl), &tCount);
                f_read(&tFile, &DataBufferControl, sizeof(DataBufferControl), &tCount);
                f_close(&tFile);
                DisplayControl.showInfoMode = INFO_MODE_LONG_INFO;
                // redraw display corresponding to new values
                redrawDisplay();
                printInfo();
                tFeedbackType = FEEDBACK_TONE_NO_ERROR;
            }
        } else {
            // Store
            tOpenResult = f_open(&tFile, sDSODataFileName, FA_CREATE_ALWAYS | FA_WRITE);
            if (tOpenResult == FR_OK) {
                f_write(&tFile, &MeasurementControl, sizeof(MeasurementControl), &tCount);
                f_write(&tFile, &DataBufferControl, sizeof(DataBufferControl), &tCount);
                f_close(&tFile);
                tFeedbackType = FEEDBACK_TONE_NO_ERROR;
            }
        }
    }
    FeedbackTone(tFeedbackType);
}
#endif

#ifdef LOCAL_DISPLAY_EXISTS
/*
 * Toggle between pixel and line draw mode (for data chart)
 */
void doDrawMode(BDButton * aTheTouchedButton, int16_t aValue) {
// erase old chart in old mode
    drawDataBuffer(NULL, REMOTE_DISPLAY_WIDTH, DisplayControl.EraseColor, 0, DRAW_MODE_CLEAR_OLD,
            MeasurementControl.isEffectiveMinMaxMode);
// switch mode
    if (!DisplayControl.drawPixelMode) {
        aTheTouchedButton->setCaptionAndDraw(DrawModeButtonStringPixel);
    } else {
        aTheTouchedButton->setCaptionAndDraw(DrawModeButtonStringLine);
    }
    DisplayControl.drawPixelMode = !DisplayControl.drawPixelMode;
}

/*
 * Toggles ADS7846Test on / off
 */
void doADS7846TestOnOff(BDButton * aTheTouchedButton, int16_t aValue) {
    aValue = !aValue;
    MeasurementControl.ADS7846ChannelsAsDatasource = aValue;
    MeasurementControl.ADCInputMUXChannelIndex = 0;
    if (aValue) {
        // ADS7846 Test on
        doAcDcMode(&TouchButtonACRangeOnOff, true);
        MeasurementControl.ChannelHasActiveAttenuator = false;
        setDisplayRange(NO_ATTENUATOR_MAX_DISPLAY_RANGE_INDEX);
    } else {
        MeasurementControl.ChannelHasActiveAttenuator = true;
    }
    aTheTouchedButton->setValueAndDraw(aValue);
}
#endif

void doShowFrequencyPage(BDButton * aTheTouchedButton, int16_t aValue) {
    DisplayControl.DisplayPage = DISPLAY_PAGE_FREQUENCY;
    startFrequencyGeneratorPage();
}

void doShowSystemInfoPage(BDButton * aTheTouchedButton, int16_t aValue) {
    DisplayControl.DisplayPage = DISPLAY_PAGE_SYST_INFO;
    startSystemInfoPage();
}

/*
 * default handle for back button
 */
void doDefaultBackButton(BDButton * aTheTouchedButton, int16_t aValue) {
    sBackButtonPressed = true;
}

/***********************************************************************
 * GUI initialisation and drawing stuff
 ***********************************************************************/
void initDSOGUI(void) {
    BlueDisplay1.setButtonsGlobalFlags(FLAG_BUTTON_GLOBAL_USE_UP_EVENTS_FOR_BUTTONS); // since swipe recognition needs it

    int tPosY = 0;

    /*
     * Start page
     */
// 1. row
// Button for Singleshot
    TouchButtonSingleshot.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
    COLOR_GUI_CONTROL, "Singleshot", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doStartSingleshot);

// standard main home button here

// 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;

#ifdef LOCAL_FILESYSTEM_EXISTS
    TouchButtonStore.init(0, tPosY, BUTTON_WIDTH_5, BUTTON_HEIGHT_4, COLOR_GUI_SOURCE_TIMEBASE,
            "Store", TEXT_SIZE_11, BUTTON_FLAG_NO_BEEP_ON_TOUCH, MODE_STORE, &doStoreLoadAcquisitionData);

    TouchButtonLoad.init(BUTTON_WIDTH_5_POS_2, tPosY, BUTTON_WIDTH_5, BUTTON_HEIGHT_4,
            COLOR_GUI_SOURCE_TIMEBASE, "Load", TEXT_SIZE_11, BUTTON_FLAG_NO_BEEP_ON_TOUCH, MODE_LOAD, &doStoreLoadAcquisitionData);
#endif

// big start stop button
    TouchButtonStartStopDSOMeasurement.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3,
            (2 * BUTTON_HEIGHT_4) + BUTTON_DEFAULT_SPACING,
            COLOR_GUI_CONTROL, "Start\nStop", TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doStartStopDSO);

// 4. row
    tPosY += 2 * BUTTON_HEIGHT_4_LINE_2;
// Button for show FFT
    TouchButtonFFT.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_GREEN, "FFT",
    TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH | BUTTON_FLAG_TYPE_TOGGLE_RED_GREEN_MANUAL_REFRESH, DisplayControl.ShowFFT, &doShowFFT);

// Button for settings pages
    TouchButtonDSOSettings.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
    COLOR_GUI_CONTROL, "Settings", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doShowSettingsPage);

    /*
     * Settings page
     */
// 1. row
    tPosY = 0;

// Button for min/max acquisition mode
#ifdef LOCAL_DISPLAY_EXISTS
    TouchButtonMinMaxMode.init(SLIDER_DEFAULT_BAR_WIDTH + 6, tPosY, BUTTON_WIDTH_3 - (SLIDER_DEFAULT_BAR_WIDTH + 6),
            BUTTON_HEIGHT_5, COLOR_GUI_DISPLAY_CONTROL, "", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doMinMaxMode);
#else
    TouchButtonMinMaxMode.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_5,
    COLOR_GUI_DISPLAY_CONTROL, "", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doMinMaxMode);
#endif
// caption is set according internal flag later

// Button for auto trigger on off
    TouchButtonTriggerMode.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_5, COLOR_GUI_TRIGGER, TriggerModeButtonStringAuto, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doTriggerMode);

// Back button for sub pages
    TouchButtonBack.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_5, COLOR_GUI_CONTROL, "Back", TEXT_SIZE_22,
            BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doDefaultBackButton);

// 2. row
    tPosY += BUTTON_HEIGHT_5_LINE_2;

// Button for chart history (erase color)
#ifdef LOCAL_DISPLAY_EXISTS
    TouchButtonChartHistoryOnOff.init(SLIDER_DEFAULT_BAR_WIDTH + 6, tPosY, BUTTON_WIDTH_3 - (SLIDER_DEFAULT_BAR_WIDTH + 6),
            BUTTON_HEIGHT_5, COLOR_GUI_DISPLAY_CONTROL, "", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doChartHistory);
#else
    TouchButtonChartHistoryOnOff.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_5, COLOR_GUI_CONTROL, "History", TEXT_SIZE_11,
            BUTTON_FLAG_DO_BEEP_ON_TOUCH | BUTTON_FLAG_TYPE_AUTO_RED_GREEN, 0, &doChartHistory);
#endif
// caption is set according internal flag later

// AutoRange on off
    TouchButtonAutoRangeOnOff.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_5, COLOR_GUI_TRIGGER,
            AutoRangeButtonStringAuto, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doRangeMode);

// Button for channel 0
    TouchButtonChannels[0].init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_5, BUTTON_AUTO_RED_GREEN_FALSE_COLOR, "",
    TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doChannelSelect);

// Button for channel 1
    TouchButtonChannels[1].init(REMOTE_DISPLAY_WIDTH - BUTTON_WIDTH_6, tPosY,
    BUTTON_WIDTH_6, BUTTON_HEIGHT_5, BUTTON_AUTO_RED_GREEN_FALSE_COLOR, "", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 1,
            &doChannelSelect);

// 3. row
    tPosY += BUTTON_HEIGHT_5_LINE_2;

// Button for pretrigger area show
#ifdef LOCAL_DISPLAY_EXISTS
    TouchButtonShowPretriggerValuesOnOff.init(SLIDER_DEFAULT_BAR_WIDTH + 6, tPosY,
            BUTTON_WIDTH_3 - (SLIDER_DEFAULT_BAR_WIDTH + 6), BUTTON_HEIGHT_5,
            COLOR_BLACK, "Show\nPretrigger", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH | BUTTON_FLAG_TYPE_AUTO_RED_GREEN,
            (DisplayControl.DatabufferPreTriggerDisplaySize != 0), &doShowPretriggerValuesOnOff);
#else
    TouchButtonShowPretriggerValuesOnOff.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_5,
    COLOR_BLACK, "Show\nPretrigger", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH | BUTTON_FLAG_TYPE_AUTO_RED_GREEN,
            (DisplayControl.DatabufferPreTriggerDisplaySize != 0), &doShowPretriggerValuesOnOff);
#endif

// Button for auto offset on off
    TouchButtonAutoOffsetOnOff.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_5,
    COLOR_GUI_TRIGGER, AutoOffsetButtonString0, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doOffsetMode);

// Button for channel 2
    TouchButtonChannels[2].init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_6,
    BUTTON_HEIGHT_5, BUTTON_AUTO_RED_GREEN_FALSE_COLOR, "", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 2, &doChannelSelect);
    setChannelButtonsCaption();

// Button for channel select
    TouchButtonChannelSelect.init(REMOTE_DISPLAY_WIDTH - BUTTON_WIDTH_6, tPosY,
    BUTTON_WIDTH_6, BUTTON_HEIGHT_5, BUTTON_AUTO_RED_GREEN_FALSE_COLOR, StringChannel3, TEXT_SIZE_11,
            BUTTON_FLAG_DO_BEEP_ON_TOUCH, 42, &doChannelSelect);

// 4. row
    tPosY += BUTTON_HEIGHT_5_LINE_2;

// Button for trigger line mode
    TouchButtonDrawModeTriggerLine.init(0, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_5, COLOR_GUI_DISPLAY_CONTROL, DrawModeTriggerLineButtonString, TEXT_SIZE_11,
            BUTTON_FLAG_DO_BEEP_ON_TOUCH | BUTTON_FLAG_TYPE_AUTO_RED_GREEN, DisplayControl.showTriggerInfoLine,
            &doDrawModeTriggerLine);

// Button for slope page
    TouchButtonSlope.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_5,
    COLOR_GUI_TRIGGER, SlopeButtonString, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doTriggerSlope);

    TouchButtonShowSystemInfo.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_5, COLOR_GREEN, "System\ninfo", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH | BUTTON_FLAG_TYPE_AUTO_RED_GREEN,
            DisplayControl.ShowFFT, &doShowSystemInfoPage);

// 5. row
    tPosY = BUTTON_HEIGHT_5_LINE_5;

// Frequency
    TouchButtonFrequencyPage.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_5, COLOR_RED, "Frequency\nGenerator", TEXT_SIZE_11,
            BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doShowFrequencyPage);

// Button for AC range
    TouchButtonACRangeOnOff.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_5, COLOR_GUI_TRIGGER, "", TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doAcDcMode);
// caption is set according internal flag later

// Button for more-settings pages
    TouchButtonDSOMoreSettings.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_5,
    COLOR_GUI_CONTROL, "More", TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doShowMoreSettingsPage);

    /*
     * More Settings page
     */
// 1. row
    tPosY = 0;
// Buttons for voltage calibration
    TouchButtonCalibrateVoltage.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
    COLOR_GUI_SOURCE_TIMEBASE, "Calibrate U", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doVoltageCalibration);
    setButtonCaptions();

    /*
     * SLIDER
     */
// make slider slightly visible
// slider for voltage picker
    TouchSliderVoltagePicker.init(SLIDER_VPICKER_POS_X, 0, SLIDER_SIZE, REMOTE_DISPLAY_HEIGHT, REMOTE_DISPLAY_HEIGHT, 0, 0,
    COLOR_VOLTAGE_PICKER_SLIDER, FLAG_SLIDER_VALUE_BY_CALLBACK, &doVoltagePicker);
    TouchSliderVoltagePicker.setBarBackgroundColor( COLOR_VOLTAGE_PICKER_SLIDER);

// slider for trigger level
    TouchSliderTriggerLevel.init(SLIDER_TLEVEL_POS_X, 0, SLIDER_SIZE, REMOTE_DISPLAY_HEIGHT, REMOTE_DISPLAY_HEIGHT, 0, 0,
    COLOR_TRIGGER_SLIDER, FLAG_SLIDER_VALUE_BY_CALLBACK, &doTriggerLevel);
    TouchSliderTriggerLevel.setBarBackgroundColor( COLOR_TRIGGER_SLIDER);

#ifdef LOCAL_DISPLAY_EXISTS
// 2. row
// Button for switching draw mode - line/pixel
    TouchButtonDrawModeLinePixel.init(0, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_3,
            BUTTON_HEIGHT_4, COLOR_GUI_DISPLAY_CONTROL, DrawModeButtonStringLine, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0,
            &doDrawMode);

// Button for ADS7846 channel
    TouchButtonADS7846TestOnOff.init(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "ADS7846",
            TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH | BUTTON_FLAG_TYPE_AUTO_RED_GREEN, MeasurementControl.ADS7846ChannelsAsDatasource,
            &doADS7846TestOnOff);

    /*
     * Backlight slider
     */
    TouchSliderBacklight.init(0, 0, SLIDER_DEFAULT_BAR_WIDTH, BACKLIGHT_MAX_VALUE, BACKLIGHT_MAX_VALUE, getBacklightValue(),
            COLOR_BLUE, COLOR_GREEN, FLAG_SLIDER_VERTICAL_SHOW_NOTHING, &doBacklightSlider);
#endif
}

void activateCommonPartOfGui(void) {
    TouchSliderVoltagePicker.drawSlider();
    if (MeasurementControl.TriggerMode == TRIGGER_MODE_MANUAL) {
        TouchSliderTriggerLevel.drawSlider();
    }

    TouchButtonDSOSettings.activate();
    TouchButtonStartStopDSOMeasurement.activate();
    TouchButtonFFT.activate();
#ifdef LOCAL_DISPLAY_EXISTS
    TouchButtonMainHome.activate();
#endif
}

/**
 * activate elements active while measurement is stopped and data is shown
 */
void activateAnalysisOnlyPartOfGui(void) {
    TouchButtonSingleshot.activate();

#ifdef LOCAL_FILESYSTEM_EXISTS
    TouchButtonStore.activate();
    TouchButtonLoad.activate();
#endif
}

/**
 * activate elements active while measurement is running
 */
void activateRunningOnlyPartOfGui(void) {
    TouchButtonChartHistoryOnOff.activate();
}

/**
 * draws elements active while measurement is stopped and data is shown
 */
void drawCommonPartOfGui(void) {
    TouchSliderVoltagePicker.drawSlider();

//1. Row
#ifdef LOCAL_DISPLAY_EXISTS
    TouchButtonMainHome.drawButton();
#endif
    TouchButtonDSOSettings.drawButton();

//2. Row
    TouchButtonStartStopDSOMeasurement.drawButton();

// 4. Row
    TouchButtonFFT.drawButton();
}

/**
 * draws elements active while measurement is stopped and data is shown
 */
void drawAnalysisOnlyPartOfGui(void) {
//1. Row
    TouchButtonSingleshot.drawButton();

//2. Row
#ifdef LOCAL_FILESYSTEM_EXISTS
    TouchButtonStore.drawButton();
    TouchButtonLoad.drawButton();
#endif

    BlueDisplay1.drawText(BUTTON_WIDTH_3, BUTTON_HEIGHT_4_LINE_3 - BUTTON_DEFAULT_SPACING + TEXT_SIZE_22_ASCEND, "\xABScale\xBB",
    TEXT_SIZE_22, COLOR_YELLOW, COLOR_BACKGROUND_DSO);
    BlueDisplay1.drawText(BUTTON_WIDTH_3, BUTTON_HEIGHT_4_LINE_4 + BUTTON_DEFAULT_SPACING + TEXT_SIZE_22_ASCEND, "\xABScroll\xBB",
    TEXT_SIZE_22, COLOR_GREEN, COLOR_BACKGROUND_DSO);
}

/**
 * draws elements active while measurement is running
 */
void drawRunningOnlyPartOfGui(void) {
    if (MeasurementControl.TriggerMode == TRIGGER_MODE_MANUAL) {
        TouchSliderTriggerLevel.drawSlider();
    }

    if (!MeasurementControl.RangeAutomatic || MeasurementControl.OffsetMode == OFFSET_MODE_MANUAL) {
#ifdef LOCAL_DISPLAY_EXISTS
        BlueDisplay1.drawMLText(TEXT_SIZE_11_WIDTH, TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND, "\xD4\nR\na\nn\ng\ne\n\xD5",
                TEXT_SIZE_22, COLOR_GUI_TRIGGER, COLOR_NO_BACKGROUND);
#else
        BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND, "\xD4\nR\na\nn\ng\ne\n\xD5",
        TEXT_SIZE_22, COLOR_GUI_TRIGGER, COLOR_NO_BACKGROUND);
#endif
    }

    if (MeasurementControl.OffsetMode != OFFSET_MODE_0_VOLT) {
#ifdef LOCAL_DISPLAY_EXISTS
        BlueDisplay1.drawMLText(BUTTON_WIDTH_3_POS_3 - TEXT_SIZE_22_WIDTH, TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND,
                "\xD4\nO\nf\nf\ns\ne\nt\n\xD5", TEXT_SIZE_22, COLOR_GUI_TRIGGER, COLOR_NO_BACKGROUND);
#else
        BlueDisplay1.drawText(BUTTON_WIDTH_3_POS_3 - TEXT_SIZE_22_WIDTH, TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND,
                "\xD4\nO\nf\nf\ns\ne\nt\n\xD5", TEXT_SIZE_22, COLOR_GUI_TRIGGER, COLOR_NO_BACKGROUND);
#endif
    }

    BlueDisplay1.drawText(BUTTON_WIDTH_8, BUTTON_HEIGHT_4_LINE_4 - TEXT_SIZE_22_DECEND, "\xABTimeBase\xBB",
    TEXT_SIZE_22,
    COLOR_GUI_SOURCE_TIMEBASE, COLOR_BACKGROUND_DSO);

    TouchButtonDSOSettings.drawButton();
    TouchButtonFFT.drawButton();
    TouchButtonChartHistoryOnOff.drawButton();

}

/**
 * draws elements active for settings page
 */
void drawDSOSettingsPage(void) {
// do not clear screen here since gui is refreshed periodically while DSO is running
    BDButton::deactivateAllButtons();
    BDSlider::deactivateAllSliders();

//1. Row
    if (MeasurementControl.ChannelHasACDCSwitch) {
        TouchButtonACRangeOnOff.drawButton();
    }
    TouchButtonTriggerMode.drawButton();
    TouchButtonDSOMoreSettings.drawButton();

//2. Row
    TouchButtonSlope.drawButton();
    TouchButtonAutoRangeOnOff.drawButton();
    if (MeasurementControl.OffsetMode == OFFSET_MODE_AUTOMATIC) {
        // lock button in OFFSET_MODE_AUTOMATIC
        TouchButtonAutoRangeOnOff.deactivate();
    }
    TouchButtonAutoOffsetOnOff.drawButton();

    int16_t tButtonColor;
    /*
     * Determine colors for 3 fixed channel buttons
     */
    for (int i = 0; i < NUMBER_OF_CHANNEL_WITH_FIXED_ATTENUATOR; ++i) {
        if (i == MeasurementControl.ADCInputMUXChannelIndex) {
            tButtonColor = BUTTON_AUTO_RED_GREEN_TRUE_COLOR;
        } else {
            tButtonColor = BUTTON_AUTO_RED_GREEN_FALSE_COLOR;
        }
        TouchButtonChannels[i].setButtonColorAndDraw(tButtonColor);
    }
    if (MeasurementControl.ADCInputMUXChannelIndex >= NUMBER_OF_CHANNEL_WITH_FIXED_ATTENUATOR) {
        tButtonColor = BUTTON_AUTO_RED_GREEN_TRUE_COLOR;
    } else {
        tButtonColor = BUTTON_AUTO_RED_GREEN_FALSE_COLOR;
    }
    TouchButtonChannelSelect.setButtonColorAndDraw(tButtonColor);

    TouchButtonDrawModeTriggerLine.drawButton();
//3. Row
    TouchButtonShowPretriggerValuesOnOff.drawButton();
    TouchButtonChannelSelect.drawButton();
    TouchButtonMinMaxMode.drawButton();

// 4. Row
    TouchButtonChartHistoryOnOff.drawButton();
    TouchButtonBack.drawButton();

#ifdef LOCAL_DISPLAY_EXISTS
    TouchSliderBacklight.drawSlider();
#endif
    TouchButtonShowSystemInfo.drawButton(); // 4. row
    TouchButtonFrequencyPage.drawButton(); // 5. row
}

void startDSOSettingsPage(void) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);
    drawDSOSettingsPage();
}

/*
 * show gui of settings screen
 */
void doShowSettingsPage(BDButton * aTheTouchedButton, int16_t aValue) {
    DisplayControl.DisplayPage = DISPLAY_PAGE_SETTINGS;
    startDSOSettingsPage();
}

void drawDSOMoreSettingsPage(void) {
// do not clear screen here since gui is refreshed periodically while DSO is running
    BDButton::deactivateAllButtons();
    BDSlider::deactivateAllSliders();
//1. Row
    TouchButtonCalibrateVoltage.drawButton();
    TouchButtonBack.drawButton();

#ifdef LOCAL_DISPLAY_EXISTS
//2. Row
    TouchButtonDrawModeLinePixel.drawButton();
    TouchButtonADS7846TestOnOff.drawButton();
#endif
}

void startDSOMoreSettingsPage(void) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);
    drawDSOMoreSettingsPage();
}

/*
 * show gui of more settings screen
 */
void doShowMoreSettingsPage(BDButton * aTheTouchedButton, int16_t aValue) {
    DisplayControl.DisplayPage = DISPLAY_PAGE_MORE_SETTINGS;
    startDSOMoreSettingsPage();
}

/**
 * Clears and redraws all elements according to IsRunning/DisplayMode
 * @param doClearbefore if display mode = SHOW_GUI_* the display is cleared anyway
 */
void redrawDisplay() {
    clearDisplayAndDisableButtonsAndSliders(COLOR_BACKGROUND_DSO);

    if (MeasurementControl.isRunning) {
        if (DisplayControl.DisplayPage >= DISPLAY_PAGE_SETTINGS) {
            drawDSOSettingsPage();
        } else {
            activateCommonPartOfGui();
            activateRunningOnlyPartOfGui();
            // initialize FFTDisplayBuffer
            memset(&DisplayBufferFFT[0], REMOTE_DISPLAY_HEIGHT - 1, sizeof(DisplayBufferFFT));
        }
        // show grid and labels - not really needed, since after MILLIS_BETWEEN_INFO_OUTPUT it is done by loop
        drawGridLinesWithHorizLabelsAndTriggerLine();
        printInfo();
    } else {
        // measurement stopped -> analysis mode
        if (DisplayControl.DisplayPage == DISPLAY_PAGE_START) {
            // show analysis gui elements
            drawCommonPartOfGui();
            drawAnalysisOnlyPartOfGui();
        } else if (DisplayControl.DisplayPage == DISPLAY_PAGE_CHART) {
            activateCommonPartOfGui();
            activateAnalysisOnlyPartOfGui();
            // show grid and labels and chart
            drawGridLinesWithHorizLabelsAndTriggerLine();
            drawMinMaxLines();
            drawDataBuffer(DataBufferControl.DataBufferDisplayStart, REMOTE_DISPLAY_WIDTH, COLOR_DATA_HOLD, 0,
            DRAW_MODE_REGULAR, MeasurementControl.isEffectiveMinMaxMode);
            printInfo();
        } else {
            drawDSOSettingsPage();
        }
    }
}
