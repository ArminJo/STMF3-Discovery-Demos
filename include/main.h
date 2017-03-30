/*
 * main.h
 *
 * @date 15.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */

#ifndef MAIN_H_
#define MAIN_H_

/*
 * Pin usage see lib/include/stm32fx0xPeripherals.h
 */

#include "ff.h"
extern FATFS Fatfs[1];

/**
 * Miscellaneous buffers
 */
// for printf etc.
#ifdef AVR
#define SIZEOF_STRINGBUFFER 50
#else
#define SIZEOF_STRINGBUFFER 240
#endif
extern char sStringBuffer[SIZEOF_STRINGBUFFER];

#endif /* MAIN_H_ */
