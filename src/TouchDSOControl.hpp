/**
 * TouchDSOControl.hpp
 *
 * contains the flow control functions for DSO.
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

#ifndef _TOUCH_DSO_CONTROL_HPP
#define _TOUCH_DSO_CONTROL_HPP

#include "TouchDSO.h"
#include "TouchDSOGui.hpp" // include sources
#include "TouchDSODisplay.hpp" // include sources
#include "TouchDSOAcquisition.hpp" // include sources
#if !defined(SUPPORT_LOCAL_DISPLAY)
#include "EventHandler.h"
#include "utils.h" // for showRTCTimeEverySecond()
#endif

#ifdef LOCAL_FILESYSTEM_EXISTS
extern "C" {
#include "ff.h"
}
#endif

/**********************
 * Buttons
 *********************/
BDButton TouchButtonShowSystemInfo;

#ifdef LOCAL_FILESYSTEM_EXISTS
BDButton TouchButtonLoad;
BDButton TouchButtonStore;
#define MODE_LOAD 0
#define MODE_STORE 1
#endif

#if defined(SUPPORT_LOCAL_DISPLAY)
BDButton *const TouchButtonsDSO[] = { &TouchButtonBack, &TouchButtonStartStopDSOMeasurement, &TouchButtonTriggerMode,
        &TouchButtonAutoRangeOnOff, &TouchButtonAutoOffsetMode, &TouchButtonChannelSelect, &TouchButtonDrawModeLinePixel,
        &TouchButtonSettingsPage, &TouchButtonDSOMoreSettings, &TouchButtonSingleshot, &TouchButtonSlope,
        &TouchButtonADS7846TestOnOff, &TouchButtonMinMaxMode, &TouchButtonChartHistoryOnOff,
#ifdef LOCAL_FILESYSTEM_EXISTS
    &TouchButtonLoad, &TouchButtonStore,
#endif
        &TouchButtonFFT, &TouchButtonCalibrateVoltage, &TouchButtonAcDc, &TouchButtonShowPretriggerValuesOnOff };
#endif

/***********************
 *   Loop control
 ***********************/
#if !defined(SUPPORT_LOCAL_DISPLAY)
uint32_t sMillisOfLastLoop;
unsigned int sMillisSinceLastInfoOutput;
#endif
#define MILLIS_BETWEEN_INFO_OUTPUT ONE_SECOND_MILLIS

/*******************************************************************************************
 * Function declaration section
 *******************************************************************************************/

#ifdef LOCAL_FILESYSTEM_EXISTS
static void doStoreLoadAcquisitionData(BDButton * aTheTouchedButton, int16_t aMode);
#endif
void initDSOGUI(void);

void doStartStopDSO(BDButton *aTheTouchedButton, int16_t aValue);

/*******************************************************************************************
 * Program code starts here
 * Setup section
 *******************************************************************************************/
void initDSOPage(void) {
// cannot be done statically
    DisplayControl.ShowFFT = false;  // before initDSOGUI()
    DisplayControl.DatabufferPreTriggerDisplaySize = (2 * DATABUFFER_DISPLAY_RESOLUTION);
    initAcquisition();
#if !defined(SUPPORT_LOCAL_DISPLAY)
    initDSOGUI();
    // for local display init is made at start of page
#endif
}

void startDSOPage(void) {
    DisplayControl.DisplayPage = DSO_PAGE_START;

    DisplayControl.showTriggerInfoLine = false;
    DisplayControl.showInfoMode = INFO_MODE_SHORT_INFO;
    DisplayControl.EraseColor = COLOR_BACKGROUND_DSO;
    DisplayControl.XScale = 0;
    resetAcquisition(); // sets MinMaxMode

#if defined(SUPPORT_LOCAL_DISPLAY)
    initDSOGUI();
    DisplayControl.drawPixelMode = false;
#endif

// show page
    redrawDisplay();

    // use max of DATABUFFER_PRE_TRIGGER_SIZE * sizeof(uint16_t) AND sizeof(float32_t) * 2 * FFT_SIZE
    // 2k
    TempBufferForPreTriggerAdjustAndFFT = malloc(sizeof(float32_t) * 2 * FFT_SIZE);
    if (TempBufferForPreTriggerAdjustAndFFT == NULL) {
        failParamMessage(sizeof(float32_t) * 2 * FFT_SIZE, "malloc() fails");
    }

    registerRedrawCallback(&redrawDisplay);
    registerLongTouchDownCallback(&doLongTouchDownDSO, TOUCH_STANDARD_LONG_TOUCH_TIMEOUT_MILLIS);
    registerSwipeEndCallback(&doSwipeEndDSO);
    // use touch up for info in order not to interfere with long touch
    registerTouchUpCallback(&doSwitchInfoModeOnTouchUp);

#if defined(SUPPORT_LOCAL_DISPLAY)
    setDimdelay(3 * BACKLIGHT_DIM_DEFAULT_DELAY_MILLIS);
    // to avoid counting touch move/up for slider.
    sDisableMoveEventsUntilTouchUpIsDone = true;
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

#if defined(SUPPORT_LOCAL_DISPLAY)
    // free buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsDSO) / sizeof(TouchButtonsDSO[0]); ++i) {
        (*TouchButtonsDSO[i]).deinit();
    }
    for (unsigned int i = 0; i < NUMBER_OF_CHANNELS_WITH_FIXED_ATTENUATOR; ++i) {
        TouchButtonChannels[i].deinit();
    }
    TouchSliderVoltagePicker.deinit();
    TouchSliderTriggerLevel.deinit();
    TouchSliderBacklight.deinit();
#endif

#if defined(SUPPORT_LOCAL_DISPLAY)
    // restore normal behavior
    BDButton::setGlobalFlags(FLAG_BUTTON_GLOBAL_USE_DOWN_EVENTS_FOR_BUTTONS);

    // restore flag, since it is not reset by button touch
    sDisableMoveEventsUntilTouchUpIsDone = false;

    setDimdelay(BACKLIGHT_DIM_DEFAULT_DELAY_MILLIS);
#endif
}

/************************************************************************
 * main loop - 32 microseconds
 ************************************************************************/
void loopDSOPage(void) {
    uint32_t tMillis, tMillisOfLoop;
    static bool sDoInfoOutput = true;

    /*
     * Must be done at beginning of loop or at least before handling of sBackButtonPressed,
     * otherwise back buttons on sub pages will return to main menu
     */
    checkAndHandleEvents();

    tMillis = millis();
    tMillisOfLoop = tMillis - sMillisOfLastLoop;
    sMillisOfLastLoop = tMillis;
    sMillisSinceLastInfoOutput += tMillisOfLoop;
    if (sMillisSinceLastInfoOutput > MILLIS_BETWEEN_INFO_OUTPUT) {
        sMillisSinceLastInfoOutput = 0;
        sDoInfoOutput = true;
    }

    if (MeasurementControl.isRunning) {
#if defined(SUPPORT_LOCAL_DISPLAY)
        if (MeasurementControl.ADS7846ChannelsAsDatasource) {
            readADS7846Channels();
            // check if button pressed - to process stop and channel buttons here
            TouchPanel.readData();
            if (TouchPanel.mPressure > MIN_REASONABLE_PRESSURE) {
                LocalTouchButton::checkAllButtons(TouchPanel.getCurrentX(), TouchPanel.getCurrentY());
            }
        }
#endif
        if (DataBufferControl.DataBufferFull) {
            /*
             * Data (from InterruptServiceRoutine or DMA) is ready
             */
            if (sDoInfoOutput) {
                sDoInfoOutput = false;
                if (DisplayControl.DisplayPage == DSO_PAGE_CHART) {
                    drawGridLinesWithHorizLabelsAndTriggerLine();
                    if (DisplayControl.showInfoMode != INFO_MODE_NO_INFO) {
                        computePeriodFrequency(); // compute values only once
                        printInfo();
                    }
                } else if (DisplayControl.DisplayPage == DSO_PAGE_SETTINGS) {
                    // refresh buttons
                    drawDSOSettingsPage();
                } else if (DisplayControl.DisplayPage == DSO_PAGE_FREQUENCY) {
                    // refresh buttons
                    drawFrequencyGeneratorPage();
#ifndef AVR
                } else if (DisplayControl.DisplayPage == DSO_PAGE_MORE_SETTINGS) {
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
#if defined(SUPPORT_LOCAL_DISPLAY)
                    DisplayControl.drawPixelMode = false;
#endif
#if defined(SUPPORT_LOCAL_DISPLAY) && defined(DISABLE_REMOTE_DISPLAY)
                    LocalTouchButton::playFeedbackTone(false);
#else
                    BDButton::playFeedbackTone(false);
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
                DisplayControl.TriggerLevelDisplayValue = getDisplayFromRawInputValue(MeasurementControl.RawTriggerLevel);
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
                drawDataBuffer((uint16_t*) TempBufferForPreTriggerAdjustAndFFT, DATABUFFER_PRE_TRIGGER_SIZE,
                COLOR_DATA_PRETRIGGER, COLOR_BACKGROUND_DSO, DRAW_MODE_REGULAR, false);
            }
            printInfo();
        }
    } else {
        // Analyze mode here
        if (sDoInfoOutput && DisplayControl.DisplayPage == DSO_PAGE_CHART && DisplayControl.showInfoMode != INFO_MODE_NO_INFO
                && DisplayControl.DisplaySubPage != DSO_SUB_PAGE_FFT) {
            sDoInfoOutput = false;
            /*
             * show time
             */
            showRTCTimeEverySecond(0, FONT_SIZE_INFO_LONG_ASC + 4 * FONT_SIZE_INFO_LONG, COLOR16_RED, COLOR_BACKGROUND_DSO);
        }
    }
    /*
     * Handle Sub-Pages
     */
    if (DisplayControl.DisplayPage == DSO_PAGE_SETTINGS) {
        if (sBackButtonPressed) {
            sBackButtonPressed = false;
            DisplayControl.DisplayPage = DSO_PAGE_CHART;
            redrawDisplay();
        }
    } else if (DisplayControl.DisplayPage == DSO_PAGE_FREQUENCY) {
        if (sBackButtonPressed) {
            sBackButtonPressed = false;
            stopFrequencyGeneratorPage();
            DisplayControl.DisplayPage = DSO_PAGE_SETTINGS;
            redrawDisplay();
        } else {
            //not needed here, because is contains only checkAndHandleEvents()
            // loopFrequencyGeneratorPage();
        }
    } else if (DisplayControl.DisplayPage == DSO_PAGE_SYST_INFO) {
        if (sBackButtonPressed) {
            sBackButtonPressed = false;
            stopSystemInfoPage();
            DisplayControl.DisplayPage = DSO_PAGE_SETTINGS;
            redrawDisplay();
        } else {
            // refresh buttons and info
            loopSystemInfoPage();
        }
    } else if (DisplayControl.DisplayPage == DSO_PAGE_MORE_SETTINGS) {
        if (sBackButtonPressed) {
            sBackButtonPressed = false;
            DisplayControl.DisplayPage = DSO_PAGE_SETTINGS;
            redrawDisplay();
        }
    }

}
/* Main loop end */

/************************************************************************
 * Button handler section
 ************************************************************************/

void doAcDcMode(BDButton *aTheTouchedButton, int16_t aValue) {
    setACMode(!MeasurementControl.isACMode);
    setACModeButtonCaption();
    aTheTouchedButton->drawButton();
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

void doStartStopDSO(BDButton *aTheTouchedButton, int16_t aValue) {
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
            uint16_t *tEndPointer = DataBufferControl.DataBufferNextInPointer;
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
    } else {
        /*
         *  Start here in normal mode (reset single shot mode)
         */
        MeasurementControl.isSingleShotMode = false; // return to continuous  mode
        DisplayControl.DisplayPage = DSO_PAGE_CHART;
        prepareForStart();
    }
}

/*
 * Compute new attenuation factor for this voltage range.
 *
 * Here we show the current measured voltage (average voltage)
 * We can then enter the actual voltage value for it, or cancel with cancel button of numberpad.
 * TODO Create a separate page with "Enter new value" and "Cancel" buttons for voltage calibration and store value permanently
 */
void doVoltageCalibration(BDButton *aTheTouchedButton, int16_t aValue) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);
    float tOriginalValue = getFloatFromRawValue(MeasurementControl.RawValueAverage);
    if (tOriginalValue < 1.0) {
        BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND,
                "Average voltage must be > 1 volt\nto do calibration",
                TEXT_SIZE_11, COLOR16_RED, COLOR_BACKGROUND_DSO);
        delay(2000);
    } else {

        BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND, "Enter actual voltage",
        TEXT_SIZE_22, COLOR16_RED, COLOR_BACKGROUND_DSO);
        snprintf(sStringBuffer, sizeof sStringBuffer, "Current average voltage=%fV", tOriginalValue);
        BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, 4 * TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND, sStringBuffer, TEXT_SIZE_11,
        COLOR16_BLACK, COLOR_INFO_BACKGROUND);

        // Blocking wait for touch to become active
        do {
            checkAndHandleEvents();
            delay(10);
        } while (!sTouchIsStillDown);

#  if defined(SUPPORT_ONLY_LOCAL_DISPLAY)
        LocalTouchButton::playFeedbackTone(false);
#  else
        BDButton::playFeedbackTone(false);
#  endif

        /*
         * Get actual voltage
         */
        float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, COLOR_GUI_SOURCE_TIMEBASE);

        // Check for cancel
        if (!isnan(tNumber) && tNumber != 0) {
            /*
             * Compute new attenuation factor for this voltage range
             */
            float tOldValue = RawAttenuationFactor[MeasurementControl.DisplayRangeIndex];
            float tChangeFactor = (tNumber / tOriginalValue);
            float tNewValue = tOldValue * tChangeFactor;

            BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);
            snprintf(sStringBuffer, sizeof sStringBuffer, "Old attenuation factor=%f", tOldValue);
            BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND, sStringBuffer, TEXT_SIZE_11,
            COLOR16_BLACK, COLOR_INFO_BACKGROUND);
            snprintf(sStringBuffer, sizeof sStringBuffer, "New attenuation factor=%f", tNewValue);
            BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, 4 * TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND, sStringBuffer, TEXT_SIZE_11,
            COLOR16_BLACK, COLOR_INFO_BACKGROUND);

            /*
             * Check for plausibility of entry
             */
            if (0.6 < tChangeFactor || tChangeFactor < 1.5) {
                RawAttenuationFactor[MeasurementControl.DisplayRangeIndex] = tNewValue;
                initRawToDisplayFactorsAndMaxPeakToPeakValues();
            } else {
                BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND,
                        "New value is > 1.5 or < 0.6 * old value\n -> cancel", TEXT_SIZE_11,
                        COLOR16_BLACK, COLOR_INFO_BACKGROUND);
            }

            // Blocking wait for touch to become active
            do {
                delay(10);
                checkAndHandleEvents();
            } while (sTouchIsStillDown);

            sDisableTouchUpOnce = true; // to avoid to interpret touch up for a button of DSOMoreSettingsPage
#  if defined(SUPPORT_LOCAL_DISPLAY) && defined(DISABLE_REMOTE_DISPLAY)
            LocalTouchButton::playFeedbackTone(false);
#  else
            BDButton::playFeedbackTone(false);
#  endif
        }
    }

    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);
    drawDSOMoreSettingsPage();
#endif
}

/***********************************************************************
 * For future use
 ***********************************************************************/
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
                tFeedbackType = FEEDBACK_TONE_OK;
            }
        } else {
            // Store
            tOpenResult = f_open(&tFile, sDSODataFileName, FA_CREATE_ALWAYS | FA_WRITE);
            if (tOpenResult == FR_OK) {
                f_write(&tFile, &MeasurementControl, sizeof(MeasurementControl), &tCount);
                f_write(&tFile, &DataBufferControl, sizeof(DataBufferControl), &tCount);
                f_close(&tFile);
                tFeedbackType = FEEDBACK_TONE_OK;
            }
        }
    }
    FeedbackTone(tFeedbackType);
}
#endif

/*
 * Switch trigger line mode on and off (for data chart)
 */
void doDrawModeTriggerLine(BDButton *aTheTouchedButton, int16_t aValue) {
// switch mode
    if (!aValue && MeasurementControl.isRunning) {
// erase old chart in old mode
        drawDataBuffer(NULL, REMOTE_DISPLAY_WIDTH, DisplayControl.EraseColor, 0, DRAW_MODE_CLEAR_OLD,
                MeasurementControl.isEffectiveMinMaxMode);
    }
    DisplayControl.showTriggerInfoLine = aValue;
}

#endif // _TOUCH_DSO_CONTROL_HPP
