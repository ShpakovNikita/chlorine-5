/*
 * This light engine was created by Shpakov Nikita with the learning
 * purpose. CHL engine is based on the SDL2 library, and also uses grew,
 * truetype and openAL for different purposes. It can render instances by the
 * layers (using z buffer), play animations, calculate and render lights,
 * draw your own GUI based on the textures, render text with the given ttf font,
 * size and color, play sounds dynamically using 4 distance gain models, check
 * fast and slow (by points, need for collisions with rotated instances)
 * collisions, can produce raycast, calculate some math tasks and rendrer scene
 * inside the camera. But still, this engine is quite unoptimized and have some
 * problems with the memory usage.
 */

#pragma once

#define GLEW_BUILD
#define MAX_DEPTH -10000
#define MIN_DEPTH 10000
#define STRIDE_ELEMENTS 5

#include <stdio.h>
#include <vector>
#include <string>
#include <memory>
#include <map>

namespace CHL    // chlorine
{
/*this enum you have to pass to the gain model function, to define, which one of
 * them you want to use.*/
enum class gain_algorithm {
    inverse_distance,
    linear_distance,
    exponent_distance,
    none
};

/*this events will be intercepted by the CHL engine with the help of the SDL2.
 * They will be available on the engine user side.*/
enum class event {
    /// input events
    left_pressed,
    left_released,
    right_pressed,
    right_released,
    up_pressed,
    up_released,
    down_pressed,
    down_released,
    select_pressed,
    select_released,
    start_pressed,
    start_released,
    button1_pressed,
    button1_released,
    button2_pressed,
    button2_released,
    /// mouse events
    left_mouse_pressed,
    left_mouse_released,
    /// virtual console events
    turn_off
};

/*the structs below used to simplify calculations*/
struct point {
    point(float _x, float _y) : x(_x), y(_y) {}
    point() : x(0.0f), y(0.0f) {}
    float x, y;
};

struct vertex_2d {
    vertex_2d() : x(0.f), y(0.f), z_index(0), x_t(0.f), y_t(0.f) {}
    vertex_2d(float _x, float _y, float _x_t, float _y_t)
        : x(_x), y(_y), z_index(0), x_t(_x_t), y_t(_y_t) {}

    float x, y;
    int z_index;
    float x_t, y_t;
};

struct vec3 {
    vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    float x, y, z;
};

/*this struct is used to hold the generated texture of the font's glyph and it's
 * parameters.*/
struct character {
    uint32_t texture_id;
    point size;
    point bearing;
    int advance;
};

/*this class holds every single glyph in the font. Only English fonts supported
 * now.*/
class font {
   public:
    font(std::string path, uint32_t y_size);
    virtual ~font();
    std::map<char, character> characters;
};

class instance;
class engine;

/*use this functions when creating and destroying engine. They will provide you
 * functions from the engine implementation.*/
engine* create_engine();
void destroy_engine(engine* e);

/*sometimes you have to know more info about current key*/
enum class event_type { pressed, released, other };

// AABB collision. It is really fast and good for most games.
bool check_collision(instance*, instance*);

// points intersection collision. It is slow, but it can calculate the point of
// intersection and can be useful for rotated boxes.
bool check_slow_collision(instance* one, instance* two, point*);

/*simple math functions*/
float get_direction(float x1, float y1, float x2, float y2);
float get_distance(float x1, float y1, float x2, float y2);
bool ray_cast(const point&, const point&, const std::vector<instance*>& map);
bool ray_cast(instance*, const point&, const std::vector<instance*>& map);
bool ray_cast(instance*, instance*, const std::vector<instance*>& map);

/*texture class. Just pass it into the engine's render method and it will render
 * all quads in buffer with this texture. Notice that this class have additional
 * alpha.*/
class texture {
   public:
    texture(const std::string&);
    ~texture();

    bool load_texture(const std::string&);

    void bind();
    void unbind();

    float alpha = 1.0f;

   private:
    int w;
    int h;
    unsigned int tex;
};

/*camera class. Pass camera to the render function to render the scene within
 * the camera. Initializing must be in virtual resolution of the world,
 * recommended to took window resolution divided by some value, in order to
 * perform not scaled picture. Borders defined also in virtual coordinates. */
class camera {
   public:
    int width, height;
    int border_width, border_height;
    instance* bind_object;

    void update_center();
    point get_center();    // actually, it is the top left corner.

    camera(int w, int h, int b_w, int b_h, instance*);
    virtual ~camera();

   private:
    point center;
};

/*this is the base class for the quad. It supports animations, have points to
 * detect collisions (defined by the position and collision box field) and many
 * other useful things. Instance realization placed in the engine's realization
 * source file for higher usability.*/
class instance {
   public:
    instance(float x, float y, float z, int size);
    instance(float x, float y, float z, int size_x, int size_y);
    virtual ~instance();

    vertex_2d position;
    point size;

    float weight = 1.0f;

    float alpha = 0;
    point collision_box_offset;
    point collision_box;
    int selected_frame = 0;
    int selected_tileset = 0;
    int frames_in_texture = 1;
    int tilesets_in_texture = 1;
    int frames_in_animation = 1;

    std::array<point, 4> mesh_points;
    std::string tag = "none";
    float alpha_channel = 1.0f;

    void update_points();
    virtual void update_data();
    std::vector<float> get_data();

    void update();
    void play_animation(float seconds_betweeen_frames, int tileset);
    void loop_animation(float seconds_betweeen_frames, int tileset);
    void play_animation(float seconds_betweeen_frames);
    void loop_animation(float seconds_betweeen_frames);

   protected:
    bool animation_playing = false;
    bool animation_loop = false;
    int delay;
    float delta_frame;
    std::vector<float> data;

   private:
    int prev_tileset;
};

/*the light class, which you should pass to the specific engine's function and
 * it will render the light on the lower layer. It has to be struct, as the font
 * class, but I decided to leave it as it is.*/
class light {
   public:
    light(float rad, point pos, vec3 col);
    virtual ~light();
    float radius;
    point position;
    vec3 color;
    float brightness = 1;
};

/*
 * life form used for dynamic objects (objects, that will be moving).
 * I decided to make this class a part of the engine, that will simplify engine
 * to new users in some sense.*/
class life_form : public instance {
   public:
    life_form(float x, float y, float z, int _speed, int s);
    virtual ~life_form();
    virtual void move(float) = 0;
    int speed;
    int health = 1;
    float delta_x = 0, delta_y = 0;
    point velocity_impulse;
};

/*GUI consists of this elements.*/
class ui_element : public instance {
   public:
    ui_element(float x,
               float y,
               float z,
               int size_x,
               int size_y,
               texture* texture);
    ui_element(float x,
               float y,
               float z,
               int size_x,
               int size_y,
               texture* texture,
               const std::string& t,
               font* font);
    virtual ~ui_element();
    texture* tex;
    void update_data() override;
    font* f = nullptr;
    std::string text = "";
    int offset = 70;
};

/*pass this class to the specific engine's function to render the UI over the
 * window's screen.*/
class user_interface {
   public:
    user_interface();
    virtual ~user_interface();
    void add_instance(ui_element*);
    std::vector<ui_element*> user_interface_elements;
};

/* sound procedures (shell for openAL functions). Use them for creation dynamic
 * sources and calculation distant sounds. */
void set_pos_s(uint32_t source, const vec3&);
void set_velocity_s(uint32_t source, const vec3&);
void set_volume_s(uint32_t source, float volume);
void pitch_s(uint32_t source, float value);
vec3 get_listener();
vec3 get_source_pos(uint32_t source);

void play_s(uint32_t source);
void play_always_s(uint32_t source);
void stop_s(uint32_t source);
void pause_s(uint32_t source);

float calculate_gain(gain_algorithm, uint32_t source);

void delete_source(uint32_t source);
void listener_update(const vec3&);

class sound;
uint32_t create_new_source(sound*, instance*);

/*sound class. Need for the source creation (to define the buffer of playing
 * source) or for simple playing without the distance model (or distance model
 * equals none).*/
class sound {
   public:
    sound(const std::string&);
    bool load(const std::string&);
    void play();
    void play_always();
    void stop();
    void pause();
    void volume(float v);
    ~sound();

    friend uint32_t create_new_source(sound*, instance*);

   private:
    uint32_t al_buffer;
    uint32_t al_source;
};

/*main engine class. It is needed for rendering functions and initializing other
 * features, like sound.*/
class engine {
   public:
    engine();
    virtual ~engine();

    // sdl package
    virtual void GL_clear_color() = 0;
    virtual void GL_swap_buffers() = 0;
    virtual float GL_time() = 0;

    virtual int CHL_init(int*, int*, int, int) = 0;
    virtual bool read_input(event&) = 0;
    virtual void CHL_exit() = 0;
    virtual point get_mouse_pos(camera*) = 0;
    virtual void add_object(instance*, camera*) = 0;
    virtual void render(texture*, camera*, instance*) = 0;
    virtual void render_text(const std::string& text,
                             font* f,
                             float x,
                             float y,
                             float offset,
                             int z_pos,
                             vec3 color) = 0;
    virtual void render_ui(user_interface*) = 0;
    virtual void render_light(light*, camera*) = 0;
    virtual void set_virtual_world(int, int) = 0;
    virtual event_type get_event_type() = 0;
    virtual point get_window_params() = 0;
};

std::ostream& operator<<(std::ostream& stream, const event e);

}    // namespace CHL
