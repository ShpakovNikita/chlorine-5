/*
 * display.h
 *
 *  Created on: 5 февр. 2018 г.
 *      Author: Shaft
 */

#ifndef HEADERS_DISPLAY_H_
#define HEADERS_DISPLAY_H_

#include "engine.hxx"

namespace display {

void render_screen(CHL::engine* eng,
                   CHL::texture* tex,
                   CHL::sound* snd /*nullptr for not playing anything*/,
                   float x,
                   float y,
                   const std::string& text,
                   CHL::font* f,
                   const CHL::vec3& color);
}

#endif /* HEADERS_DISPLAY_H_ */
