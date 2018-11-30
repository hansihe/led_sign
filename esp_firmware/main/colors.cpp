#include "math.h"
#include "colors.h"

// Corresponds roughly to RGB brighter/darker
#define LAB_Kn 18

// D65 standard referent
#define LAB_Xn 0.950470
#define LAB_Yn 1.0
#define LAB_Zn 1.088830

#define LAB_t0 0.137931034 // 4 / 29
#define LAB_t1 0.206896552 // 6 / 29
#define LAB_t2 0.12841855 // 3 * t1 * t1
#define LAB_t3 0.008856452 // t1 * t1 * t1

#define LAB_XYZ(t) (((t) > LAB_t1) ? ((t)*(t)*(t)) : (LAB_t2 * ((t) - LAB_t0)))
#define XYZ_RGB(r) (255.0 * (((r) <= 0.00304) ? (12.92 * (r)) : (1.055 * powf((r), 1 / 2.4) - 0.055)))

void lab_to_rgb(float l, float a, float b, float *out_r, float *out_g, float *out_b) {
    float x, y, z;

    y = (l + 16.0) / 116.0;
    x = isnan(a) ? y : (y + (a / 500.0));
    z = isnan(b) ? y : (y - (b / 200.0));

    y = LAB_Yn * LAB_XYZ(y);
    x = LAB_Xn * LAB_XYZ(x);
    z = LAB_Zn * LAB_XYZ(z);

    *out_r = XYZ_RGB((3.2404542*x) - (1.5371385*y) - (0.4985314*z));
    *out_g = XYZ_RGB((-0.9692660*x) + (1.8760108*y) + (0.0415560*z));
    *out_b = XYZ_RGB((0.0556434*x) - (0.2040259*y) + (1.0572252*z));
}

void lch_to_rgb(float l, float c, float h, float *out_r, float *out_g, float *out_b) {
    lab_to_rgb(l, cosf(h)*c, sinf(h)*c, out_r, out_g, out_b);
}

const float ONESIXTH = 1.0 / 6.0;

void hsv_to_rgb(color_f3_t in, color_f3_t *out) {
    float fC = in.v * in.s; // Chroma
    float fHPrime = fmod(in.h / ONESIXTH, 6);
    float fX = fC * (1 - fabs(fmod(fHPrime, 2) - 1));
    float fM = in.v - fC;

    float r, g, b;

    if(0 <= fHPrime && fHPrime < 1) {
        r = fC;
        g = fX;
        b = 0;
    } else if(1 <= fHPrime && fHPrime < 2) {
        r = fX;
        g = fC;
        b = 0;
    } else if(2 <= fHPrime && fHPrime < 3) {
        r = 0;
        g = fC;
        b = fX;
    } else if(3 <= fHPrime && fHPrime < 4) {
        r = 0;
        g = fX;
        b = fC;
    } else if(4 <= fHPrime && fHPrime < 5) {
        r = fX;
        g = 0;
        b = fC;
    } else if(5 <= fHPrime && fHPrime < 6) {
        r = fC;
        g = 0;
        b = fX;
    } else {
        r = 0;
        g = 0;
        b = 0;
    }

    r += fM;
    g += fM;
    b += fM;

    out->r = r;
    out->g = g;
    out->b = b;
}

