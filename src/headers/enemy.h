/*
 * Enemy base class for the game "Chlorine-5"
 */
#pragma once

#include "engine.hxx"

class enemy : public CHL::life_form {
   public:
    enemy(float x, float y, float z, int _speed, int _size);
    enemy(float x, float y, float z, int _speed, int _size_x, int _size_y);
    virtual ~enemy();

    CHL::point destination;
    CHL::light* visor_light;

    int* map;

    /*actions*/
    void move(float) override;
    void fire();

    /*decision tree states*/
    friend void chase(enemy*, float dt);
    friend void stall(enemy*, float dt);
    friend void smart_move(enemy*, float dt);
    friend float change_sprite(enemy*);
    friend void pathfind(enemy*);

    friend void do_actions(enemy*, float dt);

   private:
    bool moving = false;
    float shoot_delay = 0.0f;

    void (*state)(enemy*, float);

    CHL::point step_dest;
    CHL::point light_offset;
    CHL::point shooting_point;
    float delta_find = 0.0f;
    float shooting_alpha;

    uint32_t fire_source = 0;
    uint32_t steps_source = 0;
};
