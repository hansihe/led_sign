#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "display_data_out_task.h"
#include "render_task.h"
#include "colors.h"
#include "math.h"
#include <string.h>

#include "led_pos.h"

#define PI 3.1415926535897

int fadeLength = 80;

color_f3_t pixelColorBuffer[PIXEL_GRID_WIDTH][PIXEL_GRID_HEIGHT];
color_f3_t pixelAuxBuffer[PIXEL_GRID_WIDTH][PIXEL_GRID_HEIGHT];

bool hasOngoingFade = false;
int ongoingFadeLength;
int ongoingFadeCurrent;

SemaphoreHandle_t settingsUpdateMutex;

void lockPixels() {
    xSemaphoreTake(settingsUpdateMutex, portMAX_DELAY);
}
void unlockPixels() {
    xSemaphoreGive(settingsUpdateMutex);
}

void setFadeLength(int fadeLengthN) {
    fadeLength = fadeLengthN;
}

static inline uint8_t __attribute__((always_inline)) float_to_uint8(float in) {
    if (in < 0.0) return 0;
    if (in > 1.0) return 255;
    return (uint8_t) (in * 255);
}

void updatePixelData(color_f3_t pixelData[PIXEL_GRID_WIDTH][PIXEL_GRID_HEIGHT]) {
    if (fadeLength == 0) {
        //memcpy(pixelColorBuffer, pixelData, PIXEL_GRID_WIDTH * PIXEL_GRID_HEIGHT * 3);
        for (int i = 0; i < PIXEL_GRID_WIDTH; i++) {
            for (int k = 0; k < PIXEL_GRID_HEIGHT; k++) {
                pixelColorBuffer[i][k] = pixelData[i][k];
            }
        }
    } else {
        hasOngoingFade = true;
        ongoingFadeLength = fadeLength;
        ongoingFadeCurrent = 0;

        for (int i = 0; i < PIXEL_GRID_WIDTH; i++) {
            for (int k = 0; k < PIXEL_GRID_HEIGHT; k++) {
                color_f3_t *in = &pixelData[i][k];
                color_f3_t *color = &pixelColorBuffer[i][k];
                color_f3_t *delta = &pixelAuxBuffer[i][k];

                float hDelta = hue_linear_interpolate_dir(color->h, in->h);
                delta->h = hDelta / fadeLength;

                float sDelta = in->s - color->s;
                delta->s = sDelta / fadeLength;

                float vDelta = in->v - color->v;
                delta->v = vDelta / fadeLength;
            }
        }
    }
}

static void render_task(void *pvParameter) {
    vTaskDelay(1 / portTICK_PERIOD_MS);

    float rot = 0.0;

    //for (int i = 0; i < 8; i++) {
    //    for (int k = 0; i < 60; i++) {
    //        displayStagingBuffer[i][k][1] = 100;
    //    }
    //}
    
    float pos = 0.5;
    int dir = 1;

    bool time_mult_state = 0;

    while (true) {

        wait_blit_ready();
        lockPixels();

        if (hasOngoingFade) {

            ongoingFadeCurrent += 1;
            if (ongoingFadeCurrent >= ongoingFadeLength) {
                hasOngoingFade = false;
            }
            
            for (int i = 0; i < PIXEL_GRID_WIDTH; i++) {
                for (int k = 0; k < PIXEL_GRID_HEIGHT; k++) {
                    color_f3_t *current = &pixelColorBuffer[i][k];
                    color_f3_t *delta = &pixelAuxBuffer[i][k];

                    current->h = current->h + delta->h;
                    if (current->h < 0.0) current->h += 1.0;
                    else if (current->h > 1.0) current->h -= 1.0;

                    current->s += delta->s;
                    current->v += delta->v;
                }
            }
        }

        //pixelColorBuffer[6][0][0] = 1.0;
        //pixelColorBuffer[6][0][1] = 1.0;
        //pixelColorBuffer[6][0][2] = 1.0;

        for (int i = 0; i < 7; i++) {
            for (int k = 0; k < 60; k++) {
                led_pos_t phys_pos = ledPositions[i][k];
                if (phys_pos.posx == 0.0) {
                    continue;
                }

                float offgridX = phys_pos.posx * (PIXEL_GRID_WIDTH - 1);
                float offgridY = phys_pos.posy * (PIXEL_GRID_HEIGHT - 1);
                int discreteX = floorf(offgridX);
                int discreteY = floorf(offgridY);

                float xSub = fmod(offgridX, 1.0f);
                if (xSub < 0) xSub += 1;
                float xSubMirror = 1.0f - xSub;

                float ySub = fmod(offgridY, 1.0f);
                if (ySub < 0) ySub += 1;
                float ySubMirror = 1.0f - ySub;

                int x1p = discreteX;
                if (x1p < 0) x1p = 0;

                int x2p = discreteX + 1;
                if (x2p >= PIXEL_GRID_WIDTH) x2p = PIXEL_GRID_WIDTH - 1;

                int y1p = discreteY;
                if (y1p < 0) y1p = 0;

                int y2p = discreteY + 1;
                if (y2p >= PIXEL_GRID_HEIGHT) y2p = PIXEL_GRID_HEIGHT - 1;


                color_f3_t *s00 = &pixelColorBuffer[x1p][y1p];
                color_f3_t *s10 = &pixelColorBuffer[x2p][y1p];
                color_f3_t *s01 = &pixelColorBuffer[x1p][y2p];
                color_f3_t *s11 = &pixelColorBuffer[x2p][y2p];

                // Interpolate saturation and value linearly
                color_f3_t interp;
                interp.h = hue_bilinear_interpolate(s11->h, s01->h, s10->h, s00->h, xSub, ySub);
                interp.s = (s11->s*xSub + s01->s*xSubMirror)*ySub 
                    + (s10->s*xSub + s00->s*xSubMirror)*ySubMirror;
                interp.v = (s11->v*xSub + s01->v*xSubMirror)*ySub 
                    + (s10->v*xSub + s00->v*xSubMirror)*ySubMirror;

                // Hue needs a special case, don't factor a sample
                // if it doesn't have saturation and value set.
                //float hInterp = 0.0;
                //bool s00HValid = (s00.s * s00.v) > 0.0001;
                //bool s10HValid = (s10.s * s10.v) > 0.0001;
                //bool s01HValid = (s01.s * s01.v) > 0.0001;
                //bool s11HValid = (s11.s * s11.v) > 0.0001;
                //int hValidPopCnt = s00 + s01 + s10 + s11;
                //if (hValidPopCnt > 0) {
                //    float mult = 4.0 / hValidPopCnt;
                //    float h00 = s00HValid ? (s00.h * mult) : 0.0;
                //    float h01 = s01HValid ? (s01.h * mult) : 0.0;
                //    float h10 = s01HValid ? (s10.h * mult) : 0.0;
                //    float h11 = s11HValid ? (s11.h * mult) : 0.0;
                //    hInterp = (h11*xSub + h01*xSubMirror)*ySub
                //        + (h10*xSub + h00*xSubMirror)*ySubMirror;
                //}

                color_f3_t rgb;
                hsv_to_rgb(interp, &rgb);

                rgb.g *= 0.8;

                if (time_mult_state) {
                    displayStagingBuffer[i][k][0] = float_to_uint8(rgb.g);
                    displayStagingBuffer[i][k][1] = float_to_uint8(rgb.r);
                    displayStagingBuffer[i][k][2] = float_to_uint8(rgb.b);
                } else {
                    displayStagingBuffer[i][k][0] = float_to_uint8(rgb.g-(0.5/255.0));
                    displayStagingBuffer[i][k][1] = float_to_uint8(rgb.r-(0.5/255.0));
                    displayStagingBuffer[i][k][2] = float_to_uint8(rgb.b-(0.5/255.0));
                }
                time_mult_state = !time_mult_state;
            }

        }

        //vTaskDelay(1 / portTICK_PERIOD_MS);

        //if (pos < 0.0) {
        //    dir = 1;
        //}
        //if (pos > 1.0) {
        //    dir = -1;
        //}
        //pos += dir * 0.001;

        //for (int i = 0; i < 7; i++) {
        //    for (int k = 0; k < 60; k++) {
        //        led_pos_t phys_pos = ledPositions[i][k];
        //        if (phys_pos.posx == 0.0) {
        //            continue;
        //        }

        //        float proximity = fabs((phys_pos.posx - pos) * 2);
        //        displayStagingBuffer[i][k][1] = float_to_uint8(proximity);
        //    }
        //}

        //rot += 0.2;

        //float r, g, b;
        //hsv_to_rgb(rot, 1.0, 1.0, &r, &g, &b);

        //for (int i = 0; i < 7; i++) {
        //    for (int k = 0; k < 60; k++) {
        //            displayStagingBuffer[i][k][0] = (uint8_t)(r * 255.0);
        //            displayStagingBuffer[i][k][1] = (uint8_t)(g * 255.0);
        //            displayStagingBuffer[i][k][2] = (uint8_t)(b * 255.0);
        //    }
        //}

        unlockPixels();
        notify_start_blit();

        //vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void init_render_task() {
    settingsUpdateMutex = xSemaphoreCreateMutex();

    xTaskCreate(&render_task, "render_task", 8000, NULL, 5, NULL);
}
