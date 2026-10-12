/* M. Eyub Can, TECH-OHM (https://tech-ohm.com) */
/* Kesme vektörleri. SVC, PendSV ve SysTick FreeRTOS'a aittir (FreeRTOSConfig.h). */
#include "board.h"

extern TIM_HandleTypeDef htim6;

void NMI_Handler(void)        { for (;;) { } }
void HardFault_Handler(void)  { for (;;) { } }
void MemManage_Handler(void)  { for (;;) { } }
void BusFault_Handler(void)   { for (;;) { } }
void UsageFault_Handler(void) { for (;;) { } }
void DebugMon_Handler(void)   { }

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim6);
}
