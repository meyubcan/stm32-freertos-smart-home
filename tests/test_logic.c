/* M. Eyub Can, TECH-OHM (https://tech-ohm.com) */
/* Unit tests that run on a PC: make test */
#include <stdio.h>
#include <string.h>
#include "app_logic.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static command_t parse(const char *s, bool *ok)
{
    command_t c;
    *ok = Logic_ParseCommand(s, &c);
    return c;
}

int main(void)
{
    bool ok;
    command_t c;
    char b[16];

    /* ADC -> temperature: 760 mV = 25.0 C */
    CHECK(Logic_TempDeciFromAdc(1038, 3000) == 250);
    CHECK(Logic_TempDeciFromAdc(1100, 3000) > 250);   /* higher voltage, hotter */
    CHECK(Logic_TempDeciFromAdc(1000, 3000) < 250);

    /* alarm hysteresis */
    CHECK(Logic_AlarmUpdate(false, 399, 400, 20) == false);
    CHECK(Logic_AlarmUpdate(false, 400, 400, 20) == true);
    CHECK(Logic_AlarmUpdate(true, 390, 400, 20) == true);   /* stays on below the threshold until the hysteresis is passed */
    CHECK(Logic_AlarmUpdate(true, 381, 400, 20) == true);
    CHECK(Logic_AlarmUpdate(true, 380, 400, 20) == false);

    /* commands */
    c = parse("status", &ok);       CHECK(ok && c.type == CMD_STATUS);
    c = parse("  HELP  ", &ok);     CHECK(ok && c.type == CMD_HELP);
    c = parse("?", &ok);            CHECK(ok && c.type == CMD_HELP);
    c = parse("set 35", &ok);       CHECK(ok && c.type == CMD_SET && c.arg == 350);
    c = parse("SET 35.5", &ok);     CHECK(ok && c.type == CMD_SET && c.arg == 355);
    c = parse("SET 9", &ok);        CHECK(!ok && c.type == CMD_INVALID);   /* lower limit is 10 C */
    c = parse("SET 81", &ok);       CHECK(!ok && c.type == CMD_INVALID);   /* upper limit is 80 C */
    c = parse("SET abc", &ok);      CHECK(!ok && c.type == CMD_INVALID);
    c = parse("SET 35x", &ok);      CHECK(!ok && c.type == CMD_INVALID);
    c = parse("fan on", &ok);       CHECK(ok && c.type == CMD_FAN_ON);
    c = parse("FAN OFF", &ok);      CHECK(ok && c.type == CMD_FAN_OFF);
    c = parse("Fan Auto", &ok);     CHECK(ok && c.type == CMD_FAN_AUTO);
    c = parse("FAN", &ok);          CHECK(!ok && c.type == CMD_INVALID);
    c = parse("STATUSX", &ok);      CHECK(!ok && c.type == CMD_INVALID);
    c = parse("", &ok);             CHECK(ok && c.type == CMD_NONE);

    /* formatting */
    Logic_FormatDeci(b, sizeof b, 364);   CHECK(strcmp(b, "36.4") == 0);
    Logic_FormatDeci(b, sizeof b, 5);     CHECK(strcmp(b, "0.5") == 0);
    Logic_FormatDeci(b, sizeof b, -25);   CHECK(strcmp(b, "-2.5") == 0);
    Logic_FormatDeci(b, 4, 12345);        CHECK(strlen(b) == 3);          /* buffer does not overflow */

    /* moving average */
    {
        avg_t a;
        Avg_Init(&a);
        CHECK(Avg_Push(&a, 300) == 300);
        CHECK(Avg_Push(&a, 400) == 350);
        for (int i = 0; i < 10; i++) { Avg_Push(&a, 500); }
        CHECK(Avg_Push(&a, 500) == 500);
    }

    if (fails) {
        printf("%d tests failed\n", fails);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
