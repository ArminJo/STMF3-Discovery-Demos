/*
 * @file stm32f3DiscoveryLedsButtons.h
 *
 * @date 02.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

/**
 * LED   | COLOR / POS  | SOURCE                | INFO
 * ------|--------------|-----------------------|--------
 * LED4  | BLUE BACK    | ADS7846 line was active but values could not be read correctly (e.g. because of delay by higher prio interrupts) |
 * LED6  | GREEN LEFT   | Assertion but display not available |
 * LED7  | GREEN RIGHT  | USART                 | USART error / TX Buffer overrun
 * LED8  | ORANGE FRONT | Timer                 | Timeout / Watchdog
 * LED9  | BLUE FRONT   | Events                | Touch Down Event received
 * LED10 | RED FRONT    | EXTI4                 | MMC Card inserted
 */

#ifndef STM32F3_DISCOVERY_LEDS_H_
#define STM32F3_DISCOVERY_LEDS_H_

extern volatile uint32_t UserButtonPressed;

void initializeLEDs(void);
void allLedsOff(void);
void CycleLEDs(void);
void toggleAllLeds(void);

#endif /* STM32F3_DISCOVERY_LEDS_H_ */
