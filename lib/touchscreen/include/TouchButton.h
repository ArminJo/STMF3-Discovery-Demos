/**
 * @file TouchButton.h
 *
 * Renders touch buttons for lcd
 * A button can be a simple clickable text
 * or a box with or without text
 *
 * @date  30.01.2012
 * @author  Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 * 	40 Bytes per button on 32Bit ARM
 * 	22 Bytes per button on Arduino
 */

#ifndef TOUCHBUTTON_H_
#define TOUCHBUTTON_H_

// should be globally set
//#define REMOTE_DISPLAY_SUPPORTED

#include "Colors.h"
#ifdef REMOTE_DISPLAY_SUPPORTED
typedef uint8_t BDButtonHandle_t;
class BDButton;
#endif

#include <stdint.h>
/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Button
 * @{
 */
#define LOCAL_BUTTON_FLAG_MASK 0x18
#define FLAG_USE_BDBUTTON_FOR_CALLBACK 0x08
#define FLAG_IS_ACTIVE 0x10 // Button is checked by checkAllButtons() etc.

// must be equal to declaration in BlueDisplay.h static const int FLAG_BUTTON_DO_BEEP_ON_TOUCH = 0x01;
#define FLAG_DO_BEEP_ON_TOUCH  0x01

static const int FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE = 0x40;

#ifdef USE_BUTTON_POOL
// 45 needed for AccuCap/settings/Numberpad
// 50 needed for DSO/settings/calibrate/Numberpad
#define NUMBER_OF_BUTTONS_IN_POOL 50
#endif

#define TOUCHBUTTON_DEFAULT_CAPTION_COLOR 	COLOR_BLACK
// Error codes
#define TOUCHBUTTON_ERROR_X_RIGHT 			-1
#define TOUCHBUTTON_ERROR_Y_BOTTOM 			-2
#define TOUCHBUTTON_ERROR_CAPTION_TOO_LONG	-3
#define TOUCHBUTTON_ERROR_CAPTION_TOO_HIGH	-4
#define TOUCHBUTTON_ERROR_NOT_INITIALIZED   -64

#define ERROR_STRING_X_RIGHT 			"X right > 320"
#define ERROR_STRING_Y_BOTTOM 			"Y bottom > 240"
#define ERROR_STRING_CAPTION_TOO_LONG	"Caption too long"
#define ERROR_STRING_CAPTION_TOO_HIGH	"Caption too high"
#define ERROR_STRING_NOT_INITIALIZED   -64

// return values for checkAllButtons()
#define NOT_TOUCHED false
#define BUTTON_TOUCHED 1
#define BUTTON_TOUCHED_AUTOREPEAT 2 // an autorepeat button was touched

class TouchButton {
public:

    TouchButton(void);

#ifdef REMOTE_DISPLAY_SUPPORTED
    ~TouchButton(void);
    static TouchButton * getLocalButtonFromBDButtonHandle(BDButtonHandle_t aButtonHandleToSearchFor);
    static void reinitAllLocalButtonsForRemote(void);
#endif

#ifdef USE_BUTTON_POOL
    // Pool stuff
    static TouchButton * allocButton(bool aOnlyAutorepeatButtons);
    static void freeButton(TouchButton * aTouchButton);
    static TouchButton * allocAndInitButton(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX,
            uint16_t aHeightY, uint16_t aButtonColor, const char *aCaption, uint8_t aCaptionSize, uint8_t aFlags,
            int16_t aValue, void (*aOnTouchHandler)(TouchButton*, int16_t));
    void setAllocated(bool isAllocated);
    void setFree(void);
    bool isAllocated(void) const;
    static void infoButtonPool(char * aStringBuffer);
#endif

    static void setDefaultTouchBorder(uint8_t aDefaultTouchBorder);
    static void setDefaultCaptionColor(uint16_t aDefaultCaptionColor);
    static uint8_t checkAllButtons(unsigned int aTouchPositionX, unsigned int aTouchPositionY);
    static void activateAllButtons(void);
    static void deactivateAllButtons(void);

    int8_t initButton(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, uint16_t aButtonColor,
            const char *aCaption, uint8_t aCaptionSize, uint8_t aFlags, int16_t aValue,
            void (*aOnTouchHandler)(TouchButton*, int16_t));

    bool checkButton(uint16_t aTouchPositionX, uint16_t aTouchPositionY);
    bool checkButtonInArea(uint16_t aTouchPositionX, uint16_t aTouchPositionY);

    bool isAutorepeatButton(void);

    void drawButton(void);
    void removeButton(uint16_t aBackgroundColor);
    void drawCaption(void);
    int8_t setPosition(uint16_t aPositionX, uint16_t aPositionY);
    void setButtonColor(uint16_t aButtonColor);
    void setButtonColorAndDraw(uint16_t aButtonColor);
    void setCaption(const char *aCaption);
    void setCaptionAndDraw(const char *aCaption);
    void setCaptionColor(uint16_t aColor);
    void setValue(int16_t aValue);
    void setValueAndDraw(int16_t aValue);
    int8_t setPositionX(uint16_t aPositionX);
    int8_t setPositionY(uint16_t aPositionY);

    const char *getCaption(void) const;
    uint16_t getValue() const;
    uint16_t getPositionX(void) const;
    uint16_t getPositionY(void) const;
    uint16_t getPositionXRight(void) const;
    uint16_t getPositionYBottom(void) const;

    void activate(void);
    void deactivate(void);
    void setTouchHandler(void (*aOnTouchHandler)(TouchButton*, int16_t));

#ifdef AVR
    void toString(char * aStringBuffer) const;
    void setCaptionPGM(const char * aCaption);
    uint8_t getCaptionLength(char * aCaptionPointer) const;
#endif

#ifdef REMOTE_DISPLAY_SUPPORTED
    BDButton * mBDButtonPtr;
#endif
    uint16_t mButtonColor;
    uint16_t mCaptionColor;
    uint16_t mPositionX; // of upper left corner
    uint16_t mPositionY; // of upper left corner
    uint16_t mWidthX;
    uint16_t mHeightY;
    uint8_t mCaptionSize;
    uint8_t mFlags; //Flag for: Autorepeat type, allocated, only caption, Red/Green button

    const char *mCaption; // Pointer to caption
    int16_t mValue;
    void (*mOnTouchHandler)(TouchButton *, int16_t);

#ifdef USE_BUTTON_POOL
private:
    // Pool stuff
    static TouchButton TouchButtonPool[NUMBER_OF_BUTTONS_IN_POOL];
    static bool sButtonPoolIsInitialized;// used by allocButton
#endif

protected:
    static uint16_t sDefaultCaptionColor;

    static uint8_t sButtonCombinedPoolSize;
    static uint8_t sMinButtonPoolSize;

    static TouchButton *sButtonListStart; // Root pointer to list of all buttons

    TouchButton *mNextObject;

};

// Handler for red green button
void doToggleRedGreenButton(TouchButton * aTheTouchedButton, int16_t aValue);

#ifdef AVR
void FeedbackToneOK(void);
#endif

/** @} */
/** @} */

#endif /* TOUCHBUTTON_H_ */
