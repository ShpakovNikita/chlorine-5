#include "engine.hxx"

#ifndef HEADERS_BULLET_H_
#define HEADERS_BULLET_H_

enum class bullet_creator { enemy, ally, player, allmighty };

class bullet : public CHL::instance {
   public:
    int damage;
    int speed;
    bullet(float x, float y, float z, int _size, int _damage, float _alpha);
    ~bullet();

    bullet_creator creator = bullet_creator::allmighty;

    void move(float dt);
};

#endif
