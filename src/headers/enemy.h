/*
 * enemy.h
 *
 *  Created on: 12 янв. 2018 г.
 *      Author: Shaft
 */

#ifndef ENEMY_H_
#define ENEMY_H_

#include "engine.hxx"

enum class tree_states {
    chase,
    wander,
    stall,
    idle,
};

class enemy;

class enemy : public CHL::life_form {
   public:
    enemy(float x, float y, float z, int _speed, int s);
    virtual ~enemy();

    CHL::point destination;
    CHL::point shooting_point;

    int* map;
    float shooting_alpha;

    void move(float) override;

    void fire();

    friend void chase(enemy*);
    friend void stall(enemy*);
    friend void smart_move(enemy* e);
    friend float change_sprite(enemy* e);
    friend void pathfind(enemy* e);

   private:
    float shoot_delay = 0;
    void (*state)(enemy*);
    CHL::point step_dest;
    float delta_time;

    uint32_t fire_source = 0;
    uint32_t steps_source = 0;
};

#endif /* ENEMY_H_ */
