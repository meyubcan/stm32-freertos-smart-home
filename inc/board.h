/* STM32F407G-DISC1 kart katmanı: pinler, LED'ler, ADC, UART ve I2C. */
#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"

typedef enum {
    LED_GREEN  = GPIO_PIN_12,   /* PD12: kalp atışı */
    LED_ORANGE = GPIO_PIN_13,   /* PD13: her ölçümde yanıp söner */
    LED_RED    = GPIO_PIN_14,   /* PD14: sıcaklık alarmı */
    LED_BLUE   = GPIO_PIN_15    /* PD15: fan (çıkış simülasyonu) */
} board_led_t;

extern UART_HandleTypeDef huart2;   /* PA2 TX, PA3 RX: USB-TTL ile 115200 8N1 */
extern I2C_HandleTypeDef  hi2c1;    /* PB6 SCL, PB9 SDA: SSD1306 OLED, adres 0x3C */
extern ADC_HandleTypeDef  hadc1;    /* iç sıcaklık sensörü, kanal 16 */

void Board_Init(void);
void Board_Led(board_led_t led, bool on);
void Board_LedToggle(board_led_t led);
bool Board_ButtonPressed(void);        /* PA0, mavi USER butonu */
uint32_t Board_AdcReadTempRaw(void);   /* 12 bit ham değer */

#define BOARD_VDDA_MV 3000U            /* Discovery kartında analog besleme 3.0 V */

void Error_Handler(void);

#endif
