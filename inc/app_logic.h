/* M. Eyub Can, TECH-OHM (https://tech-ohm.com) */
/* Application logic with no hardware dependency, so it can be tested on a PC. */
#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Temperatures are kept in tenths of a degree: 364 = 36.4 C. No floating point. */
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
    int32_t arg;   /* threshold for CMD_SET, tenths of a degree */
} command_t;

#define AVG_WINDOW 5

typedef struct {
    int32_t buf[AVG_WINDOW];
    uint8_t count;
    uint8_t head;
} avg_t;

/* STM32F4 internal temperature sensor: V25 = 0.76 V, slope = 2.5 mV/C. Returns tenths of a degree. */
int32_t Logic_TempDeciFromAdc(uint32_t raw12, uint32_t vdda_mv);

/* Alarm with hysteresis: turns on at the threshold, turns off below (threshold - hysteresis). */
bool Logic_AlarmUpdate(bool alarm, int32_t temp_deci, int32_t thr_deci, int32_t hyst_deci);

/* Turns a line from the serial port into a command. Case does not matter. */
bool Logic_ParseCommand(const char *line, command_t *out);

/* 364 -> "36.4" */
void Logic_FormatDeci(char *buf, size_t n, int32_t deci);

/* Moving average: adds the new value and returns the current average. */
void    Avg_Init(avg_t *a);
int32_t Avg_Push(avg_t *a, int32_t v);

#endif
