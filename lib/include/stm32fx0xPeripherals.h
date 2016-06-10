/**
 * @file stm32fx0xPeripherals.h
 *
 * @date 05.12.2012
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 *  * Port pin assignment
 * -------------------
 *
 * Port |Pin|Device |Function
 * -----|---|-------|--------
 * A    |0  |Button |User button INT input
 * A    |1  |ADC1   |Channel 2 / DSO-Input 1
 * A    |2  |ADC1   |Channel 3 / DSO-Input 2
 * A    |3  |ADC1   |Channel 4 / DSO-Input 3
 * A    |4  |DAC1   |Output 1
 * A    |5  |SPI1   |CLK
 * A    |6  |SPI1   |MISO
 * A    |7  |SPI1   |CLK
 * A    |8  |       |
 * A    |9  |Freq Synth |TIM2_CH3
 * A    |10 |USB    |Disconnect
 * A    |11 |USB    |DM
 * A    |12 |USB    |DP
 * A    |13 |SWD    |IO
 * A    |14 |SWD    |CLK
 * A    |15 |       |
 * &nbsp;|&nbsp;|&nbsp;|&nbsp;
 * B    |0  |HY32D      |CS
 * B    |1  |ADS7846    |INT input
 * B    |2  |ADS7846    |CS
 * B    |4  |HY32D      |DATA
 * B    |5  |HY32D      |WR
 * B    |6  |I2C        |CLK
 * B    |7  |I2C        |SDA
 * B    |8  |IR         |IR input (CAN RX)
 * B    |9  |IR         |IR LED (CAN TX)
 * B    |10 |HY32D      |RD
 * B    |11|            |
 * B    |12|            |
 * B    |13-15|SPI2     |not used yet
 * &nbsp;|&nbsp;|&nbsp;|&nbsp;
 * C    |0  |ADC2       |Channel 6 / AccuCap-Input 1
 * C    |1  |ADC2       |Channel 7 / AccuCap-Input 2
 * C    |4  |SD CARD    |INT input / CardDetect
 * C    |5  |SD CARD    |CS
 * C    |6  |TIM3       |Tone signal output
 * C    |7,8,9|DSO      |Attenuator control
 * C    |10 |USART3+4   |TX HC-05 Bluetooth
 * C    |11 |USART3+4   |RX HC-05 Bluetooth
 * C    |12 |DSO        |AC mode of preamplifier
 * C    |13 |Bluetooth  |HC-05 Bluetooth paired in
 * C    |14 |Intern     |Clock
 * C    |15 |Intern     |Clock
 * &nbsp;|&nbsp;|&nbsp;|&nbsp;
 * D    |0-15 |HY32D    |16 bit data
 * &nbsp;|&nbsp;|&nbsp;|&nbsp;
 * E    |0  |L3GD20     |INT input 1
 * E    |1  |L3GD20     |INT input 2
 * E    |2  |LSM303     |DRDY
 * E    |3  |L3GD20     |CS
 * E    |4  |LSM303     |INT input 1
 * E    |5  |LSM303     |INT input 2
 * E    |6  |           |
 * E    |7  |DEBUG      |
 * E    |8  |LED BLUE Back    | Touch error
 * E    |9  |LED RED Back     |
 * E    |10 |LED ORANGE Back  |
 * E    |11 |LED GREEN East   | TIM1_BRK_TIM15_IRQHandler/EXTI1_IRQHandler  toggle
 * E    |12 |LED BLUE Front   | Local TOUCH_ACTION_DOWN is active
 * E    |13 |LED RED Front    | MMC mount state
 * E    |14 |LED ORANGE Front | Timeout
 * E    |15 |LED GREEN West   | Default_IRQHandler toggle, assert_failed/printError blink
 * &nbsp;|&nbsp;|&nbsp;|&nbsp;
 * F full   |0,1|Clock  |Clock generator
 * F    |2  |RELAY      |Mode data acquisition Channel 1
 * F    |4  |RELAY      |Mode data acquisition Channel 2
 * F    |6  |HY32D      |TIM4 PWM output
 * F    |9  |RELAY      |Start/Stop data acquisition Channel 1
 * F    |10 |RELAY      |Start/Stop data acquisition Channel 2
 *
 *
 *
 *  Timer usage
 * -------------------
 *
 * Timer    |Function
 * ---------|--------
 * 1        |
 * 2        | Frequency synthesizer
 * 3        | PWM tone generation -> Pin C6
 * 4        | PWM backlight led -> Pin F6
 * 6        | ADC Timebase
 * 7        | DAC Timebase
 * 8        |
 * 15       | IR handler Interrupt
 * 16       |
 * 17       | PWM IR generation -> Pin B9
 *
 *
 *  Interrupt priority (lower value is higher priority)
 * ---------------------
 * 4 bits for pre-emption priority + 0 bits for subpriority
 *
 * Prio | ISR Nr| Name                 | Usage
 * -----|------------------------------|-------------
 * 1    | 0x22 | ADC1_2_IRQ            | ADC EOC - need fixed timing
 * 3    | 0x27 | USART3_IRQ            | Usart3 TX - short ISR to empty TX/print buffer
 * 4    | 0x10 | WWDG_IRQ              | Watchdog - we have 0.9 ms to reload before reset
 * 6    | 0x16 | EXTI0_IRQ             | User button - for screenshots
 * 7    | 0x1A | TIM1_BRK_TIM15_IRQ    | IR - higher than systic
 * 8    | 0x0F | SysTick               | SysTick - 1 ms to catch - ISR may need longer because of callbacks e.g. local slider handling
 * 9    | 0x2A | USBWakeUp_IRQ         | USB Wakeup
 * 10   | 0x24 | USB_LP_CAN1_RX0_IRQ   | USB Transfer
 * 12   | 0x17 | EXTI1_IRQ             | Touch
 * 13   | 0x1B | DMA1_Channel1_IRQ     | ADC DMA - low because ISR takes almost complete CPU
 * 15   | 0x46 | TIM6_DAC_IRQ          | ADC Timer - not used yet
 * 15   | 0x1A | EXTI4_IRQ             | MMC card detect
 *
 * DMA usage
 * ----------
 * DMA | Channel | Prio | Peripheral
 *   1 |       1 | high | ADC1
 *   1 |       2 |  low | USART3_TX
 *   1 |       3 |  low | USART3_RX
 *
 * Backup register
 * DR0  | Unused |  because not existent on F103
 * DR1  | Touch  |  Magic number for
 * DR2-8| Touch  |  Matrix for touch position calibration
 * D9   | AccuCap|  ChargeVoltageRaw
 *
 ***********************
 * 32F103 QFPN48
 * *********************
 * Port |Pin|Device |Function
 * -----|---|-------|--------
 * A    |0  |ADC12  |Channel 0 / DSO-Input 1
 * A    |1  |ADC12  |Channel 1 / DSO-Input 2
 * A    |2  |ADC12  |Channel 2 / DSO-Input 3
 * A    |3  |ADC12  |Channel 3 / DSO-Input 4
 * A    |4  |   |
 * A    |5  |   |
 * A    |6  |   |
 * A    |7  |   |
 * A    |8  |TIM1   |CH1
 * A    |9  |USART1 |TX
 * A    |10 |USART1 |RX
 * A    |9  |       |
 * A    |10 |USB    |Disconnect
 * A    |11 |USB    |DM
 * A    |12 |USB    |DP
 * A    |13 |SWD    |IO
 * A    |14 |SWD    |CLK
 * A    |15 |Freq Synth |TIM2_CH1
 * &nbsp;|&nbsp;|&nbsp;|&nbsp;
 * B    |0  |      |
 * B    |1  |      |
 * B    |2  |      |
 * B    |4  |      |
 * B    |5  |      |
 * B    |6  |TONE  |TIM4_CH1
 * B    |7  |      |
 * B    |8  |ATTEN |Range select 0
 * B    |9  |ATTEN |Range select 1
 * B    |10 |ATTEN |Range select 2
 * B    |11 |ATTEN | AC/DC
 * B    |12 |ATTEN |Attenuator detect 0
 * B    |13 |ATTEN |Attenuator detect 1
 * B    |14 |      |
 * B    |15 |      |
 * &nbsp;|&nbsp;|&nbsp;|&nbsp;
 * C    |0  |ADC2  |Channel 6 / AccuCap-Input 1
 * C    |1  |ADC2  |Channel 7 / AccuCap-Input 2
 * C    |4  |      |
 * C    |5  |      |
 * C    |6  |      |
 * C    |7,8,9|DSO |
 * C    |10 |      |
 * C    |11 |      |
 * C    |12 |      |
 * C    |13 |LED   |On Board Led
 * C    |14 |LSE   |32 kHz
 * C    |15 |LSE   |32 kHz
 *
 *  Timer usage
 * -------------------
 *
 * Timer    |Function
 * ---------|--------
 * 1        |
 * 2        | Frequency synthesizer
 * 3        | ADC Timebase
 * 4        | Tone
 */

#ifndef STM32F30XPERIPHERALS_H_
#define STM32F30XPERIPHERALS_H_

#ifdef STM32F10X
#include <stm32f1xx.h>
#endif
#ifdef STM32F30X
#include <stm32f3xx.h>
#endif

#include <stdbool.h>

#include <time.h>

/** @addtogroup Peripherals_Library
 * @{
 */

extern ADC_HandleTypeDef ADC1Handle;
extern ADC_HandleTypeDef ADC2Handle;
extern DMA_HandleTypeDef DMA11_ADC1_Handle;
extern TIM_HandleTypeDef TIM_DSOHandle;
extern TIM_HandleTypeDef TIM15Handle;
extern RTC_HandleTypeDef RTCHandle;
extern SPI_HandleTypeDef SPI1Handle;

#ifdef STM32F303xC
/**
 * ADC 1 for DSO
 */
#define ADC1_PERIPH_CLOCK                       RCC_AHBPeriph_ADC12

#define DSO_INPUT1_PIN                          GPIO_PIN_1
#define DSO_INPUT2_PIN                          GPIO_PIN_2
#define DSO_INPUT3_PIN                          GPIO_PIN_3
#define DSO_INPUT_PINS                          GPIO_PIN_1 | GPIO_PIN_2 |GPIO_PIN_3
#define DSO_INPUT_PORT                          GPIOA

#define DSO_ATTENUATOR_0_PIN                   GPIO_PIN_7
#define DSO_ATTENUATOR_1_PIN                   GPIO_PIN_8
#define DSO_ATTENUATOR_2_PIN                   GPIO_PIN_9
#define DSO_AC_RANGE_PIN                       GPIO_PIN_12
#define DSO_ATTENUATOR_PORT                    GPIOC

#define ADC_SAMPLETIME_SENSOR       ADC_SAMPLETIME_61CYCLES_5

#define ADC1_EXTERNALTRIGCONV       ADC_EXTERNALTRIGCONV_T6_TRGO //TIM6_TRGO event

#define ADC_DSO_TIMER               TIM6
#define ADC_DSO_TIMER_CLK_ENABLE()  __TIM6_CLK_ENABLE()
#define ADC_DSO_TIMER_CLOCK         RCC_APB1Periph_TIM6

#define TONE_TIMER                  TIM3
#define TONE_TIMER_CLK_ENABLE()     __TIM3_CLK_ENABLE()
#define TONE_TIMER_PIN              GPIO_PIN_6
#define TONE_TIMER_PORT             GPIOC
#define TONE_TIMER_IO_CLK_ENABLE()  __GPIOC_CLK_ENABLE()

#define SYNTH_TIMER                 TIM2
#define SYNTH_TIMER_CHANNEL         TIM_CHANNEL_3
#define SYNTH_TIMER_CLOCK_ENABLE()  __TIM2_CLK_ENABLE()
#define SYNTH_TIMER_PIN             GPIO_PIN_9
#define SYNTH_TIMER_PORT            GPIOA
#define SYNTH_TIMER_IO_CLOCK_ENABLE()  __GPIOA_CLK_ENABLE()

// LED_8 / LED_ORANGE2
#define TIMEOUT_LED_PIN             GPIO_PIN_14
#define TIMEOUT_LED_PORT            GPIOE
#define TIMEOUT_LED_CLOCK_ENABLE()  __GPIOE_CLK_ENABLE()

#define DEBUG_PIN                   GPIO_PIN_7
#define DEBUG_PORT                  GPIOE
#define DEBUG_CLOCK_ENABLE()        __GPIOE_CLK_ENABLE()

/**
 *  2 pins for AccuCapacity relays
 */
#define ACCUCAP_DISCHARGE_1_PIN     GPIO_PIN_9
#define ACCUCAP_DISCHARGE_2_PIN     GPIO_PIN_10
#define ACCUCAP_CHARGE_1_PIN        GPIO_PIN_2
#define ACCUCAP_CHARGE_2_PIN        GPIO_PIN_4
#define ACCUCAP_PORT                GPIOF
#define ACCUCAP_CLOCK_ENABLE()      __GPIOF_CLK_ENABLE()

#endif

#ifdef STM32F103xB

/**
 * ADC 1 for DSO
 */
#define ADC1_PERIPH_CLOCK                       RCC_AHBPeriph_ADC12

#define DSO_INPUT1_PIN                          GPIO_PIN_0
#define DSO_INPUT2_PIN                          GPIO_PIN_1
#define DSO_INPUT3_PIN                          GPIO_PIN_2
#define DSO_INPUT4_PIN                          GPIO_PIN_3
#define DSO_INPUT_PINS                          GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |GPIO_PIN_3

#define DSO_INPUT_PORT                          GPIOA

#define DSO_ATTENUATOR_DETECT_0_PIN             GPIO_PIN_12
#define DSO_ATTENUATOR_DETECT_1_PIN             GPIO_PIN_13

#define DSO_ATTENUATOR_0_PIN                    GPIO_PIN_8
#define DSO_ATTENUATOR_1_PIN                    GPIO_PIN_9
#define DSO_ATTENUATOR_2_PIN                    GPIO_PIN_10
#define DSO_AC_RANGE_PIN                        GPIO_PIN_11
#define DSO_ATTENUATOR_PORT                     GPIOB

#define ADC1_EXTERNALTRIGCONV       ADC1_2_EXTERNALTRIG_T3_TRGO

#define ADC_DSO_TIMER TIM3
#define ADC_DSO_TIMER_CLK_ENABLE()  __TIM3_CLK_ENABLE()
#define ADC_SAMPLETIME_SENSOR       ADC_SAMPLETIME_71CYCLES_5

#define TONE_TIMER                  TIM4
#define TONE_TIMER_CLK_ENABLE()     __TIM4_CLK_ENABLE()
#define TONE_TIMER_IO_CLK_ENABLE()  __GPIOB_CLK_ENABLE()
#define TONE_TIMER_PIN              GPIO_PIN_6
#define TONE_TIMER_PORT             GPIOB
#define TONE_TIMER_IO_CLK_ENABLE()  __GPIOB_CLK_ENABLE()

#define SYNTH_TIMER                 TIM2
#define SYNTH_TIMER_CHANNEL         TIM_CHANNEL_1
#define SYNTH_TIMER_CLOCK_ENABLE()  __TIM2_CLK_ENABLE()
#define SYNTH_TIMER_PIN             GPIO_PIN_15
#define SYNTH_TIMER_PORT            GPIOA
#define SYNTH_TIMER_IO_CLOCK_ENABLE()  __GPIOA_CLK_ENABLE()

#define TIMEOUT_LED_PIN             GPIO_PIN_1
#define TIMEOUT_LED_PORT            GPIOB
#define TIMEOUT_LED_CLOCK_ENABLE()  __GPIOB_CLK_ENABLE()

#define DEBUG_PIN                   GPIO_PIN_0
#define DEBUG_PORT                  GPIOB
#define DEBUG_CLOCK_ENABLE()        __GPIOB_CLK_ENABLE()

#endif

#define DAC_TIMER_MIN_RELOAD_VALUE 4

__STATIC_INLINE void Set_GpioPin(GPIO_TypeDef * aGpioBase, uint16_t aPin) {
#ifdef STM32F30X
    aGpioBase->BSRRL = aPin;
#else
    aGpioBase->BSRR = aPin;
#endif
}

__STATIC_INLINE void Reset_GpioPin(GPIO_TypeDef * aGpioBase, uint16_t aPin) {
#ifdef STM32F30X
    aGpioBase->BSRRH = aPin;
#else
    aGpioBase->BSRR = (uint32_t) aPin << 16;
#endif
}

__STATIC_INLINE void Set_DebugPin(void) {
    Set_GpioPin(DEBUG_PORT, DEBUG_PIN);
}
__STATIC_INLINE void Reset_DebugPin(void) {
    Reset_GpioPin(DEBUG_PORT, DEBUG_PIN);
}
__STATIC_INLINE void Toggle_DebugPin(void) {
    DEBUG_PORT->ODR ^= DEBUG_PIN;
}
__STATIC_INLINE void Reset_TimeoutLEDPin(void) {
    Reset_GpioPin(TIMEOUT_LED_PORT, TIMEOUT_LED_PIN);
}
__STATIC_INLINE void Toggle_TimeoutLEDPin(void) {
    TIMEOUT_LED_PORT->ODR ^= TIMEOUT_LED_PIN;
}

void Misc_IO_initalize(void);

#ifdef USE_STM32F3_DISCO
#include "stm32f3_discovery.h"
__STATIC_INLINE void setTimeoutLED(void) {
    BSP_LED_On(LED_ORANGE_2); // ORANGE FRONT
}

__STATIC_INLINE void resetTimeoutLED(void) {
    BSP_LED_Off(LED_ORANGE_2); // ORANGE FRONT
}

#else
__STATIC_INLINE void setTimeoutLED(void) {
}

__STATIC_INLINE void resetTimeoutLED(void) {
}

#endif

__STATIC_INLINE void ADC_DSO_startTimer(void) {
// Enable the TIM Counter
    __HAL_TIM_ENABLE(&TIM_DSOHandle);
//    ADC_DSO_TIMER->CR1 |= TIM_CR1_CEN;
}

__STATIC_INLINE void ADC_DSO_stopTimer(void) {
    __HAL_TIM_DISABLE(&TIM_DSOHandle);
//    ADC_DSO_TIMER->CR1 &= ~TIM_CR1_CEN;
}

__STATIC_INLINE void ADC_enableEOCInterrupt(ADC_HandleTypeDef* aADCHandle) {
//    HAL_ADC_Start_IT(aADCId);
    __HAL_ADC_ENABLE_IT(aADCHandle, ADC_IT_EOC);
}

__STATIC_INLINE void ADC_disableEOCInterrupt(ADC_HandleTypeDef* aADCHandle) {
    __HAL_ADC_DISABLE_IT(aADCHandle, ADC_IT_EOC);
}

/**
 * Stub to pass right parameters
 */
__STATIC_INLINE void ADC1_clearITPendingBit(ADC_HandleTypeDef* aADCHandle) {
//    ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
    __HAL_ADC_CLEAR_FLAG(aADCHandle, ADC_IT_EOC);
}
/*
 * ADC
 */
#define ADC1_INPUT1_CHANNEL     ADC_CHANNEL_2
#define ADC1_INPUT2_CHANNEL     ADC_CHANNEL_3
#define ADC1_INPUT3_CHANNEL     ADC_CHANNEL_4

#define DSO_DMA_CHANNEL   		DMA1_Channel1    // The only channel for ADC1
#define ADC_DEFAULT_TIMEOUT     3 // in milliseconds
#define ADC_MAX_CONVERSION_VALUE (4096 -1) // 12 bit

void ADC12_init(void);
void ADC1_init(void);
void ADC_enableAndWait(ADC_HandleTypeDef* aADCId);
void ADC_disableAndWait(ADC_HandleTypeDef* aADCId);
void ADC2_init(void);
void ADC_SelectChannelAndSetSampleTime(ADC_HandleTypeDef* aADCHandle, uint8_t aChannelNumber, bool aFastMode);
void ADC_enableEOCInterrupt(ADC_HandleTypeDef* aADCHandle);
void ADC_disableEOCInterrupt(ADC_HandleTypeDef* aADCHandle);
void ADC1_clearITPendingBit(ADC_HandleTypeDef* aADCHandle);
uint32_t ADC_getCFGR(void);

// ADC attenuator and range
#ifndef USE_STM32F3_DISCO
int DSO_detectAttenuatorType(void);
#endif
void DSO_initializeAttenuatorAndAC(void);
void DSO_setAttenuator(uint8_t aValue);
void DSO_setACMode(bool aValue);
bool DSO_getACMode(void);

// ADC timer
void ADC_initalizeTimer(void);
void ADC_Timer6EnableInterrupt(void);
void ADC_SetTimerPeriod(uint16_t Autoreload, uint16_t aPrescaler);
void ADC12_SetClockPrescaler(uint32_t aValue);

// ADC DMA
void ADC1_DMA_initialize(void);
void ADC1_DMA_start(uint32_t aMemoryBaseAddr, uint16_t aBufferSize, bool aModeCircular);
void ADC1_DMA_stop(void);
uint16_t DMA11_GetCurrDataCounter(void);

uint16_t ADC1_getChannelValue(uint8_t aChannel, int aOversamplingExponent);
void ADC_setRawToVoltFactor(void);
int ADC1_getTemperatureMilligrades(void);
float ADC_getTemperature(void);

unsigned int AccuCapacity_ADCRead(uint8_t aProbeNumber);

extern float sVDDA;
extern float sADCToVoltFactor;  // Factor for ADC Reading -> Volt
#define ADC_SCALE_FACTOR_SHIFT 18
extern unsigned int sADCScaleFactorShift18; // Factor for ADC Reading -> Display calibrated for sReading3Volt -> 240,  shift 18 because we have 12 bit ADC and 12 + 18 + 2 = 32 bit register
extern uint16_t sReading3Volt; // Approximately 4096
extern uint16_t sRefintActualReading; // actual value of VREFINT / see *((uint16_t*) 0x1FFFF7BA) = value of VREFINT at 3.3V
extern uint16_t sTempValueAtZeroDegreeShift3;

//SPI
void SPI1_initialize(void);
uint16_t SPI1_getPrescaler(void);

//RTC
void RTC_setMagicNumber(void);
bool RTC_checkMagicNumber(void);

extern bool RTC_DateIsValid; // true if year != 0
extern bool sRTCIsInitalized;
void RTC_InitLSEOn(void);
void RTC_initialize(void);
void RTC_PWRDisableBkUpAccess(void);
void fillTimeStructure(struct tm *aTimeStructurePtr);
uint8_t RTC_getSecond(void);
int RTC_getTimeStringForFile(char * aStringBuffer);
int RTC_getDateStringForFile(char * aStringBuffer);
int RTC_getTimeString(char * StringBuffer);
void RTC_setTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t dayOfWeek, uint8_t day, uint8_t month,
        uint16_t year);
//#define RTC_BKP_DR_NUMBER   16  /* RTC Backup Data Register Number */
void RTC_WriteBackupDataRegister(uint8_t aIndex, uint32_t aRTCBackupData);
uint32_t RTC_ReadBackupDataRegister(uint8_t aIndex);

/*
 * Window Watch dog
 */
void Watchdog_init(void);
void Watchdog_start(void);
#ifdef __cplusplus
extern "C" {
#endif
void Watchdog_reload(void);
#ifdef __cplusplus
}
#endif
void reset(void);

/*
 * Tone
 */
#define FEEDBACK_TONE_NO_ERROR 0
#define FEEDBACK_TONE_SHORT_ERROR 1
#define FEEDBACK_TONE_LONG_ERROR 2
#define FEEDBACK_TONE_NO_TONE 7

void initalizeTone(void);
void tone(uint16_t aFreqHertz, uint16_t aDurationMillis);
void FeedbackToneOK(void);
void FeedbackTone(unsigned int aFeedbackType);
void EndTone(void);
void noTone(void);

void setTimeoutLED(void);
void resetTimeoutLED(void);

#ifdef HAL_DAC_MODULE_ENABLED
/*
 * DAC
 */
extern DAC_HandleTypeDef DAC1Handle;
__STATIC_INLINE void DAC_SetOutputValue(uint16_t aOutputValue) {
//    DAC_SetChannel1Data(DAC_Align_12b_R, aOutputValue);
    HAL_DAC_SetValue(&DAC1Handle, DAC_CHANNEL_1, DAC_ALIGN_12B_R, aOutputValue);
}

void DAC_init(void);
void DAC_Timer_SetReloadValue(uint32_t aReloadValue);
void DAC_Start(void);
void DAC_Stop(void);
void DAC_ModeTriangle(void);
void DAC_ModeNoise(void);
void DAC_TriangleAmplitude(unsigned int aAmplitude);
void DAC_SetOutputValue(uint16_t aOutputValue);
#endif

void IR_Timer_initialize(uint16_t aAutoreload);
void IR_Timer_Start(void);
void IR_Timer_Stop(void);

void Synth_Timer_initialize(uint32_t aAutoreload);
void Synth_Timer32_SetReloadValue(uint32_t aReloadValue);
void Synth_Timer16_SetReloadValue(uint32_t aReloadValue, uint32_t aPrescalerValue);
void Synth_Timer_Start(void);
uint32_t Synth_Timer_Stop(void);
uint32_t HIRES_Timer_GetCounter(void);

void MICROSD_IO_initalize(void);

void AccuCapacity_IO_initalize(void);
void AccuCapacity_SwitchDischarge(unsigned int aChannelIndex, bool aActive);
void AccuCapacity_SwitchCharge(unsigned int aChannelIndex, bool aActive);

#ifdef __cplusplus
extern "C" {
#endif
/*
 * SPI
 */
extern SPI_HandleTypeDef * SPI1HandlePtr;
uint8_t SPI1_sendReceiveFast(uint8_t byte);
void SPI1_setPrescaler(uint16_t aPrescaler);

bool MICROSD_isCardInserted(void);
void MICROSD_CSEnable(void);
void MICROSD_CSDisable(void);
void MICROSD_ClearITPendingBit(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* STM32F30XPERIPHERALS_H_ */
