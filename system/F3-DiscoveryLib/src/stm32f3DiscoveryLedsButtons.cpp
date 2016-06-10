/*
 * stm32f3DiscoveryLedsButtons.cpp
 *
 * @date 02.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */
extern "C" {
#include "stm32f3_discovery.h"
extern void storeScreenshot(void);
}
#include "timing.h"

void initializeLEDs(void) {
    /* Initialize LEDs and User Button available on STM32F3-Discovery board */
    BSP_LED_Init(LED3);
    BSP_LED_Init(LED4);
    BSP_LED_Init(LED5);
    BSP_LED_Init(LED6);
    BSP_LED_Init(LED7);
    BSP_LED_Init(LED8);
    BSP_LED_Init(LED9);
    BSP_LED_Init(LED10);
}

void allLedsOff(void) {
    /* LEDs Off */
    BSP_LED_Off(LED3);
    BSP_LED_Off(LED6);
    BSP_LED_Off(LED7);
    BSP_LED_Off(LED4);
    BSP_LED_Off(LED10);
    BSP_LED_Off(LED8);
    BSP_LED_Off(LED9);
    BSP_LED_Off(LED5);
}

void CycleLEDs(void) {
    /* Toggle LD3 */
    BSP_LED_Toggle(LED3); // RED BACK
    /* Insert 50 ms delay */
    delayMillis(50);
    /* Toggle LD5 */
    BSP_LED_Toggle(LED5); // ORANGE BACK
    /* Insert 50 ms delay */
    delayMillis(50);
    /* Toggle LD7 */
    BSP_LED_Toggle(LED7); // GREEN RIGHT
    /* Insert 50 ms delay */
    delayMillis(50);
    /* Toggle LD9 */
    BSP_LED_Toggle(LED9); // BLUE FRONT
    /* Insert 50 ms delay */
    delayMillis(50);
    /* Toggle LD10 */
    BSP_LED_Toggle(LED10); // RED FRONT
    /* Insert 50 ms delay */
    delayMillis(50);
    /* Toggle LD8 */
    BSP_LED_Toggle(LED8); // ORANGE FRONT
    /* Insert 50 ms delay */
    delayMillis(50);
    /* Toggle LD6 */
    BSP_LED_Toggle(LED6); // GREEN LEFT
    /* Insert 50 ms delay */
    delayMillis(50);
    /* Toggle LD4 */
    BSP_LED_Toggle(LED4); // BLUE BACK
    /* Insert 50 ms delay */
    delayMillis(50);
}

/**
 * @brief  This function handles EXTI0_IRQ Handler for User button
 * Input is active high, and will be discharged by a resistor.
 * @retval None
 */
extern "C" void EXTI0_IRQHandler(void) {

    if ((__HAL_GPIO_EXTI_GET_IT(USER_BUTTON_PIN) != RESET) && (BSP_PB_GetState(BUTTON_USER) != RESET)) {
        /* Delay */
        delayMillis(5);

        // Wait for SEL button to be pressed (high)
        setTimeoutMillis(10);
        while (BSP_PB_GetState(BUTTON_USER) == RESET) {
            if (isTimeout(0)) {
                break;
            }
        }
        storeScreenshot();
    }
    /* Clear the EXTI line pending bit */
    __HAL_GPIO_EXTI_CLEAR_IT(USER_BUTTON_PIN);
}
