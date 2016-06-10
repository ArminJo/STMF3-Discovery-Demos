/*
 * GameOfLife.h
 *
* @date 07.02.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#ifndef GAMEOFLIFE_H_
#define GAMEOFLIFE_H_

#define BG_COLOR    (0)
#define ALIVE_COLOR (1)
#define DEAD_COLOR  (2)
#define DIE1_COLOR  (3)
#define DIE2_COLOR  (4)

#define GOL_MAX_GEN  (600) //max generations
#define GOL_X_SIZE   (40)
#define GOL_Y_SIZE   (30)

// upper nibble is new state, lower nibble is actual state
#define DEL_CELL     (0x03)
#define ON_CELL    	 (0x08)
#define NEW_CELL     (0x80)
#define NEW_DEL_CELL (0x40)

extern uint8_t (*frame)[GOL_Y_SIZE];

void init_gol(void);
void play_gol(void);
void draw_gol(void);
void drawGenerationText(void);
void ClearScreenAndDrawGameOfLifeGrid(void);

extern bool GolShowDying;

#endif /* GAMEOFLIFE_H_ */
