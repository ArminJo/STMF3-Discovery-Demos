/*
 * PageDraw.cpp
 *
 * @date 16.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */

#include "Pages.h"
#include "EventHandler.h"

#include <string.h>

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static struct XYPosition sLastPos;
static uint16_t sDrawColor = COLOR_BLACK;
static bool mButtonTouched;

BDButton TouchButtonClear;
#define NUMBER_OF_DRAW_COLORS 5
BDButton TouchButtonsDrawColor[NUMBER_OF_DRAW_COLORS];
static const uint16_t DrawColors[NUMBER_OF_DRAW_COLORS] = { COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_BLUE,
        COLOR_YELLOW };

void initDrawPage(void) {
}

void drawDrawPage(void) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    for (uint8_t i = 0; i < 5; ++i) {
        TouchButtonsDrawColor[i].drawButton();
    }
    TouchButtonClear.drawButton();
    TouchButtonMainHome.drawButton();
}

void doDrawClear(BDButton * aTheTouchedButton, int16_t aValue) {
    drawDrawPage();
}

static void doDrawColor(BDButton * aTheTouchedButton, int16_t aValue) {
    sDrawColor = DrawColors[aValue];
}

/*
 * Position changed -> draw line
 */
void drawPageTouchMoveCallbackHandler(struct TouchEvent * const aActualPositionPtr) {
    if (!mButtonTouched) {
        BlueDisplay1.drawLine(sLastPos.PosX, sLastPos.PosY, aActualPositionPtr->TouchPosition.PosX, aActualPositionPtr->TouchPosition.PosY,
                sDrawColor);
        sLastPos.PosX = aActualPositionPtr->TouchPosition.PosX;
        sLastPos.PosY = aActualPositionPtr->TouchPosition.PosY;
    }
}

/*
 * Touch is going down on canvas -> draw starting point
 */
void drawPageTouchDownCallbackHandler(struct TouchEvent * const aActualPositionPtr) {
    // first check buttons
    mButtonTouched = TouchButton::checkAllButtons(aActualPositionPtr->TouchPosition.PosX, aActualPositionPtr->TouchPosition.PosY);
    if (!mButtonTouched) {
        int x = aActualPositionPtr->TouchPosition.PosX;
        int y = aActualPositionPtr->TouchPosition.PosY;
        BlueDisplay1.drawPixel(x, y, sDrawColor);
        sLastPos.PosX = x;
        sLastPos.PosY = y;
    }
}

void startDrawPage(void) {
    // Color buttons
    uint16_t tPosY = 0;
    for (uint8_t i = 0; i < 5; ++i) {
        TouchButtonsDrawColor[i].init(0, tPosY, 30, 30, DrawColors[i], NULL, TEXT_SIZE_11,
                FLAG_BUTTON_DO_BEEP_ON_TOUCH, i, &doDrawColor);
        tPosY += 30;
    }

    TouchButtonClear.init(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_RED, "Clear", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doDrawClear);

    // No need to store old values since I know, that I return to main page
    registerTouchDownCallback(&drawPageTouchDownCallbackHandler);
    registerTouchMoveCallback(&drawPageTouchMoveCallbackHandler);
    registerRedrawCallback(&drawDrawPage);

    drawDrawPage();
    // to avoid first line because of moves after touch of the button starting this page
    mButtonTouched = true;
}

void loopDrawPage(void) {
    checkAndHandleEvents();
}

void stopDrawPage(void) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
// free buttons
    for (unsigned int i = 0; i < NUMBER_OF_DRAW_COLORS; ++i) {
        TouchButtonsDrawColor[i].deinit();
    }
    TouchButtonClear.deinit();
#endif
}

