/*
 * special_effect.h
 *
 *  Created on: 13 янв. 2018 г.
 *      Author: Shaft
 */

#ifndef SPECIAL_EFFECT_H_
#define SPECIAL_EFFECT_H_

#include "engine.hxx"

class special_effect : public CHL::instance {
   public:
    special_effect(float x, float y, float z, int size);
    virtual ~special_effect();

    void update_frame();
    bool end();

    float fps = 8;

   private:
    int delay;
    bool is_end = false;
};

#endif /* SPECIAL_EFFECT_H_ */
