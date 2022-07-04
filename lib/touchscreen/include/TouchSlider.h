/**
 *
 * @file TouchSlider.h
 *
 * @date 31.01.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 * Size of one slider is 46 bytes on Arduino
 *
 */

#ifndef TOUCHSLIDER_H_
#define TOUCHSLIDER_H_

#if defined(USE_HY32D)
#include "SSD1289.h"
#else
#include "MI0283QT2.h"
#endif

#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
#include "BDSlider.h"
#  if defined(AVR)
typedef uint8_t BDSliderHandle_t;
#  else
typedef uint16_t BDSliderHandle_t;
#  endif
class BDSlider;
#endif

#include <stdint.h>

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Slider
 * @{
 */
/**
 * @name SliderOptions
 * Options for TouchSlider::initSlider() - they can be or-ed together
 * @{
 */

#define FLAG_USE_BDSLIDER_FOR_CALLBACK 0x10 // Use pointer to index in button pool instead of pointer to this in callback

// This definitions must match the SLIDER_ definitions contained in BlueDisplay.h
#define TOUCHSLIDER_VERTICAL_SHOW_NOTHING 0x00
#define TOUCHFLAG_SLIDER_SHOW_BORDER 0x01
#define TOUCHSLIDER_SHOW_VALUE 0x02
#define TOUCHFLAG_SLIDER_IS_HORIZONTAL 0x04
#define TOUCHSLIDER_HORIZONTAL_VALUE_BELOW_TITLE 0x08
#define TOUCHFLAG_SLIDER_VALUE_BY_CALLBACK 0x10

/** @} */

#define SLIDER_DEFAULT_SLIDER_COLOR         RGB( 180, 180, 180)
#ifndef SLIDER_DEFAULT_BAR_COLOR
#define SLIDER_DEFAULT_BAR_COLOR            COLOR_GREEN
#endif
#define SLIDER_DEFAULT_BAR_THRESHOLD_COLOR  COLOR_RED
#define SLIDER_DEFAULT_BAR_BACK_COLOR       COLOR_WHITE
#ifndef SLIDER_DEFAULT_CAPTION_COLOR
#define SLIDER_DEFAULT_CAPTION_COLOR        COLOR_BLACK
#endif
#define SLIDER_DEFAULT_VALUE_COLOR          COLOR_BLUE
#define SLIDER_DEFAULT_CAPTION_VALUE_BACK_COLOR  COLOR_NO_BACKGROUND
#define SLIDER_DEFAULT_BAR_WIDTH            8
#define SLIDER_MAX_BAR_WIDTH                40 /** global max value of size parameter */
#define SLIDER_DEFAULT_TOUCH_BORDER         4 /** extension of touch region in pixel */
#define SLIDER_DEFAULT_SHOW_CAPTION         true
#define SLIDER_DEFAULT_SHOW_VALUE           true
#define SLIDER_DEFAULT_MAX_VALUE            160
#define SLIDER_DEFAULT_THRESHOLD_VALUE      100

#define SLIDER_MAX_DISPLAY_VALUE            LOCAL_DISPLAY_WIDTH //! maximum value which can be displayed
// return values for checkAllSliders()
#ifndef NOT_TOUCHED
#define NOT_TOUCHED false
#endif
#define SLIDER_TOUCHED 4
/**
 * @name SliderErrorCodes
 * @{
 */
#define SLIDER_ERROR_ACTUAL_VALUE     -1
/** @} */

// Typedef in order to save program space on AVR, but allow bigger values on other platforms
#ifdef AVR
typedef uint8_t uintForPgmSpaceSaving;
typedef uint8_t uintForRamSpaceSaving;
#else
typedef unsigned int uintForPgmSpaceSaving;
typedef uint16_t uintForRamSpaceSaving;
#endif

class TouchSlider {
public:
    /*
     * Static functions
     */
    TouchSlider(void);

#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    ~TouchSlider(void);
    static TouchSlider* getLocalSliderFromBDSliderHandle(BDSliderHandle_t aSliderHandleToSearchFor);
    static void reinitAllLocalSlidersForRemote(void);
#endif

    static void setDefaults(uintForPgmSpaceSaving aDefaultTouchBorder, uint16_t aDefaultSliderColor, uint16_t aDefaultBarColor,
            uint16_t aDefaultBarThresholdColor, uint16_t aDefaultBarBackgroundColor, uint16_t aDefaultCaptionColor,
            uint16_t aDefaultValueColor, uint16_t aDefaultValueCaptionBackgroundColor);
    static void setDefaultSliderColor(uint16_t aDefaultSliderColor);
    static void setDefaultBarColor(uint16_t aDefaultBarColor);
    static bool checkAllSliders(unsigned int aTouchPositionX, unsigned int aTouchPositionY);
    static void deactivateAllSliders(void);
    static void activateAllSliders(void);

    /*
     * Member functions
     */
    void initSlider(uint16_t aPositionX, uint16_t aPositionY, uintForPgmSpaceSaving aBarWidth, uint16_t aBarLength,
            uint16_t aThresholdValue, uint16_t aInitalValue, const char *aCaption, int8_t aTouchBorder, uint8_t aOptions,
            void (*aOnChangeHandler)(TouchSlider*, uint16_t), const char* (*aValueHandler)(uint16_t));
    // Used for BDSlider
    void initSlider(uint16_t aPositionX, uint16_t aPositionY, uint8_t aBarWidth, uint16_t aBarLength, uint16_t aThresholdValue,
            int16_t aInitalValue, uint16_t aSliderColor, uint16_t aBarColor, uint8_t aOptions,
            void (*aOnChangeHandler)(TouchSlider*, uint16_t));

    void initSliderColors(uint16_t aSliderColor, uint16_t aBarColor, uint16_t aBarThresholdColor, uint16_t aBarBackgroundColor,
            uint16_t aCaptionColor, uint16_t aValueColor, uint16_t aValueCaptionBackgroundColor);

    void drawSlider(void);
    bool checkSlider(uint16_t aPositionX, uint16_t aPositionY);
    void drawBar(void);
    void drawBorder(void);

    void activate(void);
    void deactivate(void);

    void setPosition(int16_t aPositionX, int16_t aPositionY);
    uint16_t getPositionXRight(void) const;
    uint16_t getPositionYBottom(void) const;

    void setBarBackgroundColor(uint16_t aBarBackgroundColor);
    void setValueAndCaptionBackgroundColor(uint16_t aValueCaptionBackgroundColor);
    void setValueColor(uint16_t aValueColor);
    void setCaptionColors(uint16_t aCaptionColor, uint16_t aValueCaptionBackgroundColor);
    void setValueStringColors(uint16_t aValueStringColor, uint16_t aValueStringCaptionBackgroundColor);
    void setSliderColor(uint16_t sliderColor);
    void setBarColor(uint16_t barColor);
    void setBarThresholdColor(uint16_t barThresholdColor);
    uint16_t getBarColor(void) const;

    void setValue(int16_t aCurrentValue);
    void setValueAndDraw(int16_t aCurrentValue);
    void setValueAndDrawBar(int16_t aCurrentValue);
    int16_t getCurrentValue(void) const;

    void setCaption(const char *aCaption);
    void printCaption(void);
    int printValue(void);
    int printValue(const char *aValueString);
    void setXOffsetValue(int16_t aXOffsetValue);

#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    BDSlider *mBDSliderPtr;
#endif

private:
    // Start of list of touch slider
    static TouchSlider *sSliderListStart;
    /*
     * Defaults
     */
    static uint16_t sDefaultSliderColor;
    static uint16_t sDefaultBarColor;
    static uint16_t sDefaultBarThresholdColor;
    static uint16_t sDefaultBarBackgroundColor;
    static uint16_t sDefaultCaptionColor;
    static uint16_t sDefaultValueColor;
    static uint16_t sDefaultValueCaptionBackgroundColor;
    static uint8_t sDefaultTouchBorder;

    /*
     * The Slider
     */
    uint16_t mPositionX; // X left
    uint16_t mPositionXRight;
    uint16_t mXOffsetValue;
    uint16_t mPositionY; // Y top
    uint16_t mPositionYBottom;
    uint16_t mBarLength; //aMaxValue serves also as height
    uint16_t mThresholdValue; // Value for color change
    uintForRamSpaceSaving mBarWidth; // Size of border and bar
    const char *mCaption; // No caption if NULL
    uint8_t mTouchBorder; // extension of touch region
    uint8_t mFlags;

    // the value as provided by touch
    uint16_t mActualTouchValue;
    // This value can be different from mActualTouchValue and is provided by callback handler
    uint16_t mActualValue;

    // Colors
    uint16_t mSliderColor;
    uint16_t mBarColor;
    uint16_t mBarThresholdColor;
    uint16_t mBarBackgroundColor;
    uint16_t mCaptionColor;
    uint16_t mValueColor;
    uint16_t mValueCaptionBackgroundColor;

    bool mIsActive;

    // misc
    void (*mOnChangeHandler)(TouchSlider*, uint16_t);
    TouchSlider *mNextObject;
    const char* (*mValueHandler)(uint16_t); // provides the string to print

    int8_t checkParameterValues();

};
/** @} */
/** @} */

#endif /* TOUCHSLIDER_H_ */
