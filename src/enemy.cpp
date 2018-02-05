/*
 * enemy.cpp
 *
 *  Created on: 12 янв. 2018 г.
 *      Author: Shaft
 */

#include "headers/enemy.h"
#include <math.h>
#include <iostream>
#include "headers/pathfinders.h"
#include "headers/game_constants.h"
#include "headers/game_functions.hxx"

std::vector<CHL::instance*> non_material_quads;
std::vector<CHL::life_form*> entities;
std::vector<CHL::instance*> bricks;
std::vector<bullet*> bullets;
resource_manager manager;

void stall(enemy* e);
void chase(enemy* e);
void smart_move(enemy* e);

enemy::enemy(float x, float y, float z, int _speed, int s)
    : life_form(x, y, z, _speed, s) {
    // TODO Auto-generated constructor stub
    health = 3;
    state = smart_move;
    step_dest.x = position.x;
    step_dest.y = position.y;

    collision_box.x = TILE_SIZE / 2.0f + 2.0f;
    collision_box.y -= 2;

    fire_source = CHL::create_new_source(manager.get_sound("shot_sound"), this);
    steps_source =
        CHL::create_new_source(manager.get_sound("move_sound"), this);
    CHL::pitch_s(steps_source, 1 + (rand() % 100 - 50) / 300.0f);

    visor_light =
        new CHL::light(5.0f, CHL::point(0, 0), CHL::vec3(0.9, 0.0f, 0.1f));
    light_offset = CHL::point(0, 0);

    frames_in_texture = 12;
    frames_in_animation = 12;
    tilesets_in_texture = 5;
}

enemy::~enemy() {
    // TODO Auto-generated destructor stub
    std::cerr << "Enemy crushed!" << std::endl;
    if (moving)
        CHL::stop_s(steps_source);
}

void pathfind(enemy* e) {
    /* pathfinding test */
    int o_buff[x_size * y_size];
    int s = AStarFindPath(
        e->position.x / TILE_SIZE, (e->position.y - 0.05f) / TILE_SIZE,
        e->destination.x / TILE_SIZE, (e->destination.y) / TILE_SIZE, e->map,
        x_size, y_size, o_buff, x_size * y_size);
    //    std::cout << e->position.x / TILE_SIZE << " " << e->position.y /
    //    TILE_SIZE
    //              << std::endl;
    //    std::cout << e->destination.x / TILE_SIZE << " "
    //              << e->destination.y / TILE_SIZE << std::endl;

    if (s >= 1) {
        e->step_dest.y = (o_buff[0] / x_size) * TILE_SIZE - 2 + TILE_SIZE;
        e->step_dest.x =
            (o_buff[0] - x_size * (o_buff[0] / x_size)) * TILE_SIZE;
    }
    //    for (int y = 0; y < y_size; y++) {
    //        for (int x = 0; x < x_size; x++)
    //            std::cout << e->map[y * x_size + x] << " ";
    //        std::cout << std::endl;
    //    }
    //    for (int i = 0; i < s; i++)
    //        std::cout << (o_buff[i] / x_size) << " "
    //                  << o_buff[i] - (o_buff[i] / x_size) * x_size <<
    //                  std::endl;
    //    ;
}

float change_sprite(enemy* e) {
    float a = CHL::get_direction(e->step_dest.x, e->step_dest.y, e->position.x,
                                 e->position.y);

    if (a > (3 * M_PI_2 + M_PI_4) || a <= M_PI_4) {
        e->light_offset.x = 2;
        e->light_offset.y = 0;
        if (!e->moving) {
            e->selected_frame = 3;
        } else {
            e->light_offset.x = 4;
            e->selected_tileset = 1;
        }
    } else if (a > M_PI_4 && a <= M_PI_2 + M_PI_4) {
        e->light_offset.x = 0;
        e->light_offset.y = 0;
        if (!e->moving)
            e->selected_frame = 0;
        else {
            e->light_offset.y = -2;
            e->light_offset.x = 2;
            e->selected_tileset = 4;
        }
    } else if (a > M_PI_2 + M_PI_4 && a < M_PI + M_PI_4) {
        e->light_offset.x = -2;
        e->light_offset.y = 0;
        if (!e->moving)
            e->selected_frame = 2;
        else {
            e->light_offset.x = 0;
            e->selected_tileset = 2;
        }
    } else {
        e->light_offset.x = 0;
        e->light_offset.y = 0;
        if (!e->moving)
            e->selected_frame = 1;
        else {
            e->light_offset.y = -2;
            e->light_offset.x = 2;
            e->selected_tileset = 3;
        }
    }
    return a;
}

void stall(enemy* e) {
    e->step_dest.x = e->destination.x;
    e->step_dest.y = e->destination.y + 8;
    if (CHL::get_distance(e->destination.x, e->destination.y, e->position.x,
                          e->position.y) > e->size.x * 5 ||
        !CHL::ray_cast(e, e->destination, bricks)) {
        e->step_dest.x = e->position.x;
        e->step_dest.y = e->position.y;
        e->state = smart_move;
    }
    float a = change_sprite(e);
    e->shooting_alpha = a;
    e->fire();
}

void chase(enemy* e) {
    e->step_dest.x = e->destination.x;
    e->step_dest.y = e->destination.y;
    float path = e->speed * e->delta_time;
    float a = change_sprite(e);
    e->shooting_alpha = a;
    e->fire();

    e->delta_x = path * std::cos(a);
    e->delta_y = -path * std::sin(a);

    e->position.y += e->delta_y;
    e->position.x += e->delta_x;

    if (!CHL::ray_cast(e, e->destination, bricks)) {
        e->step_dest.x = e->position.x;
        e->step_dest.y = e->position.y;
        e->state = smart_move;
    }
    if (CHL::get_distance(e->destination.x, e->destination.y, e->position.x,
                          e->position.y) < e->size.x * 4) {
        e->state = stall;
    }
}

void smart_move(enemy* e) {
    if (std::fabs(e->position.x - e->step_dest.x) < 3.0f &&
        std::fabs(e->position.y - e->step_dest.y) < 3.0f) {
        pathfind(e);
    }
    float path = e->speed * e->delta_time;
    float a = change_sprite(e);
    e->shooting_alpha = a;

    e->delta_x = path * std::cos(a);
    e->delta_y = -path * std::sin(a);

    e->position.y += e->delta_y;
    e->position.x += e->delta_x;

    if (CHL::ray_cast(e, e->destination, bricks)) {
        e->state = chase;
    }
}

void enemy::move(float dt) {
    delta_time = dt;
    delta_x = 0;
    delta_y = 0;

    state(this);

    if ((delta_x != 0 || delta_y != 0) && !moving) {
        moving = true;
        loop_animation(0.04f);
        CHL::play_always_s(steps_source);
    }

    if (delta_x == 0 && delta_y == 0 && moving) {
        moving = false;
        loop_animation(0.0f);
        selected_tileset = 0;
        CHL::stop_s(steps_source);
    }

    CHL::set_pos_s(steps_source, CHL::vec3(position.x, position.y, 0.0f));
    float gain = 0.75 * CHL::calculate_gain(
                            CHL::gain_algorithm::linear_distance, steps_source);

    CHL::set_volume_s(steps_source, gain);

    update_points();
    update();
    position.z_index = position.y;
    visor_light->position.x = position.x + 6 + light_offset.x;
    visor_light->position.y = position.y - 6 + light_offset.y;
    if (shoot_delay > 0)
        shoot_delay -= dt;
}

void enemy::fire() {
    if (shoot_delay <= 0.0f) {
        shoot_delay = 1;
        shooting_point = calculate_shooting_point(this, shooting_alpha);

        bullets.insert(bullets.end(), new bullet(position.x + shooting_point.x,
                                                 position.y + shooting_point.y,
                                                 0.0f, 4, 2, 0, 2));
        (*(bullets.end() - 1))->alpha = calculate_alpha_precision(
            shooting_alpha + (rand() % 400 - 200) / 1500.0f);
        (*(bullets.end() - 1))->speed = B_SPEED;
        (*(bullets.end() - 1))->rotation_point =
            CHL::point(position.x + TILE_SIZE / 2, position.y - TILE_SIZE / 2);
        (*(bullets.end() - 1))->creator = bullet_creator::enemy;

        CHL::set_pos_s(fire_source, CHL::vec3(position.x, position.y, 0.0f));
        float gain = CHL::calculate_gain(CHL::gain_algorithm::linear_distance,
                                         fire_source);

        CHL::set_volume_s(fire_source, gain);
        CHL::pitch_s(fire_source, 1 + (rand() % 100 - 50) / 200.0f);
        CHL::play_s(fire_source);
    }
}
