/*
 * TouchButtonAutorepeat.cpp
 *
 * Extension of TouchButton
 * implements autorepeat feature for touch buttons
 *
 * @date  30.02.2012
 * @author  Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 * 		52 Byte per autorepeat button on 32Bit ARM
 *
 */

#include "TouchButtonAutorepeat.h"
#include "EventHandler.h"
#include "timing.h"
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
#include "BlueDisplayProtocol.h"
#include "BlueSerial.h"
#include <string.h> // for strlen
#endif

#include <stdlib.h> // for NULL

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Button
 * @{
 */
#define AUTOREPEAT_BUTTON_STATE_FIRST_DELAY 0
#define AUTOREPEAT_BUTTON_STATE_FIRST_PERIOD 1
#define AUTOREPEAT_BUTTON_STATE_SECOND_PERIOD 2

#ifdef USE_BUTTON_POOL
TouchButtonAutorepeat TouchButtonAutorepeat::TouchButtonAutorepeatPool[NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL];
TouchButtonAutorepeat * TouchButtonAutorepeat::sAutorepeatButtonListStart = NULL;
bool TouchButtonAutorepeat::sButtonAutorepeatPoolIsInitialized = false;
#endif

uint8_t TouchButtonAutorepeat::sState;
uint16_t TouchButtonAutorepeat::sCount;

unsigned long TouchButtonAutorepeat::sMillisOfLastCall;
unsigned long TouchButtonAutorepeat::sMillisSinceFirstTouch;
unsigned long TouchButtonAutorepeat::sMillisSinceLastCallback;

TouchButtonAutorepeat * TouchButtonAutorepeat::mLastAutorepeatButtonTouched = NULL;

TouchButtonAutorepeat::TouchButtonAutorepeat() {
}

/**
 * after aMillisFirstDelay milliseconds a callback is done every aMillisFirstRate milliseconds for aFirstCount times
 * after this a callback is done every aMillisSecondRate milliseconds
 */
void TouchButtonAutorepeat::setButtonAutorepeatTiming(uint16_t aMillisFirstDelay, uint16_t aMillisFirstRate,
        uint16_t aFirstCount, uint16_t aMillisSecondRate) {
    mMillisFirstDelay = aMillisFirstDelay;
    mMillisFirstRate = aMillisFirstRate;
    if (aFirstCount < 1) {
        aFirstCount = 1;
    }
    mFirstCount = aFirstCount;
    mMillisSecondRate = aMillisSecondRate;
    // replace standard button handler
    if (mOnTouchHandler != ((void (*)(TouchButton*, int16_t)) &autorepeatTouchHandler)) {
        mOnTouchHandlerAutorepeat = mOnTouchHandler;
        mOnTouchHandler = ((void (*)(TouchButton*, int16_t)) &autorepeatTouchHandler);
    }
}

/**
 * Touch handler for button object which implements the autorepeat functionality
 */
void TouchButtonAutorepeat::autorepeatTouchHandler(TouchButtonAutorepeat * aTheTouchedButton, int16_t aButtonValue) {
    // First touch here => register callback
    mLastAutorepeatButtonTouched = aTheTouchedButton;
    registerPeriodicTouchCallback(&TouchButtonAutorepeat::autorepeatButtonTimerHandler,
            aTheTouchedButton->mMillisFirstDelay);
    if (aTheTouchedButton->mFlags & FLAG_USE_BDBUTTON_FOR_CALLBACK) {
        aTheTouchedButton->mOnTouchHandlerAutorepeat((TouchButton *) aTheTouchedButton->mBDButtonPtr, aButtonValue);
    } else {
        aTheTouchedButton->mOnTouchHandlerAutorepeat(aTheTouchedButton, aButtonValue);
    }
    sState = AUTOREPEAT_BUTTON_STATE_FIRST_DELAY;
    sCount = aTheTouchedButton->mFirstCount;
}

/**
 * callback function for SysTick timer.
 */

bool TouchButtonAutorepeat::autorepeatButtonTimerHandler(int aTouchPositionX, int aTouchPositionY) {
    assert_param(mLastAutorepeatButtonTouched != NULL);

    bool tDoCallback = true;
    TouchButtonAutorepeat * tTouchedButton = mLastAutorepeatButtonTouched;
    if (!tTouchedButton->checkButtonInArea(aTouchPositionX, aTouchPositionY)) {
        return false;
    }
    switch (sState) {
    case AUTOREPEAT_BUTTON_STATE_FIRST_DELAY:
        setPeriodicTouchCallbackPeriod(tTouchedButton->mMillisFirstRate);
        sState++;
        sCount--;
        break;
    case AUTOREPEAT_BUTTON_STATE_FIRST_PERIOD:
        if (sCount == 0) {
            setPeriodicTouchCallbackPeriod(tTouchedButton->mMillisSecondRate);
            sCount = 0;
        } else {
            sCount--;
        }
        break;
    case AUTOREPEAT_BUTTON_STATE_SECOND_PERIOD:
        // just for fun
        sCount++;
        break;
    default:
        tDoCallback = false;
        break;
    }

    if (tDoCallback) {
        if (tTouchedButton->mFlags & FLAG_USE_BDBUTTON_FOR_CALLBACK) {
            tTouchedButton->mOnTouchHandlerAutorepeat((TouchButton *) tTouchedButton->mBDButtonPtr,
                    tTouchedButton->mValue);
        } else {
            tTouchedButton->mOnTouchHandlerAutorepeat(tTouchedButton, tTouchedButton->mValue);
        }
        return true;
    }
    return false;
}

#ifdef USE_BUTTON_POOL

/**
 * Allocate / get first button from the unallocated buttons
 * @retval Button pointer or message and NULL pointer if no button available
 */
TouchButtonAutorepeat * TouchButtonAutorepeat::allocAutorepeatButton() {
    TouchButtonAutorepeat * tButtonPointer;
    if (!sButtonAutorepeatPoolIsInitialized) {
        tButtonPointer = NULL;
        // mark pool as free
        for (int i = NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL - 1; i >= 0; i--) {
            TouchButtonAutorepeatPool[i].mFlags = 0;
            TouchButtonAutorepeatPool[i].mNextObject = tButtonPointer;
            tButtonPointer = &TouchButtonAutorepeatPool[i];
        }
        sButtonAutorepeatPoolIsInitialized = true;
        sAutorepeatButtonListStart = &TouchButtonAutorepeatPool[0];
        TouchButton::sButtonCombinedPoolSize += NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL;
    }

    // walk through list
    tButtonPointer = sAutorepeatButtonListStart;
    while (tButtonPointer != NULL && (tButtonPointer->mFlags & FLAG_IS_ALLOCATED)) {
        tButtonPointer = (TouchButtonAutorepeat*) tButtonPointer->mNextObject;
    }
    if (tButtonPointer == NULL) {
        failParamMessage(sButtonCombinedPoolSize, "No autorepeat button available");
        // to avoid NULL pointer
        tButtonPointer = sAutorepeatButtonListStart;
    } else {
        tButtonPointer->mFlags |= FLAG_IS_ALLOCATED;
        sButtonCombinedPoolSize--;
    }

    if (sButtonCombinedPoolSize < sMinButtonPoolSize) {
        sMinButtonPoolSize = sButtonCombinedPoolSize;
    }
    return tButtonPointer;
}

#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
/*
 * is called once after reconnect, to build up a remote copy of all local buttons
 */
void TouchButtonAutorepeat::reinitAllLocalAutorepeatButtonsForRemote(void) {
    if (USART_isBluetoothPaired()) {
        TouchButtonAutorepeat * tButtonPointer = sAutorepeatButtonListStart;
// walk through list
        while (tButtonPointer != NULL) {
            // cannot use BDButton.init since this allocates a new TouchButton
            sendUSARTArgsAndByteBuffer(FUNCTION_TAG_BUTTON_CREATE, 10, tButtonPointer->mBDButtonPtr->mButtonHandle,
                    tButtonPointer->mPositionX, tButtonPointer->mPositionY, tButtonPointer->mWidthX,
                    tButtonPointer->mHeightY, tButtonPointer->mButtonColor,
                    tButtonPointer->mCaptionSize | (tButtonPointer->mFlags << 8), tButtonPointer->mValue,
                    tButtonPointer->mOnTouchHandler,
                    (reinterpret_cast<uint32_t>(tButtonPointer->mOnTouchHandler) >> 16),
                    strlen(tButtonPointer->mCaption), tButtonPointer->mCaption);
            sLocalButtonIndex++;
            tButtonPointer = (TouchButtonAutorepeat *) tButtonPointer->mNextObject;
        }
    }
}
#endif
#endif

/** @} */
/** @} */
