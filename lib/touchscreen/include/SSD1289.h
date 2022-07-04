/*
 * SSD1289.h
 *
 * @date  14.02.2012
 * @author  Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 */

#ifndef SSD1289_h
#define SSD1289_h

#include "fonts.h"
#include <stdbool.h>

/** @addtogroup Graphic_Library
 * @{
 */
/** @addtogroup HY32D_basic
 * @{
 */

// Landscape format
#define LOCAL_DISPLAY_HEIGHT    240
#define LOCAL_DISPLAY_WIDTH     320

/*
 * Backlight values in percent
 */
#define BACKLIGHT_START_VALUE 50
#define BACKLIGHT_MAX_VALUE 100
#define BACKLIGHT_MIN_VALUE 0
#define BACKLIGHT_DIM_VALUE 7
#define BACKLIGHT_DIM_DEFAULT_DELAY TWO_MINUTES_MILLIS

#ifdef __cplusplus
class SSD1289 {

public:
    SSD1289();
    void init(void);

    void clearDisplay(uint16_t color);
    void drawPixel(uint16_t aXPos, uint16_t aYPos, uint16_t aColor);
    void drawCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, uint16_t aColor);
    void fillCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, uint16_t aColor);
    void fillRect(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor);
    void fillRectRel(uint16_t aXStart, uint16_t aYStart, uint16_t aWidth, uint16_t aHeight, uint16_t aColor);
    uint16_t drawChar(uint16_t aPosX, uint16_t aPosY, char aChar, uint8_t aCharSize, uint16_t aFGColor, uint16_t aBGColor);
    uint16_t drawText(uint16_t aXStart, uint16_t aYStart, const char *aStringPtr, uint8_t aSize, uint16_t aColor, uint16_t aBGColor);
    uint16_t drawNCharacters(uint16_t aXStart, uint16_t aYStart, const char *aStringPtr, uint8_t aSize, uint16_t aColor,
            uint16_t aBGColor, int aLength);
    void drawTextVertical(uint16_t aXPos, uint16_t aYPos, const char *aStringPointer, uint8_t aSize, uint16_t aColor,
            uint16_t aBackgroundColor);
    void drawLine(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor);
    void drawLineFastOneX(uint16_t x0, uint16_t y0, uint16_t y1, uint16_t color);
    void drawRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
    uint16_t drawMLText(uint16_t aPosX, uint16_t aPosY, const char *aStringPtr, uint8_t aTextSize, uint16_t aColor,
            uint16_t aBGColor);

private:

};

// The instance provided by the class itself
extern SSD1289 LocalDisplay;
#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

extern bool isInitializedSSD1289;
extern volatile uint32_t sDrawLock;

void setDimDelayMillis(int32_t aTimeMillis);
void resetBacklightTimeout(void);
int8_t getBacklightValue(void);
void setBacklightValue(int aBacklightValue);
void callbackLCDDimming(void);
int clipBrightnessValue(int aBrightnessValue);

uint16_t getWidth(void);
uint16_t getHeight(void);

void setArea(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

void setCursor(uint16_t aXStart, uint16_t aYStart);

int drawNText(uint16_t x, uint16_t y, const char *s, int aNumberOfCharacters, uint8_t size, uint16_t color, uint16_t bg_color);

uint16_t drawInteger(uint16_t x, uint16_t y, int val, uint8_t base, uint8_t size, uint16_t color, uint16_t bg_color);

uint16_t readPixel(uint16_t aXPos, uint16_t aYPos);
void storeScreenshot(void);
/*
 * fast divide by 11 for SSD1289 driver arguments
 */
uint16_t getLocalTextSize(uint16_t aTextSize);

#ifdef __cplusplus
}
#endif

// Tests
void initalizeDisplay2(void);
void setGamma(int aIndex);
void writeCommand(int aRegisterAddress, int aRegisterValue);

/** @} */
/** @} */

#endif //SSD1289_h
