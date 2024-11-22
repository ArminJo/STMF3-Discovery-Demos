/**
 * Chart.hpp
 *
 * Draws charts for LCD screen.
 * Charts have axes and a data area
 * Data can be printed as pixels or line or area
 * Labels and grid are optional
 * Origin is the Parameter PositionX + PositionY and overlaps with the axes
 *
 *  Local display interface used:
 *      - getHeight()
 *      - getDisplayWidth()
 *      - fillRect()
 *      - fillRectRel()
 *      - drawText()
 *      - drawPixel()
 *      - drawLineFastOneX()
 *      - TEXT_SIZE_11_WIDTH
 *      - TEXT_SIZE_11_HEIGHT
 *
 *  Copyright (C) 2012-2024  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay https://github.com/ArminJo/android-blue-display.
 *
 *  BlueDisplay is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 */

#ifndef _CHART_HPP
#define _CHART_HPP

#if !defined(_BLUEDISPLAY_HPP)
#include "BlueDisplay.hpp"
#endif
#if !defined(_GUI_HELPER_HPP)
#include "GUIHelper.hpp" // for getTextDecend() etc.
#endif

//#define LOCAL_TEST
#if defined(LOCAL_TEST)
#include "AVRUtils.h"       // For initStackFreeMeasurement(), printRAMInfo()
#endif

#if defined(SUPPORT_LOCAL_DISPLAY) && defined(DISABLE_REMOTE_DISPLAY)
#define DisplayForChart    LocalDisplay
#else
#define DisplayForChart    BlueDisplay1
#endif

#include "Chart.h"

#if !defined(ARDUINO)
#include "AssertErrorAndMisc.h"
#include <string.h> // for strlen
#include <stdlib.h> // for srand
#endif
//#include <stdio.h> // for snprintf in

/** @addtogroup Graphic_Library
 * @{
 */
/** @addtogroup Chart
 * @{
 */
Chart::Chart() { // @suppress("Class members should be properly initialized")
    mBackgroundColor = CHART_DEFAULT_BACKGROUND_COLOR;
    mAxesColor = CHART_DEFAULT_AXES_COLOR;
    mGridColor = CHART_DEFAULT_GRID_COLOR;
    mLabelColor = CHART_DEFAULT_LABEL_COLOR;
    mFlags = 0;
    mXLabelScaleFactor = CHART_X_AXIS_SCALE_FACTOR_1;
    mXDataFactor = CHART_X_AXIS_SCALE_FACTOR_1;
    mXLabelStartOffset = 0.0;
    mXLabelDistance = 1;
    mXTitleText = NULL;
    mYTitleText = NULL;
}

void Chart::initChartColors(const color16_t aDataColor, const color16_t aAxesColor, const color16_t aGridColor,
        const color16_t aLabelColor, const color16_t aBackgroundColor) {
    mDataColor = aDataColor;
    mAxesColor = aAxesColor;
    mGridColor = aGridColor;
    mLabelColor = aLabelColor;
    mBackgroundColor = aBackgroundColor;
}

void Chart::setDataColor(color16_t aDataColor) {
    mDataColor = aDataColor;
}

void Chart::setBackgroundColor(color16_t aBackgroundColor) {
    mBackgroundColor = aBackgroundColor;
}

/**
 * aPositionX and aPositionY are the 0 coordinates of the grid and part of the axes
 */
uint8_t Chart::initChart(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX, const uint16_t aHeightY,
        const uint8_t aAxesSize, const uint8_t aLabelTextSize, const bool aHasGrid, const uint16_t aGridOrLabelXSpacing,
        const uint16_t aGridOrLabelYSpacing) {
    mPositionX = aPositionX;
    mPositionY = aPositionY;
    mWidthX = aWidthX;
    mHeightY = aHeightY;
    mAxesSize = aAxesSize;
    mLabelTextSize = aLabelTextSize;
    mTitleTextSize = aLabelTextSize;
    mGridXSpacing = aGridOrLabelXSpacing;
    mGridYSpacing = aGridOrLabelYSpacing;

    if (aHasGrid) {
        mFlags |= CHART_HAS_GRID;
    } else {
        mFlags &= ~CHART_HAS_GRID;
    }

    return checkParameterValues();
}

uint8_t Chart::checkParameterValues(void) {
    uint8_t tRetValue = 0;
    // also check for zero :-)
    if (mAxesSize - 1 > CHART_MAX_AXES_SIZE) {
        mAxesSize = CHART_MAX_AXES_SIZE;
        tRetValue = CHART_ERROR_AXES_SIZE;
    }
    uint16_t t2AxesSize = 2 * mAxesSize;
    if (mPositionX < t2AxesSize - 1) {
        mPositionX = t2AxesSize - 1;
        mWidthX = 100;
        tRetValue = CHART_ERROR_POS_X;
    }
    if (mPositionY > DisplayForChart.getDisplayHeight() - t2AxesSize) {
        mPositionY = DisplayForChart.getDisplayHeight() - t2AxesSize;
        tRetValue = CHART_ERROR_POS_Y;
    }
    if (mPositionX + mWidthX > DisplayForChart.getDisplayWidth()) {
        mPositionX = 0;
        mWidthX = 100;
        tRetValue = CHART_ERROR_WIDTH;
    }

    if (mHeightY > mPositionY + 1) {
        mHeightY = mPositionY + 1;
        tRetValue = CHART_ERROR_HEIGHT;
    }

    if (mGridXSpacing > mWidthX) {
        mGridXSpacing = mWidthX / 2;
        tRetValue = CHART_ERROR_GRID_X_SPACING;
    }
    return tRetValue;

}
/**
 * @param aXLabelStartValue
 * @param aXLabelIncrementValue for CHART_X_AXIS_SCALE_FACTOR_1 / identity
 * @param aXLabelScaleFactor
 * @param aXMinStringWidth
 */
void Chart::initXLabelInt(const int aXLabelStartValue, const int aXLabelIncrementValue, const uint8_t aXLabelScaleFactor,
        const uint8_t aXMinStringWidth) {
    mXLabelStartValue.IntValue = aXLabelStartValue;
    mXLabelBaseIncrementValue.IntValue = aXLabelIncrementValue;
    mXLabelScaleFactor = aXLabelScaleFactor;
    mXMinStringWidth = aXMinStringWidth;
    mFlags |= CHART_X_LABEL_INT | CHART_X_LABEL_USED;
}

/*
 * Especially for long time increments
 */
void Chart::initXLabelLong(const long aXLabelStartValue, const long aXLabelIncrementValue, const uint8_t aXLabelScaleFactor,
        const uint8_t aXMinStringWidth) {
    mXLabelStartValue.LongValue = aXLabelStartValue;
    mXLabelBaseIncrementValue.LongValue = aXLabelIncrementValue;
    mXLabelScaleFactor = aXLabelScaleFactor;
    mXMinStringWidth = aXMinStringWidth;
    mFlags |= CHART_X_LABEL_INT | CHART_X_LABEL_USED;
}

/**
 * @param aXLabelStartValue
 * @param aXLabelIncrementValue
 * @param aIntegerScaleFactor
 * @param aXMinStringWidthIncDecimalPoint
 * @param aXNumVarsAfterDecimal
 */
void Chart::initXLabelFloat(const float aXLabelStartValue, const float aXLabelIncrementValue, const uint8_t aXLabelScaleFactor,
        uint8_t aXMinStringWidthIncDecimalPoint, uint8_t aXNumVarsAfterDecimal) {
    mXLabelStartValue.FloatValue = aXLabelStartValue;
    mXLabelBaseIncrementValue.FloatValue = aXLabelIncrementValue;
    mXLabelScaleFactor = aXLabelScaleFactor;
    mXNumVarsAfterDecimal = aXNumVarsAfterDecimal;
    mXMinStringWidth = aXMinStringWidthIncDecimalPoint;
    mFlags &= ~CHART_X_LABEL_INT;
    if (aXMinStringWidthIncDecimalPoint != 0) {
        mFlags |= CHART_X_LABEL_USED;
    }
}

/**
 *
 * @param aYLabelStartValue
 * @param aYLabelIncrementValue increment for one grid line
 * @param aYFactor factor for input to chart value - e.g. (3.0 / 4096) for adc reading of 4096 for 3 (Volt)
 * @param aYMinStringWidth for y axis label
 */
void Chart::initYLabelInt(const int aYLabelStartValue, const int aYLabelIncrementValue, const float aYFactor,
        const uint8_t aYMinStringWidth) {
    mYLabelStartValue.IntValue = aYLabelStartValue;
    mYLabelIncrementValue.IntValue = aYLabelIncrementValue;
    mYMinStringWidth = aYMinStringWidth;
    mFlags |= CHART_Y_LABEL_INT | CHART_Y_LABEL_USED;
    mYDataFactor = aYFactor;
}

/**
 *
 * @param aYLabelStartValue
 * @param aYLabelIncrementValue
 * @param aYFactor factor for input to chart value - e.g. (3.0 / 4096) for adc reading of 4096 for 3 Volt
 * @param aYMinStringWidthIncDecimalPoint for y axis label
 * @param aYNumVarsAfterDecimal for y axis label
 */
void Chart::initYLabelFloat(const float aYLabelStartValue, const float aYLabelIncrementValue, const float aYFactor,
        const uint8_t aYMinStringWidthIncDecimalPoint, const uint8_t aYNumVarsAfterDecimal) {
    mYLabelStartValue.FloatValue = aYLabelStartValue;
    mYLabelIncrementValue.FloatValue = aYLabelIncrementValue;
    mYNumVarsAfterDecimal = aYNumVarsAfterDecimal;
    mYMinStringWidth = aYMinStringWidthIncDecimalPoint;
    mYDataFactor = aYFactor;
    mFlags &= ~CHART_Y_LABEL_INT;
    mFlags |= CHART_Y_LABEL_USED;
}

/**
 * Render the chart on the lcd
 */
void Chart::drawAxesAndGrid(void) {
    drawAxesAndLabels();
    drawGrid();
}

void Chart::drawGrid(void) {
    if (!(mFlags & CHART_HAS_GRID)) {
        return;
    }
    int16_t tXOffset = mGridXSpacing;
    if (mXLabelStartOffset != 0.0) {
        if (mFlags & CHART_X_LABEL_INT) {
            tXOffset -= (mXLabelStartOffset * mGridXSpacing) / reduceIntWithIntegerScaleFactor(mXLabelBaseIncrementValue.IntValue);
        } else {
            tXOffset -= (mXLabelStartOffset * mGridXSpacing)
                    / reduceFloatWithIntegerScaleFactor(mXLabelBaseIncrementValue.FloatValue);
        }
    }
// draw vertical lines, X scale
    do {
        if (tXOffset > 0) {
            DisplayForChart.drawLineRel(mPositionX + tXOffset, mPositionY - (mHeightY - 1), 0, mHeightY - 1, mGridColor);
        }
        tXOffset += mGridXSpacing;
    } while (tXOffset <= (int16_t) mWidthX);

// draw horizontal lines, Y scale
    for (uint16_t tYOffset = mGridYSpacing; tYOffset <= mHeightY; tYOffset += mGridYSpacing) {
        DisplayForChart.drawLineRel(mPositionX + 1, mPositionY - tYOffset, mWidthX - 1, 0, mGridColor);
    }

}

/**
 * render axes
 * renders indicators if labels but no grid are specified
 */
void Chart::drawAxesAndLabels() {
    drawXAxisAndLabels();
    drawYAxisAndLabels();
}

/**
 * draw x title
 * Use label color, because it is the legend for the X axis label
 */
void Chart::drawXAxisTitle() const {
    if (mXTitleText != NULL) {
        /**
         * draw axis title
         */
        uint16_t tTextLenPixel = strlen(mXTitleText) * getTextWidth(mTitleTextSize);
        DisplayForChart.drawText(mPositionX + mWidthX - tTextLenPixel - 1, mPositionY - getTextDecend(mTitleTextSize), mXTitleText,
                mTitleTextSize, mLabelColor, mBackgroundColor);
    }
}

/**
 * Enlarge value if scale factor is compression
 * Reduce value, if scale factor is expansion
 *
 * aIntegerScaleFactor > 1 : expansion by factor aIntegerScaleFactor. I.e. value -> (value / factor)
 * aIntegerScaleFactor == 1 : expansion by 1.5
 * aIntegerScaleFactor == 0 : identity
 * aIntegerScaleFactor == -1 : compression by 1.5
 * aIntegerScaleFactor < -1 : compression by factor -aIntegerScaleFactor -> (value * factor)
 * multiplies value with factor if aIntegerScaleFactor is < 0 (compression) or divide if aIntegerScaleFactor is > 0 (expansion)
 */
int Chart::reduceIntWithIntegerScaleFactor(int aValue) {
    return reduceIntWithIntegerScaleFactor(aValue, mXLabelScaleFactor);
}

long Chart::reduceLongWithIntegerScaleFactor(long aValue) {
    return reduceLongWithIntegerScaleFactor(aValue, mXLabelScaleFactor);
}

float Chart::reduceFloatWithIntegerScaleFactor(float aValue) {
    return reduceFloatWithIntegerScaleFactor(aValue, mXLabelScaleFactor);
}

/**
 * Enlarge value if scale factor is expansion
 * Reduce value, if scale factor is compression
 */
int Chart::enlargeIntWithIntegerScaleFactor(int aValue) {
    return reduceIntWithIntegerScaleFactor(aValue, -mXLabelScaleFactor);
}

float Chart::enlargeFloatWithIntegerScaleFactor(float aValue) {
    return reduceFloatWithIntegerScaleFactor(aValue, -mXLabelScaleFactor);
}

/**
 * Draw X line with indicators and labels
 * Render indicators if labels enabled, but no grid is specified
 * Label increment value is adjusted with scale factor
 */
void Chart::drawXAxisAndLabels() {

    char tLabelStringBuffer[32];

    /*
     * Draw X axis line
     */
    DisplayForChart.fillRectRel(mPositionX - (mAxesSize - 1), mPositionY, mWidthX + (mAxesSize - 1), mAxesSize, mAxesColor);

    if (mFlags & CHART_X_LABEL_USED) {
        /*
         * Draw indicator and label numbers
         */
        if (!(mFlags & CHART_HAS_GRID)) {
            /*
             * Draw indicators with the same size the axis has
             */
            for (uint16_t tGridOffset = 0; tGridOffset <= mWidthX; tGridOffset += mGridXSpacing) {
                DisplayForChart.fillRectRel(mPositionX + tGridOffset, mPositionY + mAxesSize, 1, mAxesSize, mGridColor);
            }
        }

        /*
         * Now draw label numbers
         */
        uint16_t tNumberYTop = mPositionY + 2 * mAxesSize;
#if !defined(ARDUINO)
        assertParamMessage((tNumberYTop <= (DisplayForChart.getDisplayHeight() - getTextDecend(mLabelTextSize))), tNumberYTop,
                "no space for x labels");
#endif

        // first offset is negative
        int16_t tOffset = 1 - ((getTextWidth(mLabelTextSize) * mXMinStringWidth) / 2);
        // clear label space before
        DisplayForChart.fillRect(mPositionX + tOffset, tNumberYTop, mPositionX + mWidthX - (2 * tOffset),
                tNumberYTop + getTextHeight(mLabelTextSize), mBackgroundColor);

        // initialize both variables to avoid compiler warnings
        int tValue = mXLabelStartValue.IntValue;
        float tValueFloat = mXLabelStartValue.FloatValue;

        /*
         * Compute effective label distance
         * effective distance can be greater than 1 only if distance is > 1 and we have a integer expansion of scale
         */
        uint8_t tEffectiveXLabelDistance = 1;
        if (mXLabelDistance > 1
                && (mXLabelScaleFactor == CHART_X_AXIS_SCALE_FACTOR_1 || mXLabelScaleFactor >= CHART_X_AXIS_SCALE_FACTOR_EXPANSION_2)) {
            tEffectiveXLabelDistance = enlargeIntWithIntegerScaleFactor(mXLabelDistance);
        }

        int tIncrementValue = reduceIntWithIntegerScaleFactor(mXLabelBaseIncrementValue.IntValue) * tEffectiveXLabelDistance;
        float tIncrementValueFloat = reduceFloatWithIntegerScaleFactor(mXLabelBaseIncrementValue.FloatValue)
                * tEffectiveXLabelDistance;
        int16_t tOffsetOK = tOffset;
        if (mXLabelStartOffset != 0.0) {
            if (mFlags & CHART_X_LABEL_INT) {
                tOffset -= (mXLabelStartOffset * mGridXSpacing)
                        / reduceIntWithIntegerScaleFactor(mXLabelBaseIncrementValue.IntValue);
            } else {
                tOffset -= (mXLabelStartOffset * mGridXSpacing)
                        / reduceFloatWithIntegerScaleFactor(mXLabelBaseIncrementValue.FloatValue);
            }
        }
        /*
         * loop for drawing labels at X axis
         */
        do {
            if (mFlags & CHART_X_LABEL_INT) {
                snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%d", tValue);
                tValue += tIncrementValue;
            } else {
#if defined(__AVR__)
                dtostrf(tValueFloat, mXMinStringWidth, mXNumVarsAfterDecimal, tLabelStringBuffer);
#else
                    snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%*.*f", mXMinStringWidth, mXNumVarsAfterDecimal,
                        tValueFloat);
#endif
                tValueFloat += tIncrementValueFloat;
            }
            if (tOffset >= tOffsetOK) {
                DisplayForChart.drawText(mPositionX + tOffset, tNumberYTop + getTextAscend(mLabelTextSize), tLabelStringBuffer,
                        mLabelTextSize, mLabelColor, mBackgroundColor);
            }
            tOffset += mGridXSpacing * tEffectiveXLabelDistance; // skip labels

        } while (tOffset <= (int16_t) mWidthX);
    }
}

/**
 * draw x line with time
 * mXLabelBaseIncrementValue is multiplied by SECS_PER_MIN before added to timestamp, allows up to 22 days increment with 16 bit int
 * mXMinStringWidth is number of increments for one day
 */
void Chart::drawXAxisDateLabel(time_t aStartTimestamp, drawXAxisTimeDateSettingsStruct *aDrawXAxisTimeDateSettings) {

    char tLabelStringBuffer[12];

    /*
     * Draw X axis line
     */
    BlueDisplay1.fillRectRel(mPositionX - (mAxesSize - 1), mPositionY, mWidthX + (mAxesSize - 1), mAxesSize, mAxesColor);

    /*
     * Draw indicator and label numbers
     */
    if (!(mFlags & CHART_HAS_GRID)) {
        /*
         * Draw indicators with the same size the axis has
         */
        for (uint16_t tGridOffset = 0; tGridOffset <= mWidthX; tGridOffset += mGridXSpacing) {
            BlueDisplay1.fillRectRel(mPositionX + tGridOffset, mPositionY + mAxesSize, 1, mAxesSize, mGridColor);
        }
    }

    /*
     * Now draw date labels <day>.<month>
     */
    uint16_t tNumberYTop = mPositionY + 2 * mAxesSize;
#if !defined(ARDUINO)
        assertParamMessage((tNumberYTop <= (DisplayForChart.getDisplayHeight() - getTextDecend(mLabelTextSize))), tNumberYTop,
                "no space for x labels");
#endif

    // initialize both variables to avoid compiler warnings
    time_t tTimeStampForLabel = aStartTimestamp;
    int16_t tOffset;
    // first offset is negative
    if (aDrawXAxisTimeDateSettings->secondTokenFunction(tTimeStampForLabel) < 10) {
        tOffset = 1 - ((getTextWidth(mLabelTextSize) * 4) / 2);
    } else {
        tOffset = 1 - ((getTextWidth(mLabelTextSize) * 5) / 2);
    }

    // clear label space before
    BlueDisplay1.fillRect(mPositionX + tOffset, tNumberYTop, mPositionX + mWidthX - (2 * tOffset),
            tNumberYTop + getTextHeight(mLabelTextSize), mBackgroundColor);

    /*
     * Compute effective label distance
     * effective distance can be greater than 1 only if distance is > 1 and we have a integer expansion of scale
     */
    uint8_t tEffectiveXLabelDistance = 1;
    if (mXLabelDistance > 1
            && (mXLabelScaleFactor == CHART_X_AXIS_SCALE_FACTOR_1 || mXLabelScaleFactor >= CHART_X_AXIS_SCALE_FACTOR_EXPANSION_2)) {
        tEffectiveXLabelDistance = enlargeIntWithIntegerScaleFactor(mXLabelDistance);
    }
    /*
     * The base increment value is specified for one grid, so adjust it with mXLabelScaleFactor
     * and multiply it with mXLabelDistance factor, because we display only every mXLabelDistance label
     */
    time_t tIncrementValue = reduceLongWithIntegerScaleFactor(mXLabelBaseIncrementValue.UnixTimestamp) * tEffectiveXLabelDistance;
    int16_t tOffsetOK = tOffset;
    if (mXLabelStartOffset != 0.0) {
        tOffset -= (mXLabelStartOffset * mGridXSpacing) / reduceIntWithIntegerScaleFactor(mXLabelBaseIncrementValue.IntValue);
    }

    /*
     * loop for drawing date label <day>.<month>
     */
    do {
        if (tOffset >= tOffsetOK) {
            snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%d.%d",
                    aDrawXAxisTimeDateSettings->firstTokenFunction(tTimeStampForLabel),
                    aDrawXAxisTimeDateSettings->secondTokenFunction(tTimeStampForLabel));
            BlueDisplay1.drawText(mPositionX + tOffset, tNumberYTop + getTextAscend(mLabelTextSize), tLabelStringBuffer,
                    mLabelTextSize, mLabelColor, mBackgroundColor);
        }
        // set values for next loop
        tTimeStampForLabel += tIncrementValue;
        tOffset += mGridXSpacing * tEffectiveXLabelDistance; // skip labels
    } while (tOffset <= (int16_t) mWidthX);
}

/**
 * Set x label start to index.th value - start not with first but with startIndex label
 * Used for horizontal scrolling
 */
void Chart::setXLabelIntStartValueByIndex(const int aNewXStartIndex, const bool doRedrawXAxis) {
    mXLabelStartValue.IntValue = mXLabelBaseIncrementValue.IntValue * aNewXStartIndex;
    if (doRedrawXAxis) {
        drawXAxisAndLabels();
    }
}

/**
 * If aDoIncrement = true increment XLabelStartValue , else decrement
 * redraw Axis
 * @retval true if X value was not clipped
 */
bool Chart::stepXLabelStartValueInt(const bool aDoIncrement, const int aMinValue, const int aMaxValue) {
    bool tRetval = true;
    if (aDoIncrement) {
        mXLabelStartValue.IntValue += mXLabelBaseIncrementValue.IntValue;
        if (mXLabelStartValue.IntValue > aMaxValue) {
            mXLabelStartValue.IntValue = aMaxValue;
            tRetval = false;
        }
    } else {
        mXLabelStartValue.IntValue -= mXLabelBaseIncrementValue.IntValue;
        if (mXLabelStartValue.IntValue < aMinValue) {
            mXLabelStartValue.IntValue = aMinValue;
            tRetval = false;
        }
    }
    drawXAxisAndLabels();
    return tRetval;
}

/**
 * Increments or decrements the start value by one increment value (one grid line)
 * and redraws Axis
 * does not decrement below 0
 */
float Chart::stepXLabelStartValueFloat(const bool aDoIncrement) {
    if (aDoIncrement) {
        mXLabelStartValue.FloatValue += mXLabelBaseIncrementValue.FloatValue;
    } else {
        mXLabelStartValue.FloatValue -= mXLabelBaseIncrementValue.FloatValue;
    }
    if (mXLabelStartValue.FloatValue < 0) {
        mXLabelStartValue.FloatValue = 0;
    }
    drawXAxisAndLabels();
    return mXLabelStartValue.FloatValue;
}

/**
 * draw y title starting at the Y axis
 * Use data color, because it is the legend for the data
 * @param aYOffset the offset in pixel below the top of the Y line
 *
 */
void Chart::drawYAxisTitle(const int aYOffset) const {
    if (mYTitleText != NULL) {
        /**
         * draw axis title - use data color
         */
        DisplayForChart.drawText(mPositionX + mAxesSize + 1, mPositionY - mHeightY + aYOffset + getTextAscend(mTitleTextSize),
                mYTitleText, mTitleTextSize, mDataColor, mBackgroundColor);
    }
}

/**
 * draw y line with indicators and labels
 * renders indicators if labels but no grid are specified
 */
void Chart::drawYAxisAndLabels() {

    char tLabelStringBuffer[32];
    uint8_t tTextHeight = getTextHeight(mLabelTextSize);

    /*
     * Draw Y axis line
     */
    DisplayForChart.fillRectRel(mPositionX - (mAxesSize - 1), mPositionY - (mHeightY - 1), mAxesSize, (mHeightY - 1), mAxesColor);

    if (mFlags & CHART_Y_LABEL_USED) {
        /*
         * Draw indicator and label numbers
         */
        uint16_t tOffset;
        if (!(mFlags & CHART_HAS_GRID)) {
            /*
             * Draw indicators with the same size the axis has
             */
            for (tOffset = 0; tOffset <= mHeightY; tOffset += mGridYSpacing) {
                DisplayForChart.fillRectRel(mPositionX - (2 * mAxesSize) + 1, mPositionY - tOffset, mAxesSize, 1, mGridColor);
            }
        }

        /*
         * Draw labels (numbers)
         */
        int16_t tNumberXLeft = mPositionX - 2 * mAxesSize - 1 - (mYMinStringWidth * getTextWidth(mLabelTextSize));
#if !defined(ARDUINO)
        assertParamMessage((tNumberXLeft >= 0), tNumberXLeft, "no space for y labels");
#endif

        // first offset is half of character height
        tOffset = tTextHeight / 2;
        // clear label space before
        DisplayForChart.fillRect(tNumberXLeft, mPositionY - mHeightY + 1, mPositionX - mAxesSize,
                mPositionY - tOffset + tTextHeight + 1, mBackgroundColor);

        // convert to string
        // initialize both variables to avoid compiler warnings
        long tValue = mYLabelStartValue.IntValue;
        float tValueFloat = mYLabelStartValue.FloatValue;
        /*
         * draw loop
         */
        do {
            if (mFlags & CHART_Y_LABEL_INT) {
#if defined(__AVR__)
                snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%ld", tValue);
#else
                snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%*ld", mYMinStringWidth, tValue);
#endif
                tValue += mYLabelIncrementValue.IntValue;
            } else {
#if defined(__AVR__)
                dtostrf(tValueFloat, mXMinStringWidth, mXNumVarsAfterDecimal, tLabelStringBuffer);

#else
                snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%*.*f", mYMinStringWidth, mYNumVarsAfterDecimal,
                        tValueFloat);
#endif
                tValueFloat += mYLabelIncrementValue.FloatValue;
            }
            DisplayForChart.drawText(tNumberXLeft, mPositionY - tOffset + getTextAscend(mLabelTextSize), tLabelStringBuffer,
                    mLabelTextSize, mLabelColor, mBackgroundColor);
            tOffset += mGridYSpacing;
        } while (tOffset <= (mHeightY + tTextHeight / 2));
    }
}

bool Chart::stepYLabelStartValueInt(const bool aDoIncrement, const int aMinValue, const int aMaxValue) {
    bool tRetval = true;
    if (aDoIncrement) {
        mYLabelStartValue.IntValue += mYLabelIncrementValue.IntValue;
        if (mYLabelStartValue.IntValue > aMaxValue) {
            mYLabelStartValue.IntValue = aMaxValue;
            tRetval = false;
        }
    } else {
        mYLabelStartValue.IntValue -= mYLabelIncrementValue.IntValue;
        if (mYLabelStartValue.IntValue < aMinValue) {
            mYLabelStartValue.IntValue = aMinValue;
            tRetval = false;
        }
    }
    drawYAxisAndLabels();
    return tRetval;
}

/**
 * increments or decrements the start value by value (one grid line)
 * and redraws Axis
 * does not decrement below 0
 */
float Chart::stepYLabelStartValueFloat(const int aSteps) {
    mYLabelStartValue.FloatValue += mYLabelIncrementValue.FloatValue * aSteps;
    if (mYLabelStartValue.FloatValue < 0) {
        mYLabelStartValue.FloatValue = 0;
    }
    drawYAxisAndLabels();
    return mYLabelStartValue.FloatValue;
}

/**
 * Clears chart area and redraws axes lines
 */
void Chart::clear(void) {
    // (mHeightY - 1) and mHeightY as parameter aHeight left some spurious pixel on my pixel 8
    DisplayForChart.fillRectRel(mPositionX + 1, mPositionY - (mHeightY - 1), mWidthX, mHeightY - 1, mBackgroundColor);
    // draw X line
    DisplayForChart.fillRectRel(mPositionX - (mAxesSize - 1), mPositionY, mWidthX + (mAxesSize - 1), mAxesSize, mAxesColor);
    //draw y line
    DisplayForChart.fillRectRel(mPositionX - (mAxesSize - 1), mPositionY - (mHeightY - 1), mAxesSize, mHeightY - 1, mAxesColor);
}

/**
 * Draws a chart - If mYDataFactor is 1, then pixel position matches y scale.
 * mYDataFactor Factor for uint16_t values to chart value (mYFactor) is used to compute display values
 * e.g. (3.0 / 4096) for ADC reading of 4096 for 3 (Volt)
 * @param aDataPointer pointer to raw data array
 * @param aDataEndPointer pointer to first element after data
 * @param aMode CHART_MODE_PIXEL, CHART_MODE_LINE or CHART_MODE_AREA
 * @return false if clipping occurs
 */
void Chart::drawChartDataFloat(float *aDataPointer, const uint16_t aLengthOfValidData, const uint8_t aMode) {

    float tInputValue;

// used only in line mode
    int tLastValue = 0;

    // Factor for Float -> Display value
    float tYDisplayFactor = 1;
    float tYOffset = 0;

    if (mFlags & CHART_Y_LABEL_INT) {
        // mGridYSpacing / mYLabelIncrementValue.IntValue is factor float -> pixel e.g. 40 pixel for 200 value
        tYDisplayFactor = (mYDataFactor * mGridYSpacing) / mYLabelIncrementValue.IntValue;
        tYOffset = mYLabelStartValue.IntValue / mYDataFactor;
    } else {
        tYDisplayFactor = (mYDataFactor * mGridYSpacing) / mYLabelIncrementValue.FloatValue;
        tYOffset = mYLabelStartValue.FloatValue / mYDataFactor;
    }

    uint16_t tXpos = mPositionX;
    bool tFirstValue = true;

    const float *tDataEndPointer = aDataPointer + aLengthOfValidData;
    int tXScaleCounter = mXDataFactor;
    if (mXDataFactor < CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
        tXScaleCounter = -mXDataFactor;
    }

    for (int i = mWidthX; i > 0; i--) {
        /*
         *  get data and perform X scaling
         */
        if (mXDataFactor == CHART_X_AXIS_SCALE_FACTOR_1) { // == 0
            tInputValue = *aDataPointer++;
        } else if (mXDataFactor == CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) { // == -1
            // compress by factor 1.5 - every second value is the average of the next two values
            tInputValue = *aDataPointer++;
            tXScaleCounter--; // starts with 1
            if (tXScaleCounter < 0) {
                // get average of actual and next value
                tInputValue += *aDataPointer++;
                tInputValue /= 2;
                tXScaleCounter = 1;
            }
        } else if (mXDataFactor <= CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
            // compress - get average of multiple values
            tInputValue = 0;
            for (int j = 0; j < tXScaleCounter; ++j) {
                tInputValue += *aDataPointer++;
            }
            tInputValue /= tXScaleCounter;
        } else if (mXDataFactor == CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) { // ==1
            // expand by factor 1.5 - every second value will be shown 2 times
            tInputValue = *aDataPointer++;
            tXScaleCounter--; // starts with 1
            if (tXScaleCounter < 0) {
                aDataPointer--;
                tXScaleCounter = 2;
            }
        } else {
            // expand - show value several times
            tInputValue = *aDataPointer;
            tXScaleCounter--;
            if (tXScaleCounter == 0) {
                aDataPointer++;
                tXScaleCounter = mXDataFactor;
            }
        }
        // check for data pointer still in data buffer area
        if (aDataPointer >= tDataEndPointer) {
            break;
        }

        uint16_t tDisplayValue = tYDisplayFactor * (tInputValue - tYOffset);
        // clip to bottom line
        if (tYOffset > tInputValue) {
            tDisplayValue = 0;
        }
        // clip to top value
        if (tDisplayValue > mHeightY - 1) {
            tDisplayValue = mHeightY - 1;
        }
        if (aMode == CHART_MODE_AREA) {
            //since we draw a 1 pixel line for value 0
            tDisplayValue += 1;
            DisplayForChart.fillRectRel(tXpos, mPositionY - tDisplayValue, 1, tDisplayValue, mDataColor);
        } else if (aMode == CHART_MODE_PIXEL || tFirstValue) { // even for line draw first value as pixel only
            tFirstValue = false;
            DisplayForChart.drawPixel(tXpos, mPositionY - tDisplayValue, mDataColor);
        } else if (aMode == CHART_MODE_LINE) {
            DisplayForChart.drawLineFastOneX(tXpos - 1, mPositionY - tLastValue, mPositionY - tDisplayValue, mDataColor);
        }
        tLastValue = tDisplayValue;
        tXpos++;
    }
}

/**
 * Draws a chart - If mYDataFactor is 1, then pixel position matches y scale.
 * mYDataFactor Factor for uint16_t values to chart value (mYFactor) is used to compute display values
 * e.g. (3.0 / 4096) for ADC reading of 4096 for 3 (Volt)
 * @param aDataPointer pointer to input data array
 * @param aDataEndPointer pointer to first element after data
 * @param aMode CHART_MODE_PIXEL, CHART_MODE_LINE or CHART_MODE_AREA
 * @return false if clipping occurs
 */
void Chart::drawChartData(int16_t *aDataPointer, const uint16_t aLengthOfValidData, const uint8_t aMode) {

    // Factor for Input -> Display value
    float tYDisplayFactor;
    int tYDisplayOffset;

    /*
     * Compute display factor and offset, so that pixel matches the y scale
     */
    if (mFlags & CHART_Y_LABEL_INT) {
        // mGridYSpacing / mYLabelIncrementValue.IntValue is factor input -> pixel e.g. 40 pixel for 200 value
        tYDisplayFactor = (mYDataFactor * mGridYSpacing) / mYLabelIncrementValue.IntValue;
        tYDisplayOffset = mYLabelStartValue.IntValue / mYDataFactor;
    } else {
        // Label is float
        tYDisplayFactor = (mYDataFactor * mGridYSpacing) / mYLabelIncrementValue.FloatValue;
        tYDisplayOffset = mYLabelStartValue.FloatValue / mYDataFactor;
    }

    uint16_t tXpos = mPositionX;
    bool tFirstValue = true;

    int tXScaleCounter = mXDataFactor;
    if (mXDataFactor < CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
        tXScaleCounter = -mXDataFactor;
    }

    int16_t *tDataEndPointer = aDataPointer + aLengthOfValidData;
    int tDisplayValue;
    int tLastValue = 0; // used only in line mode
    for (int i = mWidthX; i > 0; i--) {
        /*
         *  get data and perform X scaling
         */
        if (mXDataFactor == CHART_X_AXIS_SCALE_FACTOR_1) {
            tDisplayValue = *aDataPointer++;
        } else if (mXDataFactor == CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
            // compress by factor 1.5 - every second value is the average of the next two values
            tDisplayValue = *aDataPointer++;
            tXScaleCounter--; // starts with 1
            if (tXScaleCounter < 0) {
                // get average of actual and next value
                tDisplayValue += *aDataPointer++;
                tDisplayValue /= 2;
                tXScaleCounter = 1;
            }
        } else if (mXDataFactor <= CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
            // compress - get average of multiple values
            tDisplayValue = 0;
            for (int j = 0; j < tXScaleCounter; ++j) {
                tDisplayValue += *aDataPointer++;
            }
            tDisplayValue /= tXScaleCounter;
        } else if (mXDataFactor == CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) {
            // expand by factor 1.5 - every second value will be shown 2 times
            tDisplayValue = *aDataPointer++;
            tXScaleCounter--; // starts with 1
            if (tXScaleCounter < 0) {
                aDataPointer--;
                tXScaleCounter = 2;
            }
        } else {
            // expand - show value several times
            tDisplayValue = *aDataPointer;
            tXScaleCounter--;
            if (tXScaleCounter == 0) {
                aDataPointer++;
                tXScaleCounter = mXDataFactor;
            }
        }
        // check for data pointer still in data buffer
        if (aDataPointer >= tDataEndPointer) {
            break;
        }

        tDisplayValue = tYDisplayFactor * (tDisplayValue - tYDisplayOffset);

        // clip to bottom line
        if (tDisplayValue < 0) {
            tDisplayValue = 0;
        }
        // clip to top value
        if (tDisplayValue > (int) mHeightY - 1) {
            tDisplayValue = mHeightY - 1;
        }
        // draw first value as pixel only
        if (aMode == CHART_MODE_PIXEL || tFirstValue) {
            tFirstValue = false;
            DisplayForChart.drawPixel(tXpos, mPositionY - tDisplayValue, mDataColor);
        } else if (aMode == CHART_MODE_LINE) {
            DisplayForChart.drawLineFastOneX(tXpos - 1, mPositionY - tLastValue, mPositionY - tDisplayValue, mDataColor);
        } else if (aMode == CHART_MODE_AREA) {
            //since we draw a 1 pixel line for value 0
            tDisplayValue += 1;
            DisplayForChart.fillRectRel(tXpos, mPositionY - tDisplayValue, 1, tDisplayValue, mDataColor);
        }
        tLastValue = tDisplayValue;
        tXpos++;
    }

}

/*
 * Draw 8 bit unsigned (compressed) data with offset, i.e. value 0 is on X axis independent of mYLabelStartValue
 */
void Chart::drawChartDataWithOffset(uint8_t *aDataPointer, const uint16_t aLengthOfValidData, const uint8_t aMode) {

    // Factor for Input -> Display value
    float tYDisplayFactor;
    /*
     * Compute display factor and offset, so that pixel matches the y scale
     */
    if (mFlags & CHART_Y_LABEL_INT) {
        // mGridYSpacing / mYLabelIncrementValue.IntValue is factor input -> pixel e.g. 40 pixel for 200 value
        tYDisplayFactor = (mYDataFactor * mGridYSpacing) / mYLabelIncrementValue.IntValue;
    } else {
        // Label is float
        tYDisplayFactor = (mYDataFactor * mGridYSpacing) / mYLabelIncrementValue.FloatValue;
    }

    // If we have scale factor -2 for compression we require 2 times as much data
    // If we have scale factor 2 for expansion we require half as much data
    uint16_t tLengthOfValidData = reduceIntWithIntegerScaleFactor(aLengthOfValidData); //
    if (tLengthOfValidData > mWidthX) {
        tLengthOfValidData = mWidthX; // clip to maximum amount we require for drawing
    }

    // draw to chart index 0 and do not clear last drawn chart line
    // -tYDisplayFactor, because origin is at upper left and therefore Y values are inverse
    BlueDisplay1.drawChartByteBufferScaled(mPositionX, mPositionY, mXDataFactor, -tYDisplayFactor, mAxesSize, aMode, mDataColor,
    COLOR16_NO_DELETE, 0, true, aDataPointer, reduceIntWithIntegerScaleFactor(aLengthOfValidData));

#if defined(SUPPORT_LOCAL_DISPLAY)
    uint16_t tXpos = mPositionX;
    bool tFirstValue = true;

    int tXScaleCounter = mXDataFactor;
    if (mXDataFactor < -1) {
        tXScaleCounter = -mXDataFactor;
    }

    const uint8_t *tDataEndPointer = aDataPointer + aLengthOfValidData;
    int tDisplayValue;
    int tLastValue = 0; // used only in line mode
    for (int i = mWidthX; i > 0; i--) {
        /*
         *  get data and perform X scaling
         */
        if (mXDataFactor == 0) {
            tDisplayValue = *aDataPointer++;
        } else if (mXDataFactor == -1) {
            // compress by factor 1.5 - every second value is the average of the next two values
            tDisplayValue = *aDataPointer++;
            tXScaleCounter--; // starts with 1
            if (tXScaleCounter < 0) {
                // get average of actual and next value
                tDisplayValue += *aDataPointer++;
                tDisplayValue /= 2;
                tXScaleCounter = 1;
            }
        } else if (mXDataFactor <= -1) {
            // compress - get average of multiple values
            tDisplayValue = 0;
            for (int j = 0; j < tXScaleCounter; ++j) {
                tDisplayValue += *aDataPointer++;
            }
            tDisplayValue /= tXScaleCounter;
        } else if (mXDataFactor == 1) {
            // expand by factor 1.5 - every second value will be shown 2 times
            tDisplayValue = *aDataPointer++;
            tXScaleCounter--; // starts with 1
            if (tXScaleCounter < 0) {
                aDataPointer--;
                tXScaleCounter = 2;
            }
        } else {
            // expand - show value several times
            tDisplayValue = *aDataPointer;
            tXScaleCounter--;
            if (tXScaleCounter == 0) {
                aDataPointer++;
                tXScaleCounter = mXDataFactor;
            }
        }
        // check for data pointer still in data buffer
        if (aDataPointer >= tDataEndPointer) {
            break;
        }

        tDisplayValue = tYDisplayFactor * tDisplayValue;

        // clip to top value
        if (tDisplayValue > (int) mHeightY - 1) {
            tDisplayValue = mHeightY - 1;
        }
        // draw first value as pixel only
        if (aMode == CHART_MODE_PIXEL || tFirstValue) {
            tFirstValue = false;
            LocalDisplay.drawPixel(tXpos, mPositionY - tDisplayValue, mDataColor);
        } else if (aMode == CHART_MODE_LINE) {
            LocalDisplay.drawLineFastOneX(tXpos - 1, mPositionY - tLastValue, mPositionY - tDisplayValue, mDataColor);
        } else if (aMode == CHART_MODE_AREA) {
            //since we draw a 1 pixel line for value 0
            tDisplayValue += 1;
            LocalDisplay.fillRectRel(tXpos, mPositionY - tDisplayValue, 1, tDisplayValue, mDataColor);
        }
        tLastValue = tDisplayValue;
        tXpos++;
    }
#endif
}

/**
 * Draws a chart of values of the uint8_t data array pointed to by aDataPointer.
 * Do not apply scale values etc.
 * @param aDataPointer
 * @param aDataLength
 * @param aMode
 * @return false if clipping occurs
 */
bool Chart::drawChartDataDirect(const uint8_t *aDataPointer, const uint16_t aLengthOfValidData, const uint8_t aMode) {

    bool tRetValue = true;
    uint8_t tValue;
    uint16_t tDataLength = aLengthOfValidData;
    if (tDataLength > mWidthX) {
        tDataLength = mWidthX;
        tRetValue = false;
    }

// used only in line mode
    uint8_t tLastValue = *aDataPointer;
    if (tLastValue > mHeightY - 1) {
        tLastValue = mHeightY - 1;
        tRetValue = false;
    }

    uint16_t tXpos = mPositionX;

    for (; tDataLength > 0; tDataLength--) {
        tValue = *aDataPointer++;
        if (tValue > mHeightY - 1) {
            tValue = mHeightY - 1;
            tRetValue = false;
        }
        if (aMode == CHART_MODE_PIXEL) {
            tXpos++;
            DisplayForChart.drawPixel(tXpos, mPositionY - tValue, mDataColor);
        } else if (aMode == CHART_MODE_LINE) {
//          Should we use drawChartByteBuffer() instead?
            DisplayForChart.drawLineFastOneX(tXpos, mPositionY - tLastValue, mPositionY - tValue, mDataColor);
//          drawLine(tXpos, mPositionY - tLastValue, tXpos + 1, mPositionY - tValue,
//                  aDataColor);
            tXpos++;
            tLastValue = tValue;
        } else {
            // aMode == CHART_MODE_AREA
            tXpos++;
            //since we draw a 1 pixel line for value 0
            tValue += 1;
            DisplayForChart.fillRectRel(tXpos, mPositionY - tValue, 1, tValue, mDataColor);
        }
    }
    return tRetValue;
}

uint16_t Chart::getHeightY(void) const {
    return mHeightY;
}

uint16_t Chart::getPositionX(void) const {
    return mPositionX;
}

uint16_t Chart::getPositionY(void) const {
    return mPositionY;
}

uint16_t Chart::getWidthX(void) const {
    return mWidthX;
}

void Chart::setHeightY(uint16_t heightY) {
    mHeightY = heightY;
}

void Chart::setPositionX(uint16_t positionX) {
    mPositionX = positionX;
}

void Chart::setPositionY(uint16_t positionY) {
    mPositionY = positionY;
}

void Chart::setWidthX(uint16_t widthX) {
    mWidthX = widthX;
}

void Chart::setXLabelDistance(uint8_t aXLabelDistance) {
    mXLabelDistance = aXLabelDistance;
}

void Chart::setXGridSpacing(uint8_t aXGridSpacing) {
    mGridXSpacing = aXGridSpacing;
}

void Chart::setYGridSpacing(uint8_t aYGridSpacing) {
    mGridYSpacing = aYGridSpacing;
}

void Chart::setXYGridSpacing(uint8_t aXGridSpacing, uint8_t aYGridSpacing) {
    mGridXSpacing = aXGridSpacing;
    mGridYSpacing = aYGridSpacing;
}

uint8_t Chart::getXGridSpacing(void) const {
    return mGridXSpacing;
}

uint8_t Chart::getYGridSpacing(void) const {
    return mGridYSpacing;
}

void Chart::setXLabelAndGridOffset(float aXLabelAndGridOffset) {
    mXLabelStartOffset = aXLabelAndGridOffset;
}

void Chart::setIntegerScaleFactor(int aIntegerScaleFactor) {
    mXLabelScaleFactor = aIntegerScaleFactor;
}

int Chart::getIntegerScaleFactor(void) const {
    return mXLabelScaleFactor;
}

void Chart::setXDataFactor(int aXDataFactor) {
    mXDataFactor = aXDataFactor;
}

int Chart::getXDataFactor(void) const {
    return mXDataFactor;
}
/*
 * Label
 */
void Chart::setXLabelStartValue(int xLabelStartValue) {
    mXLabelStartValue.IntValue = xLabelStartValue;
}

void Chart::setXLabelStartValueFloat(float xLabelStartValueFloat) {
    mXLabelStartValue.FloatValue = xLabelStartValueFloat;
}

void Chart::setYLabelStartValue(int yLabelStartValue) {
    mYLabelStartValue.IntValue = yLabelStartValue;
}

void Chart::setYLabelStartValueFloat(float yLabelStartValueFloat) {
    mYLabelStartValue.FloatValue = yLabelStartValueFloat;
}

void Chart::setYDataFactor(float aYDataFactor) {
    mYDataFactor = aYDataFactor;
}

/**
 * not tested
 * @retval (YStartValue / mYFactor)
 */
uint16_t Chart::getYLabelStartValueRawFromFloat(void) {
    return (mYLabelStartValue.FloatValue / mYDataFactor);
}

/**
 * not tested
 * @retval (YEndValue = YStartValue + (scale * (mHeightY / GridYSpacing))  / mYFactor
 */
uint16_t Chart::getYLabelEndValueRawFromFloat(void) {
    return ((mYLabelStartValue.FloatValue + mYLabelIncrementValue.FloatValue * (mHeightY / mGridYSpacing)) / mYDataFactor);
}

void Chart::setXLabelBaseIncrementValue(int xLabelBaseIncrementValue) {
    mXLabelBaseIncrementValue.IntValue = xLabelBaseIncrementValue;
}

void Chart::setXLabelBaseIncrementValueFloat(float xLabelBaseIncrementValueFloat) {
    mXLabelBaseIncrementValue.FloatValue = xLabelBaseIncrementValueFloat;
}

void Chart::setYLabelBaseIncrementValue(int yLabelBaseIncrementValue) {
    mYLabelIncrementValue.IntValue = yLabelBaseIncrementValue;
}

void Chart::setYLabelBaseIncrementValueFloat(float yLabelBaseIncrementValueFloat) {
    mYLabelIncrementValue.FloatValue = yLabelBaseIncrementValueFloat;
}

int_long_float_time_union Chart::getXLabelStartValue(void) const {
    return mXLabelStartValue;
}

int_float_union Chart::getYLabelStartValue(void) const {
    return mYLabelStartValue;
}

void Chart::disableXLabel(void) {
    mFlags &= ~CHART_X_LABEL_USED;
}

void Chart::disableYLabel(void) {
    mFlags &= ~CHART_Y_LABEL_USED;
}

void Chart::setTitleTextSize(const uint8_t aTitleTextSize) {
    mTitleTextSize = aTitleTextSize;
}

void Chart::setXTitleText(const char *aTitleText) {
    mXTitleText = aTitleText;
}

void Chart::setYTitleText(const char *aTitleText) {
    mYTitleText = aTitleText;
}

void Chart::setXAxisTimeDateSettings(drawXAxisTimeDateSettingsStruct *aDrawXAxisTimeDateSettingsStructToFill,
        int (*aFirstTokenFunction)(time_t t), int (*aSecondTokenFunction)(time_t t), char aTokenSeparatorChar,
        const char *aIntermediateLabelString) {
    aDrawXAxisTimeDateSettingsStructToFill->TokenSeparatorChar = aTokenSeparatorChar;
    aDrawXAxisTimeDateSettingsStructToFill->IntermediateLabelString = aIntermediateLabelString;
    aDrawXAxisTimeDateSettingsStructToFill->firstTokenFunction = aFirstTokenFunction;
    aDrawXAxisTimeDateSettingsStructToFill->secondTokenFunction = aSecondTokenFunction;
}

/**
 * Reduce value, if scale factor is expansion
 * Enlarge value if scale factor is compression
 * aIntegerScaleFactor > 1 : expansion by factor aIntegerScaleFactor. I.e. value -> (value / factor)
 * aIntegerScaleFactor == 1 : expansion by 1.5
 * aIntegerScaleFactor == 0 : identity
 * aIntegerScaleFactor == -1 : compression by 1.5
 * aIntegerScaleFactor < -1 : compression by factor -aIntegerScaleFactor -> (value * factor)
 * multiplies value with factor if aIntegerScaleFactor is < 0 (compression) or divide if aIntegerScaleFactor is > 0 (expansion)
 */
int Chart::reduceIntWithIntegerScaleFactor(int aValue, int aIntegerScaleFactor) {
    if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_1) {
        return aValue;
    }
    int tRetValue = aValue;
    if (aIntegerScaleFactor > CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) {
        tRetValue = aValue / aIntegerScaleFactor;
    } else if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) {
        // value * 2/3
        tRetValue = (aValue * 2) / 3;
    } else if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
        // value * 3/2
        tRetValue = (aValue * 3) / 2;
    } else {
        tRetValue = aValue * -aIntegerScaleFactor;
    }
    return tRetValue;
}

long Chart::reduceLongWithIntegerScaleFactor(long aValue, int aIntegerScaleFactor) {
    if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_1) {
        return aValue;
    }
    int tRetValue = aValue;
    if (aIntegerScaleFactor > CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) {
        tRetValue = aValue / aIntegerScaleFactor;
    } else if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) {
        // value * 2/3
        tRetValue = (aValue * 2) / 3;
    } else if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
        // value * 3/2
        tRetValue = (aValue * 3) / 2;
    } else {
        tRetValue = aValue * -aIntegerScaleFactor;
    }
    return tRetValue;
}

void Chart::getIntegerScaleFactorAsString(char *tStringBuffer, int aIntegerScaleFactor) {
    if (aIntegerScaleFactor >= CHART_X_AXIS_SCALE_FACTOR_1) {
        *tStringBuffer++ = '*';
        aIntegerScaleFactor = -aIntegerScaleFactor; // for adjustIntWithIntegerScaleFactor() down below scaleFactor must be negative
    } else {
        *tStringBuffer++ = '\xF7'; // division
    }
    if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
        *tStringBuffer++ = '1';
        *tStringBuffer++ = '.';
        *tStringBuffer++ = '5';
        *tStringBuffer++ = '\0';
    } else {
        snprintf(tStringBuffer, 5, "%-3d", reduceIntWithIntegerScaleFactor(1, aIntegerScaleFactor));
    }

}

/**
 * multiplies value with aIntegerScaleFactor if aIntegerScaleFactor is < -1 or divide if aIntegerScaleFactor is > 1
 */
float Chart::reduceFloatWithIntegerScaleFactor(float aValue, int aIntegerScaleFactor) {

    if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_1) {
        return aValue;
    }
    float tRetValue = aValue;
    if (aIntegerScaleFactor > CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) {
        tRetValue = aValue / aIntegerScaleFactor;
    } else if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) {
        // value * 2/3
        tRetValue = aValue * 0.666666666;
    } else if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
        // value * 1.5
        tRetValue = aValue * 1.5;
    } else {
        tRetValue = aValue * -aIntegerScaleFactor;
    }
    return tRetValue;
}

/*
 * Show charts features
 */
#define CHART_1_LENGTH 120
#define CHART_2_LENGTH 140
#define CHART_3_LENGTH 180
#if !defined(DISPLAY_HEIGHT)
#define DISPLAY_HEIGHT_IS_DEFINED_LOCALLY
#define DISPLAY_HEIGHT 240
#endif
void showChartDemo(void) {
    Chart ChartExample;

    /*
     * allocate memory for 180 int16_t values
     */
    int16_t *tChartBufferPtr = (int16_t*) malloc(sizeof(int16_t) * CHART_3_LENGTH);
    if (tChartBufferPtr == NULL) {
#if !defined(ARDUINO)
        failParamMessage(sizeof(int16_t) * CHART_3_LENGTH, "malloc failed");
#else
        DisplayForChart.drawText(0, 2 * TEXT_SIZE_11, "malloc of 360 byte buffer failed", TEXT_SIZE_11, COLOR16_RED, COLOR16_WHITE);
#  if defined(LOCAL_TEST)
        printRAMInfo(&Serial); // Stack used is 126 bytes
#  endif

#endif
        return;
    }

    /*
     * 1. Chart: 120 8-bit values, pixel with grid, no labels, height = 90, axes size = 2, no grid
     */
    ChartExample.disableXLabel();
    ChartExample.disableYLabel();
    ChartExample.initChartColors(COLOR16_RED, COLOR16_RED, CHART_DEFAULT_GRID_COLOR, COLOR16_RED, COLOR16_WHITE);
    ChartExample.initChart(5, DISPLAY_HEIGHT - 20, CHART_1_LENGTH, 90, 2, TEXT_SIZE_11, CHART_DISPLAY_GRID, 0, 0);
    ChartExample.setXYGridSpacing(20, 20);
    ChartExample.drawAxesAndGrid();

    char *tRandomByteFillPointer = (char*) tChartBufferPtr;
//generate random data
#if defined(ARDUINO)
    for (unsigned int i = 0; i < CHART_1_LENGTH; i++) {
        *tRandomByteFillPointer++ = 30 + random(31);
    }
#else
    srand(120);
    for (unsigned int i = 0; i < CHART_1_LENGTH; i++) {
        *tRandomByteFillPointer++ = 30 + (rand() >> 27);
    }
#endif
    ChartExample.drawChartDataDirect((uint8_t*) tChartBufferPtr, CHART_1_LENGTH, CHART_MODE_PIXEL);

    delay(1000);
    /*
     * 2. Chart: 140 16-bit values, with grid, with (negative) integer Y labels and X label offset 5 and 2 labels spacing
     */
    // new random data
    int16_t *tRandomShortFillPointer = tChartBufferPtr;
    int16_t tDataValue = -15;
#if defined(ARDUINO)
    for (unsigned int i = 0; i < CHART_2_LENGTH; i++) {
        tDataValue += random(-3, 6);
        *tRandomShortFillPointer++ = tDataValue;
    }
#else
    for (unsigned int i = 0; i < CHART_2_LENGTH; i++) {
        tDataValue += rand() >> 29;
        *tRandomShortFillPointer++ = tDataValue;
    }
#endif

    ChartExample.initXLabelInt(0, 10, CHART_X_AXIS_SCALE_FACTOR_1, 2);
    ChartExample.setXLabelAndGridOffset(5);
    ChartExample.setXLabelDistance(2);
    ChartExample.initYLabelInt(-20, 20, 20 / 15, 3);
    ChartExample.initChart(170, DISPLAY_HEIGHT - 20, CHART_2_LENGTH, 88, 2, TEXT_SIZE_11, CHART_DISPLAY_GRID, 15, 15);
    ChartExample.drawAxesAndGrid();
    ChartExample.initChartColors(COLOR16_RED, COLOR16_BLUE, COLOR16_GREEN, COLOR16_BLACK, COLOR16_WHITE);
    ChartExample.drawChartData(tChartBufferPtr, CHART_2_LENGTH, CHART_MODE_LINE);

    /*
     * 3. Chart: 140 16-bit values, without grid, with float labels, area mode
     */
    // new random data
    tRandomShortFillPointer = tChartBufferPtr;
    tDataValue = 0;
#if defined(ARDUINO)
    for (unsigned int i = 0; i < CHART_3_LENGTH; i++) {
        tDataValue += random(-2, 4);
        *tRandomShortFillPointer++ = tDataValue;
    }
#else
    for (unsigned int i = 0; i < CHART_3_LENGTH; i++) {
        tDataValue += rand() >> 30;
        *tRandomShortFillPointer++ = tDataValue;
    }
#endif

    ChartExample.initXLabelFloat(0, 0.5, CHART_X_AXIS_SCALE_FACTOR_1, 3, 1);
    ChartExample.initYLabelFloat(0, 0.3, 1.3 / 60, 3, 1); // display 1.3 for raw value of 60
    ChartExample.initChart(30, 100, CHART_3_LENGTH, 90, 2, TEXT_SIZE_11, CHART_DISPLAY_NO_GRID, 30, 16);
    ChartExample.drawAxesAndGrid();
    ChartExample.drawChartData(tChartBufferPtr, CHART_3_LENGTH, CHART_MODE_AREA);

    free(tChartBufferPtr);
}

#if defined(DISPLAY_HEIGHT_IS_DEFINED_LOCALLY)
#undef DISPLAY_HEIGHT
#endif

/** @} */
/** @} */
#undef DisplayForChart
#endif // _CHART_HPP
