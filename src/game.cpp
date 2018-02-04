#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <math.h>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <thread>

#include "headers/engine.hxx"
#include "dungeon.cpp"

#include "headers/bullet.h"
#include "headers/enemy.h"
#include "headers/special_effect.h"
#include "headers/autotile.hxx"
#include "headers/pathfinders.h"
#include "headers/collision_solves.hxx"
#include "headers/player.h"

#include "headers/game_constants.h"

enum class mode { draw, look, idle };

int main(int /*argc*/, char* /*argv*/ []) {
    using namespace CHL;
    std::unique_ptr<engine, void (*)(engine*)> eng(create_engine(),
                                                   destroy_engine);

    eng->CHL_init(WINDOW_WIDTH, WINDOW_HEIGHT, TILE_SIZE, FPS);
    eng->set_virtual_world(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    eng->snap_tile_to_screen_pixels(1920 / 15.0f);

    font* f = new font("fonts/INVASION2000.ttf");
    light* l = new light();

    /* loading textures */

    manager.add_texture("brick", new texture("textures/test.png"));
    manager.add_texture("hero", new texture("textures/hero.png"));
    manager.add_texture("tank", new texture("textures/tank.png"));
    manager.add_texture("floor", new texture("textures/tiles.png"));
    manager.add_texture("bullet", new texture("textures/bullet.png"));
    manager.add_texture("explosion", new texture("textures/explosion.png"));
    manager.add_texture("obelisk", new texture("textures/obelisk.png"));
    manager.add_texture("dialog", new texture("textures/dialog.png"));
    manager.add_texture("health", new texture("textures/health.png"));
    manager.add_texture("load", new texture("textures/load.png"));
    manager.add_texture("icon", new texture("textures/icon.png"));

    manager.add_sound("start_music", new sound(SND_FOLDER + START_MUSIC));
    manager.add_sound("move_sound", new sound(SND_FOLDER + MOVE_SOUND));
    manager.add_sound("shot_sound", new sound(SND_FOLDER + "shot.wav"));
    manager.add_sound("blink_sound", new sound(SND_FOLDER + "blink.wav"));
    //    manager.get_sound("start_music")->play_always();

    user_interface* ui = new user_interface();
    user_interface* load_screen = new user_interface();
    load_screen->add_instance(new ui_element(100, 200, MIN_DEPTH, 200, 40,
                                             manager.get_texture("load")));
    ui_element* health_bar = new ui_element(100, 200, MIN_DEPTH, 200, 40,
                                            manager.get_texture("health"));
    health_bar->tilesets_in_texture = 6;
    health_bar->selected_tileset = 5;
    ui->add_instance(new ui_element(
        400, WINDOW_HEIGHT - 100, MIN_DEPTH, 1000, 350,
        manager.get_texture("dialog"),
        "When the world is doomed, there is only one hope... And you have to "
        "be it. Save this city, and all your crimes will be forgiven.",
        f));

    ui->add_instance(health_bar);
    ui->add_instance(new ui_element(60, WINDOW_HEIGHT - 100, MIN_DEPTH, 350,
                                    500, manager.get_texture("icon")));

    for (auto e : ui->user_interface_elements)
        if (e == health_bar)
            std::cout << "true" << std::endl;
        else
            std::cout << "false" << std::endl;

    DungeonGenerator generator(x_size, y_size);
    auto map = generator.Generate();
    std::vector<int> tile_set = map.Print();

    bool placed = false;
    player* hero = new player(0.0f, 7.0f, 0.0f, P_SPEED, TILE_SIZE);

    hero->weight = 2;
    hero->register_keys(CHL::event::up_pressed, CHL::event::down_pressed,
                        CHL::event::left_pressed, CHL::event::right_pressed,
                        CHL::event::left_mouse_pressed,
                        CHL::event::button1_pressed,
                        CHL::event::button2_pressed, CHL::event::turn_off);

    camera* main_camera = new camera(WINDOW_WIDTH / 8, WINDOW_HEIGHT / 8,
                                     VIRTUAL_WIDTH, VIRTUAL_HEIGHT, hero);
    entities.insert(entities.end(), hero);
    /* generate dungeon and place character */

    instance*** grid;
    grid = new instance**[y_size];
    for (int i = 0; i < y_size; i++)
        grid[i] = new instance*[x_size];

    int** map_grid;
    map_grid = new int*[y_size];
    for (int i = 0; i < y_size; i++)
        map_grid[i] = new int[x_size];

    std::vector<instance*> floor;
    for (int y = 0; y < y_size; y++) {
        for (int x = 0; x < x_size; x++) {
            grid[y][x] = nullptr;
            map_grid[y][x] = *(tile_set.begin() + y * x_size + x);
            if (*(tile_set.begin() + y * x_size + x) != 0) {
                bricks.insert(
                    bricks.end(),
                    create_wall(x * TILE_SIZE, y * TILE_SIZE + TILE_SIZE, 1.0f,
                                TILE_SIZE));
                (*(bricks.end() - 1))->frames_in_texture = 13;
                (*(bricks.end() - 1))->tilesets_in_texture = 3;
                (*(bricks.end() - 1))->selected_frame = default_frame;
                (*(bricks.end() - 1))->selected_tileset = default_tileset;
                (*(bricks.end() - 1))->update_data();
                grid[y][x] = *(bricks.end() - 1);
            } else {
                floor.insert(floor.end(), create_wall(x * TILE_SIZE,
                                                      y * TILE_SIZE + TILE_SIZE,
                                                      MAX_DEPTH, TILE_SIZE));
                (*(floor.end() - 1))->frames_in_texture = 8;
                (*(floor.end() - 1))->selected_frame = rand() % 8;
                (*(floor.end() - 1))->update_data();
            }
            if (!placed && *(tile_set.begin() + y * x_size + x) == 0) {
                hero->position.x = x * TILE_SIZE;
                hero->position.y = y * TILE_SIZE + TILE_SIZE;
                placed = true;
                *(tile_set.begin() + y * x_size + x) = 1;
            }
        }
    }

    autotile(map_grid, grid, x_size, y_size);

    /* load background */

    /* place enemies */
    std::vector<enemy*> enemies;

    int map_grid_pf[x_size * y_size];
    int count = 0;
    int dest = 0;

    while (count < dest) {
        int x = rand() % x_size;
        int y = rand() % y_size;
        if (*(tile_set.begin() + y * x_size + x) != 1) {
            entities.insert(entities.end(),
                            new enemy(x * TILE_SIZE, y * TILE_SIZE + TILE_SIZE,
                                      0.0f, P_SPEED - 17, TILE_SIZE));
            (*(entities.end() - 1))->frames_in_texture = 4;
            //            (*(entities.end() - 1))->collision_box.y = TILE_SIZE /
            //            2;
            dynamic_cast<enemy*>(*(entities.end() - 1))->map = map_grid_pf;
            dynamic_cast<enemy*>(*(entities.end() - 1))->destination.x =
                hero->position.x;
            dynamic_cast<enemy*>(*(entities.end() - 1))->destination.y =
                hero->position.y;
            *(tile_set.begin() + y * x_size + x) = 1;
            count++;
        }
    }

    float prev_frame = eng->GL_time();
    bool quit = false;

    std::vector<special_effect*> se;

    convert2d_array(map_grid, map_grid_pf, x_size, y_size);

    /* animation test */
    instance* animated_block =
        new instance(5 * TILE_SIZE - 4, 5 * TILE_SIZE + TILE_SIZE - 4,
                     MIN_DEPTH, TILE_SIZE + 8, TILE_SIZE + 16);
    animated_block->frames_in_animation = 11;
    animated_block->frames_in_texture = 11;
    animated_block->loop_animation(0.06f);

    /* load screen */
    //    eng->GL_clear_color();
    //    eng->render_ui(load_screen);
    //    eng->render_text("LOADING.", f, WINDOW_WIDTH - 350, WINDOW_HEIGHT -
    //    100, 0,
    //                     MIN_DEPTH, vec3(1.0f, 1.0f, 1.0f));
    //    eng->GL_swap_buffers();
    //    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //    eng->GL_clear_color();
    //    eng->render_ui(load_screen);
    //    eng->render_text("LOADING..", f, WINDOW_WIDTH - 350, WINDOW_HEIGHT -
    //    100, 0,
    //                     MIN_DEPTH, vec3(1.0f, 1.0f, 1.0f));
    //    eng->GL_swap_buffers();
    //    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //    eng->GL_clear_color();
    //    eng->render_ui(load_screen);
    //    eng->render_text("LOADING...", f, WINDOW_WIDTH - 350, WINDOW_HEIGHT -
    //    100,
    //                     0, MIN_DEPTH, vec3(1.0f, 1.0f, 1.0f));
    //    eng->GL_swap_buffers();
    //    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    /* running game loop */
    while (!quit) {
        float delta_time = eng->GL_time() - prev_frame;
        prev_frame = eng->GL_time();
        event e;

        while (eng->read_input(e)) {
            //            std::cout << e << std::endl;
            switch (e) {
                case event::turn_off:
                    quit = true;
                    break;
                case event::select_pressed:
                    quit = true;
                    break;
                default:
                    break;
            }

            if (eng->get_event_type() == event_type::pressed) {
                hero->keys[static_cast<int>(e)] = true;
            } else {
                hero->keys[static_cast<int>(e) - 1] = false;
            }
        }
        hero->move(delta_time);

        /*calculate angle*/
        hero->mouth_cursor.x = eng->get_mouse_pos(main_camera).x;
        hero->mouth_cursor.y = eng->get_mouse_pos(main_camera).y;

        /* check collisions */

        /// player & enemy collision
        int tst = 0;
        for (life_form* lf : entities) {
            lf->position.z_index = lf->position.y;
            enemy* e = dynamic_cast<enemy*>(lf);
            if (e != nullptr) {
                e->move(delta_time);
                e->destination.x = hero->position.x + TILE_SIZE / 2;
                e->destination.y = hero->position.y - TILE_SIZE / 4;
            }
            for (instance* inst : bricks) {
                if (check_collision(lf, inst)) {
                    solve_dynamic_to_static_collision_fast(
                        lf, inst, lf->delta_x, lf->delta_y);
                }
            }
            tst++;
        }

        int i = 0;
        hero->update_points();
        for (bullet* b : bullets) {
            if (b->position.x + TILE_SIZE < 0 ||
                b->position.x > VIRTUAL_WIDTH + TILE_SIZE ||
                b->position.y + TILE_SIZE < 0 ||
                b->position.y > VIRTUAL_HEIGHT + TILE_SIZE) {
                delete *(bullets.begin() + i);
                bullets.erase(bullets.begin() + i);
                continue;
            }

            b->update_points();
            point* intersection_point = new point();

            bool smth_destroyed = false;
            for (int j = 0; j < entities.size(); j++) {
                if (check_slow_collision(b, entities[j], intersection_point)) {
                    if (dynamic_cast<enemy*>(entities[j]) != nullptr &&
                        b->creator != bullet_creator::enemy) {
                        delete *(bullets.begin() + i);
                        bullets.erase(bullets.begin() + i);

                        if (--entities[j]->health <= 0) {
                            hero->blink_to(point(entities[j]->position.x,
                                                 entities[j]->position.y));
                            delete *(entities.begin() + j);
                            entities.erase(entities.begin() + j);
                            bool exit = true;
                            for (auto inst : entities) {
                                if (dynamic_cast<enemy*>(inst) != nullptr) {
                                    exit = false;
                                }
                            }
                            if (exit)
                                quit = true;
                        }

                        se.insert(se.end(),
                                  new special_effect(
                                      intersection_point->x - TILE_SIZE / 4 - 1,
                                      intersection_point->y + TILE_SIZE / 4 + 1,
                                      MIN_DEPTH, TILE_SIZE / 2 + 2));
                        (*(se.end() - 1))->frames_in_texture = 8;
                        j = entities.size();
                        smth_destroyed = true;
                    } else if (dynamic_cast<player*>(entities[j]) != nullptr &&
                               b->creator == bullet_creator::enemy) {
                        delete *(bullets.begin() + i);
                        bullets.erase(bullets.begin() + i);

                        health_bar->selected_tileset--;
                        se.insert(se.end(),
                                  new special_effect(
                                      intersection_point->x - TILE_SIZE / 4 - 1,
                                      intersection_point->y + TILE_SIZE / 4 + 1,
                                      MIN_DEPTH, TILE_SIZE / 2 + 2));
                        (*(se.end() - 1))->frames_in_texture = 8;
                        if (--entities[j]->health <= 0) {
                            quit = true;
                        }
                        j = entities.size();
                        smth_destroyed = true;
                    }
                }
            }

            if (smth_destroyed)
                continue;

            for (instance* brick : bricks) {
                if (check_slow_collision(b, brick, intersection_point)) {
                    delete *(bullets.begin() + i);
                    bullets.erase(bullets.begin() + i);

                    se.insert(se.end(),
                              new special_effect(
                                  intersection_point->x - TILE_SIZE / 4 - 1,
                                  intersection_point->y + TILE_SIZE / 4 + 1,
                                  MIN_DEPTH, TILE_SIZE / 2 + 2));
                    (*(se.end() - 1))->frames_in_texture = 8;

                    i--;
                    break;
                }
            }
            i++;
        }

        for (int j = 0; j < se.size(); j++) {
            if (se[j]->end()) {
                delete *(se.begin() + j);
                se.erase(se.begin() + j);
            }
        }

        /* draw sprites */

        eng->GL_clear_color();
        //        eng->render_text(
        //            "KUNG FURY!!!!!!!!!! Class aptent taciti sociosqu ad
        //            litora " "torquent per conubia nostra, per inceptos
        //            himenaeos. Donec orci" "risus, dignissim vitae dolor eu,
        //            vehicula sodales neque. Ut et " "efficitur neque. Quisque
        //            sed finibus sem, sed laoreet metus. " "Donec tincidunt ut
        //            neque a fringilla. Donec bibendum, enim et " "condimentum
        //            lobortis, velit nunc semper magna, ut malesuada nibh"
        //            "ipsum et diam. Curabitur aliquam, orci dictum congue
        //            sodales, " "elit diam mollis orci, vel eleifend neque
        //            tortor vitae felis. " "Morbi suscipit vel ipsum consequat
        //            fermentum.", f, 400, 100, 400, MIN_DEPTH, vec3(1.0f, 0.0f,
        //            0.0f));

        for (auto tile : floor)
            eng->add_object(tile, main_camera);

        if (!floor.empty())
            eng->draw(manager.get_texture("floor"), main_camera, nullptr);

        for (auto brick : bricks)
            eng->add_object(brick, main_camera);

        if (!bricks.empty())
            eng->draw(manager.get_texture("brick"), main_camera, nullptr);

        for (auto bullet : bullets) {
            bullet->move(delta_time);
            eng->add_object(bullet, main_camera);
        }

        if (!bullets.empty())
            eng->draw(manager.get_texture("bullet"), main_camera, nullptr);

        eng->add_object(hero, main_camera);
        eng->draw(manager.get_texture("hero"), main_camera, nullptr);

        for (auto e : entities) {
            if (dynamic_cast<enemy*>(e) != nullptr)
                eng->add_object(e, main_camera);
        }
        if (!entities.empty())
            eng->draw(manager.get_texture("tank"), main_camera, nullptr);

        for (auto effect : se) {
            effect->update_frame();
            eng->add_object(effect, main_camera);
        }

        if (!se.empty())
            eng->draw(manager.get_texture("explosion"), main_camera, nullptr);

        //        eng->render_ui(ui);
        for (auto quad = non_material_quads.begin();
             quad != non_material_quads.end();) {
            (*quad)->update_data();
            eng->add_object(*quad, main_camera);
            (*quad)->alpha_channel -= delta_time * 5;
            eng->draw(manager.get_texture("hero"), main_camera, *quad);
            if ((*quad)->alpha_channel <= 0.05f) {
                delete *quad;
                quad = non_material_quads.erase(quad);
                continue;
            }
            ++quad;
        }

        l->position.x = hero->position.x + 6;
        l->position.y = hero->position.y - 6;
        l->position.z_index = hero->position.z_index - 1;
        eng->render_light(l, main_camera);

        animated_block->update();
        eng->add_object(animated_block, main_camera);
        eng->draw(manager.get_texture("obelisk"), main_camera, nullptr);

        eng->GL_swap_buffers();

        /* dynamic sleep */
        float t = (eng->GL_time() - prev_frame) * 1000;

        if (t > 1000 / FPS)
            std::cerr << "freeze" << std::endl;
        if (t < 1000 / FPS)
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1000 / FPS - (int)t));
    }

    eng->CHL_exit();
    return EXIT_SUCCESS;
}
