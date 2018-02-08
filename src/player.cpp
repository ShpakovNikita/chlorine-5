#include "include/player.h"

#include <math.h>
#include <iostream>
#include "include/game_functions.hxx"
#include "include/global_data.h"

static float SHOOT_DELAY = 0.4f;
static float SUPER_DELAY = 5.0f;
static float BLINK_DELAY = 0.2f;
static float DELAY_AFTER_BLINK = 0.2f;

static float BLINKING_PATH = 32;
static int HEALTH = 500;

CHL::camera* main_camera;

player::player(float x, float y, float z_index, int speed, int size)
    : life_form(x, y, z_index, speed, size) {
    health = HEALTH;

    /*clear the array from dust*/
    for (int i = 0; i < 18; i++)
        keys[i] = false;

    collision_box.x = TILE_SIZE / 2 + 4;
    collision_box.y -= 2;

    /*setting up the texture parameters*/
    selected_frame = 0;
    frames_in_texture = 12;
    frames_in_animation = 12;
    tilesets_in_texture = 5;

    visor_light =
        new CHL::light(5.0f, CHL::point(0, 0), CHL::vec3(0.0f, 0.8f, 0.1f));

    steps_source =
        CHL::create_new_source(manager.get_sound("move_sound"), this);
    blink_source =
        CHL::create_new_source(manager.get_sound("blink_sound"), this);
    fire_source = CHL::create_new_source(manager.get_sound("shot_sound"), this);
}

player::player(float x, float y, float z, int _speed, int _size_x, int _size_y)
    : player(x, y, z, _speed, 0) {
    size = CHL::point(x, y);
}

player::~player() {
    std::cerr << "Player deleted!" << std::endl;
    if (moving)
        CHL::stop_s(steps_source);
    CHL::delete_source(steps_source);

    CHL::stop_s(fire_source);
    CHL::delete_source(fire_source);

    CHL::stop_s(blink_source);
    CHL::delete_source(blink_source);

    delete visor_light;
}

void do_actions(player* p) {
    if (p->keys[p->key_super])
        p->super_fire();
    if (p->keys[p->key_fire])
        p->fire();
    if (p->keys[p->key_blink])
        p->blink();
}

void update_delay(player* p, float dt) {
    if (p->shoot_delay > 0)
        p->shoot_delay -= dt;
    if (p->super_delay > 0)
        p->super_delay -= dt;
    if (p->blink_delay > 0)
        p->blink_delay -= dt;
    if (p->delay_after_blink > 0)
        p->delay_after_blink -= dt;
}

float change_sprite(player* p, change_mode mode) {
    float a = CHL::get_direction(p->mouth_cursor.x, p->mouth_cursor.y,
                                 p->position.x + TILE_SIZE / 2,
                                 p->position.y - TILE_SIZE / 2);

    /*changing selected tileset for animations or frame for stall state
     * depending on current direction. Also a bit of change for the light
     * offset, to fit light perfectly on player's visor.*/

    if (a > (3 * M_PI_2 + M_PI_4) || a <= M_PI_4) {
        p->light_offset.x = 2;
        if (!p->moving) {
            if (mode == change_mode::blink) {
                p->selected_tileset = 1;
                p->selected_frame = 8;
            } else
                p->selected_frame = 3;
        }
    } else if (a > M_PI_4 && a <= M_PI_2 + M_PI_4) {
        p->light_offset.x = 0;
        if (!p->moving) {
            if (mode == change_mode::blink) {
                p->selected_tileset = 4;
                p->selected_frame = 8;
            } else
                p->selected_frame = 0;
        }
    } else if (a > M_PI_2 + M_PI_4 && a < M_PI + M_PI_4) {
        p->light_offset.x = -2;
        if (!p->moving) {
            if (mode == change_mode::blink) {
                p->selected_tileset = 2;
                p->selected_frame = 8;
            } else
                p->selected_frame = 2;
        }
    } else {
        p->light_offset.x = 0;
        if (!p->moving) {
            if (mode == change_mode::blink) {
                p->selected_tileset = 3;
                p->selected_frame = 8;
            } else
                p->selected_frame = 1;
        }
    }
    return a;
}

float check_registred_keys(player* p) {
    float a = -1;

    /*calculate direction angle depending on pressed keys*/

    if (p->keys[p->key_up]) {
        p->selected_tileset = 4;
        p->light_offset.x = 0;
        a = M_PI_2;
    }
    if (p->keys[p->key_down]) {
        p->selected_tileset = 3;
        p->light_offset.x = 0;
        a = 3 * M_PI_2;
    }
    if (p->keys[p->key_left]) {
        p->selected_tileset = 2;
        p->light_offset.x = -2;
        a = M_PI;
    }
    if (p->keys[p->key_right]) {
        p->selected_tileset = 1;
        p->light_offset.x = 2;
        a = 2 * M_PI;
    }
    if (p->keys[p->key_up] && p->keys[p->key_down]) {
        a = -1;    // mark as not moved. Tileset and frame will choose
                   // automatically
    }
    if (p->keys[p->key_up] && p->keys[p->key_left]) {
        p->selected_tileset = 2;
        a = 3 * M_PI_4;
    }
    if (p->keys[p->key_up] && p->keys[p->key_right]) {
        p->selected_tileset = 1;
        a = M_PI_4;
    }
    if (p->keys[p->key_right] && p->keys[p->key_down]) {
        a = 2 * M_PI - M_PI_4;
    }
    if (p->keys[p->key_left] && p->keys[p->key_down]) {
        a = M_PI + M_PI_4;
    }

    return a;
}

void player::register_keys(CHL::event up,
                           CHL::event down,
                           CHL::event left,
                           CHL::event right,
                           CHL::event fire,
                           CHL::event super,
                           CHL::event blink,
                           CHL::event attack) {
    key_up = static_cast<uint32_t>(up);
    key_down = static_cast<uint32_t>(down);
    key_left = static_cast<uint32_t>(left);
    key_right = static_cast<uint32_t>(right);
    key_fire = static_cast<uint32_t>(fire);
    key_super = static_cast<uint32_t>(super);
    key_blink = static_cast<uint32_t>(blink);
    key_attack = static_cast<uint32_t>(attack);
}

static float pitch_value = 1.0f;

void player::blink() {
    if (blink_delay <= 0 && !blinking) {
        if (delay_after_blink > 0) {
            pitch_value += 0.1f;
        } else {
            pitch_value = 1.0f;
        }

        blinking = true;
        blink_delay = BLINK_DELAY;
        blinking_path = BLINKING_PATH;
        blinking_alpha = change_sprite(this, change_mode::blink);

        CHL::set_pos_s(blink_source, CHL::vec3(position.x, position.y, 0.0f));
        CHL::pitch_s(blink_source, pitch_value);
        CHL::play_s(blink_source);
    }
}

void player::blink_to(const CHL::point& p) {
    if (!blinking) {
        blinking = true;
        blinking_path = CHL::get_distance(position.x, position.y, p.x, p.y);
        blinking_alpha = CHL::get_direction(p.x, p.y, position.x, position.y);

        CHL::set_pos_s(blink_source, CHL::vec3(position.x, position.y, 0.0f));
        CHL::pitch_s(blink_source, 1.0f);
        CHL::play_s(blink_source);
    }
}

void player::fire() {
    if (shoot_delay <= 0.0f && !blinking) {
        shoot_delay = SHOOT_DELAY;
        shooting_point = calculate_shooting_point(this, shooting_alpha);
        bullets.insert(bullets.end(), new bullet(position.x + shooting_point.x,
                                                 position.y + shooting_point.y,
                                                 0.0f, 4, 2, 0, 2));
        auto last_bullet = bullets.end() - 1;
        (*last_bullet)->alpha = calculate_alpha_precision(shooting_alpha);
        (*last_bullet)->speed = B_SPEED;
        (*last_bullet)->creator = bullet_creator::player;

        CHL::set_pos_s(fire_source, CHL::vec3(position.x, position.y, 0.0f));
        CHL::play_s(fire_source);
    }
}

void player::super_fire() {
    if (super_delay <= 0.0f && !blinking) {
        CHL::set_pos_s(fire_source, CHL::vec3(position.x, position.y, 0.0f));
        CHL::play_s(fire_source);
        for (int i = 0; i < 32; i++) {
            bullets.insert(bullets.end(), new bullet(position.x + TILE_SIZE / 2,
                                                     position.y - TILE_SIZE / 2,
                                                     0.0f, 4, 2, 0, 2));
            auto last_bullet = bullets.end() - 1;
            (*last_bullet)->alpha = 2 * M_PI * i / 32.0f;
            (*last_bullet)->speed = B_SPEED;
            super_delay = SUPER_DELAY;
        }
    }
}

void player::move(float dt) {
    position.z_index = position.y;

    CHL::listener_update(CHL::vec3(position.x, position.y, 0.0f));

    delta_x = 0;
    delta_y = 0;

    visor_light->position.x = position.x + 6 + light_offset.x;
    visor_light->position.y = position.y - 6 + light_offset.y;

    /*if blinking use another path calculating algorithm*/
    if (blinking) {
        float path = 6 * speed * dt;
        blinking_path -= path;
        delta_x = path * std::cos(blinking_alpha);
        delta_y = -path * std::sin(blinking_alpha);

        position.y += delta_y;
        position.x += delta_x;

        /*checking the borders*/
        if (position.x < 0 || position.x > VIRTUAL_WIDTH)
            position.x -= delta_x;
        if (position.y - TILE_SIZE < 0 || position.y > VIRTUAL_HEIGHT)
            position.y -= delta_y;

        if (blinking_path <= 0) {
            blinking = false;
            selected_tileset = 0;
            change_sprite(this, change_mode::regular);
            delay_after_blink = DELAY_AFTER_BLINK;
            return;
        }
        non_material_quads.insert(
            non_material_quads.end(),
            new CHL::instance(position.x, position.y, position.z_index,
                              TILE_SIZE));

        auto last_quad = non_material_quads.end() - 1;

        /*setting the player's fade. Using the same texture setting for
         * realistic effect*/
        (*last_quad)->frames_in_texture = frames_in_texture;
        (*last_quad)->tilesets_in_texture = tilesets_in_texture;
        (*last_quad)->selected_tileset = selected_tileset;
        (*last_quad)->selected_frame = selected_frame;

        // setting stereo sound effect
        CHL::set_pos_s(
            steps_source,
            CHL::vec3(position.x + 50 * sign(precise(delta_x, 0.1)),
                      position.y + 50 * sign(precise(delta_y, 0.1)), 0.0f));

        return;
    }

    float path = speed * dt;
    float a = change_sprite(this, change_mode::regular);
    shooting_alpha = a;
    a = check_registred_keys(this);

    /*a = -1 means that no keys are pressed*/
    if (a != -1) {
        delta_x = path * std::cos(a);
        delta_y = -path * std::sin(a);
    }

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

    // setting stereo sound effect
    CHL::set_pos_s(
        steps_source,
        CHL::vec3(position.x + 50 * sign(precise(delta_x, 0.1)),
                  position.y + 50 * sign(precise(delta_y, 0.1)), 0.0f));

    position.y += delta_y;
    position.x += delta_x;

    /*checking the borders*/
    if (position.x < 0 || position.x > VIRTUAL_WIDTH)
        position.x -= delta_x;
    if (position.y - TILE_SIZE < 0 || position.y > VIRTUAL_HEIGHT)
        position.y -= delta_y;

    do_actions(this);
    update_delay(this, dt);

    /*textures and points update*/
    update_points();
    update();
}
