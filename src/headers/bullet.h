/*
 * Bullet base class for the game "Chlorine-5"
 */
#pragma once

#include "engine.hxx"

enum class bullet_creator { enemy, ally, player, allmighty };

class bullet : public CHL::instance {
   public:
    int damage;
    int speed;
    bullet(float x,
           float y,
           float z,
           int _size_x,
           int _size_y,
           int _damage,
           float _alpha);
    ~bullet();

    bullet_creator creator = bullet_creator::allmighty;

    /* update every frame */
    void move(float dt);
};
