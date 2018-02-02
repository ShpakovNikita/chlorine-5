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

    fire_source = CHL::create_new_source(manager.get_sound("shot_sound"), this);
    steps_source =
        CHL::create_new_source(manager.get_sound("move_sound"), this);
    //    CHL::play_always_s(steps_source);
}

enemy::~enemy() {
    // TODO Auto-generated destructor stub
    std::cerr << "Enemy crushed!" << std::endl;
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
        e->step_dest.y = (o_buff[0] / x_size) * TILE_SIZE - 4 + TILE_SIZE;
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
        e->shooting_point = CHL::point(15, -8);
        e->selected_frame = 0;
    } else if (a > M_PI_4 && a <= M_PI_2 + M_PI_4) {
        e->shooting_point = CHL::point(e->size.x / 2 + 4, -e->size.x);
        e->selected_frame = 3;
    } else if (a > M_PI_2 + M_PI_4 && a < M_PI + M_PI_4) {
        e->shooting_point = CHL::point(1, -16);
        e->selected_frame = 1;
    } else {
        e->shooting_point = CHL::point(e->size.x / 2 - 4, -4);
        e->selected_frame = 2;
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
    if (std::fabs(e->position.x - e->step_dest.x) < 1.0f &&
        std::fabs(e->position.y - e->step_dest.y) < 1.0f) {
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

    state(this);
    update_points();
    if (shoot_delay > 0)
        shoot_delay -= dt;
}

void enemy::fire() {
    if (shoot_delay <= 0.0f) {
        shoot_delay = 1;
        bullets.insert(bullets.end(), new bullet(position.x + shooting_point.x,
                                                 position.y + shooting_point.y,
                                                 0.0f, 4, 2, 0, 2));
        (*(bullets.end() - 1))->alpha =
            calculate_alpha_precision(shooting_alpha);
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
