/*
 * PageAccelerometerCompassGyroDemo.cpp
 * @brief shows graphic output of the accelerometer, gyroscope and compass of the STM32F3-Discovery board
 *
 * @date 02.01.2013
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 *      Source based on STM code samples
 * @version 1.2.0
 */

#include "Pages.h"
#include "thickline.h"
#include "stm32f3DiscoveryLedsButtons.h"
#include "l3gdc20_lsm303dlhc_utils.h"
#include "stm32f3DiscoPeripherals.h"

extern "C" {
#include "usbd_misc.h"
#include "usbd_desc.h"
#include "usbd_hid.h"

#include "stm32f3_discovery.h"
#include "lsm303dlhc.h"
#include "math.h"
}

#define COLOR_ACC_GYRO_BACKGROUND COLOR_CYAN

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
    delayMillis(300);
    setZeroAccelerometerGyroValue();
}

void drawAccDemoGui(void) {
    BlueDisplay1.clearDisplay(COLOR_ACC_GYRO_BACKGROUND);

    TouchButtonMainHome.drawButton();
    TouchButtonClearScreen.drawButton();
    TouchButtonAutorepeatAccScalePlus.drawButton();
    TouchButtonAutorepeatAccScaleMinus.drawButton();
    BlueDisplay1.drawCircle(COMPASS_MID_X, COMPASS_MID_Y, COMPASS_RADIUS, COLOR_BLACK, 1);

    TouchButtonSetZero.drawButton();

    /*
     * Vertical slider
     */
    TouchSliderPitch.init(1, 10, 4, 2 * VERTICAL_SLIDER_NULL_VALUE, VERTICAL_SLIDER_NULL_VALUE,
    VERTICAL_SLIDER_NULL_VALUE, COLOR_RED, COLOR_BLACK, FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    TouchSliderPitch.setBarThresholdColor(COLOR_GREEN);

    // Horizontal slider
    TouchSliderRoll.init(0, 1, 4, 2 * HORIZONTAL_SLIDER_NULL_VALUE, HORIZONTAL_SLIDER_NULL_VALUE,
    HORIZONTAL_SLIDER_NULL_VALUE, COLOR_RED, COLOR_BLACK, FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    TouchSliderRoll.setBarThresholdColor(COLOR_GREEN);

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
    AccelerationLine.Color = COLOR_RED;
    AccelerationLine.BackgroundColor = COLOR_ACC_GYRO_BACKGROUND;

    CompassLine.StartX = COMPASS_MID_X;
    CompassLine.StartY = COMPASS_MID_Y;
    CompassLine.EndX = COMPASS_MID_X;
    CompassLine.EndY = COMPASS_MID_Y;
    CompassLine.Thickness = 5;
    CompassLine.Color = COLOR_BLACK;
    CompassLine.BackgroundColor = COLOR_ACC_GYRO_BACKGROUND;

    GyroYawLine.StartX = GYRO_MID_X;
    GyroYawLine.StartY = GYRO_MID_Y;
    GyroYawLine.EndX = GYRO_MID_X;
    GyroYawLine.EndY = GYRO_MID_Y;
    GyroYawLine.Thickness = 3;
    GyroYawLine.Color = COLOR_BLUE;
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
    BUTTON_HEIGHT_4, COLOR_RED, "+", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, 1,
            &doChangeAccScale);
    TouchButtonAutorepeatAccScalePlus.setButtonAutorepeatTiming(600, 100, 10, 20);

    TouchButtonAutorepeatAccScaleMinus.init(0, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_6, BUTTON_HEIGHT_4, COLOR_RED, "-",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, -1, &doChangeAccScale);
    TouchButtonAutorepeatAccScaleMinus.setButtonAutorepeatTiming(600, 100, 10, 20);

    TouchButtonSetZero.init(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_RED, "Zero", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doSetZero);

    // Clear button - lower left corner
    TouchButtonClearScreen.init(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_RED, "Clear", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, COLOR_BACKGROUND_DEFAULT,
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
            delayMillis(50);
        }
    }
}

void loopAccelerometerGyroCompassPage(void) {

    /* Wait 50 ms for data ready */
    uint32_t tMillis = getMillisSinceBoot();
    sMillisSinceLastInfoOutput += tMillis - MillisLastLoop;
    MillisLastLoop = tMillis;
    if (sMillisSinceLastInfoOutput > 50) {
        sMillisSinceLastInfoOutput = 0;
        /**
         * Accelerometer data
         */

        readAccelerometerZeroCompensated(&AccelerometerCompassRawDataBuffer[0]);
        snprintf(sStringBuffer, sizeof sStringBuffer, "Accelerometer X=%5d Y=%5d Z=%5d", AccelerometerCompassRawDataBuffer[0],
                AccelerometerCompassRawDataBuffer[1], AccelerometerCompassRawDataBuffer[2]);
        BlueDisplay1.drawText(TEXT_START_X, TEXT_START_Y, sStringBuffer, TEXT_SIZE_11, COLOR_BLACK, COLOR_GREEN);
        BlueDisplay1.refreshVector(&AccelerationLine, (AccelerometerCompassRawDataBuffer[0] / sAccelerationScale),
                (AccelerometerCompassRawDataBuffer[1] / sAccelerationScale));
        BlueDisplay1.drawPixel(AccelerationLine.StartX, AccelerationLine.StartY, COLOR_BLACK);

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
        COLOR_BLACK,
        COLOR_GREEN);
        // values can reach 800
        BlueDisplay1.refreshVector(&CompassLine, (AccelerometerCompassRawDataBuffer[0] >> 5),
                -(AccelerometerCompassRawDataBuffer[2] >> 5));
        BlueDisplay1.drawPixel(CompassLine.StartX, CompassLine.StartY, COLOR_RED);

        /**
         * Gyroscope data
         */
        readGyroscopeZeroCompensated(&GyroscopeRawDataBuffer[0]);
        snprintf(sStringBuffer, sizeof sStringBuffer, "Gyroscope R=%7.1f P=%7.1f Y=%7.1f", GyroscopeRawDataBuffer[0],
                GyroscopeRawDataBuffer[1], GyroscopeRawDataBuffer[2]);
        BlueDisplay1.drawText(TEXT_START_X, 2 * TEXT_SIZE_11_HEIGHT + TEXT_START_Y, sStringBuffer, TEXT_SIZE_11,
        COLOR_BLACK,
        COLOR_GREEN);

        TouchSliderRoll.setValueAndDrawBar(
                ((int16_t) (GyroscopeRawDataBuffer[0] / (sAccelerationScale / 4))) + HORIZONTAL_SLIDER_NULL_VALUE);
        TouchSliderPitch.setValueAndDrawBar(
                ((int16_t) (GyroscopeRawDataBuffer[1] / (sAccelerationScale / 4))) + VERTICAL_SLIDER_NULL_VALUE);

        BlueDisplay1.refreshVector(&GyroYawLine, -(int16_t) (GyroscopeRawDataBuffer[2] / (2 * sAccelerationScale)),
                -(2 * COMPASS_RADIUS));
        BlueDisplay1.drawPixel(GyroYawLine.StartX, GyroYawLine.StartY, COLOR_RED);
    }
    checkAndHandleEvents();
}

void stopAccelerometerCompassPage(void) {
//    registerSensorChangeCallback(TYPE_ACCELEROMETER, SENSOR_DELAY_NORMAL, NULL);
//    BlueDisplay1.setScreenOrientationLock(false);

    USBD_Stop(&USBDDeviceHandle);

    TouchButtonSetZero.deinit();
    TouchButtonClearScreen.deinit();
    TouchButtonAutorepeatAccScalePlus.deinit();
    TouchButtonAutorepeatAccScaleMinus.deinit();
    TouchSliderPitch.deinit();
    TouchSliderRoll.deinit();
}

void doSensorChange(uint8_t aSensorType, struct SensorCallback * aSensorCallbackInfo) {
    snprintf(sStringBuffer, sizeof sStringBuffer, "Accelerometer X=%7.4f Y=%7.4f Z=%7.4f", aSensorCallbackInfo->ValueX,
            aSensorCallbackInfo->ValueY, aSensorCallbackInfo->ValueZ);
    BlueDisplay1.drawText(TEXT_START_X, TEXT_START_Y, sStringBuffer, TEXT_SIZE_11, COLOR_BLACK, COLOR_GREEN);
    // do not invert Y since Y drawing direction is inverse (positive is downward)
    BlueDisplay1.refreshVector(&AccelerationLine, (-aSensorCallbackInfo->ValueX * sAccelerationScale) / 4,
            aSensorCallbackInfo->ValueY * (sAccelerationScale / 4));

}

void doChangeAccScale(BDButton * aTheTouchedButton, int16_t aValue) {
    int tFeedbackType = FEEDBACK_TONE_OK;
    sAccelerationScale += aValue * 10;
    if (sAccelerationScale < 10) {
        sAccelerationScale = 10;
        tFeedbackType = FEEDBACK_TONE_ERROR;
    }
    snprintf(sStringBuffer, sizeof sStringBuffer, "Scale=%3u", sAccelerationScale);
    BlueDisplay1.drawText(10, BUTTON_HEIGHT_4_LINE_4 - TEXT_SIZE_11_DECEND - 4, sStringBuffer, TEXT_SIZE_11,
    COLOR_BLUE,
    COLOR_ACC_GYRO_BACKGROUND);
    FeedbackTone(tFeedbackType);
}

