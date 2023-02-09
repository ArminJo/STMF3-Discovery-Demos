/*
 * PageAccelerometerCompassGyroDemo.cpp
 * @brief shows graphic output of the accelerometer, gyroscope and compass of the STM32F3-Discovery board
 *
 *  Copyright (C) 2013-2023  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
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

#ifndef _PAGE_ACCELEROMETER_COMPASS_DEMO_HPP
#define _PAGE_ACCELEROMETER_COMPASS_DEMO_HPP

#include "l3gdc20_lsm303dlhc_utils.h"
#include "stm32f3DiscoPeripherals.h" // for readCompassRaw
#include "usbd_hid.h" // for USBD_HID_SendReport
#include "lsm303dlhc.h" // for MAG_I2C_ADDRESS


#define COLOR_ACC_GYRO_BACKGROUND COLOR16_CYAN

#define COMPASS_RADIUS 40
#define COMPASS_MID_X (COMPASS_RADIUS + 10)
#define COMPASS_MID_Y (BlueDisplay1.getDisplayHeight() / 2)

#define GYRO_MID_X BUTTON_WIDTH_3_POS_3
#define GYRO_MID_Y (BUTTON_HEIGHT_4_LINE_4 - BUTTON_DEFAULT_SPACING)

#define VERTICAL_SLIDER_NULL_VALUE 90
#define HORIZONTAL_SLIDER_NULL_VALUE 160

#define TEXT_START_Y (10 + TEXT_SIZE_11_ASCEND) // Space for slider
#define TEXT_START_X 10 // Space for slider

static struct ThickLine AccelerationLine;
uint16_t sAccelerationScale;

static struct ThickLine CompassLine;
static struct ThickLine GyroYawLine;

BDButton TouchButtonAutorepeatAccScalePlus;
BDButton TouchButtonAutorepeatAccScaleMinus;

BDButton TouchButtonClearScreen;
BDButton TouchButtonSetZero;

static BDSlider TouchSliderRoll; // Horizontal
static BDSlider TouchSliderPitch; // Vertical

void doChangeAccScale(BDButton * aTheTouchedButton, int16_t aValue);

void doSensorChange(uint8_t aSensorType, struct SensorCallback * aSensorCallbackInfo);

void doSetZero(BDButton * aTheTouchedButton, int16_t aValue) {
    // wait for end of touch vibration
    delay(300);
    setZeroAccelerometerGyroValue();
}

void drawAccDemoGui(void) {
    BlueDisplay1.clearDisplay(COLOR_ACC_GYRO_BACKGROUND);

    TouchButtonMainHome.drawButton();
    TouchButtonClearScreen.drawButton();
    TouchButtonAutorepeatAccScalePlus.drawButton();
    TouchButtonAutorepeatAccScaleMinus.drawButton();
    BlueDisplay1.drawCircle(COMPASS_MID_X, COMPASS_MID_Y, COMPASS_RADIUS, COLOR16_BLACK, 1);

    TouchButtonSetZero.drawButton();

    /*
     * Vertical slider
     */
    TouchSliderPitch.init(1, 10, 4, 2 * VERTICAL_SLIDER_NULL_VALUE, VERTICAL_SLIDER_NULL_VALUE,
    VERTICAL_SLIDER_NULL_VALUE, COLOR16_RED, COLOR16_BLACK, FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    TouchSliderPitch.setBarThresholdColor(COLOR16_GREEN);

    // Horizontal slider
    TouchSliderRoll.init(0, 1, 4, 2 * HORIZONTAL_SLIDER_NULL_VALUE, HORIZONTAL_SLIDER_NULL_VALUE,
    HORIZONTAL_SLIDER_NULL_VALUE, COLOR16_RED, COLOR16_BLACK, FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    TouchSliderRoll.setBarThresholdColor(COLOR16_GREEN);

    TouchSliderPitch.drawSlider();
    TouchSliderRoll.drawSlider();

}

/**
 * init compass hardware hardware and thick lines
 */
void initAccelerometerCompassPage(void) {
    /* Configure MEMS magnetometer main parameters: temp, working mode, full Scale and Data rate */
    /* Configure MEMS: temp and Data rate */
    COMPASSACCELERO_IO_Write(MAG_I2C_ADDRESS, LSM303DLHC_CRA_REG_M, LSM303DLHC_TEMPSENSOR_ENABLE | LSM303DLHC_ODR_30_HZ);
    /* Configure MEMS: full Scale */
    COMPASSACCELERO_IO_Write(MAG_I2C_ADDRESS, LSM303DLHC_CRB_REG_M, LSM303DLHC_FS_8_1_GA);
    /* Configure MEMS: working mode */
    COMPASSACCELERO_IO_Write(MAG_I2C_ADDRESS, LSM303DLHC_MR_REG_M, LSM303DLHC_CONTINUOS_CONVERSION);

    AccelerationLine.StartX = BlueDisplay1.getDisplayWidth() / 2;
    AccelerationLine.StartY = BlueDisplay1.getDisplayHeight() / 2;
    AccelerationLine.EndX = BlueDisplay1.getDisplayWidth() / 2;
    AccelerationLine.EndY = BlueDisplay1.getDisplayHeight() / 2;
    AccelerationLine.Thickness = 5;
    AccelerationLine.Color = COLOR16_RED;
    AccelerationLine.BackgroundColor = COLOR_ACC_GYRO_BACKGROUND;

    CompassLine.StartX = COMPASS_MID_X;
    CompassLine.StartY = COMPASS_MID_Y;
    CompassLine.EndX = COMPASS_MID_X;
    CompassLine.EndY = COMPASS_MID_Y;
    CompassLine.Thickness = 5;
    CompassLine.Color = COLOR16_BLACK;
    CompassLine.BackgroundColor = COLOR_ACC_GYRO_BACKGROUND;

    GyroYawLine.StartX = GYRO_MID_X;
    GyroYawLine.StartY = GYRO_MID_Y;
    GyroYawLine.EndX = GYRO_MID_X;
    GyroYawLine.EndY = GYRO_MID_Y;
    GyroYawLine.Thickness = 3;
    GyroYawLine.Color = COLOR16_BLUE;
    GyroYawLine.BackgroundColor = COLOR_ACC_GYRO_BACKGROUND;
}

void doClearAccelerometerCompassScreen(BDButton * aTheTouchedButton, int16_t aValue) {
    BlueDisplay1.clearDisplay(aValue);
    drawAccDemoGui();
}

void startAccelerometerCompassPage(void) {

    // TODO implement switching from CDC to HID
    //USB_ChangeToJoystick();

    // 4. row
    TouchButtonAutorepeatAccScalePlus.init(BUTTON_WIDTH_6_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_6,
    BUTTON_HEIGHT_4, COLOR16_RED, "+", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, 1,
            &doChangeAccScale);
    TouchButtonAutorepeatAccScalePlus.setButtonAutorepeatTiming(600, 100, 10, 20);

    TouchButtonAutorepeatAccScaleMinus.init(0, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_6, BUTTON_HEIGHT_4, COLOR16_RED, "-",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, -1, &doChangeAccScale);
    TouchButtonAutorepeatAccScaleMinus.setButtonAutorepeatTiming(600, 100, 10, 20);

    TouchButtonSetZero.init(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR16_RED, "Zero", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doSetZero);

    // Clear button - lower left corner
    TouchButtonClearScreen.init(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR16_RED, "Clear", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, BACKGROUND_COLOR,
            &doClearAccelerometerCompassScreen);

    drawAccDemoGui();
    registerRedrawCallback(&drawAccDemoGui);

    sAccelerationScale = 90;
    sMillisSinceLastInfoOutput = 0;

    SPI1_setPrescaler(SPI_BAUDRATEPRESCALER_8);

//    registerSensorChangeCallback(TYPE_ACCELEROMETER, SENSOR_DELAY_NORMAL, &doSensorChange);
//    BlueDisplay1.setScreenOrientationLock(true);
}

void sendCursorMovementOverUSB() {
    uint8_t HID_Buffer[4] = { 0 };
    if (isUSBReady()) {
        HID_Buffer[1] = 0;
        HID_Buffer[2] = 0;
        /* RIGHT + LEFT (negative values) Direction */
        if ((AccelerometerCompassRawDataBuffer[0] / (2 * sAccelerationScale)) != 0) {
            HID_Buffer[1] = AccelerometerCompassRawDataBuffer[0] / (2 * sAccelerationScale);
        }
        /* UP + DOWN (negative values) Direction */
        if ((AccelerometerCompassRawDataBuffer[1] / (2 * sAccelerationScale)) != 0) {
            HID_Buffer[2] = AccelerometerCompassRawDataBuffer[1] / (2 * sAccelerationScale);
        }

        /* Update the cursor position */
        if ((HID_Buffer[1] != 0) || (HID_Buffer[2] != 0)) {
            USBD_HID_SendReport(&USBDDeviceHandle, HID_Buffer, 4);
            delay(50);
        }
    }
}

void loopAccelerometerGyroCompassPage(void) {

    /* Wait 50 ms for data ready */
    uint32_t tMillis = millis();
    sMillisSinceLastInfoOutput += tMillis - sMillisOfLastLoop;
    sMillisOfLastLoop = tMillis;
    if (sMillisSinceLastInfoOutput > 50) {
        sMillisSinceLastInfoOutput = 0;
        /**
         * Accelerometer data
         */

        readAccelerometerZeroCompensated(&AccelerometerCompassRawDataBuffer[0]);
        snprintf(sStringBuffer, sizeof sStringBuffer, "Accelerometer X=%5d Y=%5d Z=%5d", AccelerometerCompassRawDataBuffer[0],
                AccelerometerCompassRawDataBuffer[1], AccelerometerCompassRawDataBuffer[2]);
        BlueDisplay1.drawText(TEXT_START_X, TEXT_START_Y, sStringBuffer, TEXT_SIZE_11, COLOR16_BLACK, COLOR16_GREEN);
        BlueDisplay1.refreshVector(&AccelerationLine, (AccelerometerCompassRawDataBuffer[0] / sAccelerationScale),
                (AccelerometerCompassRawDataBuffer[1] / sAccelerationScale));
        BlueDisplay1.drawPixel(AccelerationLine.StartX, AccelerationLine.StartY, COLOR16_BLACK);

        /**
         * Send over USB
         */
        sendCursorMovementOverUSB();
        /**
         * Compass data
         */
        readCompassRaw(&AccelerometerCompassRawDataBuffer[0]);
        snprintf(sStringBuffer, sizeof sStringBuffer, "Compass X=%5d Y=%5d Z=%5d", AccelerometerCompassRawDataBuffer[0],
                AccelerometerCompassRawDataBuffer[1], AccelerometerCompassRawDataBuffer[2]);
        BlueDisplay1.drawText(TEXT_START_X, TEXT_SIZE_11_HEIGHT + TEXT_START_Y, sStringBuffer, TEXT_SIZE_11,
        COLOR16_BLACK,
        COLOR16_GREEN);
        // values can reach 800
        BlueDisplay1.refreshVector(&CompassLine, (AccelerometerCompassRawDataBuffer[0] >> 5),
                -(AccelerometerCompassRawDataBuffer[2] >> 5));
        BlueDisplay1.drawPixel(CompassLine.StartX, CompassLine.StartY, COLOR16_RED);

        /**
         * Gyroscope data
         */
        readGyroscopeZeroCompensated(&GyroscopeRawDataBuffer[0]);
        snprintf(sStringBuffer, sizeof sStringBuffer, "Gyroscope R=%7.1f P=%7.1f Y=%7.1f", GyroscopeRawDataBuffer[0],
                GyroscopeRawDataBuffer[1], GyroscopeRawDataBuffer[2]);
        BlueDisplay1.drawText(TEXT_START_X, 2 * TEXT_SIZE_11_HEIGHT + TEXT_START_Y, sStringBuffer, TEXT_SIZE_11,
        COLOR16_BLACK,
        COLOR16_GREEN);

        TouchSliderRoll.setValueAndDrawBar(
                ((int16_t) (GyroscopeRawDataBuffer[0] / (sAccelerationScale / 4))) + HORIZONTAL_SLIDER_NULL_VALUE);
        TouchSliderPitch.setValueAndDrawBar(
                ((int16_t) (GyroscopeRawDataBuffer[1] / (sAccelerationScale / 4))) + VERTICAL_SLIDER_NULL_VALUE);

        BlueDisplay1.refreshVector(&GyroYawLine, -(int16_t) (GyroscopeRawDataBuffer[2] / (2 * sAccelerationScale)),
                -(2 * COMPASS_RADIUS));
        BlueDisplay1.drawPixel(GyroYawLine.StartX, GyroYawLine.StartY, COLOR16_RED);
    }
    checkAndHandleEvents();
}

void stopAccelerometerCompassPage(void) {
//    registerSensorChangeCallback(TYPE_ACCELEROMETER, SENSOR_DELAY_NORMAL, NULL);
//    BlueDisplay1.setScreenOrientationLock(false);

    USBD_Stop(&USBDDeviceHandle);

#if defined(SUPPORT_LOCAL_DISPLAY)
    TouchButtonSetZero.deinit();
    TouchButtonClearScreen.deinit();
    TouchButtonAutorepeatAccScalePlus.deinit();
    TouchButtonAutorepeatAccScaleMinus.deinit();
    TouchSliderPitch.deinit();
    TouchSliderRoll.deinit();
#endif
}

void doSensorChange(uint8_t aSensorType, struct SensorCallback * aSensorCallbackInfo) {
    snprintf(sStringBuffer, sizeof sStringBuffer, "Accelerometer X=%7.4f Y=%7.4f Z=%7.4f", aSensorCallbackInfo->ValueX,
            aSensorCallbackInfo->ValueY, aSensorCallbackInfo->ValueZ);
    BlueDisplay1.drawText(TEXT_START_X, TEXT_START_Y, sStringBuffer, TEXT_SIZE_11, COLOR16_BLACK, COLOR16_GREEN);
    // do not invert Y since Y drawing direction is inverse (positive is downward)
    BlueDisplay1.refreshVector(&AccelerationLine, (-aSensorCallbackInfo->ValueX * sAccelerationScale) / 4,
            aSensorCallbackInfo->ValueY * (sAccelerationScale / 4));

}

void doChangeAccScale(BDButton * aTheTouchedButton, int16_t aValue) {
    bool tIsError = false;
    sAccelerationScale += aValue * 10;
    if (sAccelerationScale < 10) {
        sAccelerationScale = 10;
        tIsError = true;
    }
    snprintf(sStringBuffer, sizeof sStringBuffer, "Scale=%3u", sAccelerationScale);
    BlueDisplay1.drawText(10, BUTTON_HEIGHT_4_LINE_4 - TEXT_SIZE_11_DECEND - 4, sStringBuffer, TEXT_SIZE_11,
    COLOR16_BLUE,
    COLOR_ACC_GYRO_BACKGROUND);
    BDButton::playFeedbackTone(tIsError);
}

#endif // _PAGE_ACCELEROMETER_COMPASS_DEMO_HPP
