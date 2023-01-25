/*
 * LocalTouchButton.h
 *
 * Renders touch buttons for lcd
 * A button can be a simple clickable text
 * or a box with or without text
 *
 * 40 Bytes per button on 32Bit ARM
 * 22 Bytes per button on Arduino
 *
 *  Copyright (C) 2012-2023  Armin Joachimsmeyer
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
 *
 */

#ifndef _LOCAL_TOUCH_BUTTON_H
#define _LOCAL_TOUCH_BUTTON_H

#if !defined(DISABLE_REMOTE_DISPLAY)
class BDButton;

#  if defined(AVR)
typedef uint8_t BDButtonHandle_t;
#  else
typedef uint16_t BDButtonHandle_t;
#  endif
#endif

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Button
 * @{
 */

#define TOUCHBUTTON_DEFAULT_CAPTION_COLOR       COLOR16_BLACK
// Error codes
#define TOUCHBUTTON_ERROR_X_RIGHT               -1
#define TOUCHBUTTON_ERROR_Y_BOTTOM              -2
// for documentation
#define ERROR_STRING_X_RIGHT                    "X right > LOCAL_DISPLAY_WIDTH"
#define ERROR_STRING_Y_BOTTOM                   "Y bottom > LOCAL_DISPLAY_HEIGHT"

// return values for checkAllButtons()
#define NO_BUTTON_TOUCHED           false
#define BUTTON_TOUCHED              true

// A copy from BDButton.h for documentation
//static const int FLAG_BUTTON_NO_BEEP_ON_TOUCH = 0x00;
//static const int FLAG_BUTTON_DO_BEEP_ON_TOUCH = 0x01;  // Beep on this button touch
//static const int FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN = 0x02;  // Value true -> green, false -> red. Value toggling is generated by button.
//static const int FLAG_BUTTON_TYPE_AUTOREPEAT = 0x04;
//static const int FLAG_BUTTON_TYPE_MANUAL_REFRESH = 0x08;    // Button must be manually drawn after event. Currently only for Red/Green button
//static const int FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN_MANUAL_REFRESH = 0x0A; // Red/Green button must be manually drawn after event to show new caption/color
//// Flags for local buttons
//#define LOCAL_BUTTON_FLAG_IS_ACTIVE                     0x10 // Local button is to be checked by checkAllButtons() etc.
//#define LOCAL_BUTTON_FLAG_USE_BDBUTTON_FOR_CALLBACK     0x20 // Here we have a local and a remote button. Use only the remote button pointer as callback parameter.
//#define LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE 0x40
//#define LOCAL_BUTTON_FLAG_MASK                          0x70

class TouchButton {
public:

    TouchButton();

#if !defined(ARDUINO)
    ~TouchButton();
#endif

#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
    static TouchButton * getLocalButtonFromBDButtonHandle(BDButtonHandle_t aButtonHandleToSearchFor);
    static void createAllLocalButtonsAtRemote();
#endif
    static void setDefaultTouchBorder(uint8_t aDefaultTouchBorder);
    static void setDefaultCaptionColor(uint16_t aDefaultCaptionColor);
    static uint8_t checkAllButtons(unsigned int aTouchPositionX, unsigned int aTouchPositionY, bool aCheckOnlyAutorepeatButtons);
    static void activateAllButtons();
    static void deactivateAllButtons();

    int8_t initButton(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, uint16_t aButtonColor,
            const char *aCaption, uint8_t aCaptionSize, uint8_t aFlags, int16_t aValue,
            void (*aOnTouchHandler)(TouchButton*, int16_t));

    bool checkButton(uint16_t aTouchPositionX, uint16_t aTouchPositionY, bool aCheckOnlyAutorepeatButtons);
    bool checkButtonInArea(uint16_t aTouchPositionX, uint16_t aTouchPositionY);

    bool isAutorepeatButton();

    void drawButton();
    void removeButton(uint16_t aBackgroundColor);
    void drawCaption();
    int8_t setPosition(uint16_t aPositionX, uint16_t aPositionY);
    void setButtonColor(uint16_t aButtonColor);
    void setButtonColorAndDraw(uint16_t aButtonColor);
    void setCaptionForValueTrue(const char *aCaption);
    void setCaption(const char *aCaption, bool doDrawButton = false);
    void setCaptionAndDraw(const char *aCaption);
    void setCaptionColor(uint16_t aCaptionColor);
    void setValue(int16_t aValue);
    void setColorForRedGreenButton(bool aValue);
    void setValueAndDraw(int16_t aValue);
    int8_t setPositionX(uint16_t aPositionX);
    int8_t setPositionY(uint16_t aPositionY);

    const char* getCaption() const;
    uint16_t getValue() const;
    uint16_t getPositionX() const;
    uint16_t getPositionY() const;
    uint16_t getPositionXRight() const;
    uint16_t getPositionYBottom() const;

    void activate();
    void deactivate();
    void setTouchHandler(void (*aOnTouchHandler)(TouchButton*, int16_t));

#ifdef AVR
    void setCaptionPGM(const char *aCaption);
    void setCaptionPGMForValueTrue(const char *aCaption);
    uint8_t getCaptionLength(char *aCaptionPointer);
#  if defined(DEBUG)
    void toString(char *aStringBuffer) const;
#  endif
#endif

#if !defined(DISABLE_REMOTE_DISPLAY)
    TouchButton(BDButton *aBDButtonPtr); // Required for creating a local button for an existing aBDButton at BDButton::init
    BDButton *mBDButtonPtr;
#endif
    uint16_t mButtonColor;
    uint16_t mCaptionColor;
    uint16_t mPositionX; // of upper left corner
    uint16_t mPositionY; // of upper left corner
    uint16_t mWidthX;
    uint16_t mHeightY;
    uint8_t mCaptionSize;
    uint8_t mFlags; //Flag for: Autorepeat type, allocated, only caption, Red/Green button. For definition, see BDButton.h

    const char *mCaption; // Pointer to caption
    const char *mCaptionForTrue; // Pointer to caption for true for red green buttons
    int16_t mValue;
    void (*mOnTouchHandler)(TouchButton*, int16_t);

    static uint16_t sDefaultCaptionColor;

    /*
     * Button list, required for the *AllButtons functions
     */
    static TouchButton *sButtonListStart; // Start of list of touch buttons, required for the *AllButtons functions
    TouchButton *mNextObject;

};

// Handler for red green button
void doToggleRedGreenButton(TouchButton *aTheTouchedButton, int16_t aValue);

void playLocalFeedbackTone();

/** @} */
/** @} */

#endif // _LOCAL_TOUCH_BUTTON_H
