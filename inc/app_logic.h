/* Donanımdan bağımsız uygulama mantığı: bilgisayarda test edilebilir. */
#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Sıcaklıklar "deci derece" tutulur: 364 = 36.4 C. Kayan nokta kullanılmaz. */
#define TEMP_THR_MIN_DECI   100
#define TEMP_THR_MAX_DECI   800
#define TEMP_DEFAULT_THR    400
#define TEMP_HYSTERESIS     20

typedef enum {
    CMD_NONE = 0,
    CMD_HELP,
    CMD_STATUS,
    CMD_SET,
    CMD_FAN_ON,
    CMD_FAN_OFF,
    CMD_FAN_AUTO,
    CMD_INVALID
} cmd_type_t;

typedef struct {
    cmd_type_t type;
    int32_t arg;   /* CMD_SET için eşik, deci derece */
} command_t;

#define AVG_WINDOW 5

typedef struct {
    int32_t buf[AVG_WINDOW];
    uint8_t count;
    uint8_t head;
} avg_t;

/* STM32F4 iç sıcaklık sensörü: V25 = 0.76 V, eğim = 2.5 mV/C. Dönüş: deci derece. */
int32_t Logic_TempDeciFromAdc(uint32_t raw12, uint32_t vdda_mv);

/* Histerezisli alarm: eşiğe ulaşınca açılır, (eşik - histerezis) altına inince kapanır. */
bool Logic_AlarmUpdate(bool alarm, int32_t temp_deci, int32_t thr_deci, int32_t hyst_deci);

/* Seri porttan gelen bir satırı komuta çevirir. Büyük/küçük harf fark etmez. */
bool Logic_ParseCommand(const char *line, command_t *out);

/* 364 -> "36.4" */
void Logic_FormatDeci(char *buf, size_t n, int32_t deci);

/* Kayan ortalama: yeni değeri ekler, güncel ortalamayı döndürür. */
void    Avg_Init(avg_t *a);
int32_t Avg_Push(avg_t *a, int32_t v);

#endif
