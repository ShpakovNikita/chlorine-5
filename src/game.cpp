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

#include "headers/display.h"

#include "headers/global_data.h"

enum class mode { render, look, idle };

int main(int /*argc*/, char* /*argv*/ []) {
    using namespace CHL;
    /*initializing the CHL engine*/
    std::unique_ptr<engine, void (*)(engine*)> eng(create_engine(),
                                                   destroy_engine);
    int WINDOW_WIDTH, WINDOW_HEIGHT;
    eng->CHL_init(&WINDOW_WIDTH, &WINDOW_HEIGHT, TILE_SIZE, FPS);
    eng->set_virtual_world(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);

    /* loading font */
    font* f = new font("fonts/INVASION2000.ttf", 48);

    /* loading textures and sounds */
    manager.add_texture("brick", new texture(TEX_FOLDER + "test.png"));
    manager.add_texture("hero", new texture(TEX_FOLDER + "hero.png"));
    manager.add_texture("enemy", new texture(TEX_FOLDER + "enemy.png"));
    manager.add_texture("tank", new texture(TEX_FOLDER + "tank.png"));
    manager.add_texture("floor", new texture(TEX_FOLDER + "tiles.png"));
    manager.add_texture("bullet", new texture(TEX_FOLDER + "bullet.png"));
    manager.add_texture("explosion", new texture(TEX_FOLDER + "explosion.png"));
    manager.add_texture("obelisk", new texture(TEX_FOLDER + "obelisk.png"));
    manager.add_texture("dialog", new texture(TEX_FOLDER + "dialog.png"));
    manager.add_texture("health", new texture(TEX_FOLDER + "health.png"));
    manager.add_texture("load", new texture(TEX_FOLDER + "load.png"));
    manager.add_texture("win", new texture(TEX_FOLDER + "win.png"));
    manager.add_texture("loose", new texture(TEX_FOLDER + "loose.png"));

    manager.add_sound("start_music", new sound(SND_FOLDER + "main.wav"));
    manager.add_sound("move_sound", new sound(SND_FOLDER + "move.wav"));
    manager.add_sound("shot_sound", new sound(SND_FOLDER + "shot.wav"));
    manager.add_sound("blink_sound", new sound(SND_FOLDER + "blink.wav"));
    manager.add_sound("quit_sound", new sound(SND_FOLDER + "quit.wav"));

    manager.get_sound("start_music")->volume(0.6f);
    manager.get_sound("start_music")->play_always();

    /*load ui*/
    user_interface* ui = new user_interface();
    user_interface* load_screen = new user_interface();
    ui_element* health_bar = new ui_element(80, 50, MIN_DEPTH, 200, 40,
                                            manager.get_texture("health"));
    health_bar->tilesets_in_texture = 6;
    health_bar->selected_tileset = 5;
    /* dark cyberpunk styled dialog. To activate my custom ui dialog bar
    /*   uncomment it.
     */
    /*
       ui->add_instance(new ui_element( 400, WINDOW_HEIGHT - 100,
       MIN_DEPTH, 1000, 350, manager.get_texture("dialog"), "When the world is
       doomed, there is only one hope... And you have" "to be it. Save this
       city, and all your crimes will be" "forgiven.", f));
    */
    ui->add_instance(health_bar);

    DungeonGenerator generator(x_size, y_size);
    auto map = generator.Generate();
    std::vector<int> tile_set = map.Print();

    bool placed = false;
    player* hero = new player(0.0f, 7.0f, 0.0f, P_SPEED, TILE_SIZE);

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
                    new instance(x * TILE_SIZE, y * TILE_SIZE + TILE_SIZE, 1.0f,
                                 TILE_SIZE));
                (*(bricks.end() - 1))->frames_in_texture = 13;
                (*(bricks.end() - 1))->tilesets_in_texture = 3;
                (*(bricks.end() - 1))->selected_frame = default_frame;
                (*(bricks.end() - 1))->selected_tileset = default_tileset;
                (*(bricks.end() - 1))->update_data();
                grid[y][x] = *(bricks.end() - 1);
            } else {
                floor.insert(
                    floor.end(),
                    new instance(x * TILE_SIZE, y * TILE_SIZE + TILE_SIZE,
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

    /* place enemies */
    std::vector<enemy*> enemies;

    int map_grid_pf[x_size * y_size];
    int count = 0;
    int dest = 5;

    while (count < dest) {
        int x = rand() % x_size;
        int y = rand() % y_size;
        if (*(tile_set.begin() + y * x_size + x) != 1) {
            entities.insert(
                entities.end(),
                new enemy(x * TILE_SIZE, y * TILE_SIZE + TILE_SIZE - 2, 0.0f,
                          P_SPEED, TILE_SIZE));

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
    bool win = false;
    bool loose = false;

    // vector for bullet destructions
    std::vector<special_effect*> se;

    convert2d_array(map_grid, map_grid_pf, x_size, y_size);

    /* animation test, easter egg from the duelyst. */
    instance* animated_block =
        new instance(5 * TILE_SIZE - 4, 5 * TILE_SIZE + TILE_SIZE - 4,
                     MIN_DEPTH, TILE_SIZE + 8, TILE_SIZE + 8);
    animated_block->frames_in_animation = 11;
    animated_block->frames_in_texture = 11;
    animated_block->loop_animation(0.06f);

    /* load screen. Yes, it is just for visual effect because everything is
     * loading so fast. */
    eng->GL_clear_color();
    eng->render_ui(load_screen);
    eng->render_text("LOADING.", f, WINDOW_WIDTH - 350, WINDOW_HEIGHT - 100, 0,
                     MIN_DEPTH, vec3(1.0f, 1.0f, 1.0f));
    eng->GL_swap_buffers();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    eng->GL_clear_color();
    eng->render_ui(load_screen);
    eng->render_text("LOADING..", f, WINDOW_WIDTH - 350, WINDOW_HEIGHT - 100, 0,
                     MIN_DEPTH, vec3(1.0f, 1.0f, 1.0f));
    eng->GL_swap_buffers();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    eng->GL_clear_color();
    eng->render_ui(load_screen);
    eng->render_text("LOADING...", f, WINDOW_WIDTH - 350, WINDOW_HEIGHT - 100,
                     0, MIN_DEPTH, vec3(1.0f, 1.0f, 1.0f));
    eng->GL_swap_buffers();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    /* running game loop */
    while (!quit) {
        /*calculate delta_time for each frame to move smoothly*/
        float delta_time = (eng->GL_time() - prev_frame);
        prev_frame = eng->GL_time();
        event e;

        std::cout << "log for the current frame: " << std::endl;

        while (eng->read_input(e)) {
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

            // pass events to our hero
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

        /// player & enemy collisions to walls
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
        }

        // here I have a big problems with the architecture of my game, and here
        // is a quite complex algorithm. But everything is clear, if you
        // read it line by line

        hero->update_points();
        for (auto b = bullets.begin(); b != bullets.end();) {
            // destroy bullet if it is outside the world.
            if ((*b)->position.x + TILE_SIZE < 0 ||
                (*b)->position.x > VIRTUAL_WIDTH + TILE_SIZE ||
                (*b)->position.y + TILE_SIZE < 0 ||
                (*b)->position.y > VIRTUAL_HEIGHT + TILE_SIZE) {
                delete *b;
                b = bullets.erase(b);
                continue;    // it is like our goto, but here we cannot access
                             // it.
            }

            (*b)->update_points();
            point* intersection_point = new point();

            for (auto en = entities.begin(); en != entities.end();) {
                // check collision with entities and do some entity related
                // stuff
                if (check_slow_collision(*b, *en, intersection_point)) {
                    if (dynamic_cast<enemy*>(*en) != nullptr &&
                        (*b)->creator != bullet_creator::enemy) {
                        delete *b;
                        b = bullets.erase(b);

                        if (--(*en)->health <= 0) {
                            hero->blink_to(
                                point((*en)->position.x, (*en)->position.y));
                            hero->health = std::min(hero->health + 1, 5);
                            health_bar->selected_tileset = hero->health;
                            delete *en;
                            entities.erase(en);
                            bool exit = true;
                            for (auto inst : entities) {
                                if (dynamic_cast<enemy*>(inst) != nullptr) {
                                    exit = false;
                                }
                            }
                            if (exit)
                                win = true;
                        }

                        se.insert(se.end(),
                                  new special_effect(
                                      intersection_point->x - TILE_SIZE / 4 - 1,
                                      intersection_point->y + TILE_SIZE / 4 + 1,
                                      MIN_DEPTH, TILE_SIZE / 2 + 2));
                        (*(se.end() - 1))->frames_in_texture = 8;
                        goto loop_end;
                    } else if (dynamic_cast<player*>(*en) != nullptr &&
                               (*b)->creator == bullet_creator::enemy) {
                        delete *b;
                        b = bullets.erase(b);

                        health_bar->selected_tileset--;
                        se.insert(se.end(),
                                  new special_effect(
                                      intersection_point->x - TILE_SIZE / 4 - 1,
                                      intersection_point->y + TILE_SIZE / 4 + 1,
                                      MIN_DEPTH, TILE_SIZE / 2 + 2));
                        (*(se.end() - 1))->frames_in_texture = 8;
                        if (--(*en)->health <= 0) {
                            loose = true;
                        }
                        goto loop_end;
                    }
                }
                ++en;
            }

            for (instance* brick : bricks) {
                if (check_slow_collision(*b, brick, intersection_point)) {
                    delete *b;
                    b = bullets.erase(b);

                    se.insert(se.end(),
                              new special_effect(
                                  intersection_point->x - TILE_SIZE / 4 - 1,
                                  intersection_point->y + TILE_SIZE / 4 + 1,
                                  MIN_DEPTH, TILE_SIZE / 2 + 2));
                    (*(se.end() - 1))->frames_in_texture = 8;

                    goto loop_end;
                }
            }
            ++b;
        loop_end:    // I think that goto sometimes isn't bad at all.
            continue;
        }

        for (int j = 0; j < se.size(); j++) {
            if (se[j]->end()) {
                delete *(se.begin() + j);
                se.erase(se.begin() + j);
            }
        }

        /* render sprites */

        eng->GL_clear_color();

        for (auto tile : floor)
            eng->add_object(tile, main_camera);

        if (!floor.empty())
            eng->render(manager.get_texture("floor"), main_camera, nullptr);

        for (auto brick : bricks)
            eng->add_object(brick, main_camera);

        if (!bricks.empty())
            eng->render(manager.get_texture("brick"), main_camera, nullptr);

        for (auto bullet : bullets) {
            bullet->move(delta_time);
            eng->add_object(bullet, main_camera);
        }

        if (!bullets.empty())
            eng->render(manager.get_texture("bullet"), main_camera, nullptr);

        eng->add_object(hero, main_camera);
        eng->render(manager.get_texture("hero"), main_camera, nullptr);

        for (auto e : entities) {
            enemy* cast = dynamic_cast<enemy*>(e);
            if (cast != nullptr) {
                eng->render_light(cast->visor_light, main_camera);
                eng->add_object(e, main_camera);
            }
        }
        if (!entities.empty())
            eng->render(manager.get_texture("enemy"), main_camera, nullptr);

        for (auto effect : se) {
            effect->update_frame();
            eng->add_object(effect, main_camera);
        }

        if (!se.empty())
            eng->render(manager.get_texture("explosion"), main_camera, nullptr);

        eng->render_ui(ui);
        for (auto quad = non_material_quads.begin();
             quad != non_material_quads.end();) {
            (*quad)->update_data();
            eng->add_object(*quad, main_camera);
            (*quad)->alpha_channel -= delta_time * 5;
            eng->render(manager.get_texture("hero"), main_camera, *quad);
            if ((*quad)->alpha_channel <= 0.05f) {
                delete *quad;
                quad = non_material_quads.erase(quad);
                continue;
            }
            ++quad;
        }

        eng->render_light(hero->visor_light, main_camera);

        animated_block->update();
        eng->add_object(animated_block, main_camera);
        eng->render(manager.get_texture("obelisk"), main_camera, nullptr);

        /* win/loose screen renders */
        if (win) {
            display::render_screen(
                eng.get(), manager.get_texture("win"),
                manager.get_sound("quit_sound"), WINDOW_WIDTH / 2 - 400,
                WINDOW_HEIGHT / 2, "THE HOST HAS BEEN VANISHED", f,
                vec3(0.325f, 0.196f, 0.713f), event::select_pressed,
                WINDOW_WIDTH, WINDOW_HEIGHT);
            break;
        } else if (loose) {
            display::render_screen(
                eng.get(), manager.get_texture("loose"),
                manager.get_sound("quit_sound"), WINDOW_WIDTH / 2 - 300,
                WINDOW_HEIGHT / 2 - 50, "THE HOPE HAS BEEN LOST", f,
                vec3(1.0f, 0.0f, 0.0f), event::select_pressed, WINDOW_WIDTH,
                WINDOW_HEIGHT);
            break;
        }

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
