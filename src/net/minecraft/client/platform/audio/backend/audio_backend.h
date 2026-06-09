#pragma once
// Pure C interface to the audio device. No engine/caudio types cross this boundary.
// Minecraft format knowledge (.mus decryption, resource lookup) lives in AudioEngine.

#include <stddef.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AudioBackend AudioBackend;

enum {
    AUDIO_ATT_NONE    = 0,
    AUDIO_ATT_ROLLOFF = 1,
    AUDIO_ATT_LINEAR  = 2
};

// Shared parameters for creating a source. Plain POD, passed by value.
typedef struct {
    bool  loop;
    bool  stream;
    bool  spatial;
    float x, y, z;
    int   att_model;
    float dist;
} AudioSourceParams;

AudioBackend* audio_backend_create(void);
void          audio_backend_destroy(AudioBackend* b);
bool          audio_backend_ready(const AudioBackend* b);

void audio_set_master_volume(AudioBackend* b, float v);
void audio_set_listener_pos(AudioBackend* b, float x, float y, float z);
void audio_set_listener_dir(AudioBackend* b,
    float atX, float atY, float atZ,
    float upX, float upY, float upZ);

// Load and register a source under `name`, replacing any existing one.
bool audio_source_create_file(AudioBackend* b, const char* name,
    const char* url, AudioSourceParams params);
// Same, but decodes from an in-memory encoded buffer (the backend copies it).
bool audio_source_create_memory(AudioBackend* b, const char* name,
    const void* data, size_t len, AudioSourceParams params);

void audio_source_play(AudioBackend* b, const char* name);
void audio_source_stop(AudioBackend* b, const char* name);
void audio_source_remove(AudioBackend* b, const char* name);
void audio_source_set_volume(AudioBackend* b, const char* name, float v);
void audio_source_set_pitch(AudioBackend* b, const char* name, float v);
void audio_source_set_pos(AudioBackend* b, const char* name, float x, float y, float z);
bool audio_source_playing(AudioBackend* b, const char* name);
void audio_stop_all(AudioBackend* b);

#ifdef __cplusplus
}
#endif
