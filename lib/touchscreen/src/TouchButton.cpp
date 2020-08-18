/**
 * @file TouchButton.cpp
 *
 * Renders touch buttons for lcd
 * A button can be a simple clickable text
 * or a box with or without text or even an invisible touch area
 *
 * @date  30.01.2012
 * @author  Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 *  Local display interface used:
 * 		mDisplay->fillRectRel()
 * 		mDisplay->drawText()
 * 		LOCAL_DISPLAY_WIDTH
 * 		LOCAL_DISPLAY_HEIGHT
 *
 *
 */

#include "MI0283QT2.h"

#include "TouchButton.h"
#include "TouchButtonAutorepeat.h"
#include "EventHandler.h"
#include "AssertErrorAndMisc.h"
#include "stm32fx0xPeripherals.h"// for FeedbackToneOK
#ifdef REMOTE_DISPLAY_SUPPORTED
#include "BlueDisplayProtocol.h"
#include "BlueSerial.h"
#include "BlueDisplay.h"
#endif

#include <stdio.h>
#include <string.h> // for strlen

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Button
 * @{
 */

#ifdef USE_BUTTON_POOL
/**
 * The preallocated pool of buttons for this application
 */
TouchButton TouchButton::TouchButtonPool[NUMBER_OF_BUTTONS_IN_POOL]; // 2000 Bytes
uint8_t TouchButton::sButtonCombinedPoolSize = NUMBER_OF_BUTTONS_IN_POOL;// NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL is added while initializing autorepeat pool
uint8_t TouchButton::sMinButtonPoolSize = NUMBER_OF_BUTTONS_IN_POOL;
bool TouchButton::sButtonPoolIsInitialized = false;
#endif

/*
 * List for TouchButton(Autorepeat) objects
 */
TouchButton * TouchButton::sButtonListStart = NULL;
uint16_t TouchButton::sDefaultCaptionColor = TOUCHBUTTON_DEFAULT_CAPTION_COLOR;

/*
 * Constructor - insert in list
 */
TouchButton::TouchButton(void) {
    mNextObject = NULL;
    if (sButtonListStart == NULL) {
        // first button
        sButtonListStart = this;
    } else {
        // put object in button list
        TouchButton * tButtonPointer = sButtonListStart;
        // search last list element
        while (tButtonPointer->mNextObject != NULL) {
            tButtonPointer = tButtonPointer->mNextObject;
        }
        //insert actual button as last element
        tButtonPointer->mNextObject = this;
    }
}

/*
 * Destructor  - remove from button list
 */
TouchButton::~TouchButton(void) {
    TouchButton * tButtonPointer = sButtonListStart;
    if (tButtonPointer == this) {
        // remove first element of list
        sButtonListStart = NULL;
    } else {
        // walk through list to find previous element
        while (tButtonPointer != NULL) {
            if (tButtonPointer->mNextObject == this) {
                tButtonPointer->mNextObject = this->mNextObject;
                break;
            }
            tButtonPointer = tButtonPointer->mNextObject;
        }
    }
}

void TouchButton::setDefaultCaptionColor(uint16_t aDefaultCaptionColor) {
    sDefaultCaptionColor = aDefaultCaptionColor;
}

#ifdef REMOTE_DISPLAY_SUPPORTED
TouchButton * TouchButton::getLocalButtonFromBDButtonHandle(BDButtonHandle_t aButtonHandleToSearchFor) {
    TouchButton * tButtonPointer = sButtonListStart;
// walk through list
    while (tButtonPointer != NULL) {
        if (tButtonPointer->mBDButtonPtr->mButtonHandle == aButtonHandleToSearchFor) {
            break;
        }
        tButtonPointer = tButtonPointer->mNextObject;
    }
    if (tButtonPointer->mFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
        return (TouchButtonAutorepeat *) tButtonPointer;
    } else {
        return tButtonPointer;
    }
}

/*
 * is called once after reconnect, to build up a remote copy of all local buttons
 */
void TouchButton::reinitAllLocalButtonsForRemote(void) {
    if (USART_isBluetoothPaired()) {
        TouchButton * tButtonPointer = sButtonListStart;
        sLocalButtonIndex = 0;
// walk through list
        while (tButtonPointer != NULL) {
            // cannot use BDButton.init since this allocates a new TouchButton
            sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 11, tButtonPointer->mBDButtonPtr->mButtonHandle,
                    tButtonPointer->mPositionX, tButtonPointer->mPositionY, tButtonPointer->mWidthX, tButtonPointer->mHeightY,
                    tButtonPointer->mButtonColor, tButtonPointer->mCaptionSize, tButtonPointer->mFlags & ~(LOCAL_BUTTON_FLAG_MASK),
                    tButtonPointer->mValue, tButtonPointer->mOnTouchHandler,
                    (reinterpret_cast<uint32_t>(tButtonPointer->mOnTouchHandler) >> 16), strlen(tButtonPointer->mCaption),
                    tButtonPointer->mCaption);
            if (tButtonPointer->mFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
                TouchButtonAutorepeat * tAutorepeatButtonPointer = (TouchButtonAutorepeat*) tButtonPointer;
                sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 7, tAutorepeatButtonPointer->mBDButtonPtr->mButtonHandle,
                        SUBFUNCTION_BUTTON_SET_AUTOREPEAT_TIMING, tAutorepeatButtonPointer->mMillisFirstDelay,
                        tAutorepeatButtonPointer->mMillisFirstRate, tAutorepeatButtonPointer->mFirstCount,
                        tAutorepeatButtonPointer->mMillisSecondRate);
            }
            sLocalButtonIndex++;
            tButtonPointer = tButtonPointer->mNextObject;
        }
    }
}
#endif

/**
 * Set parameters (except colors and touch border size) for touch button
 * if aWidthX == 0 render only text no background box
 * if aCaptionSize == 0 don't render anything, just check touch area -> transparent button ;-)
 */
int8_t TouchButton::initButton(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, uint16_t aButtonColor,
        const char * aCaption, uint8_t aCaptionSize, uint8_t aFlags, int16_t aValue,
        void (*aOnTouchHandler)(TouchButton *, int16_t)) {
    mWidthX = aWidthX;
    mHeightY = aHeightY;
    mButtonColor = aButtonColor;
    mCaptionColor = sDefaultCaptionColor;
    mCaption = aCaption;
    mCaptionSize = aCaptionSize;
    mOnTouchHandler = aOnTouchHandler;

    if (aFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
        if (aValue != 0) {
            // map to boolean
            aValue = true;
            mButtonColor = COLOR_GREEN;
        } else {
            mButtonColor = COLOR_RED;
        }
    }
    mValue = aValue;
#ifdef USE_BUTTON_POOL
    mFlags = (aFlags & (~INTERNAL_FLAG_MASK)) | (mFlags & INTERNAL_FLAG_MASK);
#else
    mFlags = aFlags;
#endif
    return setPosition(aPositionX, aPositionY);
}

int8_t TouchButton::setPosition(uint16_t aPositionX, uint16_t aPositionY) {
    int8_t tRetValue = 0;
    mPositionX = aPositionX;
    mPositionY = aPositionY;

// check values
    if (aPositionX + mWidthX > LOCAL_DISPLAY_WIDTH) {
        mWidthX = LOCAL_DISPLAY_WIDTH - aPositionX;
        failParamMessage(aPositionX + mWidthX, "XRight wrong");
        tRetValue = TOUCHBUTTON_ERROR_X_RIGHT;
    }
    if (aPositionY + mHeightY > LOCAL_DISPLAY_HEIGHT) {
        mHeightY = LOCAL_DISPLAY_HEIGHT - mPositionY;
        failParamMessage(aPositionY + mHeightY, "YBottom wrong");
        tRetValue = TOUCHBUTTON_ERROR_Y_BOTTOM;
    }
    return tRetValue;
}

/**
 * renders the button on lcd
 */
void TouchButton::drawButton() {
// Draw rect
    LocalDisplay.fillRectRel(mPositionX, mPositionY, mWidthX, mHeightY, mButtonColor);
    drawCaption();
}

/**
 * deactivates the button and redraws its screen space with @a aBackgroundColor
 */
void TouchButton::removeButton(uint16_t aBackgroundColor) {
    mFlags &= ~FLAG_IS_ACTIVE;
// Draw rect
    LocalDisplay.fillRectRel(mPositionX, mPositionY, mWidthX, mHeightY, aBackgroundColor);

}

/**
 * draws the caption of a button
 * @retval 0 or error number #TOUCHBUTTON_ERROR_CAPTION_TOO_LONG etc.
 */
void TouchButton::drawCaption(void) {
    mFlags |= FLAG_IS_ACTIVE;
    if (mCaptionSize > 0) { // dont render anything if caption size == 0
        if (mCaption != NULL) {
            int tXCaptionPosition;
            int tYCaptionPosition;
            /*
             * Simple 2 line handling
             * 1. position first string in the middle of the box
             * 2. draw second string just below
             */
            char * tPosOfNewline = strchr(mCaption, '\n');
            int tCaptionHeight = getTextHeight(mCaptionSize);
            int tStringlength = strlen(mCaption);
            if (tPosOfNewline != NULL) {
                // assume 2 lines of caption
                tCaptionHeight = 2 * getTextHeight(mCaptionSize);
                tStringlength = (tPosOfNewline - mCaption);
            }
            int tLength = getTextWidth(mCaptionSize) * tStringlength;
            // try to position the string in the middle of the box
            if (tLength >= mWidthX) {
                // String too long here
                tXCaptionPosition = mPositionX;
            } else {
                tXCaptionPosition = mPositionX + ((mWidthX - tLength) / 2);
            }

            if (tCaptionHeight >= mHeightY) {
                // Font height to big
                tYCaptionPosition = mPositionY;
            } else {
                tYCaptionPosition = mPositionY + ((mHeightY - tCaptionHeight) / 2);
            }
            LocalDisplay.drawNCharacters(tXCaptionPosition, tYCaptionPosition, (char*) mCaption, getLocalTextSize(mCaptionSize),
                    mCaptionColor, mButtonColor, tStringlength);
            if (tPosOfNewline != NULL) {
                // 2. line - position the string in the middle of the box again
                tStringlength = strlen(mCaption) - tStringlength - 1;
                tLength = getTextWidth(mCaptionSize) * tStringlength;
                tXCaptionPosition = mPositionX + ((mWidthX - tLength) / 2);
                LocalDisplay.drawNCharacters(tXCaptionPosition, tYCaptionPosition + getTextHeight(mCaptionSize),
                        (tPosOfNewline + 1), getLocalTextSize(mCaptionSize), mCaptionColor, mButtonColor, tStringlength);
            }
        }
    }
}

bool TouchButton::isAutorepeatButton(void) {
    return (mFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN);
}

/**
 * Check if touch event is in button area
 * if yes - return true
 * if no - return false
 */
bool TouchButton::checkButtonInArea(uint16_t aTouchPositionX, uint16_t aTouchPositionY) {
    if (aTouchPositionX < mPositionX || aTouchPositionX > mPositionX + mWidthX || aTouchPositionY < mPositionY
            || aTouchPositionY > (mPositionY + mHeightY)) {
        return false;
    }
    return true;
}

/**
 * Check if touch event is in button area
 * if yes - call callback function and return true
 * if no - return false
 */
bool TouchButton::checkButton(uint16_t aTouchPositionX, uint16_t aTouchPositionY) {
    if ((mFlags & FLAG_IS_ACTIVE) && mOnTouchHandler != NULL && checkButtonInArea(aTouchPositionX, aTouchPositionY)) {
        /*
         *  Touch position is in button - call callback function
         */
        if (mFlags & FLAG_BUTTON_DO_BEEP_ON_TOUCH) {
            FeedbackToneOK();
        }
        /*
         * Handle Toggle red/green button
         */
        if (mFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
            setValueAndDraw(!mValue);
        }

#ifdef REMOTE_DISPLAY_SUPPORTED
        if ((mFlags & FLAG_USE_BDBUTTON_FOR_CALLBACK)
                && (&TouchButtonAutorepeat::autorepeatTouchHandler != (void (*)(TouchButtonAutorepeat *, int16_t)) mOnTouchHandler)) {
            mOnTouchHandler((TouchButton *) this->mBDButtonPtr, mValue);
        } else {
            mOnTouchHandler(this, mValue);
        }
#else
        mOnTouchHandler(this, mValue);
#endif
        return true;
    }
    return false;
}

/**
 * Static convenience method - checks all buttons for matching touch position.
 */
uint8_t TouchButton::checkAllButtons(unsigned int aTouchPositionX, unsigned int aTouchPositionY) {
// walk through list of active elements
    TouchButton * tButtonPointer = sButtonListStart;
    while (tButtonPointer != NULL) {
        if ((tButtonPointer->mFlags & FLAG_IS_ACTIVE) && tButtonPointer->checkButton(aTouchPositionX, aTouchPositionY)) {
            if (tButtonPointer->mFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
                return BUTTON_TOUCHED_AUTOREPEAT;
            } else {
                return BUTTON_TOUCHED;
            }
        }
        tButtonPointer = tButtonPointer->mNextObject;
    }
    return NOT_TOUCHED;
}

/**
 * Static convenience method - deactivate all buttons (e.g. before switching screen)
 */
void TouchButton::deactivateAllButtons() {
    TouchButton * tObjectPointer = sButtonListStart;
// walk through list
    while (tObjectPointer != NULL) {
        tObjectPointer->deactivate();
        tObjectPointer = tObjectPointer->mNextObject;
    }
}

/**
 * Static convenience method - activate all buttons
 */
void TouchButton::activateAllButtons() {
    TouchButton * tObjectPointer = sButtonListStart;
// walk through list
    while (tObjectPointer != NULL) {
        tObjectPointer->activate();
        tObjectPointer = tObjectPointer->mNextObject;
    }
}

const char * TouchButton::getCaption() const {
    return mCaption;
}

uint16_t TouchButton::getValue() const {
    return mValue;
}

/*
 * Set caption
 */
void TouchButton::setCaption(const char * aCaption) {
    mCaption = aCaption;
}

/*
 * Set caption and draw
 */
void TouchButton::setCaptionAndDraw(const char * aCaption) {
    mCaption = aCaption;
    drawButton();
}

/*
 * changes box color and redraws button
 */
void TouchButton::setButtonColor(uint16_t aButtonColor) {
    mButtonColor = aButtonColor;
}

void TouchButton::setButtonColorAndDraw(uint16_t aButtonColor) {
    mButtonColor = aButtonColor;
    this->drawButton();
}

void TouchButton::setCaptionColor(uint16_t aColor) {
    mCaptionColor = aColor;
}

/*
 * value 0 -> red, 1 -> green
 */
void TouchButton::setValue(int16_t aValue) {
    if (mFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
        if (aValue != 0) {
            // map to boolean
            aValue = true;
            mButtonColor = COLOR_GREEN;
        } else {
            mButtonColor = COLOR_RED;
        }
    }
    mValue = aValue;
}

void TouchButton::setValueAndDraw(int16_t aValue) {
    if (mFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
        if (aValue != 0) {
            // map to boolean
            aValue = true;
            mButtonColor = COLOR_GREEN;
        } else {
            mButtonColor = COLOR_RED;
        }
    }
    mValue = aValue;
    drawButton();
}

uint16_t TouchButton::getPositionX() const {
    return mPositionX;
}

int8_t TouchButton::setPositionX(uint16_t aPositionX) {
    return setPosition(aPositionX, mPositionY);
}

uint16_t TouchButton::getPositionY() const {
    return mPositionY;
}

int8_t TouchButton::setPositionY(uint16_t aPositionY) {
    return setPosition(mPositionX, aPositionY);
}

uint16_t TouchButton::getPositionXRight() const {
    return mPositionX + mWidthX - 1;
}

uint16_t TouchButton::getPositionYBottom() const {
    return mPositionY + mHeightY - 1;
}

/*
 * activate for touch checking
 */
void TouchButton::activate() {
    mFlags |= FLAG_IS_ACTIVE;
}

/*
 * deactivate for touch checking
 */
void TouchButton::deactivate() {
    mFlags &= ~FLAG_IS_ACTIVE;
}

void TouchButton::setTouchHandler(void (*aOnTouchHandler)(TouchButton*, int16_t)) {
    mOnTouchHandler = aOnTouchHandler;
}

/****************************************
 * Plain util c++ functions
 ****************************************/

#ifdef USE_BUTTON_POOL
/**
 *
 * @param aPositionX
 * @param aPositionY
 * @param aWidthX       if aWidthX == 0 render only text no background box
 * @param aHeightY
 * @param aButtonColor  if 0 take sDefaultButtonColor
 * @param aCaption
 * @param aCaptionSize  if aCaptionSize == 0 don't render anything, just check touch area -> transparent button ;-)
 * @param aValue
 * @param aOnTouchHandler
 * @return pointer to allocated button
 */
TouchButton * TouchButton::allocAndInitButton(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY,
        uint16_t aButtonColor, const char * aCaption, uint8_t aCaptionSize, uint8_t aFlags, int16_t aValue,
        void (*aOnTouchHandler)(TouchButton *, int16_t)) {

    TouchButton * tButton;
    if (aFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
        tButton = TouchButtonAutorepeat::allocAutorepeatButton();
    } else {
        tButton = allocButton(false);
    }

    tButton->mWidthX = aWidthX;
    tButton->mHeightY = aHeightY;
    tButton->mButtonColor = aButtonColor;
    tButton->mCaptionColor = sDefaultCaptionColor;
    tButton->mCaption = aCaption;
    tButton->mCaptionSize = aCaptionSize;
    tButton->mOnTouchHandler = aOnTouchHandler;

    if (aFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
        if (aValue != 0) {
            // map to boolean
            aValue = true;
            tButton->mButtonColor = COLOR_GREEN;
        } else {
            tButton->mButtonColor = COLOR_RED;
        }
    }
    tButton->mValue = aValue;

// keep internal flags set by allocButton()
    tButton->mFlags = (aFlags & (~INTERNAL_FLAG_MASK)) | (tButton->mFlags & INTERNAL_FLAG_MASK);
    tButton->setPosition(aPositionX, aPositionY);
    return tButton;
}

/****************************
 * Pool functions
 ****************************/

/**
 * Allocate / get first button from the unallocated buttons
 * @retval Button pointer or message and NULL pointer if no button available
 */
TouchButton * TouchButton::allocButton(bool aOnlyAutorepeatButtons) {
    TouchButton * tButtonPointer;
    if (!sButtonPoolIsInitialized) {
        tButtonPointer = NULL;
        // mark pool as free
        for (int i = NUMBER_OF_BUTTONS_IN_POOL - 1; i >= 0; i--) {
            TouchButtonPool[i].mFlags = 0;
            TouchButtonPool[i].mNextObject = tButtonPointer;
            tButtonPointer = &TouchButtonPool[i];
        }
        sButtonPoolIsInitialized = true;
        sButtonListStart = &TouchButtonPool[0];
    }

// walk through list
    tButtonPointer = sButtonListStart;
    while (tButtonPointer != NULL && (tButtonPointer->mFlags & FLAG_IS_ALLOCATED)) {
        tButtonPointer = tButtonPointer->mNextObject;
    }
    if (tButtonPointer == NULL) {
        failParamMessage(sButtonCombinedPoolSize, "No button available");
        // to avoid NULL pointer
        tButtonPointer = sButtonListStart;
    } else {
        tButtonPointer->mFlags |= FLAG_IS_ALLOCATED;
        sButtonCombinedPoolSize--;
    }

    if (sButtonCombinedPoolSize < sMinButtonPoolSize) {
        sMinButtonPoolSize = sButtonCombinedPoolSize;
    }
    return tButtonPointer;
}

/**
 * Deallocates / free a button and put it back to button bool
 * Do not need to remove from list since FLAG_IS_ALLOCATED ist set
 */
void TouchButton::setFree(void) {
    assertParamMessage((this != NULL), sButtonCombinedPoolSize, "Button handle is null");
    sButtonCombinedPoolSize++;
    mFlags &= ~FLAG_IS_ALLOCATED;
    mFlags &= ~FLAG_IS_ACTIVE;
}

/**
 * free / release one button to the unallocated buttons
 */
void TouchButton::freeButton(TouchButton * aTouchButton) {
    TouchButton * tButtonPointer = sButtonListStart;
// walk through list
    while (tButtonPointer != NULL && tButtonPointer != aTouchButton) {
        tButtonPointer = tButtonPointer->mNextObject;
    }
    if (tButtonPointer == NULL) {
        failParamMessage(aTouchButton, "Button not found in list");
    } else {
        sButtonCombinedPoolSize++;
        tButtonPointer->mFlags &= ~FLAG_IS_ALLOCATED;
        tButtonPointer->mFlags &= ~FLAG_IS_ACTIVE;
    }
}

/**
 * prints amount of total, free and active buttons
 * @param aStringBuffer the string buffer for the result
 */
void TouchButton::infoButtonPool(char * aStringBuffer) {
    TouchButton * tObjectPointer = sButtonListStart;
    int tTotal = 0;
    int tFree = 0;
    int tActive = 0;
// walk through list
    while (tObjectPointer != NULL) {
        tTotal++;
        if (!(tObjectPointer->mFlags & FLAG_IS_ALLOCATED)) {
            tFree++;
        }
        if (tObjectPointer->mFlags & FLAG_IS_ACTIVE) {
            tActive++;
        }
        tObjectPointer = tObjectPointer->mNextObject;
    }
// output real and computed free values
    sprintf(aStringBuffer, "total=%2d free=%2d |%2d min=%2d active=%2d ", tTotal, tFree, sButtonCombinedPoolSize,
            sMinButtonPoolSize, tActive);
}
#endif

/** @} */
/** @} */
