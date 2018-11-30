#include "stdint.h"
#include "math.h"
#include <cstdio>

static inline float __attribute__((always_inline)) hue_linear_interpolate(
        float s0, float s1, float f) {

    float difference = fabs(s1 - s0);
    if (difference > 0.5) {
        if (s1 > s0) {
            s0 += 1.0;
        } else {
            s1 += 1.0;
        }
    }

    float value = s0 + ((s1 - s0) * f);
    
    if (value >= 0.0 && value <= 1.0) {
        return value;
    }
    return fmod(value, 1.0);

}

static inline float __attribute__((always_inline)) hue_bilinear_interpolate(
        float s00, float s01, float s10, float s11, float xf, float yf) {
    float xi1 = hue_linear_interpolate(s00, s10, xf);
    float xi2 = hue_linear_interpolate(s01, s11, xf);
    return hue_linear_interpolate(xi1, xi2, yf);
}

int main() {
    float res = hue_bilinear_interpolate(0.5, 0.0, 0.0, 0.0, 0.1, 0.1);
    printf("out: %.4f\n", res);
    return 0;
}
