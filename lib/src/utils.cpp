/*
 * Pages.cpp
 *
 *  Created on: 15.12.2013
 *      Author: Armin
 */

#include "utils.h"
#include "main.h" // for StringBuffer
#include "timing.h"
#include "stm32fx0xPeripherals.h"

#include <math.h> // for pow etc.
#include <stdio.h> /* for sprintf */
#include <string.h> /* for strcpy */

static uint8_t LastSecond = 0;
/**
 * Refreshes display of time/date every second
 * @param x
 * @param y
 * @param aColor
 * @param aBackColor
 */
void showRTCTimeEverySecond(uint16_t x, uint16_t y, color16_t aFGColor, color16_t aBGColor) {
    uint8_t tSecond = RTC_getSecond();
    if (LastSecond != tSecond) {
        LastSecond = tSecond;
        //RTC
        RTC_getTimeString(sStringBuffer);
        BlueDisplay1.drawText(x, y, sStringBuffer, TEXT_SIZE_11, aFGColor, aBGColor);

    }
}

/*
 * shows RTC time if year != 0 or if aForceDisplay == true
 */
void showRTCTime(uint16_t x, uint16_t y, color16_t aFGColor, color16_t aBGColor, bool aForceDisplay) {
    RTC_getTimeString(sStringBuffer);
    if (RTC_DateIsValid || aForceDisplay) {
        BlueDisplay1.drawText(x, y, sStringBuffer, TEXT_SIZE_11, aFGColor, aBGColor);
    }
}

/**
 *
 * @param aDestPointer first position to overwrite
 * @param aSeparatorAddress position for THOUSANDS_SEPARATOR
 */
void formatThousandSeparator(char * aDestPointer, char * aSeparatorAddress) {
    // set separator for thousands
    char* tSrcPtr = aDestPointer + 1;
    while (tSrcPtr <= aSeparatorAddress) {
        *aDestPointer++ = *tSrcPtr++;
    }
    *aSeparatorAddress = THOUSANDS_SEPARATOR;
}

void printFloat(float aValue, int decimalDigits) {
    int i = 1;
    int tIntegerPart, tFractionalPart;
    for (; decimalDigits != 0; i *= 10, decimalDigits--)
        ;
    tIntegerPart = (int) aValue;
    tFractionalPart = (int) ((aValue - (float) tIntegerPart) * i);
    printf("%i.%i", tIntegerPart, tFractionalPart);
}

/**
 * Simple printf substitute for double
 * @param aValue
 * @param aFractionalFactor must be 10 for 1 digit after decimal point, 100 for 2 digits etc.
 */
void printDouble(double aValue, int aFractionalFactor) {
    double tIntegerPart;
    double tFractionalPart = modf(aValue, &tIntegerPart);
    printf("%i.%i", (int) tIntegerPart, (int) (tFractionalPart * aFractionalFactor));
}

//
//
//static double PRECISION = 0.00000000000001;
//static int MAX_NUMBER_STRING_SIZE = 32;
//
///**
// * Double to ASCII
// */
//char * dtoa(char *s, double n) {
//    // handle special cases
//    if (isnan(n)) {
//        strcpy(s, "nan");
//    } else if (isinf(n)) {
//        strcpy(s, "inf");
//    } else if (n == 0.0) {
//        strcpy(s, "0");
//    } else {
//        int digit, m, m1;
//        char *c = s;
//        int neg = (n < 0);
//        if (neg)
//            n = -n;
//        // calculate magnitude
//        m = log10(n);
//        int useExp = (m >= 14 || (neg && m >= 9) || m <= -9);
//        if (neg)
//            *(c++) = '-';
//        // set up for scientific notation
//        if (useExp) {
//            if (m < 0)
//               m -= 1.0;
//            n = n / pow(10.0, m);
//            m1 = m;
//            m = 0;
//        }
//        if (m < 1.0) {
//            m = 0;
//        }
//        // convert the number
//        while (n > PRECISION || m >= 0) {
//            double weight = pow(10.0, m);
//            if (weight > 0 && !isinf(weight)) {
//                digit = floor(n / weight);
//                n -= (digit * weight);
//                *(c++) = '0' + digit;
//            }
//            if (m == 0 && n > 0)
//                *(c++) = '.';
//            m--;
//        }
//        if (useExp) {
//            // convert the exponent
//            int i, j;
//            *(c++) = 'e';
//            if (m1 > 0) {
//                *(c++) = '+';
//            } else {
//                *(c++) = '-';
//                m1 = -m1;
//            }
//            m = 0;
//            while (m1 > 0) {
//                *(c++) = '0' + m1 % 10;
//                m1 /= 10;
//                m++;
//            }
//            c -= m;
//            for (i = 0, j = m-1; i<j; i++, j--) {
//                // swap without temporary
//                c[i] ^= c[j];
//                c[j] ^= c[i];
//                c[i] ^= c[j];
//            }
//            c += m;
//        }
//        *(c) = '\0';
//    }
//    return s;
//}

//char fstr[80];
//float num = 2.55f;
//int m = log10(num);
//int digit;
//float tolerance = .0001f;
//
//while (num > 0 + precision)
//{
//    float weight = pow(10.0f, m);
//    digit = floor(num / weight);
//    num -= (digit*weight);
//    *(fstr++)= '0' + digit;
//    if (m == 0)
//        *(fstr++) = '.';
//    m--;
//}
//*(fstr) = '\0';
