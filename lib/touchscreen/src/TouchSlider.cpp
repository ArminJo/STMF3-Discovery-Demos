/**
 * @file TouchSlider.cpp
 *
 * Slider with value as unsigned word value
 *
 * @date 30.01.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 *
 *  LCD interface used:
 *      - LocalDisplay.fillRectRel()
 *      - LocalDisplay.drawText()
 *
 *  Ram usage:
 *      15 byte
 *      Size of one slider is 46 bytes on Arduino
 *
 *  Code size:
 *      2,8 kByte
 *
 */

#include "BlueDisplay.h"

#include "TouchSlider.h"

#ifndef AVR
#include "AssertErrorAndMisc.h"
#include "myStrings.h"
#else
#define failParamMessage(wrongParam,message) void()
#endif

#ifdef REMOTE_DISPLAY_SUPPORTED
#include "BlueDisplayProtocol.h"
#include "BlueSerial.h"
#endif

#include <stdio.h> // for printf
#include <string.h> // for strlen

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Slider
 * @{
 */

TouchSlider *TouchSlider::sSliderListStart = NULL;
uint16_t TouchSlider::sDefaultSliderColor = SLIDER_DEFAULT_SLIDER_COLOR;
uint16_t TouchSlider::sDefaultBarColor = SLIDER_DEFAULT_BAR_COLOR;
uint16_t TouchSlider::sDefaultBarThresholdColor = SLIDER_DEFAULT_BAR_THRESHOLD_COLOR;
uint16_t TouchSlider::sDefaultBarBackgroundColor = SLIDER_DEFAULT_BAR_BACK_COLOR;
uint16_t TouchSlider::sDefaultCaptionColor = SLIDER_DEFAULT_CAPTION_COLOR;
uint16_t TouchSlider::sDefaultValueColor = SLIDER_DEFAULT_VALUE_COLOR;
uint16_t TouchSlider::sDefaultValueCaptionBackgroundColor = SLIDER_DEFAULT_CAPTION_VALUE_BACK_COLOR;

uint8_t TouchSlider::sDefaultTouchBorder = SLIDER_DEFAULT_TOUCH_BORDER;

#ifdef REMOTE_DISPLAY_SUPPORTED
TouchSlider * TouchSlider::getLocalSliderFromBDSliderHandle(BDSliderHandle_t aSliderHandleToSearchFor) {
    TouchSlider * tSliderPointer = sSliderListStart;
// walk through list
    while (tSliderPointer != NULL) {
        if (tSliderPointer->mBDSliderPtr->mSliderHandle == aSliderHandleToSearchFor) {
            break;
        }
        tSliderPointer = tSliderPointer->mNextObject;
    }
    return tSliderPointer;
}

/*
 * is called once after reconnect, to build up a remote copy of all local sliders
 */
void TouchSlider::reinitAllLocalSlidersForRemote(void) {
    if (USART_isBluetoothPaired()) {
        TouchSlider * tSliderPointer = sSliderListStart;
        sLocalSliderIndex = 0;
// walk through list
        while (tSliderPointer != NULL) {
            // cannot use BDSlider.init since this allocates a new TouchSlider
            sendUSARTArgs(FUNCTION_SLIDER_CREATE, 12, tSliderPointer->mBDSliderPtr->mSliderHandle, tSliderPointer->mPositionX,
                    tSliderPointer->mPositionY, tSliderPointer->mBarWidth, tSliderPointer->mBarLength,
                    tSliderPointer->mThresholdValue, tSliderPointer->mActualValue, tSliderPointer->mSliderColor,
                    tSliderPointer->mBarColor, tSliderPointer->mFlags, tSliderPointer->mOnChangeHandler,
                    (reinterpret_cast<uint32_t>(tSliderPointer->mOnChangeHandler) >> 16));
            sLocalSliderIndex++;
            tSliderPointer = tSliderPointer->mNextObject;
        }
    }
}

/*
 * Destructor  - remove from slider list
 */
TouchSlider::~TouchSlider(void) {
    TouchSlider * tSliderPointer = sSliderListStart;
    if (tSliderPointer == this) {
        // remove first element of list
        sSliderListStart = NULL;
    } else {
        // walk through list to find previous element
        while (tSliderPointer != NULL) {
            if (tSliderPointer->mNextObject == this) {
                tSliderPointer->mNextObject = this->mNextObject;
                break;
            }
            tSliderPointer = tSliderPointer->mNextObject;
        }
    }
}

/*
 * For BDSlider
 */
void TouchSlider::initSlider(uint16_t aPositionX, uint16_t aPositionY, uint8_t aBarWidth, uint16_t aBarLength,
        uint16_t aThresholdValue, int16_t aInitalValue, uint16_t aSliderColor, uint16_t aBarColor, uint8_t aOptions,
        void (*aOnChangeHandler)(TouchSlider *, uint16_t)) {

    mCaption = NULL;
    /*
     * Copy parameter
     */
    mPositionX = aPositionX;
    mPositionY = aPositionY;
    mFlags = aOptions;
    mSliderColor = aSliderColor;
    mBarColor = aBarColor;
    mBarWidth = aBarWidth;
    mBarLength = aBarLength;
    mActualValue = aInitalValue;
    mThresholdValue = aThresholdValue;
    mTouchBorder = sDefaultTouchBorder;
    mOnChangeHandler = aOnChangeHandler;
    mValueHandler = NULL;

    checkParameterValues();

    uint8_t tShortBordersAddedWidth = mBarWidth;
    uint8_t tLongBordersAddedWidth = 2 * mBarWidth;
    if (!(mFlags & TOUCHFLAG_SLIDER_SHOW_BORDER)) {
        tShortBordersAddedWidth = 0;
        tLongBordersAddedWidth = 0;
    }

    /*
     * compute lower right corner and validate
     */
    if (mFlags & TOUCHFLAG_SLIDER_IS_HORIZONTAL) {
        mPositionXRight = mPositionX + mBarLength + tShortBordersAddedWidth - 1;
        if (mPositionXRight >= LOCAL_DISPLAY_WIDTH) {
            // simple fallback
            mPositionX = 0;
            failParamMessage(mPositionXRight, "XRight wrong");
            mPositionXRight = LOCAL_DISPLAY_WIDTH - 1;
        }
        mPositionYBottom = mPositionY + (tLongBordersAddedWidth + mBarWidth) - 1;
        if (mPositionYBottom >= LOCAL_DISPLAY_HEIGHT) {
            // simple fallback
            mPositionY = 0;
            failParamMessage(mPositionYBottom, "YBottom wrong");
            mPositionYBottom = LOCAL_DISPLAY_HEIGHT - 1;
        }

    } else {
        mPositionXRight = mPositionX + (tLongBordersAddedWidth + mBarWidth) - 1;
        if (mPositionXRight >= LOCAL_DISPLAY_WIDTH) {
            // simple fallback
            mPositionX = 0;
            failParamMessage(mPositionXRight, "XRight wrong");
            mPositionXRight = LOCAL_DISPLAY_WIDTH - 1;
        }
        mPositionYBottom = mPositionY + mBarLength + tShortBordersAddedWidth - 1;
        if (mPositionYBottom >= LOCAL_DISPLAY_HEIGHT) {
            // simple fallback
            mPositionY = 0;
            failParamMessage(mPositionYBottom, "YBottom wrong");
            mPositionYBottom = LOCAL_DISPLAY_HEIGHT - 1;
        }
    }
}
#endif

/*
 * Constructor - insert in list
 */
TouchSlider::TouchSlider(void) {
    mNextObject = NULL;
    if (sSliderListStart == NULL) {
        // first slider
        sSliderListStart = this;
    } else {
        // put object in slider list
        TouchSlider *tSliderPointer = sSliderListStart;
        // search last list element
        while (tSliderPointer->mNextObject != NULL) {
            tSliderPointer = tSliderPointer->mNextObject;
        }
        //append actual slider as last element
        tSliderPointer->mNextObject = this;
    }
    mBarThresholdColor = sDefaultBarThresholdColor;
    mBarBackgroundColor = sDefaultBarBackgroundColor;
    mCaptionColor = sDefaultCaptionColor;
    mValueColor = sDefaultValueColor;
    mValueCaptionBackgroundColor = sDefaultValueCaptionBackgroundColor;
    mTouchBorder = sDefaultTouchBorder;
    mXOffsetValue = 0;
}

/**
 * Static initialization of slider default colors
 */
void TouchSlider::setDefaults(uintForPgmSpaceSaving aDefaultTouchBorder, uint16_t aDefaultSliderColor, uint16_t aDefaultBarColor,
        uint16_t aDefaultBarThresholdColor, uint16_t aDefaultBarBackgroundColor, uint16_t aDefaultCaptionColor,
        uint16_t aDefaultValueColor, uint16_t aDefaultValueCaptionBackgroundColor) {
    sDefaultSliderColor = aDefaultSliderColor;
    sDefaultBarColor = aDefaultBarColor;
    sDefaultBarThresholdColor = aDefaultBarThresholdColor;
    sDefaultBarBackgroundColor = aDefaultBarBackgroundColor;
    sDefaultCaptionColor = aDefaultCaptionColor;
    sDefaultValueColor = aDefaultValueColor;
    sDefaultValueCaptionBackgroundColor = aDefaultValueCaptionBackgroundColor;
    sDefaultTouchBorder = aDefaultTouchBorder;
}

void TouchSlider::setDefaultSliderColor(uint16_t aDefaultSliderColor) {
    sDefaultSliderColor = aDefaultSliderColor;
}

void TouchSlider::setDefaultBarColor(uint16_t aDefaultBarColor) {
    sDefaultBarColor = aDefaultBarColor;
}

void TouchSlider::initSliderColors(uint16_t aSliderColor, uint16_t aBarColor, uint16_t aBarThresholdColor,
        uint16_t aBarBackgroundColor, uint16_t aCaptionColor, uint16_t aValueColor, uint16_t aValueCaptionBackgroundColor) {
    mSliderColor = aSliderColor;
    mBarColor = aBarColor;
    mBarThresholdColor = aBarThresholdColor;
    mBarBackgroundColor = aBarBackgroundColor;
    mCaptionColor = aCaptionColor;
    mValueColor = aValueColor;
    mValueCaptionBackgroundColor = aValueCaptionBackgroundColor;
}

void TouchSlider::setValueAndCaptionBackgroundColor(uint16_t aValueCaptionBackgroundColor) {
    mValueCaptionBackgroundColor = aValueCaptionBackgroundColor;
}

void TouchSlider::setValueColor(uint16_t aValueColor) {
    mValueColor = aValueColor;
}

/**
 * Static convenience method - activate all sliders
 * @see deactivateAllSliders()
 */
void TouchSlider::activateAllSliders(void) {
    TouchSlider *tObjectPointer = sSliderListStart;
    while (tObjectPointer != NULL) {
        tObjectPointer->activate();
        tObjectPointer = tObjectPointer->mNextObject;
    }
}

/**
 * Static convenience method - deactivate all sliders
 * @see activateAllSliders()
 */
void TouchSlider::deactivateAllSliders(void) {
    TouchSlider *tObjectPointer = sSliderListStart;
    while (tObjectPointer != NULL) {
        tObjectPointer->deactivate();
        tObjectPointer = tObjectPointer->mNextObject;
    }
}

/**
 * @brief initialization with all parameters except color
 * @param aPositionX determines upper left corner
 * @param aPositionY determines upper left corner
 * @param aBarWidth width of border and bar in pixel
 * @param aBarLength length of slider bar in pixel
 * @param aThresholdValue value where color of bar changes from #SLIDER_DEFAULT_BAR_COLOR to #SLIDER_DEFAULT_BAR_THRESHOLD_COLOR
 * @param aInitalValue
 * @param aCaption if NULL no caption is drawn
 * @param aTouchBorder border in pixel, where touches are recognized
 * @param aOptions see #FLAG_SLIDER_SHOW_BORDER etc.
 * @param aOnChangeHandler - if NULL no update of bar is done on touch
 * @param aValueHandler Handler to convert actualValue to string - if NULL sprintf("%3d", mActualValue) is used
 *  * @return false if parameters are not consistent ie. are internally modified
 *  or if not enough space to draw caption or value.
 */
void TouchSlider::initSlider(uint16_t aPositionX, uint16_t aPositionY, uintForPgmSpaceSaving aBarWidth, uint16_t aBarLength,
        uint16_t aThresholdValue, uint16_t aInitalValue, const char *aCaption, int8_t aTouchBorder, uint8_t aOptions,
        void (*aOnChangeHandler)(TouchSlider*, uint16_t), const char* (*aValueHandler)(uint16_t)) {

    mSliderColor = sDefaultSliderColor;
    mBarColor = sDefaultBarColor;
    /*
     * Copy parameter
     */
    mPositionX = aPositionX;
    mPositionY = aPositionY;
    mFlags = aOptions;
    mCaption = aCaption;
    mBarWidth = aBarWidth;
    mBarLength = aBarLength;
    mActualValue = aInitalValue;
    mThresholdValue = aThresholdValue;
    mTouchBorder = aTouchBorder;
    mOnChangeHandler = aOnChangeHandler;
    mValueHandler = aValueHandler;
    if (mValueHandler != NULL) {
        mFlags |= FLAG_SLIDER_SHOW_VALUE;
    }

    checkParameterValues();

    uintForPgmSpaceSaving tShortBordersAddedWidth = mBarWidth;
    uintForPgmSpaceSaving tLongBordersAddedWidth = 2 * mBarWidth;
    if (!(mFlags & TOUCHFLAG_SLIDER_SHOW_BORDER)) {
        tShortBordersAddedWidth = 0;
        tLongBordersAddedWidth = 0;
    }

    /*
     * compute lower right corner and validate
     */
    if (mFlags & TOUCHFLAG_SLIDER_IS_HORIZONTAL) {
        mPositionXRight = mPositionX + mBarLength + tShortBordersAddedWidth - 1;
        if (mPositionXRight >= LOCAL_DISPLAY_WIDTH) {
            // simple fallback
            mPositionX = 0;
            failParamMessage(mPositionXRight, "XRight wrong");
            mPositionXRight = LOCAL_DISPLAY_WIDTH - 1;
        }
        mPositionYBottom = mPositionY + (tLongBordersAddedWidth + mBarWidth) - 1;
        if (mPositionYBottom >= LOCAL_DISPLAY_HEIGHT) {
            // simple fallback
            mPositionY = 0;
            failParamMessage(mPositionYBottom, "YBottom wrong");
            mPositionYBottom = LOCAL_DISPLAY_HEIGHT - 1;
        }

    } else {
        mPositionXRight = mPositionX + (tLongBordersAddedWidth + mBarWidth) - 1;
        if (mPositionXRight >= LOCAL_DISPLAY_WIDTH) {
            // simple fallback
            mPositionX = 0;
            failParamMessage(mPositionXRight, "XRight wrong");
            mPositionXRight = LOCAL_DISPLAY_WIDTH - 1;
        }
        mPositionYBottom = mPositionY + mBarLength + tShortBordersAddedWidth - 1;
        if (mPositionYBottom >= LOCAL_DISPLAY_HEIGHT) {
            // simple fallback
            mPositionY = 0;
            failParamMessage(mPositionYBottom, "YBottom wrong");
            mPositionYBottom = LOCAL_DISPLAY_HEIGHT - 1;
        }
    }
}

void TouchSlider::drawSlider(void) {
    mIsActive = true;

    if ((mFlags & TOUCHFLAG_SLIDER_SHOW_BORDER)) {
        drawBorder();
    }

    // Fill middle bar with initial value
    drawBar();
    printCaption();
    // Print value as string
    printValue();
}

void TouchSlider::drawBorder(void) {
    uintForPgmSpaceSaving mShortBorderWidth = mBarWidth / 2;
    if (mFlags & TOUCHFLAG_SLIDER_IS_HORIZONTAL) {
        // Create value bar upper border
        LocalDisplay.fillRectRel(mPositionX, mPositionY, mBarLength + mBarWidth, mBarWidth, mSliderColor);
        // Create value bar lower border
        LocalDisplay.fillRectRel(mPositionX, mPositionY + (2 * mBarWidth), mBarLength + mBarWidth, mBarWidth, mSliderColor);

        // Create left border
        LocalDisplay.fillRectRel(mPositionX, mPositionY + mBarWidth, mShortBorderWidth, mBarWidth, mSliderColor);
        // Create right border
        LocalDisplay.fillRectRel(mPositionXRight - mShortBorderWidth + 1, mPositionY + mBarWidth, mShortBorderWidth, mBarWidth,
                mSliderColor);
    } else {
        // Create left border
        LocalDisplay.fillRectRel(mPositionX, mPositionY, mBarWidth, mBarLength + mBarWidth, mSliderColor);
        // Create right border
        LocalDisplay.fillRectRel(mPositionX + (2 * mBarWidth), mPositionY, mBarWidth, mBarLength + mBarWidth, mSliderColor);

        // Create value bar upper border
        LocalDisplay.fillRectRel(mPositionX + mBarWidth, mPositionY, mBarWidth, mShortBorderWidth, mSliderColor);
        // Create value bar lower border
        LocalDisplay.fillRectRel(mPositionX + mBarWidth, mPositionYBottom - mShortBorderWidth + 1, mBarWidth, mShortBorderWidth,
                mSliderColor);
    }
}

/*
 * (re)draws the middle bar according to actual value
 */
void TouchSlider::drawBar(void) {
    uint16_t tActualValue = mActualValue;
    if (tActualValue > mBarLength) {
        tActualValue = mBarLength;
    }
    if (tActualValue < 0) {
        tActualValue = 0;
    }

    uintForPgmSpaceSaving mShortBorderWidth = 0;
    uintForPgmSpaceSaving tLongBorderWidth = 0;

    if ((mFlags & TOUCHFLAG_SLIDER_SHOW_BORDER)) {
        mShortBorderWidth = mBarWidth / 2;
        tLongBorderWidth = mBarWidth;
    }

// draw background bar
    if (tActualValue < mBarLength) {
        if (mFlags & TOUCHFLAG_SLIDER_IS_HORIZONTAL) {
            LocalDisplay.fillRectRel(mPositionX + mShortBorderWidth + tActualValue, mPositionY + tLongBorderWidth,
                    mBarLength - tActualValue, mBarWidth, mBarBackgroundColor);
        } else {
            LocalDisplay.fillRectRel(mPositionX + tLongBorderWidth, mPositionY + mShortBorderWidth, mBarWidth,
                    mBarLength - tActualValue, mBarBackgroundColor);
        }
    }

// Draw value bar
    if (tActualValue > 0) {
        int tColor = mBarColor;
        if (tActualValue > mThresholdValue) {
            tColor = mBarThresholdColor;
        }
        if (mFlags & TOUCHFLAG_SLIDER_IS_HORIZONTAL) {
            LocalDisplay.fillRectRel(mPositionX + mShortBorderWidth, mPositionY + tLongBorderWidth, tActualValue, mBarWidth,
                    tColor);
        } else {
            LocalDisplay.fillRectRel(mPositionX + tLongBorderWidth, mPositionYBottom - mShortBorderWidth - tActualValue + 1,
                    mBarWidth, tActualValue, tColor);
        }
    }
}

void TouchSlider::setCaption(const char *aCaption) {
    mCaption = aCaption;
}

void TouchSlider::setCaptionColors(uint16_t aCaptionColor, uint16_t aValueCaptionBackgroundColor) {
    mCaptionColor = aCaptionColor;
    mValueCaptionBackgroundColor = aValueCaptionBackgroundColor;
}

void TouchSlider::setValueStringColors(uint16_t aValueStringColor, uint16_t aValueStringCaptionBackgroundColor) {
    mValueColor = aValueStringColor;
    mValueCaptionBackgroundColor = aValueStringCaptionBackgroundColor;
}

/**
 * Print caption in the middle below slider
 */
void TouchSlider::printCaption(void) {
    if (mCaption == NULL) {
        return;
    }
    uint16_t tCaptionLengthPixel = strlen(mCaption) * TEXT_SIZE_11_WIDTH;
    if (tCaptionLengthPixel == 0) {
        mCaption = NULL;
    }

    uintForPgmSpaceSaving tSliderWidthPixel;
    if (mFlags & TOUCHFLAG_SLIDER_IS_HORIZONTAL) {
        tSliderWidthPixel = mBarLength;
        if ((mFlags & TOUCHFLAG_SLIDER_SHOW_BORDER)) {
            tSliderWidthPixel += mBarWidth;
        }
    } else {
        tSliderWidthPixel = mBarWidth;
        if ((mFlags & TOUCHFLAG_SLIDER_SHOW_BORDER)) {
            tSliderWidthPixel = 3 * tSliderWidthPixel;
        }
    }

    /**
     * try to position the string in the middle below slider
     */
    uint16_t tCaptionPositionX = mPositionX + (tSliderWidthPixel / 2) - (tCaptionLengthPixel / 2);
// unsigned arithmetic overflow handling
    if (tCaptionPositionX > mPositionXRight) {
        tCaptionPositionX = 0;
    }

    // space for caption?
    uint16_t tCaptionPositionY = mPositionYBottom + (mBarWidth / 2);
    if (tCaptionPositionY > LOCAL_DISPLAY_HEIGHT - TEXT_SIZE_11) {
        // fallback
        failParamMessage(tCaptionPositionY, "Caption Bottom wrong");
        tCaptionPositionY = LOCAL_DISPLAY_HEIGHT - TEXT_SIZE_11;
    }

    LocalDisplay.drawText(tCaptionPositionX, tCaptionPositionY, (char*) mCaption, 1, mCaptionColor, mValueCaptionBackgroundColor);
}

/**
 * Print value left aligned to slider below caption or beneath if SLIDER_HORIZONTAL_VALUE_LEFT
 */
int TouchSlider::printValue(void) {
    if (!(mFlags & FLAG_SLIDER_SHOW_VALUE)) {
        return 0;
    }
    char *pValueAsString;
    unsigned int tValuePositionY = mPositionYBottom + mBarWidth / 2 + getTextAscend(TEXT_SIZE_11);
    if (mCaption != NULL && !((mFlags & TOUCHFLAG_SLIDER_IS_HORIZONTAL) && !(mFlags & TOUCHSLIDER_HORIZONTAL_VALUE_BELOW_TITLE))) {
        tValuePositionY += TEXT_SIZE_11_HEIGHT;
    }

    if (tValuePositionY > LOCAL_DISPLAY_HEIGHT - TEXT_SIZE_11_DECEND) {
        // fallback
        failParamMessage(tValuePositionY, "Value Bottom wrong");
        tValuePositionY = LOCAL_DISPLAY_HEIGHT - TEXT_SIZE_11_DECEND;
    }

    if (mValueHandler == NULL) {
        char tValueAsString[4];
        pValueAsString = &tValueAsString[0];
        sprintf(tValueAsString, "%03d", mActualValue);
    } else {
        // mValueHandler has to provide the char array
        pValueAsString = (char*) mValueHandler(mActualValue);
    }
    return LocalDisplay.drawText(mPositionX + mXOffsetValue, tValuePositionY - TEXT_SIZE_11_ASCEND, pValueAsString, 1, mValueColor,
            mValueCaptionBackgroundColor);
}

/**
 * Print value left aligned to slider below caption or beneath if SLIDER_HORIZONTAL_VALUE_LEFT
 */
int TouchSlider::printValue(const char *aValueString) {

    unsigned int tValuePositionY = mPositionYBottom + mBarWidth / 2 + getTextAscend(TEXT_SIZE_11);
    if (mCaption != NULL && !((mFlags & TOUCHFLAG_SLIDER_IS_HORIZONTAL) && !(mFlags & TOUCHSLIDER_HORIZONTAL_VALUE_BELOW_TITLE))) {
        tValuePositionY += TEXT_SIZE_11_HEIGHT;
    }

    if (tValuePositionY > LOCAL_DISPLAY_HEIGHT - TEXT_SIZE_11_DECEND) {
        // fallback
        tValuePositionY = LOCAL_DISPLAY_HEIGHT - TEXT_SIZE_11_DECEND;
    }
    return LocalDisplay.drawText(mPositionX + mXOffsetValue, tValuePositionY - TEXT_SIZE_11_ASCEND, aValueString, 1, mValueColor,
            mValueCaptionBackgroundColor);
}

/**
 * Check if touch event is in slider
 * - if yes - set bar and value call callback function and return true
 * - if no - return false
 */

bool TouchSlider::checkSlider(uint16_t aTouchPositionX, uint16_t aTouchPositionY) {
    unsigned int tPositionBorderX = mPositionX - mTouchBorder;
    if (mTouchBorder > mPositionX) {
        tPositionBorderX = 0;
    }
    unsigned int tPositionBorderY = mPositionY - mTouchBorder;
    if (mTouchBorder > mPositionY) {
        tPositionBorderY = 0;
    }
    if (!mIsActive || aTouchPositionX < tPositionBorderX || aTouchPositionX > mPositionXRight + mTouchBorder
            || aTouchPositionY < tPositionBorderY || aTouchPositionY > mPositionYBottom + mTouchBorder) {
        return false;
    }
    uintForPgmSpaceSaving tShortBorderWidth = 0;
    if ((mFlags & TOUCHFLAG_SLIDER_SHOW_BORDER)) {
        tShortBorderWidth = mBarWidth / 2;
    }
    /*
     *  Touch position is in slider (plus additional touch border) here
     */
    unsigned int tActualTouchValue;
    if (mFlags & TOUCHFLAG_SLIDER_IS_HORIZONTAL) {
        if (aTouchPositionX < (mPositionX + tShortBorderWidth)) {
            tActualTouchValue = 0;
        } else if (aTouchPositionX > (mPositionXRight - tShortBorderWidth)) {
            tActualTouchValue = mBarLength;
        } else {
            tActualTouchValue = aTouchPositionX - mPositionX - tShortBorderWidth + 1;
        }
    } else {
// adjust value according to size of upper and lower border
        if (aTouchPositionY > (mPositionYBottom - tShortBorderWidth)) {
            tActualTouchValue = 0;
        } else if (aTouchPositionY < (mPositionY + tShortBorderWidth)) {
            tActualTouchValue = mBarLength;
        } else {
            tActualTouchValue = mPositionYBottom - tShortBorderWidth - aTouchPositionY + 1;
        }
    }

    if (tActualTouchValue != mActualTouchValue) {
        mActualTouchValue = tActualTouchValue;
        if (mOnChangeHandler != NULL) {
#ifdef REMOTE_DISPLAY_SUPPORTED
            if (mFlags & FLAG_USE_BDSLIDER_FOR_CALLBACK) {
                mOnChangeHandler((TouchSlider *) this->mBDSliderPtr, tActualTouchValue);
                // Synchronize remote slider
                BlueDisplay1.setSliderValueAndDrawBar(this->mBDSliderPtr->mSliderHandle, tActualTouchValue);
            } else {
                mOnChangeHandler(this, tActualTouchValue);
            }
#else
            mOnChangeHandler(this, tActualTouchValue);
#endif
            // call change handler
            if (tActualTouchValue == mActualValue) {
                // value returned is equal displayed value - do nothing
                return true;
            }
            // display value changed - check, store and redraw
            if (tActualTouchValue > mBarLength) {
                tActualTouchValue = mBarLength;
            }
            mActualValue = tActualTouchValue;
            drawBar();
            printValue();
        }
    }
    return true;
}

/**
 * Static convenience method - checks all sliders in for event position.
 */

bool TouchSlider::checkAllSliders(unsigned int aTouchPositionX, unsigned int aTouchPositionY) {
    TouchSlider *tObjectPointer = sSliderListStart;

// walk through list of active elements
    while (tObjectPointer != NULL) {
        if (tObjectPointer->mIsActive && tObjectPointer->checkSlider(aTouchPositionX, aTouchPositionY)) {
            return true;
        }
        tObjectPointer = tObjectPointer->mNextObject;
    }
    return false;
}

int16_t TouchSlider::getCurrentValue(void) const {
    return mActualValue;
}

/*
 * also redraws value bar
 */
void TouchSlider::setValue(int16_t aCurrentValue) {
    mActualValue = aCurrentValue;
}

void TouchSlider::setValueAndDraw(int16_t aCurrentValue) {
    mActualValue = aCurrentValue;
    drawBar();
    printValue(); // this checks for flag TOUCHFLAG_SLIDER_SHOW_VALUE itself
}

void TouchSlider::setValueAndDrawBar(int16_t aCurrentValue) {
    mActualValue = aCurrentValue;
    drawBar();
    printValue(); // this checks for flag TOUCHFLAG_SLIDER_SHOW_VALUE itself
}

void TouchSlider::setXOffsetValue(int16_t aXOffsetValue) {
    mXOffsetValue = aXOffsetValue;
}

uint16_t TouchSlider::getPositionXRight(void) const {
    return mPositionXRight;
}

uint16_t TouchSlider::getPositionYBottom(void) const {
    return mPositionYBottom;
}

void TouchSlider::activate(void) {
    mIsActive = true;
}
void TouchSlider::deactivate(void) {
    mIsActive = false;
}

int8_t TouchSlider::checkParameterValues(void) {
    /**
     * Check and copy parameter
     */
    int8_t tRetValue = 0;

    if (mBarWidth == 0) {
        mBarWidth = SLIDER_DEFAULT_BAR_WIDTH;
    } else if (mBarWidth > SLIDER_MAX_BAR_WIDTH) {
        mBarWidth = SLIDER_MAX_BAR_WIDTH;
    }
    if (mBarLength == 0) {
        mBarLength = SLIDER_DEFAULT_MAX_VALUE;
    }
    if (mActualValue > mBarLength) {
        tRetValue = SLIDER_ERROR_ACTUAL_VALUE;
        mActualValue = mBarLength;
    }

    return tRetValue;
}

void TouchSlider::setBarThresholdColor(uint16_t barThresholdColor) {
    mBarThresholdColor = barThresholdColor;
}

void TouchSlider::setSliderColor(uint16_t sliderColor) {
    mSliderColor = sliderColor;
}

void TouchSlider::setBarColor(uint16_t barColor) {
    mBarColor = barColor;
}

void TouchSlider::setBarBackgroundColor(uint16_t aBarBackgroundColor) {
    mBarBackgroundColor = aBarBackgroundColor;
}

/** @} */
/** @} */
