/*
 * player.h
 *
 *  Created on: 19 янв. 2018 г.
 *      Author: Shaft
 */

#ifndef PLAYER_H_
#define PLAYER_H_

#include "engine.hxx"

enum class change_mode { blink, regular };

class player : public CHL::life_form {
   public:
    player(float x, float y, float z_index, int speed, int size);
    virtual ~player();

    float shooting_alpha = 0;

    friend float change_sprite(player*, change_mode);
    friend float check_registred_keys(player*);
    friend void do_actions(player*);
    friend void update_delay(player*, float dt);
    void move(float dt) override;
    void fire();
    void super_fire();
    void blink();
    void blink_to(const CHL::point&);

    CHL::point mouth_cursor;
    CHL::point shooting_point;

    bool keys[18];

    void register_keys(CHL::event up,
                       CHL::event down,
                       CHL::event left,
                       CHL::event right,
                       CHL::event fire,
                       CHL::event super,
                       CHL::event blink,
                       CHL::event attack);

   private:
    uint32_t steps_source = 0;
    uint32_t blink_source = 0;
    uint32_t fire_source = 0;

    float keys_registred = false;

    float shoot_delay = 0.7f;
    float super_delay = 5.0f;
    float blink_delay = 1.0f;
    float delay_after_blink = 0.2f;

    float blinking_path = 32;
    float blinking_alpha = 0;

    bool moving = false;
    bool blinking = false;

    uint32_t key_up;
    uint32_t key_down;
    uint32_t key_left;
    uint32_t key_right;
    uint32_t key_fire;
    uint32_t key_super;
    uint32_t key_blink;
    uint32_t key_attack;
};

#endif /* PLAYER_H_ */
