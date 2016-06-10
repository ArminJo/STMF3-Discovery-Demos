/*
 * PageInfo.cpp
 *
 * @date 16.03.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#include "Pages.h"
#include "EventHandler.h"
#include "l3gdc20_lsm303dlhc_utils.h"

#include "tinyPrint.h"
#include "main.h"
#include <string.h>
#include <locale.h>
#include <stdlib.h> // for malloc
extern "C" {
#include "stm32f3_discovery.h"
#include "diskio.h"
#include "ff.h"
#include "usbd_misc.h"
}

/* Private variables ---------------------------------------------------------*/
static BDButton TouchButtonBack;
static bool sBackButtonPressed;
BDButton TouchButtonInfoFont1;
BDButton TouchButtonInfoFont2;
BDButton TouchButtonInfoColors;
BDButton TouchButtonInfoMMC;
BDButton TouchButtonInfoUSB;

BDButton TouchButtonInfoSystem;

// Buttons for a single info page
BDButton TouchButtonSettingsGamma1;
BDButton TouchButtonSettingsGamma2;

#define INFO_BUTTONS_NUMBER_TO_DISPLAY 6 // Number of buttons on main info page, without back button
BDButton * const TouchButtonsInfoPage[] = { &TouchButtonInfoFont1, &TouchButtonInfoFont2, &TouchButtonInfoColors,
        &TouchButtonInfoMMC, &TouchButtonInfoSystem, &TouchButtonInfoUSB, &TouchButtonBack,
        &TouchButtonSettingsGamma1, &TouchButtonSettingsGamma2 };

/* Private function prototypes -----------------------------------------------*/

void pickColorPeriodicCallbackHandler(struct TouchEvent * const aActualPositionPtr);
void drawInfoPage(void);

/* Private functions ---------------------------------------------------------*/

/**
 * for gamma change in color page
 * @param aTheTouchedButton
 * @param aValue
 */
void doSetGamma(BDButton * aTheTouchedButton, int16_t aValue) {
    setGamma(aValue);
}

void showSystemInfo() {

    extern char estack asm("_estack");
    // Bss, Heap, Ram
    extern char BssStart asm("_sbss");
    extern char BssEnd asm("_ebss");
    extern char HeapStart asm("_sccmram");
    extern char DataStart asm("_sdata");
    extern char DataEnd asm("_edata");

    BlueDisplay1.setPrintfSizeAndColorAndFlag(TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE, true);
    BlueDisplay1.setPrintfPosition(0, 0);
    unsigned int tControlRegisterContent = __get_CONTROL();
    printf("Control=%X xPSR=%lX\n", tControlRegisterContent, __get_xPSR());

    // DATA
    printf("DATA=%X %X %d\n", (uintptr_t) &DataStart, ((uintptr_t) &DataEnd) & 0xFFFF, &DataEnd - &DataStart);

    // BSS
    printf("BSS=%X %X %d\n", (uintptr_t) &BssStart, ((uintptr_t) &BssEnd) & 0xFFFF, &BssEnd - &BssStart);

    // HEAP
    char * tHeapEnd = (char *) malloc(16);
    printf("HEAP=%X %X %d\n", (uintptr_t) &HeapStart, ((uintptr_t) tHeapEnd) & 0xFFFF, (tHeapEnd - &HeapStart));
    free(tHeapEnd);

    // STACK
    printf("STACK=%X %X %ld\n", (uintptr_t) &estack, ((uintptr_t) __get_MSP()) & 0xFFFF,
            (uint32_t) (&estack - __get_MSP()));

    // Clocks
    printf("SYSCLK=%ldMHz HCLK=%ldMHz\n", HAL_RCC_GetSysClockFreq() / 1000000, HAL_RCC_GetHCLKFreq() / 1000000);

    // ADC's + RTC
    const char* tRTCClockSource = "HSE";
    if ( __HAL_RCC_GET_RTC_SOURCE() == RCC_RTCCLKSOURCE_LSE) {
        tRTCClockSource = "LSE";
    }
    printf("ADC12,ADC34=%ldMHz RTC CLK=%s\n", HAL_RCC_GetHCLKFreq() / 1000000, tRTCClockSource);

    // PCLK1
    printf("PCLK1=%ldMHz I2C1,USART2-4\n", HAL_RCC_GetPCLK1Freq() / 1000000);

    // PCLK2
    printf("PCLK2=%ldMHz USART1,TIM1+8 %ld NVIC lines\n", HAL_RCC_GetPCLK2Freq() / 1000000,
            ((SCnSCB->ICTR & SCnSCB_ICTR_INTLINESNUM_Msk) + 1) * 32);

    // Priorities
    printf("Prios: ADC=%ld UART3=%ld WWDG=%ld User=%ld\n", NVIC_GetPriority(ADC1_2_IRQn),
            NVIC_GetPriority(USART3_IRQn), NVIC_GetPriority(WWDG_IRQn), NVIC_GetPriority(EXTI0_IRQn));

    printf("SysTic=%ld USB=%ld Touch=%ld DMA1=%ld\n", NVIC_GetPriority(SysTick_IRQn),
            NVIC_GetPriority(USB_LP_CAN_RX0_IRQn), NVIC_GetPriority(EXTI1_IRQn),
            NVIC_GetPriority(DMA1_Channel1_IRQn));

    // Compile time
    printf("Compiled at: %s %s\n", __DATE__, __TIME__);

    // Locale
    struct lconv * tLconfPtr = localeconv();
    // decimal point is set to comma by main.c
    printf("Locale=%s decimalpoint=%s thousands=%s\n", setlocale(LC_ALL, NULL), tLconfPtr->decimal_point,
            tLconfPtr->thousands_sep);

#ifdef USE_BUTTON_POOL
    // Button Pool Info
    printf("BUTTONS: poolsize=%d + %d\n", NUMBER_OF_BUTTONS_IN_POOL,
            NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL);

    TouchButton::infoButtonPool(StringBuffer);
    printf("%s\n", StringBuffer);
#endif

    // Lock info
    printf("LOCK count=%d\n", sLockCount);

    // Debug Info - just in case
    printf("Dbg: 1=%#X, 1=%d 2=%#X\n", DebugValue1, DebugValue1, DebugValue2);
    //        printf("Dbg: 3=%#X 4=%#X 5=%#X\n", DebugValue3, DebugValue4, DebugValue5);
}

void doInfoButtons(BDButton * aTheTouchedButton, int16_t aValue) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    BDButton::deactivateAllButtons();
    TouchButtonBack.drawButton();
    sBackButtonPressed = false;
    if (aTheTouchedButton->mButtonHandle == TouchButtonInfoFont1.mButtonHandle) {
        uint16_t tXPos;
        uint16_t tYPos = 6 * TEXT_SIZE_11_HEIGHT;
        // printable ascii chars
        unsigned char tChar = FONT_START;
        for (int i = 6; i != 0; i--) {
            tXPos = 0;
            for (uint8_t j = 16; j != 0; j--) {
                tXPos = BlueDisplay1.drawChar(tXPos, tYPos, tChar, TEXT_SIZE_22, COLOR_BLACK, COLOR_YELLOW) + 4;
                tChar++;
            }
            tYPos += TEXT_SIZE_22_HEIGHT + 4;
        }
    } else if (aTheTouchedButton->mButtonHandle == TouchButtonInfoFont2.mButtonHandle) {
        uint16_t tXPos;
        uint16_t tYPos = TEXT_SIZE_22_HEIGHT + 4;
        // special chars
        unsigned char tChar = 0x80;
        for (int i = 8; i != 0; i--) {
            tXPos = 0;
            for (uint8_t j = 16; j != 0; j--) {
                tXPos = BlueDisplay1.drawChar(tXPos, tYPos, tChar, TEXT_SIZE_22, COLOR_BLACK, COLOR_YELLOW) + 4;
                tChar++;
            }
            tYPos += TEXT_SIZE_22_HEIGHT + 2;
        }
    }

    else if (aTheTouchedButton->mButtonHandle == TouchButtonInfoColors.mButtonHandle) {
        BlueDisplay1.generateColorSpectrum();
        TouchButtonSettingsGamma1.activate();
        TouchButtonSettingsGamma2.activate();
        registerTouchDownCallback(&pickColorPeriodicCallbackHandler);
        registerTouchMoveCallback(&pickColorPeriodicCallbackHandler);

        while (!sBackButtonPressed) {
            checkAndHandleEvents();
        }
        // reset handler
        registerTouchDownCallback(&simpleTouchDownHandler);
        registerTouchMoveCallback(&simpleTouchMoveHandlerForSlider);
    }

    else if (aTheTouchedButton->mButtonHandle == TouchButtonInfoMMC.mButtonHandle) {
        int tRes = getCardInfo(StringBuffer, sizeof(StringBuffer));
        if (tRes == 0 || tRes > RES_PARERR) {
            BlueDisplay1.drawMLText(0, 10, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
        } else {
            testAttachMMC();
        }
        if (tRes > RES_PARERR) {
            tRes = getFSInfo(StringBuffer, sizeof(StringBuffer));
            if (tRes == 0 || tRes > FR_INVALID_PARAMETER) {
                BlueDisplay1.drawMLText(0, 20 + 4 * TEXT_SIZE_11_HEIGHT, StringBuffer, TEXT_SIZE_11, COLOR_RED,
                COLOR_BACKGROUND_DEFAULT);
            }
        }
        // was overwritten by MLText
        TouchButtonBack.drawButton();
    }
    /*
     * USB info
     */
    else if (aTheTouchedButton->mButtonHandle == TouchButtonInfoUSB.mButtonHandle) {
        getUSB_StaticInfos(StringBuffer, sizeof StringBuffer);
        // 3 lines
        BlueDisplay1.drawMLText(10, BUTTON_HEIGHT_4_LINE_2 + TEXT_SIZE_11_ASCEND, StringBuffer, TEXT_SIZE_11,
        COLOR_RED, COLOR_BACKGROUND_DEFAULT);
//        USB_ChangeToCDC();
//        do {
//            uint8_t * tRecData = CDC_Loopback();
//            if (tRecData != NULL) {
//                myPrint((const char*) tRecData, CDC_RX_BUFFER_SIZE);
//            }
//            BlueDisplay1.drawText(10, BUTTON_HEIGHT_4_LINE_2 - TEXT_SIZE_11_DECEND, getUSBDeviceState(), TEXT_SIZE_11, COLOR_RED,
//            COLOR_BACKGROUND_DEFAULT);
//            delayMillisWithCheckAndHandleEvents(500);
//        } while (!sBackButtonPressed);
    }
    /*
     * System info
     */
    else if (aTheTouchedButton->mButtonHandle == TouchButtonInfoSystem.mButtonHandle) {
        /*
         * show temperature
         */
        showSystemInfo();
        int tTemperatureFilteredShift8 = (ADC1_getTemperatureMilligrades() << 8) / 10;
        int tTemperatureFiltered2Shift8 = tTemperatureFilteredShift8;
        do {
            int tTemp = (ADC1_getTemperatureMilligrades() << 8) / 10;
            // filter 16 samples
            tTemperatureFilteredShift8 = (tTemperatureFilteredShift8 * 15 + tTemp) / 16;
            // filter 32 samples
            tTemperatureFiltered2Shift8 = (tTemperatureFiltered2Shift8 * 31 + tTemp) / 32;
            snprintf(StringBuffer, sizeof StringBuffer, "%5.2f\xB0" "C %2d.%02d\xB0" "C %2d.%02d %2d.%02d",
                    readLSM303TempFloat(), tTemp / 100, tTemp % 100, (tTemperatureFilteredShift8 >> 8) / 100,
                    (tTemperatureFilteredShift8 >> 8) % 100, (tTemperatureFiltered2Shift8 >> 8) / 100,
                    (tTemperatureFiltered2Shift8 >> 8) % 100);
            BlueDisplay1.drawText(2, BlueDisplay1.getDisplayHeight() - TEXT_SIZE_11_DECEND, StringBuffer,
            TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_YELLOW);
            delayMillisWithCheckAndHandleEvents(500);
        } while (!sBackButtonPressed);
    }
}

// TODO implement readPixel
void pickColorPeriodicCallbackHandler(struct TouchEvent * const aActualPositionPtr) {
    // first check button
    if (!TouchButton::checkAllButtons(aActualPositionPtr->TouchPosition.PosX, aActualPositionPtr->TouchPosition.PosY)) {
        uint16_t tColor = readPixel(aActualPositionPtr->TouchPosition.PosX, aActualPositionPtr->TouchPosition.PosY);
        snprintf(StringBuffer, sizeof StringBuffer, "%3d %3d R=0x%2x G=0x%2x B=0x%2x", aActualPositionPtr->TouchPosition.PosX,
                aActualPositionPtr->TouchPosition.PosY, (tColor >> 11) << 3, ((tColor >> 5) & 0x3F) << 2, (tColor & 0x1F) << 3);
        BlueDisplay1.drawText(2, BlueDisplay1.getDisplayHeight() - TEXT_SIZE_11_DECEND, StringBuffer, TEXT_SIZE_11,
        COLOR_WHITE, COLOR_BLACK);
// show color
        BlueDisplay1.fillRect(BlueDisplay1.getDisplayWidth() - 30, BlueDisplay1.getDisplayHeight() - 30,
                BlueDisplay1.getDisplayWidth() - 1, BlueDisplay1.getDisplayHeight() - 1, tColor);
    }
}

/**
 * switch back to info menu
 */
void doInfoBackButton(BDButton * aTheTouchedButton, int16_t aValue) {
    drawInfoPage();
    sBackButtonPressed = true;
}

void initInfoPage(void) {
}

void drawInfoPage(void) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    for (unsigned int i = 0; i < INFO_BUTTONS_NUMBER_TO_DISPLAY; ++i) {
        TouchButtonsInfoPage[i]->drawButton();
    }
    TouchButtonMainHome.drawButton();
}

/**
 * allocate and position all BUTTONS
 */
void startInfoPage(void) {
    printSetPositionColumnLine(0, 0);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"

    int tPosY = 0;
//1. row
    TouchButtonInfoFont1.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_GREEN, "Font 1",
    TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

    TouchButtonInfoFont2.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_GREEN, "Font 2", TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

    TouchButtonBack.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_RED, "Back", TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, -1, &doInfoBackButton);

// 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonInfoMMC.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_GREEN, "MMC Infos",
    TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

    TouchButtonInfoSystem.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_GREEN, "System Infos", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

    TouchButtonInfoColors.init(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_GREEN, "Colors", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

// 3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonInfoUSB.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_GREEN, "USB Infos",
    TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

// 4. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;

// button for sub pages
    TouchButtonSettingsGamma1.init(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_GREEN,
    NULL, 0, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doSetGamma);
    TouchButtonSettingsGamma2.init(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_GREEN, NULL, 0, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 1, &doSetGamma);

#pragma GCC diagnostic pop

    drawInfoPage();
    registerRedrawCallback(&drawInfoPage);
}

void loopInfoPage(void) {
    ADC_setRawToVoltFactor();
    float tVBatValue = ADC1_getChannelValue(ADC_CHANNEL_VBAT, 4);
    tVBatValue *= 2 * sADCToVoltFactor;
// display VDDA voltage
    snprintf(StringBuffer, sizeof StringBuffer, "VCC=%5.3fV VBat=%5.3fV 3V=%#X|%d", sVDDA, tVBatValue, sReading3Volt,
            sReading3Volt);
    BlueDisplay1.drawText(2, BlueDisplay1.getDisplayHeight() - TEXT_SIZE_11_HEIGHT + TEXT_SIZE_11_ASCEND,
            StringBuffer,
            TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);

    delayMillisWithCheckAndHandleEvents(500);
}

/**
 * cleanup on leaving this page
 */
void stopInfoPage(void) {
// free buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsInfoPage) / sizeof(TouchButtonsInfoPage[0]); ++i) {
        TouchButtonsInfoPage[i]->deinit();
    }
}
