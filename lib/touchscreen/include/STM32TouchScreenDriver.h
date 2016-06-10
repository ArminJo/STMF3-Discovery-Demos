/*
 * STM32TouchScreenDriver.h
 *
 *  Created on: 09.04.2015
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 * Port pin assignment
 * -------------------
 *
 * Port |Pin|Device |Function
 * -----|---|-------|--------
 * B    |0  |HY32D      |CS
 * B    |1  |ADS7846    |INT input
 * B    |2  |ADS7846    |CS
 * B    |4  |HY32D      |DATA
 * B    |5  |HY32D      |WR
 * B    |10 |HY32D      |RD
 * F    |6  |HY32D      |TIM4 PWM output
 *
 *
 *
 *  Timer usage
 * -------------------
 *
 * Timer    |Function
 * ---------|--------
 * 4        | PWM backlight led -> Pin F6

 *
 *  Interrupt priority (lower value is higher priority)
 * ---------------------
 * 2 bits for pre-emption priority + 2 bits for subpriority
 *
 * Prio | ISR Nr| Name                  | Usage
 * -----|-------------------------------|-------------
 * 3 0  | 0x17 | EXTI1_IRQn             | Touch
 *
 */

#ifndef INCLUDE_STM32TOUCHSCREENDRIVER_H_
#define INCLUDE_STM32TOUCHSCREENDRIVER_H_

#include <stdint.h>

#define HY32D_CS_PIN                          GPIO_PIN_0
#define HY32D_CS_GPIO_PORT                    GPIOB

#define HY32D_DATA_CONTROL_PIN                GPIO_PIN_4
#define HY32D_DATA_CONTROL_GPIO_PORT          GPIOB  // dedicated Ports are needed by HY32D.cpp for single line setting
#define HY32D_RD_PIN                          GPIO_PIN_10
#define HY32D_RD_GPIO_PORT                    GPIOB

#define HY32D_WR_PIN                          GPIO_PIN_5
#define HY32D_WR_GPIO_PORT                    GPIOB

#define HY32D_DATA_GPIO_PORT                  GPIOD

#define ADS7846_CS_PIN                           GPIO_PIN_2
#define ADS7846_CS_GPIO_PORT                     GPIOB

#define ADS7846_EXTI_PIN                         GPIO_PIN_1
#define ADS7846_EXTI_GPIO_PORT                   GPIOB

typedef enum {
    LOW = 0, HIGH
} IOLevel;

void MI0283QT2_IO_initalize(void);

void ADS7846_IO_initalize(void);
void ADS7846_clearAndEnableInterrupt(void);
void ADS7846_disableInterrupt(void);
#define ADS7846_CSEnable() (ADS7846_CS_GPIO_PORT->BSRRH = ADS7846_CS_PIN)
#define ADS7846_CSDisable() (ADS7846_CS_GPIO_PORT->BSRRL = ADS7846_CS_PIN)
/**
 * @return true if line is NOT active
 */
#define ADS7846_getInteruptLineLevel() (HAL_GPIO_ReadPin(ADS7846_EXTI_GPIO_PORT, ADS7846_EXTI_PIN))
#define ADS7846_ClearITPendingBit() (__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1))

/*
 * PWM
 */
void PWM_BL_initalize(void);
void PWM_BL_setOnRatio(uint32_t power);

#endif /* INCLUDE_STM32TOUCHSCREENDRIVER_H_ */
