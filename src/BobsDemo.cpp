/**
 * BobsDemo.cpp
 *
 * @ brief demo playground for accelerometer etc.
 *
 * @date 16.12.2012
 * @author Robert
 */
#include "Pages.h"
#include "stm32f3DiscoPeripherals.h"
#include "l3gdc20_lsm303dlhc_utils.h"

/**
 *
 */
#define ABS(x)         (x < 0) ? (-x) : x

#define RADIUS_BALL 5
#define INCREMENT 2
#define COLOR_BALL COLOR_GREEN
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
    BUTTON_HEIGHT_4, COLOR_CYAN, "Next", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doNextLevel);
}

void startBobsDemo(void) {
    LastX = LOCAL_DISPLAY_WIDTH / 2;
    LastY = LOCAL_DISPLAY_HEIGHT / 2;
    AccumulatedValueX = 0;
    AccumulatedValueY = 0;
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    BlueDisplay1.fillCircle(LastX, LastY, RADIUS_BALL, COLOR_BALL);
    direction = INCREMENT;

    TouchButtonMainHome.drawButton();
    // male Home
    BlueDisplay1.drawChar(HOME_X - (TEXT_SIZE_22_WIDTH / 2), HOME_Y - (TEXT_SIZE_22_HEIGHT / 2) + TEXT_SIZE_22_ASCEND,
            0xD3,
            HOME_FONT_SIZE, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
    GameActive = true;
    // wait for end of touch vibration
    delayMillis(300);
    setZeroAccelerometerGyroValue();
}

void stopBobsDemo(void) {
    TouchButtonNextLevel.deinit();
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
/**
 * Bobs Demo
 */

void doNextLevel(BDButton * aTheTouchedButton, int16_t aValue) {
    startBobsDemo();
    //male Umrandung
    // left
    BlueDisplay1.fillRectRel(sHomeMidX - 20, sHomeMidY - 20, 20 - (TEXT_SIZE_22_WIDTH / 2), 30, COLOR_BLUE);
    // right
    BlueDisplay1.fillRectRel(sHomeMidX + (TEXT_SIZE_22_WIDTH / 2), sHomeMidY - 20, 20 - (TEXT_SIZE_22_WIDTH / 2), 30,
    COLOR_BLUE);
    // top
    BlueDisplay1.fillRectRel(sHomeMidX - (TEXT_SIZE_22_WIDTH / 2), sHomeMidY - 20, TEXT_SIZE_22_WIDTH, 8, COLOR_BLUE);
}

void loopBobsDemo(void) {
    int16_t aAccelerometerRawDataBuffer[3];
    unsigned int tNewX;
    unsigned int tNewY;

    delayMillis(5);
    if (GameActive) {

        // berechne neue Position
        readAccelerometerZeroCompensated(aAccelerometerRawDataBuffer);

        /**
         * X Achse
         */
        AccumulatedValueX += aAccelerometerRawDataBuffer[0];
        tNewX = LastX + AccumulatedValueX / X_RESOLUTION;
        AccumulatedValueX = AccumulatedValueX % X_RESOLUTION;

        // pr�fen ob am rechten Rand angekommen
        if (tNewX + RADIUS_BALL >= LOCAL_DISPLAY_WIDTH) {
            // stay
            tNewX = LastX;
        }
        // pr�fen ob am linken Rand angekommen
        if (tNewX < RADIUS_BALL) {
            tNewX = LastX;
        }

        /**
         * Y Achse
         */
        AccumulatedValueY += aAccelerometerRawDataBuffer[1];
        tNewY = LastY + AccumulatedValueY / Y_RESOLUTION;
        AccumulatedValueY = AccumulatedValueY % Y_RESOLUTION;

        // pr�fen ob am unteren Rand angekommen
        if (tNewY + RADIUS_BALL >= LOCAL_DISPLAY_HEIGHT) {
            // stay
            tNewY = LastY;
        }
        // pr�fen ob am oberen Rand angekommen
        if (tNewY < RADIUS_BALL) {
            // stay
            tNewY = LastY;
        }

        // Pr�fe ob Ziel ber�hrt wird
        if (((tNewX + RADIUS_BALL) > sHomeMidX) && tNewX < (sHomeMidX + RADIUS_BALL)
                && ((tNewY + RADIUS_BALL) > sHomeMidY) && tNewY < (sHomeMidY + RADIUS_BALL)) {
            // Ziel ber�hrt
            tone(440, 200);
            delayMillis(200);
            tone(660, 600);
            BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
            TouchButtonMainHome.drawButton();
//			TouchButtonClearScreen.drawButton();
            BlueDisplay1.drawText(0, 100, "Level complete", TEXT_SIZE_22, COLOR_BLACK, COLOR_BACKGROUND_DEFAULT);
            TouchButtonNextLevel.drawButton();
            GameActive = false;
        } else {
            //
            // Ziel nicht ber�hrt ->  l�sche altes Bild male neuen Ball
            BlueDisplay1.fillCircle(LastX, LastY, RADIUS_BALL, COLOR_BACKGROUND_DEFAULT);

            // Pr�fe ob HomeKnopf ber�hrt und gel�scht wird
            if (LastX + RADIUS_BALL >= BUTTON_WIDTH_5_POS_5 && LastY - RADIUS_BALL <= BUTTON_HEIGHT_4) {
                // male Knopf neu
                TouchButtonMainHome.drawButton();
            }
            // Pr�fe ob ClearKnopf ber�hrt wird
//			if (LastX - RADIUS_BALL <= TouchButtonClearScreen.getPositionXRight()
//					&& LastY + RADIUS_BALL >= TouchButtonClearScreen.getPositionY()) {
//				// male Knopf neu
//				TouchButtonClearScreen.drawButton();
//			}
            // Pr�fe ob Ziel Icon ber�hrt wird
            if (LastX + RADIUS_BALL +(TEXT_SIZE_22_WIDTH / 2) >= sHomeMidX
                    && LastX - RADIUS_BALL <= (sHomeMidX + (TEXT_SIZE_22_WIDTH / 2))
                    && LastY + RADIUS_BALL >= sHomeMidY - (TEXT_SIZE_22_HEIGHT / 2)
                    && LastY - RADIUS_BALL <= (sHomeMidY + (TEXT_SIZE_22_HEIGHT / 2))) {
                BlueDisplay1.drawChar(HOME_X - (TEXT_SIZE_22_WIDTH / 2),
                HOME_Y - (TEXT_SIZE_22_HEIGHT / 2) + TEXT_SIZE_22_ASCEND, 0xD3,
                HOME_FONT_SIZE, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
            }
            BlueDisplay1.fillCircle(tNewX, tNewY, RADIUS_BALL, COLOR_BALL);

            LastX = tNewX;
            LastY = tNewY;
        }
    }
    checkAndHandleEvents();
}
#pragma GCC diagnostic pop

