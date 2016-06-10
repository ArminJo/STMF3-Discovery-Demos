/**
 ******************************************************************************
 * @file    USB_Device/CDC_Standalone/Src/usbd_cdc_interface.c
 * @author  MCD Application Team
 * @version V1.0.1
 * @date    26-February-2014
 * @brief   Source file for USBD CDC interface
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc.h"

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
 * @{
 */

/** @defgroup USBD_CDC
 * @brief usbd core module
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
//#define APP_RX_DATA_SIZE  2048
//#define APP_TX_DATA_SIZE  2048
#define APP_RX_DATA_SIZE  512
#define APP_TX_DATA_SIZE  512

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
USBD_CDC_LineCodingTypeDef LineCoding = { 115200, /* baud rate*/
0x00, /* stop bits-1*/
0x00, /* parity - none*/
0x08 /* nb. of bits 8*/
};

uint8_t UserRxBuffer[APP_RX_DATA_SIZE];/* Received Data over USB are stored in this buffer */
uint8_t UserTxBuffer[APP_TX_DATA_SIZE];/* Received Data over UART (CDC interface) are stored in this buffer */
uint32_t BuffLength;
uint32_t UserTxBufPtrIn = 0;/* Increment this pointer or roll it back to
 start address when data are received over USART */
uint32_t UserTxBufPtrOut = 0; /* Increment this pointer or roll it back to
 start address when data are sent over USB */

/* UART handler declaration */
UART_HandleTypeDef UartHandle;
/* TIM handler declaration */
TIM_HandleTypeDef TimHandle;
/* USB handler declaration */
extern USBD_HandleTypeDef hUSBDDevice;

/* Private function prototypes -----------------------------------------------*/
static int8_t CDC_Itf_Init(void);
static int8_t CDC_Itf_DeInit(void);
static int8_t CDC_Itf_Control(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Itf_Receive(uint8_t* pbuf, uint32_t *Len);

USBD_CDC_ItfTypeDef USBD_CDC_fops = { CDC_Itf_Init, CDC_Itf_DeInit, CDC_Itf_Control, CDC_Itf_Receive };

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  CDC_Itf_Init
 *         Initializes the CDC media low layer
 * @param  None
 * @retval Result of the opeartion: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t CDC_Itf_Init(void) {
    /*##-5- Set Application Buffers ############################################*/
    USBD_CDC_SetTxBuffer(&hUSBDDevice, UserTxBuffer, 0);
    USBD_CDC_SetRxBuffer(&hUSBDDevice, UserRxBuffer);

    return (USBD_OK);
}

/**
 * @brief  CDC_Itf_DeInit
 *         DeInitializes the CDC media low layer
 * @param  None
 * @retval Result of the opeartion: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t CDC_Itf_DeInit(void) {
    return (USBD_OK);
}

/**
 * @brief  CDC_Itf_Control
 *         Manage the CDC class requests
 * @param  Cmd: Command code
 * @param  Buf: Buffer containing command data (request parameters)
 * @param  Len: Number of data to be sent (in bytes)
 * @retval Result of the opeartion: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t CDC_Itf_Control(uint8_t cmd, uint8_t* pbuf, uint16_t length) {
    switch (cmd) {
    case CDC_SEND_ENCAPSULATED_COMMAND:
        /* Add your code here */
        break;

    case CDC_GET_ENCAPSULATED_RESPONSE:
        /* Add your code here */
        break;

    case CDC_SET_COMM_FEATURE:
        /* Add your code here */
        break;

    case CDC_GET_COMM_FEATURE:
        /* Add your code here */
        break;

    case CDC_CLEAR_COMM_FEATURE:
        /* Add your code here */
        break;

    case CDC_SET_LINE_CODING:
        LineCoding.bitrate = (uint32_t) (pbuf[0] | (pbuf[1] << 8) |\
 (pbuf[2] << 16) | (pbuf[3] << 24));
        LineCoding.format = pbuf[4];
        LineCoding.paritytype = pbuf[5];
        LineCoding.datatype = pbuf[6];

        /* Set the new configuration */
        ComPort_Config();
        break;

    case CDC_GET_LINE_CODING:
        pbuf[0] = (uint8_t) (LineCoding.bitrate);
        pbuf[1] = (uint8_t) (LineCoding.bitrate >> 8);
        pbuf[2] = (uint8_t) (LineCoding.bitrate >> 16);
        pbuf[3] = (uint8_t) (LineCoding.bitrate >> 24);
        pbuf[4] = LineCoding.format;
        pbuf[5] = LineCoding.paritytype;
        pbuf[6] = LineCoding.datatype;

        /* Add your code here */
        break;

    case CDC_SET_CONTROL_LINE_STATE:
        /* Add your code here */
        break;

    case CDC_SEND_BREAK:
        /* Add your code here */
        break;

    default:
        break;
    }

    return (USBD_OK);
}

/**
 * @brief  TIM period elapsed callback
 * @param  htim: TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    uint32_t buffptr;
    uint32_t buffsize;

    if (UserTxBufPtrOut != UserTxBufPtrIn) {
        if (UserTxBufPtrOut > UserTxBufPtrIn) /* rollback */
        {
            buffsize = APP_RX_DATA_SIZE - UserTxBufPtrOut;
        } else {
            buffsize = UserTxBufPtrIn - UserTxBufPtrOut;
        }

        buffptr = UserTxBufPtrOut;

        USBD_CDC_SetTxBuffer(&hUSBDDevice, (uint8_t*) &UserTxBuffer[buffptr], buffsize);

        if (USBD_CDC_TransmitPacket(&hUSBDDevice) == USBD_OK) {
            UserTxBufPtrOut += buffsize;
            if (UserTxBufPtrOut == APP_RX_DATA_SIZE) {
                UserTxBufPtrOut = 0;
            }
        }
    }
}

/**
 * @brief  CDC_Itf_DataRx
 *         Data received over USB OUT endpoint are sent over CDC interface
 *         through this function.
 * @param  Buf: Buffer of data to be transmitted
 * @param  Len: Number of data received (in bytes)
 * @retval Result of the opeartion: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t CDC_Itf_Receive(uint8_t* Buf, uint32_t *Len) {
    return (USBD_OK);
}

/**
 * @brief  Tx Transfer completed callback
 * @param  huart: UART handle
 * @retval None
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    /* Initiate next USB packet transfer once UART completes transfer (transmitting data over Tx line) */
    //USBD_CDC_ReceivePacket(&hUSBDDevice);
}

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

