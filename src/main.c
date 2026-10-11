#include "FreeRTOS.h"
#include "task.h"
#include "app_tasks.h"
#include "board.h"
#include "uart_log.h"

/* 8 MHz HSE -> PLL -> 168 MHz. APB1 42 MHz, APB2 84 MHz. */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 8;
    osc.PLL.PLLN = 336;
    osc.PLL.PLLP = RCC_PLLP_DIV2;
    osc.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        Error_Handler();
    }

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4;
    clk.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

int main(void)
{
    HAL_Init();               /* öncelik gruplaması 4 bit: FreeRTOS için şart */
    SystemClock_Config();
    Board_Init();
    UartLog_Init();
    App_Init();
    vTaskStartScheduler();    /* buraya dönmez */

    for (;;) { }
}

void Error_Handler(void)
{
    __disable_irq();
    for (;;) {
        Board_Led(LED_RED, true);
        for (volatile uint32_t i = 0; i < 1500000U; i++) { }
        Board_Led(LED_RED, false);
        for (volatile uint32_t i = 0; i < 1500000U; i++) { }
    }
}
