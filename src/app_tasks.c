/*
 * Akıllı ev kontrol merkezi: görevler ve aralarındaki iletişim.
 *
 *   SensorTask  --(qSensor kuyruğu)-->  ControlTask  --(durum, mutex)-->  DisplayTask
 *   UART kesmesi --(g_qUartRx kuyruğu)--> CommandTask  --(durum, mutex)-->  ...
 *   Buton kesmesi --(g_semButton semaforu)--> ButtonTask --(durum, mutex)--> ...
 *
 * Ortak veri (AppState_t) yalnızca s_stateMutex altında okunur ve yazılır.
 */
#include "app_tasks.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "app_logic.h"
#include "board.h"
#include "oled.h"
#include "uart_log.h"

typedef enum { FAN_AUTO = 0, FAN_FORCE_ON, FAN_FORCE_OFF } fan_mode_t;

typedef struct {
    int32_t    temp_deci;     /* kayan ortalamalı sıcaklık */
    int32_t    thr_deci;      /* alarm eşiği */
    bool       alarm;
    bool       fan_on;
    fan_mode_t fan_mode;
    uint8_t    page;          /* OLED sayfası: 0 durum, 1 sistem */
    uint32_t   samples;       /* işlenen ölçüm sayısı */
    uint32_t   dropped;       /* kuyruk dolu olduğu için atılan ölçüm */
} AppState_t;

typedef struct {
    int32_t  temp_deci;
    uint32_t tick;
} SensorMsg_t;

SemaphoreHandle_t g_semButton;

static SemaphoreHandle_t s_stateMutex;
static QueueHandle_t     s_qSensor;
static AppState_t        s_state;
static bool              s_oledOk;

/* Yığın kullanımını raporlamak için tutulur. */
static TaskHandle_t s_hSensor, s_hControl, s_hDisplay, s_hCommand, s_hButton, s_hBeat;

/* ---- ortak durum erişimi ---- */
static void state_get(AppState_t *copy)
{
    xSemaphoreTake(s_stateMutex, portMAX_DELAY);
    *copy = s_state;
    xSemaphoreGive(s_stateMutex);
}

/* ---- 1. SensorTask: her saniye iç sıcaklığı ölçer ---- */
static void SensorTask(void *arg)
{
    TickType_t last = xTaskGetTickCount();
    (void)arg;

    for (;;) {
        uint32_t sum = 0;
        SensorMsg_t m;

        for (int i = 0; i < 8; i++) {          /* 8 ölçümün ortalaması: gürültüyü azaltır */
            sum += Board_AdcReadTempRaw();
        }
        m.temp_deci = Logic_TempDeciFromAdc(sum / 8U, BOARD_VDDA_MV);
        m.tick = xTaskGetTickCount();

        if (xQueueSend(s_qSensor, &m, 0) != pdPASS) {   /* ControlTask yetişemediyse ölçümü at, bloklanma */
            xSemaphoreTake(s_stateMutex, portMAX_DELAY);
            s_state.dropped++;
            xSemaphoreGive(s_stateMutex);
        }
        Board_LedToggle(LED_ORANGE);
        vTaskDelayUntil(&last, pdMS_TO_TICKS(1000));    /* sabit periyot: kayma birikmez */
    }
}

/* ---- 2. ControlTask: ölçümü işler, alarm ve fan kararını verir ---- */
static void ControlTask(void *arg)
{
    avg_t avg;
    SensorMsg_t m;
    char t[12], th[12];
    (void)arg;

    Avg_Init(&avg);
    for (;;) {
        if (xQueueReceive(s_qSensor, &m, portMAX_DELAY) != pdPASS) {
            continue;
        }
        int32_t filtered = Avg_Push(&avg, m.temp_deci);
        bool prev_alarm, alarm, fan;
        int32_t thr;
        uint32_t n;

        xSemaphoreTake(s_stateMutex, portMAX_DELAY);
        prev_alarm = s_state.alarm;
        s_state.temp_deci = filtered;
        s_state.alarm = Logic_AlarmUpdate(s_state.alarm, filtered, s_state.thr_deci, TEMP_HYSTERESIS);
        switch (s_state.fan_mode) {
        case FAN_FORCE_ON:  s_state.fan_on = true;  break;
        case FAN_FORCE_OFF: s_state.fan_on = false; break;
        default:            s_state.fan_on = s_state.alarm; break;
        }
        s_state.samples++;
        alarm = s_state.alarm;
        fan = s_state.fan_on;
        thr = s_state.thr_deci;
        n = s_state.samples;
        xSemaphoreGive(s_stateMutex);

        Board_Led(LED_RED, alarm);
        Board_Led(LED_BLUE, fan);

        Logic_FormatDeci(t, sizeof t, filtered);
        Logic_FormatDeci(th, sizeof th, thr);
        if (alarm != prev_alarm) {
            Log("[ctrl] ALARM %s: %s C (esik %s C)\r\n", alarm ? "BASLADI" : "BITTI", t, th);
        } else if (n % 5U == 0U) {
            Log("[ctrl] sicaklik=%s C esik=%s C alarm=%d fan=%d\r\n", t, th, alarm, fan);
        }
    }
}

/* ---- 3. DisplayTask: OLED'e (varsa) durumu çizer ---- */
static void append(char *dst, size_t n, const char *src)
{
    size_t l = strlen(dst);
    while (*src && l + 1 < n) {
        dst[l++] = *src++;
    }
    dst[l] = '\0';
}

/* Küçük işaretsiz sayıyı ondalık yazıya çevirir (printf'siz). */
static void fmt_u32(char *dst, size_t n, uint32_t x)
{
    char tmp[11];
    size_t i = 0, k = 0;

    do {
        tmp[i++] = (char)('0' + x % 10U);
        x /= 10U;
    } while (x && i < sizeof tmp);
    while (i && k + 1 < n) {
        dst[k++] = tmp[--i];
    }
    dst[k] = '\0';
}

static void DisplayTask(void *arg)
{
    AppState_t s;
    char line[OLED_COLS + 1], v[12];
    TickType_t last = xTaskGetTickCount();
    (void)arg;

    s_oledOk = Oled_Init();
    Log("[disp] OLED %s\r\n", s_oledOk ? "hazir" : "bulunamadi, yalnizca UART kullanilacak");

    for (;;) {
        if (s_oledOk) {
            uint32_t up = xTaskGetTickCount() / 1000U;
            state_get(&s);
            Oled_Clear();
            Oled_Print(0, 0, "TECH-OHM  FreeRTOS");
            if (s.page == 0) {
                line[0] = '\0'; append(line, sizeof line, "Sicaklik "); Logic_FormatDeci(v, sizeof v, s.temp_deci); append(line, sizeof line, v); append(line, sizeof line, " C");
                Oled_Print(2, 0, line);
                line[0] = '\0'; append(line, sizeof line, "Esik     "); Logic_FormatDeci(v, sizeof v, s.thr_deci); append(line, sizeof line, v); append(line, sizeof line, " C");
                Oled_Print(3, 0, line);
                Oled_Print(5, 0, s.alarm ? "ALARM: VAR" : "ALARM: yok");
                Oled_Print(6, 0, s.fan_on ? "FAN: ACIK" : "FAN: kapali");
                Oled_Print(7, 0, s.fan_mode == FAN_AUTO ? "mod: otomatik" : "mod: manuel");
            } else {
                char u[12], num[12];
                uint32_t h = up / 3600U, mi = (up / 60U) % 60U, sec = up % 60U;
                u[0] = (char)('0' + (h / 10U) % 10U); u[1] = (char)('0' + h % 10U); u[2] = ':';
                u[3] = (char)('0' + mi / 10U); u[4] = (char)('0' + mi % 10U); u[5] = ':';
                u[6] = (char)('0' + sec / 10U); u[7] = (char)('0' + sec % 10U); u[8] = '\0';
                line[0] = '\0'; append(line, sizeof line, "Calisma: "); append(line, sizeof line, u);
                Oled_Print(2, 0, line);
                fmt_u32(num, sizeof num, s.samples);
                line[0] = '\0'; append(line, sizeof line, "Olcum: "); append(line, sizeof line, num);
                Oled_Print(4, 0, line);
                Oled_Print(6, 0, "Buton: sayfa degistir");
            }
            Oled_Update();
        }
        vTaskDelayUntil(&last, pdMS_TO_TICKS(500));
    }
}

/* ---- 4. ButtonTask: kesme semaforunu bekler, sayfa değiştirir ---- */
static void ButtonTask(void *arg)
{
    (void)arg;
    for (;;) {
        xSemaphoreTake(g_semButton, portMAX_DELAY);     /* kesme gelene kadar CPU harcamaz */
        vTaskDelay(pdMS_TO_TICKS(30));                  /* kontak sıçramasını bekle */
        while (xSemaphoreTake(g_semButton, 0) == pdPASS) { }   /* sıçrama kenarlarını at */
        if (Board_ButtonPressed()) {
            uint8_t page;
            xSemaphoreTake(s_stateMutex, portMAX_DELAY);
            s_state.page ^= 1U;
            page = s_state.page;
            xSemaphoreGive(s_stateMutex);
            Log("[btn] ekran sayfasi = %u\r\n", (unsigned)page);
        }
    }
}

/* ---- 5. CommandTask: seri porttan gelen komutları çalıştırır ---- */
static void print_help(void)
{
    Log("\r\nKomutlar:\r\n"
        "  STATUS        durum ve gorev yigin raporu\r\n"
        "  SET <derece>  alarm esigi, 10 ile 80 arasi (ornek: SET 35.5)\r\n"
        "  FAN ON|OFF|AUTO  fan modu\r\n"
        "  HELP          bu liste\r\n");
}

static void print_status(void)
{
    AppState_t s;
    char t[12], th[12];

    state_get(&s);
    Logic_FormatDeci(t, sizeof t, s.temp_deci);
    Logic_FormatDeci(th, sizeof th, s.thr_deci);
    Log("sicaklik=%s C esik=%s C alarm=%d fan=%d mod=%d sayfa=%u\r\n",
        t, th, s.alarm, s.fan_on, (int)s.fan_mode, (unsigned)s.page);
    Log("olcum=%lu atilan=%lu serbest heap=%u bayt\r\n",
        (unsigned long)s.samples, (unsigned long)s.dropped, (unsigned)xPortGetFreeHeapSize());
    Log("yigin kalan (kelime): sensor=%u kontrol=%u ekran=%u komut=%u buton=%u kalp=%u\r\n",
        (unsigned)uxTaskGetStackHighWaterMark(s_hSensor), (unsigned)uxTaskGetStackHighWaterMark(s_hControl),
        (unsigned)uxTaskGetStackHighWaterMark(s_hDisplay), (unsigned)uxTaskGetStackHighWaterMark(s_hCommand),
        (unsigned)uxTaskGetStackHighWaterMark(s_hButton), (unsigned)uxTaskGetStackHighWaterMark(s_hBeat));
}

static void CommandTask(void *arg)
{
    char line[40];
    size_t len = 0;
    uint8_t c;
    (void)arg;

    UartLog_StartRx();
    Log("\r\nTECH-OHM FreeRTOS akilli ev kontrol merkezi hazir. Komutlar icin HELP yazin.\r\n");

    for (;;) {
        if (xQueueReceive(g_qUartRx, &c, portMAX_DELAY) != pdPASS) {
            continue;
        }
        if (c != '\r' && c != '\n') {
            if (len + 1 < sizeof line) {
                line[len++] = (char)c;
            }
            continue;
        }
        line[len] = '\0';
        len = 0;

        command_t cmd;
        bool ok = Logic_ParseCommand(line, &cmd);
        switch (cmd.type) {
        case CMD_NONE:
            break;
        case CMD_HELP:
            print_help();
            break;
        case CMD_STATUS:
            print_status();
            break;
        case CMD_SET: {
            char th[12];
            xSemaphoreTake(s_stateMutex, portMAX_DELAY);
            s_state.thr_deci = cmd.arg;
            xSemaphoreGive(s_stateMutex);
            Logic_FormatDeci(th, sizeof th, cmd.arg);
            Log("esik = %s C\r\n", th);
            break;
        }
        case CMD_FAN_ON:
        case CMD_FAN_OFF:
        case CMD_FAN_AUTO:
            xSemaphoreTake(s_stateMutex, portMAX_DELAY);
            s_state.fan_mode = cmd.type == CMD_FAN_ON ? FAN_FORCE_ON : cmd.type == CMD_FAN_OFF ? FAN_FORCE_OFF : FAN_AUTO;
            xSemaphoreGive(s_stateMutex);
            Log("fan modu degisti\r\n");
            break;
        default:
            break;
        }
        if (!ok) {
            Log("gecersiz komut. HELP yazin.\r\n");
        }
    }
}

/* ---- 6. HeartbeatTask: sistem canlı mı? ---- */
static void HeartbeatTask(void *arg)
{
    (void)arg;
    for (;;) {
        Board_LedToggle(LED_GREEN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void App_Init(void)
{
    s_state.thr_deci = TEMP_DEFAULT_THR;
    s_state.fan_mode = FAN_AUTO;

    s_stateMutex = xSemaphoreCreateMutex();            /* öncelik mirası sağlar */
    s_qSensor    = xQueueCreate(8, sizeof(SensorMsg_t));
    g_semButton  = xSemaphoreCreateBinary();
    configASSERT(s_stateMutex && s_qSensor && g_semButton);

    /* Öncelik: Control 4 > Sensor, Button 3 > Command, Display 2 > Heartbeat 1 */
    BaseType_t ok = pdPASS;
    ok &= xTaskCreate(SensorTask,    "sensor",  384, NULL, 3, &s_hSensor);
    ok &= xTaskCreate(ControlTask,   "control", 512, NULL, 4, &s_hControl);
    ok &= xTaskCreate(DisplayTask,   "display", 512, NULL, 2, &s_hDisplay);
    ok &= xTaskCreate(CommandTask,   "command", 512, NULL, 2, &s_hCommand);
    ok &= xTaskCreate(ButtonTask,    "button",  384, NULL, 3, &s_hButton);
    ok &= xTaskCreate(HeartbeatTask, "beat",    128, NULL, 1, &s_hBeat);
    configASSERT(ok == pdPASS);
}
