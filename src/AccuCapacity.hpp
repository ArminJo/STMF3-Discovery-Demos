/**
 *  AccuCapacity.hpp
 *
 *  measures the capacity of 1,2V rechargeable batteries while charging or discharging and shows the voltage and internal resistance graph on LCD screen.
 *
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

#ifndef _ACCU_CAPACITY_HPP
#define _ACCU_CAPACITY_HPP

#include <string.h> // for strlen

char StringDischarging[] = "discharging";

const char *const chartStrings[] = { "Volt", "mOhm", "Both" };
const char *const chartStringsExtern[] = { "Max", "Min", "Both" };

#define NUMBER_OF_PROBES 2

#define STOP_VOLTAGE_TIMES_TEN 10  // 1.0 Volt
#define START_Y_OFFSET 1.0
#define CHARGE_VOLTAGE_MILLIVOLT_DEFAULT 2490
#define SAMPLE_PERIOD_START 30 // in seconds
#define SAMPLE_PERIOD_HIGH_RESOLUTION 10 // in seconds
// Flag to signal end of measurement from ISR to main loop
volatile bool doEndTone = false;
// Flag to signal refresh from Timer Callback (ISR context) to main loop
volatile bool doDisplayRefresh = false;

/**
 * Color scheme
 */
#define COLOR_GUI_CONTROL COLOR16_RED
#define COLOR_GUI_VALUES COLOR16_GREEN
#define COLOR_GUI_DISPLAY_CONTROL COLOR16_YELLOW
// 3 different pages
#define PAGE_MAIN 0
#define PAGE_CHART 1
#define PAGE_SETTINGS 2
int ActualPage = PAGE_MAIN;

uint16_t const ProbeColors[NUMBER_OF_PROBES] = { COLOR16_RED, COLOR16_BLUE };

// Position for printActualData
#define BASIC_INFO_X 80
#define BASIC_INFO_Y 5

/**
 * Main page
 */
BDButton TouchButtonsMain[NUMBER_OF_PROBES];

/**
 * Chart page
 */

BDButton TouchButtonChartSwitchData;

BDButton TouchButtonClearDataBuffer;

BDButton TouchButtonStartStopCapacityMeasurement;

BDButton TouchButtonNext;
BDButton TouchButtonProbeSettings;

BDButton TouchButtonMode;

void setModeCaption(int16_t aProbeIndex);
void setChargeVoltageCaption(int16_t aProbeIndex);

BDButton TouchButtonExport;

BDButton TouchButtonMain;

BDButton TouchButtonLoadStore;

BDButton TouchButtonAutorepeatSamplePeriodPlus;
BDButton TouchButtonAutorepeatSamplePeriodMinus;

void doSwitchChartOverlay(void);

#define INDEX_OF_NEXT_BUTTON 5      // the index of the "Next" button which has the opposite ProbeColor color
#define INDEX_OF_COLOURED_BUTTONS 9 // from this index on the buttons have ProbeColors
#define INDEX_OF_ACTIVE_BUTTONS 3   // from this index on the buttons are active if invisible
BDButton *const ButtonsChart[] = { &TouchButtonClearDataBuffer, &TouchButtonExport, &TouchButtonLoadStore,
        &TouchButtonProbeSettings, &TouchButtonMain, &TouchButtonNext, &TouchButtonStartStopCapacityMeasurement, &TouchButtonMode,
        &TouchButtonChartSwitchData };

/**
 * Settings page
 */
BDButton TouchButtonSetProbeNumber;
BDButton TouchButtonSetLoadResistor;
BDButton TouchButtonSetStopValue;
BDButton TouchButtonSetChargeVoltage;
BDButton TouchButtonSetExternalAttenuatorFactor;

char StringProbeNumber[] = "Probe number:    "; // with space for the number
#define STRING_PROBE_NUMBER_NUMBER_INDEX 14
char StringLoadResistor[] = "Load resistor:     "; // with space for the number
#define STRING_LOAD_RESISTOR_NUMBER_INDEX 15
char StringStopVoltage[] = "Stop voltage:     "; // with space for the voltage
#define STRING_STOP_VOLTAGE_NUMBER_INDEX 14
char StringStopNominalCapacity[] = "Stop cap.:     "; // with space for the capacity
#define STRING_STOP_CAPACITY_NUMBER_INDEX 11
char StringStoreChargeVoltage[] = "Charge volt.:      ";
#define STRING_STORE_CHARGE_VOLTAGE_NUMBER_INDEX 14
char StringExternalAttenuatorFactor[] = "Volt factor:      ";
#define STRING_EXTERNAL_ATTENUATOR_FACTOR_NUMBER_INDEX 13

/*
 * CHARTS
 */
Chart VoltageChart_0;
Chart VoltageChart_1;
Chart ResistanceChart_0; // Xlabel not used, Xlabel of VoltageChart used instead
Chart ResistanceChart_1; // Xlabel not used, Xlabel of VoltageChart used instead
Chart *const VoltageCharts[NUMBER_OF_PROBES] = { &VoltageChart_0, &VoltageChart_1 };
Chart *const ResistanceCharts[NUMBER_OF_PROBES] = { &ResistanceChart_0, &ResistanceChart_1 };

#define CHART_START_X  (4 * TEXT_SIZE_11_WIDTH)
#define CHART_WIDTH    (BlueDisplay1.getDisplayWidth() - CHART_START_X)
#define CHART_START_Y  (BlueDisplay1.getDisplayHeight() - TEXT_SIZE_11_HEIGHT - 6)
#define CHART_HEIGHT   (CHART_START_Y + 1)
#define CHART_GRID_X_SIZE   22
#define CHART_GRID_Y_SIZE   40
#define CHART_GRID_Y_COUNT   (CHART_HEIGHT / CHART_GRID_Y_SIZE)
#define CHART_MIN_Y_INCREMENT_VOLT (0.1)
#define CHART_MIN_Y_INCREMENT_RESISTANCE 200
#define CHART_Y_LABEL_OFFSET_VOLTAGE 5
#define CHART_Y_LABEL_OFFSET_RESISTANCE 15

#define CHART_DATA_VOLTAGE 0 // max for external
#define CHART_DATA_RESISTANCE 1 // min for external
#define CHART_DATA_BOTH 2

#define SHOW_MODE_DATA  	0
#define SHOW_MODE_VALUES 	1 // includes the former show mode
#define SHOW_MODE_GUI 		2 // includes the former show modes
#define BUTTON_CHART_CONTROL_LEFT 38

uint8_t IndexOfDisplayedProbe = 0; // Which probe is to be displayed - index to AccuCapDisplayControl[] + DataloggerMeasurementControl[]
/*
 * Structure defining display of databuffer
 */
struct DataloggerDisplayControlStruct {
    uint16_t XStartIndex; // start value in actual label units (value is multiple of X GridSpacing) - for both charts

    int8_t ResistorYScaleFactor; // scale factor for Y increment 0=> Y increment = CHART_MIN_Y_INCREMENT
    int8_t VoltYScaleFactor; // scale factor for Y increment 0=> Y increment = CHART_MIN_Y_INCREMENT

    uint8_t ActualDataChart; // VoltageChart or ResistanceChart or null => both
    uint8_t ChartShowMode; // controls the info seen on chart screen
};
DataloggerDisplayControlStruct AccuCapDisplayControl[NUMBER_OF_PROBES];

/****************************
 * measurement stuff
 ****************************/

// Modes
#define MODE_DISCHARGING 0x00
#define MODE_CHARGING 0x01
#define MODE_EXTERNAL_VOLTAGE 0x02 // For measurement of external voltage and current
/*
 * Structure defining a capacity measurement
 */
#define RAM_DATABUFFER_MAX_LENGTH 1200 // 12 byte per entry. 1000 -> 16,6 hours for 1 sample per minute
/*
 * Current battery values set by getBatteryValues()
 */
struct BatteryInfoStruct {
    uint16_t VoltageNoLoadMillivolt;
    uint16_t VoltageLoadMillivolt; // The voltage which is always measured
    int16_t Milliampere;
    uint16_t ESRMilliohm;
    uint16_t sESRTestDeltaMillivolt = 0; // only displayed at initial ESR testing
    uint32_t CapacityAccumulator;
    uint16_t CapacityMilliampereHour;

    // not used yet
    uint8_t LoadState;
    uint8_t TypeIndex;
};

struct DataloggerMeasurementControlStruct {
    bool IsStarted;
    uint8_t Mode;
    uint8_t ProbeIndex;

    uint16_t SamplePeriodSeconds;
    uint16_t SampleCount; // number of samples just taken for measurement

    uint16_t SwitchOffVoltageMillivolt; // if Reading < StopThreshold then stop - used for discharging

    uint16_t StopMilliampereHour; // if Capacity > StopMilliampereHour then stop - used for charging - 0 means no stop

    uint16_t ChargeVoltageMillivolt; // is stored in RTC backup register 8 - Maximum is 48 (16*3) Volt at 3 Volt reference

    uint16_t ProbeNumber; // ID number of actual probe

    struct BatteryInfoStruct BatteryInfo;

    uint16_t LoadResistorMilliohm;
    float ExternalAttenuatorFactor; // to compensate external attenuator: ADC Value * factor = Volt

    uint16_t VoltageNoLoadDatabuffer[RAM_DATABUFFER_MAX_LENGTH]; // if mode == external maximum values are stored here
    uint16_t ESRMilliohmDatabuffer[RAM_DATABUFFER_MAX_LENGTH];
};
DataloggerMeasurementControlStruct BatteryControl[NUMBER_OF_PROBES];

/*
 * Values and functions for measurement
 */
#define LOAD_SWITCH_SETTLE_TIME_MILLIS  5   // Time for voltage to settle after load switch

// Global declarations

uint16_t getBatteryVoltageMillivolt(DataloggerMeasurementControlStruct *aProbe);
void getBatteryValues(DataloggerMeasurementControlStruct *aProbe);
void startStopMeasurement(DataloggerMeasurementControlStruct *aProbe, bool aDoStart);
void setYAutorange(int16_t aProbeIndex, uint16_t aADCReadingToAutorange);

void initGUIAccuCapacity(void);
void printBatteryValues(void);
void clearBasicInfo(void);
void drawData(bool doClearBefore);
void activateOrShowChartGui(void);
void adjustXAxisToSamplePeriod(unsigned int aProbeIndex, int aSamplePeriodSeconds);
bool CheckStopCondition(DataloggerMeasurementControlStruct *aProbe);
void printSamplePeriod(void);
void drawAccuCapacityMainPage(void);
void drawAccuCapacitySettingsPage(void);
void redrawAccuCapacityChartPage(void);
void redrawAccuCapacityPages(void);

void doStartStopAccuCap(BDButton *aTheTouchedButton, int16_t aProbeIndex);

bool changeXOffset(int aValue);
void changeXScaleFactor(int aValue);
void changeYOffset(int aValue);
void changeYScaleFactor(int aValue);

void storeBatteryValues(DataloggerMeasurementControlStruct *aProbe);
void callbackTimerSampleChannel0(void);
void callbackTimerSampleChannel1(void);
void callbackDisplayRefreshDelay(void);

/*
 * LOOP timing
 */
static void (*SampleDelayCallbackFunctions[NUMBER_OF_PROBES])(void) = {&callbackTimerSampleChannel0, &callbackTimerSampleChannel1};

// last sample of millis() in loop as reference for loop duration
unsigned long MillisLastLoopCapacity = 0;

// update display at least all SAMPLE_PERIOD_MILLIS millis
#define SAMPLE_PERIOD_MILLIS ONE_SECOND_MILLIS

/****************************************************************************************
 * THE CODE STARTS HERE
 ****************************************************************************************/

/**
 * Activate or deactivate the load / source according to Mode
 * For MODE_EXTERNAL_VOLTAGE every switch is disabled anyway
 */
void setSinkSource(unsigned int aProbeIndex, bool aDoActivate) {
// default is extern with both false
    bool tCharge = false;
    bool tDischarge = false;
    if (aDoActivate) {
        if (BatteryControl[aProbeIndex].Mode == MODE_CHARGING) {
            tCharge = true;
        } else if (BatteryControl[aProbeIndex].Mode == MODE_DISCHARGING) {
            // discharging
            tDischarge = true;
        }
    }
    AccuCapacity_SwitchCharge(aProbeIndex, tCharge);
    AccuCapacity_SwitchDischarge(aProbeIndex, tDischarge);
}

/**
 * Callback for Timer
 */
void callbackDisplayRefreshDelay(void) {
    registerDelayCallback(&callbackDisplayRefreshDelay, SAMPLE_PERIOD_MILLIS);
    // no drawing here!!! because it may interfere with drawing in tread
    doDisplayRefresh = true;
}

void callbackTimerSampleChannel0(void) {
    storeBatteryValues(&BatteryControl[0]);
}
void callbackTimerSampleChannel1(void) {
    storeBatteryValues(&BatteryControl[1]);
}

/**
 *
 * @param aProbeIndex
 * @return true if measurement has to be stopped
 */
bool CheckStopCondition(DataloggerMeasurementControlStruct *aProbe) {
    if (aProbe->SampleCount >= RAM_DATABUFFER_MAX_LENGTH - 1) {
        return true;
    }
    if (aProbe->Mode == MODE_DISCHARGING) {
        return (aProbe->BatteryInfo.VoltageNoLoadMillivolt < aProbe->SwitchOffVoltageMillivolt);
    } else if (aProbe->Mode == MODE_CHARGING && aProbe->StopMilliampereHour != 0) {
        return (aProbe->BatteryInfo.CapacityMilliampereHour > aProbe->StopMilliampereHour);
    }
    return false;
}

void TouchUpHandlerAccuCapacity(struct TouchEvent *const aTouchPosition) {
    // first check for buttons
//    if (!LocalTouchButton::checkAllButtons(aTouchPosition->TouchPosition.PositionX, aTouchPosition->TouchPosition.PositionY)) {
        if (ActualPage == PAGE_CHART) {
            //touch press but no gui element matched?
            // switch chart overlay;
            doSwitchChartOverlay();
        }
//    }
}

/**
 * Shows gui on long touch
 * @param aTouchPositionX
 * @param aTouchPositionY
 * @param isLongTouch
 * @return
 */
void longTouchHandlerAccuCapacity(struct TouchEvent *const aTouchPosition) {
    // show gui
    if (ActualPage == PAGE_CHART) {
        if (AccuCapDisplayControl[IndexOfDisplayedProbe].ChartShowMode == SHOW_MODE_DATA) {
            clearBasicInfo();
        }
        AccuCapDisplayControl[IndexOfDisplayedProbe].ChartShowMode = SHOW_MODE_GUI;
        activateOrShowChartGui();
    }
}

/**
 * responsible for swipe detection and dispatching
 * @param aMillisOfTouch
 * @param aTouchDeltaX
 * @param aTouchDeltaY
 * @return
 */
void swipeEndHandlerAccuCapacity(struct Swipe *const aSwipeInfo) {

    bool tIsError = false;
    if (ActualPage == PAGE_CHART) {
        if (aSwipeInfo->SwipeMainDirectionIsX) {
            int tTouchDeltaGrid = aSwipeInfo->TouchDeltaX / CHART_GRID_X_SIZE;
            // Horizontal swipe
            if (aSwipeInfo->TouchStartY > BUTTON_HEIGHT_4_LINE_4) {
                // scroll
                tIsError = changeXOffset(-tTouchDeltaGrid);
            } else {
                // scale
                changeXScaleFactor(tTouchDeltaGrid / 2);
            }
        } else {
            int tTouchDeltaGrid = aSwipeInfo->TouchDeltaY / CHART_GRID_Y_SIZE;
            // Vertical swipe
            if (aSwipeInfo->TouchStartX < BUTTON_WIDTH_5) {
                //offset
                changeYOffset(tTouchDeltaGrid);
            } else {
                //range
                changeYScaleFactor(-tTouchDeltaGrid);
            }
        }
    } else {
        // swipe on start page
        tIsError = true;
    }
#if defined(SUPPORT_LOCAL_DISPLAY) && defined(DISABLE_REMOTE_DISPLAY)
    LocalTouchButton::playFeedbackTone(tIsError);
#else
    BDButton::playFeedbackTone(tIsError);
#endif
}

void initAccuCapacity(void) {
    // enable and reset output pins
    AccuCapacity_IO_initalize();

    /*
     * 200 milliOhm per scale unit => 1 Ohm per chart
     * factor is initally 1 -> display of 1000 for raw value of 1000 milliOhm
     */

    for (int i = 0; i < NUMBER_OF_PROBES; ++i) {
        // init charts
        // use maximum height and length - show grid
        VoltageCharts[i]->initChart(CHART_START_X, CHART_START_Y, CHART_WIDTH, CHART_HEIGHT, 2, true,
        CHART_GRID_X_SIZE,
        CHART_GRID_Y_SIZE);
        VoltageCharts[i]->setDataColor(COLOR16_BLUE);

        BatteryControl[i].ProbeIndex = i;
        // y axis
        BatteryControl[i].ExternalAttenuatorFactor = 1.0;
        VoltageCharts[i]->initYLabelFloat(START_Y_OFFSET, CHART_MIN_Y_INCREMENT_VOLT, 1.0 / 1000.0, 3, 1);
        VoltageCharts[i]->setYTitleText("Volt");
        AccuCapDisplayControl[i].VoltYScaleFactor = 0;            // 0=identity

        ResistanceCharts[i]->initChart(CHART_START_X, CHART_START_Y, CHART_WIDTH, CHART_HEIGHT, 2, true,
        CHART_GRID_X_SIZE,
        CHART_GRID_Y_SIZE);
        ResistanceCharts[i]->setDataColor(COLOR16_RED);

        // y axis
        ResistanceCharts[i]->initYLabelInt(0, CHART_MIN_Y_INCREMENT_RESISTANCE, 1, 3);
        ResistanceCharts[i]->setYTitleText("mOhm");
        AccuCapDisplayControl[i].ResistorYScaleFactor = 0;            // 0=identity

        // x axis for both charts
        adjustXAxisToSamplePeriod(i, SAMPLE_PERIOD_START);

        // init control
        BatteryControl[i].IsStarted = false;
        BatteryControl[i].Mode = MODE_DISCHARGING;
        BatteryControl[i].BatteryInfo.CapacityAccumulator = 0;
        BatteryControl[i].SamplePeriodSeconds = SAMPLE_PERIOD_START;
        BatteryControl[i].LoadResistorMilliohm = 5000;
        BatteryControl[i].SwitchOffVoltageMillivolt = 1100;
        BatteryControl[i].StopMilliampereHour = 0;
        BatteryControl[i].ProbeNumber = i;
        BatteryControl[i].SampleCount = 0;

        AccuCapDisplayControl[i].ChartShowMode = SHOW_MODE_GUI;
        AccuCapDisplayControl[i].ActualDataChart = CHART_DATA_BOTH;
        // load from cmos ram
        if (RTC_checkMagicNumber()) {
            BatteryControl[i].ChargeVoltageMillivolt = HAL_RTCEx_BKUPRead(&RTCHandle, RTC_BKP_DR9) & 0xFFFF;
        } else {
            BatteryControl[i].ChargeVoltageMillivolt = CHARGE_VOLTAGE_MILLIVOLT_DEFAULT;
        }
    }
}

void startAccuCapacity(void) {
    initGUIAccuCapacity();
    // New calibration since VCC may have changed
    ADC_setRawToVoltFactor();

    drawAccuCapacityMainPage();
    registerRedrawCallback(&redrawAccuCapacityPages);

    // start display refresh
    registerDelayCallback(&callbackDisplayRefreshDelay, SAMPLE_PERIOD_MILLIS);

    registerLongTouchDownCallback(&longTouchHandlerAccuCapacity, TOUCH_STANDARD_LONG_TOUCH_TIMEOUT_MILLIS);
    // use touch up for buttons in order not to interfere with long touch
    registerTouchUpCallback(&TouchUpHandlerAccuCapacity);
    registerSwipeEndCallback(&swipeEndHandlerAccuCapacity);
}

void stopAccuCapacity(void) {
    // Stop display refresh
    changeDelayCallback(&callbackDisplayRefreshDelay, DISABLE_TIMER_DELAY_VALUE);
#if defined(SUPPORT_LOCAL_DISPLAY)
// free buttons
    for (unsigned int i = 0; i < sizeof(ButtonsChart) / sizeof(ButtonsChart[0]); ++i) {
        ButtonsChart[i]->deinit();
    }
    for (unsigned int i = 0; i < sizeof(TouchButtonsMain) / sizeof(TouchButtonsMain[0]); ++i) {
        TouchButtonsMain[i].deinit();
    }

    TouchButtonSetProbeNumber.deinit();
    TouchButtonSetLoadResistor.deinit();
    TouchButtonSetStopValue.deinit();
    TouchButtonSetChargeVoltage.deinit();
    TouchButtonSetExternalAttenuatorFactor.deinit();
    TouchButtonBack.deinit();

    TouchButtonAutorepeatSamplePeriodPlus.deinit();
    TouchButtonAutorepeatSamplePeriodMinus.deinit();
#endif

    registerLongTouchDownCallback(NULL, 0);
    registerSwipeEndCallback(NULL);
    registerTouchUpCallback(NULL);
    // restore old touch down handler
}

void loopAccuCapacity(void) {
// check Flags from ISR
    if (doDisplayRefresh == true) {
        for (int i = 0; i < NUMBER_OF_PROBES; ++i) {
            getBatteryValues(&BatteryControl[i]);
            if (BatteryControl[i].Mode == MODE_DISCHARGING && BatteryControl[i].BatteryInfo.VoltageNoLoadMillivolt < 100) {
                // stop running measurement
                if (BatteryControl[i].IsStarted) {
                    doStartStopAccuCap(&TouchButtonStartStopCapacityMeasurement, i);
                }
            }
        }
        printBatteryValues();
        doDisplayRefresh = false;
    }
    if (doEndTone == true) {
        doEndTone = false;
        playEndTone();
    }
    checkAndHandleEvents();
}

/************************************************************************
 * GUI handler section
 ************************************************************************/

/**
 * clears screen and displays chart screen
 * @param aTheTouchedButton
 * @param aProbeIndex
 */
void doShowChartScreen(BDButton *aTheTouchedButton, int16_t aProbeIndex) {
    IndexOfDisplayedProbe = aProbeIndex;
    redrawAccuCapacityChartPage();
}

/**
 * scrolls chart data left and right
 */
bool changeXOffset(int aValue) {
    signed int tNewValue = AccuCapDisplayControl[IndexOfDisplayedProbe].XStartIndex
            + VoltageCharts[IndexOfDisplayedProbe]->adjustIntWithXScaleFactor(aValue);
    bool tIsError = false;
    if (tNewValue < 0) {
        tNewValue = 0;
        tIsError = true;
    } else if (tNewValue * VoltageCharts[IndexOfDisplayedProbe]->getXGridSpacing()
            > BatteryControl[IndexOfDisplayedProbe].SampleCount) {
        tNewValue = BatteryControl[IndexOfDisplayedProbe].SampleCount / VoltageCharts[IndexOfDisplayedProbe]->getXGridSpacing();
        tIsError = true;
    }
    AccuCapDisplayControl[IndexOfDisplayedProbe].XStartIndex = tNewValue;
    drawData(true);
// redraw xLabels
    VoltageCharts[IndexOfDisplayedProbe]->setXLabelIntStartValueByIndex((int) tNewValue, true);

    return tIsError;
}

void changeXScaleFactor(int aValue) {
    signed int tXScaleFactor = VoltageCharts[IndexOfDisplayedProbe]->getXScaleFactor() + aValue;
// redraw xLabels
    VoltageCharts[IndexOfDisplayedProbe]->setXScaleFactor(tXScaleFactor, true);
    ResistanceCharts[IndexOfDisplayedProbe]->setXScaleFactor(tXScaleFactor, false);
    drawData(true);
}

void changeYOffset(int aValue) {
    VoltageCharts[IndexOfDisplayedProbe]->stepYLabelStartValueFloat(aValue);
// refresh screen
    redrawAccuCapacityChartPage();
}

/**
 * increments or decrements the Y scale factor for charts
 * @param aTheTouchedButton not used
 * @param aValue is 1 or -1
 */
void changeYScaleFactor(int aValue) {
    if (AccuCapDisplayControl[IndexOfDisplayedProbe].ActualDataChart == CHART_DATA_RESISTANCE) {
        AccuCapDisplayControl[IndexOfDisplayedProbe].ResistorYScaleFactor += aValue;
        ResistanceCharts[IndexOfDisplayedProbe]->setYLabelBaseIncrementValue(
                adjustIntWithScaleFactor(CHART_MIN_Y_INCREMENT_RESISTANCE,
                        AccuCapDisplayControl[IndexOfDisplayedProbe].ResistorYScaleFactor));
    } else {
        AccuCapDisplayControl[IndexOfDisplayedProbe].VoltYScaleFactor += aValue;
        VoltageCharts[IndexOfDisplayedProbe]->setYLabelBaseIncrementValueFloat(
                adjustFloatWithScaleFactor(CHART_MIN_Y_INCREMENT_VOLT,
                        AccuCapDisplayControl[IndexOfDisplayedProbe].VoltYScaleFactor));
    }

// refresh screen
    redrawAccuCapacityChartPage();
}

static void doShowSettings(BDButton *aTheTouchedButton, int16_t aValue) {
    drawAccuCapacitySettingsPage();
}

void doSetProbeNumber(BDButton *aTheTouchedButton, int16_t aValue) {

    float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, ProbeColors[IndexOfDisplayedProbe]);
// check for cancel
    if (!isnan(tNumber)) {
        BatteryControl[IndexOfDisplayedProbe].ProbeNumber = tNumber;
    }
    drawAccuCapacitySettingsPage();
}

void doSetStopValue(BDButton *aTheTouchedButton, int16_t aValue) {
    float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, ProbeColors[IndexOfDisplayedProbe]);
// check for cancel
    if (!isnan(tNumber)) {
        if (BatteryControl[IndexOfDisplayedProbe].Mode == MODE_CHARGING) {
            // set capacity
            BatteryControl[IndexOfDisplayedProbe].StopMilliampereHour = (tNumber * 11) / 8; // for loading factor of 1,375
        } else {
            BatteryControl[IndexOfDisplayedProbe].SwitchOffVoltageMillivolt = tNumber * 1000;
        }
    }
    drawAccuCapacitySettingsPage();
}

void doSetLoadResistorValue(BDButton *aTheTouchedButton, int16_t aValue) {
    float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, ProbeColors[IndexOfDisplayedProbe]);
// check for cancel
    if (!isnan(tNumber)) {
        BatteryControl[IndexOfDisplayedProbe].LoadResistorMilliohm = tNumber * 1000;
    }
    drawAccuCapacitySettingsPage();
}

void doSetExternalAttenuatorFactor(BDButton *aTheTouchedButton, int16_t aValue) {
    float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, ProbeColors[IndexOfDisplayedProbe]);
// check for cancel
    if (!isnan(tNumber)) {
        BatteryControl[IndexOfDisplayedProbe].ExternalAttenuatorFactor = tNumber;
    }
    drawAccuCapacitySettingsPage();
}

/**
 * stores the actual reading for the charging voltage in order to compute charge current etc.
 */
void doSetReadingChargeVoltage(BDButton *aTheTouchedButton, int16_t aValue) {
    unsigned int tAccuReading = 0;
    unsigned int tChargeVoltageMillivolt = BatteryControl[IndexOfDisplayedProbe].ChargeVoltageMillivolt;
    if (BatteryControl[IndexOfDisplayedProbe].Mode == MODE_EXTERNAL_VOLTAGE) {
        float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, ProbeColors[IndexOfDisplayedProbe]);
        // check for cancel
        if (!isnan(tNumber)) {
            tChargeVoltageMillivolt = tNumber * 1000;
        }
    } else {
        // Mode charging here because button is not shown on discharging
        // oversampling ;-)
        for (int i = 0; i < 8; ++i) {
            tAccuReading += AccuCapacity_ADCRead(IndexOfDisplayedProbe);
        }
        tAccuReading /= 8;
        tChargeVoltageMillivolt = tAccuReading * (sADCToVoltFactor * (1000 / 8));
        // store permanent

        HAL_PWR_EnableBkUpAccess();
        HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR9, tChargeVoltageMillivolt);
        RTC_PWRDisableBkUpAccess();
    }
    BatteryControl[IndexOfDisplayedProbe].ChargeVoltageMillivolt = tChargeVoltageMillivolt;
    drawAccuCapacitySettingsPage();
}

void doSetSamplePeriod(BDButton *aTheTouchedButton, int16_t aValue) {
    bool tIsError = false;
    int tSeconds = BatteryControl[IndexOfDisplayedProbe].SamplePeriodSeconds;
    if ((aValue > 0 && tSeconds < SAMPLE_PERIOD_HIGH_RESOLUTION) || (aValue < 0 && tSeconds <= SAMPLE_PERIOD_HIGH_RESOLUTION)) {
        tSeconds += aValue;
    } else {
        tSeconds += aValue * 10;
    }
    if (tSeconds <= 0) {
        tSeconds = 1;
        tIsError = true;
    }
    BatteryControl[IndexOfDisplayedProbe].SamplePeriodSeconds = tSeconds;

#if defined(SUPPORT_LOCAL_DISPLAY) && defined(DISABLE_REMOTE_DISPLAY)
    LocalTouchButton::playFeedbackTone(tIsError);
#else
    BDButton::playFeedbackTone(tIsError);
#endif
    adjustXAxisToSamplePeriod(IndexOfDisplayedProbe, tSeconds);
// Print value
    printSamplePeriod();
}
/**
 * shows main menu
 */
void doShowMainScreen(BDButton *aTheTouchedButton, int16_t aValue) {
    BDButton::deactivateAll();
    drawAccuCapacityMainPage();
}

/**
 * changes the displayed content in the chart page
 * toggles between short and long value display
 */
void doSwitchChartOverlay(void) {
    BDButton::playFeedbackTone();
// change display / overlay mode
    AccuCapDisplayControl[IndexOfDisplayedProbe].ChartShowMode++;
    if (AccuCapDisplayControl[IndexOfDisplayedProbe].ChartShowMode > SHOW_MODE_VALUES) {
        AccuCapDisplayControl[IndexOfDisplayedProbe].ChartShowMode = SHOW_MODE_DATA;

        BDButton::deactivateAll();
        // only chart no gui
        drawData(true);            // calls activateOrShowChartGui()
    } else {
        //  delete basic data and show full data
        clearBasicInfo();
        VoltageCharts[IndexOfDisplayedProbe]->drawGrid();
        printBatteryValues();
    }
}

/**
 * switch between voltage and resistance (and both) charts
 * in external mode switch between Max(voltage) and Min(resistance) charts
 */
void doSwitchChartData(BDButton *aTheTouchedButton, int16_t aProbeIndex) {
    AccuCapDisplayControl[aProbeIndex].ActualDataChart++;
    if (AccuCapDisplayControl[aProbeIndex].ActualDataChart > CHART_DATA_BOTH) {
        AccuCapDisplayControl[aProbeIndex].ActualDataChart = CHART_DATA_VOLTAGE;
    } else if (AccuCapDisplayControl[aProbeIndex].ActualDataChart == CHART_DATA_RESISTANCE
            && BatteryControl[aProbeIndex].Mode != MODE_EXTERNAL_VOLTAGE) {
        // no resistance for external mode (U min instead)
        // y axis for resistance only shown at CHART_RESISTANCE
        ResistanceCharts[IndexOfDisplayedProbe]->drawYAxis(true);
        ResistanceCharts[IndexOfDisplayedProbe]->drawYAxisTitle(CHART_Y_LABEL_OFFSET_RESISTANCE);
    } else if (AccuCapDisplayControl[aProbeIndex].ActualDataChart == CHART_DATA_BOTH) {
        VoltageCharts[IndexOfDisplayedProbe]->drawYAxis(true);
        ResistanceCharts[IndexOfDisplayedProbe]->drawYAxisTitle(CHART_Y_LABEL_OFFSET_VOLTAGE);
    }
    if (BatteryControl[aProbeIndex].Mode == MODE_EXTERNAL_VOLTAGE) {
        aTheTouchedButton->setCaption(chartStringsExtern[AccuCapDisplayControl[aProbeIndex].ActualDataChart]);
    } else {
        aTheTouchedButton->setCaption(chartStrings[AccuCapDisplayControl[aProbeIndex].ActualDataChart]);
    }
    drawData(true);
}

void startStopMeasurement(DataloggerMeasurementControlStruct *aProbe, bool aDoStart) {
    aProbe->IsStarted = aDoStart;
    setSinkSource(aProbe->ProbeIndex, aDoStart);

    if (aDoStart) {
        /*
         * START
         */
        delay(LOAD_SWITCH_SETTLE_TIME_MILLIS);
        getBatteryValues(aProbe);
        // Autorange if first value
        if (aProbe->SampleCount == 0) {
            // let measurement value start at middle of y axis
            setYAutorange(aProbe->ProbeIndex, aProbe->BatteryInfo.VoltageNoLoadMillivolt);
        }
        printBatteryValues();
        changeDelayCallback(SampleDelayCallbackFunctions[aProbe->ProbeIndex], aProbe->SamplePeriodSeconds * ONE_SECOND_MILLIS);
    } else {
        /*
         * STOP
         */
        changeDelayCallback(SampleDelayCallbackFunctions[aProbe->ProbeIndex], DISABLE_TIMER_DELAY_VALUE);
    }
}

void doChargeMode(BDButton *aTheTouchedButton, int16_t aProbeIndex) {
    unsigned int tNewState = BatteryControl[aProbeIndex].Mode;
    tNewState++;
    if (tNewState > MODE_EXTERNAL_VOLTAGE) {
        tNewState = MODE_DISCHARGING;
    }
    BatteryControl[aProbeIndex].Mode = tNewState;
    setSinkSource(aProbeIndex, BatteryControl[aProbeIndex].IsStarted);
    setModeCaption(IndexOfDisplayedProbe);

    if (AccuCapDisplayControl[IndexOfDisplayedProbe].ChartShowMode == SHOW_MODE_GUI) {
        aTheTouchedButton->drawButton();
    }
}

void doClearDataBuffer(BDButton *aTheTouchedButton, int16_t aProbeIndex) {
    AccuCapDisplayControl[IndexOfDisplayedProbe].XStartIndex = 0;
// redraw xLabels
    VoltageCharts[IndexOfDisplayedProbe]->setXLabelIntStartValueByIndex(0, false);
    VoltageCharts[IndexOfDisplayedProbe]->setXScaleFactor(1, true);
    ResistanceCharts[IndexOfDisplayedProbe]->setXScaleFactor(1, false);

    BatteryControl[aProbeIndex].BatteryInfo.CapacityAccumulator = 0;
    BatteryControl[aProbeIndex].BatteryInfo.CapacityMilliampereHour = 0;
    BatteryControl[aProbeIndex].BatteryInfo.Milliampere = 0;
    BatteryControl[aProbeIndex].BatteryInfo.ESRMilliohm = 0;
    BatteryControl[aProbeIndex].SampleCount = 0;
// Clear data buffer
    memset(&BatteryControl[aProbeIndex].VoltageNoLoadDatabuffer[0], 0, RAM_DATABUFFER_MAX_LENGTH);
    memset(&BatteryControl[aProbeIndex].ESRMilliohmDatabuffer[0], 0,
    RAM_DATABUFFER_MAX_LENGTH);
    getBatteryVoltageMillivolt(&BatteryControl[aProbeIndex]);
// set button text to load
    TouchButtonLoadStore.setCaption("Load");
    drawData(true);
}

const char* getModeString(int aProbeIndex) {
    const char *tModeString;
    if (BatteryControl[aProbeIndex].Mode == MODE_EXTERNAL_VOLTAGE) {
        tModeString = "Extern";
    } else
        tModeString = &StringDischarging[0];
    if (BatteryControl[aProbeIndex].Mode == MODE_CHARGING) {
        tModeString = &StringDischarging[3];
    }
    return tModeString;
}

static void doStoreLoadChartToFile(BDButton *aTheTouchedButton, int16_t aProbeIndex) {
    bool tIsError = true;
    if (MICROSD_isCardInserted()) {
        FIL tFile;
        FRESULT tOpenResult;
        UINT tCount;
        snprintf(sStringBuffer, sizeof sStringBuffer, "channel%d_%s.bin", aProbeIndex, getModeString(aProbeIndex));

        if (BatteryControl[aProbeIndex].SampleCount == 0) {
            // first stop running measurement
            if (BatteryControl[aProbeIndex].IsStarted) {
                doStartStopAccuCap(&TouchButtonStartStopCapacityMeasurement, aProbeIndex);
            }
            /*
             * Load data from file
             */
            tOpenResult = f_open(&tFile, sStringBuffer, FA_OPEN_EXISTING | FA_READ);
            if (tOpenResult == FR_OK) {
                // read AccuCapDisplayControl structure to reproduce the layout
                f_read(&tFile, &AccuCapDisplayControl[aProbeIndex], sizeof(AccuCapDisplayControl[aProbeIndex]), &tCount);
                // read DataloggerMeasurementControl structure - including data buffers
                f_read(&tFile, &BatteryControl[aProbeIndex], sizeof(BatteryControl[aProbeIndex]), &tCount);
                f_close(&tFile);
                // set state to stop
                BatteryControl[aProbeIndex].IsStarted = false;
                setYAutorange(aProbeIndex, BatteryControl[aProbeIndex].VoltageNoLoadDatabuffer[0]);
                AccuCapDisplayControl[aProbeIndex].ActualDataChart = CHART_DATA_BOTH;
                // redraw display corresponding to new values
                redrawAccuCapacityChartPage();
                tIsError = false;
            }
        } else {
            /*
             * Store data to file
             */
            tOpenResult = f_open(&tFile, sStringBuffer, FA_CREATE_ALWAYS | FA_WRITE);
            if (tOpenResult == FR_OK) {
                // write display control to reproduce the layout
                f_write(&tFile, &AccuCapDisplayControl[aProbeIndex], sizeof(AccuCapDisplayControl[aProbeIndex]), &tCount);
                // write control structure including the data buffers
                f_write(&tFile, &BatteryControl[aProbeIndex], sizeof(BatteryControl[aProbeIndex]), &tCount);
                f_close(&tFile);
                tIsError = false;
            }
        }
    }
#if defined(SUPPORT_LOCAL_DISPLAY) && defined(DISABLE_REMOTE_DISPLAY)
    LocalTouchButton::playFeedbackTone(tIsError);
#else
    BDButton::playFeedbackTone(tIsError);
#endif
}

/**
 * Export Data Buffer to CSV file
 * @param aTheTouchedButton
 * @param aProbeIndex
 */
static void doExportChart(BDButton *aTheTouchedButton, int16_t aProbeIndex) {
    bool tIsError = true;
    if (MICROSD_isCardInserted()) {
        FIL tFile;
        FRESULT tOpenResult;
        UINT tCount;
        unsigned int tIndex = RTC_getDateStringForFile(sStringBuffer);
        snprintf(&sStringBuffer[tIndex], sizeof sStringBuffer - tIndex, " probe%d_%s.csv", BatteryControl[aProbeIndex].ProbeNumber,
                getModeString(aProbeIndex));
        if (BatteryControl[aProbeIndex].SampleCount != 0) {
            tOpenResult = f_open(&tFile, sStringBuffer, FA_CREATE_ALWAYS | FA_WRITE);
            if (tOpenResult != FR_OK) {
                failParamMessage(tOpenResult, "f_open");
            } else {
                unsigned int tSeconds = BatteryControl[IndexOfDisplayedProbe].SamplePeriodSeconds;
                unsigned int tMinutes = tSeconds / 60;
                tSeconds %= 60;
                tIndex = snprintf(sStringBuffer, sizeof sStringBuffer, "Probe Nr:%d\nSample interval:%u:%02u min\nDate:",
                        BatteryControl[aProbeIndex].ProbeNumber, tMinutes, tSeconds);
                tIndex += RTC_getTimeString(&sStringBuffer[tIndex]);
                snprintf(&sStringBuffer[tIndex], (sizeof sStringBuffer) - tIndex,
                        "\nCapacity:%4umAh\nInternal resistance:%5u mOhm\nVolt no load;mOhm\n",
                        BatteryControl[aProbeIndex].BatteryInfo.CapacityMilliampereHour,
                        BatteryControl[aProbeIndex].BatteryInfo.ESRMilliohm);
                f_write(&tFile, sStringBuffer, strlen(sStringBuffer), &tCount);

                /**
                 * data
                 */
                tIndex = 0;
                uint16_t *tVoltageNoLoadDataPtr = &BatteryControl[aProbeIndex].VoltageNoLoadDatabuffer[0];
                uint16_t *tMilliOhmDataPtr = &BatteryControl[aProbeIndex].ESRMilliohmDatabuffer[0];
                for (int i = 0; i < BatteryControl[aProbeIndex].SampleCount; ++i) {
                    tIndex += snprintf(&sStringBuffer[tIndex], (sizeof sStringBuffer) - tIndex, "%6.3f;%5d\n",
                            sADCToVoltFactor * (*tVoltageNoLoadDataPtr++), (*tMilliOhmDataPtr++));
                    // accumulate strings in buffer and write if buffer almost full
                    if (tIndex > (sizeof sStringBuffer - 15)) {
                        tIndex = 0;
                        f_write(&tFile, sStringBuffer, strlen(sStringBuffer), &tCount);
                    }
                }
                // write last chunk
                if (tIndex > 0) {
                    f_write(&tFile, sStringBuffer, strlen(sStringBuffer), &tCount);
                }
                f_close(&tFile);
                tIsError = false;
            }
        }
    }
#if defined(SUPPORT_LOCAL_DISPLAY) && defined(DISABLE_REMOTE_DISPLAY)
    LocalTouchButton::playFeedbackTone(tIsError);
#else
    BDButton::playFeedbackTone(tIsError);
#endif
}

/***********************************************************************
 * GUI initialization and drawing stuff
 ***********************************************************************/

void initGUIAccuCapacity(void) {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
    /*
     * BUTTONS
     */

// for main screen
    TouchButtonsMain[0].init(0, 0, BUTTON_WIDTH_2_5, BUTTON_HEIGHT_4, ProbeColors[0], "Probe 1",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doShowChartScreen);
    TouchButtonsMain[1].init(BUTTON_WIDTH_2_5_POS_2, 0, BUTTON_WIDTH_2_5, BUTTON_HEIGHT_4, ProbeColors[1], "Probe 2",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 1, &doShowChartScreen);

    /*
     * chart buttons from left to right
     */
    unsigned int PosY = BUTTON_HEIGHT_5 + (BUTTON_DEFAULT_SPACING / 2);
    /*
     * 1. column
     */
    TouchButtonProbeSettings.init(BUTTON_WIDTH_5_POS_2, 0, BUTTON_WIDTH_5, BUTTON_HEIGHT_5,
    COLOR_GUI_CONTROL, "Sett.", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doShowSettings);
    TouchButtonClearDataBuffer.init(BUTTON_WIDTH_5_POS_2, PosY, BUTTON_WIDTH_5, BUTTON_HEIGHT_5,
    COLOR_GUI_VALUES, "Clear", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doClearDataBuffer);
    TouchButtonChartSwitchData.init(BUTTON_WIDTH_5_POS_2, 2 * (BUTTON_HEIGHT_5 + (BUTTON_DEFAULT_SPACING / 2)),
    BUTTON_WIDTH_5, BUTTON_HEIGHT_5, COLOR_GUI_DISPLAY_CONTROL, "Volt", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0,
            &doSwitchChartData);

    /*
     * 2. column
     */
    TouchButtonStartStopCapacityMeasurement.init(BUTTON_WIDTH_5_POS_3, 0, BUTTON_WIDTH_5,
    BUTTON_HEIGHT_5, COLOR_GUI_CONTROL, "", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doStartStopAccuCap);
    TouchButtonMode.init(BUTTON_WIDTH_5_POS_3, PosY, BUTTON_WIDTH_5, BUTTON_HEIGHT_5,
    COLOR_GUI_VALUES, "", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doChargeMode);

    /*
     * 3. column
     */
    TouchButtonNext.init(BUTTON_WIDTH_5_POS_4, 0, BUTTON_WIDTH_5, BUTTON_HEIGHT_5,
    COLOR_GUI_CONTROL, "Next", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doShowChartScreen);
    TouchButtonLoadStore.init(BUTTON_WIDTH_5_POS_4, PosY, BUTTON_WIDTH_5, BUTTON_HEIGHT_5,
    COLOR_GUI_VALUES, "", TEXT_SIZE_11, FLAG_BUTTON_NO_BEEP_ON_TOUCH, 0, &doStoreLoadChartToFile);

    /*
     * 4. column
     */
    TouchButtonMain.init(BUTTON_WIDTH_5_POS_5, 0, BUTTON_WIDTH_5, BUTTON_HEIGHT_5,
    COLOR_GUI_CONTROL, "Back", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doShowMainScreen);
    TouchButtonExport.init(BUTTON_WIDTH_5_POS_5, PosY, BUTTON_WIDTH_5, BUTTON_HEIGHT_5,
    COLOR_GUI_VALUES, "Export", TEXT_SIZE_11, FLAG_BUTTON_NO_BEEP_ON_TOUCH, 0, &doExportChart);

    /*
     * Settings page buttons
     */
// 1. row
    int tPosY = 0;
    TouchButtonSetProbeNumber.init(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR_GUI_VALUES, StringProbeNumber,
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doSetProbeNumber);

    TouchButtonBack.init(BUTTON_WIDTH_3_POS_3, 0, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
    COLOR_GUI_CONTROL, "Back", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doShowChartScreen);

// 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonSetLoadResistor.init(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR_GUI_VALUES, StringLoadResistor,
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doSetLoadResistorValue);

// Value settings buttons
    TouchButtonAutorepeatSamplePeriodPlus.init(BUTTON_WIDTH_2_POS_2, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_4,
    COLOR_GUI_CONTROL, "+", TEXT_SIZE_22, FLAG_BUTTON_NO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, 1, &doSetSamplePeriod);

    TouchButtonAutorepeatSamplePeriodMinus.init(BUTTON_WIDTH_6_POS_6, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_4,
    COLOR_GUI_CONTROL, "-", TEXT_SIZE_22, FLAG_BUTTON_NO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, -1, &doSetSamplePeriod);

    TouchButtonAutorepeatSamplePeriodPlus.setButtonAutorepeatTiming(600, 100, 10, 40);
    TouchButtonAutorepeatSamplePeriodMinus.setButtonAutorepeatTiming(600, 100, 10, 40);

// 3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonSetExternalAttenuatorFactor.init(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4,
    COLOR_GUI_VALUES, StringExternalAttenuatorFactor, TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0,
            &doSetExternalAttenuatorFactor);

// 4. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonSetStopValue.init(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR_GUI_VALUES, StringStopVoltage,
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doSetStopValue);

    TouchButtonSetChargeVoltage.init(BUTTON_WIDTH_2_POS_2, tPosY, BUTTON_WIDTH_2,
    BUTTON_HEIGHT_4, COLOR_GUI_VALUES, StringStoreChargeVoltage, TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0,
            &doSetReadingChargeVoltage);

#pragma GCC diagnostic pop

}

/**
 * adjust X values initially according to sample period
 * sets X grid size, start value, scale factor, label and title
 * set both charts because grid X spacing also changes
 */
void adjustXAxisToSamplePeriod(unsigned int aProbeIndex, int aSamplePeriodSeconds) {
    const char *tXLabelText = "min";
    AccuCapDisplayControl[aProbeIndex].XStartIndex = 0;
    if (aSamplePeriodSeconds < 40) {
        // 10 (minutes) scale
        VoltageCharts[aProbeIndex]->iniXAxisInt(600 / aSamplePeriodSeconds, 0, 10, 2);
        ResistanceCharts[aProbeIndex]->iniXAxisInt(600 / aSamplePeriodSeconds, 0, 10, 2);
    } else if (aSamplePeriodSeconds < 90) {
        // 30 min scale
        VoltageCharts[aProbeIndex]->iniXAxisInt(1800 / aSamplePeriodSeconds, 0, 30, 2);
        ResistanceCharts[aProbeIndex]->iniXAxisInt(1800 / aSamplePeriodSeconds, 0, 30, 2);
    } else if (aSamplePeriodSeconds <= 240) {
        // 60 min scale
        VoltageCharts[aProbeIndex]->iniXAxisInt(3600 / aSamplePeriodSeconds, 0, 60, 2);
        ResistanceCharts[aProbeIndex]->iniXAxisInt(3600 / aSamplePeriodSeconds, 0, 60, 2);
    } else {
        // 1 (hour) scale
        VoltageCharts[aProbeIndex]->iniXAxisInt(7200 / aSamplePeriodSeconds, 0, 2, 1);
        ResistanceCharts[aProbeIndex]->iniXAxisInt(7200 / aSamplePeriodSeconds, 0, 2, 1);
        tXLabelText = "hour";
    }
    VoltageCharts[aProbeIndex]->setXTitleText(tXLabelText);
}

void printSamplePeriod(void) {
    unsigned int tSeconds = BatteryControl[IndexOfDisplayedProbe].SamplePeriodSeconds;
    int tMinutes = tSeconds / 60;
    tSeconds %= 60;
    snprintf(sStringBuffer, sizeof sStringBuffer, "Sample period %u:%02u", tMinutes, tSeconds);
    BlueDisplay1.drawText(BUTTON_WIDTH_2_POS_2, BUTTON_HEIGHT_4_LINE_3, sStringBuffer, TEXT_SIZE_11, COLOR16_BLACK,
    BACKGROUND_COLOR);
}

void doStartStopAccuCap(BDButton *aTheTouchedButton, int16_t aProbeIndex) {
    BatteryControl[aProbeIndex].IsStarted = !BatteryControl[aProbeIndex].IsStarted;
    startStopMeasurement(&BatteryControl[aProbeIndex], BatteryControl[aProbeIndex].IsStarted);
    if (BatteryControl[aProbeIndex].IsStarted) {
        // START
        aTheTouchedButton->setCaption("Stop");
    } else {
        //STOP
        aTheTouchedButton->setCaption("Start");
    }
    if (AccuCapDisplayControl[IndexOfDisplayedProbe].ChartShowMode == SHOW_MODE_GUI) {
        aTheTouchedButton->drawButton();
    }
}

/*
 * read accumulator and calculate voltage and current
 */
uint16_t getBatteryVoltageMillivolt(DataloggerMeasurementControlStruct *aProbe) {

    uint16_t tRawVoltageReadout = AccuCapacity_ADCRead(aProbe->ProbeIndex);
// compensate for external attenuator
    uint16_t tVoltageMillivolt = tRawVoltageReadout * (aProbe->ExternalAttenuatorFactor * sADCToVoltFactor * 1000);
    return tVoltageMillivolt;
}

/*
 * Gets Load and NoLoad voltages
 * Computes resistance
 */
void getBatteryValues(DataloggerMeasurementControlStruct *aProbe) {
    /*
     * get load value, if started. Otherwise it is the no load value :-(
     */
    uint16_t tVoltageLoadMillivolt = getBatteryVoltageMillivolt(aProbe);

    if (!aProbe->IsStarted) {
        aProbe->BatteryInfo.VoltageNoLoadMillivolt = tVoltageLoadMillivolt;
    } else {
        aProbe->BatteryInfo.VoltageLoadMillivolt = tVoltageLoadMillivolt;
        /*
         * Compute current using source / load resistor value
         */
        int tEffectiveVoltageMillivolt = tVoltageLoadMillivolt;
        if (aProbe->Mode != MODE_DISCHARGING) {
            // assume charging for extern
            tEffectiveVoltageMillivolt = aProbe->ChargeVoltageMillivolt - tVoltageLoadMillivolt;
        }
        uint16_t tMilliampere = (tEffectiveVoltageMillivolt * 1000) / aProbe->LoadResistorMilliohm;
        aProbe->BatteryInfo.Milliampere = tMilliampere;

        if (tMilliampere > 1) {
            // capacity computation
            aProbe->BatteryInfo.CapacityAccumulator += tMilliampere;
            aProbe->BatteryInfo.CapacityMilliampereHour = aProbe->BatteryInfo.CapacityAccumulator
                    / ((3600L * ONE_SECOND_MILLIS) / SAMPLE_PERIOD_MILLIS); // = / 3600 for 1 s sample period
        }

        /*
         * Compute ESR = internal Equivalent Series Resistance
         */

        if (aProbe->Mode != MODE_EXTERNAL_VOLTAGE) {

            // disconnect from sink/source and wait
            setSinkSource(aProbe->ProbeIndex, false);
            delay(LOAD_SWITCH_SETTLE_TIME_MILLIS);

            /*
             * Get NoLoad value
             */
            uint16_t tVoltageNoLoadMillivolt = getBatteryVoltageMillivolt(aProbe);
            aProbe->BatteryInfo.VoltageNoLoadMillivolt = tVoltageNoLoadMillivolt;

            //reconnect to sink/source
            setSinkSource(aProbe->ProbeIndex, true);

            /*
             * Compute resistance
             */
            int16_t tESRTestDeltaMillivolt = tVoltageNoLoadMillivolt - tVoltageLoadMillivolt;
            if (aProbe->Mode == MODE_CHARGING) {
                tESRTestDeltaMillivolt = -tESRTestDeltaMillivolt;
            }
            if (tESRTestDeltaMillivolt <= 0 || tVoltageLoadMillivolt < 100) {
                // plausibility fails
                aProbe->BatteryInfo.ESRMilliohm = 0;
            } else {
                aProbe->BatteryInfo.sESRTestDeltaMillivolt = tESRTestDeltaMillivolt;
                aProbe->BatteryInfo.ESRMilliohm = (tESRTestDeltaMillivolt * 1000L) / aProbe->BatteryInfo.Milliampere;
            }
        }
    }
}

/*
 * Store the current value in array
 * Check for stop condition
 * Redraw chart
 */
void storeBatteryValues(DataloggerMeasurementControlStruct *aProbe) {
    registerDelayCallback(SampleDelayCallbackFunctions[aProbe->ProbeIndex], aProbe->SamplePeriodSeconds * ONE_SECOND_MILLIS);

    /*
     * Store
     */
    aProbe->VoltageNoLoadDatabuffer[aProbe->SampleCount] = aProbe->BatteryInfo.VoltageNoLoadMillivolt;
    aProbe->ESRMilliohmDatabuffer[aProbe->SampleCount] = aProbe->BatteryInfo.ESRMilliohm;
    aProbe->SampleCount++;

    /*
     * Stop detection
     */
    bool tStop = CheckStopCondition(aProbe);

    if (tStop) {
        // Stop Measurement
        doStartStopAccuCap(&TouchButtonStartStopCapacityMeasurement, aProbe->ProbeIndex);
        // signal end of measurement
        doEndTone = true;
        return;
    }

    if (ActualPage == PAGE_CHART) {
        if (IndexOfDisplayedProbe == aProbe->ProbeIndex) {
            // Redraw chart
            drawData(false);
        }
        if ((aProbe->ProbeIndex == IndexOfDisplayedProbe) && (aProbe->SampleCount == 1)
                && (AccuCapDisplayControl[aProbe->ProbeIndex].ChartShowMode == SHOW_MODE_GUI)) {
            // set caption to "Store"
            TouchButtonLoadStore.setCaption("Store");
            TouchButtonLoadStore.drawButton();
        }
    }
}

/**
 * let measurement value start at middle of y axis
 * @param aProbeIndex
 * @param aADCReadingToAutorange Raw reading
 */
void setYAutorange(int16_t aProbeIndex, uint16_t aVoltageMillivolt) {
    float tNumberOfMinYIncrementsPerChart = CHART_HEIGHT / CHART_GRID_Y_SIZE;
    adjustFloatWithScaleFactor(tNumberOfMinYIncrementsPerChart, AccuCapDisplayControl[IndexOfDisplayedProbe].VoltYScaleFactor);
// let start voltage = readingVoltage - half of actual chart range
    float tYLabelStartVoltage = ((float) (aVoltageMillivolt) / 1000)
            - (tNumberOfMinYIncrementsPerChart * (CHART_MIN_Y_INCREMENT_VOLT / 2));
// round to 0.1
    tYLabelStartVoltage = roundf(10 * tYLabelStartVoltage) * CHART_MIN_Y_INCREMENT_VOLT;
    VoltageCharts[IndexOfDisplayedProbe]->setYLabelStartValueFloat(tYLabelStartVoltage);
    VoltageCharts[IndexOfDisplayedProbe]->drawYAxis(true);
    if (AccuCapDisplayControl[IndexOfDisplayedProbe].ChartShowMode == SHOW_MODE_GUI) {
        // show symbol for vertical swipe
        BlueDisplay1.drawChar(0, BUTTON_HEIGHT_5_LINE_4, '\xE0', TEXT_SIZE_22, COLOR16_BLUE, BACKGROUND_COLOR);
    }
}

/*
 * clears print region of printBasicInfo
 */
void clearBasicInfo(void) {
// values correspond to drawText above
    BlueDisplay1.fillRectRel(BASIC_INFO_X, BASIC_INFO_Y, BlueDisplay1.getDisplayWidth() - BASIC_INFO_X, 2 * TEXT_SIZE_11_HEIGHT,
    BACKGROUND_COLOR);
}

//show A/D data on LCD screen
void printBatteryValues(void) {
    if (ActualPage == PAGE_SETTINGS) {
        return;
    }
    unsigned int tPosX, tPosY;
    unsigned int tSeconds;
    unsigned int tMinutes;
    if (ActualPage == PAGE_MAIN) {
        /*
         * Version with 2 data columns for main screen
         */
        tPosX = 0;
        for (uint8_t i = 0; i < NUMBER_OF_PROBES; ++i) {
            tPosY = BUTTON_HEIGHT_4 + TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND;

            snprintf(sStringBuffer, sizeof sStringBuffer, "%5.3fV",
                    ((float) BatteryControl[i].BatteryInfo.VoltageNoLoadMillivolt) / 1000);

            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_22, ProbeColors[i], BACKGROUND_COLOR);

            tPosY += 4 + TEXT_SIZE_22_HEIGHT;
            snprintf(sStringBuffer, sizeof sStringBuffer, "%4umA", BatteryControl[i].BatteryInfo.Milliampere);
            BlueDisplay1.drawText(tPosX + TEXT_SIZE_22_WIDTH, tPosY, sStringBuffer, TEXT_SIZE_22, ProbeColors[i],
            BACKGROUND_COLOR);

            tPosY += 4 + TEXT_SIZE_22_HEIGHT;
            snprintf(sStringBuffer, sizeof sStringBuffer, "%4umAh", BatteryControl[i].BatteryInfo.CapacityMilliampereHour);
            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_22, ProbeColors[i], BACKGROUND_COLOR);

            tPosY += 4 + TEXT_SIZE_22_HEIGHT;
            snprintf(sStringBuffer, sizeof sStringBuffer, "%5.3f\x81", ((float) BatteryControl[i].BatteryInfo.ESRMilliohm) / 1000);

            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_22, ProbeColors[i], BACKGROUND_COLOR);

            tPosY += 4 + TEXT_SIZE_22_HEIGHT;
            tSeconds = BatteryControl[i].SamplePeriodSeconds;
            tMinutes = tSeconds / 60;
            tSeconds %= 60;
            snprintf(sStringBuffer, sizeof sStringBuffer, "Nr.%d - %u:%02u", BatteryControl[i].ProbeNumber, tMinutes, tSeconds);
            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_11, ProbeColors[i], BACKGROUND_COLOR);

            if (BatteryControl[i].Mode != MODE_EXTERNAL_VOLTAGE) {
                tPosY += 2 + TEXT_SIZE_11_HEIGHT;
                if (BatteryControl[i].Mode == MODE_DISCHARGING) {
                    snprintf(sStringBuffer, sizeof sStringBuffer, "stop %4.2fV  ",
                            (float) (BatteryControl[i].SwitchOffVoltageMillivolt) / 1000);
                } else if (BatteryControl[i].Mode == MODE_CHARGING) {
                    snprintf(sStringBuffer, sizeof sStringBuffer, "stop %4dmAh", BatteryControl[i].StopMilliampereHour);
                }
                BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_11, ProbeColors[i],
                BACKGROUND_COLOR);
            }
            tPosX = BUTTON_WIDTH_2_5_POS_2;
        }
    } else {
        if (AccuCapDisplayControl[IndexOfDisplayedProbe].ChartShowMode == SHOW_MODE_DATA) {
            /*
             * basic info on 2 lines on top of screen
             */
            if (BatteryControl[IndexOfDisplayedProbe].Mode != MODE_EXTERNAL_VOLTAGE) {
                snprintf(sStringBuffer, sizeof sStringBuffer, "Probe %d %umAh ESR %4.2f\x81",
                        BatteryControl[IndexOfDisplayedProbe].ProbeNumber,
                        BatteryControl[IndexOfDisplayedProbe].BatteryInfo.CapacityMilliampereHour,
                        ((float) BatteryControl[IndexOfDisplayedProbe].BatteryInfo.ESRMilliohm) / 1000);
            } else {
                snprintf(sStringBuffer, sizeof sStringBuffer, "Probe %d %umAh extern",
                        BatteryControl[IndexOfDisplayedProbe].ProbeNumber,
                        BatteryControl[IndexOfDisplayedProbe].BatteryInfo.CapacityMilliampereHour);
            }
            BlueDisplay1.drawText(BASIC_INFO_X, BASIC_INFO_Y + TEXT_SIZE_11_ASCEND, sStringBuffer, TEXT_SIZE_11,
                    ProbeColors[IndexOfDisplayedProbe], BACKGROUND_COLOR);
            tSeconds = BatteryControl[IndexOfDisplayedProbe].SamplePeriodSeconds;
            tMinutes = tSeconds / 60;
            tSeconds %= 60;
            int tCount = (snprintf(sStringBuffer, sizeof sStringBuffer, "%u:%02u %2.1f\x81", tMinutes, tSeconds,
                    (float) (BatteryControl[IndexOfDisplayedProbe].LoadResistorMilliohm) / 1000) + 1) * TEXT_SIZE_11_WIDTH;
            BlueDisplay1.drawText(BASIC_INFO_X, BASIC_INFO_Y + TEXT_SIZE_11_HEIGHT + 2 + TEXT_SIZE_11_ASCEND, sStringBuffer,
            TEXT_SIZE_11, ProbeColors[IndexOfDisplayedProbe], BACKGROUND_COLOR);
            showRTCTime(BASIC_INFO_X + tCount, BASIC_INFO_Y + TEXT_SIZE_11_HEIGHT + 2 + TEXT_SIZE_11_ASCEND,
                    ProbeColors[IndexOfDisplayedProbe], BACKGROUND_COLOR, false);
        } else {
            /*
             * version of one data set for chart screen
             */
            tPosY = (2 * BUTTON_HEIGHT_5) + BUTTON_DEFAULT_SPACING / 2 + 2 + TEXT_SIZE_22_ASCEND;
            tPosX = BUTTON_WIDTH_5_POS_3 + BUTTON_WIDTH_5;

            snprintf(sStringBuffer, sizeof sStringBuffer, "%5.3fV",
                    ((float) BatteryControl[IndexOfDisplayedProbe].BatteryInfo.VoltageNoLoadMillivolt) / 1000);
            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_22, ProbeColors[IndexOfDisplayedProbe],
            BACKGROUND_COLOR);

            tPosY += 2 + TEXT_SIZE_22_HEIGHT;
            snprintf(sStringBuffer, sizeof sStringBuffer, "%4umA", BatteryControl[IndexOfDisplayedProbe].BatteryInfo.Milliampere);
            BlueDisplay1.drawText(tPosX + TEXT_SIZE_22_WIDTH, tPosY, sStringBuffer, TEXT_SIZE_22,
                    ProbeColors[IndexOfDisplayedProbe],
                    BACKGROUND_COLOR);

            tPosY += 2 + TEXT_SIZE_22_HEIGHT;
            snprintf(sStringBuffer, sizeof sStringBuffer, "%4umAh",
                    BatteryControl[IndexOfDisplayedProbe].BatteryInfo.CapacityMilliampereHour);
            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_22, ProbeColors[IndexOfDisplayedProbe],
            BACKGROUND_COLOR);

            tPosY += 2 + TEXT_SIZE_22_HEIGHT;
            snprintf(sStringBuffer, sizeof sStringBuffer, "%5.3f\x81",
                    ((float) BatteryControl[IndexOfDisplayedProbe].BatteryInfo.ESRMilliohm) / 1000);

            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_22, ProbeColors[IndexOfDisplayedProbe],
            BACKGROUND_COLOR);

            tPosY += 4 + TEXT_SIZE_11_HEIGHT;
            tSeconds = BatteryControl[IndexOfDisplayedProbe].SamplePeriodSeconds;
            tMinutes = tSeconds / 60;
            tSeconds %= 60;
            snprintf(sStringBuffer, sizeof sStringBuffer, "%d %2.1f\x81 %u:%02umin",
                    BatteryControl[IndexOfDisplayedProbe].ProbeNumber,
                    (float) (BatteryControl[IndexOfDisplayedProbe].LoadResistorMilliohm) / 1000, tMinutes, tSeconds);
            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_11, ProbeColors[IndexOfDisplayedProbe],
            BACKGROUND_COLOR);

            if (BatteryControl[IndexOfDisplayedProbe].Mode != MODE_EXTERNAL_VOLTAGE) {
                tPosY += 1 + TEXT_SIZE_11_HEIGHT;
                if (BatteryControl[IndexOfDisplayedProbe].Mode == MODE_DISCHARGING) {
                    snprintf(sStringBuffer, sizeof sStringBuffer, "stop %4.2fV  ",
                            (float) (BatteryControl[IndexOfDisplayedProbe].SwitchOffVoltageMillivolt) / 1000);
                } else if (BatteryControl[IndexOfDisplayedProbe].Mode == MODE_CHARGING) {
                    snprintf(sStringBuffer, sizeof sStringBuffer, "stop %4dmAh",
                            BatteryControl[IndexOfDisplayedProbe].StopMilliampereHour);
                }
                BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_11, ProbeColors[IndexOfDisplayedProbe],
                BACKGROUND_COLOR);
            }
        }
    }
}

void drawAccuCapacityMainPage(void) {
    BlueDisplay1.clearDisplay(BACKGROUND_COLOR);
// display buttons
    for (int i = 0; i < NUMBER_OF_PROBES; ++i) {
        TouchButtonsMain[i].drawButton();
    }
    TouchButtonMainHome.drawButton();
    ActualPage = PAGE_MAIN;
// show data
    printBatteryValues();
}

void drawAccuCapacitySettingsPage(void) {
    BDButton::deactivateAll();
    BlueDisplay1.clearDisplay(BACKGROUND_COLOR);
    TouchButtonBack.drawButton();
    TouchButtonAutorepeatSamplePeriodPlus.drawButton();
    TouchButtonAutorepeatSamplePeriodMinus.drawButton();
    ActualPage = PAGE_SETTINGS;
    printSamplePeriod();

    snprintf(&StringProbeNumber[STRING_PROBE_NUMBER_NUMBER_INDEX], 4, "%3u", BatteryControl[IndexOfDisplayedProbe].ProbeNumber);
    TouchButtonSetProbeNumber.setCaption(StringProbeNumber, true);

    snprintf(&StringLoadResistor[STRING_LOAD_RESISTOR_NUMBER_INDEX], 5, "%4.1f",
            (float) (BatteryControl[IndexOfDisplayedProbe].LoadResistorMilliohm) / 1000);
    TouchButtonSetLoadResistor.setCaption(StringLoadResistor, true);
    if (BatteryControl[IndexOfDisplayedProbe].Mode != MODE_DISCHARGING) {
        setChargeVoltageCaption(IndexOfDisplayedProbe);
        TouchButtonSetChargeVoltage.drawButton();
    }

    snprintf(&StringExternalAttenuatorFactor[STRING_EXTERNAL_ATTENUATOR_FACTOR_NUMBER_INDEX], 6, "%5.2f",
            BatteryControl[IndexOfDisplayedProbe].ExternalAttenuatorFactor);
    TouchButtonSetExternalAttenuatorFactor.setCaption(StringExternalAttenuatorFactor, true);

    if (BatteryControl[IndexOfDisplayedProbe].Mode != MODE_EXTERNAL_VOLTAGE) {
        if (BatteryControl[IndexOfDisplayedProbe].Mode == MODE_CHARGING) {
            snprintf(&StringStopNominalCapacity[STRING_STOP_CAPACITY_NUMBER_INDEX], 5, "%4d",
                    BatteryControl[IndexOfDisplayedProbe].StopMilliampereHour);
            TouchButtonSetStopValue.setCaption(StringStopNominalCapacity, true);
        } else {
            snprintf(&StringStopVoltage[STRING_STOP_VOLTAGE_NUMBER_INDEX], 5, "%4.2f",
                    (float) (BatteryControl[IndexOfDisplayedProbe].SwitchOffVoltageMillivolt) / 1000);
            TouchButtonSetStopValue.setCaption(StringStopVoltage, true);
        }
    }

}

void setChargeVoltageCaption(int16_t aProbeIndex) {
    snprintf(&StringStoreChargeVoltage[STRING_STORE_CHARGE_VOLTAGE_NUMBER_INDEX], 6, "%5.2f",
            (float) (BatteryControl[IndexOfDisplayedProbe].ChargeVoltageMillivolt) / 1000);
    TouchButtonSetChargeVoltage.setCaption(StringStoreChargeVoltage);
}

void setModeCaption(int16_t aProbeIndex) {
    if (BatteryControl[aProbeIndex].Mode == MODE_CHARGING) {
        TouchButtonMode.setCaption("Charge");
    } else if (BatteryControl[aProbeIndex].Mode == MODE_DISCHARGING) {
        TouchButtonMode.setCaption("Dischg");
    } else {
        TouchButtonMode.setCaption("Extern");
    }
}

void redrawAccuCapacityPages(void) {
    if (ActualPage == PAGE_CHART) {
        redrawAccuCapacityChartPage();
    } else if (ActualPage == PAGE_MAIN) {
        drawAccuCapacityMainPage();
    } else {
        drawAccuCapacitySettingsPage();
    }
}
/**
 * Used to draw the chart screen completely new.
 * Clears display
 * sets button colors, values and captions
 */
void redrawAccuCapacityChartPage(void) {
    ActualPage = PAGE_CHART;
    BlueDisplay1.clearDisplay(BACKGROUND_COLOR);
    BDButton::deactivateAll();

// set button colors and values
    for (unsigned int i = 0; i < sizeof(ButtonsChart) / sizeof(ButtonsChart[0]); ++i) {
        unsigned int tValue = IndexOfDisplayedProbe;
        if (i == INDEX_OF_NEXT_BUTTON) {
            tValue = 1 ^ IndexOfDisplayedProbe;
            (*ButtonsChart[i]).setButtonColor(ProbeColors[tValue]);
        }
        if (i >= INDEX_OF_COLOURED_BUTTONS) {
            // activate and color the + - and < > buttons
            (*ButtonsChart[i]).setButtonColor(ProbeColors[tValue]);
        } else {
            // set probe value for the action buttons
            (*ButtonsChart[i]).setValue(tValue);
        }
    }
    TouchButtonBack.setValue(IndexOfDisplayedProbe);

// set right captions
    if (BatteryControl[IndexOfDisplayedProbe].IsStarted) {
        TouchButtonStartStopCapacityMeasurement.setCaption("Stop");
    } else {
        TouchButtonStartStopCapacityMeasurement.setCaption("Start");
    }
    if (BatteryControl[IndexOfDisplayedProbe].SampleCount == 0) {
        TouchButtonLoadStore.setCaption("Load");
    } else {
        TouchButtonLoadStore.setCaption("Store");
    }
    TouchButtonChartSwitchData.setCaption(chartStrings[AccuCapDisplayControl[IndexOfDisplayedProbe].ActualDataChart]);
    setModeCaption(IndexOfDisplayedProbe);

    if (AccuCapDisplayControl[IndexOfDisplayedProbe].ActualDataChart == CHART_DATA_RESISTANCE) {
        ResistanceCharts[IndexOfDisplayedProbe]->drawAxesAndGrid(); // X labels are not drawn here
        VoltageCharts[IndexOfDisplayedProbe]->drawXAxis(false); // Because X labels are taken from Voltage chart
        ResistanceCharts[IndexOfDisplayedProbe]->drawYAxisTitle(CHART_Y_LABEL_OFFSET_RESISTANCE);
    } else {
        VoltageCharts[IndexOfDisplayedProbe]->drawAxesAndGrid();
        VoltageCharts[IndexOfDisplayedProbe]->drawYAxisTitle(CHART_Y_LABEL_OFFSET_VOLTAGE);
        if (AccuCapDisplayControl[IndexOfDisplayedProbe].ActualDataChart == CHART_DATA_BOTH) {
            ResistanceCharts[IndexOfDisplayedProbe]->drawYAxisTitle(CHART_Y_LABEL_OFFSET_RESISTANCE);
        }
    }

// show eventually buttons and values
    activateOrShowChartGui();
    printBatteryValues();

    drawData(false);
}

/**
 * draws the actual data chart(s)
 * @param doClearBefore do a clear and refresh cleared gui etc. before
 */
void drawData(bool doClearBefore) {
    if (doClearBefore) {
        VoltageCharts[IndexOfDisplayedProbe]->clear();
        VoltageCharts[IndexOfDisplayedProbe]->drawGrid();

        // show eventually gui and values which was cleared before
        activateOrShowChartGui();
        printBatteryValues();

        VoltageCharts[IndexOfDisplayedProbe]->drawXAxisTitle();
    }

    if (AccuCapDisplayControl[IndexOfDisplayedProbe].ActualDataChart == CHART_DATA_BOTH
            || AccuCapDisplayControl[IndexOfDisplayedProbe].ActualDataChart == CHART_DATA_VOLTAGE) {
        if (doClearBefore) {
            VoltageCharts[IndexOfDisplayedProbe]->drawYAxisTitle(CHART_Y_LABEL_OFFSET_VOLTAGE);
        }
        // Voltage buffer
        VoltageCharts[IndexOfDisplayedProbe]->drawChartData(
                (int16_t*) (&BatteryControl[IndexOfDisplayedProbe].VoltageNoLoadDatabuffer[0]
                        + AccuCapDisplayControl[IndexOfDisplayedProbe].XStartIndex
                                * VoltageCharts[IndexOfDisplayedProbe]->getXGridSpacing()),
                (int16_t*) &BatteryControl[IndexOfDisplayedProbe].VoltageNoLoadDatabuffer[BatteryControl[IndexOfDisplayedProbe].SampleCount],
                CHART_MODE_LINE);

    }
    if (AccuCapDisplayControl[IndexOfDisplayedProbe].ActualDataChart == CHART_DATA_BOTH
            || AccuCapDisplayControl[IndexOfDisplayedProbe].ActualDataChart == CHART_DATA_RESISTANCE) {
        // No draw for mode external
        if (BatteryControl[IndexOfDisplayedProbe].Mode != MODE_EXTERNAL_VOLTAGE) {
            if (doClearBefore) {
                ResistanceCharts[IndexOfDisplayedProbe]->drawYAxisTitle(CHART_Y_LABEL_OFFSET_RESISTANCE);
            }
            // Milli-OHM buffer
            ResistanceCharts[IndexOfDisplayedProbe]->drawChartData(
                    (int16_t*) (&BatteryControl[IndexOfDisplayedProbe].ESRMilliohmDatabuffer[0]
                            + AccuCapDisplayControl[IndexOfDisplayedProbe].XStartIndex
                                    * VoltageCharts[IndexOfDisplayedProbe]->getXGridSpacing()),
                    (int16_t*) &BatteryControl[IndexOfDisplayedProbe].ESRMilliohmDatabuffer[BatteryControl[IndexOfDisplayedProbe].SampleCount],
                    CHART_MODE_LINE);
        }
    }
}

/**
 * 	draws buttons if mode == SHOW_MODE_GUI
 * 	else only activate them
 */
void activateOrShowChartGui(void) {
    int tLoopEnd = sizeof(ButtonsChart) / sizeof(ButtonsChart[0]);
    if (AccuCapDisplayControl[IndexOfDisplayedProbe].ChartShowMode == SHOW_MODE_GUI) {
        // render all buttons
        for (int i = 0; i < tLoopEnd; ++i) {
            (*ButtonsChart[i]).drawButton();
        }
        // show Symbols for horizontal swipe
        BlueDisplay1.drawText(BUTTON_WIDTH_5_POS_3,
        BUTTON_HEIGHT_4_LINE_4 - TEXT_SIZE_22_HEIGHT - BUTTON_DEFAULT_SPACING, StringDoubleHorizontalArrow,
        TEXT_SIZE_22, COLOR16_BLUE, BACKGROUND_COLOR);
        BlueDisplay1.drawText(BUTTON_WIDTH_5_POS_3, BUTTON_HEIGHT_4_LINE_4, StringDoubleHorizontalArrow, TEXT_SIZE_22,
        COLOR16_BLUE, BACKGROUND_COLOR);
        // show Symbols for vertical swipe
        BlueDisplay1.drawChar(0, BUTTON_HEIGHT_5_LINE_4, '\xE0', TEXT_SIZE_22, COLOR16_BLUE, BACKGROUND_COLOR);
        BlueDisplay1.drawChar(BUTTON_WIDTH_5 + BUTTON_DEFAULT_SPACING, BUTTON_HEIGHT_5_LINE_4, '\xE0', TEXT_SIZE_22,
        COLOR16_BLUE, BACKGROUND_COLOR);

    } else {
        // activate only next button
        (*ButtonsChart[INDEX_OF_NEXT_BUTTON]).activate();
    }
}

#endif // _ACCU_CAPACITY_HPP
