/* M. Eyub Can, TECH-OHM (https://tech-ohm.com) */
/* STM32F407G-DISC1 board layer: pins, LEDs, ADC, UART and I2C. */
#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"

typedef enum {
    LED_GREEN  = GPIO_PIN_12,   /* PD12: heartbeat */
    LED_ORANGE = GPIO_PIN_13,   /* PD13: toggles on every sample */
    LED_RED    = GPIO_PIN_14,   /* PD14: temperature alarm */
    LED_BLUE   = GPIO_PIN_15    /* PD15: fan (simulated output) */
} board_led_t;

extern UART_HandleTypeDef huart2;   /* PA2 TX, PA3 RX: 115200 8N1 through a USB-TTL adapter */
extern I2C_HandleTypeDef  hi2c1;    /* PB6 SCL, PB9 SDA: SSD1306 OLED, address 0x3C */
extern ADC_HandleTypeDef  hadc1;    /* internal temperature sensor, channel 16 */

void Board_Init(void);
void Board_Led(board_led_t led, bool on);
void Board_LedToggle(board_led_t led);
bool Board_ButtonPressed(void);        /* PA0, blue USER button */
uint32_t Board_AdcReadTempRaw(void);   /* raw 12-bit value */

#define BOARD_VDDA_MV 3000U            /* analog supply is 3.0 V on the Discovery board */

void Error_Handler(void);

#endif
