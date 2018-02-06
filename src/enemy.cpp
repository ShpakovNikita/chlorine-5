#include "headers/enemy.h"
#include <math.h>
#include <iostream>
#include "headers/pathfinders.h"
#include "headers/game_functions.hxx"
#include "headers/global_data.h"

std::vector<CHL::instance*> non_material_quads;
std::vector<CHL::life_form*> entities;
std::vector<CHL::instance*> bricks;
std::vector<bullet*> bullets;
resource_manager manager;
static float SHOOT_DELAY = 1.0f;
static float DELTA_FIND = 1.0f;
static uint32_t HEALTH = 3;

void stall(enemy* e, float dt);
void chase(enemy* e, float dt);
void smart_move(enemy* e, float dt);

enemy::enemy(float x, float y, float z, int _speed, int s)
    : life_form(x, y, z, _speed, s) {
    health = HEALTH;

    /*starting with the pathfind*/
    state = smart_move;

    collision_box.x = TILE_SIZE / 2.0f + 2.0f;
    collision_box.y -= 2.0f;

    fire_source = CHL::create_new_source(manager.get_sound("shot_sound"), this);
    steps_source =
        CHL::create_new_source(manager.get_sound("move_sound"), this);
    CHL::pitch_s(steps_source,
                 1 + (rand() % 100 - 50) /
                         300.0f);    // a bit of randomness in the steps

    visor_light =
        new CHL::light(5.0f, CHL::point(0, 0), CHL::vec3(0.9, 0.0f, 0.1f));
    light_offset = CHL::point(0, 0);

    /*setup texture params*/
    frames_in_texture = 12;
    frames_in_animation = 12;
    tilesets_in_texture = 5;
}

enemy::enemy(float x, float y, float z, int _speed, int _size_x, int _size_y)
    : enemy(x, y, z, _speed, 0) {
    size = CHL::point(x, y);
}

enemy::~enemy() {
    std::cerr << "Enemy crushed!" << std::endl;
    if (moving)
        CHL::stop_s(steps_source);
    CHL::delete_source(steps_source);

    CHL::stop_s(fire_source);
    CHL::delete_source(fire_source);

    state = NULL;
    delete visor_light;
}

void pathfind(enemy* e) {
    int o_buff[x_size * y_size];
    int s = AStarFindPath(
        e->position.x / TILE_SIZE, (e->position.y - 0.05f) / TILE_SIZE,
        e->destination.x / TILE_SIZE, (e->destination.y) / TILE_SIZE, e->map,
        x_size, y_size, o_buff, x_size * y_size);

    /*if path length > 1*/
    if (s >= 1) {
        e->step_dest.y = (o_buff[0] / x_size) * TILE_SIZE - 2 + TILE_SIZE;
        e->step_dest.x =
            (o_buff[0] - x_size * (o_buff[0] / x_size)) * TILE_SIZE;
    } else {
        e->step_dest.y = e->position.y;
        e->step_dest.x = e->position.x;
    }
}

float change_sprite(enemy* e) {
    float a = CHL::get_direction(e->step_dest.x, e->step_dest.y, e->position.x,
                                 e->position.y);

    /*changing selected tileset for animations or frame for stall state
     * depending on current direction. Also a bit of change for the light
     * offset, to fit light perfectly on enemy's visor.*/

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

void stall(enemy* e, float dt) {
    e->step_dest.x = e->destination.x;
    e->step_dest.y = e->destination.y;

    /*if player if far or isn't visible for enemy, begin pathfinding*/
    if (CHL::get_distance(e->destination.x, e->destination.y, e->position.x,
                          e->position.y) > TILE_SIZE * 5 ||
        !CHL::ray_cast(e, e->destination, bricks)) {
        e->step_dest.x = e->position.x;
        e->step_dest.y = e->position.y;
        e->state = smart_move;
    }
    float a = change_sprite(e);
    e->shooting_alpha = a;
    e->fire();
}

void chase(enemy* e, float dt) {
    /*current step destination become a player's position*/
    e->step_dest.x = e->destination.x;
    e->step_dest.y = e->destination.y;

    float path = e->speed * dt;
    float a = change_sprite(e);
    e->shooting_alpha = a;
    e->fire();

    e->delta_x = path * std::cos(a);
    e->delta_y = -path * std::sin(a);

    e->position.y += e->delta_y;
    e->position.x += e->delta_x;

    /*if player is not visible, begin pathfind*/
    if (!CHL::ray_cast(e, e->destination, bricks)) {
        e->step_dest.x = e->position.x;
        e->step_dest.y = e->position.y;
        e->state = smart_move;
    }
    /*if player is close, then stop and fire*/
    if (CHL::get_distance(e->destination.x, e->destination.y, e->position.x,
                          e->position.y) < e->size.x * 4) {
        e->state = stall;
    }
}

void smart_move(enemy* e, float dt) {
    /*if we are close to the destination point, find a new one via pathfinding*/
    if ((std::fabs(e->position.x - e->step_dest.x) < 3.0f &&
         std::fabs(e->position.y - e->step_dest.y) < 3.0f) ||
        e->delta_find <= 0) {
        e->delta_find = DELTA_FIND;
        pathfind(e);
    }
    float path = e->speed * dt;
    float a = change_sprite(e);
    e->shooting_alpha = a;

    e->delta_x = path * std::cos(a);
    e->delta_y = -path * std::sin(a);

    e->position.y += e->delta_y;
    e->position.x += e->delta_x;

    /*stop thinking, begin chasing the player*/
    if (CHL::ray_cast(e, e->destination, bricks)) {
        e->state = chase;
    }
}

void do_actions(enemy* e, float dt) {
    if (e->shoot_delay > 0)
        e->shoot_delay -= dt;
    if (e->delta_find > 0)
        e->delta_find -= dt;
}

void enemy::move(float dt) {
    delta_x = 0;
    delta_y = 0;

    /*execute current node in the decision tree*/
    state(this, dt);

    /*one time smart sound on*/
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

    /*update the position of the sound source and calculate gain of the sound
     * using the linear model*/
    CHL::set_pos_s(steps_source, CHL::vec3(position.x, position.y, 0.0f));
    float gain = 0.75 * CHL::calculate_gain(
                            CHL::gain_algorithm::linear_distance, steps_source);

    CHL::set_volume_s(steps_source, gain);

    /*textures and points update*/
    update_points();
    update();
    position.z_index = position.y;
    visor_light->position.x = position.x + 6 + light_offset.x;
    visor_light->position.y = position.y - 6 + light_offset.y;

    do_actions(this, dt);
}

void enemy::fire() {
    /*if reloaded, then shoot*/
    if (shoot_delay <= 0.0f) {
        shoot_delay = SHOOT_DELAY;
        shooting_point = calculate_shooting_point(this, shooting_alpha);

        /*creating a new bullet*/
        bullets.insert(bullets.end(), new bullet(position.x + shooting_point.x,
                                                 position.y + shooting_point.y,
                                                 0.0f, 4, 2, 0, 2));
        auto last_bullet = bullets.end() - 1;    // getting the iterator
        (*last_bullet)->alpha = calculate_alpha_precision(
            shooting_alpha +
            (rand() % 400 - 200) / 1500.0f);    // a bit of randomness in
                                                // shooting and calculating the
                                                // angle precision
        (*last_bullet)->speed = B_SPEED;
        (*last_bullet)->creator = bullet_creator::enemy;

        /* set the current pos as the shooting pos and calculate sound gain via
         * the linear model*/
        CHL::set_pos_s(fire_source, CHL::vec3(position.x, position.y, 0.0f));
        float gain = CHL::calculate_gain(CHL::gain_algorithm::linear_distance,
                                         fire_source);

        CHL::set_volume_s(fire_source, gain);
        CHL::pitch_s(fire_source, 1 + (rand() % 100 - 50) / 200.0f);
        CHL::play_s(fire_source);    // a bit of randomness in sound
    }
}
