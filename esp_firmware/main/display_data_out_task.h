#ifndef _DISPLAY_DATA_OUT_HEADER
#define _DISPLAY_DATA_OUT_HEADER

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define CHANNEL_LED_NUM 60

#define RGB(r, g, b) (led_color_t){ g, r, b }
typedef struct {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} led_color_t;

extern uint8_t displayStagingBuffer[8][CHANNEL_LED_NUM][3];
//extern SemaphoreHandle_t dataBufferSemaphore;

void notify_start_blit();
void wait_blit_ready();
void init_display_data_out_task();

#endif
