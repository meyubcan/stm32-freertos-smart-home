/* M. Eyub Can, TECH-OHM (https://tech-ohm.com) */
/* The HAL 1 ms time base comes from TIM6. SysTick stays free as the FreeRTOS tick source. */
#include "stm32f4xx_hal.h"

TIM_HandleTypeDef htim6;

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    RCC_ClkInitTypeDef clk;
    uint32_t flash_latency, timclk;

    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, TickPriority, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    __HAL_RCC_TIM6_CLK_ENABLE();

    HAL_RCC_GetClockConfig(&clk, &flash_latency);
    timclk = (clk.APB1CLKDivider == RCC_HCLK_DIV1) ? HAL_RCC_GetPCLK1Freq() : 2U * HAL_RCC_GetPCLK1Freq();

    htim6.Instance = TIM6;
    htim6.Init.Prescaler = (timclk / 1000000U) - 1U;   /* 1 MHz counter */
    htim6.Init.Period = 1000U - 1U;                    /* 1 ms */
    htim6.Init.ClockDivision = 0;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
        return HAL_ERROR;
    }
    return HAL_TIM_Base_Start_IT(&htim6);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        HAL_IncTick();
    }
}
