/*
 * game_functions.hxx
 *
 *  Created on: 19 янв. 2018 г.
 *      Author: Shaft
 */

#ifndef HEADERS_GAME_FUNCTIONS_HXX_
#define HEADERS_GAME_FUNCTIONS_HXX_

#include <math.h>
#include "engine.hxx"

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

template <typename T>
inline int sign(T val) {
    return (T(0) < val) - (val < T(0));
}

inline float precise(float value, float precision) {
    return (int)(value / precision) * precision;
}

inline void render(CHL::engine* eng) {
    //    eng->GL_clear_color();
    //
    //    for (auto tile : floor)
    //        eng->add_object(tile);
    //
    //    if (!floor.empty())
    //        eng->draw(manager.get_texture("floor"));
    //
    //    for (auto brick : bricks)
    //        eng->add_object(brick);
    //
    //    if (!bricks.empty())
    //        eng->draw(manager.get_texture("brick"));
    //
    //    for (auto bullet : bullets) {
    //        bullet->move(delta_time);
    //        eng->add_object(bullet);
    //    }
    //
    //    if (!bullets.empty())
    //        eng->draw(manager.get_texture("bullet"));
    //
    //    eng->add_object(hero);
    //    eng->draw(manager.get_texture("player"));
    //
    //    for (auto e : entities) {
    //        if (dynamic_cast<enemy*>(e) != nullptr)
    //            eng->add_object(e);
    //    }
    //    if (!entities.empty())
    //        eng->draw(manager.get_texture("player"));
    //
    //    for (auto effect : se) {
    //        effect->update_frame();
    //        eng->add_object(effect);
    //    }
    //
    //    if (!se.empty())
    //        eng->draw(manager.get_texture("explosion"));
    //
    //    animated_block->update();
    //    eng->add_object(animated_block);
    //    eng->draw(manager.get_texture("obelisk"));
    //
    //    eng->GL_swap_buffers();
}

#endif /* HEADERS_GAME_FUNCTIONS_HXX_ */
