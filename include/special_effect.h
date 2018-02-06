/*
 * Very simple capsule for animation system. Created outside the CHL engine.
 */

#pragma once
#include "engine.hxx"

class special_effect : public CHL::instance {
   public:
    special_effect(float x, float y, float z, int _size);
    virtual ~special_effect();

    void update_frame();
    bool end();

    float fps = 8;

   private:
    int delay;
    bool is_end = false;
};
