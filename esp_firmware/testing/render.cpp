#include "stdio.h"
#include "string.h"
#include "math.h"
#include "stdint.h"

#include "../main/led_pos.h"
#include "../main/colors.cpp"

#define PIXEL_GRID_WIDTH 8
#define PIXEL_GRID_HEIGHT 8

#define STAGING_BUFFER_WIDTH 7
#define STAGING_BUFFER_HEIGHT 60

#define PI 3.1415926535897

int fadeLength = 0;

float pixelBuffer[2][PIXEL_GRID_WIDTH][PIXEL_GRID_HEIGHT][3];
uint8_t displayStagingBuffer[STAGING_BUFFER_WIDTH][STAGING_BUFFER_HEIGHT][3];

const float PIXEL_GRID_X_HALF_STEP = 0.5 / PIXEL_GRID_WIDTH;
const float PIXEL_GRID_Y_HALF_STEP = 0.5 / PIXEL_GRID_HEIGHT;

bool hasOngoingFade = false;
int ongoingFadeLength;
int ongoingFadeCurrent;

static inline uint8_t __attribute__((always_inline)) float_to_uint8(float in) {
    if (in < 0.0) return 0;
    if (in > 1.0) return 255;
    return (uint8_t) (in * 255);
}

void updatePixelData(float pixelData[PIXEL_GRID_WIDTH][PIXEL_GRID_HEIGHT][3]) {
    if (fadeLength == 0) {
        memcpy(pixelBuffer[0], pixelData, PIXEL_GRID_WIDTH * PIXEL_GRID_HEIGHT * 3);
    } else {
        hasOngoingFade = true;
        ongoingFadeLength = fadeLength;
        ongoingFadeCurrent = 0;

        for (int i = 0; i < PIXEL_GRID_WIDTH; i++) {
            for (int k = 0; k < PIXEL_GRID_HEIGHT; k++) {
                float *in = pixelData[i][k];
                float *current = pixelBuffer[0][i][k];
                float *out = pixelBuffer[1][i][k];

                float hDir1 = in[0] - current[0];
                float hDir2 = current[0] - in[0];
                float hDelta = (hDir1 < hDir2) ? -hDir1 : hDir2;
                out[0] = hDelta / fadeLength;

                float sDelta = in[1] - current[1];
                out[1] = sDelta / fadeLength;

                float vDelta = in[2] - current[2];
                out[2] = vDelta / fadeLength;
            }
        }
    }
}

void renderTaskPart() {
    for (int i = 0; i < PIXEL_GRID_WIDTH; i++) {
        for (int j = 0; j < PIXEL_GRID_HEIGHT; j++) {
            printf("%f %f %f\n", pixelBuffer[0][i][j][0], pixelBuffer[0][i][j][1], pixelBuffer[0][i][j][2]);
        }
    }

    if (hasOngoingFade) {

        ongoingFadeCurrent += 1;
        if (ongoingFadeCurrent >= ongoingFadeLength) {
            hasOngoingFade = false;
        }

        for (int i = 0; i < PIXEL_GRID_WIDTH; i++) {
            for (int k = 0; k < PIXEL_GRID_HEIGHT; k++) {
                float *current = pixelBuffer[0][i][k];
                float *step = pixelBuffer[1][i][k];

                current[0] = current[0] + step[0];
                if (current[0] < 0.0) current[0] += 2*PI;
                if (current[0] > 2*PI) current[0] -= 2*PI;

                current[1] += step[1];
                current[2] += step[2];
            }
        }
    }

    for (int i = 0; i < STAGING_BUFFER_WIDTH; i++) {
        for (int k = 0; k < STAGING_BUFFER_HEIGHT; k++) {
            led_pos_t phys_pos = ledPositions[i][k];
            if (phys_pos.posx == 0.0) {
                continue;
            }

            float offgridX = phys_pos.posx * (PIXEL_GRID_WIDTH - 1);
            float offgridY = phys_pos.posy * (PIXEL_GRID_HEIGHT - 1);
            int discreteX = floorf(offgridX);
            int discreteY = floorf(offgridY);
            printf("rel_led_pos: %f %f offgrid: %f %f discrete: %d %d\n", phys_pos.posx, phys_pos.posy, offgridX, offgridY, discreteX, discreteY);

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

            float hsvInterp[3];
            for (int c = 0; c<3; c++) {
                float s1 = pixelBuffer[0][x1p][y1p][c];
                float s2 = pixelBuffer[0][x1p][y2p][c];
                float s3 = pixelBuffer[0][x2p][y1p][c];
                float s4 = pixelBuffer[0][x2p][y2p][c];
                printf("component %d samples: %f %f %f %f\n", c, s1, s2, s3, s4);

                float sc1 = (s1 * xSub) + (s3 * xSubMirror);
                float sc2 = (s2 * xSub) + (s4 * xSubMirror);
                float sc = (sc1 * ySub) + (sc2 * ySubMirror);

                displayStagingBuffer[i][k][c] = float_to_uint8(sc);
                hsvInterp[c] = sc;
            }
            printf("%d %d: sub: %f %f colors: %f %f %f discrete: %d %d\n", x1p, y1p, xSub, ySub, hsvInterp[0], hsvInterp[1], hsvInterp[2], discreteX, discreteY);

            // TODO: Twoway hue interp

            float r, g, b;
            hsv_to_rgb(hsvInterp[0] / PI * 180.0, hsvInterp[1], hsvInterp[2], &r, &g, &b);

            displayStagingBuffer[i][k][0] = float_to_uint8(r);
            displayStagingBuffer[i][k][1] = float_to_uint8(g);
            displayStagingBuffer[i][k][2] = float_to_uint8(b);
        }

    }

}

int main() {
    float update[PIXEL_GRID_WIDTH][PIXEL_GRID_HEIGHT][3];
    for (int i = 0; i < PIXEL_GRID_WIDTH; i++) {
        for (int j = 0; j < PIXEL_GRID_HEIGHT; j++) {
            update[i][j][0] = 0.0;
            update[i][j][1] = 0.0;
            update[i][j][2] = 0.0;
        }
    }
    update[0][0][0] = 1.2;
    update[0][0][1] = 0.8;
    update[0][0][2] = 0.8;

    update[1][1][0] = 1.2;
    update[1][1][1] = 0.8;
    update[1][1][2] = 0.8;

    updatePixelData(update);

    printf("%f\n", pixelBuffer[0][0][0][0]);

    renderTaskPart();

    printf("%d\n", *displayStagingBuffer[0][0]);

    return 0;
}
