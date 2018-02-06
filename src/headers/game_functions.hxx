/*
 * Functions that are very helpful in some moments
 */
#pragma once

#include <math.h>
#include "engine.hxx"

/*calculate a precision for better shooting if rotating object relative position
 * equals bottom left corner*/
inline float calculate_alpha_precision(float a) {
    if (a > (3 * M_PI_2 + M_PI_4) || a <= M_PI_4) {
        a -= 0.02f;
    } else if (a > M_PI_4 && a <= M_PI_2 + M_PI_4) {
        a += 0.0f;
    } else if (a > M_PI_2 + M_PI_4 && a < M_PI + M_PI_4) {
        a += 0.02f;
    } else {
        a -= 0.0f;
    }
    return a;
}

/*function for calculation shooting oval*/
inline CHL::point calculate_shooting_point(CHL::instance* e, float a) {
    float x = e->collision_box.x / 2.0f, y = -e->collision_box.y / 2.0f;
    x += cos(a) * e->collision_box.x / 2.0f;
    y -= sin(a) * e->collision_box.y / 2.0f;
    return CHL::point(x, y);
}

/*helpful sign function*/
template <typename T>
inline int sign(T val) {
    return (T(0) < val) - (val < T(0));
}

/*helpful precise function*/
inline float precise(float value, float precision) {
    return (int)(value / precision) * precision;
}
