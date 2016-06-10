/*
 * PageNumberPad.cpp
 *
 * @date 26.02.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */

#include "Pages.h"
#include <string.h>
#include <stdlib.h>  //strtod
#include <locale.h>

BDButton TouchButtonNumberPad0;
BDButton TouchButtonNumberPad1;
BDButton TouchButtonNumberPad2;
BDButton TouchButtonNumberPad3;
BDButton TouchButtonNumberPad4;
BDButton TouchButtonNumberPad5;
BDButton TouchButtonNumberPad6;
BDButton TouchButtonNumberPad7;
BDButton TouchButtonNumberPad8;
BDButton TouchButtonNumberPad9;
BDButton TouchButtonNumberPadDecimalSeparator;
BDButton TouchButtonNumberPadBack;
BDButton TouchButtonNumberPadClear;
BDButton TouchButtonNumberPadSign;
BDButton TouchButtonNumberPadEnter;
BDButton TouchButtonNumberPadCancel;
BDButton * const TouchButtonsNumberPad[] = { &TouchButtonNumberPad7, &TouchButtonNumberPad8, &TouchButtonNumberPad9,
        &TouchButtonNumberPadBack, &TouchButtonNumberPad4, &TouchButtonNumberPad5, &TouchButtonNumberPad6,
        &TouchButtonNumberPadClear, &TouchButtonNumberPad1, &TouchButtonNumberPad2, &TouchButtonNumberPad3,
        &TouchButtonNumberPadEnter, &TouchButtonNumberPad0, &TouchButtonNumberPadSign,
        &TouchButtonNumberPadDecimalSeparator, &TouchButtonNumberPadCancel };

static uint16_t sXStart;
static uint16_t sYStart;

/*************************************************************
 * Numberpad stuff
 *************************************************************/
void drawNumberPadValue(char * aNumberPadBuffer);
void doNumberPad(BDButton * aTheTouchedButton, int16_t aValue);

// for Touch numberpad
#define SIZEOF_NUMBERPADBUFFER 9
#define NUMBERPADBUFFER_INDEX_LAST_CHAR (SIZEOF_NUMBERPADBUFFER - 2)
extern char NumberPadBuffer[SIZEOF_NUMBERPADBUFFER];
char NumberPadBuffer[SIZEOF_NUMBERPADBUFFER];
int sNumberPadBufferSignIndex;  // Main pointer to NumberPadBuffer

static volatile bool numberpadInputHasFinished;
static volatile bool numberpadInputHasCanceled;

/**
 * allocates and draws the buttons
 */
void drawNumberPad(uint16_t aXStart, uint16_t aYStart, uint16_t aButtonColor) {
    aYStart += (3 * TEXT_SIZE_11_HEIGHT);
    TouchButtonNumberPad7.init(aXStart, aYStart, BUTTON_WIDTH_6, BUTTON_HEIGHT_6, aButtonColor, "7",
    TEXT_SIZE_22, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0x37, &doNumberPad);
    TouchButtonNumberPad8.init(aXStart + BUTTON_WIDTH_6_POS_2, aYStart, BUTTON_WIDTH_6,
    BUTTON_HEIGHT_6, aButtonColor, "8", TEXT_SIZE_22, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0x38, &doNumberPad);
    TouchButtonNumberPad9.init(aXStart + BUTTON_WIDTH_6_POS_3, aYStart, BUTTON_WIDTH_6,
    BUTTON_HEIGHT_6, aButtonColor, "9", TEXT_SIZE_22, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0x39, &doNumberPad);
    TouchButtonNumberPadBack.init(aXStart + BUTTON_WIDTH_6_POS_4, aYStart, BUTTON_WIDTH_6,
    BUTTON_HEIGHT_6, aButtonColor, "<", TEXT_SIZE_22, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0x60, &doNumberPad);

    aYStart += BUTTON_HEIGHT_6 + BUTTON_DEFAULT_SPACING;
    TouchButtonNumberPad4.init(aXStart, aYStart, BUTTON_WIDTH_6, BUTTON_HEIGHT_6, aButtonColor, "4",
    TEXT_SIZE_22, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0x34, &doNumberPad);
    TouchButtonNumberPad5.init(aXStart + BUTTON_WIDTH_6_POS_2, aYStart, BUTTON_WIDTH_6,
    BUTTON_HEIGHT_6, aButtonColor, "5", TEXT_SIZE_22, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0x35, &doNumberPad);
    TouchButtonNumberPad6.init(aXStart + BUTTON_WIDTH_6_POS_3, aYStart, BUTTON_WIDTH_6,
    BUTTON_HEIGHT_6, aButtonColor, "6", TEXT_SIZE_22, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0x36, &doNumberPad);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
    TouchButtonNumberPadClear.init(aXStart + BUTTON_WIDTH_6_POS_4, aYStart, BUTTON_WIDTH_6,
    BUTTON_HEIGHT_6, aButtonColor, "C", TEXT_SIZE_22, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0x43, &doNumberPad);
#pragma GCC diagnostic pop
    aYStart += BUTTON_HEIGHT_6 + BUTTON_DEFAULT_SPACING;
    TouchButtonNumberPad1.init(aXStart, aYStart, BUTTON_WIDTH_6, BUTTON_HEIGHT_6, aButtonColor, "1",
    TEXT_SIZE_22, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0x31, &doNumberPad);
    TouchButtonNumberPad2.init(aXStart + BUTTON_WIDTH_6_POS_2, aYStart, BUTTON_WIDTH_6,
    BUTTON_HEIGHT_6, aButtonColor, "2", TEXT_SIZE_22, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0x32, &doNumberPad);
    TouchButtonNumberPad3.init(aXStart + BUTTON_WIDTH_6_POS_3, aYStart, BUTTON_WIDTH_6,
    BUTTON_HEIGHT_6, aButtonColor, "3", TEXT_SIZE_22, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0x33, &doNumberPad);
    //Enter Button
    // size is 2 * BUTTON_WIDTH_6 or less if not enough space on screen
    int tWidth = 2 * BUTTON_WIDTH_6;
    if ((tWidth + aXStart + BUTTON_WIDTH_6_POS_4) > BlueDisplay1.getDisplayWidth()) {
        tWidth = BlueDisplay1.getDisplayWidth() - (aXStart + BUTTON_WIDTH_6_POS_4);
    }
    TouchButtonNumberPadEnter.init(aXStart + BUTTON_WIDTH_6_POS_4, aYStart, tWidth,
            (2 * BUTTON_HEIGHT_6) + BUTTON_DEFAULT_SPACING, aButtonColor, StringEnterChar, 44,
            BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0x0D, &doNumberPad);

    aYStart += BUTTON_HEIGHT_6 + BUTTON_DEFAULT_SPACING;
    TouchButtonNumberPad0.init(aXStart, aYStart, BUTTON_WIDTH_6, BUTTON_HEIGHT_6, aButtonColor, "0",
    TEXT_SIZE_22, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0x30, &doNumberPad);
    TouchButtonNumberPadSign.init(aXStart + BUTTON_WIDTH_6_POS_2, aYStart, BUTTON_WIDTH_6,
    BUTTON_HEIGHT_6, aButtonColor, StringPlusMinus, TEXT_SIZE_22, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0xF3, &doNumberPad);
    struct lconv * tLconfPtr = localeconv();
    TouchButtonNumberPadDecimalSeparator.init(aXStart + BUTTON_WIDTH_6_POS_3, aYStart,
    BUTTON_WIDTH_6, BUTTON_HEIGHT_6, aButtonColor, tLconfPtr->decimal_point, TEXT_SIZE_22,
            BUTTON_FLAG_NO_BEEP_ON_TOUCH, tLconfPtr->decimal_point[0], &doNumberPad);
    TouchButtonNumberPadCancel.init(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, aButtonColor, "Cancel", TEXT_SIZE_22, BUTTON_FLAG_NO_BEEP_ON_TOUCH, 0xFF, &doNumberPad);
    for (unsigned int i = 0; i < sizeof(TouchButtonsNumberPad) / sizeof(TouchButtonsNumberPad[0]); ++i) {
        TouchButtonsNumberPad[i]->drawButton();
    }
}

void drawNumberPadValue(char * aNumberPadBuffer) {
    BlueDisplay1.drawText(sXStart, sYStart + TEXT_SIZE_22_ASCEND, aNumberPadBuffer, TEXT_SIZE_22, COLOR_PAGE_INFO,
            COLOR_WHITE);
}

/**
 * set buffer to 0 and display it
 */
void clearNumberPadBuffer(void) {
    for (int i = 0; i <= NUMBERPADBUFFER_INDEX_LAST_CHAR - 1; ++i) {
        NumberPadBuffer[i] = 0x20;
    }
    NumberPadBuffer[NUMBERPADBUFFER_INDEX_LAST_CHAR] = 0x30;
    NumberPadBuffer[SIZEOF_NUMBERPADBUFFER - 1] = 0x00;
    sNumberPadBufferSignIndex = NUMBERPADBUFFER_INDEX_LAST_CHAR;
    drawNumberPadValue(NumberPadBuffer);
}

void doNumberPad(BDButton * aTheTouchedButton, int16_t aValue) {
    uint8_t tFeedbackType = FEEDBACK_TONE_NO_ERROR;
    if (aValue == 0x60) { // "<"
        if (sNumberPadBufferSignIndex < NUMBERPADBUFFER_INDEX_LAST_CHAR) {
            // copy all chars one right
            for (int i = SIZEOF_NUMBERPADBUFFER - 2; i > 0; i--) {
                NumberPadBuffer[i] = NumberPadBuffer[i - 1];
            }
            NumberPadBuffer[sNumberPadBufferSignIndex++] = 0x20;
        } else {
            tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
        }
    } else if (aValue == 0xF3) { // Change sign
        if (NumberPadBuffer[sNumberPadBufferSignIndex] == 0x2D) {
            // clear minus sign
            NumberPadBuffer[sNumberPadBufferSignIndex] = 0x20;
        } else {
            NumberPadBuffer[sNumberPadBufferSignIndex] = 0x2D;
        }
    } else if (aValue == 0x43) { // Clear "C"
        clearNumberPadBuffer();
    } else if (aValue == 0x0D) { // Enter
        numberpadInputHasFinished = true;
    } else if (aValue == 0xFF) { // Cancel
        numberpadInputHasFinished = true;
        numberpadInputHasCanceled = true;
    } else {
        // plain numbers and decimal separator
        if (sNumberPadBufferSignIndex > 0) {
            if (sNumberPadBufferSignIndex < NUMBERPADBUFFER_INDEX_LAST_CHAR) {
                // copy all chars one left
                for (int i = 0; i < NUMBERPADBUFFER_INDEX_LAST_CHAR; ++i) {
                    NumberPadBuffer[i] = NumberPadBuffer[i + 1];
                }
            }
            sNumberPadBufferSignIndex--;
            // set number at last (rightmost) position
            NumberPadBuffer[NUMBERPADBUFFER_INDEX_LAST_CHAR] = aValue;
        } else {
            tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
        }
    }
    drawNumberPadValue(NumberPadBuffer);
    FeedbackTone(tFeedbackType);
}

/**
 * clears display, shows numberpad and returns with a float
 */
float getNumberFromNumberPad(uint16_t aXStart, uint16_t aYStart, uint16_t aButtonColor) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    sXStart = aXStart;
    sYStart = aYStart;
    BDButton::deactivateAllButtons();
    // disable temporarily the end callback function
    setTouchUpCallbackEnabled(false);
    drawNumberPad(aXStart, aYStart, aButtonColor);
    clearNumberPadBuffer();
    numberpadInputHasFinished = false;
    numberpadInputHasCanceled = false;
    while (!numberpadInputHasFinished) {
        checkAndHandleEvents();
    }
    // free numberpad buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsNumberPad) / sizeof(TouchButtonsNumberPad[0]); ++i) {
        TouchButtonsNumberPad[i]->deinit();
    }
    // to avoid end touch handling of releasing OK or Cancel button of numberpad
    sDisableTouchUpOnce = true;
    setTouchUpCallbackEnabled(true);
    if (numberpadInputHasCanceled) {
        return NAN;
    } else {
        return strtod(NumberPadBuffer, 0);
    }
}
