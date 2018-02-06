/*
 * Render helper functions.
 */

#pragma once

#include "engine.hxx"

namespace display {

void render_screen(
    CHL::engine* eng,
    CHL::texture* tex,
    CHL::sound* snd /*nullptr for not playing anything*/,
    float x,
    float y,
    const std::string& text,
    CHL::font* font,
    const CHL::vec3& color,
    const CHL::event quit_event /*pressing this button will break the loop*/,
    int WINDOW_WIDTH,
    int WINDOW_HEIGHT);
}
