/**
 * @file TouchButtonAutorepeat.h
 *
 * Extension of ToucButton
 * implements autorepeat feature for touchbuttons
 *
 * @date  30.02.2012
 * @author  Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#ifndef TOUCHBUTTON_AUTOREPEAT_H_
#define TOUCHBUTTON_AUTOREPEAT_H_

#include "TouchButton.h"

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Button
 * @{
 */
#ifdef USE_BUTTON_POOL
// 4 are used in Page Setting
#define NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL 4
#endif

class TouchButtonAutorepeat: public TouchButton {
public:

// static because only one touch possible
    static unsigned long sMillisOfLastCall;
    static unsigned long sMillisSinceFirstTouch;
    static unsigned long sMillisSinceLastCallback;

#ifdef USE_BUTTON_POOL
    static TouchButtonAutorepeat * allocAutorepeatButton(void);
#endif

    static int checkAllButtons(int aTouchPositionX, int aTouchPositionY, bool doCallback);
    static void autorepeatTouchHandler(TouchButtonAutorepeat * aTheTouchedButton, int16_t aButtonValue);
    static bool autorepeatButtonTimerHandler(int aTouchPositionX, int aTouchPositionY);
    void setButtonAutorepeatTiming(uint16_t aMillisFirstDelay, uint16_t aMillisFirstRate, uint16_t aFirstCount,
            uint16_t aMillisSecondRate);

    /*
     * Static functions
     */
    TouchButtonAutorepeat();
#ifdef USE_BUTTON_POOL
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    static void reinitAllLocalAutorepeatButtonsForRemote(void);
#endif
#endif

    uint16_t mMillisFirstDelay;
    uint16_t mMillisFirstRate;
    uint16_t mFirstCount;
    uint16_t mMillisSecondRate;

private:
#ifdef USE_BUTTON_POOL
    static TouchButtonAutorepeat TouchButtonAutorepeatPool[NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL];
    static bool sButtonAutorepeatPoolIsInitialized; // used by allocButton
    static TouchButtonAutorepeat *sAutorepeatButtonListStart; // Root pointer to list of all buttons
#endif

    void (*mOnTouchHandlerAutorepeat)(TouchButton *, int16_t);
    static uint8_t sState;
    static uint16_t sCount;
    static TouchButtonAutorepeat * mLastAutorepeatButtonTouched; // For callback

};
/** @} */
/** @} */
#endif /* TOUCHBUTTON_AUTOREPEAT_H_ */
