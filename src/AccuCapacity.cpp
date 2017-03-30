/**
 *  @file AccuCapacity.cpp
 *
 *  @date 23.02.2012
 *  @author Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *  @brief measures the capacity of 1,2V rechargeable batteries while charging or discharging
 *  and shows the voltage and internal resistance graph on LCD screen.
 *
 */

#include "Pages.h"
#include "myStrings.h" // for StringDoubleHorizontalArrow
#include "Chart.h"
#include <string.h>

/*
 * Includes for plain C libs
 */
extern "C" {
#include "ff.h"
}

char StringDischarging[] = "discharging";

const char * const chartStrings[] = { "Volt", "mOhm", "Both" };
const char * const chartStringsExtern[] = { "Max", "Min", "Both" };

#define NUMBER_OF_PROBES 2

#define STOP_VOLTAGE_TIMES_TEN 10  // 1.0 Volt
#define START_Y_OFFSET 1.0
#define READING_CHARGE_VOLTAGE_DEFAULT 0x0CDD
#define SAMPLE_PERIOD_START 60 // in seconds
#define SAMPLE_PERIOD_MINIMUM 10 // in seconds
// Flag to signal end of measurement from ISR to main loop
volatile bool doEndTone = false;
// Flag to signal refresh from Timer Callback (ISR context) to main loop
volatile bool doDisplayRefresh = false;

// Global declarations
void initGUIAccuCapacity(void);
void printMeasurementValues(void);
void clearBasicInfo(void);
void drawData(bool doClearBefore);
void activateOrShowChartGui(void);
void adjustXAxisToSamplePeriod(unsigned int aProbeIndex, int aSamplePeriodSeconds);
bool CheckStopCondition(int aProbeIndex);
uint16_t getInternalResistanceMilliohm(unsigned int aProbeIndex);
void printSamplePeriod(void);
void drawAccuCapacityMainPage(void);
void drawAccuCapacitySettingsPage(void);
void redrawAccuCapacityChartPage(void);
void redrawAccuCapacityPages(void);

void doStartStopAccuCap(BDButton * aTheTouchedButton, int16_t aProbeIndex);

int changeXOffset(int aValue);
int changeXScaleFactor(int aValue);
int changeYOffset(int aValue);
int changeYScaleFactor(int aValue);

void handleTimerMinMaxCallback(int aProbeIndex);
void handleTimerCallback(int aProbeIndex);
void callbackTimerSampleChannel0(void);
void callbackTimerSampleChannel1(void);
void callbackTimerMinMaxSampleChannel0(void);
void callbackTimerMinMaxSampleChannel1(void);
void callbackDisplayRefreshDelay(void);

/**
 * Color scheme
 */
#define COLOR_GUI_CONTROL COLOR_RED
#define COLOR_GUI_VALUES COLOR_GREEN
#define COLOR_GUI_DISPLAY_CONTROL COLOR_YELLOW
// 3 different pages
#define PAGE_MAIN 0
#define PAGE_CHART 1
#define PAGE_SETTINGS 2
int ActualPage = PAGE_MAIN;

uint16_t const ProbeColors[NUMBER_OF_PROBES] = { COLOR_RED, COLOR_BLUE };

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
BDButton * const ButtonsChart[] = { &TouchButtonClearDataBuffer, &TouchButtonExport, &TouchButtonLoadStore,
        &TouchButtonProbeSettings, &TouchButtonMain, &TouchButtonNext, &TouchButtonStartStopCapacityMeasurement,
        &TouchButtonMode, &TouchButtonChartSwitchData };

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
 * LOOP timing
 */
static void (*SampleDelayCallbackFunctions[NUMBER_OF_PROBES * 2])(
        void) = {&callbackTimerSampleChannel0, &callbackTimerMinMaxSampleChannel0, &callbackTimerSampleChannel1, &callbackTimerMinMaxSampleChannel1};

// last sample of millis() in loop as reference for loop duration
unsigned long MillisLastLoopCapacity = 0;

// update display at least all SHOW_PERIOD millis
#define SHOW_PERIOD ONE_SECOND
#define MIN_MAX_SAMPLE_PERIOD_MILLIS 77

/*
 * CHARTS
 */
Chart VoltageChart_0;
Chart VoltageChart_1;
Chart ResistanceChart_0; // Xlabel not used, Xlabel of VoltageChart used instead
Chart ResistanceChart_1; // Xlabel not used, Xlabel of VoltageChart used instead
Chart * const VoltageCharts[NUMBER_OF_PROBES] = { &VoltageChart_0, &VoltageChart_1 };
Chart * const ResistanceCharts[NUMBER_OF_PROBES] = { &ResistanceChart_0, &ResistanceChart_1 };

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

uint8_t ActualProbe = 0; // Which probe is to be displayed - index to AccuCapDisplayControl[] + DataloggerMeasurementControl[]
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

unsigned int readSample(uint8_t aProbeIndex);
void setBasicMeasurementValues(uint8_t aProbeIndex, unsigned int aRawReading);
void setYAutorange(int16_t aProbeIndex, uint16_t aADCReadingToAutorange);

// Modes
#define MODE_DISCHARGING 0x00
#define MODE_CHARGING 0x01
#define MODE_EXTERN 0x02 // For measurement of external voltage
/*
 * Structure defining a capacity measurement
 */
#define RAM_DATABUFFER_MAX_LENGTH 1200 // 12 byte per entry. 1000 -> 16,6 hours for 1 sample per minute
struct DataloggerMeasurementControlStruct {
    bool IsStarted;
    uint8_t Mode;

    uint16_t SamplePeriodSeconds;
    uint16_t SampleCount; // number of samples just taken for measurement
    uint16_t ActualReading;
    uint32_t SampleSumRawReadings; // sum of raw adc readings

    uint16_t Min;
    uint16_t Max;

    uint16_t StopThreshold; // if Reading < StopThreshold then stop - used for discharging

    uint16_t StopMilliampereHour; // if Capacity > StopMilliampereHour then stop - used for charging - 0 means no stop

    uint16_t ChargeVoltageRaw; // is stored in RTC backup register 8 - Maximum is 48 (16*3) Volt at 3 Volt reference

    uint16_t ProbeNumber; // ID number of actual probe

    float OhmLoadResistor;
    float Volt;
    float ExternalAttenuatorFactor; // to compensate external attenuator: ADC Value * factor = Volt
    float Milliampere;
    float OhmAccuResistance;
    float Capacity; //Capacity in microAh
    uint16_t VoltageDatabuffer[RAM_DATABUFFER_MAX_LENGTH]; // if mode == external maximum values are stored here
    uint16_t MinVoltageDatabuffer[RAM_DATABUFFER_MAX_LENGTH];
    uint16_t InternalResistanceDataMilliOhm[RAM_DATABUFFER_MAX_LENGTH];
};
DataloggerMeasurementControlStruct DataloggerMeasurementControl[NUMBER_OF_PROBES];

/*
 * Values and functions for measurement
 */
#define AD_SETTLE_DELAY 30 // Delay in milliseconds to allow AD and probe to settle after switching mode
/****************************************************************************************
 * THE CODE STARTS HERE
 ****************************************************************************************/

/**
 * sets the lines
 * according to aProbeIndex, IsStarted and IsCharging
 */
void setOutputLines(int16_t aProbeIndex) {
// default is extern
    bool tCharge = false;
    bool tDischarge = false;
    if (DataloggerMeasurementControl[aProbeIndex].IsStarted) {
        if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_CHARGING) {
            tCharge = true;
        } else if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_DISCHARGING) {
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
    registerDelayCallback(&callbackDisplayRefreshDelay, SHOW_PERIOD);
    // no drawing here!!! because it may interfere with drawing in tread
    doDisplayRefresh = true;
}

void callbackTimerSampleChannel0(void) {
    handleTimerCallback(0);
}
void callbackTimerSampleChannel1(void) {
    handleTimerCallback(1);
}
void callbackTimerMinMaxSampleChannel0(void) {
    handleTimerMinMaxCallback(0);
}
void callbackTimerMinMaxSampleChannel1(void) {
    handleTimerMinMaxCallback(1);
}
void handleTimerMinMaxCallback(int aProbeIndex) {
    // reload timer
    registerDelayCallback(SampleDelayCallbackFunctions[(aProbeIndex * 2) + 1], MIN_MAX_SAMPLE_PERIOD_MILLIS);
    unsigned int tAccuReading = readSample(aProbeIndex);
    // compare with existent values
    if (tAccuReading > DataloggerMeasurementControl[aProbeIndex].Max) {
        DataloggerMeasurementControl[aProbeIndex].Max = tAccuReading;
    } else if (tAccuReading < DataloggerMeasurementControl[aProbeIndex].Min) {
        DataloggerMeasurementControl[aProbeIndex].Min = tAccuReading;
    }
}

void handleTimerCallback(int aProbeIndex) {
    // reload timer
    registerDelayCallback(SampleDelayCallbackFunctions[aProbeIndex * 2],
            DataloggerMeasurementControl[aProbeIndex].SamplePeriodSeconds * 1000);

    if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_EXTERN) {
        // store maximum
        DataloggerMeasurementControl[aProbeIndex].VoltageDatabuffer[DataloggerMeasurementControl[aProbeIndex].SampleCount] =
                DataloggerMeasurementControl[aProbeIndex].Max;
        // assume extern charging
        DataloggerMeasurementControl[aProbeIndex].SampleSumRawReadings +=
                DataloggerMeasurementControl[aProbeIndex].ChargeVoltageRaw
                        - DataloggerMeasurementControl[aProbeIndex].Max;

        // store minimum
        DataloggerMeasurementControl[aProbeIndex].MinVoltageDatabuffer[DataloggerMeasurementControl[aProbeIndex].SampleCount] =
                DataloggerMeasurementControl[aProbeIndex].Min;
        // init min + max for next compare
        DataloggerMeasurementControl[aProbeIndex].Max = 0;
        DataloggerMeasurementControl[aProbeIndex].Min = 0xFFFF;
    } else {
        unsigned int tAccuReading = readSample(aProbeIndex);
        // store sample
        DataloggerMeasurementControl[aProbeIndex].VoltageDatabuffer[DataloggerMeasurementControl[aProbeIndex].SampleCount] =
                tAccuReading;
        if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_DISCHARGING) {
            DataloggerMeasurementControl[aProbeIndex].SampleSumRawReadings += tAccuReading;
        } else {
            DataloggerMeasurementControl[aProbeIndex].SampleSumRawReadings +=
                    DataloggerMeasurementControl[aProbeIndex].ChargeVoltageRaw - tAccuReading;
        }
    }

    uint32_t tCap = DataloggerMeasurementControl[aProbeIndex].SampleSumRawReadings
            * DataloggerMeasurementControl[aProbeIndex].SamplePeriodSeconds;
    // 277.777 = 1000 *(10/36)
    DataloggerMeasurementControl[aProbeIndex].Capacity = tCap * sADCToVoltFactor * 277.777
            / DataloggerMeasurementControl[aProbeIndex].OhmLoadResistor;

// stop detection here, because it uses actual reading which is changed by getInternalResistanceMilliohm() :-(
    bool tStop = CheckStopCondition(aProbeIndex);

    // Do not measure internal resistance if channe 0 and node == discharge and only volt chart is shown
    if (aProbeIndex
            != 0|| DataloggerMeasurementControl[0].Mode != MODE_DISCHARGING || AccuCapDisplayControl[0].ActualDataChart != CHART_DATA_VOLTAGE) {
        DataloggerMeasurementControl[aProbeIndex].InternalResistanceDataMilliOhm[DataloggerMeasurementControl[aProbeIndex].SampleCount] =
                getInternalResistanceMilliohm(aProbeIndex);
    }
    DataloggerMeasurementControl[aProbeIndex].SampleCount++;

    if (tStop) {
        // Stop Measurement
        doStartStopAccuCap(&TouchButtonStartStopCapacityMeasurement, aProbeIndex);
        // signal end of measurement
        doEndTone = true;
        return;
    }
    if (ActualPage == PAGE_CHART) {
        if (ActualProbe == aProbeIndex) {
            // Redraw chart
            drawData(false);
        }
        if ((aProbeIndex == ActualProbe) && (DataloggerMeasurementControl[aProbeIndex].SampleCount == 1)
                && (AccuCapDisplayControl[aProbeIndex].ChartShowMode == SHOW_MODE_GUI)) {
            // set caption to "Store"
            TouchButtonLoadStore.setCaption("Store");
            TouchButtonLoadStore.drawButton();
        }
    }
}

/**
 *
 * @param aProbeIndex
 * @return true if measurement has to be stopped
 */
bool CheckStopCondition(int aProbeIndex) {
    if (DataloggerMeasurementControl[aProbeIndex].SampleCount >= RAM_DATABUFFER_MAX_LENGTH - 1) {
        return true;
    }
    if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_DISCHARGING) {
        return (DataloggerMeasurementControl[aProbeIndex].ActualReading
                < DataloggerMeasurementControl[aProbeIndex].StopThreshold);
    } else if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_CHARGING
            && DataloggerMeasurementControl[aProbeIndex].StopMilliampereHour != 0) {
        return ((DataloggerMeasurementControl[aProbeIndex].Capacity / 1000)
                > DataloggerMeasurementControl[aProbeIndex].StopMilliampereHour);
    }
    return false;
}

/**
 *
 * @param aProbeIndex
 * @return 0 if external mode or value not plausible
 */
uint16_t getInternalResistanceMilliohm(unsigned int aProbeIndex) {
    // No internal resistance measurement possible in extern mode
    if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_EXTERN) {
        return 0;
    }
// disconnect from sink/source
    if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_CHARGING) {
        AccuCapacity_SwitchCharge(aProbeIndex, false);
    } else {
        AccuCapacity_SwitchDischarge(aProbeIndex, false);
    }

    delayMillis(AD_SETTLE_DELAY);

// save old voltage value (under load)
    float tVoltLoad = DataloggerMeasurementControl[aProbeIndex].Volt;
    float tVoltNoLoad = sADCToVoltFactor * readSample(aProbeIndex);
// restore original value
    DataloggerMeasurementControl[aProbeIndex].Volt = tVoltLoad;

// reconnect to sink/source
    if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_CHARGING) {
        AccuCapacity_SwitchCharge(aProbeIndex, true);
    } else {
        AccuCapacity_SwitchDischarge(aProbeIndex, true);
    }

    float tResistance = 0;
    if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_CHARGING) {
        // Charging
        if (tVoltNoLoad >= tVoltLoad) {
            // plausibility fails
            DataloggerMeasurementControl[aProbeIndex].OhmAccuResistance = 0;
        } else {
            // compute resistance
            tResistance = ((tVoltLoad - tVoltNoLoad) * 1000) / DataloggerMeasurementControl[aProbeIndex].Milliampere;
        }
    } else {
        // Discharging
        if (tVoltNoLoad <= tVoltLoad || tVoltLoad == 0) {
            // plausibility fails
            DataloggerMeasurementControl[aProbeIndex].OhmAccuResistance = 0;
        } else {
            // compute resistance
            tResistance = ((tVoltNoLoad - tVoltLoad) * 1000) / DataloggerMeasurementControl[aProbeIndex].Milliampere;
        }
    }
    // Plausi
    if (tResistance > 0 && tResistance < 10) {
        DataloggerMeasurementControl[aProbeIndex].OhmAccuResistance = tResistance;
    }
    return (DataloggerMeasurementControl[aProbeIndex].OhmAccuResistance * 1000);
}

void TouchUpHandlerAccuCapacity(struct TouchEvent * const aTouchPosition) {
    // first check for buttons
    if (!TouchButton::checkAllButtons(aTouchPosition->TouchPosition.PosX, aTouchPosition->TouchPosition.PosY)) {
        if (ActualPage == PAGE_CHART) {
            //touch press but no gui element matched?
            // switch chart overlay;
            doSwitchChartOverlay();
        }
    }
}

/**
 * Shows gui on long touch
 * @param aTouchPositionX
 * @param aTouchPositionY
 * @param isLongTouch
 * @return
 */

void longTouchHandlerAccuCapacity(struct TouchEvent * const aTouchPosition) {
    // show gui
    if (ActualPage == PAGE_CHART) {
        if (AccuCapDisplayControl[ActualProbe].ChartShowMode == SHOW_MODE_DATA) {
            clearBasicInfo();
        }
        AccuCapDisplayControl[ActualProbe].ChartShowMode = SHOW_MODE_GUI;
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
void swipeEndHandlerAccuCapacity(struct Swipe * const aSwipeInfo) {

    int tFeedbackType;
    if (ActualPage == PAGE_CHART) {
        if (aSwipeInfo->SwipeMainDirectionIsX) {
            int tTouchDeltaGrid = aSwipeInfo->TouchDeltaX / CHART_GRID_X_SIZE;
            // Horizontal swipe
            if (aSwipeInfo->TouchStartY > BUTTON_HEIGHT_4_LINE_4) {
                // scroll
                tFeedbackType = changeXOffset(-tTouchDeltaGrid);
            } else {
                // scale
                tFeedbackType = changeXScaleFactor(tTouchDeltaGrid / 2);
            }
        } else {
            int tTouchDeltaGrid = aSwipeInfo->TouchDeltaY / CHART_GRID_Y_SIZE;
            // Vertical swipe
            if (aSwipeInfo->TouchStartX < BUTTON_WIDTH_5) {
                //offset
                tFeedbackType = changeYOffset(tTouchDeltaGrid);
            } else {
                //range
                tFeedbackType = changeYScaleFactor(-tTouchDeltaGrid);
            }
        }
    } else {
        // swipe on start page
        tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
    }
#ifdef LOCAL_DISPLAY_EXISTS
    FeedbackTone(tFeedbackType);
#else
    BlueDisplay1.playFeedbackTone(tFeedbackType);
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
        VoltageCharts[i]->setDataColor(COLOR_BLUE);

        // y axis
        DataloggerMeasurementControl[i].ExternalAttenuatorFactor = 1.0;
        VoltageCharts[i]->initYLabelFloat(START_Y_OFFSET, CHART_MIN_Y_INCREMENT_VOLT, sADCToVoltFactor, 3, 1);
        VoltageCharts[i]->setYTitleText("Volt");
        AccuCapDisplayControl[i].VoltYScaleFactor = 0;            // 0=identity

        ResistanceCharts[i]->initChart(CHART_START_X, CHART_START_Y, CHART_WIDTH, CHART_HEIGHT, 2, true,
        CHART_GRID_X_SIZE,
        CHART_GRID_Y_SIZE);
        ResistanceCharts[i]->setDataColor(COLOR_RED);

        // y axis
        ResistanceCharts[i]->initYLabelInt(0, CHART_MIN_Y_INCREMENT_RESISTANCE, 1, 3);
        ResistanceCharts[i]->setYTitleText("mOhm");
        AccuCapDisplayControl[i].ResistorYScaleFactor = 0;            // 0=identity

        // x axis for both charts
        adjustXAxisToSamplePeriod(i, SAMPLE_PERIOD_START);

        // init control
        DataloggerMeasurementControl[i].IsStarted = false;
        DataloggerMeasurementControl[i].Mode = MODE_DISCHARGING;
        DataloggerMeasurementControl[i].Capacity = 0;
        DataloggerMeasurementControl[i].SampleSumRawReadings = 0;
        DataloggerMeasurementControl[i].SamplePeriodSeconds = SAMPLE_PERIOD_START;
        DataloggerMeasurementControl[i].OhmLoadResistor = 5.0;
        DataloggerMeasurementControl[i].StopThreshold = sReading3Volt * STOP_VOLTAGE_TIMES_TEN / 30;
        DataloggerMeasurementControl[i].StopMilliampereHour = 0;
        DataloggerMeasurementControl[i].ProbeNumber = i;
        DataloggerMeasurementControl[i].SampleCount = 0;

        AccuCapDisplayControl[i].ChartShowMode = SHOW_MODE_GUI;
        AccuCapDisplayControl[i].ActualDataChart = CHART_DATA_BOTH;
        // load from cmos ram
        if (RTC_checkMagicNumber()) {
            DataloggerMeasurementControl[i].ChargeVoltageRaw = HAL_RTCEx_BKUPRead(&RTCHandle, RTC_BKP_DR9) & 0xFFFF;
        } else {
            DataloggerMeasurementControl[i].ChargeVoltageRaw = READING_CHARGE_VOLTAGE_DEFAULT;
        }
    }
}

void startAccuCapacity(void) {
    initGUIAccuCapacity();
    // New calibration of chart since VCC may have changed
    ADC_setRawToVoltFactor();
    VoltageCharts[0]->setYDataFactor(sADCToVoltFactor);
    VoltageCharts[1]->setYDataFactor(sADCToVoltFactor);

    drawAccuCapacityMainPage();
    registerRedrawCallback(&redrawAccuCapacityPages);

    // start display refresh
    registerDelayCallback(&callbackDisplayRefreshDelay, SHOW_PERIOD);

    registerLongTouchDownCallback(&longTouchHandlerAccuCapacity, TOUCH_STANDARD_LONG_TOUCH_TIMEOUT_MILLIS);
    // use touch up for buttons in order not to interfere with long touch
    registerTouchUpCallback(&TouchUpHandlerAccuCapacity);
    registerSwipeEndCallback(&swipeEndHandlerAccuCapacity);
    registerTouchDownCallback(NULL);
}

void stopAccuCapacity(void) {
    // Stop display refresh
    changeDelayCallback(&callbackDisplayRefreshDelay, DISABLE_TIMER_DELAY_VALUE);
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

    registerLongTouchDownCallback(NULL, 0);
    registerSwipeEndCallback(NULL);
    registerTouchUpCallback(NULL);
    // restore old touch down handler
    registerTouchDownCallback(&simpleTouchDownHandler);
}

void loopAccuCapacity(void) {
// check Flags from ISR
    if (doDisplayRefresh == true) {
        for (int i = 0; i < NUMBER_OF_PROBES; ++i) {
            readSample(i);
        }
        printMeasurementValues();
        doDisplayRefresh = false;
    }
    if (doEndTone == true) {
        doEndTone = false;
        EndTone();
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
void doShowChartScreen(BDButton * aTheTouchedButton, int16_t aProbeIndex) {
    ActualProbe = aProbeIndex;
    redrawAccuCapacityChartPage();
}

/**
 * scrolls chart data left and right
 */
int changeXOffset(int aValue) {
    signed int tNewValue = AccuCapDisplayControl[ActualProbe].XStartIndex
            + VoltageCharts[ActualProbe]->adjustIntWithXScaleFactor(aValue);
    int tRetValue = FEEDBACK_TONE_NO_ERROR;
    if (tNewValue < 0) {
        tNewValue = 0;
        tRetValue = FEEDBACK_TONE_SHORT_ERROR;
    } else if (tNewValue * VoltageCharts[ActualProbe]->getXGridSpacing()
            > DataloggerMeasurementControl[ActualProbe].SampleCount) {
        tNewValue = DataloggerMeasurementControl[ActualProbe].SampleCount
                / VoltageCharts[ActualProbe]->getXGridSpacing();
        tRetValue = FEEDBACK_TONE_SHORT_ERROR;
    }
    AccuCapDisplayControl[ActualProbe].XStartIndex = tNewValue;
    drawData(true);
// redraw xLabels
    VoltageCharts[ActualProbe]->setXLabelIntStartValueByIndex((int) tNewValue, true);

    return tRetValue;
}

int changeXScaleFactor(int aValue) {
    signed int tXScaleFactor = VoltageCharts[ActualProbe]->getXScaleFactor() + aValue;
// redraw xLabels
    VoltageCharts[ActualProbe]->setXScaleFactor(tXScaleFactor, true);
    ResistanceCharts[ActualProbe]->setXScaleFactor(tXScaleFactor, false);
    drawData(true);
    return FEEDBACK_TONE_NO_ERROR;
}

int changeYOffset(int aValue) {
    VoltageCharts[ActualProbe]->stepYLabelStartValueFloat(aValue);
// refresh screen
    redrawAccuCapacityChartPage();
    return FEEDBACK_TONE_NO_ERROR;
}

/**
 * increments or decrements the Y scale factor for charts
 * @param aTheTouchedButton not used
 * @param aValue is 1 or -1
 */
int changeYScaleFactor(int aValue) {
    if (AccuCapDisplayControl[ActualProbe].ActualDataChart == CHART_DATA_RESISTANCE) {
        AccuCapDisplayControl[ActualProbe].ResistorYScaleFactor += aValue;
        ResistanceCharts[ActualProbe]->setYLabelBaseIncrementValue(
                adjustIntWithScaleFactor(CHART_MIN_Y_INCREMENT_RESISTANCE,
                        AccuCapDisplayControl[ActualProbe].ResistorYScaleFactor));
    } else {
        AccuCapDisplayControl[ActualProbe].VoltYScaleFactor += aValue;
        VoltageCharts[ActualProbe]->setYLabelBaseIncrementValueFloat(
                adjustFloatWithScaleFactor(CHART_MIN_Y_INCREMENT_VOLT,
                        AccuCapDisplayControl[ActualProbe].VoltYScaleFactor));
    }

// refresh screen
    redrawAccuCapacityChartPage();
    return FEEDBACK_TONE_NO_ERROR;
}

static void doShowSettings(BDButton * aTheTouchedButton, int16_t aValue) {
    drawAccuCapacitySettingsPage();
}

void doSetProbeNumber(BDButton * aTheTouchedButton, int16_t aValue) {

    float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, ProbeColors[ActualProbe]);
// check for cancel
    if (!isnan(tNumber)) {
        DataloggerMeasurementControl[ActualProbe].ProbeNumber = tNumber;
    }
    drawAccuCapacitySettingsPage();
}

void doSetStopValue(BDButton * aTheTouchedButton, int16_t aValue) {
    float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, ProbeColors[ActualProbe]);
// check for cancel
    if (!isnan(tNumber)) {
        if (DataloggerMeasurementControl[ActualProbe].Mode == MODE_CHARGING) {
            // set capacity
            DataloggerMeasurementControl[ActualProbe].StopMilliampereHour = (tNumber * 11) / 8; // for loading factor of 1,375
        } else {
            DataloggerMeasurementControl[ActualProbe].StopThreshold = tNumber / sADCToVoltFactor;
        }
    }
    drawAccuCapacitySettingsPage();
}

void doSetLoadResistorValue(BDButton * aTheTouchedButton, int16_t aValue) {
    float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, ProbeColors[ActualProbe]);
// check for cancel
    if (!isnan(tNumber)) {
        DataloggerMeasurementControl[ActualProbe].OhmLoadResistor = tNumber;
    }
    drawAccuCapacitySettingsPage();
}

void doSetExternalAttenuatorFactor(BDButton * aTheTouchedButton, int16_t aValue) {
    float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, ProbeColors[ActualProbe]);
// check for cancel
    if (!isnan(tNumber)) {
        DataloggerMeasurementControl[ActualProbe].ExternalAttenuatorFactor = tNumber;
    }
    drawAccuCapacitySettingsPage();
}

/**
 * stores the actual reading for the charging voltage in order to compute charge current etc.
 */
void doSetReadingChargeVoltage(BDButton * aTheTouchedButton, int16_t aValue) {
    unsigned int tAccuReading = 0;
    if (DataloggerMeasurementControl[ActualProbe].Mode == MODE_EXTERN) {
        float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, ProbeColors[ActualProbe]);
        // check for cancel
        if (!isnan(tNumber)) {
            tAccuReading = tNumber / sADCToVoltFactor;
        }
    } else {
        // Mode charging here because button is not shown on discharging
        // oversampling ;-)
        for (int i = 0; i < 8; ++i) {
            tAccuReading += AccuCapacity_ADCRead(ActualProbe);
        }
        tAccuReading /= 8;
        // store permanent
        HAL_PWR_EnableBkUpAccess();
        HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR9, tAccuReading);
        RTC_PWRDisableBkUpAccess();
    }
    DataloggerMeasurementControl[ActualProbe].ChargeVoltageRaw = tAccuReading;
    drawAccuCapacitySettingsPage();
}

void doSetSamplePeriod(BDButton * aTheTouchedButton, int16_t aValue) {
    unsigned int tFeedbackType = FEEDBACK_TONE_NO_ERROR;
    unsigned int tSeconds = DataloggerMeasurementControl[ActualProbe].SamplePeriodSeconds;
    tSeconds += aValue * 10;
    if (tSeconds <= 0) {
        tSeconds = SAMPLE_PERIOD_MINIMUM;
        tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
    }
    DataloggerMeasurementControl[ActualProbe].SamplePeriodSeconds = tSeconds;
#ifdef LOCAL_DISPLAY_EXISTS
    FeedbackTone(tFeedbackType);
#else
    BlueDisplay1.playFeedbackTone(tFeedbackType);
#endif
    adjustXAxisToSamplePeriod(ActualProbe, tSeconds);
// Print value
    printSamplePeriod();
}
/**
 * shows main menu
 */
void doShowMainScreen(BDButton * aTheTouchedButton, int16_t aValue) {
    BDButton::deactivateAllButtons();
    drawAccuCapacityMainPage();
}

/**
 * changes the displayed content in the chart page
 * toggles between short and long value display
 */
void doSwitchChartOverlay(void) {
    FeedbackToneOK();
// change display / overlay mode
    AccuCapDisplayControl[ActualProbe].ChartShowMode++;
    if (AccuCapDisplayControl[ActualProbe].ChartShowMode > SHOW_MODE_VALUES) {
        AccuCapDisplayControl[ActualProbe].ChartShowMode = SHOW_MODE_DATA;

        BDButton::deactivateAllButtons();
        // only chart no gui
        drawData(true);            // calls activateOrShowChartGui()
    } else {
        //  delete basic data and show full data
        clearBasicInfo();
        VoltageCharts[ActualProbe]->drawGrid();
        printMeasurementValues();
    }
}

/**
 * switch between voltage and resistance (and both) charts
 * in external mode switch between Max(voltage) and Min(resistance) charts
 */
void doSwitchChartData(BDButton * aTheTouchedButton, int16_t aProbeIndex) {
    AccuCapDisplayControl[aProbeIndex].ActualDataChart++;
    if (AccuCapDisplayControl[aProbeIndex].ActualDataChart > CHART_DATA_BOTH) {
        AccuCapDisplayControl[aProbeIndex].ActualDataChart = CHART_DATA_VOLTAGE;
    } else if (AccuCapDisplayControl[aProbeIndex].ActualDataChart == CHART_DATA_RESISTANCE
            && DataloggerMeasurementControl[aProbeIndex].Mode != MODE_EXTERN) {
        // no resistance for external mode (U min instead)
        // y axis for resistance only shown at CHART_RESISTANCE
        ResistanceCharts[ActualProbe]->drawYAxis(true);
        ResistanceCharts[ActualProbe]->drawYAxisTitle(CHART_Y_LABEL_OFFSET_RESISTANCE);
    } else if (AccuCapDisplayControl[aProbeIndex].ActualDataChart == CHART_DATA_BOTH) {
        VoltageCharts[ActualProbe]->drawYAxis(true);
        ResistanceCharts[ActualProbe]->drawYAxisTitle(CHART_Y_LABEL_OFFSET_VOLTAGE);
    }
    if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_EXTERN) {
        aTheTouchedButton->setCaption(chartStringsExtern[AccuCapDisplayControl[aProbeIndex].ActualDataChart]);
    } else {
        aTheTouchedButton->setCaption(chartStrings[AccuCapDisplayControl[aProbeIndex].ActualDataChart]);
    }
    drawData(true);
}

void doStartStopAccuCap(BDButton * aTheTouchedButton, int16_t aProbeIndex) {
    DataloggerMeasurementControl[aProbeIndex].IsStarted = !DataloggerMeasurementControl[aProbeIndex].IsStarted;
    if (DataloggerMeasurementControl[aProbeIndex].IsStarted) {
        /*
         * START
         */
        setOutputLines(aProbeIndex);
        aTheTouchedButton->setCaption("Stop");
        delayMillis(AD_SETTLE_DELAY);
        readSample(aProbeIndex);
        // Autorange if first value
        if (DataloggerMeasurementControl[aProbeIndex].SampleCount == 0) {
            // let measurement value start at middle of y axis
            setYAutorange(aProbeIndex, DataloggerMeasurementControl[aProbeIndex].ActualReading);
        }
        printMeasurementValues();
        changeDelayCallback(SampleDelayCallbackFunctions[aProbeIndex * 2],
                DataloggerMeasurementControl[aProbeIndex].SamplePeriodSeconds * 1000);
        // start min max detection
        if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_EXTERN) {
            DataloggerMeasurementControl[0].Max = 0;
            DataloggerMeasurementControl[0].Min = 0xFFFF;
            changeDelayCallback(SampleDelayCallbackFunctions[(aProbeIndex * 2) + 1], MIN_MAX_SAMPLE_PERIOD_MILLIS);
        }
    } else {
        /*
         *  STOP
         */
        changeDelayCallback(SampleDelayCallbackFunctions[aProbeIndex * 2], DISABLE_TIMER_DELAY_VALUE);
        if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_EXTERN) {
            changeDelayCallback(SampleDelayCallbackFunctions[(aProbeIndex * 2) + 1], DISABLE_TIMER_DELAY_VALUE);
        }
        getInternalResistanceMilliohm(aProbeIndex);
        setOutputLines(aProbeIndex);
        aTheTouchedButton->setCaption("Start");

        // no addition to capacity on stop because start may be pressed in same measurement interval
        // -> this would lead to double adding to capacity

    }
    if (AccuCapDisplayControl[ActualProbe].ChartShowMode == SHOW_MODE_GUI) {
        aTheTouchedButton->drawButton();
    }
}

void doChargeMode(BDButton * aTheTouchedButton, int16_t aProbeIndex) {
    unsigned int tNewState = DataloggerMeasurementControl[aProbeIndex].Mode;
    tNewState++;
    if (tNewState > MODE_EXTERN) {
        tNewState = MODE_DISCHARGING;
        changeDelayCallback(SampleDelayCallbackFunctions[(aProbeIndex * 2) + 1], DISABLE_TIMER_DELAY_VALUE);
    }
    DataloggerMeasurementControl[aProbeIndex].Mode = tNewState;
    setOutputLines(aProbeIndex);
    setModeCaption(ActualProbe);
// start min max detection
    if (tNewState == MODE_EXTERN) {
        DataloggerMeasurementControl[0].Max = 0;
        DataloggerMeasurementControl[0].Min = 0xFFFF;
        changeDelayCallback(SampleDelayCallbackFunctions[(aProbeIndex * 2) + 1], MIN_MAX_SAMPLE_PERIOD_MILLIS);
    }

    if (AccuCapDisplayControl[ActualProbe].ChartShowMode == SHOW_MODE_GUI) {
        aTheTouchedButton->drawButton();
    }
}

void doClearDataBuffer(BDButton * aTheTouchedButton, int16_t aProbeIndex) {
    AccuCapDisplayControl[ActualProbe].XStartIndex = 0;
// redraw xLabels
    VoltageCharts[ActualProbe]->setXLabelIntStartValueByIndex(0, false);
    VoltageCharts[ActualProbe]->setXScaleFactor(1, true);
    ResistanceCharts[ActualProbe]->setXScaleFactor(1, false);

    DataloggerMeasurementControl[aProbeIndex].Capacity = 0;
    DataloggerMeasurementControl[aProbeIndex].SampleCount = 0;
    DataloggerMeasurementControl[aProbeIndex].SampleSumRawReadings = 0;
// Clear data buffer
    memset(&DataloggerMeasurementControl[aProbeIndex].VoltageDatabuffer[0], 0, RAM_DATABUFFER_MAX_LENGTH);
    memset(&DataloggerMeasurementControl[aProbeIndex].MinVoltageDatabuffer[0], 0, RAM_DATABUFFER_MAX_LENGTH);
    memset(&DataloggerMeasurementControl[aProbeIndex].InternalResistanceDataMilliOhm[0], 0,
    RAM_DATABUFFER_MAX_LENGTH);
    readSample(aProbeIndex);
// set button text to load
    TouchButtonLoadStore.setCaption("Load");
    drawData(true);
}

const char * getModeString(int aProbeIndex) {
    const char * tModeString;
    if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_EXTERN) {
        tModeString = "Extern";
    } else
        tModeString = &StringDischarging[0];
    if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_CHARGING) {
        tModeString = &StringDischarging[3];
    }
    return tModeString;
}

static void doStoreLoadChart(BDButton * aTheTouchedButton, int16_t aProbeIndex) {
    unsigned int tFeedbackType = FEEDBACK_TONE_LONG_ERROR;
    if (MICROSD_isCardInserted()) {
        FIL tFile;
        FRESULT tOpenResult;
        UINT tCount;
        snprintf(sStringBuffer, sizeof sStringBuffer, "channel%d_%s.bin", aProbeIndex, getModeString(aProbeIndex));

        if (DataloggerMeasurementControl[aProbeIndex].SampleCount == 0) {
            // first stop running measurement
            if (DataloggerMeasurementControl[aProbeIndex].IsStarted) {
                doStartStopAccuCap(&TouchButtonStartStopCapacityMeasurement, aProbeIndex);
            }
            /*
             * Load data from file
             */
            tOpenResult = f_open(&tFile, sStringBuffer, FA_OPEN_EXISTING | FA_READ);
            if (tOpenResult == FR_OK) {
                // read AccuCapDisplayControl structure to reproduce the layout
                f_read(&tFile, &AccuCapDisplayControl[aProbeIndex], sizeof(AccuCapDisplayControl[aProbeIndex]),
                        &tCount);
                // read DataloggerMeasurementControl structure - including data buffers
                f_read(&tFile, &DataloggerMeasurementControl[aProbeIndex],
                        sizeof(DataloggerMeasurementControl[aProbeIndex]), &tCount);
                f_close(&tFile);
                // set state to stop
                DataloggerMeasurementControl[aProbeIndex].IsStarted = false;
                setYAutorange(aProbeIndex, DataloggerMeasurementControl[aProbeIndex].VoltageDatabuffer[0]);
                AccuCapDisplayControl[aProbeIndex].ActualDataChart = CHART_DATA_BOTH;
                // redraw display corresponding to new values
                redrawAccuCapacityChartPage();
                tFeedbackType = FEEDBACK_TONE_NO_ERROR;
            }
        } else {
            /*
             * Store data to file
             */
            tOpenResult = f_open(&tFile, sStringBuffer, FA_CREATE_ALWAYS | FA_WRITE);
            if (tOpenResult == FR_OK) {
                // write display control to reproduce the layout
                f_write(&tFile, &AccuCapDisplayControl[aProbeIndex], sizeof(AccuCapDisplayControl[aProbeIndex]),
                        &tCount);
                // write control structure including the data buffers
                f_write(&tFile, &DataloggerMeasurementControl[aProbeIndex],
                        sizeof(DataloggerMeasurementControl[aProbeIndex]), &tCount);
                f_close(&tFile);
                tFeedbackType = FEEDBACK_TONE_NO_ERROR;
            }
        }
    }
#ifdef LOCAL_DISPLAY_EXISTS
    FeedbackTone(tFeedbackType);
#else
    BlueDisplay1.playFeedbackTone(tFeedbackType);
#endif
}

/**
 * Export Data Buffer to CSV file
 * @param aTheTouchedButton
 * @param aProbeIndex
 */
static void doExportChart(BDButton * aTheTouchedButton, int16_t aProbeIndex) {
    unsigned int tFeedbackType = FEEDBACK_TONE_LONG_ERROR;
    if (MICROSD_isCardInserted()) {
        FIL tFile;
        FRESULT tOpenResult;
        UINT tCount;
        unsigned int tIndex = RTC_getDateStringForFile(sStringBuffer);
        snprintf(&sStringBuffer[tIndex], sizeof sStringBuffer - tIndex, " probe%d_%s.csv",
                DataloggerMeasurementControl[aProbeIndex].ProbeNumber, getModeString(aProbeIndex));
        if (DataloggerMeasurementControl[aProbeIndex].SampleCount != 0) {
            tOpenResult = f_open(&tFile, sStringBuffer, FA_CREATE_ALWAYS | FA_WRITE);
            if (tOpenResult != FR_OK) {
                failParamMessage(tOpenResult, "f_open");
            } else {
                unsigned int tSeconds = DataloggerMeasurementControl[ActualProbe].SamplePeriodSeconds;
                unsigned int tMinutes = tSeconds / 60;
                tSeconds %= 60;
                tIndex = snprintf(sStringBuffer, sizeof sStringBuffer,
                        "Probe Nr:%d\nSample interval:%u:%02u min\nDate:",
                        DataloggerMeasurementControl[aProbeIndex].ProbeNumber, tMinutes, tSeconds);
                tIndex += RTC_getTimeString(&sStringBuffer[tIndex]);
                snprintf(&sStringBuffer[tIndex], (sizeof sStringBuffer) - tIndex,
                        "\nCapacity:%4fmAH\nInternal resistance:%5.2f Ohm\nVolt Max;Volt Min;mOhm\n",
                        DataloggerMeasurementControl[aProbeIndex].Capacity / 1000,
                        DataloggerMeasurementControl[aProbeIndex].OhmAccuResistance);
                f_write(&tFile, sStringBuffer, strlen(sStringBuffer), &tCount);

                /**
                 * data
                 */
                tIndex = 0;
                uint16_t * tMaxVoltDataPtr = &DataloggerMeasurementControl[aProbeIndex].VoltageDatabuffer[0];
                uint16_t * tMinVoltDataPtr = &DataloggerMeasurementControl[aProbeIndex].MinVoltageDatabuffer[0];
                uint16_t * tOhmDataPtr = &DataloggerMeasurementControl[aProbeIndex].InternalResistanceDataMilliOhm[0];
                for (int i = 0; i < DataloggerMeasurementControl[aProbeIndex].SampleCount; ++i) {
                    tIndex += snprintf(&sStringBuffer[tIndex], (sizeof sStringBuffer) - tIndex, "%6.3f;%6.3f;%4d\n",
                            sADCToVoltFactor * (*tMaxVoltDataPtr++), sADCToVoltFactor * (*tMinVoltDataPtr++),
                            (*tOhmDataPtr++));
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
                tFeedbackType = FEEDBACK_TONE_NO_ERROR;
            }
        }
    }
#ifdef LOCAL_DISPLAY_EXISTS
    FeedbackTone(tFeedbackType);
#else
    BlueDisplay1.playFeedbackTone(tFeedbackType);
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
    TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doShowChartScreen);
    TouchButtonsMain[1].init(BUTTON_WIDTH_2_5_POS_2, 0, BUTTON_WIDTH_2_5, BUTTON_HEIGHT_4, ProbeColors[1], "Probe 2",
    TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 1, &doShowChartScreen);

    /*
     * chart buttons from left to right
     */
    unsigned int PosY = BUTTON_HEIGHT_5 + (BUTTON_DEFAULT_SPACING / 2);
    /*
     * 1. column
     */
    TouchButtonProbeSettings.init(BUTTON_WIDTH_5_POS_2, 0, BUTTON_WIDTH_5, BUTTON_HEIGHT_5,
    COLOR_GUI_CONTROL, "Sett.", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doShowSettings);
    TouchButtonClearDataBuffer.init(BUTTON_WIDTH_5_POS_2, PosY, BUTTON_WIDTH_5, BUTTON_HEIGHT_5,
    COLOR_GUI_VALUES, "Clear", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doClearDataBuffer);
    TouchButtonChartSwitchData.init(BUTTON_WIDTH_5_POS_2, 2 * (BUTTON_HEIGHT_5 + (BUTTON_DEFAULT_SPACING / 2)),
    BUTTON_WIDTH_5, BUTTON_HEIGHT_5, COLOR_GUI_DISPLAY_CONTROL, "Volt", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0,
            &doSwitchChartData);

    /*
     * 2. column
     */
    TouchButtonStartStopCapacityMeasurement.init(BUTTON_WIDTH_5_POS_3, 0, BUTTON_WIDTH_5,
    BUTTON_HEIGHT_5, COLOR_GUI_CONTROL, "", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doStartStopAccuCap);
    TouchButtonMode.init(BUTTON_WIDTH_5_POS_3, PosY, BUTTON_WIDTH_5, BUTTON_HEIGHT_5,
    COLOR_GUI_VALUES, "", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doChargeMode);

    /*
     * 3. column
     */
    TouchButtonNext.init(BUTTON_WIDTH_5_POS_4, 0, BUTTON_WIDTH_5, BUTTON_HEIGHT_5,
    COLOR_GUI_CONTROL, "Next", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doShowChartScreen);
    TouchButtonLoadStore.init(BUTTON_WIDTH_5_POS_4, PosY, BUTTON_WIDTH_5, BUTTON_HEIGHT_5,
    COLOR_GUI_VALUES, "", TEXT_SIZE_11, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0, &doStoreLoadChart);

    /*
     * 4. column
     */
    TouchButtonMain.init(BUTTON_WIDTH_5_POS_5, 0, BUTTON_WIDTH_5, BUTTON_HEIGHT_5,
    COLOR_GUI_CONTROL, "Back", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doShowMainScreen);
    TouchButtonExport.init(BUTTON_WIDTH_5_POS_5, PosY, BUTTON_WIDTH_5, BUTTON_HEIGHT_5,
    COLOR_GUI_VALUES, "Export", TEXT_SIZE_11, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0, &doExportChart);

    /*
     * Settings page buttons
     */
// 1. row
    int tPosY = 0;
    TouchButtonSetProbeNumber.init(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR_GUI_VALUES, StringProbeNumber,
    TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doSetProbeNumber);

    TouchButtonBack.init(BUTTON_WIDTH_3_POS_3, 0, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
    COLOR_GUI_CONTROL, "Back", TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doShowChartScreen);

// 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonSetLoadResistor.init(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR_GUI_VALUES, StringLoadResistor,
    TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doSetLoadResistorValue);

// Value settings buttons
    TouchButtonAutorepeatSamplePeriodPlus.init(BUTTON_WIDTH_2_POS_2, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_4,
    COLOR_GUI_CONTROL, "+", TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH | BUTTON_FLAG_TYPE_AUTOREPEAT, 1,
            &doSetSamplePeriod);

    TouchButtonAutorepeatSamplePeriodMinus.init(BUTTON_WIDTH_6_POS_6, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_4,
    COLOR_GUI_CONTROL, "-", TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH | BUTTON_FLAG_TYPE_AUTOREPEAT, -1,
            &doSetSamplePeriod);

    TouchButtonAutorepeatSamplePeriodPlus.setButtonAutorepeatTiming(600, 100, 10, 40);
    TouchButtonAutorepeatSamplePeriodMinus.setButtonAutorepeatTiming(600, 100, 10, 40);

// 3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonSetExternalAttenuatorFactor.init(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4,
    COLOR_GUI_VALUES, StringExternalAttenuatorFactor, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0,
            &doSetExternalAttenuatorFactor);

// 4. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonSetStopValue.init(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR_GUI_VALUES, StringStopVoltage,
    TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doSetStopValue);

    TouchButtonSetChargeVoltage.init(BUTTON_WIDTH_2_POS_2, tPosY, BUTTON_WIDTH_2,
    BUTTON_HEIGHT_4, COLOR_GUI_VALUES, StringStoreChargeVoltage, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0,
            &doSetReadingChargeVoltage);

#pragma GCC diagnostic pop

}

/**
 * adjust X values initially according to sample period
 * sets X grid size, start value, scale factor, label and title
 * set both charts because grid X spacing also changes
 */
void adjustXAxisToSamplePeriod(unsigned int aProbeIndex, int aSamplePeriodSeconds) {
    const char*tXLabelText = "min";
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
    unsigned int tSeconds = DataloggerMeasurementControl[ActualProbe].SamplePeriodSeconds;
    int tMinutes = tSeconds / 60;
    tSeconds %= 60;
    snprintf(sStringBuffer, sizeof sStringBuffer, "Sample period %u:%02u", tMinutes, tSeconds);
    BlueDisplay1.drawText(BUTTON_WIDTH_2_POS_2, BUTTON_HEIGHT_4_LINE_3, sStringBuffer, TEXT_SIZE_11, COLOR_BLACK,
    COLOR_BACKGROUND_DEFAULT);
}

void setBasicMeasurementValues(uint8_t aProbeIndex, unsigned int aRawReading) {
    DataloggerMeasurementControl[aProbeIndex].ActualReading = aRawReading;
    DataloggerMeasurementControl[aProbeIndex].Volt = sADCToVoltFactor * aRawReading;
    if (DataloggerMeasurementControl[aProbeIndex].IsStarted) {
        int tAccuEffectiveReading = aRawReading;
        if (DataloggerMeasurementControl[aProbeIndex].Mode != MODE_DISCHARGING) {
            // assume charging for extern
            tAccuEffectiveReading = DataloggerMeasurementControl[aProbeIndex].ChargeVoltageRaw - aRawReading;
        }
        DataloggerMeasurementControl[aProbeIndex].Milliampere = (sADCToVoltFactor * tAccuEffectiveReading * 1000)
                / DataloggerMeasurementControl[aProbeIndex].OhmLoadResistor;
    }
}

/*
 * read accumulator and calculate voltage and current
 * read 2 Values to "increase" resolution
 */
volatile bool inReadSample = false; // reentrance guard since function is called also by timer
unsigned int readSample(uint8_t aProbeIndex) {
    if (inReadSample == true) {
        // fallback
        return DataloggerMeasurementControl[aProbeIndex].ActualReading;
    }
    inReadSample = true;
    unsigned int tAccuReading = AccuCapacity_ADCRead(aProbeIndex);

// oversampling ;-)
    tAccuReading += AccuCapacity_ADCRead(aProbeIndex);
    inReadSample = false;

    tAccuReading /= 2;
// compensate for external attenuator
    tAccuReading = tAccuReading * DataloggerMeasurementControl[aProbeIndex].ExternalAttenuatorFactor;

    setBasicMeasurementValues(aProbeIndex, tAccuReading);
    return tAccuReading;
}

/**
 * let measurement value start at middle of y axis
 * @param aProbeIndex
 * @param aADCReadingToAutorange Raw reading
 */
void setYAutorange(int16_t aProbeIndex, uint16_t aADCReadingToAutorange) {
    float tNumberOfMinYIncrementsPerChart = CHART_HEIGHT / CHART_GRID_Y_SIZE;
    adjustFloatWithScaleFactor(tNumberOfMinYIncrementsPerChart, AccuCapDisplayControl[ActualProbe].VoltYScaleFactor);
// let start voltage = readingVoltage - half of actual chart range
    float tYLabelStartVoltage = (aADCReadingToAutorange * sADCToVoltFactor)
            - (tNumberOfMinYIncrementsPerChart * (CHART_MIN_Y_INCREMENT_VOLT / 2));
// round to 0.1
    tYLabelStartVoltage = roundf(10 * tYLabelStartVoltage) * CHART_MIN_Y_INCREMENT_VOLT;
    VoltageCharts[ActualProbe]->setYLabelStartValueFloat(tYLabelStartVoltage);
    VoltageCharts[ActualProbe]->drawYAxis(true);
    if (AccuCapDisplayControl[ActualProbe].ChartShowMode == SHOW_MODE_GUI) {
        // show Symbol for vertical swipe
        BlueDisplay1.drawChar(0, BUTTON_HEIGHT_5_LINE_4, '\xE0', TEXT_SIZE_22, COLOR_BLUE, COLOR_BACKGROUND_DEFAULT);
    }
}

/*
 * clears print region of printBasicInfo
 */
void clearBasicInfo(void) {
// values correspond to drawText above
    BlueDisplay1.fillRectRel(BASIC_INFO_X, BASIC_INFO_Y, BlueDisplay1.getDisplayWidth() - BASIC_INFO_X,
            2 * TEXT_SIZE_11_HEIGHT,
            COLOR_BACKGROUND_DEFAULT);
}

//show A/D data on LCD screen
void printMeasurementValues(void) {
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
            snprintf(sStringBuffer, sizeof sStringBuffer, "%4u", DataloggerMeasurementControl[i].ActualReading);
            BlueDisplay1.drawText(tPosX + TEXT_SIZE_22_WIDTH, tPosY, sStringBuffer, TEXT_SIZE_22, ProbeColors[i],
            COLOR_BACKGROUND_DEFAULT);

            tPosY += 4 + TEXT_SIZE_22_HEIGHT;
            snprintf(sStringBuffer, sizeof sStringBuffer, "%4.3fV", DataloggerMeasurementControl[i].Volt);

            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_22, ProbeColors[i], COLOR_BACKGROUND_DEFAULT);

            tPosY += 4 + TEXT_SIZE_22_HEIGHT;
            snprintf(sStringBuffer, sizeof sStringBuffer, "%4.0fmA", DataloggerMeasurementControl[i].Milliampere);
            BlueDisplay1.drawText(tPosX + TEXT_SIZE_22_WIDTH, tPosY, sStringBuffer, TEXT_SIZE_22, ProbeColors[i],
            COLOR_BACKGROUND_DEFAULT);

            tPosY += 4 + TEXT_SIZE_22_HEIGHT;
            // capacity is micro Ampere hour
            snprintf(sStringBuffer, sizeof sStringBuffer, "%5.0fmAh", DataloggerMeasurementControl[i].Capacity / 1000);
            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_22, ProbeColors[i], COLOR_BACKGROUND_DEFAULT);

            tPosY += 4 + TEXT_SIZE_22_HEIGHT;
            if (DataloggerMeasurementControl[ActualProbe].Mode != MODE_EXTERN) {
                snprintf(sStringBuffer, sizeof sStringBuffer, "%4.3f\x81",
                        DataloggerMeasurementControl[i].OhmAccuResistance);
            } else {
                snprintf(sStringBuffer, sizeof sStringBuffer, "%4.3fV",
                        DataloggerMeasurementControl[ActualProbe].Min * sADCToVoltFactor);
            }
            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_22, ProbeColors[i], COLOR_BACKGROUND_DEFAULT);

            tPosY += 4 + TEXT_SIZE_22_HEIGHT;
            tSeconds = DataloggerMeasurementControl[i].SamplePeriodSeconds;
            tMinutes = tSeconds / 60;
            tSeconds %= 60;
            snprintf(sStringBuffer, sizeof sStringBuffer, "Nr.%d - %u:%02u",
                    DataloggerMeasurementControl[i].ProbeNumber, tMinutes, tSeconds);
            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_11, ProbeColors[i], COLOR_BACKGROUND_DEFAULT);

            if (DataloggerMeasurementControl[i].Mode != MODE_EXTERN) {
                tPosY += 2 + TEXT_SIZE_11_HEIGHT;
                if (DataloggerMeasurementControl[i].Mode == MODE_DISCHARGING) {
                    snprintf(sStringBuffer, sizeof sStringBuffer, "stop %4.2fV  ",
                            DataloggerMeasurementControl[i].StopThreshold * sADCToVoltFactor);
                } else if (DataloggerMeasurementControl[i].Mode == MODE_CHARGING) {
                    snprintf(sStringBuffer, sizeof sStringBuffer, "stop %4dmAh",
                            DataloggerMeasurementControl[i].StopMilliampereHour);
                }
                BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_11, ProbeColors[i],
                COLOR_BACKGROUND_DEFAULT);
            }
            tPosX = BUTTON_WIDTH_2_5_POS_2;
        }
    } else {
        if (AccuCapDisplayControl[ActualProbe].ChartShowMode == SHOW_MODE_DATA) {
            /*
             * basic info on 2 lines on top of screen
             */
            if (DataloggerMeasurementControl[ActualProbe].Mode != MODE_EXTERN) {
                snprintf(sStringBuffer, sizeof sStringBuffer, "Probe %d %.0fmAh Rint. %4.2f\x81",
                        DataloggerMeasurementControl[ActualProbe].ProbeNumber,
                        DataloggerMeasurementControl[ActualProbe].Capacity / 1000,
                        DataloggerMeasurementControl[ActualProbe].OhmAccuResistance);
            } else {
                snprintf(sStringBuffer, sizeof sStringBuffer, "Probe %d %.0fmAh extern",
                        DataloggerMeasurementControl[ActualProbe].ProbeNumber,
                        DataloggerMeasurementControl[ActualProbe].Capacity / 1000);
            }
            BlueDisplay1.drawText(BASIC_INFO_X, BASIC_INFO_Y + TEXT_SIZE_11_ASCEND, sStringBuffer, TEXT_SIZE_11,
                    ProbeColors[ActualProbe], COLOR_BACKGROUND_DEFAULT);
            tSeconds = DataloggerMeasurementControl[ActualProbe].SamplePeriodSeconds;
            tMinutes = tSeconds / 60;
            tSeconds %= 60;
            int tCount = (snprintf(sStringBuffer, sizeof sStringBuffer, "%u:%02u %2.1f\x81", tMinutes, tSeconds,
                    DataloggerMeasurementControl[ActualProbe].OhmLoadResistor) + 1) * TEXT_SIZE_11_WIDTH;
            BlueDisplay1.drawText(BASIC_INFO_X, BASIC_INFO_Y + TEXT_SIZE_11_HEIGHT + 2 + TEXT_SIZE_11_ASCEND,
                    sStringBuffer,
                    TEXT_SIZE_11, ProbeColors[ActualProbe], COLOR_BACKGROUND_DEFAULT);
            showRTCTime(BASIC_INFO_X + tCount, BASIC_INFO_Y + TEXT_SIZE_11_HEIGHT + 2 + TEXT_SIZE_11_ASCEND,
                    ProbeColors[ActualProbe], COLOR_BACKGROUND_DEFAULT, false);
        } else {
            /*
             * version of one data set for chart screen
             */
            tPosY = (2 * BUTTON_HEIGHT_5) + BUTTON_DEFAULT_SPACING / 2 + 2 + TEXT_SIZE_22_ASCEND;
            tPosX = BUTTON_WIDTH_5_POS_3 + BUTTON_WIDTH_5;

            snprintf(sStringBuffer, sizeof sStringBuffer, "%5.3fV", DataloggerMeasurementControl[ActualProbe].Volt);
            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_22, ProbeColors[ActualProbe],
            COLOR_BACKGROUND_DEFAULT);

            tPosY += 2 + TEXT_SIZE_22_HEIGHT;
            snprintf(sStringBuffer, sizeof sStringBuffer, "%4.0fmA",
                    DataloggerMeasurementControl[ActualProbe].Milliampere);
            BlueDisplay1.drawText(tPosX + TEXT_SIZE_22_WIDTH, tPosY, sStringBuffer, TEXT_SIZE_22,
                    ProbeColors[ActualProbe],
                    COLOR_BACKGROUND_DEFAULT);

            tPosY += 2 + TEXT_SIZE_22_HEIGHT;
            snprintf(sStringBuffer, sizeof sStringBuffer, "%5.0fmAh",
                    DataloggerMeasurementControl[ActualProbe].Capacity / 1000);
            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_22, ProbeColors[ActualProbe],
            COLOR_BACKGROUND_DEFAULT);

            tPosY += 2 + TEXT_SIZE_22_HEIGHT;
            if (DataloggerMeasurementControl[ActualProbe].Mode != MODE_EXTERN) {
                snprintf(sStringBuffer, sizeof sStringBuffer, "%5.3f\x81",
                        DataloggerMeasurementControl[ActualProbe].OhmAccuResistance);
            } else {
                snprintf(sStringBuffer, sizeof sStringBuffer, "%5.3fV",
                        DataloggerMeasurementControl[ActualProbe].Min * sADCToVoltFactor);
            }
            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_22, ProbeColors[ActualProbe],
            COLOR_BACKGROUND_DEFAULT);

            tPosY += 2 + TEXT_SIZE_22_HEIGHT;
            tSeconds = DataloggerMeasurementControl[ActualProbe].SamplePeriodSeconds;
            tMinutes = tSeconds / 60;
            tSeconds %= 60;
            snprintf(sStringBuffer, sizeof sStringBuffer, "%d %2.1f\x81 %u:%02umin",
                    DataloggerMeasurementControl[ActualProbe].ProbeNumber,
                    DataloggerMeasurementControl[ActualProbe].OhmLoadResistor, tMinutes, tSeconds);
            BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_11, ProbeColors[ActualProbe],
            COLOR_BACKGROUND_DEFAULT);

            if (DataloggerMeasurementControl[ActualProbe].Mode != MODE_EXTERN) {
                tPosY += 1 + TEXT_SIZE_11_HEIGHT;
                if (DataloggerMeasurementControl[ActualProbe].Mode == MODE_DISCHARGING) {
                    snprintf(sStringBuffer, sizeof sStringBuffer, "stop %4.2fV  ",
                            DataloggerMeasurementControl[ActualProbe].StopThreshold * sADCToVoltFactor);
                } else if (DataloggerMeasurementControl[ActualProbe].Mode == MODE_CHARGING) {
                    snprintf(sStringBuffer, sizeof sStringBuffer, "stop %4dmAh",
                            DataloggerMeasurementControl[ActualProbe].StopMilliampereHour);
                }
                BlueDisplay1.drawText(tPosX, tPosY, sStringBuffer, TEXT_SIZE_11, ProbeColors[ActualProbe],
                COLOR_BACKGROUND_DEFAULT);
            }
        }
    }
}

void drawAccuCapacityMainPage(void) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
// display buttons
    for (int i = 0; i < NUMBER_OF_PROBES; ++i) {
        TouchButtonsMain[i].drawButton();
    }
    TouchButtonMainHome.drawButton();
    ActualPage = PAGE_MAIN;
// show data
    printMeasurementValues();
}

void drawAccuCapacitySettingsPage(void) {
    BDButton::deactivateAllButtons();
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    TouchButtonBack.drawButton();
    TouchButtonAutorepeatSamplePeriodPlus.drawButton();
    TouchButtonAutorepeatSamplePeriodMinus.drawButton();
    ActualPage = PAGE_SETTINGS;
    printSamplePeriod();

    snprintf(&StringProbeNumber[STRING_PROBE_NUMBER_NUMBER_INDEX], 4, "%3u",
            DataloggerMeasurementControl[ActualProbe].ProbeNumber);
    TouchButtonSetProbeNumber.setCaptionAndDraw(StringProbeNumber);

    snprintf(&StringLoadResistor[STRING_LOAD_RESISTOR_NUMBER_INDEX], 5, "%4.1f",
            DataloggerMeasurementControl[ActualProbe].OhmLoadResistor);
    TouchButtonSetLoadResistor.setCaptionAndDraw(StringLoadResistor);
    if (DataloggerMeasurementControl[ActualProbe].Mode != MODE_DISCHARGING) {
        setChargeVoltageCaption(ActualProbe);
        TouchButtonSetChargeVoltage.drawButton();
    }

    snprintf(&StringExternalAttenuatorFactor[STRING_EXTERNAL_ATTENUATOR_FACTOR_NUMBER_INDEX], 6, "%5.2f",
            DataloggerMeasurementControl[ActualProbe].ExternalAttenuatorFactor);
    TouchButtonSetExternalAttenuatorFactor.setCaptionAndDraw(StringExternalAttenuatorFactor);

    if (DataloggerMeasurementControl[ActualProbe].Mode != MODE_EXTERN) {
        if (DataloggerMeasurementControl[ActualProbe].Mode == MODE_CHARGING) {
            snprintf(&StringStopNominalCapacity[STRING_STOP_CAPACITY_NUMBER_INDEX], 5, "%4d",
                    DataloggerMeasurementControl[ActualProbe].StopMilliampereHour);
            TouchButtonSetStopValue.setCaptionAndDraw(StringStopNominalCapacity);
        } else {
            snprintf(&StringStopVoltage[STRING_STOP_VOLTAGE_NUMBER_INDEX], 5, "%4.2f",
                    DataloggerMeasurementControl[ActualProbe].StopThreshold * sADCToVoltFactor);
            TouchButtonSetStopValue.setCaptionAndDraw(StringStopVoltage);
        }
    }

}

void setChargeVoltageCaption(int16_t aProbeIndex) {
    snprintf(&StringStoreChargeVoltage[STRING_STORE_CHARGE_VOLTAGE_NUMBER_INDEX], 6, "%5.2f",
            DataloggerMeasurementControl[ActualProbe].ChargeVoltageRaw * sADCToVoltFactor);
    TouchButtonSetChargeVoltage.setCaption(StringStoreChargeVoltage);
}

void setModeCaption(int16_t aProbeIndex) {
    if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_CHARGING) {
        TouchButtonMode.setCaption("Charge");
    } else if (DataloggerMeasurementControl[aProbeIndex].Mode == MODE_DISCHARGING) {
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
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    BDButton::deactivateAllButtons();

// set button colors and values
    for (unsigned int i = 0; i < sizeof(ButtonsChart) / sizeof(ButtonsChart[0]); ++i) {
        unsigned int tValue = ActualProbe;
        if (i == INDEX_OF_NEXT_BUTTON) {
            tValue = 1 ^ ActualProbe;
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
    TouchButtonBack.setValue(ActualProbe);

// set right captions
    if (DataloggerMeasurementControl[ActualProbe].IsStarted) {
        TouchButtonStartStopCapacityMeasurement.setCaption("Stop");
    } else {
        TouchButtonStartStopCapacityMeasurement.setCaption("Start");
    }
    if (DataloggerMeasurementControl[ActualProbe].SampleCount == 0) {
        TouchButtonLoadStore.setCaption("Load");
    } else {
        TouchButtonLoadStore.setCaption("Store");
    }
    TouchButtonChartSwitchData.setCaption(chartStrings[AccuCapDisplayControl[ActualProbe].ActualDataChart]);
    setModeCaption(ActualProbe);

    if (AccuCapDisplayControl[ActualProbe].ActualDataChart == CHART_DATA_RESISTANCE) {
        ResistanceCharts[ActualProbe]->drawAxesAndGrid(); // X labels are not drawn here
        VoltageCharts[ActualProbe]->drawXAxis(false); // Because X labels are taken from Voltage chart
        ResistanceCharts[ActualProbe]->drawYAxisTitle(CHART_Y_LABEL_OFFSET_RESISTANCE);
    } else {
        VoltageCharts[ActualProbe]->drawAxesAndGrid();
        VoltageCharts[ActualProbe]->drawYAxisTitle(CHART_Y_LABEL_OFFSET_VOLTAGE);
        if (AccuCapDisplayControl[ActualProbe].ActualDataChart == CHART_DATA_BOTH) {
            ResistanceCharts[ActualProbe]->drawYAxisTitle(CHART_Y_LABEL_OFFSET_RESISTANCE);
        }
    }

// show eventually buttons and values
    activateOrShowChartGui();
    printMeasurementValues();

    drawData(false);
}

/**
 * draws the actual data chart(s)
 * @param doClearBefore do a clear and refresh cleared gui etc. before
 */
void drawData(bool doClearBefore) {
    if (doClearBefore) {
        VoltageCharts[ActualProbe]->clear();
        VoltageCharts[ActualProbe]->drawGrid();

        // show eventually gui and values which was cleared before
        activateOrShowChartGui();
        printMeasurementValues();

        VoltageCharts[ActualProbe]->drawXAxisTitle();
    }

    if (AccuCapDisplayControl[ActualProbe].ActualDataChart == CHART_DATA_BOTH
            || AccuCapDisplayControl[ActualProbe].ActualDataChart == CHART_DATA_VOLTAGE) {
        if (doClearBefore) {
            VoltageCharts[ActualProbe]->drawYAxisTitle(CHART_Y_LABEL_OFFSET_VOLTAGE);
        }
        // MAX line
        VoltageCharts[ActualProbe]->drawChartData(
                (int16_t *) (DataloggerMeasurementControl[ActualProbe].VoltageDatabuffer
                        + AccuCapDisplayControl[ActualProbe].XStartIndex
                                * VoltageCharts[ActualProbe]->getXGridSpacing()),
                (int16_t *) &DataloggerMeasurementControl[ActualProbe].VoltageDatabuffer[DataloggerMeasurementControl[ActualProbe].SampleCount],
                CHART_MODE_LINE);

    }
    if (AccuCapDisplayControl[ActualProbe].ActualDataChart == CHART_DATA_BOTH
            || AccuCapDisplayControl[ActualProbe].ActualDataChart == CHART_DATA_RESISTANCE) {
        // MIN voltage line for mode external
        if (DataloggerMeasurementControl[ActualProbe].Mode == MODE_EXTERN) {
            VoltageCharts[ActualProbe]->drawChartData(
                    (int16_t *) (DataloggerMeasurementControl[ActualProbe].MinVoltageDatabuffer
                            + AccuCapDisplayControl[ActualProbe].XStartIndex
                                    * VoltageCharts[ActualProbe]->getXGridSpacing()),
                    (int16_t *) &DataloggerMeasurementControl[ActualProbe].MinVoltageDatabuffer[DataloggerMeasurementControl[ActualProbe].SampleCount],
                    CHART_MODE_LINE);
            if (doClearBefore) {
                VoltageCharts[ActualProbe]->drawYAxisTitle(CHART_Y_LABEL_OFFSET_VOLTAGE);
            }
        } else {
            if (doClearBefore) {
                ResistanceCharts[ActualProbe]->drawYAxisTitle(CHART_Y_LABEL_OFFSET_RESISTANCE);
            }
            // Milli-OHM line
            ResistanceCharts[ActualProbe]->drawChartData(
                    (int16_t *) (&DataloggerMeasurementControl[ActualProbe].InternalResistanceDataMilliOhm[0]
                            + AccuCapDisplayControl[ActualProbe].XStartIndex
                                    * VoltageCharts[ActualProbe]->getXGridSpacing()),
                    (int16_t *) &DataloggerMeasurementControl[ActualProbe].InternalResistanceDataMilliOhm[DataloggerMeasurementControl[ActualProbe].SampleCount],
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
    if (AccuCapDisplayControl[ActualProbe].ChartShowMode == SHOW_MODE_GUI) {
        // render all buttons
        for (int i = 0; i < tLoopEnd; ++i) {
            (*ButtonsChart[i]).drawButton();
        }
        // show Symbols for horizontal swipe
        BlueDisplay1.drawText(BUTTON_WIDTH_5_POS_3,
        BUTTON_HEIGHT_4_LINE_4 - TEXT_SIZE_22_HEIGHT - BUTTON_DEFAULT_SPACING, StringDoubleHorizontalArrow,
        TEXT_SIZE_22, COLOR_BLUE, COLOR_BACKGROUND_DEFAULT);
        BlueDisplay1.drawText(BUTTON_WIDTH_5_POS_3, BUTTON_HEIGHT_4_LINE_4, StringDoubleHorizontalArrow, TEXT_SIZE_22,
        COLOR_BLUE, COLOR_BACKGROUND_DEFAULT);
        // show Symbols for vertical swipe
        BlueDisplay1.drawChar(0, BUTTON_HEIGHT_5_LINE_4, '\xE0', TEXT_SIZE_22, COLOR_BLUE, COLOR_BACKGROUND_DEFAULT);
        BlueDisplay1.drawChar(BUTTON_WIDTH_5 + BUTTON_DEFAULT_SPACING, BUTTON_HEIGHT_5_LINE_4, '\xE0', TEXT_SIZE_22,
        COLOR_BLUE, COLOR_BACKGROUND_DEFAULT);

    } else {
        // activate only next button
        (*ButtonsChart[INDEX_OF_NEXT_BUTTON]).activate();
    }
}
