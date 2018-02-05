/*
 * engine.cxx
 *
 *  Created on: 24 нояб. 2017 г.
 *      Author: Shaft
 */
#include "headers/engine.hxx"

#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/gl.h>

#include <SDL2/SDL_main.h>
#include <SDL2/SDL.h>

#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_platform.h>
#include <assert.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <math.h>
#include <sstream>
#include <array>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <AL/al.h>
#include <AL/alc.h>

#ifdef __WINDOWS__
#include <windows.h>
#endif

#define NUM_BUFFERS 3
#define BUFFER_SIZE 4096

#define GL_CHECK()                                                  \
    {                                                               \
        const int err = glGetError();                               \
        if (err != GL_NO_ERROR) {                                   \
            switch (err) {                                          \
                case GL_INVALID_ENUM:                               \
                    std::cerr << GL_INVALID_ENUM << std::endl;      \
                    break;                                          \
                case GL_INVALID_VALUE:                              \
                    std::cerr << GL_INVALID_VALUE << std::endl;     \
                    break;                                          \
                case GL_INVALID_OPERATION:                          \
                    std::cerr << GL_INVALID_OPERATION << std::endl; \
                    break;                                          \
                case GL_INVALID_FRAMEBUFFER_OPERATION:              \
                    std::cerr << GL_INVALID_FRAMEBUFFER_OPERATION   \
                              << std::endl;                         \
                    break;                                          \
                case GL_OUT_OF_MEMORY:                              \
                    std::cerr << GL_OUT_OF_MEMORY << std::endl;     \
                    break;                                          \
            }                                                       \
            assert(false);                                          \
        }                                                           \
    }

namespace CHL {

static std::vector<float> quad_data{
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};

struct bind {
    bind(SDL_Keycode k, std::string n, event pressed, event released)
        : key(k), name(n), event_pressed(pressed), event_released(released) {}

    SDL_Keycode key;
    std::string name;
    event event_pressed;
    event event_released;
};

static std::array<std::string, 18> event_names = {
    /// input events
    "left_pressed", "left_released", "right_pressed", "right_released",
    "up_pressed", "up_released", "down_pressed", "down_released",
    "select_pressed", "select_released", "start_pressed", "start_released",
    "button1_pressed", "button1_released", "button2_pressed",
    "button2_released",
    /// mouse events
    "left_mouse_pressed",
    /// virtual console events
    "turn_off"};

static std::string color_fragment =
    "uniform vec4 color;"

    "void main()"
    "{"
    "gl_FragColor = color;"
    "}";

static std::string fragment_glsl =
    "varying vec4 vertex_color;"
    "varying vec2 tex_coord;"

    "uniform sampler2D our_texture;"

    "void main()"
    "{"
    "gl_FragColor = texture(our_texture, tex_coord) * vertex_color;"
    "}";

static std::string text_fragment_glsl =
    "varying vec4 vertex_color;"
    "varying vec2 tex_coord;"

    "uniform sampler2D our_texture;"

    "void main()"
    "{"
    "vec4 sampled = vec4(1.0, 1.0, 1.0, texture(our_texture, tex_coord).r);"
    "gl_FragColor = sampled * vertex_color;"
    "}";

static std::string light_fragment =
    "uniform vec2 object;"
    "uniform vec2 resolution;"
    "uniform float time;"
    "uniform float radius;"
    "uniform vec3 color;"

    "void main()"
    "{"
    "float r = 2.0 * radius;"

    "vec2 delta = vec2 (( gl_FragCoord.x - object.x ), ( "
    "gl_FragCoord.y - (resolution.y -object.y )));"
    "float distance = sqrt( delta.x * delta.x + delta.y * delta.y) / "
    "sqrt(resolution.x * resolution.x + resolution.y * resolution. y);"
    "float k = r / resolution.x;"

    "vec4 light = vec4(color.xyz, pow(0.1, distance / "
    "k) - 0.1);"
    "gl_FragColor = light;"
    "}";

static std::string light_vertex =
    "attribute vec3 position;"

    "uniform mat4 transform;"

    "void main()"
    "{"
    "gl_Position = transform * vec4(position, 1.0);"
    "}";

static std::string color_vertex =
    "attribute vec3 position;"

    "uniform mat4 transform;"

    "void main()"
    "{"
    "gl_Position = transform * vec4(position, 1.0);"
    "}";

static std::string vertex_glsl =
    "attribute vec3 position;"
    "attribute vec2 texCoord;"

    "varying vec4 vertex_color;"
    "varying vec2 tex_coord;"

    "uniform vec4 our_color;"
    "uniform mat4 transform;"

    "void main()"
    "{"
    "gl_Position = transform * vec4(position, 1.0);"
    "vertex_color = our_color;"
    "tex_coord = vec2(texCoord.x, 1.0 - texCoord.y);"
    "}";

static std::string text_vertex_glsl =
    "attribute vec3 position;"
    "attribute vec2 texCoord;"

    "varying vec2 tex_coord;"
    "varying vec4 vertex_color;"

    "uniform vec3 color;"
    "uniform mat4 transform;"

    "void main()"
    "{"
    " gl_Position = transform * vec4(position, 1.0);"
    "  tex_coord = texCoord;"
    "   vertex_color = vec4(color, 1.0);"
    "}";

const std::array<bind, 8> bindings{
    bind{SDLK_w, "up", event::up_pressed, event::up_released},
    bind{SDLK_a, "left", event::left_pressed, event::left_released},
    bind{SDLK_s, "down", event::down_pressed, event::down_released},
    bind{SDLK_d, "right", event::right_pressed, event::right_released},
    bind{SDLK_LCTRL, "button1", event::button1_pressed,
         event::button1_released},
    bind{SDLK_SPACE, "button2", event::button2_pressed,
         event::button2_released},
    bind{SDLK_ESCAPE, "select", event::select_pressed, event::select_released},
    bind{SDLK_RETURN, "start", event::start_pressed, event::start_released}};

std::ostream& operator<<(std::ostream& out, const SDL_version& v) {
    out << static_cast<int>(v.major) << '.';
    out << static_cast<int>(v.minor) << '.';
    out << static_cast<int>(v.patch);
    return out;
}

std::ostream& operator<<(std::ostream& stream, const event e) {
    std::uint32_t value = static_cast<std::uint32_t>(e);
    std::uint32_t minimal = static_cast<std::uint32_t>(event::left_pressed);
    std::uint32_t maximal = static_cast<std::uint32_t>(event::turn_off);
    if (value >= minimal && value <= maximal) {
        stream << event_names[value];
        return stream;
    } else {
        throw std::runtime_error("too big event value");
    }
}

static bool check_input(const SDL_Event& e, const bind*& result) {
    using namespace std;

    const auto it =
        find_if(bindings.begin(), bindings.end(),
                [&](const bind& b) { return b.key == e.key.keysym.sym; });

    if (it != bindings.end()) {
        result = &(*it);
        return true;
    }
    return false;
}

engine::engine() {}
engine::~engine() {
    SDL_Quit();
}

static int t_size, w_t_size;
static int w_w, w_h;
static int virtual_w = 0;
static int virtual_h = 0;
static int FPS;

bool check_collision(instance* one,
                     instance* two)    // AABB - AABB collision
{
    // Collision x-axis?
    float precision = 0.1f;

    bool collisionX =
        one->position.x + one->collision_box_offset.x + one->collision_box.x >
            two->position.x + two->collision_box_offset.x + precision &&
        two->position.x + two->collision_box.x + two->collision_box_offset.x >
            one->position.x + one->collision_box_offset.x + precision;
    // Collision y-axis?
    bool collisionY =
        one->position.y - one->collision_box_offset.y - one->collision_box.y <
            two->position.y - two->collision_box_offset.y - precision &&
        two->position.y - two->collision_box_offset.y - two->collision_box.y <
            one->position.y - one->collision_box_offset.y - precision;

    return collisionX && collisionY;
}

camera::camera(int w, int h, int b_w, int b_h, instance* object) {
    width = w;
    height = h;
    border_width = b_w;
    border_height = b_h;
    bind_object = object;
}

camera::~camera() {}

void camera::update_center() {
    float precision = 0.01f;
    if (bind_object->position.x + t_size / 2 > width / 2 &&
        bind_object->position.x + t_size / 2 < border_width - width / 2)
        center.x =
            std::floor((bind_object->position.x + t_size / 2 - width / 2) /
                       precision) *
            precision;
    else if (bind_object->position.x + t_size / 2 >= border_width - width / 2)
        center.x = border_width - width;

    if (bind_object->position.y - t_size / 2 > height / 2 &&
        bind_object->position.y - t_size / 2 < border_height - height / 2)
        center.y =
            std::floor((bind_object->position.y - t_size / 2 - height / 2) /
                       precision) *
            precision;
    else if (bind_object->position.y - t_size / 2 >= border_height - height / 2)
        center.y = border_height - height;
}
point camera::get_center() {
    return center;
}

instance::instance(float x, float y, float z, int s) {
    position = vertex_2d(x, y, 0.0f, 0.0f);
    position.z_index = z;
    size.x = s;
    size.y = s;
    collision_box = point(s, s);

    update_points();
    update_data();
}

instance::instance(float x, float y, float z, int size_x, int size_y)
    : instance(x, y, z, 0) {
    size.x = size_x;
    size.y = size_y;

    collision_box = point(size_x, size_y);
}

void instance::get_points() {
    glm::mat4 transform;
    transform = glm::rotate(transform,
                            /*(GLfloat)GL_time() * */
                            alpha, glm::vec3(0.0f, 0.0f, 1.0f));

    float pixel_precision = 1.0f;
    mesh_points[0].x = 0 + collision_box_offset.x;
    mesh_points[0].y = 0 - collision_box_offset.y;

    mesh_points[1].x = 0 + collision_box_offset.x;
    mesh_points[1].y =
        -collision_box.y - collision_box_offset.y + pixel_precision;

    mesh_points[2].x =
        collision_box.x + collision_box_offset.x - pixel_precision;
    mesh_points[2].y =
        -collision_box.y - collision_box_offset.y + pixel_precision;

    mesh_points[3].x =
        collision_box.x + collision_box_offset.x - pixel_precision;
    mesh_points[3].y = 0 - collision_box_offset.y;

    for (int i = 0; i < 4; i++) {
        glm::vec4 vector =
            glm::vec4(mesh_points[i].x, mesh_points[i].y, 0.0f, 0.0f) *
            transform;

        mesh_points[i].x = vector.x;
        mesh_points[i].y = vector.y;

        mesh_points[i].x += position.x;
        mesh_points[i].y += position.y;
    }
}

void instance::update_points() {
    update_data();
    get_points();
}

void instance::update() {
    if ((animation_playing || animation_loop) && delay <= 0) {
        update_data();
        delay = delta_frame * FPS;
        selected_frame += 1;
        if (selected_frame == frames_in_animation) {
            selected_frame = 0;
            if (animation_playing) {
                animation_playing = false;
                selected_tileset = prev_tileset;
            }
        }
    } else if (animation_playing || animation_loop)
        delay -= 1;
}

void instance::play_animation(float seconds_betweeen_frames) {
    play_animation(seconds_betweeen_frames, selected_tileset);
}

void instance::play_animation(float seconds_betweeen_frames, int tileset) {
    selected_tileset = tileset % tilesets_in_texture;
    prev_tileset = tileset % tilesets_in_texture;
    animation_playing = true;
    delta_frame = seconds_betweeen_frames;
    delay = delta_frame * FPS;
}

void instance::loop_animation(float seconds_betweeen_frames) {
    loop_animation(seconds_betweeen_frames, selected_tileset);
}

void instance::loop_animation(float seconds_betweeen_frames, int tileset) {
    selected_tileset = tileset % tilesets_in_texture;
    delta_frame = seconds_betweeen_frames;
    delay = delta_frame * FPS;
    animation_loop ^= 1;
}

std::vector<float> instance::get_data() {
    return data;
}

void instance::update_data() {
    data = quad_data;
    float k_x = 1.0f / frames_in_texture;
    float k_y = 1.0f / tilesets_in_texture;

    glm::mat4 transform;
    transform = glm::rotate(transform,
                            /*(GLfloat)GL_time() * */
                            -alpha, glm::vec3(0.0f, 0.0f, 1.0f));

    for (int i = 0; i < quad_data.size(); i += STRIDE_ELEMENTS) {
        data[i] *= (size.x / t_size);
        data[i + 1] *= (size.y / t_size);
        if (alpha != 0) {
            glm::vec4 vector =
                glm::vec4(data[i], data[i + 1], 0.0f, 0.0f) * transform;

            data[i] = vector.x;
            data[i + 1] = vector.y;

            data[i] += position.x / t_size;
            data[i + 1] -= position.y / t_size;
        } else {
            data[i] += position.x / t_size;
            data[i + 1] -= position.y / t_size;
        }
        data[i + 2] = glm::clamp(position.z_index, MAX_DEPTH, MIN_DEPTH) /
                      2.0f / MAX_DEPTH;

        data[i + 3] *= k_x;
        data[i + 3] += k_x * (selected_frame % frames_in_texture);

        data[i + 4] *= k_y;
        data[i + 4] += k_y * (selected_tileset % tilesets_in_texture);
    }
}

void ui_element::update_data() {
    data = quad_data;
    float k_x = 1.0f / frames_in_texture;
    float k_y = 1.0f / tilesets_in_texture;

    glm::mat4 transform;
    transform = glm::rotate(transform,
                            /*(GLfloat)GL_time() * */
                            -alpha, glm::vec3(0.0f, 0.0f, 1.0f));

    for (int i = 0; i < quad_data.size(); i += STRIDE_ELEMENTS) {
        data[i] *= (size.x / w_w);
        data[i + 1] *= (size.y / w_h);
        if (alpha != 0) {
            glm::vec4 vector =
                glm::vec4(data[i], data[i + 1], 0.0f, 0.0f) * transform;

            data[i] = vector.x;
            data[i + 1] = vector.y;

            data[i] += position.x / w_w;
            data[i + 1] -= position.y / w_h;
        } else {
            data[i] += position.x / w_w;
            data[i + 1] -= position.y / w_h;
        }
        data[i + 2] = glm::clamp(position.z_index, MAX_DEPTH, MIN_DEPTH) /
                      2.0f / MAX_DEPTH;

        data[i + 3] *= k_x;
        data[i + 3] += k_x * (selected_frame % frames_in_texture);

        data[i + 4] *= k_y;
        data[i + 4] += k_y * (selected_tileset % tilesets_in_texture);
    }
}

ui_element::ui_element(float x,
                       float y,
                       float z,
                       int size_x,
                       int size_y,
                       texture* texture)
    : instance(x, y, z, size_x, size_y) {
    tex = texture;
}

ui_element::ui_element(float x,
                       float y,
                       float z,
                       int size_x,
                       int size_y,
                       texture* texture,
                       const std::string& t,
                       font* font)
    : ui_element(x, y, z, size_x, size_y, texture) {
    f = font;
    text = t;
}

ui_element::~ui_element() {}

life_form::life_form(float x, float y, float z, int _speed, int size)
    : instance(x, y, z, size) {
    speed = _speed;
}

instance::~instance() {
    data.clear();
}    // TODO something

life_form::~life_form() {}

light::light(float rad, point pos, vec3 col) {
    radius = rad;
    position = pos;
    color = col;
    std::cout << col.x << col.y << col.z << std::endl;
}

light::~light() {}    // TODO something

class wall : public instance {
   private:
   public:
    wall(float x, float y, float z_index, int size)
        : instance(x, y, z_index, size) {}
};

instance* create_wall(float x, float y, float z, int size) {
    instance* inst = new wall(x, y, z, size);
    return inst;
}

class engine_impl final : public engine {
   private:
    event_type e_type = event_type::other;
    SDL_Window* window = nullptr;
    GLuint shader_program;
    GLuint text_shader_program;
    GLuint light_program;
    GLuint color_program;

    GLfloat light_rendered = false;

    std::vector<float> vertex_buffer;

    int _x = 0, _y = 0;

    void GL_unbind() {
        glBindBuffer(GL_ARRAY_BUFFER, 0);    // test
        glBindVertexArray(0);
    }

    GLuint compile_shader(const GLchar* source, GLenum target) {
        GLuint shader = glCreateShader(target);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success;
        GLchar infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (!success) {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
        }

        return shader;
    }

    GLuint create_shader_program(GLuint vertex_shader, GLuint fragment_shader) {
        GLuint shader = glCreateProgram();

        glAttachShader(shader, vertex_shader);
        glAttachShader(shader, fragment_shader);

        glLinkProgram(shader);

        GLchar infoLog[512];
        GLint success;

        glGetProgramiv(shader, GL_LINK_STATUS, &success);

        if (!success) {
            glGetProgramInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
        }

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        return shader;
    }

   public:
    int CHL_init(int width, int height, int size, int fps) final {
        SDL_version compiled = {0, 0, 0};
        SDL_version linked = {0, 0, 0};

        FPS = fps;

        SDL_VERSION(&compiled);
        SDL_GetVersion(&linked);

        if (SDL_COMPILEDVERSION !=
            SDL_VERSIONNUM(linked.major, linked.minor, linked.patch)) {
            std::cerr << "warning: SDL2 compiled and linked version mismatch: "
                      << compiled << " " << linked << std::endl;
        }

        const int init_result = SDL_Init(SDL_INIT_EVERYTHING);
        if (init_result != 0) {
            const char* err_message = SDL_GetError();
            std::cerr << "error: failed call SDL_Init: " << err_message
                      << std::endl;
            return EXIT_FAILURE;
        }

        // Get device display mode
        SDL_DisplayMode displayMode;
        if (SDL_GetCurrentDisplayMode(0, &displayMode)) {
            const char* err_message = SDL_GetError();
            std::cerr << "error: failed call SDL_Init: " << err_message
                      << std::endl;
            return EXIT_FAILURE;
        }

        w_h = displayMode.h;
        w_w = displayMode.w;

        //        w_h = height;
        //        w_w = width;

        std::cout << w_w << " " << w_h << std::endl;

        window = SDL_CreateWindow("Chlorine-5", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, w_w, w_h,
                                  SDL_WINDOW_OPENGL);

        if (window == nullptr) {
            const char* err_message = SDL_GetError();
            std::cerr << "error: failed call SDL_Init: " << err_message
                      << std::endl;
            SDL_Quit();
            return EXIT_FAILURE;
        }

        SDL_GLContext gl_context = SDL_GL_CreateContext(window);
        assert(gl_context != nullptr);

        SDL_GL_SetSwapInterval(1);

        /* Openal */

        // Initialization
        ALCdevice* device;
        device = alcOpenDevice(NULL);    // select the "preferred device"

        if (device) {
            ALCcontext* context = alcCreateContext(device, NULL);
            alcMakeContextCurrent(context);
        }

        alGetError();    // clear error code

        vec3 listenerPos(0, 0, 0);    // listeners position
        vec3 listenerVel(0, 0, 0);    // listern's velocity

        // listeners orientation (forward, up)
        float listenerOri[] = {0, 0, -1, 0, 1, 0};

        alListener3f(AL_POSITION, listenerPos.x, listenerPos.y, listenerPos.z);
        alListener3f(AL_VELOCITY, listenerVel.x, listenerVel.y, listenerVel.z);
        alListenerfv(AL_ORIENTATION, listenerOri);
        if (alGetError() != AL_NO_ERROR) {
            std::cerr << "openal error: " << std::endl;
            return EXIT_FAILURE;
        }

        alDistanceModel(AL_EXPONENT_DISTANCE);

        alDopplerFactor(5.0f);
        alDopplerVelocity(50.0f);
        /* Glew */

#ifndef __ANDROID__
        GLenum glew_init = glewInit();
        if (glew_init != GLEW_OK) {
            std::cerr << "glewInit error " << glew_init << std::endl;
            SDL_Quit();

            return EXIT_FAILURE;
        }
#endif
        int gl_major_ver = 0;
        int res =
            SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &gl_major_ver);
        assert(res == 0);

        int gl_minor_ver = 0;
        res = SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &gl_minor_ver);
        assert(res == 0);

        std::cerr << "Gl:" << gl_major_ver << '.' << gl_minor_ver << std::endl;

        // test
        glViewport(0, 0, width, height);
        // test

        GLuint vertex_shader =
            compile_shader(vertex_glsl.c_str(), GL_VERTEX_SHADER);

        GLuint fragment_shader =
            compile_shader(fragment_glsl.c_str(), GL_FRAGMENT_SHADER);

        shader_program = create_shader_program(vertex_shader, fragment_shader);

        glBindAttribLocation(shader_program, 0, "position");
        glBindAttribLocation(shader_program, 1, "texCoord");

        GLuint text_vertex_shader =
            compile_shader(text_vertex_glsl.c_str(), GL_VERTEX_SHADER);

        GLuint text_fragment_shader =
            compile_shader(text_fragment_glsl.c_str(), GL_FRAGMENT_SHADER);

        text_shader_program =
            create_shader_program(text_vertex_shader, text_fragment_shader);

        glBindAttribLocation(text_shader_program, 0, "position");
        glBindAttribLocation(text_shader_program, 1, "texCoord");

        GLuint light_vertex_shader =
            compile_shader(light_vertex.c_str(), GL_VERTEX_SHADER);

        GLuint light_fragment_shader =
            compile_shader(light_fragment.c_str(), GL_FRAGMENT_SHADER);

        light_program =
            create_shader_program(light_vertex_shader, light_fragment_shader);

        glBindAttribLocation(light_program, 0, "position");

        GLuint color_vertex_shader =
            compile_shader(color_vertex.c_str(), GL_VERTEX_SHADER);

        GLuint color_fragment_shader =
            compile_shader(color_fragment.c_str(), GL_FRAGMENT_SHADER);

        color_program =
            create_shader_program(color_vertex_shader, color_fragment_shader);

        glBindAttribLocation(color_program, 0, "position");

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glAlphaFunc(GL_GREATER, 0.001);
        glEnable(GL_ALPHA_TEST);

        t_size = size;

        return EXIT_SUCCESS;
    }

    point get_mouse_pos(camera* cam) final {
        return point(
            _x * (float)virtual_w / w_w * ((float)cam->width / virtual_w) +
                cam->get_center().x,
            _y * (float)virtual_h / w_h * ((float)cam->height / virtual_h) +
                cam->get_center().y);
    }

    void GL_swap_buffers() final { SDL_GL_SwapWindow(window); }
    void GL_clear_color() final {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        light_rendered = false;
        GL_CHECK();
    }

    float GL_time() final { return SDL_GetTicks() / 1000.f; }
    event_type get_event_type() final { return e_type; }

    void add_object(instance* in, camera* cam) final {
        if ((in->position.x + in->size.x < cam->get_center().x ||
             in->position.x > cam->get_center().x + cam->width ||
             in->position.y < cam->get_center().y ||
             in->position.y - in->size.y > cam->get_center().y + cam->height)) {
            return;
        }

        for (float f : in->get_data())
            vertex_buffer.insert(vertex_buffer.end(), f);
    }

    void set_virtual_world(int _w, int _h) final {
        virtual_w = _w;
        virtual_h = _h;
    }

    void snap_tile_to_screen_pixels(int screen_pixels) final {
        w_t_size = screen_pixels;
    }

    void draw(texture* text, camera* cam, instance* inst) final {
        glUseProgram(shader_program);

        text->bind();

        glUniform1i(glGetUniformLocation(shader_program, "our_texture"), 0);

        GL_CHECK();
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        GL_CHECK();
        glBufferData(GL_ARRAY_BUFFER,
                     vertex_buffer.size() * sizeof(vertex_buffer[0]),
                     vertex_buffer.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              STRIDE_ELEMENTS * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);
        GL_CHECK();

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                              STRIDE_ELEMENTS * sizeof(GLfloat),
                              (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        GL_CHECK();
        //        GLint vertexColorLocation =
        //            glGetUniformLocation(shader_program, "our_color");
        //        glUniform4f(vertexColorLocation, red, 0.2f, 0.0f, 1.0f);

        glm::mat4 transform;

        transform = glm::translate(transform, glm::vec3(-1.0f, 1.0f, 0.0f));
        transform = glm::scale(
            transform, glm::vec3(2.0f * (t_size / (float)virtual_w),
                                 2.0f * (t_size / (float)virtual_h), 1.0f));

        if (cam != nullptr) {
            cam->update_center();
            //            float h_tiles = (float)virtual_h / t_size,
            //                  w_tiles = (float)virtual_w / t_size;
            transform = glm::scale(
                transform, glm::vec3((float)virtual_w / cam->width,
                                     (float)virtual_h / cam->height, 1.0f));
            transform = glm::translate(
                transform,
                glm::vec3(-1.0f * cam->get_center().x / t_size,
                          1.0f * cam->get_center().y / t_size, 0.0f));
        }

        GLint color_pos = glGetUniformLocation(shader_program, "our_color");
        float green = glm::sin(GL_time()) / 6;
        float blue = glm::sin(GL_time()) / 6;

        if (inst != nullptr) {
            glUniform4f(color_pos, 1.0f, /*0.4 + green*/ 1.0,
                        /*0.4 + blue*/ 1.0, text->alpha * inst->alpha_channel);
        } else
            glUniform4f(color_pos, 1.0f, /*0.4 + green*/ 1.0,
                        /*0.4 + blue*/ 1.0, text->alpha);

        GLint transformLoc = glGetUniformLocation(shader_program, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE,
                           glm::value_ptr(transform));

        GL_CHECK();
        glDrawArrays(GL_TRIANGLES, 0, vertex_buffer.size() / STRIDE_ELEMENTS);
        vertex_buffer.clear();
        glUseProgram(0);
        text->unbind();
        glDeleteBuffers(1, &vbo);
    }

    virtual void render_text(const std::string& text,
                             font* f,
                             float x,
                             float y,
                             float offset,
                             int z_pos,
                             vec3 color) final {
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 5, nullptr,
                     GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                              (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glUseProgram(text_shader_program);

        glUniform3f(glGetUniformLocation(text_shader_program, "color"), color.x,
                    color.y, color.z);
        glActiveTexture(GL_TEXTURE0);

        glm::mat4 transform;
        transform = glm::translate(
            transform, glm::vec3(-1.0f, 1.0f - (48.0f * 2 / w_h), 0.0f));
        transform = glm::scale(
            transform, glm::vec3(2.0f / (float)w_w, 2.0f / (float)w_h, 1.0f));
        GLint transformLoc =
            glGetUniformLocation(text_shader_program, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE,
                           glm::value_ptr(transform));

        // Iterate through all characters

        float z = glm::clamp(z_pos, MAX_DEPTH, MIN_DEPTH) / 2.0f / MAX_DEPTH;

        std::string::const_iterator c;
        int beginning_x = x;
        for (c = text.begin(); c != text.end(); c++) {
            character ch = f->characters[*c];

            GLfloat xpos = x + ch.bearing.x;
            GLfloat ypos = -y - (ch.size.y - ch.bearing.y);

            GLfloat w = ch.size.x;
            GLfloat h = ch.size.y;
            // Render glyph texture over quad
            glBindTexture(GL_TEXTURE_2D, ch.texture_id);
            glUniform1i(
                glGetUniformLocation(text_shader_program, "our_texture"), 0);
            // Update content of VBO memory
            glBindBuffer(GL_ARRAY_BUFFER, vbo);

            GLfloat vertices[6][5] = {
                {xpos, ypos + h, z, 0.0, 0.0},    {xpos, ypos, z, 0.0, 1.0},
                {xpos + w, ypos, z, 1.0, 1.0},

                {xpos, ypos + h, z, 0.0, 0.0},    {xpos + w, ypos, z, 1.0, 1.0},
                {xpos + w, ypos + h, z, 1.0, 0.0}};

            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            // Render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);
            x += (ch.advance >> 6);
            if (xpos > w_w - offset) {
                y += 48.0f;
                x = beginning_x;
            }
            // Now advance cursors for next glyph (note that advance
            // is number of 1/64 pixels)
            // Bitshift by 6 to get value in pixels (2^6 = 64)
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDeleteBuffers(1, &vbo);
        glUseProgram(0);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    virtual void render_ui(user_interface* ui) {
        for (ui_element* e : ui->user_interface_elements) {
            GLuint vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 5, nullptr,
                         GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                                  0);
            glEnableVertexAttribArray(0);

            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                                  (GLvoid*)(3 * sizeof(GLfloat)));
            glEnableVertexAttribArray(1);
            GL_CHECK();

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glUseProgram(shader_program);
            GL_CHECK();

            glm::mat4 transform;
            transform = glm::translate(transform, glm::vec3(-1.0f, 1.0f, 0.0f));
            transform = glm::scale(transform, glm::vec3(2.0f, 2.0f, 1.0f));
            GLint transformLoc =
                glGetUniformLocation(shader_program, "transform");
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE,
                               glm::value_ptr(transform));

            e->tex->bind();

            GL_CHECK();
            glUniform1i(glGetUniformLocation(shader_program, "our_texture"), 0);
            GL_CHECK();
            // Update content of VBO memory
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            GL_CHECK();
            e->update_data();
            std::vector<float> vertices = e->get_data();
            //            for (float v : vertices)
            //                std::cout << v << " ";
            //            std::cout << std::endl;

            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 5 * 6,
                            vertices.data());
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            GL_CHECK();
            // Render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glUseProgram(0);
            GL_CHECK();
            glBindTexture(GL_TEXTURE_2D, 0);
            GL_CHECK();
            glDeleteBuffers(1, &vbo);
            GL_CHECK();
            if (!e->text.empty())
                render_text(e->text, e->f, e->position.x + e->offset,
                            e->position.y - e->size.y + e->offset,
                            w_w - e->position.x - e->size.x + e->offset,
                            MIN_DEPTH, vec3(1.0f, 0.0f, 0.0f));
        }
    }

    void render_light(light* source, camera* cam) {
        if (cam == nullptr)
            return;

        GL_CHECK();
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        GL_CHECK();
        std::vector<float> data = quad_data;

        for (int i = 0; i < 6 * 5; i += STRIDE_ELEMENTS)
            data[i + 2] =
                glm::clamp(MAX_DEPTH, MAX_DEPTH, MIN_DEPTH) / 2.0f / MAX_DEPTH;

        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(data[0]),
                     data.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              STRIDE_ELEMENTS * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);
        GL_CHECK();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glm::mat4 transform;
        transform = glm::translate(transform, glm::vec3(-1.0f, -1.0f, 0.0f));
        transform = glm::scale(transform, glm::vec3(2.0f, 2.0f, 1.0f));

        glUseProgram(light_program);

        GLuint transform_loc = glGetUniformLocation(light_program, "transform");
        glUniformMatrix4fv(transform_loc, 1, GL_FALSE,
                           glm::value_ptr(transform));

        GLint resolution_pos =
            glGetUniformLocation(light_program, "resolution");
        glUniform2f(resolution_pos, virtual_w, virtual_h);

        GLint time_pos = glGetUniformLocation(light_program, "time");
        glUniform1f(time_pos, GL_time());

        GLint radius_pos = glGetUniformLocation(light_program, "radius");
        glUniform1f(radius_pos, source->radius * w_w / virtual_w * 2.0f);

        GLint object_pos = glGetUniformLocation(light_program, "object");
        glUniform2f(
            object_pos,
            (source->position.x - cam->get_center().x) * w_w / cam->width,
            (source->position.y - cam->get_center().y +
             source->radius * virtual_h / w_h * 2.0f) *
                    w_h / cam->height -
                w_h / 2);

        GLuint color_loc = glGetUniformLocation(light_program, "color");
        glUniform3f(color_loc, source->color.x, source->color.y,
                    source->color.z);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        glUseProgram(0);
        GL_CHECK();
        glDeleteBuffers(1, &vbo);
        GL_CHECK();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void CHL_exit() final {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return (void)EXIT_SUCCESS;
    }

    bool read_input(event& e) final {
        SDL_Event event;
        e_type = event_type::other;

        if (SDL_PollEvent(&event)) {
            const bind* binding = nullptr;

            switch (event.type) {
                case SDL_QUIT:
                    e = event::turn_off;
                    return true;
                case SDL_KEYDOWN:
                    if (check_input(event, binding)) {
                        e = binding->event_pressed;
                        e_type = event_type::pressed;
                        return true;
                    }
                    break;
                case SDL_KEYUP:
                    if (check_input(event, binding)) {
                        e = binding->event_released;
                        e_type = event_type::released;
                        return true;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        e = event::left_mouse_pressed;
                        e_type = event_type::pressed;
                        return true;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        e = event::left_mouse_released;
                        e_type = event_type::released;
                        return true;
                    }
                    break;
                case SDL_MOUSEMOTION: {
                    _x = event.motion.x;
                    _y = event.motion.y;
                } break;
                default:
                    break;
            }
        }

        return false;
    }

    ~engine_impl() {
        //        glDeleteVertexArrays(1, &vao);
        //        glDeleteBuffers(1, &vbo);
    }
};

user_interface::user_interface() {}

user_interface::~user_interface() {
    std::vector<ui_element*>::iterator e = user_interface_elements.begin();
    for (; e != user_interface_elements.end(); e++) {
        delete *e;
        user_interface_elements.erase(e);
    }
}

void user_interface::add_instance(ui_element* e) {
    user_interface_elements.insert(user_interface_elements.end(), e);
}

bool already_exist = false;

engine* create_engine() {
    if (already_exist) {
        throw std::runtime_error("engine already exist");
    }
    engine* result = new engine_impl();
    already_exist = true;
    return result;
}

void destroy_engine(engine* e) {
    if (already_exist == false) {
        throw std::runtime_error("Engine is not created");
    }
    if (nullptr == e) {
        throw std::runtime_error("Engine is null");
    }
    delete e;
}

float triangle_area(point a, point b, point c) {
    return (a.x - c.x) * (b.y - c.y) - (a.y - c.y) * (b.x - c.x);
}

bool line_intersect(point a, point b, point c, point d, point* p) {
    float a1 = triangle_area(a, b, d);
    float a2 = triangle_area(a, b, c);

    float t;
    if (a1 * a2 < 0.0f) {
        float a3 = triangle_area(c, d, a);
        float a4 = a3 + a2 - a1;
        if (a3 * a4 < 0.0f) {
            if (p != nullptr) {
                t = a3 / (a3 - a4);
                p->x = a.x + t * (b.x - a.x);
                p->y = a.y + t * (b.y - a.y);
            }
            return true;
        }
    }

    return false;
}

bool check_slow_collision(instance* one, instance* two, point* intersection_p) {
    if (std::fabs(one->position.x - two->position.x) > 3 * t_size &&
        std::fabs(one->position.y - two->position.y) > 3 * t_size)
        return false;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (line_intersect(one->mesh_points[i],
                               one->mesh_points[(i + 1) % 4],
                               two->mesh_points[j],
                               two->mesh_points[(j + 1) % 4], intersection_p)) {
                return true;
            }
        }
    }
    return false;
}

float get_direction(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2;
    float dy = y2 - y1;

    if (dx >= 0 && dy >= 0)
        return std::atan((float)dy / dx);
    else if (dx >= 0 && dy < 0)
        return (std::atan((float)dy / dx) + 2 * M_PI);
    else
        return (M_PI + std::atan((float)dy / dx));
}

float get_distance(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2;
    float dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

bool ray_cast(const point& p1,
              const point& p2,
              const std::vector<instance*>& map) {
    for (instance* inst : map) {
        for (int i = 0; i < 4; i++) {
            if (line_intersect(inst->mesh_points[i],
                               inst->mesh_points[(i + 1) % 4], p1, p2,
                               nullptr)) {
                return false;
            }
        }
    }
    return true;
}

bool ray_cast(instance* mesh,
              const point& p2,
              const std::vector<instance*>& map) {
    for (instance* inst : map) {
        for (int j = 0; j < 4; j++) {
            for (int i = 0; i < 4; i++) {
                if (line_intersect(inst->mesh_points[i],
                                   inst->mesh_points[(i + 1) % 4],
                                   mesh->mesh_points[j], p2, nullptr)) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool ray_cast(instance* mesh_one,
              instance* mesh_two,
              const std::vector<instance*>& map) {
    for (instance* inst : map) {
        for (int k = 0; k < 4; k++) {
            for (int j = 0; j < 4; j++) {
                for (int i = 0; i < 4; i++) {
                    if (line_intersect(inst->mesh_points[i],
                                       inst->mesh_points[(i + 1) % 4],
                                       mesh_one->mesh_points[j],
                                       mesh_two->mesh_points[k], nullptr)) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

}    // namespace CHL
