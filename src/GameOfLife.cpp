/**
 Game Of Life (Display)
 */

#include "BlueDisplay.h"
#include "GameOfLife.h"
#include "main.h" // for StringBuffer
#include "timing.h"

#include "stdlib.h"
#include <stdio.h>

uint16_t generation = 0;
uint16_t drawcolor[5];

uint8_t (*frame)[GOL_Y_SIZE];

uint8_t alive(uint8_t x, uint8_t y) {
    if ((x < GOL_X_SIZE)&& (y < GOL_Y_SIZE)){
    if (frame[x][y] & ON_CELL) {
        return 1;
    }
}

return 0;
}

uint8_t neighbors(uint8_t x, uint8_t y) {
    uint8_t count = 0;

    //3 above
    if (alive(x - 1, y - 1)) {
        count++;
    }
    if (alive(x, y - 1)) {
        count++;
    }
    if (alive(x + 1, y - 1)) {
        count++;
    }

    //2 on each side
    if (alive(x - 1, y)) {
        count++;
    }
    if (alive(x + 1, y)) {
        count++;
    }

    //3 below
    if (alive(x - 1, y + 1)) {
        count++;
    }
    if (alive(x, y + 1)) {
        count++;
    }
    if (alive(x + 1, y + 1)) {
        count++;
    }

    return count;
}

void play_gol(void) {
    uint8_t x, y, count;

    //update cells
    for (x = 0; x < GOL_X_SIZE; x++) {
        for (y = 0; y < GOL_Y_SIZE; y++) {
            count = neighbors(x, y);
            if ((count == 3) && !alive(x, y)) {
                frame[x][y] = NEW_CELL; //new cell
            }
            if (((count < 2) || (count > 3)) && alive(x, y)) {
                frame[x][y] |= NEW_DEL_CELL; //cell dies
            }
        }
    }

    //increment generation
    if (++generation > GOL_MAX_GEN) {
        init_gol();
    }
}

void draw_gol(void) {
    uint8_t c, x, y, color = 0;
    uint16_t px, py;

    for (x = 0, px = 0; x < GOL_X_SIZE; x++) {
        for (y = 0, py = 0; y < GOL_Y_SIZE; y++) {
            c = frame[x][y];
            if (c) {
                if (c & NEW_CELL) { //new
                    frame[x][y] = ON_CELL;
                    color = ALIVE_COLOR;
                } else if (c & NEW_DEL_CELL) { //die 1. time -> clear alive Bits and decrease die count
                    frame[x][y] = 2;
                    color = DIE2_COLOR;
                } else if (c == 2) { //die
                    frame[x][y] = 1;
                    color = DIE1_COLOR;
                } else if (c == 1) { //delete
                    frame[x][y] = 0;
                    color = DEAD_COLOR;
                }
                if (c != ON_CELL) {
                    BlueDisplay1.fillRect(px + 1, py + 1,
                            px
                                    + (BlueDisplay1.getDisplayWidth() / GOL_X_SIZE)- 2, py + (BlueDisplay1.getDisplayHeight() / GOL_Y_SIZE) - 2, drawcolor[color]);

}                        }
            py += (BlueDisplay1.getDisplayHeight() / GOL_Y_SIZE);
        }
        px += (BlueDisplay1.getDisplayWidth() / GOL_X_SIZE);
    }
}

/**
 * switch color scheme
 */
void init_gol(void) {
    int x, y;
    uint32_t c;

    generation = 0;

    //change color
    drawcolor[DEAD_COLOR] = RGB( 255, 255, 255);

    switch (drawcolor[ALIVE_COLOR]) {
    case RGB( 0,255, 0): //RGB(255,  0,  0)
        drawcolor[BG_COLOR] = RGB( 0, 180, 180);
        drawcolor[ALIVE_COLOR] = RGB(255, 0, 0);
        drawcolor[DIE1_COLOR] = RGB( 130, 0, 0);
        drawcolor[DIE2_COLOR] = RGB(180, 0, 0);
        break;

    case RGB( 0, 0,255): //RGB(  0,255,  0)
        drawcolor[BG_COLOR] = RGB( 180, 0, 180);
        drawcolor[ALIVE_COLOR] = RGB( 0,255, 0);
        drawcolor[DIE1_COLOR] = RGB( 0, 130, 0);
        drawcolor[DIE2_COLOR] = RGB( 0,180, 0);
        break;

    default: //RGB(  0,  0,255)
        drawcolor[BG_COLOR] = RGB(180, 180, 0);
        drawcolor[ALIVE_COLOR] = RGB( 0, 0,255);
        drawcolor[DIE1_COLOR] = RGB( 0, 0, 130);
        drawcolor[DIE2_COLOR] = RGB( 0, 0,180);
        break;
    }
    if (!GolShowDying) {
        // change colors
        drawcolor[DIE1_COLOR] = RGB( 255, 255, 255);
        drawcolor[DIE2_COLOR] = RGB( 255, 255, 255);
    }

    //generate random start data
    srand(getMillisSinceBoot());
    for (x = 0; x < GOL_X_SIZE; x++) {
        c = (rand() | (rand() << 16)) & 0xAAAAAAAA; //0xAAAAAAAA 0x33333333 0xA924A924
        for (y = 0; y < GOL_Y_SIZE; y++) {
            if (c & (1 << y)) {
                frame[x][y] = ON_CELL;
            } else {
                frame[x][y] = 0;
            }
        }
    }
    ClearScreenAndDrawGameOfLifeGrid();

}

void ClearScreenAndDrawGameOfLifeGrid(void) {
    int px, py;
    int x, y;

    BlueDisplay1.clearDisplay(drawcolor[BG_COLOR]);
    //clear cells
    for (x = 0, px = 0; x < GOL_X_SIZE; x++) {
        for (y = 0, py = 0; y < GOL_Y_SIZE; y++) {
            BlueDisplay1.fillRect(px + 1, py + 1,
                    px - 2 + (BlueDisplay1.getDisplayWidth() / GOL_X_SIZE), py + (BlueDisplay1.getDisplayHeight() / GOL_Y_SIZE) - 2,
                    drawcolor[DEAD_COLOR]);
                    py += (BlueDisplay1.getDisplayHeight() / GOL_Y_SIZE);
                }
                px += (BlueDisplay1.getDisplayWidth() / GOL_X_SIZE);
            }
        }

void drawGenerationText(void) {
    //draw current generation
    snprintf(sStringBuffer, sizeof sStringBuffer, "Gen.%3d", generation);
    BlueDisplay1.drawText(0, TEXT_SIZE_11_ASCEND, sStringBuffer, TEXT_SIZE_11, RGB(50,50,50), drawcolor[DEAD_COLOR]);
}

void test(void) {
    frame[2][2] = ON_CELL;
    frame[3][2] = ON_CELL;
    frame[4][2] = ON_CELL;

    frame[6][2] = ON_CELL;
    frame[7][2] = ON_CELL;
    frame[6][3] = ON_CELL;
    frame[7][3] = ON_CELL;
}
