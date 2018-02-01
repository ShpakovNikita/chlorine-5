#include <SDL2/SDL.h>
#include <AL/al.h>
#include <AL/alc.h>
#include "headers/engine.hxx"
#include <iostream>
#include <assert.h>
#include <math.h>

namespace CHL {
void play_s(uint32_t source) {
    alSourceStop(source);
    alSourcePlay(source);
}

void play_always_s(uint32_t source) {
    alSourcePause(source);
    alSourcei(source, AL_LOOPING, AL_TRUE);
    alSourcePlay(source);
}

void stop_s(uint32_t source) {
    alSourceStop(source);
}

void pause_s(uint32_t source) {
    alSourcePause(source);
}

void delete_source(uint32_t source) {
    alDeleteSources(1, &source);
}

void set_pos_s(uint32_t source, const vec3& sourcePos) {
    alSource3f(source, AL_POSITION, sourcePos.x, sourcePos.y, sourcePos.z);
}

void set_velocity_s(uint32_t source, const vec3& velocityPos) {
    alSource3f(source, AL_VELOCITY, velocityPos.x, velocityPos.y,
               velocityPos.z);
}

void set_volume_s(uint32_t source, float volume) {
    volume = std::min(std::max(0.0f, volume), 1.0f);
    alSourcef(source, AL_GAIN, volume);
}

void pitch_s(uint32_t source, float value) {
    value = std::min(std::max(0.0f, value), 2.0f);
    alSourcef(source, AL_PITCH, value);
}

vec3 get_listener() {
    float x, y, z;
    alGetListener3f(AL_POSITION, &x, &y, &z);
    return vec3(x, y, z);
}

vec3 get_source_pos(uint32_t source) {
    float x, y, z;
    alGetSource3f(source, AL_POSITION, &x, &y, &z);
    return vec3(x, y, z);
}

float calculate_gain(gain_algorithm algo, uint32_t source) {
    float listener_pos_x, listener_pos_y, source_pos_x, source_pos_y, _z;

    alGetListener3f(AL_POSITION, &listener_pos_x, &listener_pos_y, &_z);
    alGetSource3f(source, AL_POSITION, &source_pos_x, &source_pos_y, &_z);

    float reference_distance, rolloff_factor;
    alGetSourcef(source, AL_REFERENCE_DISTANCE, &reference_distance);
    alGetSourcef(source, AL_ROLLOFF_FACTOR, &rolloff_factor);

    float distance = get_distance(listener_pos_x, listener_pos_y, source_pos_x,
                                  source_pos_y);

    switch (algo) {
        case gain_algorithm::exponent_distance: {
            return powf(distance / reference_distance, -rolloff_factor);
        }

        case gain_algorithm::inverse_distance: {
            return reference_distance /
                   (reference_distance +
                    rolloff_factor * (distance - reference_distance));
        }

        case gain_algorithm::linear_distance: {
            float max_distance;
            alGetSourcef(source, AL_MAX_DISTANCE, &max_distance);
            distance = std::min(distance, max_distance);
            return (1 - rolloff_factor * (distance - reference_distance) /
                            (max_distance - reference_distance));
        }

        case gain_algorithm::none:
            return 1;

        default:
            return -1;
    }
}

uint32_t create_new_source(sound* s, instance* i) {
    ALuint source;
    alGenSources(1, &source);

    vec3 sourcePos(i->position.x, -i->position.y, -1);
    vec3 sourceVel(0, 0, 0);

    alSourcei(source, AL_BUFFER, s->al_buffer);
    //    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcef(source, AL_PITCH, 1.0f);
    alSourcef(source, AL_GAIN, 1.0f);
    alSource3f(source, AL_POSITION, sourcePos.x, sourcePos.y, sourcePos.z);
    alSource3f(source, AL_VELOCITY, sourceVel.x, sourceVel.y, sourcePos.z);
    alSourcef(source, AL_MAX_GAIN, 1.0f);
    alSourcef(source, AL_MIN_GAIN, 0.0f);
    alSourcef(source, AL_REFERENCE_DISTANCE, 100.0f);
    alSourcef(source, AL_ROLLOFF_FACTOR, 2.0f);
    alSourcef(source, AL_MAX_DISTANCE, 250.0f);
    alSourcei(source, AL_LOOPING, AL_FALSE);
    return source;
}

void listener_update(const vec3& listenerPos) {
    alListener3f(AL_POSITION, listenerPos.x, listenerPos.y, listenerPos.z);
}

sound::sound(const std::string& file)
    : device_id(0), buffer(nullptr), buffer_size(0) {
    if (!load(file))
        throw std::runtime_error("can't load sound");
}

bool sound::load(const std::string& file) {
    alGenBuffers(1, &al_buffer);
    alGenSources(1, &al_source);

    int error;

    SDL_AudioSpec wavspec;
    uint32_t wavlen;
    uint8_t* wavbuf;

    if (!SDL_LoadWAV(file.c_str(), &wavspec, &wavbuf, &wavlen))
        return false;

    // map wav header to openal format
    ALenum format;
    switch (wavspec.format) {
        case AUDIO_U8:
        case AUDIO_S8:
            format = wavspec.channels == 2 ? AL_FORMAT_STEREO8
                                           : AL_FORMAT_MONO8;
            break;
        case AUDIO_U16:
        case AUDIO_S16:
            format = wavspec.channels == 2 ? AL_FORMAT_STEREO16
                                           : AL_FORMAT_MONO16;
            break;
        default:
            SDL_FreeWAV(wavbuf);
            return CHL_FAILURE;
    }

    alBufferData(al_buffer, format, wavbuf, wavlen, wavspec.freq);
    if ((error = alGetError()) != AL_NO_ERROR) {
        std::cerr << "openal error: " << error << std::endl;
        return CHL_FAILURE;
    }

    SDL_FreeWAV(wavbuf);
    vec3 sourcePos(0, 0, -2);    // source position
    vec3 sourceVel(0, 0, 0);     // source velocity (used for doppler
    // effect)

    alSourcei(al_source, AL_BUFFER, al_buffer);
    alSourcef(al_source, AL_PITCH, 1.0f);
    alSourcef(al_source, AL_GAIN, 1.0f);
    alSource3f(al_source, AL_POSITION, sourcePos.x, sourcePos.y, sourcePos.z);
    alSource3f(al_source, AL_VELOCITY, sourceVel.x, sourceVel.y, sourcePos.z);
    alSourcei(al_source, AL_LOOPING, AL_FALSE);

    return true;
}
void sound::play() {
    play_s(al_source);
}

void sound::play_always() {
    play_always_s(al_source);
}

void sound::pause() {
    pause_s(al_source);
}

void sound::stop() {
    stop_s(al_source);
}

sound::~sound() {
    alDeleteBuffers(1, &al_buffer);
    alDeleteSources(1, &al_source);
}

}    // namespace CHL
