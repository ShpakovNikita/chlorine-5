cmake_minimum_required(VERSION 3.2)
project(SDLdynamic)

set(CMAKE_CXX_STANDARD 11)
set(SOURCE game.cpp engine.cxx sound.cxx bullet.cpp headers/autotile.hxx 
texture.cxx headers/bullet.h headers/engine.hxx enemy.cpp headers/enemy.h 
headers/picopng.hxx dungeon.cpp special_effect.cpp headers/special_effect.h
headers/pathfinders.h pathfinders.cpp headers/collision_solves.hxx 
headers/game_constants.h player.cpp headers/player.h headers/game_functions.hxx
resource_manager.cpp headers/resource_manager.h font.cxx)

if(UNIX)
    set(PROJECT_LINK_LIB -lglew32 -lSDL2main -lSDL2 -lfreetype)
else()
    set(PROJECT_LINK_LIB -lmingw32 -lglew32 -lSDL2main -lSDL2 -mwindows -lopengl32 -lfreetype -lopenal32)
endif(UNIX)


link_directories(${PROJECT_LIBS_DIR})

add_executable(${PROJECT_NAME} ${SOURCE})

target_link_libraries(${PROJECT_NAME} ${PROJECT_LINK_LIB})

message("Build!")
