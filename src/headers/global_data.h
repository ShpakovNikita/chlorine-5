/*
 * Global game data for game "Chlorine-5"
 */
#pragma once

#include "engine.hxx"
#include "bullet.h"
#include "resource_manager.h"

#include <string>
#include <vector>
#include <math.h>

const std::string SND_FOLDER = "sounds\\";
const std::string TEX_FOLDER = "textures\\";

constexpr int VIRTUAL_WIDTH = 1024, VIRTUAL_HEIGHT = 640;

constexpr int TILE_SIZE = 16;

constexpr int FPS = 60;

constexpr int P_SPEED = 32;
constexpr int B_SPEED = 100;

constexpr int x_size = VIRTUAL_WIDTH / TILE_SIZE,
              y_size = VIRTUAL_HEIGHT / TILE_SIZE;

extern std::vector<CHL::instance*> non_material_quads;
extern std::vector<CHL::instance*> bricks;
extern std::vector<bullet*> bullets;
extern std::vector<CHL::life_form*> entities;
extern CHL::camera* main_camera;
extern resource_manager manager;
