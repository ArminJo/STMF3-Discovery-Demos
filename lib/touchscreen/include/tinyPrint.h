/*
 * tinyPrint.h
 *
 * @date  05.1.2014
 * @author  Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 */
#ifndef TINYPRINT_h
#define TINYPRINT_h

#include <stdint.h>
#include <stdbool.h>


void printSetOptions(uint8_t aPrintSize, uint16_t aPrintColor, uint16_t aPrintBackgroundColor, bool aClearOnNewScreen);
int printNewline(void);
void printClearScreen(void);
void printSetPosition(int aPosX, int aPosY);
void printSetPositionColumnLine(int aColumnNumber, int aLineNumber);
int printNewline(void);
int myPrintf(const char *aFormat, ...);

#ifdef __cplusplus
extern "C" {
#endif
void myPrint(const char * StringPointer, int aLength);
#ifdef __cplusplus
}
#endif

#endif //TINYPRINT_h
