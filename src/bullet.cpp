/*
 * bullet.cpp
 *
 *  Created on: 6 янв. 2018 г.
 *      Author: Shaft
 */

#include "headers/bullet.h"
#include <iostream>

#include <math.h>

bullet::bullet(float x,
               float y,
               float z,
               int _size_x,
               int _size_y,
               int _damage,
               float _alpha)
    : instance(x, y, z, _size_x, _size_y) {
    damage = _damage;
    alpha = _alpha;
    speed = 1;
}

bullet::~bullet() {
    //    std::cerr << "Destroyed bullet!" << std::endl;a
}

void bullet::move(float dt) {
    position.z_index = position.y;
    float path = speed * dt;

    position.y -= path * std::sin(alpha);
    position.x += path * std::cos(alpha);
}
