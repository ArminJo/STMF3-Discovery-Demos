/*
 * utils.h
 *
 *  Created on: 15.12.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#ifndef UTILS_H_
#define UTILS_H_

#include "BlueDisplay.h"
#include <stdint.h>

#define THOUSANDS_SEPARATOR '.'

void showRTCTime(uint16_t x, uint16_t y, color16_t aFGColor, color16_t aBGColor, bool aForceDisplay);
void showRTCTimeEverySecond(uint16_t x, uint16_t y, color16_t aFGColor, color16_t aBGColor);
void formatThousandSeparator(char * aDestPointer, char * aSeparatorAddress);

#endif /* UTILS_H_ */
