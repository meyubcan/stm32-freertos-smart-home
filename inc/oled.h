/* Minimal SSD1306 (128x64, I2C) sürücüsü. Yazı tipi STM32Cube paketindeki Font8'dir. */
#ifndef OLED_H
#define OLED_H

#include <stdbool.h>

#define OLED_COLS  21   /* 6 piksellik karakterle bir satıra sığan karakter */
#define OLED_ROWS  8

bool Oled_Init(void);                              /* ekran bulunamazsa false */
void Oled_Clear(void);
void Oled_Print(unsigned row, unsigned col, const char *text);
bool Oled_Update(void);                            /* çerçeve tamponunu ekrana gönderir */

#endif
