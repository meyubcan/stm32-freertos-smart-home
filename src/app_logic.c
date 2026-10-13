/* M. Eyub Can, TECH-OHM (https://tech-ohm.com) */
#include "app_logic.h"

int32_t Logic_TempDeciFromAdc(uint32_t raw12, uint32_t vdda_mv)
{
    /* Vsense (mV), rounded */
    int32_t v_mv = (int32_t)((raw12 * vdda_mv + 2047U) / 4095U);
    /* T = (Vsense - 0.76 V) / 2.5 mV + 25 C  ->  tenths of a degree: (v - 760) * 4 + 250 */
    return (v_mv - 760) * 4 + 250;
}

bool Logic_AlarmUpdate(bool alarm, int32_t temp_deci, int32_t thr_deci, int32_t hyst_deci)
{
    if (!alarm && temp_deci >= thr_deci) {
        return true;
    }
    if (alarm && temp_deci <= thr_deci - hyst_deci) {
        return false;
    }
    return alarm;
}

static char up(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

/* Skips leading spaces. */
static const char *skip_ws(const char *s)
{
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    return s;
}

/* If s starts with the given word, returns what follows it. */
static const char *match_word(const char *s, const char *word)
{
    s = skip_ws(s);
    while (*word) {
        if (up(*s) != *word) {
            return NULL;
        }
        s++;
        word++;
    }
    if (*s != '\0' && *s != ' ' && *s != '\t') {
        return NULL;
    }
    return s;
}

/* "35" or "35.5" -> 350 / 355. Only spaces may follow. */
static bool parse_deci(const char *s, int32_t *out)
{
    int32_t whole = 0, frac = 0;
    int digits = 0;

    s = skip_ws(s);
    while (*s >= '0' && *s <= '9') {
        whole = whole * 10 + (*s - '0');
        digits++;
        if (whole > 10000) {
            return false;
        }
        s++;
    }
    if (digits == 0) {
        return false;
    }
    if (*s == '.') {
        s++;
        if (*s < '0' || *s > '9') {
            return false;
        }
        frac = *s - '0';
        s++;
    }
    s = skip_ws(s);
    if (*s != '\0') {
        return false;
    }
    *out = whole * 10 + frac;
    return true;
}

bool Logic_ParseCommand(const char *line, command_t *out)
{
    const char *p;

    out->type = CMD_NONE;
    out->arg = 0;
    line = skip_ws(line);
    if (*line == '\0') {
        return true; /* empty line: ignore quietly */
    }

    if (match_word(line, "HELP") || match_word(line, "?")) {
        out->type = CMD_HELP;
        return true;
    }
    if (match_word(line, "STATUS")) {
        out->type = CMD_STATUS;
        return true;
    }
    if ((p = match_word(line, "SET")) != NULL) {
        int32_t v;
        if (parse_deci(p, &v) && v >= TEMP_THR_MIN_DECI && v <= TEMP_THR_MAX_DECI) {
            out->type = CMD_SET;
            out->arg = v;
            return true;
        }
        out->type = CMD_INVALID;
        return false;
    }
    if ((p = match_word(line, "FAN")) != NULL) {
        if (match_word(p, "ON")) {
            out->type = CMD_FAN_ON;
            return true;
        }
        if (match_word(p, "OFF")) {
            out->type = CMD_FAN_OFF;
            return true;
        }
        if (match_word(p, "AUTO")) {
            out->type = CMD_FAN_AUTO;
            return true;
        }
    }
    out->type = CMD_INVALID;
    return false;
}

void Logic_FormatDeci(char *buf, size_t n, int32_t deci)
{
    char tmp[16];
    size_t i = 0, k = 0;
    bool neg = deci < 0;
    uint32_t u = neg ? (uint32_t)(-deci) : (uint32_t)deci;
    uint32_t whole = u / 10U, frac = u % 10U;

    if (n == 0) {
        return;
    }
    if (neg) {
        tmp[i++] = '-';
    }
    {
        char d[10];
        int dn = 0;
        do {
            d[dn++] = (char)('0' + whole % 10U);
            whole /= 10U;
        } while (whole && dn < 10);
        while (dn) {
            tmp[i++] = d[--dn];
        }
    }
    tmp[i++] = '.';
    tmp[i++] = (char)('0' + frac);
    tmp[i] = '\0';
    while (tmp[k] && k + 1 < n) {
        buf[k] = tmp[k];
        k++;
    }
    buf[k] = '\0';
}

void Avg_Init(avg_t *a)
{
    a->count = 0;
    a->head = 0;
    for (int i = 0; i < AVG_WINDOW; i++) {
        a->buf[i] = 0;
    }
}

int32_t Avg_Push(avg_t *a, int32_t v)
{
    int64_t sum = 0;

    a->buf[a->head] = v;
    a->head = (uint8_t)((a->head + 1U) % AVG_WINDOW);
    if (a->count < AVG_WINDOW) {
        a->count++;
    }
    for (uint8_t i = 0; i < a->count; i++) {
        sum += a->buf[i];
    }
    return (int32_t)(sum / a->count);
}
