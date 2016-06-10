/**
 * @file stm32fx0xPeripherals.cpp
 *
 * @date 05.12.2012
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#include "stm32fx0xPeripherals.h"
#include "BlueDisplay.h" // for WWDG_IRQHandler

#include "timing.h"
#include <stdio.h>
/** @addtogroup Peripherals_Library
 * @{
 */

/**
 *  Calibrating with value from SystemMemory:
 *
 * ActualRefVoltageADC (normally VDDA) = ((CalValueForRefint * 3.3V) / ActualReadingRefint)
 *
 * 3V (gives better resolution than 1V)         Reading 3V                         3.0V * 4096 * ActualReadingRefint
 *  ----------------------------------  = --------------------- => Reading 3V = -------------------------------------
 *       ActualRefVoltageADC                Reading VRef (4096)                    CalValueForRefint * 3.3V
 *
 *  Formula for ScaleFactor (3V for 240 Pixel) is:
 *    Reading 3V * ScaleFactor = 240 (DISPLAY_HEIGHT)
 *
 *                       240  * 3.3V * CalValueForRefint
 *  => ADCScaleFactor = ---------------------------------
 *                       4096 * 3.0V * ActualReadingRefint
 *
 *                    CalValueForRefint
 *   = 0,064453125 * -------------------
 *                    ActualReadingRefint
 *
 *   0,064453125 * 2^18(=262144) = 16896.0
 *
 */

float sADCToVoltFactor; // Factor for ADC Reading -> Volt = sADCRefVoltage / 4096
uint16_t sReading3Volt; // Raw value for 3V input
uint16_t sRefintActualReading; // actual conversion value of VREFINT / see VREFINT_CAL or *((uint16_t*) 0x1FFFF7BA) = value of VREFINT at 3.3V
unsigned int sADCScaleFactorShift18; // Factor for ADC Reading -> Display for 0,5 Volt/div(=40pixel) scale - shift 18 because we have 12 bit ADC and 12 + 18 + sign + buffer = 32 bit register
float sVDDA;
uint16_t sTempValueAtZeroDegreeShift3 = 0;

#ifdef HAL_DAC_MODULE_ENABLED
DAC_HandleTypeDef DAC1Handle;
#endif
ADC_HandleTypeDef ADC1Handle;
static bool isADCInitializedForTimerEvents;

ADC_HandleTypeDef ADC2Handle;
DMA_HandleTypeDef DMA11_ADC1_Handle;
TIM_HandleTypeDef TIM15Handle;
TIM_HandleTypeDef TIMSynthHandle;
TIM_HandleTypeDef TIMToneHandle;
TIM_HandleTypeDef TIM_DSOHandle;
TIM_HandleTypeDef TIM7Handle;
RTC_HandleTypeDef RTCHandle;
SPI_HandleTypeDef * SPI1HandlePtr;
#ifdef HAL_WWDG_MODULE_ENABLED
WWDG_HandleTypeDef WWDGHandle;
#endif
ADC_ChannelConfTypeDef ADCChannelConfigDefault; // pre filled structure for ADC channels - ADC_REGULAR_RANK_1, ADC_SINGLE_ENDED, ADC_OFFSET_NONE
TIM_OC_InitTypeDef TIMOCInitDefault;

/**
 * ADC must be ready and continuous mode disabled
 * read ref channel and use Value from Address 0x1FFFF7BA
 * USE ADC 1
 */
#define VREFINT_CAL       ((uint16_t*) ((uint32_t)0x1FFFF7BA))  /* Internal temperature sensor, parameter VREFINT_CAL: Raw data acquired at a temperature of 30 DegC (+-5 DegC), VDDA = 3.3 V (+-10 mV) */

void ADC_setRawToVoltFactor(void) {

    if (isADCInitializedForTimerEvents) {
#ifdef STM32F30X
        MODIFY_REG(ADC1Handle.Instance->CFGR, ADC_CFGR_EXTSEL|ADC_CFGR_EXTEN,
                ADC_SOFTWARE_START | ADC_EXTERNALTRIGCONVEDGE_NONE);
#else
        MODIFY_REG(ADC1Handle.Instance->CR2, ADC_CR2_EXTSEL, ADC_SOFTWARE_START);
#endif
    }

    // Set one channel for rank1
#ifdef STM32F30X
    ADC1Handle.Instance->SQR1 = __HAL_ADC_SQR1_RK(ADC_CHANNEL_VREFINT, 1);
    // get VREFINT_CAL (value of reading calibration input acquired at temperature of 30 °C and VDDA=3.3 V) - see chapter 3.12.2 of datasheet
    uint16_t tREFCalibrationValue = *VREFINT_CAL;
#else
    ADC1Handle.Instance->SQR3 = ADC_CHANNEL_VREFINT;
    // assume 1.2 Volt
    uint16_t tREFCalibrationValue = (1.2 * 4095) / 3.3;// = 1489
#endif

    bool tTimeout = false;
    setTimeoutMillis(10);
    uint32_t tRefActualReadingTimes8 = 0;

    for (int i = 0; i < 8; ++i) {
        // Do not use HAL_ADC_Start() is "enables" ADC first and calls MSP_init
#ifdef STM32F30X
        SET_BIT(ADC1Handle.Instance->CR, ADC_CR_ADSTART);
#else
        // could use external software trigger and (ADC_CR2_SWSTART | ADC_CR2_EXTTRIG) instead
        SET_BIT(ADC1Handle.Instance->CR2, ADC_CR2_ADON);
#endif

        /* Test EOC flag */
        while (__HAL_ADC_GET_FLAG(&ADC1Handle, ADC_FLAG_EOC) == RESET) {
#ifdef STM32F30X
            if (isTimeout(ADC1Handle.Instance->ISR)) {
#else
                if (isTimeout(ADC1Handle.Instance->SR)) {
#endif
                tTimeout = true;
                break;
            }
        }
        // TODO check word access
        tRefActualReadingTimes8 += ADC1Handle.Instance->DR & 0xFFFF; // & 0xFFFF since on 32F1xx the upper word contains ADC2->DR value
    }

    if (tTimeout) {
        sRefintActualReading = tREFCalibrationValue;
    } else {
        sRefintActualReading = tRefActualReadingTimes8 / 8;
    }
    // use stored calibration value from system area - 0,064453125 * 2^21 = 135168
    sVDDA = 3.3 * tREFCalibrationValue / sRefintActualReading;
    sADCScaleFactorShift18 = tREFCalibrationValue * 135168 / tRefActualReadingTimes8;
    sReading3Volt = (3.0 * tRefActualReadingTimes8 * (4096 / 8)) / (3.3 * tREFCalibrationValue);
    sADCToVoltFactor = 3.3 * tREFCalibrationValue / (tRefActualReadingTimes8 * (4096 / 8));
    //    // restore ADC configuration for timer events
    if (isADCInitializedForTimerEvents) {
#ifdef STM32F30X
        MODIFY_REG(ADC1Handle.Instance->CFGR, ADC_CFGR_EXTSEL|ADC_CFGR_EXTEN,
                ADC1_EXTERNALTRIGCONV | ADC_EXTERNALTRIGCONVEDGE_RISING);
#else
        MODIFY_REG(ADC1Handle.Instance->CR2, ADC_CR2_EXTSEL, ADC1_EXTERNALTRIGCONV);
#endif
    }
}

// TODO replace HAL_ADC_ConfigChannel (448 bytes code size)
void MY_ADC_ConfigChannel(ADC_HandleTypeDef* hadc, ADC_ChannelConfTypeDef* sConfig) {

}

/**
 * ADC2 for AccuCapacity
 */
#define ADC2_PERIPH_CLOCK                       RCC_AHBPeriph_ADC12

// Two input pins
#define ADC2_INPUT1_CHANNEL                     ADC_Channel_6
#define ADC2_INPUT1_PIN                         GPIO_PIN_0
#define ADC2_INPUT1_PORT                        GPIOC

#define ADC2_INPUT2_CHANNEL                     ADC_Channel_7
#define ADC2_INPUT2_PIN                         GPIO_PIN_1
#define ADC2_INPUT2_PORT                        GPIOC

void HAL_ADC_MspInit(ADC_HandleTypeDef* aADCHandle) {
    GPIO_InitTypeDef GPIO_InitStructure;

    // use slow clock
#ifdef STM32F30X
    __HAL_RCC_ADC12_CONFIG(RCC_ADC12PLLCLK_DIV64); // 1,125 MHz
#else
            __HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV8); // 9 MHz
#endif
    if (aADCHandle == &ADC1Handle) {
        __ADC1_CLK_ENABLE();
        /**
         * ADC GPIO input pin init
         */
        __GPIOA_CLK_ENABLE();
        // Configure pin A0 + A1 + A2 (+ A3) as analog input
        GPIO_InitStructure.Pin = DSO_INPUT_PINS;
        GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStructure.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(DSO_INPUT_PORT, &GPIO_InitStructure);

        // almost highest priority for ADC
        NVIC_SetPriority((IRQn_Type) (ADC1_2_IRQn), 1);
        HAL_NVIC_EnableIRQ((IRQn_Type) (ADC1_2_IRQn));

        ADC1_DMA_initialize();
    } else {
        __ADC2_CLK_ENABLE();
        /**
         * ADC GPIO input pin init
         */
        __GPIOC_CLK_ENABLE();
        /* Configure pin as analog input */
        GPIO_InitStructure.Pin = ADC2_INPUT1_PIN | ADC2_INPUT2_PIN;
        GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStructure.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(ADC2_INPUT1_PORT, &GPIO_InitStructure);
    }
}

void ADC2_init(void) {
    ADC2Handle.Instance = ADC2;

    /*
     * configure ADC
     */
    ADC2Handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    ADC2Handle.Init.ScanConvMode = ADC_SCAN_DISABLE;
    ADC2Handle.Init.ContinuousConvMode = DISABLE; // one conversion after trigger
    ADC2Handle.Init.NbrOfConversion = 1;
    ADC2Handle.Init.DiscontinuousConvMode = DISABLE;
    ADC2Handle.Init.NbrOfDiscConversion = 1;
    ADC2Handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
#ifdef STM32F30X
    ADC2Handle.Init.ClockPrescaler = ADC_CLOCK_ASYNC; // uses ADC_12CK Clock source
    ADC2Handle.Init.Resolution = ADC_RESOLUTION12b;
    ADC2Handle.Init.EOCSelection = EOC_SINGLE_CONV;
    ADC2Handle.Init.LowPowerAutoWait = DISABLE;
    ADC2Handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    ADC2Handle.Init.DMAContinuousRequests = DISABLE;
    ADC2Handle.Init.Overrun = OVR_DATA_OVERWRITTEN;
#endif
    /*
     * Starts also the voltage regulator and calls HAL_ADC_MspInit()
     */
    HAL_ADC_Init(&ADC2Handle);

// Calibration
#ifdef STM32F30X
    HAL_ADCEx_Calibration_Start(&ADC2Handle, ADC_SINGLE_ENDED);
#else
    HAL_ADCEx_Calibration_Start(&ADC2Handle);
#endif

// Enable ADC
    ADC_enableAndWait(&ADC2Handle);
}

void ADC12_init(void) {
    /*
     * pre fill default channel structure
     */
    ADCChannelConfigDefault.Rank = ADC_REGULAR_RANK_1;
#ifdef STM32F30X
    ADCChannelConfigDefault.SingleDiff = ADC_SINGLE_ENDED;
    ADCChannelConfigDefault.OffsetNumber = ADC_OFFSET_NONE;
    ADCChannelConfigDefault.Offset = 0;
#endif
    int tErrorCode;

    __ADC1_CLK_ENABLE();
    __ADC2_CLK_ENABLE();
    /*
     * Configure and enable special channels / set sample time
     */
    ADC1Handle.Instance = ADC1;
// TEMP + VBAT channel need 2.2 micro sec. sampling so ADC_SAMPLETIME_61CYCLES_5 should be enough;
    ADCChannelConfigDefault.Channel = ADC_CHANNEL_TEMPSENSOR;
    ADCChannelConfigDefault.SamplingTime = ADC_SAMPLETIME_SENSOR;
// Mainly - set sampling time
    tErrorCode = HAL_ADC_ConfigChannel(&ADC1Handle, &ADCChannelConfigDefault);

// REFINT is also available on ADC2
    ADCChannelConfigDefault.Channel = ADC_CHANNEL_VREFINT;
    ADCChannelConfigDefault.SamplingTime = ADC_SAMPLETIME_SENSOR;
    tErrorCode |= HAL_ADC_ConfigChannel(&ADC1Handle, &ADCChannelConfigDefault);

#ifdef STM32F30X
    ADCChannelConfigDefault.Channel = ADC_CHANNEL_VBAT;
    ADCChannelConfigDefault.SamplingTime = ADC_SAMPLETIME_SENSOR;
    tErrorCode |= HAL_ADC_ConfigChannel(&ADC1Handle, &ADCChannelConfigDefault);
#endif
    assert_param(tErrorCode == HAL_OK);
}

/**
 * ADC1
 */

void ADC1_init(void) {
    int tErrorCode;

    ADC1Handle.Instance = ADC1;
// must be done before HAL_ADC_Init()
    ADC1Handle.DMA_Handle = &DMA11_ADC1_Handle;

    ADC1Handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    ADC1Handle.Init.ScanConvMode = DISABLE;
    ADC1Handle.Init.ContinuousConvMode = DISABLE; // one conversion after trigger
    ADC1Handle.Init.NbrOfConversion = 1;
    ADC1Handle.Init.DiscontinuousConvMode = DISABLE;
    ADC1Handle.Init.NbrOfDiscConversion = 1;
#ifdef STM32F30X
    ADC1Handle.Init.ClockPrescaler = ADC_CLOCK_ASYNC; // uses ADC_12CK Clock source
    ADC1Handle.Init.Resolution = ADC_RESOLUTION12b;
    ADC1Handle.Init.EOCSelection = EOC_SINGLE_CONV;
    ADC1Handle.Init.LowPowerAutoWait = DISABLE;
    ADC1Handle.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T6_TRGO; //TIM6_TRGO event
    ADC1Handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    ADC1Handle.Init.DMAContinuousRequests = DISABLE;
    ADC1Handle.Init.Overrun = OVR_DATA_OVERWRITTEN;
#endif
    /*
     * this call starts also the voltage regulator, waits for stabilizing and calls HAL_ADC_MspInit()
     */
    tErrorCode = HAL_ADC_Init(&ADC1Handle);

    /*
     * calibration
     */
#ifdef STM32F30X
    tErrorCode |= HAL_ADCEx_Calibration_Start(&ADC1Handle, ADC_SINGLE_ENDED);
#else
    tErrorCode |= HAL_ADCEx_Calibration_Start(&ADC1Handle);
#endif
    assert_param(tErrorCode == HAL_OK);

    /*
     * Multi mode disabled
     */
//    ADC_MultiModeTypeDef tADC_MultiModeInitStructure;
//    tADC_MultiModeInitStructure.Mode = ADC_MODE_INDEPENDENT;
#ifdef STM32F30X
//    tADC_MultiModeInitStructure.DMAAccessMode = ADC_DMAACCESSMODE_DISABLED; // multiple DMA mode disabled
//    tADC_MultiModeInitStructure.TwoSamplingDelay = 0;
#endif
//    HAL_ADCEx_MultiModeConfigChannel(&ADC1Handle, &tADC_MultiModeInitStructure);

//
    isADCInitializedForTimerEvents = true;

    /* Enable ADC DMA mode */
#ifdef STM32F30X
    SET_BIT(ADC1Handle.Instance->CFGR, ADC_CFGR_DMAEN);
#else
    SET_BIT(ADC1Handle.Instance->CR2, ADC_CR2_DMA);
#endif

    /* Enable ADC */
    ADC_enableAndWait(&ADC1Handle);

    // set actual multiplier etc. by reading the ref voltage
    ADC_setRawToVoltFactor();

    DSO_initializeAttenuatorAndAC();
    ADC_initalizeTimer();
}

void ADC_enableAndWait(ADC_HandleTypeDef* aADCHandle) {
    __HAL_ADC_ENABLE(aADCHandle);
#ifdef STM32F30X
    // wait for ADRDY
    setTimeoutMillis(50);
    while (__HAL_ADC_GET_FLAG(aADCHandle, ADC_FLAG_RDY) == RESET) {
        __HAL_ADC_ENABLE(aADCHandle); // The statement above is not sufficient :-(
        if (isTimeout(aADCHandle->Instance->CR)) {
            break;
        }
    }
#else
// wait for 0-1 usec (not really needed yet)
#endif
}

void ADC_disableAndWait(ADC_HandleTypeDef* aADCHandle) {
#ifdef STM32F30X
// stop if not already done
    if (HAL_IS_BIT_SET(aADCHandle->Instance->CR, ADC_CR_ADSTART)) {
        SET_BIT(aADCHandle->Instance->CR, ADC_CR_ADSTP);
    }
// wait for stop to complete
    setTimeoutMillis(20);
    while (HAL_IS_BIT_SET(aADCHandle->Instance->CR, ADC_CR_ADSTP)) {
        if (isTimeout(aADCHandle->Instance->CR)) {
            break;
        }
    }
// then disable ADC otherwise sometimes interrupts just stops after the first reading
    __HAL_ADC_DISABLE(aADCHandle);
// wait for disable to complete
    setTimeoutMillis(10);
    while (HAL_IS_BIT_SET(aADCHandle->Instance->CR, ADC_CR_ADDIS)) {
        if (isTimeout(aADCHandle->Instance->CR)) {
            break;
        }
    }
#else
    __HAL_ADC_DISABLE(aADCHandle);
#endif
}

/**
 * ADC must be ready and continuous mode disabled
 * @param aChannel
 * @param aOversamplingExponent 0 -> 2^0 = 1 sample, 1 -> 2^1 = 2 samples etc.
 * @return 2 * ADC_MAX_CONVERSION_VALUE on timeout
 */
uint16_t ADC1_getChannelValue(uint8_t aChannel, int aOversamplingExponent) {

    if (isADCInitializedForTimerEvents) {
#ifdef STM32F30X
        MODIFY_REG(ADC1Handle.Instance->CFGR, ADC_CFGR_EXTSEL|ADC_CFGR_EXTEN,
                ADC_SOFTWARE_START | ADC_EXTERNALTRIGCONVEDGE_NONE);
#else
        MODIFY_REG(ADC1Handle.Instance->CR2, ADC_CR2_EXTSEL, ADC_SOFTWARE_START);
#endif
    }
// use conservative sample time (ADC Clocks!)
// temperature needs 2.2 micro seconds
    ADCChannelConfigDefault.Channel = aChannel;
    ADCChannelConfigDefault.SamplingTime = ADC_SAMPLETIME_SENSOR;
    HAL_ADC_ConfigChannel(&ADC1Handle, &ADCChannelConfigDefault);

// use slow clock
#ifdef STM32F30X
    __HAL_RCC_ADC12_CONFIG(RCC_ADC12PLLCLK_DIV64); // 1,125 MHz
#else
            __HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV8); // 9 MHz
#endif
    uint16_t tValue = 0;
    bool tTimeout = false;
    for (int i = 0; i < (1 << aOversamplingExponent); ++i) {
#ifdef STM32F30X
        SET_BIT(ADC1Handle.Instance->CR, ADC_CR_ADSTART);
#else
        // could use external software trigger and (ADC_CR2_SWSTART | ADC_CR2_EXTTRIG) instead
        SET_BIT(ADC1Handle.Instance->CR2, ADC_CR2_ADON);
#endif        /* Test EOC flag */
        setTimeoutMillis(ADC_DEFAULT_TIMEOUT);
        while (__HAL_ADC_GET_FLAG(&ADC1Handle, ADC_FLAG_EOC) == RESET) {
#ifdef STM32F30X
            if (isTimeout(ADC1Handle.Instance->ISR)) {
#else
                if (isTimeout(ADC1Handle.Instance->SR)) {
#endif
                tTimeout = true;
                break;
            }
        }
        tValue += HAL_ADC_GetValue(&ADC1Handle);
    }
    if (tTimeout) {
        return 2 * ADC_MAX_CONVERSION_VALUE;
    }
    tValue = tValue >> aOversamplingExponent;

//    // restore ADC configuration for timer events
    if (isADCInitializedForTimerEvents) {
#ifdef STM32F30X
        MODIFY_REG(ADC1Handle.Instance->CFGR, ADC_CFGR_EXTSEL|ADC_CFGR_EXTEN,
                ADC1_EXTERNALTRIGCONV |ADC_EXTERNALTRIGCONVEDGE_RISING);
#else
        MODIFY_REG(ADC1Handle.Instance->CR2, ADC_CR2_EXTSEL, ADC1_EXTERNALTRIGCONV);
#endif
    }
    return tValue;
}

void ADC_SelectChannelAndSetSampleTime(ADC_HandleTypeDef* aADCHandle, uint8_t aChannelNumber, bool aFastMode) {
    ADCChannelConfigDefault.Channel = aChannelNumber;
    uint32_t tSampleTime = 0;
    switch (aChannelNumber) {
    case ADC_CHANNEL_1: // fast channels
    case ADC_CHANNEL_2: // fast channels
    case ADC_CHANNEL_3:
#ifdef STM32F30X
        tSampleTime = ADC_SAMPLETIME_4CYCLES_5;
#else
        case ADC_CHANNEL_0: // fast channels
        tSampleTime = ADC_SAMPLETIME_7CYCLES_5;
#endif
        break;
    case ADC_CHANNEL_7: //slow channel
        break;
    case ADC_CHANNEL_TEMPSENSOR:
    case ADC_CHANNEL_VREFINT:
#ifdef STM32F30X
    case ADC_CHANNEL_VBAT:
#endif
        tSampleTime = ADC_SAMPLETIME_SENSOR;
// TEMP + VBAT channel need 2.2 micro sec. sampling
// VREFINT needs 5.1 micro sec on F103
//        tSampleTime = ADC_SAMPLETIME_61CYCLES_5;
// already set at ADC_init => just return
        break;
    default:
        break;
    }
    if (aFastMode || tSampleTime == 0) {
        tSampleTime = ADC_SAMPLETIME_1CYCLE_5;
    }
    ADCChannelConfigDefault.SamplingTime = tSampleTime;
    HAL_ADC_ConfigChannel(aADCHandle, &ADCChannelConfigDefault);
}

#ifdef STM32F30X
/**
 * from 0x00 to 0x0B which results in prescaler from 1 to 256
 */
void ADC12_SetClockPrescaler(uint32_t aValue) {
    aValue &= 0x0F;
    if (aValue > 0x0B) {
        aValue = 0x0B;
    }
    aValue = (aValue << 4) + RCC_ADC12PLLCLK_DIV1;
//RCC_ADCCLKConfig(aValue);
    CLEAR_BIT(RCC->CFGR2, RCC_CFGR2_ADCPRE12);
    /* Set ADCPRE bits according to RCC_PLLCLK value */
    SET_BIT(RCC->CFGR2, aValue);
}
#endif

#define STM32F3D_ADC_TIMER_PRESCALER_START (9 - 1)

void ADC_initalizeTimer(void) {
    TIM_DSOHandle.Instance = ADC_DSO_TIMER;

    /* TIM3 TIM6 clock enable */
    ADC_DSO_TIMER_CLK_ENABLE();

    /* Time base configuration */
    TIM_DSOHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    TIM_DSOHandle.Init.Prescaler = STM32F3D_ADC_TIMER_PRESCALER_START; // 1/8 us resolution (72/9)
    TIM_DSOHandle.Init.Period = 25 - 1; // 10 us
    TIM_DSOHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    TIM_DSOHandle.Init.RepetitionCounter = 0x00;
    HAL_TIM_Base_Init(&TIM_DSOHandle);

    TIM_MasterConfigTypeDef tMasterConfig;
    tMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE; // trigger on each update
    tMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&TIM_DSOHandle, &tMasterConfig);
//    TIM_SelectOutputTrigger(STM32F3D_ADC_TIMER, TIM_TRGOSource_Update);

// enable Interrupt for testing purposes
//  ADC_Timer6EnableInterrupt();
}

/**
 * not used yet
 */
//void ADC_Timer6EnableInterrupt(void) {
//// lowest possible priority
//    NVIC_SetPriority((IRQn_Type) (TIM6_DAC_IRQn), 15);
//    HAL_NVIC_EnableIRQ((IRQn_Type) (TIM6_DAC_IRQn));
//    __HAL_TIM_ENABLE_IT(&TIM_DSOHandle, TIM_IT_UPDATE);
//}
void ADC_SetTimerPeriod(uint16_t aDivider, uint16_t aPrescalerDivider) {
    TIM_DSOHandle.Instance->PSC = aPrescalerDivider - 1;
    __HAL_TIM_SetAutoreload(&TIM_DSOHandle, aDivider - 1)
    ;
    HAL_TIM_GenerateEvent(&TIM_DSOHandle, TIM_EventSource_Update);
// TIM_PrescalerConfig(STM32F3D_ADC_TIMER, aPrescalerDivider - 1, TIM_PSCReloadMode_Immediate);
}

/*
 * ADC DMA
 */
void ADC1_DMA_initialize(void) {
    __DMA1_CLK_ENABLE();

    DMA_HandleTypeDef * tDMA_ADCHandle = ADC1Handle.DMA_Handle;

    tDMA_ADCHandle->Instance = DMA1_Channel1;

// DMA1 channel1 configuration
    tDMA_ADCHandle->Init.Direction = DMA_PERIPH_TO_MEMORY;
    tDMA_ADCHandle->Init.PeriphInc = DMA_PINC_DISABLE;
    tDMA_ADCHandle->Init.MemInc = DMA_MINC_ENABLE;
    tDMA_ADCHandle->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    tDMA_ADCHandle->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    tDMA_ADCHandle->Init.Mode = DMA_NORMAL;
    tDMA_ADCHandle->Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(tDMA_ADCHandle);

    tDMA_ADCHandle->Instance->CPAR = (uint32_t) &ADC1Handle.Instance->DR;

//  prio 13 because it takes so much CPU that touch events do not really work
    NVIC_SetPriority((IRQn_Type) (DMA1_Channel1_IRQn), 13);
    HAL_NVIC_EnableIRQ((IRQn_Type) (DMA1_Channel1_IRQn));

//TC -> transfer complete interrupt
// Enable DMA1 channel1
//  DMA_ITConfig(DSO_DMA_CHANNEL, DMA_IT_TC | DMA_IT_HT | DMA_IT_TE, ENABLE);

    __HAL_DMA_ENABLE_IT(tDMA_ADCHandle, DMA_IT_TC| DMA_IT_TE);
}

//void ADC12_initForDualMode(void) {
//    ADC_CommonInitTypeDef ADC12_InitStructure;
//    ADC12_InitStructure.ADC_Clock = ADC_Clock_SynClkModeDiv1;
//    ADC12_InitStructure.ADC_DMAMode = ADC_DMAMode_OneShot;
//    ADC12_InitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_1; // MDMA Mode for 12 bit
//    ADC12_InitStructure.ADC_Mode = ADC_Mode_Independent; //still in independent mode
//    ADC12_InitStructure.ADC_TwoSamplingDelay = 4; // Delay 5 clocks ( + 2 for sample)
//    ADC_CommonInit(ADC1, &ADC12_InitStructure);
//}
//
//void ADC12_DMA1_setDualMode(void) {
//// DMA 32 bit PeripheralDataSize
//    DMA11_ADC1_Handle.Instance->CCR &= ~DMA_CCR_PSIZE;
//    DMA11_ADC1_Handle.Instance->CCR |= DMA_PeripheralDataSize_Word;
//// Use common data register of ADC1+2
//    DMA11_ADC1_Handle.Instance->CPAR = (uint32_t) &(ADC1_2->CDR);
//    ADC1_2->CCR &= ~ADC12_CCR_MULTI;
//    ADC1_2->CCR |= ADC_Mode_Interleave;
//}
//
//void ADC12_DMA1_setSingleADC1Mode(void) {
//// DMA 32 bit PeripheralDataSize
//    DMA11_ADC1_Handle.Instance->CCR &= ~DMA_CCR_PSIZE;
//    DMA11_ADC1_Handle.Instance->CCR |= DMA_PeripheralDataSize_HalfWord;
//    DMA11_ADC1_Handle.Instance->CPAR = (uint32_t) &(ADC1->DR);
//    ADC1_2->CCR &= ~ADC12_CCR_MULTI;
//    ADC1_2->CCR |= ADC_Mode_Interleave;
//}

void ADC1_DMA_start(uint32_t aMemoryBaseAddr, uint16_t aBufferSize, bool aModeCircular) {
// Disable DMA1 channel1 - is really needed here!
    DMA_HandleTypeDef * tDMA_ADCHandle = ADC1Handle.DMA_Handle;
    CLEAR_BIT(tDMA_ADCHandle->Instance->CCR, DMA_CCR_EN);

#ifdef STM32F30X
    if (aModeCircular) {
// ADC_DMAConfig(ADC1, ADC_DMAMode_Circular );
        SET_BIT(ADC1Handle.Instance->CFGR, ADC_CFGR_DMACFG);
        SET_BIT(ADC1_2_COMMON->CCR, ADC12_CCR_DMACFG); //for dual modus
        SET_BIT(tDMA_ADCHandle->Instance->CCR, DMA_CCR_CIRC);
    } else {
// ADC_DMAConfig(ADC1, ADC_DMAMode_OneShot );
        CLEAR_BIT(ADC1Handle.Instance->CFGR, ADC_CFGR_DMACFG);
        CLEAR_BIT(ADC1_2_COMMON->CCR, ADC12_CCR_DMACFG);
        CLEAR_BIT(tDMA_ADCHandle->Instance->CCR, DMA_CCR_CIRC);
    }
#else
    if (aModeCircular) {
// ADC_DMAConfig(ADC1, ADC_DMAMode_Circular );
        //SET_BIT(ADC1_2_COMMON->CCR, ADC12_CCR_DMACFG); //for dual modus
        SET_BIT(tDMA_ADCHandle->Instance->CCR, DMA_CCR_CIRC);
    } else {
// ADC_DMAConfig(ADC1, ADC_DMAMode_OneShot );
        //CLEAR_BIT(ADC1_2_COMMON->CCR, ADC12_CCR_DMACFG);
        CLEAR_BIT(tDMA_ADCHandle->Instance->CCR, DMA_CCR_CIRC);
    }
#endif

// Write to DMA Channel1 CMAR
    tDMA_ADCHandle->Instance->CMAR = aMemoryBaseAddr;
// Write to DMA1 Channel1 CNDTR
    tDMA_ADCHandle->Instance->CNDTR = aBufferSize;

//  DMA_Cmd(DSO_DMA_CHANNEL, ENABLE);
// enable channel and all interrupts too
    SET_BIT(tDMA_ADCHandle->Instance->CCR, DMA_CCR_EN | DMA_CCR_TCIE | DMA_CCR_HTIE | DMA_CCR_TEIE);

// starts conversion at next timer edge - is definitely needed to start dma transfer!
#ifdef STM32F30X
    SET_BIT(ADC1Handle.Instance->CR, ADC_CR_ADSTART);
#else
    SET_BIT(ADC1Handle.Instance->CR2, ADC_CR2_EXTTRIG);
#endif
}

/*
 * Stop DMA channel
 */
void ADC1_DMA_stop(void) {
// Disable DMA1 channel1
    CLEAR_BIT(ADC1Handle.DMA_Handle->Instance->CCR, DMA_CCR_EN);
// reset all bits
    __HAL_DMA_CLEAR_FLAG(ADC1Handle.DMA_Handle, DMA_FLAG_GL1);
//    DMA_ClearITPendingBit (DMA1_IT_GL1);
}

/*
 * read internal Temp sensor by ADC1 channel 16, compensate for VDD not 3.3V and convert it
 */
#define TEMP110_CAL_ADDR  ((uint16_t*) ((uint32_t)0x1FFFF7C2))  /* Internal temperature sensor, parameter TS_CAL1: TS ADC raw data acquired at a temperature of  30 DegC (+-5 DegC), VDDA = 3.3 V (+-10 mV) */
#define TEMP30_CAL_ADDR   ((uint16_t*) ((uint32_t)0x1FFFF7B8))  /* Internal temperature sensor, parameter TS_CAL2: TS ADC raw data acquired at a temperature of 110 DegC (+-5 DegC), VDDA = 3.3 V (+-10 mV) */

#define INTERNAL_TEMPSENSOR_V25        ((int32_t)1430)         /* Internal temperature sensor, parameter V25 (unit: mV). Refer to device datasheet for min/typ/max values. */
#define INTERNAL_TEMPSENSOR_AVGSLOPE   ((int32_t)4300)         /* Internal temperature sensor, parameter Avg_Slope (unit: uV/DegCelsius). Refer to device datasheet for min/typ/max values. */                                                               /* This calibration parameter is intended to calculate the actual VDDA from Vrefint ADC measurement. */

// Temperature = (VTS - V25)/Avg_Slope + 25
#define COMPUTATION_TEMPERATURE_STD_PARAMS(TS_ADC_DATA)                        \
  ((((int32_t)(INTERNAL_TEMPSENSOR_V25 - (((TS_ADC_DATA) * (sVDDA * 1000)) / 4095)   \
     ) * 1000000                                                                  \
    ) / INTERNAL_TEMPSENSOR_AVGSLOPE                                           \
   ) + 25000                                                                      \
  )

int ADC1_getTemperatureMilligrades(void) {
    int tADCTempShift3 = ADC1_getChannelValue(ADC_CHANNEL_TEMPSENSOR, 4);
#ifdef STM32F30X
// convert it to value at 3.3 V
    tADCTempShift3 = ((tADCTempShift3 * (*VREFINT_CAL)) << 3) / sRefintActualReading;

    int tTempValueAt30Degree = *(uint16_t*) (TEMP30_CAL_ADDR);        //0x6BE
    int tTempValueAt110Degree = *(uint16_t*) (TEMP110_CAL_ADDR);        //0x509
// 1.43V at 25C = 0x6EE  4.3mV/C => 344mV/80C
    if (sTempValueAtZeroDegreeShift3 == 0) {
// not initialized -> do it here
// tTempValueAt30Degree - (((tTempValueAt110Degree - tTempValueAt30Degree)/80)*30)
// => tTempValueAt30Degree - (((tTempValueAt110Degree - tTempValueAt30Degree)/8)*3)
        sTempValueAtZeroDegreeShift3 = (tTempValueAt30Degree << 3)
                - ((tTempValueAt110Degree - tTempValueAt30Degree) * 3);
    }
    return ((sTempValueAtZeroDegreeShift3 - tADCTempShift3) * 10000) / (tTempValueAt30Degree - tTempValueAt110Degree);

#else
    return COMPUTATION_TEMPERATURE_STD_PARAMS(tADCTempShift3);
#endif
}

float ADC_getTemperature(void) {
    int tMilligrades = ADC1_getTemperatureMilligrades();
    float tActualTemperature = (float) tMilligrades;
    return tActualTemperature / 1000;
}

/*
 * DSO Attenuator
 */
#ifndef USE_STM32F3_DISCO
int DSO_detectAttenuatorType(void) {
    int tAttenuatorType = !(DSO_ATTENUATOR_PORT->IDR & DSO_ATTENUATOR_DETECT_0_PIN);
    tAttenuatorType |= (!(DSO_ATTENUATOR_PORT->IDR & DSO_ATTENUATOR_DETECT_1_PIN)) << 1;
    return tAttenuatorType;
}
#endif

void DSO_initializeAttenuatorAndAC(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable the GPIO Clock */
    __GPIOC_CLK_ENABLE();

// Bit 0,1,2 pin, AC range pin
    GPIO_InitStructure.Pin =
    DSO_ATTENUATOR_0_PIN | DSO_ATTENUATOR_1_PIN | DSO_ATTENUATOR_2_PIN | DSO_AC_RANGE_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(DSO_ATTENUATOR_PORT, &GPIO_InitStructure);
// set LOW
    HAL_GPIO_WritePin(DSO_ATTENUATOR_PORT,
    DSO_ATTENUATOR_0_PIN | DSO_ATTENUATOR_1_PIN | DSO_ATTENUATOR_2_PIN | DSO_AC_RANGE_PIN, GPIO_PIN_RESET);

#ifndef USE_STM32F3_DISCO
    // Atttenuator detect pin
    GPIO_InitStructure.Pin = DSO_ATTENUATOR_DETECT_0_PIN | DSO_ATTENUATOR_DETECT_1_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(DSO_ATTENUATOR_PORT, &GPIO_InitStructure);
#endif
}

void DSO_setAttenuator(uint8_t aValue) {
// shift to right bit position
    uint16_t tValue = aValue << 7;

    tValue = tValue & 0x380;
    MODIFY_REG(DSO_ATTENUATOR_PORT->ODR, 0x380, tValue);
//GPIO_Write(ADC1_ATTENUATOR_PORT, tPort);
}

void DSO_setACMode(bool aValue) {
    if (aValue) {
        Set_GpioPin(DSO_ATTENUATOR_PORT, DSO_AC_RANGE_PIN);
    } else {
        Reset_GpioPin(DSO_ATTENUATOR_PORT, DSO_AC_RANGE_PIN);
    }
}

bool DSO_getACMode(void) {
    return (DSO_ATTENUATOR_PORT->ODR & DSO_AC_RANGE_PIN);
// GPIO_ReadOutputDataBit(ADC1_ATTENUATOR_PORT, ADC1_AC_RANGE_PIN);
}

unsigned int AccuCapacity_ADCRead(uint8_t aProbeNumber) {
    ADCChannelConfigDefault.SamplingTime = ADC_SAMPLETIME_SENSOR;
    if (aProbeNumber == 0) {
        ADCChannelConfigDefault.Channel = ADC_CHANNEL_6;
    } else {
        ADCChannelConfigDefault.Channel = ADC_CHANNEL_7;
    }
    HAL_ADC_ConfigChannel(&ADC2Handle, &ADCChannelConfigDefault);

#ifdef STM32F30X
    SET_BIT(ADC2Handle.Instance->CR, ADC_CR_ADSTART);
#else
    SET_BIT(ADC2Handle.Instance->CR2, ADC_CR2_ADON);
#endif    /* Test EOC flag */
    setTimeoutMillis(ADC_DEFAULT_TIMEOUT);
    while (__HAL_ADC_GET_FLAG(&ADC2Handle, ADC_FLAG_EOC) == RESET) {
#ifdef STM32F30X
        if (isTimeout(ADC2Handle.Instance->ISR)) {
#else
            if (isTimeout(ADC2Handle.Instance->SR)) {
#endif
            break;
        }
    }
    return HAL_ADC_GetValue(&ADC2Handle);;
}

uint16_t SPI1_getPrescaler(void) {
    return SPI1HandlePtr->Init.BaudRatePrescaler;
}

#define SPI_TIMEOUT   0x4
/**
 * @brief  Sends a Byte through the SPI interface and return the Byte received
 *         from the SPI bus.
 * @param  Byte : Byte send.
 * @retval The received byte value
 */
//uint8_t SPI1_sendReceiveOriginal(uint8_t byte) {
//    uint32_t tLR14 = getLR14();
//
//// discard RX FIFO content
//    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != RESET) {
//        SPI_ReceiveData8(SPI1);
//    }
//    setTimeoutMillis(SPI_TIMEOUT);
//// Wait if transmit FIFO buffer full
//    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) {
//        if (isTimeout(tLR14)) {
//            return 0;
//        }
//    }
//    /* Send a Byte through the SPI peripheral */
//    SPI_SendData8(SPI1, byte);
//
//    setTimeoutMillis(SPI_TIMEOUT);
//    /* Wait to receive a Byte */
//    setTimeoutMillis(SPI_TIMEOUT);
//    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) {
//        if (isTimeout(tLR14)) {
//            return 0;
//        }
//    }
//    /* Return the Byte read from the SPI bus */
//    return (uint8_t) SPI_ReceiveData8(SPI1);
//}
/**
 * Version of sendReceive without function calling
 */
static uint32_t inSendReceiveFast = 0; // 0 = not in function LR14 (!=0) already in function

extern "C" uint8_t SPI1_sendReceiveFast(uint8_t byte) {
    uint32_t tLR14 = getLR14();
    if (inSendReceiveFast) {
        failParamMessage(inSendReceiveFast, "inSendReceiveFast is already in use");
    }
    inSendReceiveFast = tLR14; // set semaphore
// wait for ongoing transfer to end
    while (HAL_IS_BIT_SET(SPI1HandlePtr->Instance->SR, SPI_FLAG_BSY)) {
        ;
    }
// discard RX FIFO content
    uint32_t spixbase = (uint32_t) SPI1;
    spixbase += 0x0C;

    // to suppress unused warnings
    uint8_t dummy __attribute__((unused));

    while (HAL_IS_BIT_SET(SPI1HandlePtr->Instance->SR, SPI_FLAG_RXNE)) {
        /* fetch the Byte read from the SPI bus in order to empty RX fifo */
        dummy = *(__IO uint8_t *) spixbase;
    }

    /* Send a Byte through the SPI peripheral */
    *(__IO uint8_t *) spixbase = byte;

    /* Wait to receive a Byte */
    setTimeoutMillis(SPI_TIMEOUT);
    while (HAL_IS_BIT_CLR(SPI1HandlePtr->Instance->SR, SPI_FLAG_RXNE)) {
        if (isTimeoutVerbose((uint8_t *) __FILE__, __LINE__, tLR14, 2000)) {
            break;
        }
    }

    /* Return the Byte read from the SPI bus */
    spixbase = (uint32_t) SPI1;
    spixbase += 0x0C;
    inSendReceiveFast = 0; // reset semaphore
    return *(__IO uint8_t *) spixbase;
}

bool sRTCIsInitalized = false;
void RTC_InitLSEOn(void) {
    __PWR_CLK_ENABLE(); // must be done prior to __HAL_RCC_LSE_CONFIG(RCC_LSE_ON)
#ifndef STM32F30X
    /* Enable BKP CLK enable for backup registers */
    __HAL_RCC_BKP_CLK_ENABLE()
    ;
#endif

    HAL_PWR_EnableBkUpAccess(); // enable RTC Bits of RCC
    // set LSE on - 2500ms on power up 0 millis else
    __HAL_RCC_LSE_CONFIG(RCC_LSE_ON);//LSEON is on the backup domain

    // Set instance here for use of access to backup domain.
    RTCHandle.Instance = RTC;
}

/*
 * To ensure that PWR Backup access is not disabled if LSE is not used
 */
void RTC_PWRDisableBkUpAccess() {
    if (__HAL_RCC_GET_RTC_SOURCE() == RCC_RTCCLKSOURCE_LSE) {
        HAL_PWR_DisableBkUpAccess();
    }
}

/*
 * Assume that RTC_LSEOn and no HAL_PWR_DisableBkUpAccess() is called before
 */
void RTC_initialize(void) {
    /*
     * Check if LSE crystal is attached
     */
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) != RESET) {
        /*
         * LSE is active -> use it. This can only be done once after power on.
         * The manual says: This selection cannot be modified without resetting the Backup domain.
         */
        __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);
#ifdef STM32F30X
        RTCHandle.Init.AsynchPrediv = 128 - 1; // %128 is the default value for 32kHz - gives 256 Hz clock;
        RTCHandle.Init.SynchPrediv = 256 - 1;
#endif
    } else {
        __HAL_RCC_LSE_CONFIG(RCC_LSE_OFF);
        /*
         * Use HSE/32. It is still running :-) datasheet says, do not disable Backup domain
         */
#ifdef STM32F30X
        __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_HSE_DIV32); // -> 250 usec clock

        RTCHandle.Init.AsynchPrediv = 125 - 1; // max sensible value 250 us / 125 -> 2kHz / 0.5 ms clock
        RTCHandle.Init.SynchPrediv = 2000 - 1;
#else
        // if used once we cannot switch back while power is on
        // The manual says: This selection cannot be modified without resetting the Backup domain.
        __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_HSE_DIV128);
#endif
    }
    __HAL_RCC_RTC_ENABLE();

    /* RTC prescaler configuration */
#ifdef STM32F30X
    RTCHandle.Init.HourFormat = RTC_HOURFORMAT_24;
    RTCHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    RTCHandle.Init.OutPutType = RTC_OUTPUT_TYPE_PUSHPULL;
    RTCHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
#else
    RTCHandle.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
    RTCHandle.Init.OutPut = RTC_OUTPUTSOURCE_SECOND; // Pin C13
#endif
    HAL_RTC_Init(&RTCHandle);

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET) {
        RTC_setTime(0, 0, 12, 6, 12, 9, 2015);
    }
    sRTCIsInitalized = true;
}

void fillTimeStructure(struct tm *aTimeStructurePtr) {
    RTC_DateTypeDef RTC_DateStructure;
    RTC_TimeTypeDef RTC_TimeStructure;

    /* Get the RTC current Time */
    HAL_RTC_GetTime(&RTCHandle, &RTC_TimeStructure, FORMAT_BIN);
    /* Get the RTC current Date */
    HAL_RTC_GetDate(&RTCHandle, &RTC_DateStructure, FORMAT_BIN);
    aTimeStructurePtr->tm_year = RTC_DateStructure.Year + 100;
    aTimeStructurePtr->tm_mon = RTC_DateStructure.Month - 1;
    aTimeStructurePtr->tm_mday = RTC_DateStructure.Date;
    aTimeStructurePtr->tm_hour = RTC_TimeStructure.Hours;
    aTimeStructurePtr->tm_min = RTC_TimeStructure.Minutes;
    aTimeStructurePtr->tm_sec = RTC_TimeStructure.Seconds;
}

uint8_t RTC_getSecond(void) {
    if (!sRTCIsInitalized) {
        return 0;
    }

#ifdef STM32F30X
    uint8_t tSecond = (uint8_t) (RTC->TR & (RTC_TR_ST | RTC_TR_SU));
    return tSecond;
#else
    RTC_TimeTypeDef RTC_TimeStructure;
    HAL_RTC_GetTime(&RTCHandle, &RTC_TimeStructure, FORMAT_BIN);
    return RTC_TimeStructure.Seconds;
#endif
}

#ifdef LOCAL_DISPLAY_EXISTS
#define CALIBRATION_MAGIC_NUMBER 0x5A5A5A5A
// value is set only if values are manually set by do calibration and not if default values are set
void RTC_setMagicNumber(void) {
    HAL_PWR_EnableBkUpAccess();
// Write magic number
    HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR1, CALIBRATION_MAGIC_NUMBER);
    RTC_PWRDisableBkUpAccess();
}

/**
 *
 * @return true if magic number valid
 */bool RTC_checkMagicNumber(void) {
    HAL_PWR_EnableBkUpAccess();
// Read magic number
    uint32_t tMagic = HAL_RTCEx_BKUPRead(&RTCHandle, RTC_BKP_DR1);
    RTC_PWRDisableBkUpAccess();
    if (tMagic == CALIBRATION_MAGIC_NUMBER) {
        return true;
    }
    return false;
}
#endif

/**
 * not used yet
 */
long RTC_getTimeAsLong(void) {
    struct tm tTm;
    fillTimeStructure(&tTm);
    return mktime(&tTm);
}

/**
 * not used yet
 * @param aStringBuffer
 * @return
 */
int RTC_getTimeStringForFile(char * aStringBuffer) {
    if (!sRTCIsInitalized) {
        //return empty string
        aStringBuffer[0] = '\0';
        return 0;
    }

    RTC_TimeTypeDef RTC_TimeStructure;

    /* Get the RTC current Time */
    HAL_RTC_GetTime(&RTCHandle, &RTC_TimeStructure, FORMAT_BIN);
    /* Display  Format : hh-mm-ss */
// do not have a buffer size :-(
    return sprintf(aStringBuffer, "%02i-%02i-%02i", RTC_TimeStructure.Hours, RTC_TimeStructure.Minutes,
            RTC_TimeStructure.Seconds);
}

/**
 * gets time/date string or seconds since boot if RTC not available
 * Format : yyyy-mm-dd hh-mm-ss
 * @param aStringBuffer
 * @return value of sprintf()
 */
int RTC_getDateStringForFile(char * aStringBuffer) {
    if (!sRTCIsInitalized) {
        //return empty string
        aStringBuffer[0] = '\0';
        return 0;
    }

    RTC_DateTypeDef RTC_DateStructure;
    RTC_TimeTypeDef RTC_TimeStructure;

    /* Get the RTC current Time */
    HAL_RTC_GetTime(&RTCHandle, &RTC_TimeStructure, FORMAT_BIN);
    /* Get the RTC current Date */
    HAL_RTC_GetDate(&RTCHandle, &RTC_DateStructure, FORMAT_BIN);
    if (RTC_DateStructure.Year != 0) {
        RTC_DateIsValid = true;
    } else {
// fallback - set time to seconds since boot
        uint32_t tTime = getMillisSinceBoot() / 1000;
        RTC_TimeStructure.Seconds = tTime % 60;
        tTime = tTime / 60;
        RTC_TimeStructure.Minutes = tTime % 60;
        tTime = tTime / 60;
        RTC_TimeStructure.Hours = tTime % 24;
        RTC_DateStructure.Date = (tTime / 24) + 1;
    }
    /* Display  Format : yyyy-mm-dd hh-mm-ss */
// do not have a buffer size :-(
    return sprintf(aStringBuffer, "%04i-%02i-%02i %02i-%02i-%02i", 2000 + RTC_DateStructure.Year,
            RTC_DateStructure.Month, RTC_DateStructure.Date, RTC_TimeStructure.Hours, RTC_TimeStructure.Minutes,
            RTC_TimeStructure.Seconds);
}

bool RTC_DateIsValid = false; // true if year != 0

/**
 * Get time/date string for display (European format)
 * Format : dd.mm.yyyy hh:mm:ss
 * sets RTC_DateIsValid to false if RTC not available
 * @param aStringBuffer
 * @return value of sprintf()
 */
int RTC_getTimeString(char * aStringBuffer) {
    if (!sRTCIsInitalized) {
        //return empty string
        aStringBuffer[0] = '\0';
        return 0;
    }

    RTC_DateTypeDef RTC_DateStructure;
    RTC_TimeTypeDef RTC_TimeStructure;

    /* Get the RTC current Time */
    HAL_RTC_GetTime(&RTCHandle, &RTC_TimeStructure, FORMAT_BIN);
    /* Get the RTC current Date */
    HAL_RTC_GetDate(&RTCHandle, &RTC_DateStructure, FORMAT_BIN);
    if (RTC_DateStructure.Year != 0) {
        RTC_DateIsValid = true;
    }
    /* Display  Format : dd.mm.yyyy hh:mm:ss */
// do not have a buffer size :-(
    return sprintf(aStringBuffer, "%02i.%02i.%04i %02i:%02i:%02i", RTC_DateStructure.Date, RTC_DateStructure.Month,
            2000 + RTC_DateStructure.Year, RTC_TimeStructure.Hours, RTC_TimeStructure.Minutes,
            RTC_TimeStructure.Seconds);
}

void RTC_setTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t dayOfWeek, uint8_t day, uint8_t month,
        uint16_t year) {
    RTC_DateTypeDef RTC_DateStructure;
    RTC_TimeTypeDef RTC_TimeStructure;

    RTC_DateStructure.Year = year - 2000;
    RTC_DateStructure.Month = month;
    RTC_DateStructure.Date = day;
    RTC_DateStructure.WeekDay = dayOfWeek;
    HAL_RTC_SetDate(&RTCHandle, &RTC_DateStructure, FORMAT_BIN);

    RTC_TimeStructure.Hours = hour;
    RTC_TimeStructure.Minutes = min;
    RTC_TimeStructure.Seconds = sec;
#ifdef STM32F30X
    RTC_TimeStructure.TimeFormat = RTC_HOURFORMAT12_AM;
    RTC_TimeStructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    RTC_TimeStructure.StoreOperation = RTC_STOREOPERATION_RESET;
#endif
    HAL_RTC_SetTime(&RTCHandle, &RTC_TimeStructure, FORMAT_BIN);
}

#ifdef HAL_WWDG_MODULE_ENABLED
/*
 * WWDG Window Watchdog
 */
static int WWDG_ExtendedCounter;
#define WWDG_EXTENDED_COUNTER_RELOAD_VALUE 100 // -> 6 sec
void Watchdog_init(void) {
    __WWDG_CLK_ENABLE();
// Disable WWDG on debug
    __HAL_FREEZE_WWDG_DBGMCU();
// set high priority
    NVIC_SetPriority((IRQn_Type) (WWDG_IRQn), 4);
    HAL_NVIC_EnableIRQ((IRQn_Type) (WWDG_IRQn));

    WWDGHandle.Instance = WWDG;
    WWDGHandle.Init.Prescaler = WWDG_PRESCALER_8; // highest prescaler -> lowest clock
    WWDGHandle.Init.Window = 0x7F; // no window
    WWDGHandle.Init.Counter = 0x7F; // appr. 57ms
    HAL_WWDG_Init(&WWDGHandle);
    WWDG_ExtendedCounter = WWDG_EXTENDED_COUNTER_RELOAD_VALUE;
}

void Watchdog_start(void) {
// Start it with interrupt enabled
    HAL_WWDG_Start_IT(&WWDGHandle);
// Because of BUG in HAL Driver (missing unlock)
    __HAL_UNLOCK(&WWDGHandle);
}

// must be reloaded after appr. 57 ms
extern "C" void Watchdog_reload(void) {
//HAL_WWDG_Refresh(&WWDGHandle, 0x7F);
    MODIFY_REG(WWDGHandle.Instance->CR, WWDG_CR_T, 0x7F);
    WWDG_ExtendedCounter = WWDG_EXTENDED_COUNTER_RELOAD_VALUE;
}

extern "C" void WWDG_IRQHandler(void) {
//HAL_WWDG_Refresh(&WWDGHandle, 0x7F);
    MODIFY_REG(WWDGHandle.Instance->CR, WWDG_CR_T, 0x7F);
    Toggle_TimeoutLEDPin();
    /* Clear the WWDG Data Ready flag */
    __HAL_WWDG_CLEAR_FLAG(&WWDGHandle, WWDG_FLAG_EWIF);
    if (WWDG_ExtendedCounter-- < 0) {
        WWDG_ExtendedCounter = WWDG_EXTENDED_COUNTER_RELOAD_VALUE * 10;
        BlueDisplay1.setPrintfSizeAndColorAndFlag(TEXT_SIZE_11, COLOR_RED, COLOR_WHITE, true);
        printStackpointerAndStacktrace(9, 8);
    }
}

void reset(void) {
// generate immediate reset by writing 3F to WWDG
// can't use HAL_WWDG_Refresh() since it does not accept 0x3F
    MODIFY_REG(WWDGHandle.Instance->CR, WWDG_CR_T, 0x3F);
}
#endif

/*
 * Timer
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *aTIMHandle) {
    GPIO_InitTypeDef GPIO_InitStructure;

    if (aTIMHandle == &TIMToneHandle) {
        TONE_TIMER_IO_CLK_ENABLE();
        GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStructure.Pull = GPIO_NOPULL; //GPIO_PuPd_DOWN;
        GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
        GPIO_InitStructure.Pin = TONE_TIMER_PIN;
#ifdef STM32F30X
        GPIO_InitStructure.Alternate = GPIO_AF2_TIM3;
#endif
        HAL_GPIO_Init(TONE_TIMER_PORT, &GPIO_InitStructure);

    } else if (aTIMHandle == &TIMSynthHandle) {
        SYNTH_TIMER_IO_CLOCK_ENABLE();
// Synthesizer output pin
        GPIO_InitStructure.Pin = SYNTH_TIMER_PIN;
        GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStructure.Pull = GPIO_NOPULL;
        GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
#ifdef STM32F30X
        GPIO_InitStructure.Alternate = GPIO_AF10_TIM2;
#endif
        HAL_GPIO_Init(SYNTH_TIMER_PORT, &GPIO_InitStructure);
#ifndef STM32F30X
        __HAL_AFIO_REMAP_TIM2_PARTIAL_1();
#endif
    }
}

extern "C" void TIM_OC1_SetConfig(TIM_TypeDef *TIMx, TIM_OC_InitTypeDef *OC_Config);

/**
 * PWM for tone generation for Pin C6 with timer 3
 */
#define TONE_TIMER_TICK_FREQ 1000000
void initalizeTone(void) {
    TIMToneHandle.Instance = TONE_TIMER;

    /* clock enable */
    TONE_TIMER_CLK_ENABLE();
    uint16_t PrescalerValue = (uint16_t) ((SystemCoreClock / 2) / (TONE_TIMER_TICK_FREQ)) - 1; // 1 MHZ Timer Frequency

    /* Time base configuration */
    TIMToneHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    TIMToneHandle.Init.Period = 1000; // 1 kHz initial
    TIMToneHandle.Init.Prescaler = PrescalerValue;
    TIMToneHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    HAL_TIM_Base_Init(&TIMToneHandle);

// Channel 1
    TIM_OC_InitTypeDef TIM_OCInitStructure;
    /* PWM1 Mode configuration */
    TIM_OCInitStructure.OCMode = TIM_OCMODE_PWM1;
    TIM_OCInitStructure.OCFastMode = TIM_OCFAST_DISABLE;
    TIM_OCInitStructure.Pulse = 500; // start with 50 % duty cycle
    TIM_OCInitStructure.OCPolarity = TIM_OCPOLARITY_HIGH;
    TIM_OCInitStructure.OCIdleState = TIM_OCIDLESTATE_RESET;
    TIM_OCInitStructure.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    TIM_OCInitStructure.OCNIdleState = TIM_OCNIDLESTATE_RESET;
//    HAL_TIM_PWM_ConfigChannel(&TIMToneHandle, &TIM_OCInitStructure, TIM_CHANNEL_1);

// substitution here by 3 lines and one line at Synth_Timer_initialize saves 1464 bytes code
    /* Configure the Channel 1 in PWM mode */
    TIM_OC1_SetConfig(TIMToneHandle.Instance, &TIM_OCInitStructure);
    /* Set the Preload enable bit for channel1 */
    SET_BIT(TIMToneHandle.Instance->CCMR1, TIM_CCMR1_OC1PE);
    /* Configure the Output Fast mode */
    CLEAR_BIT(TIMToneHandle.Instance->CCMR1, TIM_CCMR1_OC1FE);

    __HAL_TIM_ENABLE(&TIMToneHandle);
}

int32_t ToneDuration = 0; //! signed because >0 means wait, == 0 means action and < 0 means do nothing

void tone(uint16_t aFreqHertz, uint16_t aDurationMillis) {
// compute reload value
    uint32_t tPeriod = TONE_TIMER_TICK_FREQ / aFreqHertz;
    if (tPeriod >= 0x10000) {
        tPeriod = 0x10000 - 1;
    }
    __HAL_TIM_SetAutoreload(&TIMToneHandle, tPeriod - 1)
    ;
    __HAL_TIM_SetCompare(&TIMToneHandle, TIM_CHANNEL_1, tPeriod / 2); // 50 % duty cycle
    HAL_TIM_OC_Start(&TIMToneHandle, TIM_CHANNEL_1);

    ToneDuration = aDurationMillis;
    changeDelayCallback(&noTone, aDurationMillis);
}

void noTone(void) {
// disable timer PWM output
    HAL_TIM_OC_Stop(&TIMToneHandle, TIM_CHANNEL_1);
}

void FeedbackToneOK(void) {
    FeedbackTone(FEEDBACK_TONE_NO_ERROR);
}

void FeedbackTone(unsigned int aFeedbackType) {
    if (aFeedbackType == FEEDBACK_TONE_NO_TONE) {
        return;
    }
    if (aFeedbackType == FEEDBACK_TONE_NO_ERROR) {
        tone(3000, 50);
    } else if (aFeedbackType == FEEDBACK_TONE_SHORT_ERROR) {
// two short beeps
        tone(4000, 30);
        delayMillis(60);
        tone(2000, 30);
    } else {
// long tone
        tone(4500, 500);
    }
}

void EndTone(void) {
    tone(880, 1000);
    delayMillis(1000);
    tone(660, 1000);
    delayMillis(1000);
    tone(440, 1000);
}

#define MICROSD_CS_PIN                         GPIO_PIN_5
#define MICROSD_CS_PORT                   GPIOC

#define MICROSD_CARD_DETECT_PIN                GPIO_PIN_4
#define MICROSD_CARD_DETECT_PORT          GPIOC
#define MICROSD_CARD_DETECT_EXTI_IRQn          EXTI4_IRQn

/**
 * Init the MicroSD card pins CS + CardDetect
 */
void MICROSD_IO_initalize(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

// Enable the GPIO Clock
    __GPIOC_CLK_ENABLE();

// CS pin
    GPIO_InitStructure.Pin = MICROSD_CS_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(MICROSD_CS_PORT, &GPIO_InitStructure);
// set High
    HAL_GPIO_WritePin(MICROSD_CS_PORT, MICROSD_CS_PIN, GPIO_PIN_SET);

// CardDetect pin
    GPIO_InitStructure.Pin = MICROSD_CARD_DETECT_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(MICROSD_CARD_DETECT_PORT, &GPIO_InitStructure);

    /* Enable and set MicoSD EXTI Interrupt to the lowest priority */
    NVIC_SetPriority((IRQn_Type) (MICROSD_CARD_DETECT_EXTI_IRQn), 15);
    HAL_NVIC_EnableIRQ((IRQn_Type) (MICROSD_CARD_DETECT_EXTI_IRQn));
}

#ifdef HAL_DAC_MODULE_ENABLED

#define DAC_OUTPUT_PIN                       GPIO_PIN_4
#define DAC_OUTPUT_PORT                 GPIOA

/**
 * @brief  DAC channels configurations PA4 in analog,
 */
void HAL_DAC_MspInit(DAC_HandleTypeDef* aDACHandle) {
    GPIO_InitTypeDef GPIO_InitStructure;
    /* GPIOA clock enable */
    __GPIOA_CLK_ENABLE();

    /* Configure PA.04 (DAC_OUT1) as analog in to avoid parasitic consumption */
    GPIO_InitStructure.Pin = DAC_OUTPUT_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DAC_OUTPUT_PORT, &GPIO_InitStructure);

    TIM7Handle.Instance = TIM7;

    /* TIM7 Peripheral clock enable */
    __TIM7_CLK_ENABLE();

    /* Time base configuration */
    TIM7Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    TIM7Handle.Init.Prescaler = 0x0; // no prescaler
    TIM7Handle.Init.CounterMode = TIM_COUNTERMODE_DOWN; // to easily change reload value
    TIM7Handle.Init.Period = 1000; //1000=>10 Hz
    HAL_TIM_Base_Init(&TIM7Handle);

    TIM_MasterConfigTypeDef tMasterConfig;
    tMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE; // trigger on each update
    tMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&TIM7Handle, &tMasterConfig);

    /* TIM7 enable counter */
    HAL_TIM_Base_Start(&TIM7Handle);
}

void DAC_init(void) {
    DAC1Handle.Instance = DAC;
    /* DAC Periph clock enable */
    __DAC1_CLK_ENABLE();

    HAL_DAC_Init(&DAC1Handle);

    static DAC_ChannelConfTypeDef tDACChannelConfig;
    /* Configuration of DAC channel 1 */
    tDACChannelConfig.DAC_Trigger = DAC_TRIGGER_T7_TRGO;
    tDACChannelConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&DAC1Handle, &tDACChannelConfig, DAC1_CHANNEL_1);

// set triangle Mode
    HAL_DACEx_TriangleWaveGenerate(&DAC1Handle, DAC_CHANNEL_1, DAC_TRIANGLEAMPLITUDE_4095);
    HAL_DAC_Start(&DAC1Handle, DAC1_CHANNEL_1);
    HAL_DAC_SetValue(&DAC1Handle, DAC1_CHANNEL_1, DAC_ALIGN_12B_R, 0x0);
}

/**
 *
 * @param aReloadValue value less than 4 makes no sense since reload operation takes 3 ticks
 */
void DAC_Timer_SetReloadValue(uint32_t aReloadValue) {
    if (aReloadValue < DAC_TIMER_MIN_RELOAD_VALUE) {
        failParamMessage(aReloadValue, "DAC reload value < 4");
    }
    uint32_t tReloadValue = aReloadValue;
    uint16_t tPrescalerValue = 1;
// convert 32 bit to 16 bit prescaler and 16 bit counter
    if (tReloadValue > 0xFFFF) {
//Count Leading Zeros
        int tShiftCount = 16 - __CLZ(aReloadValue);
        tPrescalerValue <<= tShiftCount;
        tReloadValue = aReloadValue >> tShiftCount;
    }
// BUG in Library (space) __HAL_TIM_PRESCALER(&TIM7Handle, tPrescalerValue - 1);
    TIM7Handle.Instance->PSC = tPrescalerValue - 1;
    __HAL_TIM_SetAutoreload(&TIM7Handle, tReloadValue - 1);
}

/**
 * Enable DAC Channel1: Once the DAC channel1 is enabled, Pin A.05 is automatically connected to the DAC converter.
 */
void DAC_Start(void) {
    __HAL_DAC_ENABLE(&DAC1Handle, DAC_CHANNEL_1);
    __HAL_TIM_ENABLE(&TIM7Handle);
}

void DAC_Stop(void) {
    __HAL_TIM_DISABLE(&TIM7Handle);
    __HAL_DAC_DISABLE(&DAC1Handle, DAC_CHANNEL_1);

}

/* Noise Wave generator */
void DAC_ModeNoise(void) {
    HAL_DACEx_NoiseWaveGenerate(&DAC1Handle, DAC_CHANNEL_1, DAC_LFSRUNMASK_BITS11_0);
}

/* Triangle Wave generator */
void DAC_ModeTriangle(void) {
    HAL_DACEx_TriangleWaveGenerate(&DAC1Handle, DAC_CHANNEL_1, DAC_TRIANGLEAMPLITUDE_2047);
}

// amplitude from 0 to 0x0B
void DAC_TriangleAmplitude(unsigned int aAmplitude) {
    aAmplitude &= 0x0F;
    if (aAmplitude > 0x0B) {
        aAmplitude = 0x0B;
    }
    MODIFY_REG(DAC->CR, 0xF00, aAmplitude << 8);
}
#endif

#ifdef STM32F30X

/*
 * Timer 15 for IR handling
 */
void IR_Timer_initialize(uint16_t aAutoreload) {
    TIM15Handle.Instance = TIM15;
    __TIM15_CLK_ENABLE();

    TIM15Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    TIM15Handle.Init.Prescaler = 0x0; // no prescaler
    TIM15Handle.Init.CounterMode = TIM_COUNTERMODE_UP; // to easily change reload value
    TIM15Handle.Init.Period = aAutoreload;
    HAL_TIM_Base_Init(&TIM15Handle);

// high priority - in order to interrupt slider handling of systic interrupt
    NVIC_SetPriority((IRQn_Type) (TIM1_BRK_TIM15_IRQn), 7);
    HAL_NVIC_EnableIRQ((IRQn_Type) (TIM1_BRK_TIM15_IRQn));
    __HAL_TIM_ENABLE_IT(&TIM15Handle, TIM_IT_UPDATE);
}

void IR_Timer_Start(void) {
    __HAL_TIM_ENABLE(&TIM15Handle);
}

void IR_Timer_Stop(void) {
    __HAL_TIM_DISABLE(&TIM15Handle);
}
#endif

/* TIM2 configuration for frequency synthesizer */
void Synth_Timer_initialize(uint32_t aAutoreload) {

    TIMSynthHandle.Instance = SYNTH_TIMER;

    SYNTH_TIMER_CLOCK_ENABLE();

    /* Time base configuration */
    TIMSynthHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    TIMSynthHandle.Init.Prescaler = 0x0; // no prescaler
    TIMSynthHandle.Init.Period = aAutoreload;
    TIMSynthHandle.Init.CounterMode = TIM_COUNTERMODE_DOWN; // to easily change reload value
    HAL_TIM_Base_Init(&TIMSynthHandle);

// Channel
    TIM_OC_InitTypeDef TIM_OCInitStructure;
    TIM_OCInitStructure.OCMode = TIM_OCMODE_TOGGLE; // Toggle Mode configuration
    TIM_OCInitStructure.Pulse = 0;
    TIM_OCInitStructure.OCPolarity = TIM_OCPOLARITY_HIGH;
    TIM_OCInitStructure.OCIdleState = TIM_OCIDLESTATE_RESET;
    TIM_OCInitStructure.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    TIM_OCInitStructure.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    TIM_OCInitStructure.OCFastMode = TIM_OCFAST_DISABLE;
//    HAL_TIM_OC_ConfigChannel(&TIMSynthHandle, &TIM_OCInitStructure, SYNTH_TIMER_CHANNEL);
#ifdef STM32F30X
    TIM_OC3_SetConfig(TIMSynthHandle.Instance, &TIM_OCInitStructure);
#else
    /* Configure the TIM Channel 1 in Output Compare */
    TIM_OC1_SetConfig(TIMSynthHandle.Instance, &TIM_OCInitStructure); // saves 1464 bytes
#endif
// TODO direct counter update not wait til next change esp at 1/10 Hz
}

/**
 *
 * @param reload value
 */
void Synth_Timer32_SetReloadValue(uint32_t aReloadValue) {
// this causes the compiler to crash here
//assertParamMessage((aReloadValue > 1), aReloadValue, "HIRES reload value < 2");
    if (aReloadValue < 2) {
        failParamMessage(aReloadValue, "Synthesizer reload value < 2");
    }
    __HAL_TIM_SetAutoreload(&TIMSynthHandle, aReloadValue - 1)
    ;
}

/**
 *
 * @param reload value
 */
void Synth_Timer16_SetReloadValue(uint32_t aReloadValue, uint32_t aPrescalerValue) {
// this causes the compiler to crash here
//assertParamMessage((aReloadValue > 1), aReloadValue, "HIRES reload value < 2");
    if (aReloadValue < 2) {
        failParamMessage(aReloadValue, "Synthesizer reload value < 2");
    }

    __HAL_TIM_SetAutoreload(&TIMSynthHandle, aReloadValue - 1)
    ;
// The Macro
    TIMSynthHandle.Instance->PSC = aPrescalerValue - 1;
}

void Synth_Timer_Start(void) {
//HAL_TIMEx_OCN_Start(&TIM2Handle, TIM_CHANNEL_3);
    HAL_TIM_OC_Start(&TIMSynthHandle, SYNTH_TIMER_CHANNEL);
}

/**
 *
 * @return actual counter before stop
 */
uint32_t Synth_Timer_Stop(void) {
    uint32_t tCount = __HAL_TIM_GetCounter(&TIMSynthHandle);
    HAL_TIM_OC_Stop(&TIMSynthHandle, SYNTH_TIMER_CHANNEL);
    return tCount;
}

/**
 * Init the Debug pin E7 / B0
 */
void Misc_IO_initalize(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable the GPIO Clocks */
    DEBUG_CLOCK_ENABLE();

// Debug pin
    GPIO_InitStructure.Pin = DEBUG_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(DEBUG_PORT, &GPIO_InitStructure);
// set Low
    HAL_GPIO_WritePin(DEBUG_PORT, DEBUG_PIN, GPIO_PIN_RESET);
#ifndef USE_STM32F3_DISCO
    TIMEOUT_LED_CLOCK_ENABLE()
    ;
    GPIO_InitStructure.Pin = TIMEOUT_LED_PIN;
    GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(TIMEOUT_LED_PORT, &GPIO_InitStructure);
    HAL_GPIO_WritePin(TIMEOUT_LED_PORT, TIMEOUT_LED_PIN, GPIO_PIN_RESET);

#endif
}

#ifdef STM32F30X

void AccuCapacity_IO_initalize(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable the GPIO Clock */
    ACCUCAP_CLOCK_ENABLE();

// Discharge 1+2 pin, Charge 1+2 pin
    GPIO_InitStructure.Pin = ACCUCAP_DISCHARGE_1_PIN | ACCUCAP_DISCHARGE_2_PIN | ACCUCAP_CHARGE_1_PIN
            | ACCUCAP_CHARGE_2_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(ACCUCAP_PORT, &GPIO_InitStructure);
// set LOW
    HAL_GPIO_WritePin(ACCUCAP_PORT,
    ACCUCAP_DISCHARGE_1_PIN | ACCUCAP_DISCHARGE_2_PIN | ACCUCAP_CHARGE_1_PIN | ACCUCAP_CHARGE_2_PIN, GPIO_PIN_RESET);
}

void AccuCapacity_SwitchDischarge(unsigned int aChannelIndex, bool aActive) {
    uint16_t tPinNr = ACCUCAP_DISCHARGE_2_PIN;
    if (aChannelIndex != 0) {
        tPinNr = ACCUCAP_DISCHARGE_1_PIN;
    }
    if (aActive) {
        Set_GpioPin(ACCUCAP_PORT, tPinNr);
    } else {
// Low
        Reset_GpioPin(ACCUCAP_PORT, tPinNr);
    }
}

void AccuCapacity_SwitchCharge(unsigned int aChannelIndex, bool aActive) {
    uint16_t tPinNr = ACCUCAP_CHARGE_2_PIN;
    if (aChannelIndex != 0) {
        tPinNr = ACCUCAP_CHARGE_1_PIN;
    }
    if (aActive) {
        Set_GpioPin(ACCUCAP_PORT, tPinNr);
    } else {
// Low
        Reset_GpioPin(ACCUCAP_PORT, tPinNr);
    }
}
#endif

/***************************************************************
 * EXTERN "C" functions
 ***************************************************************/
extern "C" void SPI1_setPrescaler(uint16_t aPrescaler) {
// SPI_BaudRatePrescaler_256 is also prescaler mask
    assert_param(aPrescaler == (aPrescaler & SPI_BAUDRATEPRESCALER_256) || aPrescaler == 0);
    /* Clear BR[2:0] bits (Baud Rate Control)*/
    /* Configure SPIx: Data Size */
    MODIFY_REG(SPI1->CR1, SPI_CR1_BR, aPrescaler);
    SPI1HandlePtr->Init.BaudRatePrescaler = aPrescaler;
}

/**
 * year since 1980
 * @return time since 1980 in 10 seconds resolution
 */
extern "C" uint32_t get_fattime(void) {
    RTC_DateTypeDef RTC_DateStructure;
    RTC_TimeTypeDef RTC_TimeStructure;

    /* Get the RTC current Time */
    HAL_RTC_GetTime(&RTCHandle, &RTC_TimeStructure, FORMAT_BIN);
    /* Get the RTC current Date */
    HAL_RTC_GetDate(&RTCHandle, &RTC_DateStructure, FORMAT_BIN);
// year since 1980
    uint32_t res = (((uint32_t) RTC_DateStructure.Year + 20) << 25) | ((uint32_t) RTC_DateStructure.Month << 21)

    | ((uint32_t) RTC_DateStructure.Date << 16) | (uint16_t) (RTC_TimeStructure.Hours << 11)
            | (uint16_t) (RTC_TimeStructure.Minutes << 5) | (uint16_t) (RTC_TimeStructure.Seconds >> 1);
    return res;
}

extern "C" bool MICROSD_isCardInserted(void) {
    return HAL_GPIO_ReadPin(MICROSD_CARD_DETECT_PORT, MICROSD_CARD_DETECT_PIN);
}

extern "C" void MICROSD_CSEnable(void) {
// set to LOW
    Reset_GpioPin(MICROSD_CARD_DETECT_PORT, MICROSD_CS_PIN);
}

extern "C" void MICROSD_CSDisable(void) {
// set to HIGH
    Set_GpioPin(MICROSD_CARD_DETECT_PORT, MICROSD_CS_PIN);
}

extern "C" void MICROSD_ClearITPendingBit(void) {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_4);
}

/**
 * ISR for ADC timer 6 and for DAC1&2 underrun error interrupts
 * not used yet
 */
extern "C" void TIM6_DAC_IRQHandler(void) {
    __HAL_TIM_CLEAR_FLAG(&TIM_DSOHandle, TIM_IT_UPDATE);

}

#ifdef USE_STM32F3_DISCO
#include "stm32f3_discovery.h"
extern "C" void Default_IRQHandler(void) {
    BSP_LED_Toggle(LED_GREEN); // GREEN WEST
}
#else
extern "C" void Default_IRQHandler(void) {
}
#endif

/** @} */
