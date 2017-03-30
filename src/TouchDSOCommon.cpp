/*
 *      Created on: 29.03.2017
 *      Author: Armin Joachimsmeyer
 *      Email: armin.joachimsmeyer@gmail.com
 *      License: GPL v3 (http://www.gnu.org/licenses/gpl.html)
 *
 * For the common parts of AVR and ARM development
 */
#ifdef AVR
#else
#endif

#ifdef AVR
#include <Arduino.h>
#include "SimpleTouchScreenDSO.h"
#else
#include "Pages.h"
#include "TouchDSO.h"

#include "Chart.h" // for adjustIntWithScaleFactor()
#endif

#include "BlueDisplay.h"

BDButton TouchButtonSlope;
char SlopeButtonString[] = "Slope A";

#define MIN_SAMPLES_PER_PERIOD_FOR_RELIABLE_FREQUENCY_VALUE 3

#ifndef AVR
/**
 * Get max and min for display and automatic triggering.
 */
void computeMinMax(void) {
    uint16_t tMax, tMin;

    uint16_t * tDataBufferPointer = DataBufferControl.DataBufferDisplayStart
    + adjustIntWithScaleFactor(DisplayControl.DatabufferPreTriggerDisplaySize, DisplayControl.XScale);
    if (DataBufferControl.DataBufferEndPointer <= tDataBufferPointer) {
        return;
    }
    uint16_t tAcquisitionSize = DataBufferControl.DataBufferEndPointer + 1 - tDataBufferPointer;

    tMax = *tDataBufferPointer;
    if (MeasurementControl.isEffectiveMinMaxMode) {
        tMin = *(tDataBufferPointer + DATABUFFER_MIN_OFFSET);
    } else {
        tMin = *tDataBufferPointer;
    }

    for (int i = 0; i < tAcquisitionSize; ++i) {
        /*
         * get new Min and Max
         */
        uint16_t tValue = *tDataBufferPointer;
        if (tValue > tMax) {
            tMax = tValue;
        }
        if (MeasurementControl.isEffectiveMinMaxMode) {
            uint16_t tValueMin = *(tDataBufferPointer + DATABUFFER_MIN_OFFSET);
            if (tValueMin < tMin) {
                tMin = tValueMin;
            }
        } else {
            if (tValue < tMin) {
                tMin = tValue;
            }
        }

        tDataBufferPointer++;
    }

    MeasurementControl.RawValueMin = tMin;
    MeasurementControl.RawValueMax = tMax;
}
#endif

void computePeriodFrequency(void) {
#ifdef AVR
    /*
     * Scan data buffer (8 Bit display (!inverted!) values) for trigger conditions.
     * Assume that first value is the first valid after a trigger
     * (which does not hold for fast timebases and delayed or external trigger)
     */
    uint8_t *tDataBufferPointer = &DataBufferControl.DataBuffer[0];
    uint8_t tValue;
    int16_t tCount;
    uint16_t tStartPositionForPulsPause = 0;
    uint16_t tFirstEndPositionForPulsPause = 0;
    uint16_t tCountPosition = 0;
    uint16_t i = 0;
    uint8_t tTriggerStatus = TRIGGER_STATUS_START;
    uint8_t tActualCompareValue = getDisplayFromRawValue(MeasurementControl.RawTriggerLevelHysteresis);
    uint8_t tActualCompareValueForFirstPeriod = getDisplayFromRawValue(MeasurementControl.RawTriggerLevel);
//    BlueDisplay1.debug("trgr=", MeasurementControl.RawTriggerLevel);
//    BlueDisplay1.debug("trg=",getDisplayFromRawValue( MeasurementControl.RawTriggerLevel));
//    BlueDisplay1.debug("tcmp=", tActualCompareValue);
//    BlueDisplay1.debug("data5=", DataBufferControl.DataBuffer[5]);

    MeasurementControl.PeriodFirst = 0;
    MeasurementControl.PeriodSecond = 0;

    if (MeasurementControl.TriggerMode >= TRIGGER_MODE_FREE || MeasurementControl.TriggerDelay != TRIGGER_DELAY_NONE) {
        /*
         * For TRIGGER_MODE_FREE, TRIGGER_MODE_EXTERN or delayed trigger first display value is not any trigger,
         * so start with search for begin of period.
         */
        tCount = -1;
    } else {
        // First display value is the first after triggering condition so we have at least one trigger, but still no period.
        tCount = 0;
    }
    for (; i < REMOTE_DISPLAY_WIDTH; ++i) {
#else
        /**
         * Get period and frequency and average for display
         *
         * Use databuffer and only post trigger area!
         * For frequency use only max values!
         */
        uint16_t * tDataBufferPointer = DataBufferControl.DataBufferDisplayStart
        + adjustIntWithScaleFactor(DisplayControl.DatabufferPreTriggerDisplaySize, DisplayControl.XScale);
        if (DataBufferControl.DataBufferEndPointer <= tDataBufferPointer) {
            return;
        }
        uint16_t tAcquisitionSize = DataBufferControl.DataBufferEndPointer + 1 - tDataBufferPointer;

        uint16_t tValue;
        uint32_t tIntegrateValue = 0;
        uint32_t tIntegrateValueForTotalPeriods = 0;
        int tCount = 0;
        uint16_t tStartPositionForPulsPause = 0;
        uint16_t tFirstEndPositionForPulsPause = 0;
        int tCountPosition = 0;
        int tPeriodDelta = 0;
        int tPeriodMin = 1024;
        int tPeriodMax = 0;
        int tTriggerStatus = TRIGGER_STATUS_START;
        uint16_t tActualCompareValue = MeasurementControl.RawTriggerLevelHysteresis;
        uint16_t tActualCompareValueForFirstPeriod = MeasurementControl.RawTriggerLevel;

        bool tReliableValue = true;

        /*
         * Trigger condition and average taken only from entire periods
         * Use only max value for period
         */
        for (int i = 0; i < tAcquisitionSize; ++i) {
#endif
        tValue = *tDataBufferPointer;

        bool tValueGreaterRefForFirstPeriod;
        if (tFirstEndPositionForPulsPause == 00 && tCount == 0) {
            tValueGreaterRefForFirstPeriod = (tValue > tActualCompareValueForFirstPeriod);
#ifdef AVR
            // toggle compare result if TriggerSlopeRising == true since all values are inverted Display values we have to use just the inverted condition
            tValueGreaterRefForFirstPeriod = tValueGreaterRefForFirstPeriod ^ MeasurementControl.TriggerSlopeRising;
#else
            // toggle compare result if TriggerSlopeRising == false
            tValueGreaterRefForFirstPeriod = tValueGreaterRefForFirstPeriod ^ (!MeasurementControl.TriggerSlopeRising);
#endif
        }

        bool tValueGreaterRef = (tValue > tActualCompareValue);
#ifdef AVR
        // toggle compare result if TriggerSlopeRising == true since all values are inverted Display values we have to use just the inverted condition
        tValueGreaterRef = tValueGreaterRef ^ MeasurementControl.TriggerSlopeRising;
#else
        // toggle compare result if TriggerSlopeRising == false
        tValueGreaterRef = tValueGreaterRef ^ (!MeasurementControl.TriggerSlopeRising);
#endif
        /*
         * First value is the first after triggering condition (including delay)
         */
        if (tTriggerStatus == TRIGGER_STATUS_START) {
            // rising slope - wait for value below 1. threshold
            // falling slope - wait for value above 1. threshold
            if (tFirstEndPositionForPulsPause == 00 && tCount == 0) {
                if (!tValueGreaterRefForFirstPeriod) {
                    tFirstEndPositionForPulsPause = i;
                    MeasurementControl.PeriodFirst = getMicrosFromHorizontalDisplayValue(i - tStartPositionForPulsPause, 1);
                }
            }
            if (!tValueGreaterRef) {
                tTriggerStatus = TRIGGER_STATUS_BEFORE_THRESHOLD;
#ifdef AVR
                tActualCompareValue = getDisplayFromRawValue(MeasurementControl.RawTriggerLevel);
#else
                tActualCompareValue = MeasurementControl.RawTriggerLevel;
#endif
            }
        } else {
            // rising slope - wait for value to rise above 2. threshold
            // falling slope - wait for value to go below 2. threshold
            if (tValueGreaterRef) {
#ifdef AVR
                tTriggerStatus = TRIGGER_STATUS_START;
                tActualCompareValue = getDisplayFromRawValue(MeasurementControl.RawTriggerLevelHysteresis);
#else
                if ((tPeriodDelta) < MIN_SAMPLES_PER_PERIOD_FOR_RELIABLE_FREQUENCY_VALUE) {
                    // found new trigger in less than MIN_SAMPLES_PER_PERIOD_FOR_RELIABLE_FREQUENCY_VALUE samples => no reliable value
                    tReliableValue = false;
                } else {
                    // search for next slope
                    tTriggerStatus = TRIGGER_STATUS_START;
                    tActualCompareValue = MeasurementControl.RawTriggerLevelHysteresis;
                    if (tPeriodDelta < tPeriodMin) {
                        tPeriodMin = tPeriodDelta;
                    } else if (tPeriodDelta > tPeriodMax) {
                        tPeriodMax = tPeriodDelta;
                    }
                    tPeriodDelta = 0;
                    // found and search for next slope
                    tIntegrateValueForTotalPeriods = tIntegrateValue;
#endif
                tCount++;
                if (tCount == 0) {
                    // set start position for TRIGGER_MODE_FREE, TRIGGER_MODE_EXTERN or delayed trigger.
                    tStartPositionForPulsPause = i;
                } else if (tCount == 1) {
                    MeasurementControl.PeriodSecond = getMicrosFromHorizontalDisplayValue(i - tFirstEndPositionForPulsPause, 1);
                }
                tCountPosition = i;
#ifndef AVR
            }
#endif
            }
        }
#ifndef AVR
        if (MeasurementControl.isEffectiveMinMaxMode) {
            uint16_t tValueMin = *(tDataBufferPointer + DATABUFFER_MIN_OFFSET);
            tIntegrateValue += (tValue + tValueMin) / 2;

        } else {
            tIntegrateValue += tValue;
        }
        tPeriodDelta++;
#endif
        tDataBufferPointer++;
    } // for
#ifndef AVR
    /*
     * check for plausi of period values
     * allow delta of periods to be at least 1/8 period + 3
     */
    tPeriodDelta = tPeriodMax - tPeriodMin;
    if (((tCountPosition / (8 * tCount)) + 3) < tPeriodDelta) {
        tReliableValue = false;
    }
#endif

    /*
     * compute period and frequency
     */
#ifdef AVR
    if (tCount <= 0) {
#else
        if (tCountPosition <= 0 && tCount <= 0 && !tReliableValue) {
            MeasurementControl.FrequencyHertz = 0;
            MeasurementControl.RawValueAverage = (tIntegrateValue + (tAcquisitionSize / 2)) / tAcquisitionSize;
#endif
        MeasurementControl.PeriodMicros = 0;
    } else {
#ifdef AVR
        tCountPosition -= tStartPositionForPulsPause;
        if (MeasurementControl.TimebaseIndex <= TIMEBASE_INDEX_FAST_MODES + 1) {
            tCountPosition++; // compensate for 1 measurement delay between trigger detection and acquisition
        }
        uint32_t tPeriodMicros = getMicrosFromHorizontalDisplayValue(tCountPosition, tCount);
        MeasurementControl.PeriodMicros = tPeriodMicros;
#else
        MeasurementControl.RawValueAverage = (tIntegrateValueForTotalPeriods + (tCountPosition / 2)) / tCountPosition;

        // compute microseconds per period
        float tPeriodMicros = getMicrosFromHorizontalDisplayValue(tCountPosition, tCount) + 0.005;
#endif
        MeasurementControl.PeriodMicros = tPeriodMicros;
        // frequency
        float tHertz = 1000000.0 / tPeriodMicros;
        MeasurementControl.FrequencyHertz = tHertz + 0.5;

    }
    return;
}

/*
 * draws vertical timing + horizontal reference voltage lines
 */
void drawGridLinesWithHorizLabelsAndTriggerLine() {
    if (DisplayControl.DisplayPage != DISPLAY_PAGE_CHART) {
        return;
    }
// vertical (timing) lines
    for (unsigned int tXPos = TIMING_GRID_WIDTH - 1; tXPos < REMOTE_DISPLAY_WIDTH; tXPos += TIMING_GRID_WIDTH) {
        BlueDisplay1.drawLineRel(tXPos, 0, 0, REMOTE_DISPLAY_HEIGHT, COLOR_GRID_LINES);
    }
#ifdef AVR
    /*
     * drawHorizontalLineLabels
     * adjust the distance between the lines to the actual range which is also determined by the reference voltage
     */
    float tActualVoltage = 0;
    char tStringBuffer[6];
    uint8_t tPrecision = 2 - MeasurementControl.AttenuatorValue;
    uint8_t tLength = 2 + tPrecision;
    if (MeasurementControl.ChannelIsACMode) {
        /*
         * draw from middle of screen to top and "mirror" lines for negative values
         */
        // Start at DISPLAY_HEIGHT/2 shift 8
        for (int32_t tYPosLoop = 0x8000; tYPosLoop > 0; tYPosLoop -= MeasurementControl.HorizontalGridSizeShift8) {
            uint16_t tYPos = tYPosLoop / 0x100;
            // horizontal line
            BlueDisplay1.drawLineRel(0, tYPos, REMOTE_DISPLAY_WIDTH, 0, COLOR_GRID_LINES);
            dtostrf(tActualVoltage, tLength, tPrecision, tStringBuffer);
            // draw label over the line
            BlueDisplay1.drawText(HORIZONTAL_LINE_LABELS_CAPION_X, tYPos + (TEXT_SIZE_11_ASCEND / 2), tStringBuffer, 11,
            COLOR_HOR_REF_LINE_LABEL, COLOR_NO_BACKGROUND);
            if (tYPos != REMOTE_DISPLAY_HEIGHT / 2) {
                // line with negative value
                BlueDisplay1.drawLineRel(0, REMOTE_DISPLAY_HEIGHT - tYPos, REMOTE_DISPLAY_WIDTH, 0, COLOR_GRID_LINES);
                dtostrf(-tActualVoltage, tLength, tPrecision, tStringBuffer);
                // draw label over the line
                BlueDisplay1.drawText(HORIZONTAL_LINE_LABELS_CAPION_X - TEXT_SIZE_11_WIDTH,
                        REMOTE_DISPLAY_HEIGHT - tYPos + (TEXT_SIZE_11_ASCEND / 2), tStringBuffer, 11, COLOR_HOR_REF_LINE_LABEL,
                        COLOR_NO_BACKGROUND);
            }
            tActualVoltage += MeasurementControl.HorizontalGridVoltage;
        }
    } else {
        if (MeasurementControl.OffsetAutomatic) {
            tActualVoltage = MeasurementControl.HorizontalGridVoltage * MeasurementControl.OffsetGridCount;
        }
        // draw first caption over the line
        int8_t tCaptionOffset = 1;
        // Start at (DISPLAY_VALUE_FOR_ZERO) shift 8) + 1/2 shift8 for better rounding
        for (int32_t tYPosLoop = 0xFF80; tYPosLoop > 0; tYPosLoop -= MeasurementControl.HorizontalGridSizeShift8) {
            uint16_t tYPos = tYPosLoop / 0x100;
            // horizontal line
            BlueDisplay1.drawLineRel(0, tYPos, REMOTE_DISPLAY_WIDTH, 0, COLOR_GRID_LINES);
            dtostrf(tActualVoltage, tLength, tPrecision, tStringBuffer);
            // draw label over the line
            BlueDisplay1.drawText(HORIZONTAL_LINE_LABELS_CAPION_X, tYPos - tCaptionOffset, tStringBuffer, 11,
            COLOR_HOR_REF_LINE_LABEL, COLOR_NO_BACKGROUND);
            // draw next caption on the line
            tCaptionOffset = -(TEXT_SIZE_11_ASCEND / 2);
            tActualVoltage += MeasurementControl.HorizontalGridVoltage;
        }
    }

#else
    /*
     * Here we have a fixed layout
     */
// add 0.0001 to avoid display of -0.00
    float tActualVoltage = (ScaleVoltagePerDiv[MeasurementControl.DisplayRangeIndexForPrint] * (MeasurementControl.OffsetGridCount)
            + 0.0001);
    /*
     * check if range or offset changed in order to change label
     */
    bool tLabelChanged = false;
    if (DisplayControl.LastDisplayRangeIndex != MeasurementControl.DisplayRangeIndexForPrint
            || DisplayControl.LastOffsetGridCount != MeasurementControl.OffsetGridCount) {
        DisplayControl.LastDisplayRangeIndex = MeasurementControl.DisplayRangeIndexForPrint;
        DisplayControl.LastOffsetGridCount = MeasurementControl.OffsetGridCount;
        tLabelChanged = true;
    }
    int tCaptionOffset = 1;
    Color_t tLabelColor;
    int tPosX;
    for (int tYPos = DISPLAY_VALUE_FOR_ZERO; tYPos > 0; tYPos -= HORIZONTAL_GRID_HEIGHT) {
        if (tLabelChanged) {
            // clear old label
            int tXpos = REMOTE_DISPLAY_WIDTH - PIXEL_AFTER_LABEL - (5 * TEXT_SIZE_11_WIDTH);
            int tY = tYPos - tCaptionOffset;
            BlueDisplay1.fillRect(tXpos, tY - TEXT_SIZE_11_ASCEND, REMOTE_DISPLAY_WIDTH - PIXEL_AFTER_LABEL + 1,
                    tY + TEXT_SIZE_11_HEIGHT - TEXT_SIZE_11_ASCEND, COLOR_BACKGROUND_DSO);
            // restore last vertical line since label may overlap these line
            BlueDisplay1.drawLineRel(9 * TIMING_GRID_WIDTH - 1, tY, 0, TEXT_SIZE_11_HEIGHT, COLOR_GRID_LINES);
        }
        // draw horizontal line
        BlueDisplay1.drawLineRel(0, tYPos, REMOTE_DISPLAY_WIDTH, 0, COLOR_GRID_LINES);

        int tCount = snprintf(sStringBuffer, sizeof sStringBuffer, "%0.*f",
                RangePrecision[MeasurementControl.DisplayRangeIndexForPrint], tActualVoltage);
        // right align but leave 2 pixel free after label for the last horizontal line
        tPosX = REMOTE_DISPLAY_WIDTH - (tCount * TEXT_SIZE_11_WIDTH) - PIXEL_AFTER_LABEL;
        // draw label over the line - use different color for negative values
        if (tActualVoltage >= 0) {
            tLabelColor = COLOR_HOR_GRID_LINE_LABEL;
        } else {
            tLabelColor = COLOR_HOR_GRID_LINE_LABEL_NEGATIVE;
        }
        BlueDisplay1.drawText(tPosX, tYPos - tCaptionOffset, sStringBuffer, TEXT_SIZE_11, tLabelColor, COLOR_BACKGROUND_DSO);
        tCaptionOffset = -(TEXT_SIZE_11_ASCEND / 2);
        tActualVoltage += ScaleVoltagePerDiv[MeasurementControl.DisplayRangeIndexForPrint];
    }
#endif
    drawTriggerLine();
}

void clearTriggerLine(uint8_t aTriggerLevelDisplayValue) {
// clear old line
    BlueDisplay1.drawLineRel(0, aTriggerLevelDisplayValue, REMOTE_DISPLAY_WIDTH, 0, COLOR_BACKGROUND_DSO);

    if (DisplayControl.showInfoMode != INFO_MODE_NO_INFO) {
        // restore grid at old y position
        for (uint16_t tXPos = TIMING_GRID_WIDTH - 1; tXPos < REMOTE_DISPLAY_WIDTH - 1; tXPos += TIMING_GRID_WIDTH) {
            BlueDisplay1.drawPixel(tXPos, aTriggerLevelDisplayValue, COLOR_GRID_LINES);
        }
    }
#ifndef AVR
    if (!MeasurementControl.isRunning) {
        // in analysis mode restore graph at old y position
        uint8_t* ScreenBufferPointer = &DisplayBuffer[0];
        for (unsigned int i = 0; i < REMOTE_DISPLAY_WIDTH; ++i) {
            int tValueByte = *ScreenBufferPointer++;
            if (tValueByte == aTriggerLevelDisplayValue) {
                // restore old pixel
                BlueDisplay1.drawPixel(i, tValueByte, COLOR_DATA_HOLD);
            }
        }
    }
#endif
}

/**
 * draws trigger line if it is visible - do not draw clipped value e.g. value was higher than display range
 */
void drawTriggerLine(void) {
    uint8_t tValue = DisplayControl.TriggerLevelDisplayValue;
    if (tValue != 0 && MeasurementControl.TriggerMode < TRIGGER_MODE_FREE) {
        BlueDisplay1.drawLineRel(0, tValue, REMOTE_DISPLAY_WIDTH, 0, COLOR_TRIGGER_LINE);
    }
}

/**
 * compute new trigger value and hysteresis
 * If old values are reasonable don't change them to avoid jitter
 */
void computeAutoTrigger(void) {
    if (MeasurementControl.TriggerMode == TRIGGER_MODE_AUTOMATIC) {
        /*
         * Set auto trigger in middle between min and max
         */
        uint16_t tPeakToPeak = MeasurementControl.RawValueMax - MeasurementControl.RawValueMin;
        // middle between min and max
        uint16_t tPeakToPeakHalf = tPeakToPeak / 2;
        uint16_t tNewRawTriggerValue = MeasurementControl.RawValueMin + tPeakToPeakHalf;

        //set effective hysteresis to quarter delta
        int tTriggerHysteresis = tPeakToPeakHalf / 2;

        // keep reasonable value - avoid jitter - abs does not work, it may give negative values
        // int tTriggerDelta = abs(tNewTriggerValue - MeasurementControl.RawTriggerLevel);
        int tTriggerDelta = tNewRawTriggerValue - MeasurementControl.RawTriggerLevel;
        if (tTriggerDelta < 0) {
            tTriggerDelta = -tTriggerDelta;
        }
        int tOldHysteresis3Quarter = (MeasurementControl.RawHysteresis / 4) * 3;
        if (tTriggerDelta > (tTriggerHysteresis / 4) || tTriggerHysteresis <= tOldHysteresis3Quarter) {
            setTriggerLevelAndHysteresis(tNewRawTriggerValue, tTriggerHysteresis);
        }
    }
}

void setTriggerLevelAndHysteresis(int aRawTriggerValue, int aRawTriggerHysteresis) {
    MeasurementControl.RawTriggerLevel = aRawTriggerValue;
    MeasurementControl.RawHysteresis = aRawTriggerHysteresis;
    if (MeasurementControl.TriggerSlopeRising) {
        MeasurementControl.RawTriggerLevelHysteresis = aRawTriggerValue - aRawTriggerHysteresis;
    } else {
        MeasurementControl.RawTriggerLevelHysteresis = aRawTriggerValue + aRawTriggerHysteresis;
    }
}

/************************************************************************
 * Button caption section
 ************************************************************************/

void setSlopeButtonCaption(void) {
    uint8_t tChar;
    if (MeasurementControl.TriggerSlopeRising) {
        tChar = 'A'; // 0xD1; //ascending
    } else {
        tChar = 'D'; // 0xD2; //descending
    }
    SlopeButtonString[SLOPE_STRING_INDEX] = tChar;
    TouchButtonSlope.setCaption(SlopeButtonString, (DisplayControl.DisplayPage == DISPLAY_PAGE_SETTINGS));
}

void setTriggerModeButtonCaption(void) {
    const char * tCaption;
// switch statement code is 12 bytes shorter here
    switch (MeasurementControl.TriggerMode) {
    case TRIGGER_MODE_AUTOMATIC:
        tCaption = TriggerModeButtonStringAuto;
        break;
    case TRIGGER_MODE_MANUAL_TIMEOUT:
        tCaption = TriggerModeButtonStringManualTimeout;
        break;
    case TRIGGER_MODE_MANUAL:
        tCaption = TriggerModeButtonStringManual;
        break;
    case TRIGGER_MODE_FREE:
        tCaption = TriggerModeButtonStringFree;
        break;
    default:
        tCaption = TriggerModeButtonStringExtern;
        break;
    }
#ifdef AVR
    TouchButtonTriggerMode.setCaptionPGM(tCaption, (DisplayControl.DisplayPage == DISPLAY_PAGE_SETTINGS));
#else
    TouchButtonTriggerMode.setCaption(tCaption, (DisplayControl.DisplayPage == DISPLAY_PAGE_SETTINGS));
#endif
    /* This saves >12 byte program space but needs 10 byte RAM
     TouchButtonTriggerMode.setCaptionPGM(TriggerModeButtonStrings[MeasurementControl.TriggerMode],
     (DisplayControl.DisplayPage == DISPLAY_PAGE_SETTINGS));
     */
}

void setAutoRangeModeAndButtonCaption(bool aNewAutoRangeMode) {
    MeasurementControl.RangeAutomatic = aNewAutoRangeMode;
    const char * tCaption;
    if (MeasurementControl.RangeAutomatic) {
        tCaption = AutoRangeButtonStringAuto;
    } else {
        tCaption = AutoRangeButtonStringManual;
    }
#ifdef AVR
    TouchButtonAutoRangeOnOff.setCaptionPGM(tCaption, (DisplayControl.DisplayPage == DISPLAY_PAGE_SETTINGS));
#else
    TouchButtonAutoRangeOnOff.setCaption(tCaption, (DisplayControl.DisplayPage == DISPLAY_PAGE_SETTINGS));
#endif
}

/************************************************************************
 * Button handler section
 ************************************************************************/

/*
 * toggle between ascending and descending trigger slope
 */
void doTriggerSlope(BDButton * aTheTouchedButton, int16_t aValue) {
    MeasurementControl.TriggerSlopeRising = (!MeasurementControl.TriggerSlopeRising);
    setTriggerLevelAndHysteresis(MeasurementControl.RawTriggerLevel, MeasurementControl.RawHysteresis);
    setSlopeButtonCaption();
}

/*
 * switch between automatic, manual, free and external trigger mode
 */
void doTriggerMode(BDButton * aTheTouchedButton, int16_t aValue) {
    uint8_t tNewMode = MeasurementControl.TriggerMode + 1;
    if (tNewMode > TRIGGER_MODE_EXTERN) {
        tNewMode = TRIGGER_MODE_AUTOMATIC;
        MeasurementControl.TriggerMode = tNewMode;
#ifdef AVR
        cli();
        if (EIMSK != 0) {
            // Release waiting for external trigger
            INT0_vect();
        }
        sei();
#endif
    }
    MeasurementControl.TriggerMode = tNewMode;
    setTriggerModeButtonCaption();
}

void doRangeMode(BDButton * aTheTouchedButton, int16_t aValue) {
    setAutoRangeModeAndButtonCaption(!MeasurementControl.RangeAutomatic);
}

/*
 *  Toggle history mode
 */
void doChartHistory(BDButton * aTheTouchedButton, int16_t aValue) {
    DisplayControl.showHistory = aValue;
    if (DisplayControl.DisplayPage == DISPLAY_PAGE_SETTINGS) {
        aTheTouchedButton->drawButton();
    }

    if (DisplayControl.showHistory) {
        DisplayControl.EraseColor = COLOR_DATA_HISTORY;
    } else {
        DisplayControl.EraseColor = COLOR_BACKGROUND_DSO;
        if (MeasurementControl.isRunning) {
            // clear history on screen
            redrawDisplay();
        }
    }
}

uint32_t getMicrosFromHorizontalDisplayValue(uint16_t aDisplayValueHorizontal, uint8_t aNumberOfPeriods) {
#ifdef AVR
    uint32_t tMicros = aDisplayValueHorizontal * pgm_read_float(&TimebaseExactDivValuesMicros[MeasurementControl.TimebaseIndex]);
#else
    uint32_t tMicros = aDisplayValueHorizontal * getDataBufferTimebaseExactValueMicros(MeasurementControl.TimebaseEffectiveIndex);
#endif
    return (tMicros / (aNumberOfPeriods * TIMING_GRID_WIDTH));
}
