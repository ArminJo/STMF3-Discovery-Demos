/*
 * @file l3gdc20_lsm303dlhc_utils.h
 *
 *  Created on: 22.01.2014
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#ifndef L3GDC20_LSM303DLHC_UTILS_H_
#define L3GDC20_LSM303DLHC_UTILS_H_

#include <stdint.h>

#ifndef PI
#define PI                         (float)     3.14159265f
#endif

extern int16_t AccelerometerCompassRawDataBuffer[3];
extern float GyroscopeRawDataBuffer[3];
void readAccelerometerZeroCompensated(int16_t* aAccelerometerRawData);

void readGyroscopeZeroCompensated(float* aGyroscopeRawData);
void setZeroAccelerometerGyroValue(void);

float readLSM303TempFloat(void);

// Demo functions
void Demo_CompassReadAcc(float* pfData);

#endif /* L3GDC20_LSM303DLHC_UTILS_H_ */
