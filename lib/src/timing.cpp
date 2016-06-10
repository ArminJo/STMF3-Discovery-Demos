/**
 * @file timing.cpp
 *
 * @date 21.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 * Timing related functions like delays and systic handler
 */

#include "timing.h"
#include "myStrings.h"
#include "BlueDisplay.h"
#include "stm32fx0xPeripherals.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <time.h>
#include <core_cmInstr.h>

void doOneSystic(void);

//! for timeouts
volatile uint32_t TimeoutCounterForThread = 0;
volatile uint32_t TimeoutCounterForISR[NUMBER_OF_PREEMPTIVE_PRIOS] = { 0, 0, 0, 0 }; //! for timeout used by interrupt handlers - one for each preemptive prio lower than systic prio (gives 3)
//! for delays
volatile uint32_t TimingDelayForThread = 0;
volatile uint32_t TimingDelayForISR[NUMBER_OF_PREEMPTIVE_PRIOS] = { 0, 0, 0, 0 }; //! delay to be used by interrupt handlers (first 2 values are not really used, because delay for this prio levels is done by delayMillisBusy())

volatile uint32_t MillisSinceBoot = 0;

// 4 + 1 callbacks for accuCapacity
// 3 callbacks for Touch
// 1 callback for LCD dimming
// 1 callback for tone duration
// 1 one time callback for delayed MMC initialization
#define CALLBACK_ENTRIES_SIZE 11
// array of pointer to timer-delay callback function
void (*DelayCallbackPointer[CALLBACK_ENTRIES_SIZE])(void);
volatile int32_t DelayCallbackMillis[CALLBACK_ENTRIES_SIZE] = { 0, 0, 0, 0, 0 };

/**
 * loop delay of nanoseconds.
 * 0-249 ns gives 18 ticks for loading parameter and return without a loop
 * 250 gives 22 ticks for one loop
 * 29 ticks for 2 loops
 * 39 ticks for 3 loops
 * 49 ticks for 4 loops
 * => resolution is app. 139 ns
 * @par clocks used for every value:
 * - 1 - set Parameter
 * - 3 - branch to function
 * - 1 - load (PICOS_PER_CLOCK * 6) / 1000
 * - 1 - compare
 * - 1 or 3 - branch on compare result
 * - 1 - return
 *
 * - 1 - shift
 *  @par clocks used per loop
 * - 1 - sub
 * - 1 - nop
 * - 1 - compare
 * - 3 - branch
 */
#define CLOCK_TICKS_INITIAL 18
#define CLOCK_TICKS_PER_LOOP 10
void delayNanos(int32_t aTimeNanos) {
    // tTimeNanos not needed if compilation with -Os
    register int32_t tTimeNanos = aTimeNanos;
    tTimeNanos -= ((PICOS_PER_CLOCK * CLOCK_TICKS_INITIAL) / 1000); // 249
    if (tTimeNanos <= 0) {
        return;
    }
    // for better resolution
    tTimeNanos = tTimeNanos << 4;
    do {
        __NOP(); // to avoid removal of loop and function body for -Os compiler switch
        tTimeNanos = tTimeNanos - ((PICOS_PER_CLOCK * CLOCK_TICKS_PER_LOOP * 16) / 1000); //2222 or 139*16
    } while (tTimeNanos > 0);
}

/**
 * poll counted flag in systic status register.
 * @note Only for use if systic interrupt is disabled,
 * otherwise 2 calls of doOneSystic() happens per systic count.
 */
static void delayMillisBusy(int32_t aTimeMillis) {
    // reset systic counted flag
    hasSysticCounted();
    while (aTimeMillis != 0) {
        // detect reload of systic timer value
        if (hasSysticCounted()) {
            //Timer reload -> 1 ms elapsed
            doOneSystic();
            aTimeMillis--;
        }
#ifdef HAL_WWDG_MODULE_ENABLED
        Watchdog_reload();
#endif
    }
}

/**
 * register for delay callback, does not check if callback is already registered
 * if check is needed use changeDelayCallback()
 */
void registerDelayCallback(void (*aDelayCallback)(void), int32_t aTimeMillis) {
    int i;
    for (i = 0; i < CALLBACK_ENTRIES_SIZE; ++i) {
        if (DelayCallbackMillis[i] == 0) {
            DelayCallbackPointer[i] = aDelayCallback;
            DelayCallbackMillis[i] = aTimeMillis;
            return;
        }
    }
    failParamMessage(aDelayCallback, "No more free callback entries");
}

/**
 * change a delay for a already registered callback or insert one if not existent
 */
void changeDelayCallback(void (*aDelayCallback)(void), int32_t aTimeMillis) {
    int i;
    for (i = 0; i < CALLBACK_ENTRIES_SIZE; ++i) {
        if (DelayCallbackPointer[i] == aDelayCallback) {
            DelayCallbackMillis[i] = aTimeMillis;
            // entry found, job done
            return;
        }
    }
    // no entry found -> insert one
    registerDelayCallback(aDelayCallback, aTimeMillis);
}

/**
 * This function implements SysTick Handler.
 * Dims LCD after period of touch inactivity
 */
void doOneSystic(void) {
    /**
     * delays
     */
    if (TimingDelayForThread != 0) {
        TimingDelayForThread--;
    }
    int i;
    for (i = 0; i < NUMBER_OF_PREEMPTIVE_PRIOS; ++i) {
        if (TimingDelayForISR[i] != 0) {
            TimingDelayForISR[i]--;
        }
    }

    /**
     * delays with callback
     */
    for (i = 0; i < CALLBACK_ENTRIES_SIZE; ++i) {
        if (DelayCallbackMillis[i] > 0) {
            DelayCallbackMillis[i]--;
            if (DelayCallbackMillis[i] == 0) {
                DelayCallbackPointer[i]();
            }
        }
    }

    /**
     * timeouts
     */
    if (TimeoutCounterForThread != 0) {
        TimeoutCounterForThread--;
    }

    for (i = 0; i < NUMBER_OF_PREEMPTIVE_PRIOS; ++i) {
        if (TimeoutCounterForISR[i] != 0) {
            TimeoutCounterForISR[i]--;
        }
    }
}

void displayTimings(uint16_t aYDisplayPos) {
    uint32_t tSysticReloadValue = getSysticReloadValue() + 1;
    clearSystic(); // 1 tick
    Set_DebugPin(); // 1 tick
    Reset_DebugPin();
    Set_DebugPin();
    Reset_DebugPin();
    Set_DebugPin();
    Reset_DebugPin();
    Set_DebugPin();
    Reset_DebugPin();
    clearSystic(); // 1 tick
    Set_DebugPin(); // 1 tick
    Reset_DebugPin();
    Set_DebugPin();
    Reset_DebugPin();
    Set_DebugPin();
    Reset_DebugPin();
    Set_DebugPin();
    Reset_DebugPin();
    clearSystic(); // 1 tick
    Set_DebugPin(); // 1 tick
    Reset_DebugPin();
    Set_DebugPin();
    Reset_DebugPin();
    Set_DebugPin();
    Reset_DebugPin();
    Set_DebugPin();
    Reset_DebugPin();
    clearSystic(); // 1 tick
    Set_DebugPin(); // 1 tick
    Reset_DebugPin();
    Set_DebugPin();
    Reset_DebugPin();
    Set_DebugPin();
    Reset_DebugPin();
    Set_DebugPin();
    Reset_DebugPin();
    uint32_t tDuration = getSysticValue(); // needs 1 tick
    printf("\nSYSTICS: reload+1=%ld 16*set+res=%ld\n", tSysticReloadValue, tSysticReloadValue - tDuration);
    clearSystic();
    delayNanos(250 + NANOS_ONE_LOOP);
    delayNanos(250 + NANOS_ONE_LOOP);
    delayNanos(250 + NANOS_ONE_LOOP);
    delayNanos(250 + NANOS_ONE_LOOP);
    delayNanos(250 + NANOS_ONE_LOOP);
    delayNanos(250 + NANOS_ONE_LOOP);
    delayNanos(250 + NANOS_ONE_LOOP);
    delayNanos(250 + NANOS_ONE_LOOP);
    tDuration = getSysticValue();
    printf("2 loops=%ld", tSysticReloadValue - tDuration);
    clearSystic();
    delayNanos(250 + 2 * NANOS_ONE_LOOP);
    delayNanos(250 + 2 * NANOS_ONE_LOOP);
    delayNanos(250 + 2 * NANOS_ONE_LOOP);
    delayNanos(250 + 2 * NANOS_ONE_LOOP);
    delayNanos(250 + 2 * NANOS_ONE_LOOP);
    delayNanos(250 + 2 * NANOS_ONE_LOOP);
    delayNanos(250 + 2 * NANOS_ONE_LOOP);
    delayNanos(250 + 2 * NANOS_ONE_LOOP);
    tDuration = getSysticValue();
    printf(" 3=%ld", tSysticReloadValue - tDuration);
    clearSystic();
    delayNanos(250 + 3 * NANOS_ONE_LOOP);
    delayNanos(250 + 3 * NANOS_ONE_LOOP);
    delayNanos(250 + 3 * NANOS_ONE_LOOP);
    delayNanos(250 + 3 * NANOS_ONE_LOOP);
    delayNanos(250 + 3 * NANOS_ONE_LOOP);
    delayNanos(250 + 3 * NANOS_ONE_LOOP);
    delayNanos(250 + 3 * NANOS_ONE_LOOP);
    delayNanos(250 + 3 * NANOS_ONE_LOOP);
    tDuration = getSysticValue();
    printf(" 4=%ld", tSysticReloadValue - tDuration);
    clearSystic();
    delayNanos(250 + 4 * NANOS_ONE_LOOP);
    delayNanos(250 + 4 * NANOS_ONE_LOOP);
    delayNanos(250 + 4 * NANOS_ONE_LOOP);
    delayNanos(250 + 4 * NANOS_ONE_LOOP);
    delayNanos(250 + 4 * NANOS_ONE_LOOP);
    delayNanos(250 + 4 * NANOS_ONE_LOOP);
    delayNanos(250 + 4 * NANOS_ONE_LOOP);
    delayNanos(250 + 4 * NANOS_ONE_LOOP);
    tDuration = getSysticValue();
    printf(" 5=%ld", tSysticReloadValue - tDuration);
    clearSystic();
    delayNanos(250 + 7 * NANOS_ONE_LOOP);
    delayNanos(250 + 7 * NANOS_ONE_LOOP);
    delayNanos(250 + 7 * NANOS_ONE_LOOP);
    delayNanos(250 + 7 * NANOS_ONE_LOOP);
    delayNanos(250 + 7 * NANOS_ONE_LOOP);
    delayNanos(250 + 7 * NANOS_ONE_LOOP);
    delayNanos(250 + 7 * NANOS_ONE_LOOP);
    delayNanos(250 + 7 * NANOS_ONE_LOOP);
    tDuration = getSysticValue();
    printf(" 8=%ld\n", tSysticReloadValue - tDuration);
    printf("\nnext Line\n");
    printf(" Das ist jetzt wirklich mal ein seeeeeehr seeeeehr langer Test Text für die Ausgabe\n");
}

void testTimingsLoop(int aCount) {
    while (aCount > 0) {
        Set_DebugPin(); // 1 tick ???
        delayNanos(10); // call gives 18 ticks -> 250 ns
        Reset_DebugPin();
        delayNanos(10);
        Set_DebugPin();
        delayNanos(10);
        Reset_DebugPin();
        delayNanos(10);

        Set_DebugPin();
        delayNanos(250); // results in app. 330 ns
        Reset_DebugPin();
        delayNanos(250);
        Set_DebugPin();
        delayNanos(250);
        Reset_DebugPin();
        delayNanos(250);

        Set_DebugPin();
        delayNanos(417); // // results in app. 420 ns
        Reset_DebugPin();
        delayNanos(417);
        Set_DebugPin();
        delayNanos(417);
        Reset_DebugPin();
        delayNanos(417);

        Set_DebugPin();
        delayNanos(2000); // results in 2000
        Reset_DebugPin();
        delayNanos(2000);
        Set_DebugPin();
        delayNanos(2000);
        Reset_DebugPin();
        delayNanos(2000);

        Set_DebugPin();
        delayNanos(20000); // results in 20000
        Reset_DebugPin();
        delayNanos(20000);
        Set_DebugPin();
        delayNanos(20000);
        Reset_DebugPin();
        delayNanos(20000);
        aCount--;
    }
}

extern "C" void SysTick_Handler(void) {
    HAL_IncTick();
//  Toggle_DebugPin(); // to measure crystal by external counter
    MillisSinceBoot++;
    doOneSystic();
}

/**
 * @retval millis since start of program
 */
extern "C" uint32_t getMillisSinceBoot() {
    return MillisSinceBoot;
}

/**
 * wait for aTimeMillis milliseconds.
 * @param  aTimeMillis: specifies the delay time length, in 1 ms.
 */
extern "C" void delayMillis(int32_t aTimeMillis) {
    // get interrupt level
    uint32_t tIPSR = (__get_IPSR() & 0xFF);
    // in high priority system interrupt like bus_fault, systic etc.
    if (tIPSR > 0 && tIPSR < OFFSET_INTERRUPT_TYPE_TO_ISR_INT_NUMBER) {
        // no other delay possible
        delayMillisBusy(aTimeMillis);
    } else if (tIPSR == 0) {
        // No interrupt here
        if (TimingDelayForThread == 0) {
            // value is free to use
            TimingDelayForThread = aTimeMillis;
            // wait for variable changed by systic handler
            while (TimingDelayForThread != 0) {
#ifdef HAL_WWDG_MODULE_ENABLED
                Watchdog_reload();
#endif                // TODO FUTURE use sleep here
                ;
            }
        } else {
            failParamMessage(aTimeMillis, "delay in delay for thread mode");
        }
    } else {
        int tPreemptivePrio = NVIC_GetPriority((IRQn_Type) (tIPSR - OFFSET_INTERRUPT_TYPE_TO_ISR_INT_NUMBER));
        if (tPreemptivePrio > SYS_TICK_INTERRUPT_PRIO) {
            // Here in ISR with (logical) lower preemptive prio than systic handler => systic interrupt will happen
            // value is free to use
            TimingDelayForISR[tPreemptivePrio] = aTimeMillis;
            // wait for variable changed by systic handler
            while (TimingDelayForISR[tPreemptivePrio] != 0) {
#ifdef HAL_WWDG_MODULE_ENABLED
                Watchdog_reload();
#endif
            }
        } else {
            // Here in ISR with equal or higher priority than systic handler itself
            delayMillisBusy(aTimeMillis);
        }
    }
}

/**
 * Sets timeout value to be decremented by SysTick handler.
 * @param  aTimeMillis
 */
extern "C" void setTimeoutMillis(int32_t aTimeMillis) {
// get interrupt level
    uint32_t tISPR = (__get_IPSR() & 0xFF);
    if (tISPR == 0) {
        TimeoutCounterForThread = aTimeMillis;
    } else {
        int tPreemptivePrio = NVIC_GetPriority((IRQn_Type) (tISPR - OFFSET_INTERRUPT_TYPE_TO_ISR_INT_NUMBER));
        // Here in ISR
        TimeoutCounterForISR[tPreemptivePrio] = aTimeMillis;
        // reset systic counted flag
        hasSysticCounted();
    }
}

/**
 * Check if timeout period has expired. Displays a timeout message on screen if screen available.
 * @retval 1 if timeout period has expired, 0 else
 */
extern "C" bool isTimeoutVerbose(uint8_t* aFile, uint32_t aLine, uint32_t aValue, int32_t aMessageDisplayTimeMillis) {
// get interrupt level
    if (isTimeoutSimple()) {
        if (isLocalDisplayAvailable) {
            char * tFile = (strrchr(((char*) aFile), '/') + 1);
            snprintf(StringBuffer, sizeof StringBuffer, "Timeout on line: %lu %#X %u\nfile: %s", aLine,
                    (unsigned int) aValue, (unsigned int) aValue, tFile);
#ifdef LOCAL_DISPLAY_EXISTS
            BlueDisplay1.drawMLText(0, TEXT_SIZE_11_ASCEND, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_WHITE);
#else
            BlueDisplay1.drawText(0, TEXT_SIZE_11_ASCEND, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_WHITE);
#endif
            delayMillis(aMessageDisplayTimeMillis);
        }
        return true;
    }
    return false;
}

/**
 * Check if timeout period has expired.
 * for high priority level, assume that it is called at least once every millis since it must manually check for hasSysticCounted()
 * @retval true if timeout period has expired, false else
 */
// for use in syscalls.c
extern "C" bool isTimeoutSimple(void) {
// get interrupt level
    bool tRetval = false;
    uint32_t tISPR = (__get_IPSR() & 0xFF);
    if (tISPR == 0) {
        // thread
        if (TimeoutCounterForThread == 0) {
            setTimeoutLED();
            tRetval = true;
        }
    } else {
        // in interrupt service routine
        int tPreemptivePrio = NVIC_GetPriority((IRQn_Type) (tISPR - OFFSET_INTERRUPT_TYPE_TO_ISR_INT_NUMBER));
        if (tPreemptivePrio <= SYS_TICK_INTERRUPT_PRIO) {
            // with higher or same master (preemptive) priority than system clock
            if (hasSysticCounted()) {
                //Timer reload -> 1 ms elapsed
                doOneSystic();
            }
        }
        if (TimeoutCounterForISR[tPreemptivePrio] == 0) {
            setTimeoutLED();
            tRetval = true;
        }
    }
#ifdef HAL_WWDG_MODULE_ENABLED
    Watchdog_reload();
#endif
    return tRetval;
}
/**
 * Sets timeout LED.
 * @retval 0
 */
extern "C" uint32_t LSM303DLHC_TIMEOUT_UserCallback(void) {
    setTimeoutLED();
    return 0;
}

/**
 * Sets timeout LED.
 * @retval 0
 */
extern "C" uint32_t L3GD20_TIMEOUT_UserCallback(void) {
    setTimeoutLED();
    return 0;
}
