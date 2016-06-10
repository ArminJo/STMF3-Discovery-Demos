/*
 * USB.h
 *
 * @date 27.03.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 *      Source based on STM code samples
 * @version 1.1.0
 */

#ifndef USB_H_
#define USB_H_

#include <stdbool.h>
#include <stddef.h>
#include "usbd_def.h"
#include "usbd_desc.h"

#if !defined (USE_USB_INTERRUPT_DEFAULT) && !defined (USE_USB_INTERRUPT_REMAPPED)
 #error "Missing define Please Define Your Interrupt Mode By Setting Value For All Languages"
#endif

bool isUsbCdcReady(void);

extern USBD_HandleTypeDef USBDDeviceHandle;

const char * getUSBDeviceState(void);
int getUSB_StaticInfos(char *aStringBuffer, size_t sizeofStringBuffer);
bool isUSBTypeCDC(void);

bool isUSBReady(void);

void CDC_TestSend(void);
uint8_t * CDC_Loopback(void);
void USB_ChangeToCDC(void);
void USB_ChangeToJoystick(void);

#endif /* USB_H_ */
