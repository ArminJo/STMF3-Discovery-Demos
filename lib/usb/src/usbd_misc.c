/**
 * usb_misc.c
 *
 * @date 27.03.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 *      Source based on STM code samples
 * @version 1.0.0
 */

#include "usbd_misc.h"
#include "tinyPrint.h"
#include "myStrings.h"

#include "timing.h"
#include "usbd_hid.h"
#include "usbd_cdc.h"

#include <stdio.h> /* for sprintf */
USBD_HandleTypeDef USBDDeviceHandle;
extern PCD_HandleTypeDef PCDHandle;
extern USBD_CDC_LineCodingTypeDef LineCoding;

const char * getUSBDeviceState(void) {
    switch (USBDDeviceHandle.dev_state) {
    case USBD_STATE_DEFAULT:
        return "default";
        break;
//    case ATTACHED:
//        return "attached";
//        break;
//    case POWERED:
//        return "powered";
//        break;
    case USBD_STATE_SUSPENDED:
        return "suspended";
        break;
    case USBD_STATE_ADDRESSED:
        return "addressed";
        break;
    case USBD_STATE_CONFIGURED:
        return "configured";
        break;
    default:
        break;
    }
    return "Error";
}

bool isUSBReady(void) {
    if (USBDDeviceHandle.dev_state == USBD_STATE_CONFIGURED) {
        return true;
    }
    return false;
}

bool isUsbCdcReady(void) {
    if (USBDDeviceHandle.dev_state == USBD_STATE_CONFIGURED && USBDDeviceHandle.pClass == &USBD_CDC) {
        return true;
    }
    return false;
}

bool isUSBTypeCDC(void) {
    if (USBDDeviceHandle.pClass == &USBD_CDC) {
        return true;
    }
    return false;
}

int getUSB_StaticInfos(char *aStringBuffer, size_t sizeofStringBuffer) {
    const char * tUSBTypeString;
    const char * tUSBSpeedString;
    int tPacketSize;
    if (USBDDeviceHandle.pClass == &USBD_HID) {
        tUSBTypeString = "Joystick";
    } else if (USBDDeviceHandle.pClass == &USBD_CDC) {
        tUSBTypeString = "Serial";
    } else {
        tUSBTypeString = "Unknown";
    }
    if (USBDDeviceHandle.dev_speed == USBD_SPEED_HIGH) {
        tUSBSpeedString = "HighSpeed";
        tPacketSize = CDC_DATA_HS_OUT_PACKET_SIZE;
    } else {
        tUSBSpeedString = "FullSpeed";
        tPacketSize = CDC_DATA_FS_OUT_PACKET_SIZE;
    }
    int tIndex = snprintf(aStringBuffer, sizeofStringBuffer, "USB-Type:%s %s\n", tUSBTypeString, tUSBSpeedString);
    if (USBDDeviceHandle.pClass == &USBD_CDC) {
        // print additional CDC info
        tIndex += snprintf(&aStringBuffer[tIndex], sizeofStringBuffer - tIndex, "Buffer:%3d Byte\n", tPacketSize);
        tIndex += snprintf(&aStringBuffer[tIndex], sizeofStringBuffer - tIndex, "%6ld Baud  %d Bit  %d Stop", LineCoding.bitrate,
                LineCoding.datatype, LineCoding.format + 1);

    }
    return tIndex;
}

/**
 * @brief  This function handles USB Handler.
 * @param  None
 * @retval None
 */
#if defined (USE_USB_INTERRUPT_DEFAULT)
void USB_LP_CAN_RX0_IRQHandler(void)
#elif defined (USE_USB_INTERRUPT_REMAPPED)
void USB_LP_IRQHandler(void)
#endif
{
    HAL_PCD_IRQHandler(&PCDHandle);
}

/**
 * @brief  This function handles USB WakeUp interrupt request.
 * @param  None
 * @retval None
 */
#if defined (USE_USB_INTERRUPT_DEFAULT)
void USBWakeUp_IRQHandler(void)
#elif defined (USE_USB_INTERRUPT_REMAPPED)
void USBWakeUp_RMP_IRQHandler(void)
#endif
{
    __HAL_USB_EXTI_CLEAR_FLAG();
    myPrint(" USB Wakeup Handler", 32);
}

uint8_t * CDC_Loopback(void) {
//    if (bDeviceState == CONFIGURED) {
//        CDC_Receive_DATA();
//        if (USB_PacketReceived) {
//            USB_PacketReceived = false;
//            if (USB_ReceiveLength >= CDC_RX_BUFFER_SIZE) {
//                // adjust for trailing string delimiter
//                USB_ReceiveLength--;
//            }
//            USB_ReceiveBuffer[USB_ReceiveLength] = '\0';
//            if (USB_PacketSent) {
//    USBD_CDC_SetTxBuffer(&USBDDeviceHandle, (uint8_t*) USB_ReceiveBuffer, USB_ReceiveLength);
//    USBD_CDC_TransmitPacket(&USBDDeviceHandle);
//                CDC_Send_DATA((unsigned char*) USB_ReceiveBuffer, USB_ReceiveLength);
//            }
//            return &USB_ReceiveBuffer[0];
//        }
//    }
    return NULL ;
}

void CDC_TestSend(void) {
    int tCount = snprintf(StringBuffer, sizeof StringBuffer, "Test %p", &StringBuffer[0]);
    USBD_CDC_SetTxBuffer(&USBDDeviceHandle, (uint8_t*) &StringBuffer[0], tCount);
    while (USBD_CDC_TransmitPacket(&USBDDeviceHandle) == USBD_BUSY) {
        delayMillis(100);
    }
}
