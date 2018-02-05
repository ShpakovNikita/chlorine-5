/*
 * display.cpp
 *
 *  Created on: 5 февр. 2018 г.
 *      Author: Shaft
 */
#include "headers/display.h"
#include "headers/game_constants.h"
#include <thread>
#include <chrono>
#include <iostream>

namespace display {
void render_screen(CHL::engine* eng,
                   CHL::texture* tex,
                   float time,
                   CHL::sound* snd) {
    float prev_frame = eng->GL_time();
    CHL::user_interface* ui = new CHL::user_interface();
    CHL::ui_element* element = new CHL::ui_element(
        0, WINDOW_HEIGHT, MIN_DEPTH, WINDOW_WIDTH, WINDOW_HEIGHT, tex);
    ui->add_instance(element);
    eng->render_ui(ui);
    eng->GL_swap_buffers();
    snd->play();
    while (time > 0) {
        float delta_time = (eng->GL_time() - prev_frame);
        prev_frame = eng->GL_time();

        time -= delta_time;
        CHL::event e;

        while (eng->read_input(e)) {
            switch (e) {
                case CHL::event::turn_off:
                    time = 0;
                    break;
                default:
                    break;
            }
        }
        float t = (eng->GL_time() - prev_frame) * 1000;

        if (t < 1000 / FPS)
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1000 / FPS - (int)t));
    }
}
}    // namespace display
