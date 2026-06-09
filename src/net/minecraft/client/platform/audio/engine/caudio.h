/* caudio — clean-room cross-platform PCM playback and mixing.
 * Outputs via WASAPI (Windows), ALSA (Linux), or AudioQueue (macOS).
 * Decodes OGG (vorbiscompiler) and WAV in-memory. No third-party audio library code. */

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
    float* stream_ring;
    ca_uint32 stream_ring_frames;
    ca_uint32 stream_ring_read;
    ca_uint32 stream_ring_write;
    ca_uint32 stream_ring_fill;
    ca_uint32 stream_frames_consumed;
};

#define CA_ENGINE_MAX_VOICES 256

struct ca_engine {
    ca_uint32 channels;
    ca_uint32 sampleRate;
    ca_uint32 listenerCount;
    float master_volume;
    float listener_x, listener_y, listener_z;
    float listener_at_x, listener_at_y, listener_at_z;
    float listener_up_x, listener_up_y, listener_up_z;
    void* device;
    ca_voice* sounds[CA_ENGINE_MAX_VOICES];
    ca_uint32 sound_count;
};

struct ca_mutex {
    void* handle;
};

caudio_config caudio_config_init(void);
ca_result caudio_engine_init_internal(const caudio_config* cfg, ca_engine* engine);
void caudio_engine_uninit_internal(ca_engine* engine);
void ca_engine_set_volume(ca_engine* engine, float volume);
ca_result ca_engine_listener_set_position(ca_engine* engine, ca_uint32 idx, float x, float y, float z);
ca_result ca_engine_listener_set_direction(ca_engine* engine, ca_uint32 idx, float x, float y, float z);
ca_result ca_engine_listener_set_world_up(ca_engine* engine, ca_uint32 idx, float x, float y, float z);

ca_result ca_mutex_init(ca_mutex* m);
void ca_mutex_uninit(ca_mutex* m);
void ca_mutex_lock(ca_mutex* m);
void ca_mutex_unlock(ca_mutex* m);

ca_result ca_decoder_init_memory(const void* data, size_t size, const void* cfg, ca_decoder* dec);
ca_result ca_decoder_uninit(ca_decoder* dec);

ca_result ca_voice_init_from_file(ca_engine* engine, const char* path, ca_uint32 flags,
    void* group, void* fence, ca_voice* sound);
ca_result ca_voice_init_from_data_source(ca_engine* engine, ca_data_source* src, ca_uint32 flags,
    void* group, ca_voice* sound);
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

#ifdef CAUDIO_IMPLEMENTATION

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef VORBISCOMPILER_H
#include "net/minecraft/client/platform/audio/decode/vorbiscompiler.c"
#endif

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#define INITGUID
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#elif defined(__APPLE__)
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <pthread.h>
#include <unistd.h>
#elif defined(__linux__)
#include <alsa/asoundlib.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#else
#error "Unsupported platform for cleanroom caudio subset."
#endif

typedef struct {
    volatile int running;
    ca_engine* engine;
    float* mix_buf;
    ca_uint32 mix_cap;
    ca_uint32 period_frames;
#if defined(_WIN32)
    IAudioClient* client;
    IAudioRenderClient* render;
    HANDLE event;
    HANDLE thread;
#elif defined(__APPLE__)
    AudioQueueRef queue;
    pthread_t thread;
    ca_uint32 buffer_count;
    AudioQueueBufferRef* buffers;
#elif defined(__linux__)
    snd_pcm_t* pcm;
    pthread_t thread;
#endif
} ca_device;

typedef struct { vc_stream* vorbis; } ca_vorbis_backend;

typedef struct {
    const unsigned char* data;
    size_t size;
    size_t pos;
    ca_uint32 channels;
    ca_uint32 sampleRate;
    ca_uint64 frames;
} ca_wav_backend;

static ca_result ca_vorbis_read(ca_data_source* src, void* out, ca_uint64 frames, ca_uint64* read)
{
    ca_decoder* dec = (ca_decoder*)src;
    ca_vorbis_backend* vb = (ca_vorbis_backend*)dec->backend;
    if (!vb || !vb->vorbis) return CAUDIO_INVALID_ARGS;
    int ch = (int)dec->channels;
    int got = vc_read_samples_f32_interleaved(vb->vorbis, ch, (float*)out, (int)frames * ch);
    if (read) *read = (ca_uint64)(got > 0 ? got : 0);
    dec->cursor += (ca_uint64)(got > 0 ? got : 0);
    return got > 0 ? CAUDIO_OK : CAUDIO_ERROR;
}

static ca_result ca_vorbis_seek(ca_data_source* src, ca_uint64 frame)
{
    ca_decoder* dec = (ca_decoder*)src;
    ca_vorbis_backend* vb = (ca_vorbis_backend*)dec->backend;
    if (!vb || !vb->vorbis) return CAUDIO_INVALID_ARGS;
    if (!vc_seek(vb->vorbis, (unsigned int)frame)) return CAUDIO_ERROR;
    dec->cursor = frame;
    return CAUDIO_OK;
}

static ca_result ca_vorbis_format(ca_data_source* src, ca_format* fmt, ca_uint32* ch, ca_uint32* rate)
{
    ca_decoder* dec = (ca_decoder*)src;
    if (fmt) *fmt = dec->format;
    if (ch) *ch = dec->channels;
    if (rate) *rate = dec->sampleRate;
    return CAUDIO_OK;
}

static const ca_data_source_vtable g_vorbis_vtable = { ca_vorbis_read, ca_vorbis_seek, ca_vorbis_format };

static ca_result ca_wav_read(ca_data_source* src, void* out, ca_uint64 frames, ca_uint64* read)
{
    ca_decoder* dec = (ca_decoder*)src;
    ca_wav_backend* wb = (ca_wav_backend*)dec->backend;
    if (!wb) return CAUDIO_INVALID_ARGS;
    ca_uint64 avail = wb->frames - dec->cursor;
    ca_uint64 n = frames < avail ? frames : avail;
    size_t bps = (size_t)wb->channels * sizeof(float);
    memcpy(out, wb->data + (size_t)dec->cursor * bps, (size_t)n * bps);
    dec->cursor += n;
    if (read) *read = n;
    return n > 0 ? CAUDIO_OK : CAUDIO_ERROR;
}

static ca_result ca_wav_seek(ca_data_source* src, ca_uint64 frame)
{
    ca_decoder* dec = (ca_decoder*)src;
    ca_wav_backend* wb = (ca_wav_backend*)dec->backend;
    if (!wb || frame > wb->frames) return CAUDIO_INVALID_ARGS;
    dec->cursor = frame;
    return CAUDIO_OK;
}

static ca_result ca_wav_format(ca_data_source* src, ca_format* fmt, ca_uint32* ch, ca_uint32* rate)
{
    ca_decoder* dec = (ca_decoder*)src;
    if (fmt) *fmt = dec->format;
    if (ch) *ch = dec->channels;
    if (rate) *rate = dec->sampleRate;
    return CAUDIO_OK;
}

static const ca_data_source_vtable g_wav_vtable = { ca_wav_read, ca_wav_seek, ca_wav_format };

static int ca_is_ogg(const unsigned char* d, size_t n)
{
    return n >= 4 && d[0]=='O' && d[1]=='g' && d[2]=='g' && d[3]=='S';
}

static int ca_is_wav(const unsigned char* d, size_t n)
{
    return n >= 12 && d[0]=='R' && d[1]=='I' && d[2]=='F' && d[3]=='F' &&
           d[8]=='W' && d[9]=='A' && d[10]=='V' && d[11]=='E';
}

static ca_result ca_load_wav_memory(const void* data, size_t size, ca_wav_backend* wb)
{
    const unsigned char* p = (const unsigned char*)data;
    if (!ca_is_wav(p, size)) return CAUDIO_INVALID_FILE;
    ca_uint32 channels = (ca_uint32)(p[22] | (p[23] << 8));
    ca_uint32 rate = (ca_uint32)(p[24] | (p[25]<<8) | (p[26]<<16) | (p[27]<<24));
    ca_uint16 bits = (ca_uint16)(p[34] | (p[35] << 8));
    size_t pos = 36;
    while (pos + 8 <= size) {
        ca_uint32 chunk = (ca_uint32)(p[pos] | (p[pos+1]<<8) | (p[pos+2]<<16) | (p[pos+3]<<24));
        ca_uint32 csz = (ca_uint32)(p[pos+4] | (p[pos+5]<<8) | (p[pos+6]<<16) | (p[pos+7]<<24));
        pos += 8;
        if (chunk == 0x61746164u) {
            ca_uint64 frames = csz / ((ca_uint64)channels * (bits/8));
            float* pcm = (float*)malloc((size_t)frames * channels * sizeof(float));
            if (!pcm) return CAUDIO_OUT_OF_MEMORY;
            const unsigned char* src = p + pos;
            for (ca_uint64 i = 0; i < frames * channels; ++i) {
                if (bits == 16) {
                    int idx = (int)(i * 2);
                    int16_t s = (int16_t)(src[idx] | (src[idx+1] << 8));
                    pcm[i] = (float)s / 32768.0f;
                } else if (bits == 8) {
                    pcm[i] = ((float)src[i] - 128.0f) / 128.0f;
                } else {
                    free(pcm);
                    return CAUDIO_INVALID_FILE;
                }
            }
            wb->data = (const unsigned char*)pcm;
            wb->size = (size_t)frames * channels * sizeof(float);
            wb->pos = 0;
            wb->channels = channels;
            wb->sampleRate = rate;
            wb->frames = frames;
            return CAUDIO_OK;
        }
        pos += csz;
    }
    return CAUDIO_INVALID_FILE;
}

caudio_config caudio_config_init(void)
{
    caudio_config c;
    memset(&c, 0, sizeof(c));
    c.listenerCount = 1;
    return c;
}

static float ca_spatial_gain(const ca_engine* e, const ca_voice* s)
{
    if (!s->spatial) return 1.0f;
    float dx = s->pos_x - e->listener_x;
    float dy = s->pos_y - e->listener_y;
    float dz = s->pos_z - e->listener_z;
    float dist = sqrtf(dx*dx + dy*dy + dz*dz);
    if (dist <= s->min_dist) return 1.0f;
    if (dist >= s->max_dist) return 0.0f;
    float t = (dist - s->min_dist) / (s->max_dist - s->min_dist);
    if (s->rolloff <= 1.0f) return 1.0f - t;
    return 1.0f / (1.0f + s->rolloff * t * 4.0f);
}

static double ca_pitch_step(const ca_engine* e, const ca_voice* s)
{
    if (!e || !s || s->sampleRate == 0) return 1.0;
    return (double)s->pitch * (double)s->sampleRate / (double)e->sampleRate;
}

static float ca_lerp_sample(const float* pcm, ca_uint64 frames, ca_uint32 channels,
    double pos, ca_uint32 out_ch)
{
    if (!pcm || frames == 0 || pos < 0.0) return 0.0f;
    ca_uint64 max_frame = frames - 1;
    if (pos >= (double)frames) {
        ca_uint32 sc = out_ch < channels ? out_ch : 0;
        return pcm[max_frame * channels + sc];
    }
    ca_uint64 i0 = (ca_uint64)pos;
    ca_uint64 i1 = i0 + 1 < frames ? i0 + 1 : i0;
    float frac = (float)(pos - (double)i0);
    ca_uint32 sc = out_ch < channels ? out_ch : 0;
    float a = pcm[i0 * channels + sc];
    float b = pcm[i1 * channels + sc];
    return a + (b - a) * frac;
}

static float ca_ring_sample(const ca_voice* s, double pos, ca_uint32 out_ch)
{
    if (!s->stream_ring || s->stream_ring_frames == 0 || s->stream_ring_fill == 0) return 0.0f;
    if (pos < 0.0) return 0.0f;
    ca_uint32 cap = s->stream_ring_frames;
    ca_uint32 ch = s->channels;
    ca_uint32 avail = s->stream_ring_fill;
    if (pos >= (double)avail) {
        ca_uint32 sc = out_ch < ch ? out_ch : 0;
        ca_uint32 last = (s->stream_ring_read + avail - 1) % cap;
        return s->stream_ring[last * ch + sc];
    }
    ca_uint32 i0 = (ca_uint32)pos;
    ca_uint32 i1 = i0 + 1 < avail ? i0 + 1 : i0;
    float frac = (float)(pos - (double)i0);
    ca_uint32 sc = out_ch < ch ? out_ch : 0;
    ca_uint32 idx0 = (s->stream_ring_read + i0) % cap;
    ca_uint32 idx1 = (s->stream_ring_read + i1) % cap;
    float a = s->stream_ring[idx0 * ch + sc];
    float b = s->stream_ring[idx1 * ch + sc];
    return a + (b - a) * frac;
}

static void ca_stream_ring_push(ca_voice* s, const float* data, ca_uint32 frames)
{
    if (!s->stream_ring || frames == 0) return;
    ca_uint32 cap = s->stream_ring_frames;
    ca_uint32 ch = s->channels;
    for (ca_uint32 i = 0; i < frames; ++i) {
        if (s->stream_ring_fill >= cap) break;
        ca_uint32 wi = s->stream_ring_write;
        for (ca_uint32 c = 0; c < ch; ++c)
            s->stream_ring[wi * ch + c] = data[i * ch + c];
        s->stream_ring_write = (wi + 1) % cap;
        ++s->stream_ring_fill;
    }
}

static void ca_stream_ring_discard(ca_voice* s, ca_uint32 frames)
{
    if (!s->stream_ring || frames == 0) return;
    ca_uint32 drop = frames < s->stream_ring_fill ? frames : s->stream_ring_fill;
    s->stream_ring_read = (s->stream_ring_read + drop) % s->stream_ring_frames;
    s->stream_ring_fill -= drop;
}

static void ca_voice_stream_refill(ca_voice* s, ca_uint32 target_frames)
{
    if (!s->streaming || !s->source) return;
    float chunk[4096];
    ca_uint32 ch = s->channels;
    while (s->stream_ring_fill < target_frames) {
        ca_uint32 want = 1024;
        if (want * ch > 4096) want = 4096 / ch;
        ca_uint64 got = 0;
        ca_result r = s->source->base.vtable->onRead(s->source, chunk, want, &got);
        if (r != CAUDIO_OK || got == 0) {
            if (got == 0) s->at_end = 1;
            break;
        }
        ca_stream_ring_push(s, chunk, (ca_uint32)got);
    }
}

static float ca_voice_sample(const ca_voice* s, double src_pos, ca_uint32 out_ch)
{
    if (s->streaming) {
        double ring_pos = src_pos - (double)s->stream_frames_consumed;
        return ca_ring_sample(s, ring_pos, out_ch);
    }
    return ca_lerp_sample(s->pcm, s->pcm_frames, s->channels, src_pos, out_ch);
}

static void ca_engine_mix(ca_engine* engine, float* out, ca_uint32 frames)
{
    ca_uint32 ch = engine->channels;
    memset(out, 0, (size_t)frames * ch * sizeof(float));
    for (ca_uint32 si = 0; si < engine->sound_count; ++si) {
        ca_voice* s = engine->sounds[si];
        if (!s || !s->playing || s->at_end) continue;
        float gain = engine->master_volume * s->volume * ca_spatial_gain(engine, s);
        if (gain <= 0.0f) continue;
        if (s->streaming) {
            ca_voice_stream_refill(s, 4096);
            if (s->stream_ring_fill == 0 && s->at_end) { s->playing = 0; continue; }
        } else if (!s->pcm || s->pcm_frames == 0) {
            continue;
        }
        double step = ca_pitch_step(engine, s);
        ca_uint32 produced = 0;
        for (ca_uint32 i = 0; i < frames; ++i) {
            double src_pos = s->pitch_cursor + (double)i * step;
            if (s->streaming) {
                if (src_pos >= (double)(s->stream_frames_consumed + s->stream_ring_fill)) {
                    if (s->at_end) { s->playing = 0; break; }
                    ca_voice_stream_refill(s, (ca_uint32)src_pos - s->stream_frames_consumed + 2048);
                    if (src_pos >= (double)(s->stream_frames_consumed + s->stream_ring_fill) && s->at_end) {
                        s->playing = 0; break;
                    }
                }
            } else if (src_pos >= (double)s->pcm_frames) {
                if (s->flags & CA_VOICE_FLAG_LOOPING && s->pcm_frames > 0)
                    src_pos = fmod(src_pos, (double)s->pcm_frames);
                else { s->at_end = 1; s->playing = 0; break; }
            }
            for (ca_uint32 c = 0; c < ch; ++c)
                out[i * ch + c] += ca_voice_sample(s, src_pos, c) * gain;
            produced = i + 1;
        }
        if (produced > 0) {
            s->pitch_cursor += (double)produced * step;
            if (s->streaming) {
                ca_uint32 new_consumed = (ca_uint32)s->pitch_cursor;
                if (new_consumed > s->stream_frames_consumed) {
                    ca_stream_ring_discard(s, new_consumed - s->stream_frames_consumed);
                    s->stream_frames_consumed = new_consumed;
                }
            }
        }
        if (!s->streaming && s->pitch_cursor >= (double)s->pcm_frames) {
            if (s->flags & CA_VOICE_FLAG_LOOPING && s->pcm_frames > 0)
                s->pitch_cursor = fmod(s->pitch_cursor, (double)s->pcm_frames);
            else { s->at_end = 1; s->playing = 0; }
        }
    }
}

#if defined(_WIN32)
static DWORD WINAPI ca_device_thread(LPVOID param)
{
    ca_device* dev = (ca_device*)param;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    while (dev->running) {
        WaitForSingleObject(dev->event, INFINITE);
        if (!dev->running) break;
        UINT32 padding = 0;
        dev->client->lpVtbl->GetCurrentPadding(dev->client, &padding);
        UINT32 buffer_frames = 0;
        dev->client->lpVtbl->GetBufferSize(dev->client, &buffer_frames);
        UINT32 need = buffer_frames - padding;
        if (need == 0) continue;
        BYTE* dst = NULL;
        dev->render->lpVtbl->GetBuffer(dev->render, need, &dst);
        ca_uint32 total = need * dev->engine->channels;
        if (dev->mix_cap < total) {
            float* nb = (float*)realloc(dev->mix_buf, (size_t)total * sizeof(float));
            if (nb) { dev->mix_buf = nb; dev->mix_cap = total; }
        }
        if (dev->mix_buf) {
            ca_engine_mix(dev->engine, dev->mix_buf, need);
            memcpy(dst, dev->mix_buf, (size_t)total * sizeof(float));
        } else {
            memset(dst, 0, (size_t)total * sizeof(float));
        }
        dev->render->lpVtbl->ReleaseBuffer(dev->render, need, 0);
    }
    CoUninitialize();
    return 0;
}
#elif defined(__APPLE__)
static void ca_audio_queue_callback(void* user, AudioQueueRef queue, AudioQueueBufferRef buf)
{
    ca_device* dev = (ca_device*)user;
    if (!dev || !dev->running || !dev->engine) {
        memset(buf->mAudioData, 0, buf->mAudioDataByteSize);
        AudioQueueEnqueueBuffer(queue, buf, 0, NULL);
        return;
    }
    ca_uint32 frames = dev->period_frames;
    ca_uint32 total = frames * dev->engine->channels;
    if (dev->mix_cap < total) {
        float* nb = (float*)realloc(dev->mix_buf, (size_t)total * sizeof(float));
        if (nb) { dev->mix_buf = nb; dev->mix_cap = total; }
    }
    if (dev->mix_buf) {
        ca_engine_mix(dev->engine, dev->mix_buf, frames);
        memcpy(buf->mAudioData, dev->mix_buf, (size_t)total * sizeof(float));
    } else {
        memset(buf->mAudioData, 0, buf->mAudioDataByteSize);
    }
    AudioQueueEnqueueBuffer(dev->queue, buf, 0, NULL);
}

static void* ca_device_thread(void* param)
{
    ca_device* dev = (ca_device*)param;
    while (dev->running) usleep(20000);
    return NULL;
}
#elif defined(__linux__)
static void* ca_device_thread(void* param)
{
    ca_device* dev = (ca_device*)param;
    ca_uint32 frames = dev->period_frames;
    ca_uint32 total = frames * dev->engine->channels;
    if (dev->mix_cap < total) {
        dev->mix_buf = (float*)realloc(dev->mix_buf, (size_t)total * sizeof(float));
        if (dev->mix_buf) dev->mix_cap = total;
    }
    while (dev->running) {
        if (dev->mix_buf) {
            ca_engine_mix(dev->engine, dev->mix_buf, frames);
            snd_pcm_sframes_t written = snd_pcm_writei(dev->pcm, dev->mix_buf, (snd_pcm_uframes_t)frames);
            if (written == -EPIPE) snd_pcm_prepare(dev->pcm);
            else if (written < 0) snd_pcm_recover(dev->pcm, (int)written, 0);
        } else {
            usleep(20000);
        }
    }
    return NULL;
}
#endif

static ca_result ca_device_start(ca_engine* engine, ca_device** out_dev)
{
    ca_device* dev = (ca_device*)calloc(1, sizeof(*dev));
    if (!dev) return CAUDIO_OUT_OF_MEMORY;
    dev->engine = engine;
    dev->period_frames = engine->sampleRate / 50;
    if (dev->period_frames < 256) dev->period_frames = 256;
    dev->running = 1;

#if defined(_WIN32)
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    IMMDeviceEnumerator* enumerator = NULL;
    IMMDevice* device = NULL;
    if (FAILED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
            &IID_IMMDeviceEnumerator, (void**)&enumerator))) { free(dev); return CAUDIO_ERROR; }
    if (FAILED(enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device))) {
        enumerator->lpVtbl->Release(enumerator); free(dev); return CAUDIO_ERROR;
    }
    if (FAILED(device->lpVtbl->Activate(device, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&dev->client))) {
        device->lpVtbl->Release(device); enumerator->lpVtbl->Release(enumerator); free(dev); return CAUDIO_ERROR;
    }
    WAVEFORMATEX fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = (WORD)engine->channels;
    fmt.nSamplesPerSec = engine->sampleRate;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = (WORD)(fmt.nChannels * 4);
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    if (FAILED(dev->client->lpVtbl->Initialize(dev->client, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 10000000 / 20, 0, &fmt, NULL))) {
        dev->client->lpVtbl->Release(dev->client); device->lpVtbl->Release(device);
        enumerator->lpVtbl->Release(enumerator); free(dev); return CAUDIO_ERROR;
    }
    if (FAILED(dev->client->lpVtbl->GetService(dev->client, &IID_IAudioRenderClient, (void**)&dev->render))) {
        dev->client->lpVtbl->Release(dev->client); device->lpVtbl->Release(device);
        enumerator->lpVtbl->Release(enumerator); free(dev); return CAUDIO_ERROR;
    }
    dev->event = CreateEventA(NULL, FALSE, FALSE, NULL);
    dev->client->lpVtbl->SetEventHandle(dev->client, dev->event);
    dev->thread = CreateThread(NULL, 0, ca_device_thread, dev, 0, NULL);
    dev->client->lpVtbl->Start(dev->client);
    device->lpVtbl->Release(device);
    enumerator->lpVtbl->Release(enumerator);
#elif defined(__APPLE__)
    AudioStreamBasicDescription asbd;
    memset(&asbd, 0, sizeof(asbd));
    asbd.mSampleRate = engine->sampleRate;
    asbd.mFormatID = kAudioFormatLinearPCM;
    asbd.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    asbd.mBitsPerChannel = 32;
    asbd.mChannelsPerFrame = engine->channels;
    asbd.mFramesPerPacket = 1;
    asbd.mBytesPerFrame = engine->channels * sizeof(float);
    asbd.mBytesPerPacket = asbd.mBytesPerFrame;
    if (AudioQueueNewOutput(&asbd, ca_audio_queue_callback, dev, NULL, NULL, 0, &dev->queue) != noErr) {
        free(dev); return CAUDIO_ERROR;
    }
    dev->buffer_count = 3;
    dev->buffers = (AudioQueueBufferRef*)calloc(dev->buffer_count, sizeof(AudioQueueBufferRef));
    if (!dev->buffers) { AudioQueueDispose(dev->queue, CA_TRUE); free(dev); return CAUDIO_OUT_OF_MEMORY; }
    ca_uint32 bytes = dev->period_frames * engine->channels * sizeof(float);
    for (ca_uint32 i = 0; i < dev->buffer_count; ++i) {
        if (AudioQueueAllocateBuffer(dev->queue, bytes, &dev->buffers[i]) != noErr) {
            AudioQueueDispose(dev->queue, CA_TRUE); free(dev->buffers); free(dev); return CAUDIO_ERROR;
        }
        dev->buffers[i]->mAudioDataByteSize = bytes;
        memset(dev->buffers[i]->mAudioData, 0, bytes);
        AudioQueueEnqueueBuffer(dev->queue, dev->buffers[i], 0, NULL);
    }
    if (AudioQueueStart(dev->queue, NULL) != noErr) {
        AudioQueueDispose(dev->queue, CA_TRUE); free(dev->buffers); free(dev); return CAUDIO_ERROR;
    }
    pthread_create(&dev->thread, NULL, ca_device_thread, dev);
#elif defined(__linux__)
    if (snd_pcm_open(&dev->pcm, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) { free(dev); return CAUDIO_ERROR; }
    snd_pcm_hw_params_t* hw = NULL;
    snd_pcm_hw_params_alloca(&hw);
    snd_pcm_hw_params_any(dev->pcm, hw);
    snd_pcm_hw_params_set_access(dev->pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(dev->pcm, hw, SND_PCM_FORMAT_FLOAT_LE);
    snd_pcm_hw_params_set_channels(dev->pcm, hw, engine->channels);
    unsigned int rate = engine->sampleRate;
    snd_pcm_hw_params_set_rate_near(dev->pcm, hw, &rate, NULL);
    snd_pcm_uframes_t period = dev->period_frames;
    snd_pcm_hw_params_set_period_size_near(dev->pcm, hw, &period, NULL);
    if (snd_pcm_hw_params(dev->pcm, hw) < 0) { snd_pcm_close(dev->pcm); free(dev); return CAUDIO_ERROR; }
    snd_pcm_prepare(dev->pcm);
    pthread_create(&dev->thread, NULL, ca_device_thread, dev);
#endif
    *out_dev = dev;
    return CAUDIO_OK;
}

static void ca_device_stop(ca_device* dev)
{
    if (!dev) return;
    dev->running = 0;
#if defined(_WIN32)
    if (dev->event) SetEvent(dev->event);
    if (dev->thread) { WaitForSingleObject(dev->thread, INFINITE); CloseHandle(dev->thread); }
    if (dev->client) dev->client->lpVtbl->Stop(dev->client);
    if (dev->render) dev->render->lpVtbl->Release(dev->render);
    if (dev->client) dev->client->lpVtbl->Release(dev->client);
    if (dev->event) CloseHandle(dev->event);
#elif defined(__APPLE__)
    if (dev->queue) { AudioQueueStop(dev->queue, CA_TRUE); AudioQueueDispose(dev->queue, CA_TRUE); }
    if (dev->thread) pthread_join(dev->thread, NULL);
    free(dev->buffers);
#elif defined(__linux__)
    if (dev->thread) pthread_join(dev->thread, NULL);
    if (dev->pcm) snd_pcm_close(dev->pcm);
#endif
    free(dev->mix_buf);
    free(dev);
}

ca_result caudio_engine_init_internal(const caudio_config* cfg, ca_engine* engine)
{
    if (!cfg || !engine) return CAUDIO_INVALID_ARGS;
    memset(engine, 0, sizeof(*engine));
    engine->channels = cfg->channels ? cfg->channels : 2;
    engine->sampleRate = cfg->sampleRate ? cfg->sampleRate : 44100;
    engine->listenerCount = cfg->listenerCount ? cfg->listenerCount : 1;
    engine->master_volume = 1.0f;
    engine->listener_at_z = -1.0f;
    engine->listener_up_y = 1.0f;
    ca_device* dev = NULL;
    ca_result r = ca_device_start(engine, &dev);
    if (r != CAUDIO_OK) return r;
    engine->device = dev;
    return CAUDIO_OK;
}

void caudio_engine_uninit_internal(ca_engine* engine)
{
    if (!engine) return;
    ca_device_stop((ca_device*)engine->device);
    engine->device = NULL;
}

void ca_engine_set_volume(ca_engine* engine, float volume) { if (engine) engine->master_volume = volume; }

ca_result ca_engine_listener_set_position(ca_engine* engine, ca_uint32 idx, float x, float y, float z)
{
    (void)idx; if (!engine) return CAUDIO_INVALID_ARGS;
    engine->listener_x = x; engine->listener_y = y; engine->listener_z = z;
    return CAUDIO_OK;
}

ca_result ca_engine_listener_set_direction(ca_engine* engine, ca_uint32 idx, float x, float y, float z)
{
    (void)idx; if (!engine) return CAUDIO_INVALID_ARGS;
    engine->listener_at_x = x; engine->listener_at_y = y; engine->listener_at_z = z;
    return CAUDIO_OK;
}

ca_result ca_engine_listener_set_world_up(ca_engine* engine, ca_uint32 idx, float x, float y, float z)
{
    (void)idx; if (!engine) return CAUDIO_INVALID_ARGS;
    engine->listener_up_x = x; engine->listener_up_y = y; engine->listener_up_z = z;
    return CAUDIO_OK;
}

ca_result ca_mutex_init(ca_mutex* m)
{
    if (!m) return CAUDIO_INVALID_ARGS;
#if defined(_WIN32)
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
    if (!cs) return CAUDIO_OUT_OF_MEMORY;
    InitializeCriticalSection(cs);
    m->handle = cs;
#else
    pthread_mutex_t* pm = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (!pm) return CAUDIO_OUT_OF_MEMORY;
    if (pthread_mutex_init(pm, NULL) != 0) { free(pm); return CAUDIO_ERROR; }
    m->handle = pm;
#endif
    return CAUDIO_OK;
}

void ca_mutex_uninit(ca_mutex* m)
{
    if (!m || !m->handle) return;
#if defined(_WIN32)
    DeleteCriticalSection((CRITICAL_SECTION*)m->handle);
#else
    pthread_mutex_destroy((pthread_mutex_t*)m->handle);
#endif
    free(m->handle);
    m->handle = NULL;
}

void ca_mutex_lock(ca_mutex* m)
{
    if (!m || !m->handle) return;
#if defined(_WIN32)
    EnterCriticalSection((CRITICAL_SECTION*)m->handle);
#else
    pthread_mutex_lock((pthread_mutex_t*)m->handle);
#endif
}

void ca_mutex_unlock(ca_mutex* m)
{
    if (!m || !m->handle) return;
#if defined(_WIN32)
    LeaveCriticalSection((CRITICAL_SECTION*)m->handle);
#else
    pthread_mutex_unlock((pthread_mutex_t*)m->handle);
#endif
}

ca_result ca_decoder_init_memory(const void* data, size_t size, const void* cfg, ca_decoder* dec)
{
    (void)cfg;
    if (!data || size == 0 || !dec) return CAUDIO_INVALID_ARGS;
    memset(dec, 0, sizeof(*dec));
    const unsigned char* p = (const unsigned char*)data;
    if (ca_is_ogg(p, size)) {
        ca_vorbis_backend* vb = (ca_vorbis_backend*)calloc(1, sizeof(*vb));
        if (!vb) return CAUDIO_OUT_OF_MEMORY;
        int err = 0;
        vb->vorbis = vc_open_memory(p, (int)size, &err, NULL);
        if (!vb->vorbis) { free(vb); return CAUDIO_INVALID_FILE; }
        vc_stream_info info = vc_get_info(vb->vorbis);
        dec->backend = vb;
        dec->backend_kind = 1;
        dec->base.vtable = &g_vorbis_vtable;
        dec->base.backend = dec;
        dec->format = ca_format_f32;
        dec->channels = (ca_uint32)info.channels;
        dec->sampleRate = info.sample_rate;
        dec->length = vc_stream_length_samples(vb->vorbis);
        return CAUDIO_OK;
    }
    if (ca_is_wav(p, size)) {
        ca_wav_backend* wb = (ca_wav_backend*)calloc(1, sizeof(*wb));
        if (!wb) return CAUDIO_OUT_OF_MEMORY;
        ca_result r = ca_load_wav_memory(data, size, wb);
        if (r != CAUDIO_OK) { free(wb); return r; }
        dec->backend = wb;
        dec->backend_kind = 2;
        dec->base.vtable = &g_wav_vtable;
        dec->base.backend = dec;
        dec->format = ca_format_f32;
        dec->channels = wb->channels;
        dec->sampleRate = wb->sampleRate;
        dec->length = wb->frames;
        return CAUDIO_OK;
    }
    return CAUDIO_INVALID_FILE;
}

ca_result ca_decoder_uninit(ca_decoder* dec)
{
    if (!dec) return CAUDIO_INVALID_ARGS;
    if (dec->backend_kind == 1) {
        ca_vorbis_backend* vb = (ca_vorbis_backend*)dec->backend;
        if (vb) { if (vb->vorbis) vc_close(vb->vorbis); free(vb); }
    } else if (dec->backend_kind == 2) {
        ca_wav_backend* wb = (ca_wav_backend*)dec->backend;
        if (wb) { free((void*)wb->data); free(wb); }
    }
    memset(dec, 0, sizeof(*dec));
    return CAUDIO_OK;
}

static ca_result ca_voice_decode_all(ca_voice* s)
{
    if (!s->source) return CAUDIO_INVALID_ARGS;
    ca_format fmt; ca_uint32 ch, rate;
    s->source->base.vtable->onFormat(s->source, &fmt, &ch, &rate);
    s->channels = ch; s->sampleRate = rate;
    ca_uint64 cap = 4096, total = 0;
    float* buf = (float*)malloc((size_t)cap * ch * sizeof(float));
    if (!buf) return CAUDIO_OUT_OF_MEMORY;
    for (;;) {
        ca_uint64 got = 0;
        ca_result r = s->source->base.vtable->onRead(s->source, buf + total * ch, cap - total, &got);
        if (r != CAUDIO_OK || got == 0) break;
        total += got;
        if (total + 4096 > cap) {
            cap *= 2;
            float* nb = (float*)realloc(buf, (size_t)cap * ch * sizeof(float));
            if (!nb) { free(buf); return CAUDIO_OUT_OF_MEMORY; }
            buf = nb;
        }
    }
    s->pcm = buf;
    s->pcm_frames = total;
    s->cursor = 0;
    return CAUDIO_OK;
}

static ca_result ca_voice_init_streaming(ca_voice* s)
{
    if (!s->source) return CAUDIO_INVALID_ARGS;
    ca_format fmt; ca_uint32 ch, rate;
    s->source->base.vtable->onFormat(s->source, &fmt, &ch, &rate);
    s->channels = ch;
    s->sampleRate = rate;
    s->streaming = 1;
    s->stream_ring_frames = CA_STREAM_RING_FRAMES;
    s->stream_ring = (float*)calloc((size_t)s->stream_ring_frames * ch, sizeof(float));
    if (!s->stream_ring) return CAUDIO_OUT_OF_MEMORY;
    ca_voice_stream_refill(s, 4096);
    return CAUDIO_OK;
}

ca_result ca_voice_init_from_data_source(ca_engine* engine, ca_data_source* src, ca_uint32 flags,
    void* group, ca_voice* sound)
{
    (void)group;
    if (!engine || !src || !sound) return CAUDIO_INVALID_ARGS;
    memset(sound, 0, sizeof(*sound));
    sound->engine = engine;
    sound->source = src;
    sound->flags = flags;
    sound->volume = 1.0f;
    sound->pitch = 1.0f;
    sound->min_dist = 1.0f;
    sound->max_dist = 16.0f;
    sound->rolloff = 1.5f;
    sound->spatial = !(flags & CA_VOICE_FLAG_NO_SPATIALIZATION);
    if (flags & CA_VOICE_FLAG_DECODE) {
        ca_result r = (flags & CA_VOICE_FLAG_STREAM) ? ca_voice_init_streaming(sound) : ca_voice_decode_all(sound);
        if (r != CAUDIO_OK) return r;
    }
    if (engine->sound_count < CA_ENGINE_MAX_VOICES)
        engine->sounds[engine->sound_count++] = sound;
    return CAUDIO_OK;
}

static ca_result ca_read_file(const char* path, unsigned char** out, size_t* out_len)
{
    FILE* f = fopen(path, "rb");
    if (!f) return CAUDIO_INVALID_FILE;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return CAUDIO_INVALID_FILE; }
    unsigned char* buf = (unsigned char*)malloc((size_t)sz);
    if (!buf) { fclose(f); return CAUDIO_OUT_OF_MEMORY; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return CAUDIO_ERROR; }
    fclose(f);
    *out = buf;
    *out_len = (size_t)sz;
    return CAUDIO_OK;
}

ca_result ca_voice_init_from_file(ca_engine* engine, const char* path, ca_uint32 flags,
    void* group, void* fence, ca_voice* sound)
{
    (void)fence; (void)group;
    unsigned char* data = NULL;
    size_t len = 0;
    ca_result r = ca_read_file(path, &data, &len);
    if (r != CAUDIO_OK) return r;
    ca_decoder* dec = (ca_decoder*)calloc(1, sizeof(*dec));
    if (!dec) { free(data); return CAUDIO_OUT_OF_MEMORY; }
    r = ca_decoder_init_memory(data, len, NULL, dec);
    free(data);
    if (r != CAUDIO_OK) { free(dec); return r; }
    r = ca_voice_init_from_data_source(engine, (ca_data_source*)dec, flags, NULL, sound);
    if (r != CAUDIO_OK) { ca_decoder_uninit(dec); free(dec); return r; }
    sound->owns_source = 1;
    return CAUDIO_OK;
}

void ca_voice_uninit(ca_voice* sound)
{
    if (!sound) return;
    if (sound->engine) {
        for (ca_uint32 i = 0; i < sound->engine->sound_count; ++i) {
            if (sound->engine->sounds[i] == sound) {
                sound->engine->sounds[i] = sound->engine->sounds[--sound->engine->sound_count];
                break;
            }
        }
    }
    free(sound->pcm);
    free(sound->stream_ring);
    if (sound->owns_source && sound->source) {
        ca_decoder_uninit((ca_decoder*)sound->source);
        free(sound->source);
    }
    memset(sound, 0, sizeof(*sound));
}

ca_result ca_voice_start(ca_voice* sound)
{
    if (!sound) return CAUDIO_INVALID_ARGS;
    sound->playing = 1;
    sound->at_end = 0;
    sound->cursor = 0;
    sound->pitch_cursor = 0.0;
    if (sound->streaming) {
        sound->stream_ring_read = 0;
        sound->stream_ring_write = 0;
        sound->stream_ring_fill = 0;
        sound->stream_frames_consumed = 0;
        sound->at_end = 0;
        ca_voice_stream_refill(sound, 4096);
    }
    return CAUDIO_OK;
}

ca_result ca_voice_stop(ca_voice* sound)
{
    if (!sound) return CAUDIO_INVALID_ARGS;
    sound->playing = 0;
    return CAUDIO_OK;
}

void ca_voice_set_volume(ca_voice* sound, float volume) { if (sound) sound->volume = volume; }
void ca_voice_set_pitch(ca_voice* sound, float pitch) { if (sound) sound->pitch = pitch > 0 ? pitch : 1.0f; }
void ca_voice_set_position(ca_voice* sound, float x, float y, float z)
{
    if (!sound) return;
    sound->pos_x = x; sound->pos_y = y; sound->pos_z = z;
}
void ca_voice_set_min_distance(ca_voice* sound, float d) { if (sound) sound->min_dist = d; }
void ca_voice_set_max_distance(ca_voice* sound, float d) { if (sound) sound->max_dist = d; }
void ca_voice_set_rolloff(ca_voice* sound, float r) { if (sound) sound->rolloff = r; }

ca_bool32 ca_voice_is_playing(const ca_voice* sound)
{
    return sound && sound->playing && !sound->at_end ? CA_TRUE : CA_FALSE;
}

#endif /* CAUDIO_IMPLEMENTATION */
#endif /* CAUDIO_H */
