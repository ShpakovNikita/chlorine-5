/*
 * Resource manager for game "Chlorine-5".
 */
#pragma once

#include "engine.hxx"

#include <map>

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
