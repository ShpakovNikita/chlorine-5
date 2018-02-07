cmake_minimum_required(VERSION 3.2)
project(Chlorine-5)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE 
game.cpp 
engine.cxx 
sound.cxx 
bullet.cpp  
texture.cxx 
enemy.cpp 
dungeon.cpp 
special_effect.cpp 
pathfinders.cpp
player.cpp 
resource_manager.cpp  
font.cxx 
display.cpp)

#uncomment below for adding all sources automatically
#FILE(GLOB SOURCE *.c *.cpp *.cxx)

if(UNIX)
    set(PROJECT_LINK_LIB -lglew32 -lSDL2main -lSDL2 -lfreetype)
else()
    set(PROJECT_LINK_LIB -lmingw32 -lglew32 -lSDL2main -lSDL2 -mwindows 
                                       -lopengl32 -lfreetype -lopenal32)
endif(UNIX)


link_directories(${PROJECT_LIBS_DIR})

add_executable(${PROJECT_NAME} ${SOURCE})

target_link_libraries(${PROJECT_NAME} ${PROJECT_LINK_LIB})

message("Build!")