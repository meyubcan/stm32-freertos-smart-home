/* M. Eyub Can, TECH-OHM (https://tech-ohm.com) */
#include "oled.h"
#include <string.h>
#include "board.h"
#include "fonts.h"      /* STM32Cube/Utilities/Fonts */
#include "FreeRTOS.h"
#include "semphr.h"

#define OLED_ADDR   (0x3C << 1)
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define CHAR_W      6   /* Font8 genişliği 5 piksel + 1 boşluk */

static uint8_t s_fb[OLED_WIDTH * OLED_HEIGHT / 8];
static SemaphoreHandle_t s_busMutex;   /* I2C hattını ileride başka sensörlerle paylaşmak için */

static bool cmd(const uint8_t *c, uint16_t n)
{
    return HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, (uint8_t *)c, n, 50) == HAL_OK;
}

bool Oled_Init(void)
{
    static const uint8_t init[] = {
        0xAE,             /* ekran kapalı */
        0xD5, 0x80,       /* saat bölücü */
        0xA8, 0x3F,       /* 64 satır */
        0xD3, 0x00,       /* ofset yok */
        0x40,             /* başlangıç satırı */
        0x8D, 0x14,       /* dahili şarj pompası açık */
        0x20, 0x00,       /* yatay adresleme */
        0xA1, 0xC8,       /* ayna: sağ-sol ve üst-alt */
        0xDA, 0x12,
        0x81, 0xCF,       /* kontrast */
        0xD9, 0xF1,
        0xDB, 0x40,
        0xA4, 0xA6,       /* RAM içeriğini göster, normal polarite */
        0xAF              /* ekran açık */
    };

    if (s_busMutex == NULL) {
        s_busMutex = xSemaphoreCreateMutex();
    }
    if (HAL_I2C_IsDeviceReady(&hi2c1, OLED_ADDR, 2, 20) != HAL_OK) {
        return false;
    }
    if (!cmd(init, sizeof init)) {
        return false;
    }
    Oled_Clear();
    return Oled_Update();
}

void Oled_Clear(void)
{
    memset(s_fb, 0, sizeof s_fb);
}

static void put_pixel(unsigned x, unsigned y)
{
    if (x < OLED_WIDTH && y < OLED_HEIGHT) {
        s_fb[x + (y / 8) * OLED_WIDTH] |= (uint8_t)(1U << (y % 8));
    }
}

static void put_char(unsigned x, unsigned y, char c)
{
    const uint8_t *g;

    if (c < 32 || c > 126) {
        c = '?';
    }
    g = &Font8.table[(c - 32) * Font8.Height];
    for (unsigned row = 0; row < Font8.Height; row++) {
        uint8_t line = g[row];
        for (unsigned col = 0; col < Font8.Width; col++) {
            if (line & (0x80U >> col)) {      /* en soldaki piksel en yüksek bit */
                put_pixel(x + col, y + row);
            }
        }
    }
}

void Oled_Print(unsigned row, unsigned col, const char *text)
{
    unsigned x = col * CHAR_W, y = row * 8;

    while (*text && col < OLED_COLS && row < OLED_ROWS) {
        put_char(x, y, *text++);
        x += CHAR_W;
        col++;
    }
}

bool Oled_Update(void)
{
    static const uint8_t window[] = { 0x21, 0, OLED_WIDTH - 1, 0x22, 0, 7 };
    bool ok = false;

    if (s_busMutex == NULL || xSemaphoreTake(s_busMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }
    if (cmd(window, sizeof window)) {
        ok = HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, 0x40, I2C_MEMADD_SIZE_8BIT, s_fb, sizeof s_fb, 200) == HAL_OK;
    }
    xSemaphoreGive(s_busMutex);
    return ok;
}
