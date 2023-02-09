/**
 * BobsDemo.hpp
 *
 * demo playground for accelerometer etc.
 *
 * @date 16.12.2012
 *
 *  Copyright (C) 2012-2023  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com, Robert Derichs
 *
 *  This file is part of STMF3-Discovery-Demos https://github.com/ArminJo/STMF3-Discovery-Demos.
 *
 *  STMF3-Discovery-Demos is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 */

#ifndef _BOBS_DEMO_HPP
#define _BOBS_DEMO_HPP

#include "Pages.h"
#include "stm32f3DiscoPeripherals.h"
#include "l3gdc20_lsm303dlhc_utils.h"

/**
 *
 */
#define ABS(x)         (x < 0) ? (-x) : x

#define RADIUS_BALL 5
#define INCREMENT 2
#define COLOR_BALL COLOR16_GREEN
unsigned int LastX;
unsigned int LastY;
int32_t AccumulatedValueX;
int32_t AccumulatedValueY;
int16_t direction;
BDButton TouchButtonNextLevel;
bool GameActive;

#define HOME_X 220
#define HOME_Y 50
#define HOME_FONT_SIZE TEXT_SIZE_22
#define HOME_MID_X 220
#define HOME_MID_Y 50
#define Y_RESOLUTION 2048
#define X_RESOLUTION 2048

void doNextLevel(BDButton * aTheTouchedButton, int16_t aValue);
unsigned int sHomeMidX = HOME_MID_X;
unsigned int sHomeMidY = HOME_MID_Y;

void initBobsDemo(void) {

    TouchButtonNextLevel.init(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR16_CYAN, "Next", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doNextLevel);
}

void startBobsDemo(void) {
    LastX = LOCAL_DISPLAY_WIDTH / 2;
    LastY = LOCAL_DISPLAY_HEIGHT / 2;
    AccumulatedValueX = 0;
    AccumulatedValueY = 0;
    BlueDisplay1.clearDisplay(BACKGROUND_COLOR);
    BlueDisplay1.fillCircle(LastX, LastY, RADIUS_BALL, COLOR_BALL);
    direction = INCREMENT;

    TouchButtonMainHome.drawButton();
    // male Home
    BlueDisplay1.drawChar(HOME_X - (TEXT_SIZE_22_WIDTH / 2), HOME_Y - (TEXT_SIZE_22_HEIGHT / 2) + TEXT_SIZE_22_ASCEND,
            0xD3,
            HOME_FONT_SIZE, COLOR16_RED, BACKGROUND_COLOR);
    GameActive = true;
    // wait for end of touch vibration
    delay(300);
    setZeroAccelerometerGyroValue();
}

void stopBobsDemo(void) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    TouchButtonNextLevel.deinit();
#endif
}

/**
 * Bobs Demo
 */

void doNextLevel(BDButton * aTheTouchedButton, int16_t aValue) {
    startBobsDemo();
    //male Umrandung
    // left
    BlueDisplay1.fillRectRel(sHomeMidX - 20, sHomeMidY - 20, 20 - (TEXT_SIZE_22_WIDTH / 2), 30, COLOR16_BLUE);
    // right
    BlueDisplay1.fillRectRel(sHomeMidX + (TEXT_SIZE_22_WIDTH / 2), sHomeMidY - 20, 20 - (TEXT_SIZE_22_WIDTH / 2), 30,
    COLOR16_BLUE);
    // top
    BlueDisplay1.fillRectRel(sHomeMidX - (TEXT_SIZE_22_WIDTH / 2), sHomeMidY - 20, TEXT_SIZE_22_WIDTH, 8, COLOR16_BLUE);
}

void loopBobsDemo(void) {
    int16_t aAccelerometerRawDataBuffer[3];
    unsigned int tNewX;
    unsigned int tNewY;

    delay(5);
    if (GameActive) {

        // berechne neue Position
        readAccelerometerZeroCompensated(aAccelerometerRawDataBuffer);

        /**
         * X Achse
         */
        AccumulatedValueX += aAccelerometerRawDataBuffer[0];
        tNewX = LastX + AccumulatedValueX / X_RESOLUTION;
        AccumulatedValueX = AccumulatedValueX % X_RESOLUTION;

        // prüfen ob am rechten Rand angekommen
        if (tNewX + RADIUS_BALL >= LOCAL_DISPLAY_WIDTH) {
            // stay
            tNewX = LastX;
        }
        // prüfen ob am linken Rand angekommen
        if (tNewX < RADIUS_BALL) {
            tNewX = LastX;
        }

        /**
         * Y Achse
         */
        AccumulatedValueY += aAccelerometerRawDataBuffer[1];
        tNewY = LastY + AccumulatedValueY / Y_RESOLUTION;
        AccumulatedValueY = AccumulatedValueY % Y_RESOLUTION;

        // prüfen ob am unteren Rand angekommen
        if (tNewY + RADIUS_BALL >= LOCAL_DISPLAY_HEIGHT) {
            // stay
            tNewY = LastY;
        }
        // prüfen ob am oberen Rand angekommen
        if (tNewY < RADIUS_BALL) {
            // stay
            tNewY = LastY;
        }

        // Prüfe ob Ziel berührt wird
        if (((tNewX + RADIUS_BALL) > sHomeMidX) && tNewX < (sHomeMidX + RADIUS_BALL)
                && ((tNewY + RADIUS_BALL) > sHomeMidY) && tNewY < (sHomeMidY + RADIUS_BALL)) {
            // Ziel berührt
            tone(440, 200);
            delay(200);
            tone(660, 600);
            BlueDisplay1.clearDisplay(BACKGROUND_COLOR);
            TouchButtonMainHome.drawButton();
//			TouchButtonClearScreen.drawButton();
            BlueDisplay1.drawText(0, 100, "Level complete", TEXT_SIZE_22, COLOR16_BLACK, BACKGROUND_COLOR);
            TouchButtonNextLevel.drawButton();
            GameActive = false;
        } else {
            //
            // Ziel nicht berührt ->  lösche altes Bild male neuen Ball
            BlueDisplay1.fillCircle(LastX, LastY, RADIUS_BALL, BACKGROUND_COLOR);

            // Prüfe ob HomeKnopf berührt und gelöscht wird
            if (LastX + RADIUS_BALL >= BUTTON_WIDTH_5_POS_5 && LastY - RADIUS_BALL <= BUTTON_HEIGHT_4) {
                // male Knopf neu
                TouchButtonMainHome.drawButton();
            }
            // Prüfe ob ClearKnopf berührt wird
//			if (LastX - RADIUS_BALL <= TouchButtonClearScreen.getPositionXRight()
//					&& LastY + RADIUS_BALL >= TouchButtonClearScreen.getPositionY()) {
//				// male Knopf neu
//				TouchButtonClearScreen.drawButton();
//			}
            // Prüfe ob Ziel Icon berührt wird
            if (LastX + RADIUS_BALL +(TEXT_SIZE_22_WIDTH / 2) >= sHomeMidX
                    && LastX - RADIUS_BALL <= (sHomeMidX + (TEXT_SIZE_22_WIDTH / 2))
                    && LastY + RADIUS_BALL >= sHomeMidY - (TEXT_SIZE_22_HEIGHT / 2)
                    && LastY - RADIUS_BALL <= (sHomeMidY + (TEXT_SIZE_22_HEIGHT / 2))) {
                BlueDisplay1.drawChar(HOME_X - (TEXT_SIZE_22_WIDTH / 2),
                HOME_Y - (TEXT_SIZE_22_HEIGHT / 2) + TEXT_SIZE_22_ASCEND, 0xD3,
                HOME_FONT_SIZE, COLOR16_RED, BACKGROUND_COLOR);
            }
            BlueDisplay1.fillCircle(tNewX, tNewY, RADIUS_BALL, COLOR_BALL);

            LastX = tNewX;
            LastY = tNewY;
        }
    }
    checkAndHandleEvents();
}

#endif // _BOBS_DEMO_HPP
