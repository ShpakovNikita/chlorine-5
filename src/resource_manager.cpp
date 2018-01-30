/*
 * resource_manager.cpp
 *
 *  Created on: 20 янв. 2018 г.
 *      Author: Shaft
 */

#include "headers/resource_manager.h"

resource_manager::resource_manager() {
    // TODO Auto-generated constructor stub
}

resource_manager::~resource_manager() {
    // TODO Auto-generated destructor stub
}

void resource_manager::add_texture(const std::string& key,
                                   CHL::texture* value) {
    textures.insert(std::make_pair(key, value));
}

void resource_manager::add_sound(const std::string& key, CHL::sound* value) {
    sounds.insert(std::make_pair(key, value));
}

CHL::texture* resource_manager::get_texture(const std::string& key) {
    return textures[key];
}

CHL::sound* resource_manager::get_sound(const std::string& key) {
    return sounds[key];
}
