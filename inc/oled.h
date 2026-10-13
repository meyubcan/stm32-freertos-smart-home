/* M. Eyub Can, TECH-OHM (https://tech-ohm.com) */
/* Minimal SSD1306 driver (128x64, I2C). The font is Font8 from the STM32Cube package. */
#ifndef OLED_H
#define OLED_H

#include <stdbool.h>

#define OLED_COLS  21   /* characters per row with a 6-pixel-wide font */
#define OLED_ROWS  8

bool Oled_Init(void);                              /* false if no display is found */
void Oled_Clear(void);
void Oled_Print(unsigned row, unsigned col, const char *text);
bool Oled_Update(void);                            /* sends the frame buffer to the display */

#endif
