// Single C translation unit: clean-room vorbiscompiler + caudio engine + slot table.
// No C++ types cross this boundary, and no Minecraft format knowledge lives here.

#define VORBISCOMPILER_NO_S16
#include "net/minecraft/client/platform/audio/decode/vorbiscompiler.c"

#define CAUDIO_IMPLEMENTATION
#include "net/minecraft/client/platform/audio/engine/caudio.h"

#include "net/minecraft/client/platform/audio/backend/audio_backend.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------- Source table ----------

#define MAX_SLOTS 256
#define NAME_CAP  128

typedef struct {
    char        name[NAME_CAP]; /* empty = free */
    ca_voice*   voice;          /* heap-allocated: address is stable, never moved */
    ca_decoder* dec;            /* heap-allocated decoder for memory sources, or NULL */
    void*       mem;            /* owned copy of encoded bytes for memory sources, or NULL */
    bool        inited;         /* true once `voice` is live */
} Slot;

struct AudioBackend {
    ca_engine engine;
    bool      ready;
    ca_mutex  lock;
    Slot      slots[MAX_SLOTS];
};

static void slot_clear(Slot* s)
{
    if (s->voice) {
        if (s->inited) {
            ca_voice_stop(s->voice);
            ca_voice_uninit(s->voice);
        }
        free(s->voice);
        s->voice = NULL;
    }
    if (s->dec) {
        ca_decoder_uninit(s->dec);
        free(s->dec);
        s->dec = NULL;
    }
    free(s->mem);
    s->mem     = NULL;
    s->inited  = false;
    s->name[0] = '\0';
}

static Slot* slot_find(AudioBackend* b, const char* name)
{
    for (int i = 0; i < MAX_SLOTS; ++i)
        if (b->slots[i].name[0] && strcmp(b->slots[i].name, name) == 0)
            return &b->slots[i];
    return NULL;
}

static Slot* slot_acquire(AudioBackend* b, const char* name)
{
    Slot* existing = slot_find(b, name);
    if (existing) { slot_clear(existing); return existing; }

    for (int i = 0; i < MAX_SLOTS; ++i)
        if (!b->slots[i].name[0])
            return &b->slots[i];

    for (int i = 0; i < MAX_SLOTS; ++i) {
        if (strcmp(b->slots[i].name, "BgMusic") != 0 &&
            strcmp(b->slots[i].name, "streaming") != 0) {
            slot_clear(&b->slots[i]);
            return &b->slots[i];
        }
    }
    return NULL;
}

static void slot_set_name(Slot* s, const char* name)
{
    strncpy(s->name, name, NAME_CAP - 1);
    s->name[NAME_CAP - 1] = '\0';
}

static ca_uint32 voice_flags(const AudioSourceParams* p)
{
    ca_uint32 f = CA_VOICE_FLAG_DECODE;
    if (p->loop)     f |= CA_VOICE_FLAG_LOOPING;
    if (p->stream)   f |= CA_VOICE_FLAG_STREAM;
    if (!p->spatial) f |= CA_VOICE_FLAG_NO_SPATIALIZATION;
    return f;
}

static bool slot_finish(Slot* s, ca_result result, const char* what, const AudioSourceParams* p)
{
    if (result != CAUDIO_OK) {
        fprintf(stderr, "audio: failed to load '%s' (error %d)\n", what, (int)result);
        slot_clear(s);
        return false;
    }
    s->inited = true;
    if (p->spatial) {
        float max_dist = p->dist > 0.0f ? p->dist : 16.0f;
        ca_voice_set_position(s->voice, p->x, p->y, p->z);
        ca_voice_set_min_distance(s->voice, 1.0f);
        ca_voice_set_max_distance(s->voice, max_dist);
        ca_voice_set_rolloff(s->voice, p->att_model == AUDIO_ATT_LINEAR ? 1.0f : 1.5f);
    } else {
        ca_voice_set_position(s->voice, 0.0f, 0.0f, 0.0f);
    }
    return true;
}

// ---------- Backend lifecycle ----------

AudioBackend* audio_backend_create(void)
{
    AudioBackend* b = (AudioBackend*)calloc(1, sizeof(AudioBackend));
    if (!b) return NULL;

    caudio_config cfg = caudio_config_init();
    cfg.channels      = 2;
    cfg.sampleRate    = 44100;
    cfg.listenerCount = 1;

    if (caudio_engine_init_internal(&cfg, &b->engine) != CAUDIO_OK) {
        free(b);
        return NULL;
    }
    if (ca_mutex_init(&b->lock) != CAUDIO_OK) {
        caudio_engine_uninit_internal(&b->engine);
        free(b);
        return NULL;
    }
    b->ready = true;
    return b;
}

static void backend_stop_all_nolock(AudioBackend* b)
{
    for (int i = 0; i < MAX_SLOTS; ++i)
        if (b->slots[i].name[0])
            slot_clear(&b->slots[i]);
}

void audio_backend_destroy(AudioBackend* b)
{
    if (!b) return;
    ca_mutex_lock(&b->lock);
    backend_stop_all_nolock(b);
    ca_mutex_unlock(&b->lock);
    if (b->ready)
        caudio_engine_uninit_internal(&b->engine);
    ca_mutex_uninit(&b->lock);
    free(b);
}

bool audio_backend_ready(const AudioBackend* b)
{
    return b && b->ready;
}

// ---------- Volume / listener ----------

void audio_set_master_volume(AudioBackend* b, float v)
{
    if (!b || !b->ready) return;
    ca_engine_set_volume(&b->engine, v);
}

void audio_set_listener_pos(AudioBackend* b, float x, float y, float z)
{
    if (!b || !b->ready) return;
    ca_mutex_lock(&b->lock);
    ca_engine_listener_set_position(&b->engine, 0, x, y, z);
    ca_mutex_unlock(&b->lock);
}

static void normalize3(float* x, float* y, float* z)
{
    float len = sqrtf(*x * *x + *y * *y + *z * *z);
    if (len > 1e-4f) { *x /= len; *y /= len; *z /= len; }
}

void audio_set_listener_dir(AudioBackend* b,
    float atX, float atY, float atZ,
    float upX, float upY, float upZ)
{
    if (!b || !b->ready) return;
    normalize3(&atX, &atY, &atZ);
    normalize3(&upX, &upY, &upZ);
    ca_mutex_lock(&b->lock);
    ca_engine_listener_set_direction(&b->engine, 0, atX, atY, atZ);
    ca_engine_listener_set_world_up(&b->engine, 0, upX, upY, upZ);
    ca_mutex_unlock(&b->lock);
}

// ---------- Source creation ----------

bool audio_source_create_file(AudioBackend* b, const char* name,
    const char* url, AudioSourceParams params)
{
    if (!b || !b->ready || !name || !url || !url[0]) return false;

    ca_mutex_lock(&b->lock);
    Slot* s = slot_acquire(b, name);
    if (!s) { ca_mutex_unlock(&b->lock); return false; }
    slot_set_name(s, name);

    s->voice = (ca_voice*)calloc(1, sizeof(ca_voice));
    if (!s->voice) { slot_clear(s); ca_mutex_unlock(&b->lock); return false; }

    ca_result r = ca_voice_init_from_file(&b->engine, url, voice_flags(&params), NULL, NULL, s->voice);
    bool ok = slot_finish(s, r, url, &params);
    ca_mutex_unlock(&b->lock);
    return ok;
}

bool audio_source_create_memory(AudioBackend* b, const char* name,
    const void* data, size_t len, AudioSourceParams params)
{
    if (!b || !b->ready || !name || !data || len == 0) return false;

    ca_mutex_lock(&b->lock);
    Slot* s = slot_acquire(b, name);
    if (!s) { ca_mutex_unlock(&b->lock); return false; }
    slot_set_name(s, name);

    s->mem = malloc(len);
    if (!s->mem) { slot_clear(s); ca_mutex_unlock(&b->lock); return false; }
    memcpy(s->mem, data, len);

    s->dec = (ca_decoder*)calloc(1, sizeof(ca_decoder));
    if (!s->dec) { slot_clear(s); ca_mutex_unlock(&b->lock); return false; }
    if (ca_decoder_init_memory(s->mem, len, NULL, s->dec) != CAUDIO_OK) {
        free(s->dec); s->dec = NULL;
        slot_clear(s);
        ca_mutex_unlock(&b->lock);
        return false;
    }

    s->voice = (ca_voice*)calloc(1, sizeof(ca_voice));
    if (!s->voice) { slot_clear(s); ca_mutex_unlock(&b->lock); return false; }

    ca_result r = ca_voice_init_from_data_source(&b->engine, (ca_data_source*)s->dec,
        voice_flags(&params), NULL, s->voice);
    bool ok = slot_finish(s, r, name, &params);
    ca_mutex_unlock(&b->lock);
    return ok;
}

// ---------- Source control ----------

void audio_source_play(AudioBackend* b, const char* name)
{
    if (!b) return;
    ca_mutex_lock(&b->lock);
    Slot* s = slot_find(b, name);
    if (s && s->inited) ca_voice_start(s->voice);
    ca_mutex_unlock(&b->lock);
}

void audio_source_stop(AudioBackend* b, const char* name)
{
    if (!b) return;
    ca_mutex_lock(&b->lock);
    Slot* s = slot_find(b, name);
    if (s && s->inited) ca_voice_stop(s->voice);
    ca_mutex_unlock(&b->lock);
}

void audio_source_remove(AudioBackend* b, const char* name)
{
    if (!b) return;
    ca_mutex_lock(&b->lock);
    Slot* s = slot_find(b, name);
    if (s) slot_clear(s);
    ca_mutex_unlock(&b->lock);
}

void audio_source_set_volume(AudioBackend* b, const char* name, float v)
{
    if (!b) return;
    ca_mutex_lock(&b->lock);
    Slot* s = slot_find(b, name);
    if (s && s->inited) ca_voice_set_volume(s->voice, v);
    ca_mutex_unlock(&b->lock);
}

void audio_source_set_pitch(AudioBackend* b, const char* name, float v)
{
    if (!b) return;
    ca_mutex_lock(&b->lock);
    Slot* s = slot_find(b, name);
    if (s && s->inited) ca_voice_set_pitch(s->voice, v);
    ca_mutex_unlock(&b->lock);
}

void audio_source_set_pos(AudioBackend* b, const char* name, float x, float y, float z)
{
    if (!b) return;
    ca_mutex_lock(&b->lock);
    Slot* s = slot_find(b, name);
    if (s && s->inited) ca_voice_set_position(s->voice, x, y, z);
    ca_mutex_unlock(&b->lock);
}

bool audio_source_playing(AudioBackend* b, const char* name)
{
    if (!b) return false;
    ca_mutex_lock(&b->lock);
    Slot* s = slot_find(b, name);
    bool playing = s && s->inited && ca_voice_is_playing(s->voice) == CA_TRUE;
    ca_mutex_unlock(&b->lock);
    return playing;
}

void audio_stop_all(AudioBackend* b)
{
    if (!b) return;
    ca_mutex_lock(&b->lock);
    backend_stop_all_nolock(b);
    ca_mutex_unlock(&b->lock);
}
