/*
 * special_effect.cpp
 *
 *  Created on: 13 янв. 2018 г.
 *      Author: Shaft
 */

#include "headers/special_effect.h"

special_effect::special_effect(float x, float y, float z, int size)
    : instance(x, y, z, size) {
    // TODO Auto-generated constructor stub
    delay = 60 / fps;
}

special_effect::~special_effect() {
    // TODO Auto-generated destructor stub
}

void special_effect::update_frame() {
    if (!is_end) {
        delay -= 1;
        if (!delay) {
            delay = 60 / fps;
            selected_frame += 1;
            if (selected_frame == frames_in_texture - 1)
                is_end = true;
        }
    }
}

bool special_effect::end() {
    return is_end;
}
