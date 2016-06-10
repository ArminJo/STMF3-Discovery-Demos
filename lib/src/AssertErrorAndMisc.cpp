/*
 * AssertErrorAndMisc.cpp
 *
 * @date 21.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#include "AssertErrorAndMisc.h"
#include "stm32fx0xPeripherals.h" // for Watchdog_reload()
#include "BlueDisplay.h"
#include "BlueSerial.h" // for UART_BD_IRQHANDLER()
#include "timing.h"
#include "myStrings.h" // for StringBuffer

#ifdef USE_STM32F3_DISCO
#include "stm32f3_discovery.h"
#endif

#include <stdio.h> /* for sprintf */
#include <string.h> // for strrchr
#include <stdlib.h> // for malloc

int sLockCount = 0; // counts skipped drawChars because of display locks

int DebugValue1;
int DebugValue2;
int DebugValue3;
int DebugValue4;
int DebugValue5;

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  aFile: pointer to the source file name
 * @param  aLine: assert_param error line source number
 * @retval false
 */
extern "C" void assert_failed(uint8_t* aFile, uint32_t aLine) {
    if (isLocalDisplayAvailable) {
        char * tFile = (strrchr(reinterpret_cast<char*>(aFile), '/') + 1);
        snprintf(StringBuffer, sizeof StringBuffer, "Wrong parameters value on line: %lu\nfile: %s", aLine, tFile);
#ifdef LOCAL_DISPLAY_EXISTS
        BlueDisplay1.drawMLText(0, ASSERT_START_Y, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_WHITE);
#else
        BlueDisplay1.drawText(0, ASSERT_START_Y, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_WHITE);
#endif
        delayMillis(2000);
    } else {
        /* Infinite loop */
        while (1) {
#ifdef USE_STM32F3_DISCO
            BSP_LED_Toggle(LED_GREEN); // GREEN LEFT
#endif
            /* Insert delay */
            delayMillis(500);
        }
    }
}

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 *         usage: assertFailedMessageParam((uint8_t *) __FILE__, __LINE__, getLR14(), "Message", tValue);
 * @param aFile pointer to the source file name
 * @param aLine
 * @param aLinkRegister
 * @param aMessage
 * @param aWrongParameter
 * @retval false
 *
 */
extern "C" void assertFailedParamMessage(uint8_t* aFile, uint32_t aLine, uint32_t aLinkRegister, int aWrongParameter,
        const char * aMessage) {
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    if (isLocalDisplayAvailable) {
        char * tFile = (strrchr(reinterpret_cast<char*>(aFile), '/') + 1);
        snprintf(StringBuffer, sizeof StringBuffer, "%s on line: %lu\nfile: %s\nLR=%#X val: %#X %d", aMessage, aLine,
                tFile, (unsigned int) aLinkRegister, aWrongParameter, aWrongParameter);
#ifdef LOCAL_DISPLAY_EXISTS
        // reset lock (just in case...)
        sDrawLock = 0;
        BlueDisplay1.drawMLText(0, ASSERT_START_Y, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_WHITE);
#else
        BlueDisplay1.drawText(0, ASSERT_START_Y, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_WHITE);
#endif
        delayMillis(2000);
    } else {
        /* Infinite loop */
        while (1) {
#ifdef USE_STM32F3_DISCO
            BSP_LED_Toggle(LED_GREEN); // GREEN LEFT
#endif
            /* Insert delay */
            delayMillis(500);
        }
    }
}

extern "C" void printError(uint8_t* aFile, uint32_t aLine) {
    if (isLocalDisplayAvailable) {
        char * tFile = (strrchr(reinterpret_cast<char*>(aFile), '/') + 1);
        snprintf(StringBuffer, sizeof StringBuffer, "Error on line: %lu\nfile: %s", aLine, tFile);
#ifdef LOCAL_DISPLAY_EXISTS
        // reset lock (just in case...)
        sDrawLock = 0;
#endif
        BlueDisplay1.drawText(0, ASSERT_START_Y, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_WHITE);
        delayMillis(2000);
    } else {
        /* Infinite loop */
        while (1) {
#ifdef USE_STM32F3_DISCO
            BSP_LED_Toggle(LED_GREEN); // GREEN LEFT
#endif
            /* Insert delay */
            delayMillis(500);
        }
    }
}

extern "C" void errorMessage(const char * aMessage) {
    if (isLocalDisplayAvailable) {
#ifdef LOCAL_DISPLAY_EXISTS
        BlueDisplay1.drawMLText(0, ASSERT_START_Y, aMessage, TEXT_SIZE_11, COLOR_RED, COLOR_WHITE);
#else
        BlueDisplay1.drawText(0, ASSERT_START_Y, aMessage, TEXT_SIZE_11, COLOR_RED, COLOR_WHITE);
#endif
    }
}

extern char estack asm("_estack");
#ifdef STM32F30X
extern char HeapStart asm("_sccmram");
#else
extern char HeapStart asm("_ebss");
#endif
extern uint32_t stext asm("_stext");
extern uint32_t srodata asm("_srodata");
//extern uint32_t etext asm("_etext");

void printStackpointerAndStacktrace(int aStackEntriesToSkip, int aStartLine) {
    /*
     * Stackpointer and stacktrace
     */
    BlueDisplay1.setPrintfPositionColumnLine(0, aStartLine);

    char* tHeapEnd = (char*) (malloc(16));
    free(tHeapEnd);
    uint32_t* tStackpointer = (uint32_t*) (__get_MSP());
    // (tHeapEnd - 8) since 8 Bytes are used for management info so malloc of 16 needs 16+8 bytes
    printf("MSP=%X %ld  HEAP=%X %ld\n", (uintptr_t) tStackpointer, ((uint32_t) (&estack) - (uint32_t) tStackpointer),
            (uintptr_t) (tHeapEnd - 8), (uint32_t) ((tHeapEnd - 8 - &HeapStart)));
#ifdef HAL_WWDG_MODULE_ENABLED
    Watchdog_reload();
#endif

    // Stacktrace
    uint32_t StackContent;
    unsigned int tPrintColumn = 0;
    tStackpointer += 14 + aStackEntriesToSkip; // since this function pushes 9 registers to stack
    for (int i = 0; i < 8;) {
        StackContent = *tStackpointer++;
        if (StackContent & 0x01) {
            // adjust odd (lr) values
            StackContent -= 1;
        }
        if (StackContent > (uint32_t) &stext && StackContent < (uint32_t) &srodata) {
            tPrintColumn += printf("%X ", (uintptr_t) StackContent);
#ifdef HAL_WWDG_MODULE_ENABLED
            Watchdog_reload();
#endif
            if (tPrintColumn > REMOTE_DISPLAY_WIDTH - (8 * TEXT_SIZE_11_WIDTH)) {
                printf("\n");
                tPrintColumn = 0;
                i++;
            }
        }
        if (tStackpointer >= (uint32_t *) &estack) {
            break;
        }
    }
}

void FaultHandler(struct ExceptionStackFrame * aStackPointer) {
    BlueDisplay1.setPrintfSizeAndColorAndFlag(TEXT_SIZE_11, COLOR_RED, COLOR_WHITE, true);
    BlueDisplay1.setPrintfPosition(0, 0);
    printf("PC=%lX LR [R14]=%lX\n", aStackPointer->pc, aStackPointer->lr);
    printf("R0=%lX R1=%lX\n", aStackPointer->r0, aStackPointer->r1);
    printf("R2=%lX R3=%lX R12=%lX\n", aStackPointer->r2, aStackPointer->r3, aStackPointer->r12);

    // Vector numbers
//	0 = Thread mode
//	1 = Reserved
//	2 = NMI
//	3 = HardFault
//	4 = MemManage
//	5 = BusFault
//	6 = UsageFault
//	7-10 = Reserved
//	11 = SVCall
//	12 = Reserved for Debug
//	13 = Reserved
//	14 = PendSV
//	15 = SysTick
//	16 = IRQ0.

    printf("PSP=%lX PSR=%lX\n", __get_PSP(), aStackPointer->psr);
    printf("VectNr=%X PendVectNr=%X ICSR=%lX\n", (char) ((*((volatile uint32_t *) (0xE000ED04))) & 0xFF),
            (unsigned short) (((*((volatile uint32_t *) (0xE000ED04))) >> 12) & 0x3F),
            (*((volatile uint32_t *) (0xE000ED04))));

    // Faults on 6. line
    IRQn_Type tIRQNr = (IRQn_Type) (((*((volatile long *) (0xE000ED04))) & 0xFF) - 16);
    // Hard Fault is not defined in stmf30x.h :-(
    if (tIRQNr == MemoryManagement_IRQn - 1) {
        printf("HardFSR=%lX\n", (*((volatile uint32_t *) (0xE000ED2C))));
    } else if (tIRQNr == UsageFault_IRQn) {
        printf("UsageFSR=%X\n", (*((volatile unsigned short *) (0xE000ED2A))));
    } else if (tIRQNr == BusFault_IRQn) {
        printf("BusFAR=%lX BusFSR=%X\n", (*((volatile uint32_t *) (0xE000ED38))),
                (*((volatile char *) (0xE000ED29))));

    } else if (tIRQNr == MemoryManagement_IRQn) {
        printf("MemManFSR=%X\n", (*((volatile char *) (0xE000ED28))));
    }

    uint32_t tAuxFaultStatusRegister = (*((volatile uint32_t *) (0xE000ED3C)));
    if ((*((volatile uint32_t *) (0xE000ED3C))) != 0x00000000) {
        // makes only sense if != 0
        printf("AuxFSR=%lX\n", tAuxFaultStatusRegister);
    }

    printStackpointerAndStacktrace(7, 8);

#ifdef LOCAL_DISPLAY_EXISTS
    // reset lock (just in case...)
    sDrawLock = 0;
#endif
    while (1) {
#ifdef HAL_WWDG_MODULE_ENABLED
        Watchdog_reload();
#endif
        // enable sending print data over UART
        if (__HAL_UART_GET_FLAG(&UART_BD_Handle, UART_FLAG_TC) != RESET) {
            UART_BD_IRQHANDLER();
        }
    }
}
