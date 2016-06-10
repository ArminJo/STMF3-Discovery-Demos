/**
 * @file stm32f3DiscoPeripherals.cpp
 *
 * @date 05.12.2012
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#include "timing.h"
#include "stm32f3DiscoPeripherals.h"
#include "stm32fx0xPeripherals.h"

extern "C" {
#include "stm32f3_discovery_accelerometer.h"
#include "stm32f3_discovery_gyroscope.h"
//#include "stm32f30x_tim.h"
#include "stm32f3_discovery.h"
}

#include <stdio.h>
/** @addtogroup Peripherals_Library
 * @{
 */

void ACCELERO_IO_Read(uint8_t* pBuffer, uint8_t ReadAddr, uint16_t NumByteToRead) {
    for (int i = 0; i < NumByteToRead; ++i) {
        *pBuffer++ = COMPASSACCELERO_IO_Read(ACC_I2C_ADDRESS, ReadAddr++);
    }
}

void COMPASS_IO_Read(uint8_t* pBuffer, uint8_t ReadAddr, uint16_t NumByteToRead) {
    for (int i = 0; i < NumByteToRead; ++i) {
        *pBuffer++ = COMPASSACCELERO_IO_Read(MAG_I2C_ADDRESS, ReadAddr++);
    }
}

void readCompassRaw(int16_t aCompassRawData[]) {
    uint8_t ctrlx[2];
    uint8_t cDivider;
    uint8_t i = 0;

    /* Read the register content */
    ACCELERO_IO_Read(ctrlx, LSM303DLHC_CTRL_REG4_A, 2);
    COMPASS_IO_Read((uint8_t *) aCompassRawData, LSM303DLHC_OUT_X_H_M, 6);

    if (ctrlx[1] & 0x40)
        // High resolution mode
        cDivider = 64;
    else
        cDivider = 16;

    for (i = 0; i < 3; i++) {
        aCompassRawData[i] = aCompassRawData[i] / cDivider;
    }
}

/*
 * 12 bit resolution 8 LSB/Centigrade
 */
int16_t readLSM303TempRaw(void) {
    uint8_t ctrlx[2];

    /* Read the register content */
//    LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_TEMP_OUT_H_M, ctrlx, 2);
    COMPASS_IO_Read(ctrlx, LSM303DLHC_TEMP_OUT_H_M, 2);

    return ctrlx[0] << 4 | ctrlx[1] >> 4;
//    return ctrlx[0] << 8 | ctrlx[1];
}
/** @} */
