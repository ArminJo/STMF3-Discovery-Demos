/**
 * @file stm32f3DiscoPeripherals.h
 *
 * @date 05.12.2012
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#ifndef STM32F3DISCOPERIPHERALS_H_
#define STM32F3DISCOPERIPHERALS_H_

#include <stm32f3xx.h>

void readGyroscopeRaw(int16_t* aGyroscopeRawData);

void readAccelerometerRaw(int16_t aAccelerometerRawData[]);

void readCompassRaw(int16_t * aCompassRawData);
int16_t readLSM303TempRaw(void);

/** @} */

#endif /* STM32F3DISCOPERIPHERALS_H_ */
