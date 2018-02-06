#include "headers/resource_manager.h"

#include <iostream>

resource_manager::resource_manager() {
    // there is no need for the code
}

resource_manager::~resource_manager() {
    auto t_itr = textures.begin();
    while (t_itr != textures.end()) {
        delete t_itr->second;
        t_itr = textures.erase(t_itr);
    }

    auto s_itr = sounds.begin();
    while (s_itr != sounds.end()) {
        delete s_itr->second;
        s_itr = sounds.erase(s_itr);
    }

    std::cerr << "manager cleared" << std::endl;
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
