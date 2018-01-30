/*
 * resource_manager.h
 *
 *  Created on: 20 янв. 2018 г.
 *      Author: Shaft
 */

#ifndef RESOURCE_MANAGER_H_
#define RESOURCE_MANAGER_H_

#include <map>
#include "engine.hxx"

class resource_manager {
   public:
    resource_manager();
    virtual ~resource_manager();

    void add_texture(const std::string&, CHL::texture*);
    void add_sound(const std::string&, CHL::sound*);

    CHL::texture* get_texture(const std::string&);
    CHL::sound* get_sound(const std::string&);

   private:
    std::map<std::string, CHL::sound*> sounds;
    std::map<std::string, CHL::texture*> textures;
};

#endif /* RESOURCE_MANAGER_H_ */
