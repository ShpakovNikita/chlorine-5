#include "include/special_effect.h"

special_effect::special_effect(float x, float y, float z, int _size)
    : instance(x, y, z, _size) {
    // TODO something nice
    delay = 60 / fps;
}

special_effect::~special_effect() {
    // No code required here
}

void special_effect::update_frame() {
    update_data();
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
