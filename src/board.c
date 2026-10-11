#include "board.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "app_tasks.h"

UART_HandleTypeDef huart2;
I2C_HandleTypeDef  hi2c1;
ADC_HandleTypeDef  hadc1;

#define IRQ_PRIO_APP 6U   /* configMAX_SYSCALL_INTERRUPT_PRIORITY (5) değerinden büyük: FromISR çağırabilir */

void Board_Init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* LED'ler: PD12..PD15 */
    g.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &g);
    HAL_GPIO_WritePin(GPIOD, g.Pin, GPIO_PIN_RESET);

    /* USER butonu: PA0, kartta harici pull-down var, yükselen kenarda kesme */
    g.Pin = GPIO_PIN_0;
    g.Mode = GPIO_MODE_IT_RISING;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
    HAL_NVIC_SetPriority(EXTI0_IRQn, IRQ_PRIO_APP, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    /* USART2: 115200 8N1 */
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }

    /* ADC1: iç sıcaklık sensörü (kanal 16), tek dönüşüm, yazılım tetikli */
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler();
    }
    {
        ADC_ChannelConfTypeDef ch = {0};
        ch.Channel = ADC_CHANNEL_TEMPSENSOR;
        ch.Rank = 1;
        ch.SamplingTime = ADC_SAMPLETIME_480CYCLES;   /* sıcaklık sensörü en az 10 us ister */
        if (HAL_ADC_ConfigChannel(&hadc1, &ch) != HAL_OK) {
            Error_Handler();
        }
    }

    /* I2C1: 100 kHz. Kartın ses entegresiyle aynı hat, adresler çakışmaz. */
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}

/* ---- HAL Msp: saat ve pin ayarları ---- */
void HAL_UART_MspInit(UART_HandleTypeDef *h)
{
    if (h->Instance == USART2) {
        GPIO_InitTypeDef g = {0};
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        g.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        g.Mode = GPIO_MODE_AF_PP;
        g.Pull = GPIO_PULLUP;
        g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        g.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &g);
        HAL_NVIC_SetPriority(USART2_IRQn, IRQ_PRIO_APP, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    }
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *h)
{
    if (h->Instance == ADC1) {
        __HAL_RCC_ADC1_CLK_ENABLE();
    }
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *h)
{
    if (h->Instance == I2C1) {
        GPIO_InitTypeDef g = {0};
        __HAL_RCC_GPIOB_CLK_ENABLE();
        g.Pin = GPIO_PIN_6 | GPIO_PIN_9;
        g.Mode = GPIO_MODE_AF_OD;
        g.Pull = GPIO_PULLUP;
        g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        g.Alternate = GPIO_AF4_I2C1;
        HAL_GPIO_Init(GPIOB, &g);
        __HAL_RCC_I2C1_CLK_ENABLE();
    }
}

/* ---- LED, buton, ADC ---- */
void Board_Led(board_led_t led, bool on)
{
    HAL_GPIO_WritePin(GPIOD, (uint16_t)led, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Board_LedToggle(board_led_t led)
{
    HAL_GPIO_TogglePin(GPIOD, (uint16_t)led);
}

bool Board_ButtonPressed(void)
{
    return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET;
}

uint32_t Board_AdcReadTempRaw(void)
{
    uint32_t v = 0;

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        v = HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);
    return v;
}

/* ---- Buton kesmesi: semafor ver, işi ButtonTask yapsın ---- */
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (pin == GPIO_PIN_0 && g_semButton != NULL) {
        BaseType_t woken = pdFALSE;
        xSemaphoreGiveFromISR(g_semButton, &woken);
        portYIELD_FROM_ISR(woken);
    }
}
