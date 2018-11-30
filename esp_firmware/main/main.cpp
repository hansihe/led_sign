/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "display_data_out_task.h"
#include "render_task.h"
#include "networking_task.h"
//#include "micropython_task.h"

extern "C" void app_main() {
    printf("Hello world!\n");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_display_data_out_task();
    init_render_task();
    //init_micropython_task();
    init_networking_task();

    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

