/*
 * @file l3gdc20_lsm303dlhc_utils.cpp
 *
 *  Created on: 22.01.2014
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#include "timing.h"
#include "l3gdc20_lsm303dlhc_utils.h"
#include "stm32f3DiscoPeripherals.h"

extern "C" {
#include "stm32f3_discovery_accelerometer.h"
#include "stm32f3_discovery_gyroscope.h"
}

static int16_t AccelerometerZeroCompensation[3];
int16_t AccelerometerCompassRawDataBuffer[3];
static float GyroscopeZeroCompensation[3];
float GyroscopeRawDataBuffer[3];

#define LSM303DLHC_CALIBRATION_OVERSAMPLING 8

void readAccelerometerZeroCompensated(int16_t * aAccelerometerRawData) {
    BSP_ACCELERO_GetXYZ(aAccelerometerRawData);
    // invert X value, it fits better :-)
    aAccelerometerRawData[0] = -aAccelerometerRawData[0];
    aAccelerometerRawData[0] -= AccelerometerZeroCompensation[0];
    aAccelerometerRawData[1] -= AccelerometerZeroCompensation[1];
    aAccelerometerRawData[2] -= AccelerometerZeroCompensation[2];
}

void readGyroscopeZeroCompensated(float aGyroscopeRawData[]) {
    BSP_GYRO_GetXYZ(aGyroscopeRawData);
    aGyroscopeRawData[0] -= GyroscopeZeroCompensation[0];
    aGyroscopeRawData[1] -= GyroscopeZeroCompensation[1];
    aGyroscopeRawData[2] -= GyroscopeZeroCompensation[2];
}

void setZeroAccelerometerGyroValue(void) {
    // use oversampling
    AccelerometerZeroCompensation[0] = 0;
    AccelerometerZeroCompensation[1] = 0;
    AccelerometerZeroCompensation[2] = 0;
    GyroscopeZeroCompensation[0] = 0;
    GyroscopeZeroCompensation[1] = 0;
    GyroscopeZeroCompensation[2] = 0;
    int i = 0;
    for (i = 0; i < LSM303DLHC_CALIBRATION_OVERSAMPLING; ++i) {
        // Accelerometer
        BSP_ACCELERO_GetXYZ(AccelerometerCompassRawDataBuffer);
        // invert X value, it fits better :-)
        AccelerometerZeroCompensation[0] = -AccelerometerZeroCompensation[0];
        AccelerometerZeroCompensation[0] += AccelerometerCompassRawDataBuffer[0];
        AccelerometerZeroCompensation[1] += AccelerometerCompassRawDataBuffer[1];
        AccelerometerZeroCompensation[2] += AccelerometerCompassRawDataBuffer[2];
        // Gyroscope
        BSP_GYRO_GetXYZ(GyroscopeRawDataBuffer);
        GyroscopeZeroCompensation[0] += GyroscopeRawDataBuffer[0];
        GyroscopeZeroCompensation[1] += GyroscopeRawDataBuffer[1];
        GyroscopeZeroCompensation[2] += GyroscopeRawDataBuffer[2];
        delay(10);
    }
    AccelerometerZeroCompensation[0] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
    AccelerometerZeroCompensation[1] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
    AccelerometerZeroCompensation[2] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
    GyroscopeZeroCompensation[0] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
    GyroscopeZeroCompensation[1] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
    GyroscopeZeroCompensation[2] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
}

float readLSM303TempFloat(void) {
    float tTemp = readLSM303TempRaw();
    return tTemp / 2;
}
