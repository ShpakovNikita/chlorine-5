/*
 * Player base class for the game "Chlorine-5"
 */
#pragma once

#include "engine.hxx"

/*mode for sprite change*/
enum class change_mode { blink, regular };

class player : public CHL::life_form {
   public:
    player(float x, float y, float z_index, int speed, int size);
    player(float x, float y, float z, int _speed, int _size_x, int _size_y);
    virtual ~player();

    float shooting_alpha = 0;

    CHL::light* visor_light;

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

    /*array of key states*/
    bool keys[18];

    /*ragister the regular keyboard controls*/
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

    CHL::point light_offset;

    float shoot_delay = 0.0f;
    float super_delay = 0.0f;
    float blink_delay = 0.0f;
    float delay_after_blink = 0.0f;

    float blinking_path = 0.0f;
    float blinking_alpha = 0.0f;

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
