/*
 * timing.h
 *
 * @date 05.12.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#ifndef TIMING_H_
#define TIMING_H_

#include "AssertErrorAndMisc.h" // for usage of timeouts

#include <stdint.h>
#include <stdbool.h>

#ifdef STM32F10X
#include <stm32f1xx.h>
#endif
#ifdef STM32F30X
#include <stm32f3xx.h>
#endif

__STATIC_INLINE uint32_t getSysticValue(void) {
    return SysTick->VAL;
}
__STATIC_INLINE uint32_t getSysticReloadValue(void) {
    return SysTick->LOAD;
}
__STATIC_INLINE void clearSystic(void) {
    SysTick->VAL = 0;
}
__STATIC_INLINE bool hasSysticCounted(void) {
    return (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk);
}

// some microsecond values for timing
#define ONE_SECOND 1000
#define TWO_SECONDS 2000
#define FIVE_SECONDS 5000
#define TEN_SECONDS 10000
#define THIRTY_SECONDS 30000
#define ONE_MINUTE 60000
#define TWO_MINUTES 120000
#define TEN_MINUTES 600000

#define SYSCLK_VALUE (HSE_VALUE*9) // compile time value is 72 MHZ / runtime value is RCC_Clocks.SYSCLK_Frequency
#define NANOS_PER_CLOCK (1000000000/SYSCLK_VALUE)
#define PICOS_PER_CLOCK (1000000000000/SYSCLK_VALUE)

#define OFFSET_INTERRUPT_TYPE_TO_ISR_INT_NUMBER 0x10
// Value for set...Milliseconds in order to disable Timer
#define DISABLE_TIMER_DELAY_VALUE (0)

#define SYS_TICK_INTERRUPT_PRIO 0x08
#define NUMBER_OF_PREEMPTIVE_PRIOS (1 << __NVIC_PRIO_BITS)

extern volatile uint32_t TimeoutCounterForThread;

#define isTimeoutShowDelay(aLinkRegister, aMessageDisplayTimeMillis) (isTimeoutVerbose((uint8_t *)__FILE__, __LINE__,aLinkRegister,aMessageDisplayTimeMillis))

void delayNanos(int32_t aTimeNanos);

void registerDelayCallback(void (*aGenericCallback)(void), int32_t aTimeMillis);
void changeDelayCallback(void (*aGenericCallback)(void), int32_t aTimeMillis);

void doOneSystic(void);

#define NANOS_ONE_LOOP 139
void displayTimings(uint16_t aYDisplayPos);
void testTimingsLoop(int aCount);

#ifdef __cplusplus
extern "C" {
#endif
uint32_t getMillisSinceBoot(void);
void delayMillis(int32_t aTimeMillis);

void setTimeoutMillis(int32_t aTimeMillis);
#define isTimeout(aValue) (isTimeoutVerbose((uint8_t *)__FILE__, __LINE__,aValue,2000))
bool isTimeoutVerbose(uint8_t* file, uint32_t line, uint32_t aValue, int32_t aMessageDisplayTimeMillis);
// for use in syscalls.c
bool isTimeoutSimple(void);

uint32_t LSM303DLHC_TIMEOUT_UserCallback(void);
uint32_t L3GD20_TIMEOUT_UserCallback(void);
#ifdef __cplusplus
}
#endif

#endif /* TIMING_H_ */
