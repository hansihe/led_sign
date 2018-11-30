#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event_loop.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "mqtt_client.h"

#include "display_data_out_task.h"
#include "render_task.h"

#define TAG "networking_task"

#define PI 3.1415926535897

static EventGroupHandle_t wifi_event_group;

const int WIFI_CONNECTED_BIT = BIT0;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, "hackheim/bigsign/control", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            {
                ESP_LOGI(TAG, "MQTT_EVENT_DATA");
                //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
                //printf("DATA=%.*s\r\n", event->data_len, event->data);

                if (event->data_len == 0) return ESP_OK;
                printf("Received MQTT command, id: %d, len: %d\n", event->data[0], event->data_len);

                //if (event->data[0] == 't') {
                //    uint8_t data[event->data_len + 1];
                //    memcpy(data, event->data, event->data_len);
                //    data[event->data_len] = 0;

                //    int strip_num, led_num;
                //    float r, g, b;

                //    if (sscanf((char *)data, "t %d %d %f %f %f d", &strip_num, &led_num, &r, &g, &b)) {
                //        for (int i = 0; i < 8; i++) {
                //            for (int k = 0; k < 60; k++) {
                //                displayStagingBuffer[i][k][0] = 0;
                //                displayStagingBuffer[i][k][1] = 0;
                //                displayStagingBuffer[i][k][2] = 0;
                //            }
                //        }
                //        if (strip_num < 8 && led_num < 60) {
                //            displayStagingBuffer[strip_num][led_num][0] = (uint8_t) (r * 255);
                //            displayStagingBuffer[strip_num][led_num][1] = (uint8_t) (g * 255);
                //            displayStagingBuffer[strip_num][led_num][2] = (uint8_t) (b * 255);
                //        }
                //    }
                //}

                uint8_t *ptr = (uint8_t *) event->data;
                size_t data_len = event->data_len;
                if (*ptr == 10 && event->data_len >= (1 + PIXEL_GRID_WIDTH*PIXEL_GRID_HEIGHT*3)) {
                    ptr += 1;

                    printf("Received buffer update\n");
                    color_f3_t buf[PIXEL_GRID_WIDTH][PIXEL_GRID_HEIGHT];

                    for (int i = 0; i < PIXEL_GRID_HEIGHT; i++) {
                        for (int k = 0; k < PIXEL_GRID_WIDTH; k++) {
                            buf[k][i].h = ((float) *ptr) / 255.0;
                            ptr += 1;
                            buf[k][i].s = ((float) *ptr) / 255.0;
                            ptr += 1;
                            buf[k][i].v = ((float) *ptr) / 255.0;
                            ptr += 1;
                        }
                    }

                    lockPixels();
                    updatePixelData(buf);
                    unlockPixels();
                } else if (ptr[0] == 't' && data_len < 500) {
                    printf("Text message\n");
                    uint8_t buf[501];
                    memcpy(buf, ptr, data_len);
                    buf[data_len] = 0;

                    switch (ptr[1]) {
                        case 'u':
                            printf("Update single color\n");
                            uint8_t h, s, v;
                            if (sscanf(&event->data[2], "%hhu %hhu %hhu", &h, &s, &v)) {
                                printf("Parse success!\n");
                                color_f3_t buf[PIXEL_GRID_WIDTH][PIXEL_GRID_HEIGHT];
                                for (int i = 0; i < PIXEL_GRID_HEIGHT; i++) {
                                    for (int k = 0; i < PIXEL_GRID_WIDTH; i++) {
                                        buf[k][i].h = ((float) h) / 255.0;
                                        buf[k][i].s = ((float) s) / 255.0;
                                        buf[k][i].v = ((float) v) / 255.0;
                                    }
                                }

                                lockPixels();
                                updatePixelData(buf);
                                unlockPixels();
                            }
                            break;
                        case 'f':
                            printf("Update fade\n");
                            uint8_t fade;
                            if (sscanf(&event->data[2], "%hhu", &fade)) {
                                printf("Parse success!\n");
                                lockPixels();
                                setFadeLength(fade);
                                unlockPixels();
                            }
                            break;
                    }
                }

                break;
            }
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}

static esp_err_t event_handler(void *ctx, system_event_t *event) {
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "got ip:%s",
                    ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void networking_task(void *pvParameter) {

    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    memcpy(&wifi_config.sta.ssid, "HackheimWifi", 12);
    memcpy(&wifi_config.sta.password, "TeletextFTW", 11);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.uri = "mqtt://10.9.8.60";
    mqtt_cfg.event_handle = mqtt_event_handler;
    mqtt_cfg.username = "big_hackheim_sign";
    mqtt_cfg.password = "DdX2LfRxpjwezmcw";
    mqtt_cfg.client_id = "big_hackheim_sign";

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);

    while (true) {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void init_networking_task() {
    xTaskCreate(&networking_task, "networking_task", 8000, NULL, 5, NULL);
}
