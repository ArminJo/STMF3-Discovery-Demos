/*
 * BlueSerial.h
 *
 *  Copyright (C) 2014-2023  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay.
 *  BlueDisplay is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#ifndef _BLUESERIAL_H
#define _BLUESERIAL_H

#ifdef STM32F10X
#include <stm32f1xx.h>
#endif
#ifdef STM32F30X
#include <stm32f3xx.h>
#include "stm32f3xx_hal_conf.h" // for UART_HandleTypeDef
#endif
#include <stddef.h>

#define BAUD_STRING_4800 "4800"
#define BAUD_STRING_9600 "9600"
#define BAUD_STRING_19200 "19200"
#define BAUD_STRING_38400 "38400"
#define BAUD_STRING_57600 "57600"
#define BAUD_STRING_115200 "115200"
#define BAUD_STRING_230400 "230400"
#define BAUD_STRING_460800 "460800"
#define BAUD_STRING_921600 " 921600"
#define BAUD_STRING_1382400 "1382400"

#define BAUD_4800 (4800)
#define BAUD_9600 (9600)
#define BAUD_19200 (19200)
#define BAUD_38400 (38400)
#define BAUD_57600 (57600)
#define BAUD_115200 (115200)
#define BAUD_230400 (230400)
#define BAUD_460800 (460800)
#define BAUD_921600 ( 921600)
#define BAUD_1382400 (1382400)

/*
 * common functions
 */
void sendUSARTArgs(uint8_t aFunctionTag, int aNumberOfArgs, ...);
void sendUSARTArgsAndByteBuffer(uint8_t aFunctionTag, int aNumberOfArgs, ...);
void sendUSART5Args(uint8_t aFunctionTag, uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd,
        uint16_t aColor);
void sendUSART5ArgsAndByteBuffer(uint8_t aFunctionTag, uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd,
        uint16_t aYEnd, uint16_t aColor, uint8_t * aBuffer, size_t aBufferLength);
void sendUSART5ArgsAndShortBuffer(uint8_t aFunctionTag, uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd,
        uint16_t aYEnd, uint16_t aColor, uint16_t * aBuffer, size_t aBufferLength);

#ifdef STM32F303xC
#define UART_BD                     USART3
#define UART_BD_TX_PIN              GPIO_PIN_10
#define UART_BD_RX_PIN              GPIO_PIN_11
#define UART_BD_PORT                GPIOC
#define UART_BD_IO_CLOCK_ENABLE()   __GPIOC_CLK_ENABLE()
#define UART_BD_CLOCK_ENABLE()      __USART3_CLK_ENABLE()
#define UART_BD_IRQ                 USART3_IRQn
#define UART_BD_IRQHANDLER          USART3_IRQHandler

#define UART_BD_DMA_TX_CHANNEL      DMA1_Channel2
#define UART_BD_DMA_RX_CHANNEL      DMA1_Channel3
#define UART_BD_DMA_CLOCK_ENABLE()  __DMA1_CLK_ENABLE()

#define BLUETOOTH_PAIRED_DETECT_PIN     GPIO_PIN_13
#define BLUETOOTH_PAIRED_DETECT_PORT    GPIOC
#endif

#ifdef STM32F103xB
#define UART_BD                     USART1
#define UART_BD_TX_PIN              GPIO_PIN_9
#define UART_BD_RX_PIN              GPIO_PIN_10
#define UART_BD_PORT                GPIOA
#define UART_BD_IO_CLOCK_ENABLE()   __GPIOA_CLK_ENABLE()
#define UART_BD_CLOCK_ENABLE()      __USART1_CLK_ENABLE()
#define UART_BD_IRQ                 USART1_IRQn
#define UART_BD_IRQHANDLER          USART1_IRQHandler

#define UART_BD_DMA_TX_CHANNEL      DMA1_Channel4
#define UART_BD_DMA_RX_CHANNEL      DMA1_Channel5
#define UART_BD_DMA_CLOCK_ENABLE()  __DMA1_CLK_ENABLE()

#define BLUETOOTH_PAIRED_DETECT_PIN     GPIO_PIN_7
#define BLUETOOTH_PAIRED_DETECT_PORT    GPIOA
#endif

// The UART used by BlueDisplay
extern UART_HandleTypeDef UART_BD_Handle;
#ifdef __cplusplus
extern "C" {
#endif
void UART_BD_IRQHANDLER(void);
#ifdef __cplusplus
}
#endif

__STATIC_INLINE bool USART_isBluetoothPaired(void) {
    if ((BLUETOOTH_PAIRED_DETECT_PORT->IDR & BLUETOOTH_PAIRED_DETECT_PIN) != 0) {
        return true;
    }
    return false;
}

// Send functions using buffer and DMA
int getSendBufferFreeSpace(void);

void UART_BD_initialize(uint32_t aBaudRate);
void HAL_UART_MspInit(UART_HandleTypeDef* aUARTHandle);

uint32_t getUSART_BD_BaudRate(void);
void setUART_BD_BaudRate(uint32_t aBaudRate);

// Simple blocking serial version without overhead
void sendUSARTBufferSimple(uint8_t * aParameterBufferPointer, size_t aParameterBufferLength,
        uint8_t * aDataBufferPointer, size_t aDataBufferLength);

// Function using DMA
void sendUSARTBufferNoSizeCheck(uint8_t * aParameterBufferPointer, size_t aParameterBufferLength,
        uint8_t * aDataBufferPointer, size_t aDataBufferLength);
void checkAndHandleMessageReceived(void);

#endif // _BLUESERIAL_H
