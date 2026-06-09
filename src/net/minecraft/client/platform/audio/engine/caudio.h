/* caudio — clean-room cross-platform PCM playback and mixing.
 * Outputs via WASAPI (Windows), ALSA (Linux), or AudioQueue (macOS).
 * Decodes OGG (vorbiscompiler) and WAV in-memory. No third-party audio library code.
 * Implementation lives in decode/vorbiscompiler.c */

#ifndef CAUDIO_H
#define CAUDIO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t  ca_result;
typedef uint16_t ca_uint16;
typedef uint32_t ca_uint32;
typedef uint64_t ca_uint64;
typedef int64_t  ca_int64;
typedef uint8_t  ca_bool8;
typedef uint32_t ca_bool32;

#define CA_TRUE  ((ca_bool32)1)
#define CA_FALSE ((ca_bool32)0)

enum {
    CAUDIO_OK = 0,
    CAUDIO_ERROR = -1,
    CAUDIO_INVALID_ARGS = -2,
    CAUDIO_INVALID_FILE = -44,
    CAUDIO_OUT_OF_MEMORY = -4,
    CAUDIO_NOT_IMPLEMENTED = -5
};

typedef enum {
    ca_format_unknown = 0,
    ca_format_f32 = 5
} ca_format;

typedef enum {
    CA_VOICE_FLAG_STREAM            = 0x00000001,
    CA_VOICE_FLAG_DECODE            = 0x00000002,
    CA_VOICE_FLAG_LOOPING           = 0x00000020,
    CA_VOICE_FLAG_NO_SPATIALIZATION = 0x00004000
} ca_voice_flags;

typedef struct ca_mutex ca_mutex;
typedef struct ca_data_source ca_data_source;
typedef struct ca_decoder ca_decoder;
typedef struct ca_voice ca_voice;
typedef struct ca_engine ca_engine;

typedef struct {
    ca_uint32 channels;
    ca_uint32 sampleRate;
    ca_uint32 listenerCount;
} caudio_config;

typedef ca_result (*ca_ds_read_proc)(ca_data_source* src, void* out, ca_uint64 frames, ca_uint64* read);
typedef ca_result (*ca_ds_seek_proc)(ca_data_source* src, ca_uint64 frame);
typedef ca_result (*ca_ds_format_proc)(ca_data_source* src, ca_format* fmt, ca_uint32* ch, ca_uint32* rate);

typedef struct {
    ca_ds_read_proc   onRead;
    ca_ds_seek_proc   onSeek;
    ca_ds_format_proc onFormat;
} ca_data_source_vtable;

typedef struct {
    const ca_data_source_vtable* vtable;
    void* backend;
} ca_data_source_base;

struct ca_data_source {
    ca_data_source_base base;
};

struct ca_decoder {
    ca_data_source_base base;
    ca_format format;
    ca_uint32 channels;
    ca_uint32 sampleRate;
    ca_uint64 cursor;
    ca_uint64 length;
    void* backend;
    int backend_kind;
};

#define CA_STREAM_RING_FRAMES 16384

struct ca_voice {
    ca_engine* engine;
    ca_data_source* source;
    int owns_source;
    float* pcm;
    ca_uint64 pcm_frames;
    ca_uint64 cursor;
    ca_uint32 channels;
    ca_uint32 sampleRate;
    ca_uint32 flags;
    float volume;
    float pitch;
    float pos_x, pos_y, pos_z;
    float min_dist;
    float max_dist;
    float rolloff;
    int spatial;
    int playing;
    int at_end;
    double pitch_cursor;
    int streaming;
    int owns_pcm;
    float* stream_ring;
    ca_uint32 stream_ring_frames;
    ca_uint32 stream_ring_read;
    ca_uint32 stream_ring_write;
    ca_uint32 stream_ring_fill;
    ca_uint32 stream_frames_consumed;
};

#define CA_ENGINE_MAX_VOICES 256
#define CA_DEFER_FREE_MAX 128

struct ca_mutex {
    void* handle;
};

struct ca_engine {
    ca_uint32 channels;
    ca_uint32 sampleRate;
    ca_uint32 listenerCount;
    float master_volume;
    float listener_x, listener_y, listener_z;
    float listener_at_x, listener_at_y, listener_at_z;
    float listener_up_x, listener_up_y, listener_up_z;
    void* device;
    ca_mutex lock;
    ca_voice* sounds[CA_ENGINE_MAX_VOICES];
    ca_uint32 sound_count;
    float*    defer_pcm[CA_DEFER_FREE_MAX];
    float*    defer_ring[CA_DEFER_FREE_MAX];
    ca_voice* defer_voice[CA_DEFER_FREE_MAX];
    ca_uint32 defer_count;
};

/* --- Engine: lifecycle, master volume, listener ------------------------------- */
caudio_config caudio_config_init(void);
ca_result caudio_engine_init_internal(const caudio_config* cfg, ca_engine* engine);
void caudio_engine_uninit_internal(ca_engine* engine);
void ca_engine_set_volume(ca_engine* engine, float volume);
ca_result ca_engine_listener_set_position(ca_engine* engine, ca_uint32 idx, float x, float y, float z);
ca_result ca_engine_listener_set_direction(ca_engine* engine, ca_uint32 idx, float x, float y, float z);
ca_result ca_engine_listener_set_world_up(ca_engine* engine, ca_uint32 idx, float x, float y, float z);

/* --- Mutex (platform-backed) -------------------------------------------------- */
ca_result ca_mutex_init(ca_mutex* m);
void ca_mutex_uninit(ca_mutex* m);
void ca_mutex_lock(ca_mutex* m);
void ca_mutex_unlock(ca_mutex* m);

/* --- Decoder: OGG/WAV from memory --------------------------------------------- */
ca_result ca_decoder_init_memory(const void* data, size_t size, const void* cfg, ca_decoder* dec);
ca_result ca_decoder_uninit(ca_decoder* dec);

/* --- Voice: create, attach, control ------------------------------------------- */
ca_result ca_voice_init_from_file(ca_engine* engine, const char* path, ca_uint32 flags,
    void* group, void* fence, ca_voice* sound);
ca_result ca_voice_init_from_data_source(ca_engine* engine, ca_data_source* src, ca_uint32 flags,
    void* group, ca_voice* sound);
void ca_voice_init_borrowed_pcm(ca_voice* sound, ca_engine* engine, float* pcm, ca_uint64 frames,
    ca_uint32 channels, ca_uint32 sample_rate, ca_uint32 flags);
ca_result ca_voice_attach_engine(ca_engine* engine, ca_voice* sound);
void ca_voice_uninit(ca_voice* sound);
ca_result ca_voice_start(ca_voice* sound);
ca_result ca_voice_stop(ca_voice* sound);
void ca_voice_set_volume(ca_voice* sound, float volume);
void ca_voice_set_pitch(ca_voice* sound, float pitch);
void ca_voice_set_position(ca_voice* sound, float x, float y, float z);
void ca_voice_set_min_distance(ca_voice* sound, float d);
void ca_voice_set_max_distance(ca_voice* sound, float d);
void ca_voice_set_rolloff(ca_voice* sound, float r);
ca_bool32 ca_voice_is_playing(const ca_voice* sound);

#ifdef __cplusplus
}
#endif

#endif /* CAUDIO_H */
