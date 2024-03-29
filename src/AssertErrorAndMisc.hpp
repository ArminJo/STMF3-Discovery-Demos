/*
 * AssertErrorAndMisc.hpp
 *
 *  Copyright (C) 2013-2023  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of STMF3-Discovery-Demos https://github.com/ArminJo/STMF3-Discovery-Demos.
 *
 *  STMF3-Discovery-Demos is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 */

#ifndef _ASERT_ERRROR_AND_MISC_HPP
#define _ASERT_ERRROR_AND_MISC_HPP

#include "Pages.h" // for REMOTE_DISPLAY_WIDTH
#include "AssertErrorAndMisc.h"
#include "stm32fx0xPeripherals.h" // for Watchdog_reload()
#include "BlueDisplay.h"
#include "BlueSerial.h" // for UART_BD_IRQHANDLER()
#include "timing.h"
#include "main.h" // for StringBuffer

#ifdef USE_STM32F3_DISCO
#include "stm32f3_discovery.h"
#endif

#include <stdio.h> /* for sprintf */
#include <string.h> // for strrchr
#include <stdlib.h> // for malloc

#if defined(SUPPORT_LOCAL_DISPLAY)
int sLockCount = 0; // counts skipped drawChars because of display locks
#endif

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
        snprintf(sStringBuffer, sizeof sStringBuffer, "Wrong parameters value on line: %lu\nfile: %s", aLine, tFile);
#if defined(SUPPORT_LOCAL_DISPLAY)
        BlueDisplay1.drawMLText(0, ASSERT_START_Y, sStringBuffer, TEXT_SIZE_11, COLOR16_RED, COLOR16_WHITE);
#else
        BlueDisplay1.drawText(0, ASSERT_START_Y, sStringBuffer, TEXT_SIZE_11, COLOR16_RED, COLOR16_WHITE);
#endif
        delay(2000);
    } else {
        /* Infinite loop */
        while (1) {
#ifdef USE_STM32F3_DISCO
            BSP_LED_Toggle(LED_GREEN); // GREEN LEFT
#endif
            /* Insert delay */
            delay(500);
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
        snprintf(sStringBuffer, sizeof sStringBuffer, "%s on line: %lu\nfile: %s\nLR=%#X val: %#X %d", aMessage, aLine,
                tFile, (unsigned int) aLinkRegister, aWrongParameter, aWrongParameter);
#if defined(SUPPORT_LOCAL_DISPLAY)
        // reset lock (just in case...)
        sDrawLock = 0;
        BlueDisplay1.drawMLText(0, ASSERT_START_Y, sStringBuffer, TEXT_SIZE_11, COLOR16_RED, COLOR16_WHITE);
#else
        BlueDisplay1.drawText(0, ASSERT_START_Y, sStringBuffer, TEXT_SIZE_11, COLOR16_RED, COLOR16_WHITE);
#endif
        delay(2000);
    } else {
        /* Infinite loop */
        while (1) {
#ifdef USE_STM32F3_DISCO
            BSP_LED_Toggle(LED_GREEN); // GREEN LEFT
#endif
            /* Insert delay */
            delay(500);
        }
    }
}

extern "C" void printError(uint8_t* aFile, uint32_t aLine) {
    if (isLocalDisplayAvailable) {
        char * tFile = (strrchr(reinterpret_cast<char*>(aFile), '/') + 1);
        snprintf(sStringBuffer, sizeof sStringBuffer, "Error on line: %lu\nfile: %s", aLine, tFile);
#if defined(SUPPORT_LOCAL_DISPLAY)
        // reset lock (just in case...)
        sDrawLock = 0;
#endif
        BlueDisplay1.drawText(0, ASSERT_START_Y, sStringBuffer, TEXT_SIZE_11, COLOR16_RED, COLOR16_WHITE);
        delay(2000);
    } else {
        /* Infinite loop */
        while (1) {
#ifdef USE_STM32F3_DISCO
            BSP_LED_Toggle(LED_GREEN); // GREEN LEFT
#endif
            /* Insert delay */
            delay(500);
        }
    }
}

extern "C" void errorMessage(const char * aMessage) {
    if (isLocalDisplayAvailable) {
#if defined(SUPPORT_LOCAL_DISPLAY)
        BlueDisplay1.drawMLText(0, ASSERT_START_Y, aMessage, TEXT_SIZE_11, COLOR16_RED, COLOR16_WHITE);
#else
        BlueDisplay1.drawText(0, ASSERT_START_Y, aMessage, TEXT_SIZE_11, COLOR16_RED, COLOR16_WHITE);
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
    BlueDisplay1.setWriteStringPositionColumnLine(0, aStartLine);

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
    BlueDisplay1.setWriteStringSizeAndColorAndFlag(TEXT_SIZE_11, COLOR16_RED, COLOR16_WHITE, true);
    BlueDisplay1.setWriteStringPosition(0, 0);
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

#if defined(SUPPORT_LOCAL_DISPLAY)
    // reset lock (just in case...)
    sDrawLock = 0;
#endif
    while (1) {
#ifdef HAL_WWDG_MODULE_ENABLED
        Watchdog_reload();
#endif
#if !defined(DISABLE_REMOTE_DISPLAY)
        // enable sending print data over UART
        if (__HAL_UART_GET_FLAG(&UART_BD_Handle, UART_FLAG_TC) != RESET) {
            UART_BD_IRQHANDLER();
        }
#endif
    }
}

#endif // _ASERT_ERRROR_AND_MISC_HPP
