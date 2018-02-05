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
                   float time,
                   CHL::sound* snd /*nullptr for not playing anything*/);
}

#endif /* HEADERS_DISPLAY_H_ */
