/*
 * thickLine.h
 *
 * @date 25.03.2013
 * @author  Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#ifndef THICKLINE_H_
#define THICKLINE_H_

#include <stdint.h>

#define LINE_OVERLAP_NONE 0 	// No line overlap
#define LINE_OVERLAP_MAJOR 0x01 // Overlap - first go major then minor direction
#define LINE_OVERLAP_MINOR 0x02 // Overlap - first go minor then major direction
#define LINE_OVERLAP_BOTH 0x03  // Overlap - both
#define LINE_THICKNESS_MIDDLE 0
#define LINE_THICKNESS_DRAW_CLOCKWISE 1
#define LINE_THICKNESS_DRAW_COUNTERCLOCKWISE 2

#ifdef __cplusplus
extern "C" {
#endif

void drawLineOverlap(unsigned int aXStart, unsigned int aYStart, unsigned int aXEnd, unsigned int aYEnd, uint8_t aOverlap, uint16_t aColor);
void drawThickLine(unsigned int aXStart, unsigned int aYStart, unsigned int aXEnd, unsigned int aYEnd, unsigned int aThickness, uint8_t aThicknessMode,
        uint16_t aColor);
void drawThickLineSimple(unsigned int aXStart, unsigned int aYStart, unsigned int aXEnd, unsigned int aYEnd, unsigned int aThickness, uint8_t aThicknessMode,
        uint16_t aColor);

#ifdef __cplusplus
}
#endif

#endif /* THICKLINE_H_ */
