#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/rmt.h"
#include "display_data_out_task.h"
#include "colors.h"

SemaphoreHandle_t dataBufferSemaphore;
TaskHandle_t displayDataOutTaskHandle;

static SemaphoreHandle_t dataOutSemaphore;
static SemaphoreHandle_t dataUpdateSemaphore;

#define DOEG_RENDERING_BIT 0
#define DOEG_RENDERING_MASK (1 << DOEG_RENDERING_BIT)
#define DOEG_READY_BIT 1
#define DOEG_READY_MASK (1 << DOEG_READY_BIT)
static EventGroupHandle_t dataOutEventGroup;

//float displayDataBuffer[8][CHANNEL_LED_NUM][3];
uint8_t displayStagingBuffer[8][CHANNEL_LED_NUM][3];

static void IRAM_ATTR color_val_to_rmt(const void *src, rmt_item32_t *dest, size_t src_size,
        size_t wanted_num, size_t *translated_size, size_t *item_num) {

    if (src == NULL || dest == NULL) {
        *translated_size = 0;
        *item_num = 0;
    }

    // ACTUAL
    //const rmt_item32_t bit0 = {{{ 7, 1, 16, 0 }}};
    //const rmt_item32_t bit1 = {{{ 14, 1, 12, 0 }}};
    
    const rmt_item32_t bit0 = {{{ 10, 1, 19, 0 }}};
    const rmt_item32_t bit1 = {{{ 17, 1, 15, 0 }}};

    size_t size = 0;
    size_t num = 0;

    uint8_t *psrc = (uint8_t *)src;
    rmt_item32_t *pdest = dest;

    while (size < src_size && num < wanted_num) {
        for (int i = 0; i < 8; i++) {
            if (*psrc & (0b10000000 >> i)) {
                pdest->val = bit1.val;
            } else {
                pdest->val = bit0.val;
            }
            num++;
            pdest++;
        }
        size++;
        psrc++;
    }

    *translated_size = size;
    *item_num = num;

}

static void init_led_channel(gpio_num_t pin, rmt_channel_t channel) {
    rmt_config_t cfg;
    cfg.rmt_mode = RMT_MODE_TX;
    cfg.channel = channel;
    cfg.gpio_num = pin;
    cfg.mem_block_num = 1;
    cfg.tx_config.loop_en = 0;

    cfg.tx_config.carrier_en = 0;
    cfg.tx_config.idle_output_en = 1;
    cfg.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;

    // 1 / (80MHz / 4) = 0.05us
    cfg.clk_div = 4;
    // T0H: 0.05us * 7 = 0.35us
    // T1H: 0.05us * 14 = 0.7us
    // T0L: 0.05us * 16 = 0.8us
    // T1L: 0.05us * 12 = 0.6us
    // RES: 0.05us * 1400 = 70us
    
    ESP_ERROR_CHECK(rmt_config(&cfg));
    ESP_ERROR_CHECK(rmt_driver_install(cfg.channel, 0, 0));
    ESP_ERROR_CHECK(rmt_translator_init(cfg.channel, color_val_to_rmt));
}

static void copy_to_staging(float *buf, uint8_t *staging, size_t num) {
    for (int i = 0; i < num; i++) {
        *staging = (uint8_t)(255 * (*buf));
        if (*buf < 0) {
            *staging = 0;
        } else if (*buf >= 1) {
            *staging = 255;
        }
        buf++;
        staging++;
    }
}

void notify_start_blit() {
    xEventGroupSetBits(dataOutEventGroup, DOEG_RENDERING_MASK);
}

void wait_blit_ready() {
    xEventGroupWaitBits(dataOutEventGroup, DOEG_READY_MASK, pdTRUE, pdFALSE, portMAX_DELAY);
}

static void display_data_out_task(void *pvParameter) {
    //led_color_t colors[80];

    init_led_channel(GPIO_NUM_18, RMT_CHANNEL_0);
    init_led_channel(GPIO_NUM_4 , RMT_CHANNEL_1);
    init_led_channel(GPIO_NUM_16, RMT_CHANNEL_2);
    init_led_channel(GPIO_NUM_25, RMT_CHANNEL_3);
    init_led_channel(GPIO_NUM_26, RMT_CHANNEL_4);
    init_led_channel(GPIO_NUM_27, RMT_CHANNEL_5);
    init_led_channel(GPIO_NUM_14, RMT_CHANNEL_6);
    init_led_channel(GPIO_NUM_17, RMT_CHANNEL_7);

    //float rot = 0.0;

    while (true) {
        xEventGroupSetBits(dataOutEventGroup, DOEG_READY_MASK);
        xEventGroupWaitBits(dataOutEventGroup, DOEG_RENDERING_MASK, pdFALSE, pdFALSE, portMAX_DELAY);

        //rot += 0.1;

        //float r, g, b;
        //hsv_to_rgb(rot, 1.0, 1.0, &r, &g, &b);

        //for (int i = 0; i < 7; i++) {
        //    for (int k = 0; k < 60; k++) {
        //        displayDataBuffer[i][k][0] = r;
        //        displayDataBuffer[i][k][1] = g;
        //        displayDataBuffer[i][k][2] = b;
        //    }
        //}

        // Hackheim: 40
        // hAckheim: 37
        // haCkheim: 33
        // hacKheim: 42
        // hackHeim: 24
        // hackhEIm: 57
        // hackheiM: 30

        //ESP_ERROR_CHECK(rmt_write_sample(RMT_CHANNEL_0, (uint8_t *) colors, 3*80, true)); // NC
        int64_t startTime = esp_timer_get_time();

//#define N_hackhEIm_CHNUM RMT_CHANNEL_1
//#define N_haCkheim_CHNUM RMT_CHANNEL_3
//#define N_hackheiM_CHNUM RMT_CHANNEL_7
//#define N_hacKheim_CHNUM RMT_CHANNEL_6
//#define N_hAckheim_CHNUM RMT_CHANNEL_2
//#define N_Hackheim_CHNUM RMT_CHANNEL_4
//#define N_hackHeim_CHNUM RMT_CHANNEL_5

#define N_hackHeim_CHNUM RMT_CHANNEL_1 //sH
#define N_hackheiM_CHNUM RMT_CHANNEL_3 //M
#define N_haCkheim_CHNUM RMT_CHANNEL_7 //C
#define N_hacKheim_CHNUM RMT_CHANNEL_6 //K
#define N_Hackheim_CHNUM RMT_CHANNEL_2 //H
#define N_hAckheim_CHNUM RMT_CHANNEL_4 //A
#define N_hackhEIm_CHNUM RMT_CHANNEL_5 //EI

        //copy_to_staging(displayDataBuffer[5][0], displayStagingBuffer[5][0], 3*57);

        ESP_ERROR_CHECK(rmt_write_sample(N_hackhEIm_CHNUM, displayStagingBuffer[5][0], 3*57, true)); // hackhEIm

        //copy_to_staging(displayDataBuffer[0][0], displayStagingBuffer[0][0], 3*42);
        //copy_to_staging(displayDataBuffer[3][0], displayStagingBuffer[3][0], 3*42);
        //rmt_wait_tx_done(N_hackhEIm_CHNUM, 100 / portTICK_PERIOD_MS);

        ESP_ERROR_CHECK(rmt_write_sample(N_Hackheim_CHNUM, (const uint8_t *) displayStagingBuffer[0][0], 3*42, true)); // Hackheim
        ESP_ERROR_CHECK(rmt_write_sample(N_hacKheim_CHNUM, (const uint8_t *) displayStagingBuffer[3][0], 3*42, true)); // hacKheim

        //copy_to_staging(displayDataBuffer[1][0], displayStagingBuffer[1][0], 3*37);
        //copy_to_staging(displayDataBuffer[2][0], displayStagingBuffer[2][0], 3*37);
        //rmt_wait_tx_done(N_hacKheim_CHNUM, 100 / portTICK_PERIOD_MS);

        ESP_ERROR_CHECK(rmt_write_sample(N_hAckheim_CHNUM, (const uint8_t *) displayStagingBuffer[1][0], 3*37, true)); // hAckheim
        ESP_ERROR_CHECK(rmt_write_sample(N_haCkheim_CHNUM, (const uint8_t *) displayStagingBuffer[2][0], 3*37, true)); // haCkheim

        //copy_to_staging(displayDataBuffer[4][0], displayStagingBuffer[4][0], 3*30);
        //copy_to_staging(displayDataBuffer[6][0], displayStagingBuffer[6][0], 3*30);
        //rmt_wait_tx_done(N_haCkheim_CHNUM, 100 / portTICK_PERIOD_MS);

        ESP_ERROR_CHECK(rmt_write_sample(N_hackHeim_CHNUM, (const uint8_t *) displayStagingBuffer[4][0], 3*30, true)); // hackHeim
        ESP_ERROR_CHECK(rmt_write_sample(N_hackheiM_CHNUM, (const uint8_t *) displayStagingBuffer[6][0], 3*30, true)); // hackheiM

        //rmt_wait_tx_done(N_hackheiM_CHNUM, 100 / portTICK_PERIOD_MS);


        //copy_to_staging(displayDataBuffer[0][0], displayStagingBuffer[0][0], 3*42);
        //copy_to_staging(displayDataBuffer[3][0], displayStagingBuffer[3][0], 3*42);
        //copy_to_staging(displayDataBuffer[1][0], displayStagingBuffer[1][0], 3*37);
        //copy_to_staging(displayDataBuffer[2][0], displayStagingBuffer[2][0], 3*37);
        //copy_to_staging(displayDataBuffer[4][0], displayStagingBuffer[4][0], 3*30);
        //copy_to_staging(displayDataBuffer[6][0], displayStagingBuffer[6][0], 3*30);

        //ESP_ERROR_CHECK(rmt_write_sample(N_hackhEIm_CHNUM, displayStagingBuffer[5][0], 3*57, false)); // hackhEIm
        //ESP_ERROR_CHECK(rmt_write_sample(N_Hackheim_CHNUM, (const uint8_t *) displayStagingBuffer[0][0], 3*42, false)); // Hackheim
        //ESP_ERROR_CHECK(rmt_write_sample(N_hacKheim_CHNUM, (const uint8_t *) displayStagingBuffer[3][0], 3*42, false)); // hacKheim
        //ESP_ERROR_CHECK(rmt_write_sample(N_hAckheim_CHNUM, (const uint8_t *) displayStagingBuffer[1][0], 3*37, false)); // hAckheim
        //ESP_ERROR_CHECK(rmt_write_sample(N_haCkheim_CHNUM, (const uint8_t *) displayStagingBuffer[2][0], 3*37, false)); // haCkheim
        //ESP_ERROR_CHECK(rmt_write_sample(N_hackHeim_CHNUM, (const uint8_t *) displayStagingBuffer[4][0], 3*30, false)); // hackHeim
        //ESP_ERROR_CHECK(rmt_write_sample(N_hackheiM_CHNUM, (const uint8_t *) displayStagingBuffer[6][0], 3*30, false)); // hackheiM

        int64_t endTime = esp_timer_get_time();

        //printf("%lld\n", endTime - startTime);

        //vTaskDelay(1 / portTICK_PERIOD_MS);
        
        xEventGroupClearBits(dataOutEventGroup, DOEG_RENDERING_MASK);

    }


}

void init_display_data_out_task() {

    dataOutEventGroup = xEventGroupCreate();

    dataBufferSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(dataBufferSemaphore);
    
    dataOutSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(dataOutSemaphore);
    dataUpdateSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(dataUpdateSemaphore);

    xTaskCreate(&display_data_out_task, "display_data_out_task", 8000, NULL, 8, 
            &displayDataOutTaskHandle);
}
