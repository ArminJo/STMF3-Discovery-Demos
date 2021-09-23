/**
 * @file SSD1289.cpp
 *
 * @date 05.12.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 *      based on SSD1289.cpp with license
 *      https://github.com/watterott/mSD-Shield/blob/master/src/license.txt
 */

#include "SSD1289.h"
#include "BlueDisplay.h"
#include "thickline.h"
#include "main.h" // for StringBuffer
#include "STM32TouchScreenDriver.h"
#include "timing.h"
#include "stm32fx0xPeripherals.h"

#include <stdio.h> // for sprintf
#include <string.h>  // for strcat
#include <stdlib.h>  // for malloc

extern "C" {
#include "ff.h"
#include "BlueSerial.h"
}

/** @addtogroup Graphic_Library
 * @{
 */
/** @addtogroup SSD1289_basic
 * @{
 */
#define LCD_GRAM_WRITE_REGISTER    0x22

bool isInitializedSSD1289 = false;
volatile uint32_t sDrawLock = 0;
/*
 * For automatic LCD dimming
 */
int LCDBacklightValue = BACKLIGHT_START_VALUE;
int LCDLastBacklightValue; //! for state of backlight before dimming
int LCDDimDelay; //actual dim delay

//-------------------- Private functions --------------------
void drawStart(void);
inline void draw(uint16_t color);
inline void drawStop(void);
void writeCommand(int aRegisterAddress, int aRegisterValue);
bool initalizeDisplay(void);
uint16_t * fillDisplayLineBuffer(uint16_t * aBufferPtr, uint16_t yLineNumber);
void setBrightness(int power); //0-100

//-------------------- Constructor --------------------

SSD1289::SSD1289(void) {
    return;
}

// One instance of SSD1289 called LocalDisplay
SSD1289 LocalDisplay;
//-------------------- Public --------------------
void SSD1289::init(void) {
//init pins
    SSD1289_IO_initalize();
// init PWM for background LED
    PWM_BL_initalize();
    setBrightness(BACKLIGHT_START_VALUE);

// deactivate read output control
    HY32D_RD_GPIO_PORT->BSRR = HY32D_RD_PIN;
//initalize display
    if (initalizeDisplay()) {
        isLocalDisplayAvailable = true;
    }
}

void setArea(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd) {
    if ((aXEnd >= LOCAL_DISPLAY_WIDTH) || (aYEnd >= LOCAL_DISPLAY_HEIGHT)) {
        assertFailedParamMessage((uint8_t *) __FILE__, __LINE__, aXEnd, aYEnd, "");
    }

    writeCommand(0x44, aYStart + (aYEnd << 8)); //set ystart, yend
    writeCommand(0x45, aXStart); //set xStart
    writeCommand(0x46, aXEnd); //set xEnd
// also set cursor to right start position
    writeCommand(0x4E, aYStart);
    writeCommand(0x4F, aXStart);
}

void setCursor(uint16_t aXStart, uint16_t aYStart) {
    writeCommand(0x4E, aYStart);
    writeCommand(0x4F, aXStart);
}

void SSD1289::clearDisplay(uint16_t aColor) {
    unsigned int size;
    setArea(0, 0, LOCAL_DISPLAY_WIDTH - 1, LOCAL_DISPLAY_HEIGHT - 1);

    drawStart();
    for (size = (LOCAL_DISPLAY_HEIGHT * LOCAL_DISPLAY_WIDTH); size != 0; size--) {
        HY32D_DATA_GPIO_PORT->ODR = aColor;
        // Latch data write
        HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_WR_PIN << 16;
        HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;

    }
    HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;

}

/**
 * set register address to LCD_GRAM_READ/WRITE_REGISTER
 */
void drawStart(void) {
// CS enable (low)
    HY32D_CS_GPIO_PORT->BSRR = (uint32_t) HY32D_CS_PIN << 16;
// Control enable (low)
    HY32D_DATA_CONTROL_GPIO_PORT->BSRR = (uint32_t) HY32D_DATA_CONTROL_PIN << 16;
// set value
    HY32D_DATA_GPIO_PORT->ODR = LCD_GRAM_WRITE_REGISTER;
// Latch data write
    HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_WR_PIN << 16;
    HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;
// Data enable (high)
    HY32D_DATA_CONTROL_GPIO_PORT->BSRR = HY32D_DATA_CONTROL_PIN;
}

inline void draw(uint16_t color) {
// set value
    HY32D_DATA_GPIO_PORT->ODR = color;
// Latch data write
    HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_WR_PIN << 16;
    HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;
}

void drawStop(void) {
    HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;
}

void SSD1289::drawPixel(uint16_t aXPos, uint16_t aYPos, uint16_t aColor) {
    if ((aXPos >= LOCAL_DISPLAY_WIDTH) || (aYPos >= LOCAL_DISPLAY_HEIGHT)) {
        return;
    }

// setCursor
    writeCommand(0x4E, aYPos);
    writeCommand(0x4F, aXPos);

    drawStart();
    draw(aColor);
    drawStop();
}

void SSD1289::drawLine(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor) {
    drawLineOverlap(aXStart, aYStart, aXEnd, aYEnd, LINE_OVERLAP_NONE, aColor);
}

void SSD1289::fillRect(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor) {
    uint32_t size;
    uint16_t tmp, i;

    if (aXStart > aXEnd) {
        tmp = aXStart;
        aXStart = aXEnd;
        aXEnd = tmp;
    }
    if (aYStart > aYEnd) {
        tmp = aYStart;
        aYStart = aYEnd;
        aYEnd = tmp;
    }

    if (aXEnd >= LOCAL_DISPLAY_WIDTH) {
        aXEnd = LOCAL_DISPLAY_WIDTH - 1;
    }
    if (aYEnd >= LOCAL_DISPLAY_HEIGHT) {
        aYEnd = LOCAL_DISPLAY_HEIGHT - 1;
    }

    setArea(aXStart, aYStart, aXEnd, aYEnd);

    drawStart();
    size = (uint32_t) (1 + (aXEnd - aXStart)) * (uint32_t) (1 + (aYEnd - aYStart));
    tmp = size / 8;
    if (tmp != 0) {
        for (i = tmp; i != 0; i--) {
            draw(aColor); //1
            draw(aColor); //2
            draw(aColor); //3
            draw(aColor); //4
            draw(aColor); //5
            draw(aColor); //6
            draw(aColor); //7
            draw(aColor); //8
        }
        for (i = size - tmp; i != 0; i--) {
            draw(aColor);
        }
    } else {
        for (i = size; i != 0; i--) {
            draw(aColor);
        }
    }
    drawStop();
}

void SSD1289::fillRectRel(uint16_t aXStart, uint16_t aYStart, uint16_t aWidth, uint16_t aHeight, color16_t aColor) {
    LocalDisplay.fillRect(aXStart, aYStart, aXStart + aWidth - 1, aYStart + aHeight - 1, aColor);
}

void SSD1289::drawCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, uint16_t aColor) {
    int16_t err, x, y;

    err = -aRadius;
    x = aRadius;
    y = 0;

    while (x >= y) {
        drawPixel(aXCenter + x, aYCenter + y, aColor);
        drawPixel(aXCenter - x, aYCenter + y, aColor);
        drawPixel(aXCenter + x, aYCenter - y, aColor);
        drawPixel(aXCenter - x, aYCenter - y, aColor);
        drawPixel(aXCenter + y, aYCenter + x, aColor);
        drawPixel(aXCenter - y, aYCenter + x, aColor);
        drawPixel(aXCenter + y, aYCenter - x, aColor);
        drawPixel(aXCenter - y, aYCenter - x, aColor);

        err += y;
        y++;
        err += y;
        if (err >= 0) {
            x--;
            err -= x;
            err -= x;
        }
    }
}

void SSD1289::fillCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, uint16_t aColor) {
    int16_t err, x, y;

    err = -aRadius;
    x = aRadius;
    y = 0;

    while (x >= y) {
        drawLine(aXCenter - x, aYCenter + y, aXCenter + x, aYCenter + y, aColor);
        drawLine(aXCenter - x, aYCenter - y, aXCenter + x, aYCenter - y, aColor);
        drawLine(aXCenter - y, aYCenter + x, aXCenter + y, aYCenter + x, aColor);
        drawLine(aXCenter - y, aYCenter - x, aXCenter + y, aYCenter - x, aColor);

        err += y;
        y++;
        err += y;
        if (err >= 0) {
            x--;
            err -= x;
            err -= x;
        }
    }
}

uint16_t readPixel(uint16_t aXPos, uint16_t aYPos) {
    if ((aXPos >= LOCAL_DISPLAY_WIDTH) || (aYPos >= LOCAL_DISPLAY_HEIGHT)) {
        return 0;
    }

// setCursor
    writeCommand(0x4E, aYPos);
    writeCommand(0x4F, aXPos);

    drawStart();
    uint16_t tValue = 0;
// set port pins to input
    HY32D_DATA_GPIO_PORT->MODER = 0x00000000;
// Latch data read
    HY32D_WR_GPIO_PORT->BRR = HY32D_RD_PIN;
// wait >250ns
    delayNanos(300);

    tValue = HY32D_DATA_GPIO_PORT->IDR;
    HY32D_WR_GPIO_PORT->BSRR = HY32D_RD_PIN;
// set port pins to output
    HY32D_DATA_GPIO_PORT->MODER = 0x55555555;
    HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;

    return tValue;
}

void SSD1289::drawRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    fillRect(x0, y0, x0, y1, color);
    fillRect(x0, y1, x1, y1, color);
    fillRect(x1, y0, x1, y1, color);
    fillRect(x0, y0, x1, y0, color);
}

/*
 * Fast routine for drawing data charts
 * draws a line only from x to x+1
 * first pixel is omitted because it is drawn by preceding line
 * uses setArea instead if drawPixel to speed up drawing
 */
void SSD1289::drawLineFastOneX(uint16_t aXStart, uint16_t aYStart, uint16_t aYEnd, uint16_t aColor) {
    uint8_t i;
    bool up = true;
//calculate direction
    int16_t deltaY = aYEnd - aYStart;
    if (deltaY < 0) {
        deltaY = -deltaY;
        up = false;
    }
    if (deltaY <= 1) {
        // constant y or one pixel offset => no line needed
        LocalDisplay.drawPixel(aXStart + 1, aYEnd, aColor);
    } else {
        // draw line here
        // deltaY1 is == deltaYHalf for even numbers and deltaYHalf -1 for odd Numbers
        uint8_t deltaY1 = (deltaY - 1) >> 1;
        uint8_t deltaYHalf = deltaY >> 1;
        if (up) {
            // for odd numbers, first part of line is 1 pixel shorter than second
            if (deltaY1 > 0) {
                // first pixel was drawn by preceding line :-)
                setArea(aXStart, aYStart + 1, aXStart, aYStart + deltaY1);
                drawStart();
                for (i = deltaY1; i != 0; i--) {
                    draw(aColor);
                }
                drawStop();
            }
            setArea(aXStart + 1, aYStart + deltaY1 + 1, aXStart + 1, aYEnd);
            drawStart();
            for (i = deltaYHalf + 1; i != 0; i--) {
                draw(aColor);
            }
            drawStop();
        } else {
            // for odd numbers, second part of line is 1 pixel shorter than first
            if (deltaYHalf > 0) {
                setArea(aXStart, aYStart - deltaYHalf, aXStart, aYStart - 1);
                drawStart();
                for (i = deltaYHalf; i != 0; i--) {
                    draw(aColor);
                }
                drawStop();
            }
            setArea(aXStart + 1, aYEnd, aXStart + 1, (aYStart - deltaYHalf) - 1);
            drawStart();
            for (i = deltaY1 + 1; i != 0; i--) {
                draw(aColor);
            }
            drawStop();
        }
    }
}

/**
 * @param bg_color start x for next character / x + (FONT_WIDTH * size)
 * @return
 */
uint16_t SSD1289::drawChar(uint16_t x, uint16_t y, char c, uint8_t size, uint16_t color, uint16_t bg_color) {
    /*
     * check if a draw in routine which uses setArea() is already executed
     */
    uint32_t tLock;
    do {
        tLock = __LDREXW(&sDrawLock);
        tLock++;
    } while (__STREXW(tLock, &sDrawLock));

    if (tLock != 1) {
        // here in ISR, but interrupted process was still in drawChar()
        sLockCount++;
        // first approach skip drawing and return input x value
        return x;
    }
    uint16_t tRetValue;
#if FONT_WIDTH <= 8
    uint8_t data, mask;
#elif FONT_WIDTH <= 16
    uint16_t data, mask;
#elif FONT_WIDTH <= 32
    uint32_t data, mask;
#endif
    uint8_t i, j, width, height;
    const uint8_t *ptr;
// characters below 20 are not printable
    if (c < 0x20) {
        c = 0x20;
    }
    i = (uint8_t) c;
#if FONT_WIDTH <= 8
    ptr = &font[(i - FONT_START) * (8 * FONT_HEIGHT / 8)];
#elif FONT_WIDTH <= 16
    ptr = &font_PGM[(i-FONT_START)*(16*FONT_HEIGHT/8)];
#elif FONT_WIDTH <= 32
    ptr = &font_PGM[(i-FONT_START)*(32*FONT_HEIGHT/8)];
#endif
    width = FONT_WIDTH;
    height = FONT_HEIGHT;

    if (size <= 1) {
        tRetValue = x + width;
        if ((y + height) > LOCAL_DISPLAY_HEIGHT) {
            tRetValue = LOCAL_DISPLAY_WIDTH + 1;
        }
        if (tRetValue <= LOCAL_DISPLAY_WIDTH) {
            setArea(x, y, (x + width - 1), (y + height - 1));
            drawStart();
            for (; height != 0; height--) {
#if FONT_WIDTH <= 8
                data = *ptr;
                ptr += 1;
#elif FONT_WIDTH <= 16
                data = read_word(ptr); ptr+=2;
#elif FONT_WIDTH <= 32
                data = read_dword(ptr); ptr+=4;
#endif
                for (mask = (1 << (width - 1)); mask != 0; mask >>= 1) {
                    if (data & mask) {
                        draw(color);
                    } else {
                        draw(bg_color);
                    }
                }
            }
            drawStop();
        }
    } else {
        tRetValue = x + (width * size);
        if ((y + (height * size)) > LOCAL_DISPLAY_HEIGHT) {
            tRetValue = LOCAL_DISPLAY_WIDTH + 1;
        }
        if (tRetValue <= LOCAL_DISPLAY_WIDTH) {
            setArea(x, y, (x + (width * size) - 1), (y + (height * size) - 1));
            drawStart();
            for (; height != 0; height--) {
#if FONT_WIDTH <= 8
                data = *ptr;
                ptr += 1;
#elif FONT_WIDTH <= 16
                data = pgm_read_word(ptr); ptr+=2;
#elif FONT_WIDTH <= 32
                data = pgm_read_dword(ptr); ptr+=4;
#endif
                for (i = size; i != 0; i--) {
                    for (mask = (1 << (width - 1)); mask != 0; mask >>= 1) {
                        if (data & mask) {
                            for (j = size; j != 0; j--) {
                                draw(color);
                            }
                        } else {
                            for (j = size; j != 0; j--) {
                                draw(bg_color);
                            }
                        }
                    }
                }
            }
            drawStop();
        }
    }
    sDrawLock = 0;
    return tRetValue;
}

/**
 * draw aNumberOfCharacters from string and clip at display border
 * @return uint16_t start x for next character - next x Parameter
 */
int drawNText(uint16_t x, uint16_t y, const char *s, int aNumberOfCharacters, uint8_t size, uint16_t color, uint16_t bg_color) {
    while (*s != 0 && --aNumberOfCharacters > 0) {
        x = LocalDisplay.drawChar(x, y, (char) *s++, size, color, bg_color);
        if (x > LOCAL_DISPLAY_WIDTH) {
            break;
        }
    }
    return x;
}

/**
 *
 * @param x left position
 * @param y upper position
 * @param s String
 * @param size Font size
 * @param color
 * @param bg_color
 * @return uint16_t start x for next character - next x Parameter
 */
uint16_t SSD1289::drawText(uint16_t aXStart, uint16_t aYStart, const char *aStringPtr, uint8_t aSize, uint16_t aColor,
        uint16_t aBGColor) {
    uint16_t tLength = 0;
    const char *tStringPtr = aStringPtr;
    uint16_t tXPos = aXStart;
    while (*tStringPtr != 0) {
        tXPos = drawChar(tXPos, aYStart, (char) *tStringPtr++, aSize, aColor, aBGColor);
        tLength++;
        if (tXPos > LOCAL_DISPLAY_WIDTH) {
            break;
        }
    }
    return tXPos;
}

uint16_t SSD1289::drawNCharacters(uint16_t aXStart, uint16_t aYStart, const char *aStringPtr, uint8_t aSize, uint16_t aColor,
        uint16_t aBGColor, int aLength) {
    uint16_t tLength = 0;
    const char *tStringPtr = aStringPtr;
    uint16_t tXPos = aXStart;
    while (aLength-- != 0) {
        tXPos = drawChar(tXPos, aYStart, (char) *tStringPtr++, aSize, aColor, aBGColor);
        tLength++;
        if (tXPos > LOCAL_DISPLAY_WIDTH) {
            break;
        }
    }
    return tXPos;
}
/**
 *
 * @param aXPos
 * @param aYPos
 * @param aStringPointer
 * @param aSize
 * @param aColor
 * @param aBackgroundColor
 */
void SSD1289::drawTextVertical(uint16_t aXPos, uint16_t aYPos, const char *aStringPointer, uint8_t aSize, uint16_t aColor,
        uint16_t aBackgroundColor) {
    while (*aStringPointer != 0) {
        LocalDisplay.drawChar(aXPos, aYPos, (char) *aStringPointer++, aSize, aColor, aBackgroundColor);
        aYPos += FONT_HEIGHT * aSize;
        if (aYPos > LOCAL_DISPLAY_HEIGHT) {
            break;
        }
    }
}

/**
 *
 * @param x0 left position
 * @param y0 upper position
 * @param aStringPtr
 * @param size Font size
 * @param color
 * @param bg_color if COLOR_NO_BACKGROUND, then do not clear rest of line
 * @return uint16_t start y for next line
 */
uint16_t SSD1289::drawMLText(uint16_t aPosX, uint16_t aPosY, const char *aStringPtr, uint8_t aTextSize, uint16_t aColor,
        uint16_t aBGColor) {
    uint16_t x = aPosX, y = aPosY, wlen, llen;
    char c;
    const char *wstart;

    if (aBGColor != COLOR_NO_BACKGROUND) {
        LocalDisplay.fillRect(aPosX, aPosY, LOCAL_DISPLAY_WIDTH - 1, aPosY + (FONT_HEIGHT * aTextSize) - 1, aBGColor);
    }

    llen = (LOCAL_DISPLAY_WIDTH - aPosX) / (FONT_WIDTH * aTextSize); //line length in chars
    wstart = aStringPtr;
    while (*aStringPtr && (y < LOCAL_DISPLAY_HEIGHT - (FONT_HEIGHT * aTextSize))) {
        c = *aStringPtr++;
        if (c == '\n') {
            //new line
            x = aPosX;
            y += (FONT_HEIGHT * aTextSize) + 1;
            LocalDisplay.fillRect(x, y, LOCAL_DISPLAY_WIDTH - 1, y + (FONT_HEIGHT * aTextSize) - 1, aBGColor);
            continue;
        } else if (c == '\r') {
            //skip
            continue;
        }

        if (c == ' ') {
            //start of a new word
            wstart = aStringPtr;
            if (x == aPosX) {
                //do nothing
                continue;
            }
        }

        if (c) {
            if ((x + (FONT_WIDTH * aTextSize)) > LOCAL_DISPLAY_WIDTH - 1) {
                //start at new line
                if (c == ' ') {
                    //do not start with space
                    x = aPosX;
                    y += (FONT_HEIGHT * aTextSize) + 1;
                    LocalDisplay.fillRect(x, y, LOCAL_DISPLAY_WIDTH - 1, y + (FONT_HEIGHT * aTextSize) - 1, aBGColor);
                } else {
                    wlen = (aStringPtr - wstart);
                    if (wlen > llen) {
                        //word too long just print on next line
                        x = aPosX;
                        y += (FONT_HEIGHT * aTextSize) + 1;
                        LocalDisplay.fillRect(x, y, LOCAL_DISPLAY_WIDTH - 1, y + (FONT_HEIGHT * aTextSize) - 1, aBGColor);
                        x = LocalDisplay.drawChar(x, y, c, aTextSize, aColor, aBGColor);
                    } else {
                        //clear actual word in line and start on next line
                        LocalDisplay.fillRect(x - (wlen * FONT_WIDTH * aTextSize), y, LOCAL_DISPLAY_WIDTH - 1,
                                (y + (FONT_HEIGHT * aTextSize)), aBGColor);
                        x = aPosX;
                        y += (FONT_HEIGHT * aTextSize) + 1;
                        LocalDisplay.fillRect(x, y, LOCAL_DISPLAY_WIDTH - 1, y + (FONT_HEIGHT * aTextSize) - 1, aBGColor);
                        aStringPtr = wstart;
                    }
                }
            } else {
                // continue on line
                x = LocalDisplay.drawChar(x, y, c, aTextSize, aColor, aBGColor);
            }
        }
    }
    return y;
}

uint16_t drawInteger(uint16_t x, uint16_t y, int val, uint8_t base, uint8_t size, uint16_t color, uint16_t bg_color) {
    char tmp[16 + 1];
    switch (base) {
    case 8:
        sprintf(tmp, "%o", (uint) val);
        break;
    case 10:
        sprintf(tmp, "%i", (uint) val);
        break;
    case 16:
        sprintf(tmp, "%x", (uint) val);
        break;
    }

    return LocalDisplay.drawText(x, y, tmp, size, color, bg_color);
}

void setBrightness(int aBacklightValue) {
    PWM_BL_setOnRatio(aBacklightValue);
    LCDLastBacklightValue = aBacklightValue;
}

/**
 * Value for lcd backlight dimming delay
 */
void setDimDelayMillis(int32_t aTimeMillis) {
    changeDelayCallback(&callbackLCDDimming, aTimeMillis);
    LCDDimDelay = aTimeMillis;
}

/**
 * restore backlight to value before dimming
 */
void resetBacklightTimeout(void) {
    if (LCDLastBacklightValue != LCDBacklightValue) {
        setBrightness(LCDBacklightValue);
    }
    changeDelayCallback(&callbackLCDDimming, LCDDimDelay);
}

int8_t getBacklightValue(void) {
    return LCDBacklightValue;
}

void setBacklightValue(int aBacklightValue) {
    LCDBacklightValue = clipBrightnessValue(aBacklightValue);
    setBrightness(LCDBacklightValue);
}

/**
 * Callback routine for SysTick handler
 * Dim LCD after period of touch inactivity
 */
void callbackLCDDimming(void) {
    if (LCDBacklightValue > BACKLIGHT_DIM_VALUE) {
        LCDLastBacklightValue = LCDBacklightValue;
        setBrightness(BACKLIGHT_DIM_VALUE);
    }
}

int clipBrightnessValue(int aBrightnessValue) {
    if (aBrightnessValue > BACKLIGHT_MAX_VALUE) {
        aBrightnessValue = BACKLIGHT_MAX_VALUE;
    }
    if (aBrightnessValue < BACKLIGHT_MIN_VALUE) {
        aBrightnessValue = BACKLIGHT_MIN_VALUE;
    }
    return aBrightnessValue;
}

void writeCommand(int aRegisterAddress, int aRegisterValue) {
// CS enable (low)
    HY32D_CS_GPIO_PORT->BSRR = (uint32_t) HY32D_CS_PIN << 16;
// Control enable (low)
    HY32D_DATA_CONTROL_GPIO_PORT->BSRR = (uint32_t) HY32D_DATA_CONTROL_PIN << 16;
// set value
    HY32D_DATA_GPIO_PORT->ODR = aRegisterAddress;
// Latch data write
    HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_WR_PIN << 16;
    HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;

// Data enable (high)
    HY32D_DATA_CONTROL_GPIO_PORT->BSRR = HY32D_DATA_CONTROL_PIN;
// set value
    HY32D_DATA_GPIO_PORT->ODR = aRegisterValue;
// Latch data write
    HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_WR_PIN << 16;
    HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;

// CS disable (high)
    HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;
    return;
}

uint16_t readCommand(int aRegisterAddress) {
// CS enable (low)
    HY32D_CS_GPIO_PORT->BSRR = (uint32_t) HY32D_CS_PIN << 16;
// Control enable (low)
    HY32D_DATA_CONTROL_GPIO_PORT->BSRR = (uint32_t) HY32D_DATA_CONTROL_PIN << 16;
// set value
    HY32D_DATA_GPIO_PORT->ODR = aRegisterAddress;
// Latch data write
    HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_WR_PIN << 16;
    HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;

// Data enable (high)
    HY32D_DATA_CONTROL_GPIO_PORT->BSRR = HY32D_DATA_CONTROL_PIN;
// set port pins to input
    HY32D_DATA_GPIO_PORT->MODER = 0x00000000;
// Latch data read
    HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_RD_PIN << 16;
// wait >250ns
    delayNanos(300);
    uint16_t tValue = HY32D_DATA_GPIO_PORT->IDR;
    HY32D_WR_GPIO_PORT->BSRR = HY32D_RD_PIN;
// set port pins to output
    HY32D_DATA_GPIO_PORT->MODER = 0x55555555;
// CS disable (high)
    HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;
    return tValue;
}

bool initalizeDisplay(void) {
// Reset is done by hardware reset button
// Original Code
    writeCommand(0x0000, 0x0001); // Enable LCD Oscillator
    delayMillis(10);
// Check Device Code - 0x8989
    if (readCommand(0x0000) != 0x8989) {
        return false;
    }

    writeCommand(0x0003, 0xA8A4); // Power control A=fosc/4 - 4= Small to medium
    writeCommand(0x000C, 0x0000); // VCIX2 only bit [2:0]
    writeCommand(0x000D, 0x080C); // VLCD63 only bit [3:0]
    writeCommand(0x000E, 0x2B00);
    writeCommand(0x001E, 0x00B0); // Bit7 + VcomH bit [5:0]
    writeCommand(0x0001, 0x293F); // reverse 320

    writeCommand(0x0002, 0x0600); // LCD driver AC setting
    writeCommand(0x0010, 0x0000); // Exit sleep mode
    delayMillis(50);

    writeCommand(0x0011, 0x6038); // 6=65k Color, 38=draw direction -> 3=horizontal increment, 8=vertical increment
//	writeCommand(0x0016, 0xEF1C); // 240 pixel
    writeCommand(0x0017, 0x0003);
    writeCommand(0x0007, 0x0133); // 1=the 2-division LCD drive is performed, 8 Color mode, grayscale
    writeCommand(0x000B, 0x0000);
    writeCommand(0x000F, 0x0000); // Gate Scan Position start (0-319)
    writeCommand(0x0041, 0x0000); // Vertical Scroll Control
    writeCommand(0x0042, 0x0000);
//	writeCommand(0x0048, 0x0000); // 0 is default 1st Screen driving position
//	writeCommand(0x0049, 0x013F); // 13F is default
//	writeCommand(0x004A, 0x0000); // 0 is default 2nd Screen driving position
//	writeCommand(0x004B, 0x0000);  // 13F is default

    delayMillis(10);
//gamma control
    writeCommand(0x0030, 0x0707);
    writeCommand(0x0031, 0x0204);
    writeCommand(0x0032, 0x0204);
    writeCommand(0x0033, 0x0502);
    writeCommand(0x0034, 0x0507);
    writeCommand(0x0035, 0x0204);
    writeCommand(0x0036, 0x0204);
    writeCommand(0x0037, 0x0502);
    writeCommand(0x003A, 0x0302);
    writeCommand(0x003B, 0x0302);

    writeCommand(0x0025, 0x8000); // Frequency Control 8=65Hz 0=50HZ E=80Hz
    return true;
}

/*
 * not checked after reset, only after first calling initalizeDisplay() above
 */
void initalizeDisplay2(void) {
// Reset is done by hardware reset button
    delayMillis(1);
    writeCommand(0x0011, 0x6838); // 6=65k Color, 8 = OE defines the display window 0 =the display window is defined by R4Eh and R4Fh.
//writeCommand(0x0011, 0x6038); // 6=65k Color, 8 = OE defines the display window 0 =the display window is defined by R4Eh and R4Fh.
//Entry Mode setting
    writeCommand(0x0002, 0x0600); // LCD driver AC setting
    writeCommand(0x0012, 0x6CEB); // RAM data write
// power control
    writeCommand(0x0003, 0xA8A4);
    writeCommand(0x000C, 0x0000); //VCIX2 only bit [2:0]
    writeCommand(0x000D, 0x080C); // VLCD63 only bit [3:0] ==
//	writeCommand(0x000D, 0x000C); // VLCD63 only bit [3:0]
    writeCommand(0x000E, 0x2B00); // ==
    writeCommand(0x001E, 0x00B0); // Bit7 + VcomH bit [5:0] ==
    writeCommand(0x0001, 0x293F); // reverse 320

// compare register
//writeCommand(0x0005, 0x0000);
//writeCommand(0x0006, 0x0000);

//writeCommand(0x0017, 0x0103); //Vertical Porch
    delayMillis(1);

    delayMillis(30);
//gamma control
    writeCommand(0x0030, 0x0707);
    writeCommand(0x0031, 0x0204);
    writeCommand(0x0032, 0x0204);
    writeCommand(0x0033, 0x0502);
    writeCommand(0x0034, 0x0507);
    writeCommand(0x0035, 0x0204);
    writeCommand(0x0036, 0x0204);
    writeCommand(0x0037, 0x0502);
    writeCommand(0x003A, 0x0302);
    writeCommand(0x003B, 0x0302);

    writeCommand(0x002F, 0x12BE);
    writeCommand(0x0023, 0x0000);
    delayMillis(1);
    writeCommand(0x0024, 0x0000);
    delayMillis(1);
    writeCommand(0x0025, 0x8000);

    writeCommand(0x004e, 0x0000); // RAM address set
    writeCommand(0x004f, 0x0000);
    return;
}

void setGamma(int aIndex) {
    switch (aIndex) {
    case 0:
        //old gamma
        writeCommand(0x0030, 0x0707);
        writeCommand(0x0031, 0x0204);
        writeCommand(0x0032, 0x0204);
        writeCommand(0x0033, 0x0502);
        writeCommand(0x0034, 0x0507);
        writeCommand(0x0035, 0x0204);
        writeCommand(0x0036, 0x0204);
        writeCommand(0x0037, 0x0502);
        writeCommand(0x003A, 0x0302);
        writeCommand(0x003B, 0x0302);
        break;
    case 1:
        // new gamma
        writeCommand(0x0030, 0x0707);
        writeCommand(0x0031, 0x0704);
        writeCommand(0x0032, 0x0204);
        writeCommand(0x0033, 0x0201);
        writeCommand(0x0034, 0x0203);
        writeCommand(0x0035, 0x0204);
        writeCommand(0x0036, 0x0204);
        writeCommand(0x0037, 0x0502);
        writeCommand(0x003A, 0x0302);
        writeCommand(0x003B, 0x0500);
        break;
    default:
        break;
    }
}

/**
 * reads a display line in BMP 16 Bit format. ie. only 5 bit for green
 */
uint16_t * fillDisplayLineBuffer(uint16_t * aBufferPtr, uint16_t yLineNumber) {
// set area is needed!
    setArea(0, yLineNumber, LOCAL_DISPLAY_WIDTH - 1, yLineNumber);
    drawStart();
    uint16_t tValue = 0;
// set port pins to input
    HY32D_DATA_GPIO_PORT->MODER = 0x00000000;
    for (uint16_t i = 0; i <= LOCAL_DISPLAY_WIDTH; ++i) {
        // Latch data read
        HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_RD_PIN << 16;
        // wait >250ns (and process former value)
        if (i > 1) {
            // skip inital value (=0) and first reading from display (is from last read => scrap)
            // shift red and green one bit down so that every color has 5 bits
            tValue = (tValue & BLUEMASK) | ((tValue >> 1) & ~BLUEMASK);
            *aBufferPtr++ = tValue;
        }
        tValue = HY32D_DATA_GPIO_PORT->IDR;
        HY32D_WR_GPIO_PORT->BSRR = HY32D_RD_PIN;
    }
// last value
    tValue = (tValue & BLUEMASK) | ((tValue >> 1) & ~BLUEMASK);
    *aBufferPtr++ = tValue;
// set port pins to output
    HY32D_DATA_GPIO_PORT->MODER = 0x55555555;
    HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;
    return aBufferPtr;
}

extern "C" void storeScreenshot(void) {
    uint8_t tFeedbackType = FEEDBACK_TONE_LONG_ERROR;
    if (MICROSD_isCardInserted()) {

        FIL tFile;
        FRESULT tOpenResult;
        UINT tCount;
//	int filesize = 54 + 2 * LOCAL_DISPLAY_WIDTH * LOCAL_DISPLAY_HEIGHT;

        unsigned char bmpfileheader[14] = { 'B', 'M', 54, 88, 02, 0, 0, 0, 0, 0, 54, 0, 0, 0 };
        unsigned char bmpinfoheader[40] = { 40, 0, 0, 0, 64, 1, 0, 0, 240, 0, 0, 0, 1, 0, 16, 0 };

//	bmpfileheader[2] = (unsigned char) (filesize);
//	bmpfileheader[3] = (unsigned char) (filesize >> 8);
//	bmpfileheader[4] = (unsigned char) (filesize >> 16);
//	bmpfileheader[5] = (unsigned char) (filesize >> 24);

        RTC_getDateStringForFile(sStringBuffer);
        strcat(sStringBuffer, ".bmp");
        tOpenResult = f_open(&tFile, sStringBuffer, FA_CREATE_ALWAYS | FA_WRITE);
        uint16_t * tBufferPtr;
        if (tOpenResult == FR_OK) {
            uint16_t * tFourDisplayLinesBufferPointer = (uint16_t *) malloc(sizeof(uint16_t) * 4 * DISPLAY_DEFAULT_WIDTH);
            if (tFourDisplayLinesBufferPointer == NULL) {
                failParamMessage(sizeof(uint16_t) * 4 * DISPLAY_DEFAULT_WIDTH, "malloc() fails");
            }
            f_write(&tFile, bmpfileheader, 14, &tCount);
            f_write(&tFile, bmpinfoheader, 40, &tCount);
            // from left to right and from bottom to top
            for (int i = LOCAL_DISPLAY_HEIGHT - 1; i >= 0;) {
                tBufferPtr = tFourDisplayLinesBufferPointer;
                // write 4 lines at a time to speed up I/O
                tBufferPtr = fillDisplayLineBuffer(tBufferPtr, i--);
                tBufferPtr = fillDisplayLineBuffer(tBufferPtr, i--);
                tBufferPtr = fillDisplayLineBuffer(tBufferPtr, i--);
                fillDisplayLineBuffer(tBufferPtr, i--);
                // write a display line
                f_write(&tFile, tFourDisplayLinesBufferPointer, LOCAL_DISPLAY_WIDTH * 8, &tCount);
            }
            free(tFourDisplayLinesBufferPointer);
            f_close(&tFile);
            tFeedbackType = FEEDBACK_TONE_OK;
        }
    }
    FeedbackTone(tFeedbackType);
}

/*
 * fast divide by 11 for SSD1289 driver arguments
 */
uint16_t getLocalTextSize(uint16_t aTextSize) {
    if (aTextSize <= 11) {
        return 1;
    }
#ifdef PGMSPACE_MATTERS
    return 2;
#else
    if (aTextSize == 22) {
        return 2;
    }
    return aTextSize / 11;
#endif
}

/** @} */
/** @} */

