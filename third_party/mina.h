/*
 * MINA - a dependency-free, single-header audio library for (almost) everything.
 * ============================================================================
 *
 *   Windows 10/7/XP/98/95, macOS, Linux, BSD, early Unix, and DOS.
 *
 * One file. No runtime dependencies, no dynamic linking of codec libraries,
 * nothing pre-installed. Drop mina.h into your project, define
 * MINA_IMPLEMENTATION in exactly one translation unit, and include it
 * everywhere else.
 *
 *   #define MINA_IMPLEMENTATION
 *   #include "mina.h"
 *
 * LANGUAGE
 *   Strict ISO C90 ("C89"/ANSI C). The whole library compiles clean under
 *   -std=c89 -pedantic-errors -Wall -Wextra with no extensions: no //
 *   comments, no mixed declarations, no long long, no variable-length arrays,
 *   no designated initializers, no compound literals. <stdint.h> is used when
 *   the compiler provides it and emulated from <limits.h> when it does not.
 *   A 64-bit integer type is used when one exists and is emulated with a
 *   portable 32/32 pair when it does not, so the library builds unmodified on
 *   16-bit DOS compilers.
 *
 * WHAT IS IMPLEMENTED
 *   WAV     decode + encode. PCM 8/16/24/32, float32/64, mu-law, A-law,
 *           WAVE_FORMAT_EXTENSIBLE, any channel count, any sample rate.
 *   FLAC    full decoder. CONSTANT/VERBATIM/FIXED(0-4)/LPC(1-32) subframes,
 *           partitioned-Rice residuals (4- and 5-bit parameters + escape),
 *           left/side, side/right and mid/side decorrelation, wasted bits,
 *           every block-size and sample-rate code, UTF-8 coded numbers,
 *           CRC-8 and CRC-16 verification, all metadata block types.
 *   MP3     full MPEG-1/2/2.5 Layer III decoder. Side info, scalefactors,
 *           Huffman (all tables + count1), requantisation, MS and intensity
 *           stereo, alias reduction, IMDCT 36/12, polyphase synthesis, bit
 *           reservoir, Xing/Info/VBRI parsing and LAME gapless trimming.
 *           Layer I/II headers are parsed for identification only.
 *   Vorbis  full Ogg Vorbis I decoder. Codebooks (ordered/sparse/unordered,
 *           VQ lookup 1 and 2), floor 1, residue 0/1/2, square-polar channel
 *           coupling, all four window transition cases, and an FFT-factored
 *           inverse MDCT. Floor 0 is implemented from the specification but
 *           is untested: no released encoder emits floor 0 streams.
 *   Ogg     page/packet layer, codec identification (Vorbis, Opus, FLAC,
 *           Speex, Theora, OggPCM).
 *   AAC     AAC LC decoding in ADTS and MP4/M4A containers.
 *   DSP     sample conversion, streaming resampler (linear + windowed sinc),
 *           mixer with soft clip, small synthesiser.
 *   Output  ALSA, OSS, PulseAudio, CoreAudio, WinMM, WAV file, null,
 *           DOS PC speaker / AdLib OPL2 / MPU-401 MIDI.
 *   Stream  pull-mode decode (mina_stream_*). WAV, MP3 and Ogg Vorbis are
 *           decoded on demand a frame or packet at a time, so a long track
 *           costs its encoded size instead of its decoded size; other
 *           codecs are decoded once at open and served from that buffer.
 *           Either way the samples match mina_decode exactly.
 *   Engine  named-voice playback (mina_engine_*). A fixed-capacity mixer
 *           with reference-counted clips, streaming voices, per-voice
 *           volume/pitch/loop, volume ramps, listener-relative distance
 *           attenuation and constant-power panning, master gain and soft
 *           clip. No threads and no per-block allocation: render from
 *           whatever pump loop or audio callback you already have, and
 *           install mina_engine_set_lock if another thread drives the
 *           control calls.
 *
 * BUILD-TIME OPTIONS (define before including the implementation)
 *   MINA_NO_STDIO       drop every <stdio.h> dependency (disables the wavfile
 *                       sink; everything else still works)
 *   MINA_NO_DEVICES     drop the output-device layer entirely
 *   MINA_NO_VORBIS      drop the Vorbis decoder
 *   MINA_NO_MP3         drop the MP3 decoder
 *   MINA_NO_FLAC        drop the FLAC decoder
 *   MINA_NO_AAC         drop the AAC decoder (identification is kept)
 *   MINA_FORCE_NO_64    ignore any native 64-bit type and use the portable
 *                       emulation (used by the test suite)
 *   MINA_MALLOC/MINA_REALLOC/MINA_FREE   override the allocator
 *
 * THREAD SAFETY
 *   Every decoder is re-entrant and allocates no static mutable state. The
 *   codec registry is process-global; register custom codecs before the first
 *   concurrent mina_decode(). Device handles are not shared between threads.
 *
 * LICENSE: MIT (see the end of this file).
 */

#ifndef MINA_H
#define MINA_H

#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MINA_VERSION_MAJOR 2
#define MINA_VERSION_MINOR 0
#define MINA_VERSION_PATCH 0
#define MINA_VERSION_STRING "2.0.0"

/* ------------------------------------------------------------------ */
/* Platform detection                                                  */
/*                                                                     */
/* Pre-define exactly one of MINA_WINDOWS / MINA_MACOS / MINA_LINUX /   */
/* MINA_BSD / MINA_UNIX / MINA_DOS before including this header to      */
/* override the auto-detection - useful for cross builds and for        */
/* compile-testing one platform's backend from another host.           */
/* ------------------------------------------------------------------ */
#if !defined(MINA_WINDOWS) && !defined(MINA_MACOS) && !defined(MINA_LINUX) && \
    !defined(MINA_BSD) && !defined(MINA_UNIX) && !defined(MINA_DOS)
#  if defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__)
#    define MINA_WINDOWS 1
#    if defined(_WIN64)
#      define MINA_WINDOWS_64 1
#    else
#      define MINA_WINDOWS_32 1
#    endif
#  elif defined(__APPLE__) && defined(__MACH__)
#    define MINA_MACOS 1
#  elif defined(__linux__)
#    define MINA_LINUX 1
#  elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || \
        defined(__DragonFly__)
#    define MINA_BSD 1
#  elif defined(__MSDOS__) || defined(__DOS__) || defined(MSDOS) || defined(_MSDOS)
#    define MINA_DOS 1
#  elif defined(__unix__) || defined(__unix) || defined(unix)
#    define MINA_UNIX 1
#  endif
#endif

/* Anything vaguely POSIX-ish that we did not recognise by name. */
#if !defined(MINA_WINDOWS) && !defined(MINA_MACOS) && !defined(MINA_LINUX) && \
    !defined(MINA_BSD) && !defined(MINA_UNIX) && !defined(MINA_DOS) && \
    !defined(MINA_POSIX_GENERIC)
#  define MINA_POSIX_GENERIC 1
#endif

/* ------------------------------------------------------------------ */
/* Fixed-width integer types                                           */
/*                                                                     */
/* <stdint.h> is C99. Under a C89 compiler we derive the same types    */
/* from <limits.h>, which every hosted implementation has.             */
/* ------------------------------------------------------------------ */
#if !defined(MINA_NO_STDINT)
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#    define MINA_USE_STDINT 1
#  elif defined(_MSC_VER) && _MSC_VER >= 1600
#    define MINA_USE_STDINT 1
#  elif defined(__GNUC__) && (__GNUC__ >= 3)
#    define MINA_USE_STDINT 1
#  endif
#endif

#ifdef MINA_USE_STDINT
#include <stdint.h>
typedef int8_t   mina_i8;
typedef uint8_t  mina_u8;
typedef int16_t  mina_i16;
typedef uint16_t mina_u16;
typedef int32_t  mina_i32;
typedef uint32_t mina_u32;
#else
typedef signed char    mina_i8;
typedef unsigned char  mina_u8;
#if USHRT_MAX == 0xFFFF
typedef short          mina_i16;
typedef unsigned short mina_u16;
#else
#error "mina: no 16-bit integer type available"
#endif
#if UINT_MAX == 0xFFFFFFFFUL
typedef int            mina_i32;
typedef unsigned int   mina_u32;
#elif ULONG_MAX == 0xFFFFFFFFUL
typedef long           mina_i32;
typedef unsigned long  mina_u32;
#else
#error "mina: no 32-bit integer type available"
#endif
#endif

/* ------------------------------------------------------------------ */
/* 64-bit support                                                      */
/*                                                                     */
/* mina_u64 is a real 64-bit unsigned type when the compiler has one.  */
/* Otherwise it is a { lo, hi } pair with just enough arithmetic for    */
/* sample counters. MINA_HAS_I64 says which you got.                   */
/* ------------------------------------------------------------------ */
#if !defined(MINA_FORCE_NO_64)
#  if defined(MINA_USE_STDINT)
#    define MINA_HAS_I64 1
typedef int64_t  mina_i64;
typedef uint64_t mina_u64;
#  elif ULONG_MAX > 0xFFFFFFFFUL
#    define MINA_HAS_I64 1
typedef long          mina_i64;
typedef unsigned long mina_u64;
#  elif defined(_MSC_VER) && _MSC_VER >= 1200
#    define MINA_HAS_I64 1
typedef __int64          mina_i64;
typedef unsigned __int64 mina_u64;
#  endif
#endif

#ifndef MINA_HAS_I64
/* Portable 64-bit unsigned counter: hi * 2^32 + lo. */
typedef struct { mina_u32 lo, hi; } mina_u64;
typedef struct { mina_u32 lo, hi; } mina_i64;
#endif

typedef int   mina_bool;
typedef float mina_f32;
typedef double mina_f64;

typedef enum {
    MINA_OK              =  0,
    MINA_ERR_GENERIC     = -1,
    MINA_ERR_NOTFOUND    = -2,  /* magic/signature not recognised          */
    MINA_ERR_UNSUPPORTED = -3,  /* recognised, but no decoder registered   */
    MINA_ERR_INVALID     = -4,  /* corrupt / malformed data                */
    MINA_ERR_NOMEM       = -5,
    MINA_ERR_IO          = -6,
    MINA_ERR_DEVICE      = -7,  /* output device open/write failure        */
    MINA_ERR_PARAM       = -8,
    MINA_ERR_TRUNCATED   = -9   /* decoded, but the stream ended early     */
} mina_result;

/* PCM sample formats understood by the conversion layer. */
typedef enum {
    MINA_FMT_U8     = 0,   /* unsigned 8-bit                    */
    MINA_FMT_S16    = 1,   /* signed 16-bit LE                  */
    MINA_FMT_S24    = 2,   /* signed 24-bit packed 3-byte LE    */
    MINA_FMT_S32    = 3,   /* signed 32-bit LE                  */
    MINA_FMT_F32    = 4,   /* IEEE-754 binary32 LE              */
    MINA_FMT_F64    = 5,   /* IEEE-754 binary64 LE              */
    MINA_FMT_MULAW  = 6,   /* 8-bit mu-law (ITU-T G.711)        */
    MINA_FMT_ALAW   = 7,   /* 8-bit A-law  (ITU-T G.711)        */
    MINA_FMT_S16BE  = 8,   /* signed 16-bit BE                  */
    MINA_FMT_S24BE  = 9,   /* signed 24-bit packed 3-byte BE    */
    MINA_FMT_S32BE  = 10,  /* signed 32-bit BE                  */
    MINA_FMT_F32BE  = 11,  /* IEEE-754 binary32 BE              */
    MINA_FMT_F64BE  = 12,  /* IEEE-754 binary64 BE              */
    MINA_FMT_COUNT  = 13
} mina_format;

/* Canonical decoded audio: interleaved float32 in [-1, 1]. */
typedef struct {
    float    *samples;     /* interleaved, length = frames * channels */
    mina_u64  frames;
    mina_u32  channels;
    mina_u32  sample_rate;
} mina_pcm;

/* Format identification, without decoding. */
typedef struct {
    char      codec[16];        /* "wav","flac","vorbis","opus","mp3",...  */
    mina_u32  sample_rate;
    mina_u32  channels;
    mina_u32  bits_per_sample;  /* 0 when the codec has no fixed depth     */
    mina_u64  total_frames;     /* 0 if unknown                            */
    double    duration_seconds; /* 0 if unknown                            */
    mina_u32  bitrate_kbps;     /* average, 0 if unknown                   */
} mina_fileinfo;

/* A codec: probe + decode + info. The registry lets you add decoders
 * without touching the core; see the built-in registrations. */
typedef struct mina_codec mina_codec;
struct mina_codec {
    const char *name;       /* short canonical name, e.g. "flac" */
    const char *long_name;  /* human-readable                    */
    mina_bool   (*probe)(const mina_u8 *data, size_t size);
    mina_result (*decode)(const mina_u8 *data, size_t size, mina_pcm *out);
    mina_result (*info)(const mina_u8 *data, size_t size, mina_fileinfo *out);
    mina_codec *next;
};

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

/* Decode any supported format from a memory buffer into float32 PCM.
 * On success out->samples is heap-allocated; free it with mina_pcm_free().
 * Returns MINA_ERR_TRUNCATED (not a hard failure) when the stream ended
 * early but usable audio was produced. */
mina_result mina_decode(const void *data, size_t size, mina_pcm *out);

/* Identify a buffer without decoding it. */
mina_result mina_info(const void *data, size_t size, mina_fileinfo *out);

void mina_pcm_free(mina_pcm *pcm);

/* Human-readable name for a result code. Never returns NULL. */
const char *mina_result_string(mina_result r);

/* mina_u64 accessors. On a compiler with a native 64-bit type these are
 * trivial; where the type is emulated they are the supported way to get a
 * frame count in and out. */
mina_u64 mina_u64_of(mina_u32 v);
double   mina_u64_to_double(mina_u64 v);
size_t   mina_u64_to_size(mina_u64 v);

/* ---- Sample conversion ---- */
/* count is the number of *samples* (frames * channels), not frames. */
void mina_convert_to_f32  (const void *src, mina_format fmt,
                           mina_u32 channels, size_t count, float *dst);
void mina_convert_from_f32(const float *src, mina_format fmt,
                           mina_u32 channels, size_t count, void *dst);
const char *mina_format_name(mina_format fmt);
/* Bytes occupied by one sample of fmt (0 for an unknown format). */
size_t mina_format_size(mina_format fmt);

/* ---- WAV writing ---- */
/* Serialise interleaved float32 into a RIFF/WAVE image. bits is 8, 16, 24,
 * 32 (integer) or 33 for float32. Returns the byte count and stores a
 * malloc'd buffer in *out_data (free() it), or 0 on failure. */
size_t mina_wav_write(const float *samples, mina_u64 frames, mina_u32 channels,
                      mina_u32 sample_rate, int bits, mina_u8 **out_data);
#define MINA_WAV_FLOAT32 33

/* ---- Resampling (streaming, interleaved, 1..N channels) ---- */
typedef struct mina_resampler mina_resampler;
#define MINA_QUALITY_LINEAR 0
#define MINA_QUALITY_SINC   1
mina_resampler *mina_resampler_create(mina_u32 in_rate, mina_u32 out_rate,
                                      mina_u32 channels, int quality);
/* Consumes up to in_frames and writes up to out_cap frames. Returns the
 * frames written and stores the frames consumed in *in_used (may be NULL). */
mina_u32 mina_resampler_process(mina_resampler *r, const float *in,
                                mina_u32 in_frames, float *out, mina_u32 out_cap,
                                mina_u32 *in_used);
/* Flush the internal delay line at end of stream. */
mina_u32 mina_resampler_flush(mina_resampler *r, float *out, mina_u32 out_cap);
void mina_resampler_reset(mina_resampler *r);
void mina_resampler_free(mina_resampler *r);
/* Convenience: resample a whole buffer in one call. Returns frames written. */
mina_u64 mina_resample_buffer(const float *in, mina_u64 in_frames,
                              mina_u32 in_rate, mina_u32 out_rate,
                              mina_u32 channels, int quality, float **out);

/* ---- Mixing ---- */
/* count is samples, not frames. mina_mix soft-clips; mina_mix_raw does not. */
void mina_mix(float *dst, const float *src, size_t count, float gain);
void mina_mix_raw(float *dst, const float *src, size_t count, float gain);

/* ---- Small synthesiser ---- */
typedef struct mina_synth mina_synth;
#define MINA_WAVE_SINE   0
#define MINA_WAVE_SQUARE 1
#define MINA_WAVE_SAW    2
#define MINA_WAVE_TRI    3
#define MINA_WAVE_NOISE  4
mina_synth *mina_synth_create(mina_u32 sample_rate);
void        mina_synth_free(mina_synth *s);
void        mina_synth_note_on (mina_synth *s, float freq, float velocity);
void        mina_synth_note_off(mina_synth *s);
void        mina_synth_set_wave(mina_synth *s, int wave);
/* Renders `frames` mono samples into out. Returns frames written. */
mina_u32    mina_synth_render(mina_synth *s, float *out, mina_u32 frames);

/* ---- Output devices ---- */
typedef struct mina_device mina_device;
/* Backend names: "alsa" "oss" "pulse" "coreaudio" "winmm" "pcspeaker"
 * "adlib" "midi" "wavfile" "null". NULL or "" selects the platform default,
 * falling back through the available backends to "null". */
mina_device *mina_device_open(const char *backend, mina_u32 sample_rate,
                              mina_u32 channels);
mina_result  mina_device_write(mina_device *d, const float *interleaved,
                               mina_u32 frames);
void         mina_device_close(mina_device *d);
/* Which backend actually opened, and whether it is a real audio sink. */
const char  *mina_device_backend(const mina_device *d);
mina_bool    mina_device_is_real(const mina_device *d);
/* Destination path for the "wavfile" backend; defaults to "mina_out.wav". */
void         mina_device_set_path(mina_device *d, const char *path);

/* ---- Codec registry ---- */
void        mina_codec_register(mina_codec *c);
mina_codec *mina_codec_first(void);

/* ---- Streaming decode ---- */
/* A pull-mode decoder over an encoded image held in memory. Where the codec
 * allows it (WAV, MP3, Ogg Vorbis) frames are produced on demand, so a long
 * track costs its encoded size plus one decoder's working set instead of the
 * whole decoded stream. Other codecs decode once on open and are served from
 * that buffer; mina_stream_incremental() says which happened. Every stream
 * reads back identical samples to mina_decode() on the same image. */
typedef struct mina_stream mina_stream;
/* `own` != 0 hands `data` to the stream, which frees it with MINA_FREE on
 * close; otherwise the caller must keep `data` alive until then. */
mina_stream *mina_stream_open(const void *data, size_t size, int own);
void         mina_stream_close(mina_stream *s);
mina_u32     mina_stream_channels(const mina_stream *s);
mina_u32     mina_stream_rate(const mina_stream *s);
/* Total frames, or 0 when the container does not say. */
mina_u64     mina_stream_frames(const mina_stream *s);
mina_bool    mina_stream_incremental(const mina_stream *s);
/* Fills up to `frames` interleaved frames; returns frames written. A short
 * or zero return means end of stream. */
mina_u32     mina_stream_read(mina_stream *s, float *out, mina_u32 frames);
/* Rewind to the first sample. Returns 0 if the stream could not be reset. */
int          mina_stream_rewind(mina_stream *s);
const char  *mina_stream_codec(const mina_stream *s);

/* ---- Playback engine: named voices, mixing, panning, distance ---- */
/* A fixed-capacity mixer. Nothing here allocates per block and nothing here
 * starts a thread: call mina_engine_render() from whatever audio callback or
 * pump loop you already have. Install mina_engine_set_lock() if a second
 * thread drives the control calls. */
typedef struct mina_engine mina_engine;
typedef struct mina_clip   mina_clip;

typedef struct {
    int   loop;          /* restart at the end instead of stopping         */
    int   spatial;       /* pan and attenuate by listener position         */
    int   stream;        /* decode on demand rather than all at once       */
    float x, y, z;       /* world position, used when spatial              */
    float min_distance;  /* full volume at or inside this radius           */
    float max_distance;  /* silent at or beyond this radius                */
    float rolloff;       /* attenuation exponent; 1 = linear               */
} mina_source_params;
/* loop=0 spatial=0 pos=0 min=1 max=16 rolloff=1 */
void mina_source_params_init(mina_source_params *p);

/* Clips hold PCM already converted to the engine's rate and channel count.
 * They are reference counted, so one decode can back many voices. */
mina_clip *mina_clip_decode(mina_engine *e, const void *data, size_t size);
mina_clip *mina_clip_from_pcm(mina_engine *e, const float *interleaved,
                              mina_u64 frames, mina_u32 channels, mina_u32 rate);
mina_clip *mina_clip_retain(mina_clip *c);
void       mina_clip_release(mina_clip *c);
mina_u64   mina_clip_frames(const mina_clip *c);

/* backend: as mina_device_open, or "" to run without a device (render only).
 * block/max_voices of 0 select the defaults (1024 frames, 64 voices). */
mina_engine *mina_engine_create(const char *backend, mina_u32 rate,
                                mina_u32 channels, mina_u32 block,
                                mina_u32 max_voices);
void         mina_engine_destroy(mina_engine *e);
mina_bool    mina_engine_is_real(const mina_engine *e);
const char  *mina_engine_backend(const mina_engine *e);
mina_u32     mina_engine_rate(const mina_engine *e);
mina_u32     mina_engine_channels(const mina_engine *e);
mina_u32     mina_engine_block(const mina_engine *e);
/* Both hooks may be NULL (the default: no locking, single-threaded use). */
void mina_engine_set_lock(mina_engine *e, void (*lock)(void *),
                          void (*unlock)(void *), void *user);
void mina_engine_set_master(mina_engine *e, float gain);
void mina_engine_listener(mina_engine *e, float px, float py, float pz,
                          float fx, float fy, float fz,
                          float ux, float uy, float uz);

/* Voices are addressed by a caller-chosen name of up to 47 bytes; starting a
 * voice under a name that is already playing replaces it. Return 1 on
 * success, 0 when the name is bad or every voice slot is busy. */
int  mina_engine_play_clip  (mina_engine *e, const char *name, mina_clip *c,
                             const mina_source_params *p, float volume, float pitch);
/* Decodes now, then plays; convenience over mina_clip_decode. */
int  mina_engine_play_memory(mina_engine *e, const char *name,
                             const void *data, size_t size,
                             const mina_source_params *p, float volume, float pitch);
/* The engine takes ownership of `s` and closes it when the voice ends. */
int  mina_engine_play_stream(mina_engine *e, const char *name, mina_stream *s,
                             const mina_source_params *p, float volume, float pitch);
void mina_engine_stop(mina_engine *e, const char *name);
/* Ramp to silence over `seconds`, then stop. 0 stops immediately. */
void mina_engine_stop_fade(mina_engine *e, const char *name, float seconds);
void mina_engine_stop_all(mina_engine *e);
mina_bool mina_engine_playing(const mina_engine *e, const char *name);
mina_u32  mina_engine_voices(const mina_engine *e);
int  mina_engine_set_volume  (mina_engine *e, const char *name, float volume);
int  mina_engine_set_pitch   (mina_engine *e, const char *name, float pitch);
int  mina_engine_set_position(mina_engine *e, const char *name,
                              float x, float y, float z);
int  mina_engine_set_loop    (mina_engine *e, const char *name, int loop);
/* Ramp the voice's volume to `volume` over `seconds`. */
int  mina_engine_fade(mina_engine *e, const char *name, float volume, float seconds);

/* Mix every live voice into `out` (block frames * channels floats),
 * overwriting it. Returns the frames written. */
mina_u32    mina_engine_render(mina_engine *e, float *out, mina_u32 frames);
/* Render one block and hand it to the engine's device. MINA_ERR_UNSUPPORTED
 * when the engine was created without one. */
mina_result mina_engine_pump(mina_engine *e);

/* ---- Threads ---- */
/* 1 when this build has a threading layer (Win32 from Windows 95 on, or
 * POSIX threads); 0 on DOS and under MINA_NO_THREADS, where the engine
 * stays entirely caller-driven. */
mina_bool mina_have_threads(void);

/* ---- Engine: file loading and background threads ---- */
/* Reads `path` and returns a MINA_MALLOC'd image, storing its size. Install
 * one to decrypt, unpack, or serve out of your own archive; the default
 * reads the file with <stdio.h>. Called on the loader thread, so it must be
 * safe to call from a thread other than the one driving the engine. */
typedef mina_u8 *(*mina_loader_fn)(const char *path, size_t *size, void *user);
void mina_engine_set_loader(mina_engine *e, mina_loader_fn fn, void *user);

/* Start the engine's own threads: one loader that reads and decodes, and -
 * when the engine has a device - one mixer that renders and writes at the
 * device's pace. Also installs the engine's own lock, so every other call
 * here becomes safe from any thread. Returns 0 where threads are
 * unavailable; then keep calling mina_engine_pump yourself. */
int  mina_engine_start(mina_engine *e);
void mina_engine_stop_threads(mina_engine *e);
mina_bool mina_engine_running(const mina_engine *e);
/* 1 once the device has refused a write; the mixer thread stops there. */
mina_bool mina_engine_device_failed(const mina_engine *e);

/* Play a file by path. Reserves the voice and returns at once; the load and
 * decode happen off the caller's thread when mina_engine_start has run, and
 * inline otherwise. Playing or stopping the same name again cancels a load
 * still in flight. mina_engine_playing reports 1 while one is pending. */
int  mina_engine_play_file(mina_engine *e, const char *name, const char *path,
                           const mina_source_params *p, float volume, float pitch);
/* Run queued loads on the calling thread. Only needed when no loader thread
 * is running and you want the work done at a moment of your choosing. */
void mina_engine_service_pending(mina_engine *e);

/* Decoded clips are cached by path and shared between voices. Default 64 MB;
 * 0 disables the cache. Only clips no voice is using are ever evicted. */
void   mina_engine_set_cache_limit(mina_engine *e, size_t bytes);
void   mina_engine_clear_cache(mina_engine *e);
size_t mina_engine_cache_bytes(const mina_engine *e);

/* Resampling quality for clips and streaming voices, MINA_QUALITY_SINC (the
 * default) or MINA_QUALITY_LINEAR, which costs about an eighth as much and
 * is the right choice on hardware that cannot spare the multiply-adds. */
void mina_engine_set_quality(mina_engine *e, int quality);
/* Drop a queued load that has waited longer than this, so a burst of short
 * effects never plays late as an echo. Looping and streaming requests are
 * always kept. 0 keeps everything. Default 250 ms. */
void mina_engine_set_load_deadline(mina_engine *e, mina_u32 milliseconds);

/* ================================================================== */
/* IMPLEMENTATION                                                      */
/* ================================================================== */
#ifdef MINA_IMPLEMENTATION

#if !defined(MINA_NO_STDIO)
#include <stdio.h>
#endif

#ifndef MINA_MALLOC
#define MINA_MALLOC(n)     malloc(n)
#endif
#ifndef MINA_REALLOC
#define MINA_REALLOC(p, n) realloc((p), (n))
#endif
#ifndef MINA_FREE
#define MINA_FREE(p)       free(p)
#endif

#ifndef MINA_PI
#define MINA_PI 3.14159265358979323846
#endif

/* Largest decoded stream we will allocate, in samples. Keeps a corrupt
 * header from asking for terabytes; tunable by the host. */
#ifndef MINA_MAX_SAMPLES
#define MINA_MAX_SAMPLES ((size_t)1 << 28)
#endif

/* ------------------------------------------------------------------ */
/* 64-bit helpers                                                      */
/*                                                                     */
/* Everything below is expressed through these so the emulated path     */
/* (MINA_FORCE_NO_64, or a compiler with no 64-bit type) behaves        */
/* identically to the native one.                                      */
/* ------------------------------------------------------------------ */
#ifdef MINA_HAS_I64

#define mina_u64_from(x)     ((mina_u64)(x))
#define mina_u64_lo(x)       ((mina_u32)((x) & 0xFFFFFFFFUL))
#define mina_u64_add(a, b)   ((mina_u64)((a) + (b)))
#define mina_u64_sub(a, b)   ((mina_u64)((a) - (b)))
#define mina_u64_lt(a, b)    ((a) < (b))
#define mina_u64_gt(a, b)    ((a) > (b))
#define mina_u64_eq(a, b)    ((a) == (b))
#define mina_u64_zero(a)     ((a) == 0)
#define mina_u64_dbl(a)      ((double)(a))
#define mina_u64_shl(a, n)   ((mina_u64)((a) << (n)))
#define mina_u64_or(a, b)    ((mina_u64)((a) | (b)))

static mina_u64 mina_u64_mul32(mina_u32 a, mina_u32 b) {
    return (mina_u64)a * (mina_u64)b;
}
/* Clamp to size_t; sets *ovf when the value did not fit. */
static size_t mina_u64_size(mina_u64 a, int *ovf) {
    size_t s = (size_t)a;
    if (ovf) *ovf = ((mina_u64)s != a);
    return s;
}

#else /* ---- portable 32/32 emulation ---- */

static mina_u64 mina_u64_from(mina_u32 x) {
    mina_u64 r; r.lo = x; r.hi = 0; return r;
}
#define mina_u64_lo(x)  ((x).lo)
static mina_u64 mina_u64_add(mina_u64 a, mina_u64 b) {
    mina_u64 r;
    r.lo = a.lo + b.lo;
    r.hi = a.hi + b.hi + (r.lo < a.lo ? 1u : 0u);
    return r;
}
static mina_u64 mina_u64_sub(mina_u64 a, mina_u64 b) {
    mina_u64 r;
    r.lo = a.lo - b.lo;
    r.hi = a.hi - b.hi - (a.lo < b.lo ? 1u : 0u);
    return r;
}
static int mina_u64_lt(mina_u64 a, mina_u64 b) {
    return a.hi != b.hi ? (a.hi < b.hi) : (a.lo < b.lo);
}
static int mina_u64_gt(mina_u64 a, mina_u64 b) { return mina_u64_lt(b, a); }
static int mina_u64_eq(mina_u64 a, mina_u64 b) { return a.hi == b.hi && a.lo == b.lo; }
static int mina_u64_zero(mina_u64 a) { return a.hi == 0 && a.lo == 0; }
static double mina_u64_dbl(mina_u64 a) {
    return (double)a.hi * 4294967296.0 + (double)a.lo;
}
static mina_u64 mina_u64_shl(mina_u64 a, unsigned n) {
    mina_u64 r;
    if (n == 0) return a;
    if (n >= 32) { r.hi = a.lo << (n - 32); r.lo = 0; }
    else { r.hi = (a.hi << n) | (a.lo >> (32 - n)); r.lo = a.lo << n; }
    return r;
}
static mina_u64 mina_u64_or(mina_u64 a, mina_u64 b) {
    mina_u64 r; r.lo = a.lo | b.lo; r.hi = a.hi | b.hi; return r;
}
static mina_u64 mina_u64_mul32(mina_u32 a, mina_u32 b) {
    mina_u32 al = a & 0xFFFFu, ah = a >> 16;
    mina_u32 bl = b & 0xFFFFu, bh = b >> 16;
    mina_u32 ll = al * bl, lh = al * bh, hl = ah * bl, hh = ah * bh;
    mina_u32 mid = lh + hl;
    mina_u64 r;
    r.hi = hh + (mid >> 16) + ((mid < lh) ? 0x10000u : 0u);
    r.lo = ll + (mid << 16);
    if (r.lo < ll) r.hi++;
    return r;
}
static size_t mina_u64_size(mina_u64 a, int *ovf) {
    if (sizeof(size_t) >= 8) {
        if (ovf) *ovf = 0;
        return (size_t)a.lo + (size_t)((size_t)a.hi << 16 << 16);
    }
    if (ovf) *ovf = (a.hi != 0);
    return (size_t)a.lo;
}
#endif /* MINA_HAS_I64 */

/* ------------------------------------------------------------------ */
/* Signed 64-bit accumulator (FLAC LPC).                               */
/* Native where possible; a 32/32 two's-complement pair otherwise.      */
/* ------------------------------------------------------------------ */
#ifndef MINA_NO_FLAC
#ifdef MINA_HAS_I64
typedef mina_i64 mina_acc;
#define mina_acc_zero()         ((mina_acc)0)
#define mina_acc_from(x)        ((mina_acc)(x))
#define mina_acc_addmul(a,x,y)  ((a) + (mina_acc)(x) * (mina_acc)(y))
static mina_i32 mina_acc_shr32(mina_acc a, unsigned n) {
    /* portable arithmetic right shift, then truncate to 32 bits */
    mina_u64 u;
    if (a >= 0) u = (mina_u64)a >> n;
    else u = ~((mina_u64)(~a) >> n);
    return (mina_i32)(mina_u32)(u & 0xFFFFFFFFUL);
}
#else
typedef struct { mina_u32 lo; mina_u32 hi; } mina_acc; /* two's complement */
static mina_acc mina_acc_zero(void) { mina_acc a; a.lo = 0; a.hi = 0; return a; }
static mina_acc mina_acc_from(mina_i32 x) {
    mina_acc a; a.lo = (mina_u32)x; a.hi = (x < 0) ? 0xFFFFFFFFUL : 0UL; return a;
}
static mina_acc mina_acc_add(mina_acc a, mina_acc b) {
    mina_acc r;
    r.lo = a.lo + b.lo;
    r.hi = a.hi + b.hi + (r.lo < a.lo ? 1u : 0u);
    return r;
}
/* signed 32x32 -> 64 */
static mina_acc mina_acc_mul(mina_i32 x, mina_i32 y) {
    mina_u32 ax = (mina_u32)(x < 0 ? -(mina_i32)x : x);
    mina_u32 ay = (mina_u32)(y < 0 ? -(mina_i32)y : y);
    mina_u32 al = ax & 0xFFFFu, ah = ax >> 16;
    mina_u32 bl = ay & 0xFFFFu, bh = ay >> 16;
    mina_u32 ll = al * bl, lh = al * bh, hl = ah * bl, hh = ah * bh;
    mina_u32 mid = lh + hl;
    mina_acc r;
    int neg = ((x < 0) != (y < 0));
    r.hi = hh + (mid >> 16) + ((mid < lh) ? 0x10000u : 0u);
    r.lo = ll + (mid << 16);
    if (r.lo < ll) r.hi++;
    if (neg) { r.lo = ~r.lo; r.hi = ~r.hi; r.lo++; if (!r.lo) r.hi++; }
    return r;
}
#define mina_acc_addmul(a,x,y) mina_acc_add((a), mina_acc_mul((x),(y)))
static mina_i32 mina_acc_shr32(mina_acc a, unsigned n) {
    mina_u32 lo = a.lo, hi = a.hi;
    while (n >= 32) { lo = hi; hi = (hi & 0x80000000UL) ? 0xFFFFFFFFUL : 0UL; n -= 32; }
    if (n) {
        lo = (lo >> n) | (hi << (32 - n));
        hi = (mina_u32)((mina_i32)hi >> n); /* sign-propagating on any sane impl */
        if (a.hi & 0x80000000UL) hi |= ~(0xFFFFFFFFUL >> n);
    }
    (void)hi;
    return (mina_i32)lo;
}
#endif
#endif /* MINA_NO_FLAC */

/* ------------------------------------------------------------------ */
/* Small helpers                                                       */
/* ------------------------------------------------------------------ */
/* Reinterpret a 32-bit pattern as two's-complement signed. Casting an
 * out-of-range unsigned to signed is only implementation-defined in C90,
 * so spell the conversion out; every compiler folds this away. */
static mina_i32 mina_u32_to_i32(mina_u32 u) {
    if (u <= 0x7FFFFFFFUL) return (mina_i32)u;
    return (mina_i32)(u - 0x80000000UL) - (mina_i32)0x7FFFFFFFL - 1;
}

/* Logical shift-based helpers that avoid signed-shift UB. */
static mina_i32 mina_shl_i32(mina_i32 v, unsigned n) {
    return mina_u32_to_i32((mina_u32)v << n);
}
static mina_i32 mina_shr_i32(mina_i32 v, unsigned n) {
    if (v >= 0) return (mina_i32)((mina_u32)v >> n);
    return (mina_i32)(~((mina_u32)(~v) >> n));
}

static void mina_strcpy_n(char *dst, size_t cap, const char *src) {
    size_t i = 0;
    if (!cap) return;
    while (src[i] && i + 1 < cap) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

const char *mina_result_string(mina_result r) {
    switch (r) {
    case MINA_OK:              return "ok";
    case MINA_ERR_GENERIC:     return "generic error";
    case MINA_ERR_NOTFOUND:    return "format not recognised";
    case MINA_ERR_UNSUPPORTED: return "format recognised but not decodable";
    case MINA_ERR_INVALID:     return "corrupt or malformed data";
    case MINA_ERR_NOMEM:       return "out of memory";
    case MINA_ERR_IO:          return "I/O error";
    case MINA_ERR_DEVICE:      return "audio device error";
    case MINA_ERR_PARAM:       return "invalid parameter";
    case MINA_ERR_TRUNCATED:   return "stream ended early";
    }
    return "unknown error";
}

/* ------------------------------------------------------------------ */
/* Byte readers. Everything goes through these, so the library is       */
/* endian-independent and never does an unaligned load.                */
/* ------------------------------------------------------------------ */
static mina_u16 mina_be16(const mina_u8 *p) {
    return (mina_u16)(((mina_u32)p[0] << 8) | (mina_u32)p[1]);
}
static mina_u32 mina_be24(const mina_u8 *p) {
    return ((mina_u32)p[0] << 16) | ((mina_u32)p[1] << 8) | (mina_u32)p[2];
}
static mina_u32 mina_be32(const mina_u8 *p) {
    return ((mina_u32)p[0] << 24) | ((mina_u32)p[1] << 16) |
           ((mina_u32)p[2] << 8)  | (mina_u32)p[3];
}
static mina_u16 mina_le16(const mina_u8 *p) {
    return (mina_u16)(((mina_u32)p[1] << 8) | (mina_u32)p[0]);
}
static mina_u32 mina_le32(const mina_u8 *p) {
    return ((mina_u32)p[3] << 24) | ((mina_u32)p[2] << 16) |
           ((mina_u32)p[1] << 8)  | (mina_u32)p[0];
}
static mina_u64 mina_le64(const mina_u8 *p) {
    return mina_u64_or(mina_u64_shl(mina_u64_from(mina_le32(p + 4)), 32),
                       mina_u64_from(mina_le32(p)));
}
static mina_u64 mina_be64(const mina_u8 *p) {
    return mina_u64_or(mina_u64_shl(mina_u64_from(mina_be32(p)), 32),
                       mina_u64_from(mina_be32(p + 4)));
}

/* IEEE-754 decoding from bytes: correct even where float is not IEEE and
 * regardless of host endianness. */
static float mina_f32_from_bits(mina_u32 b) {
    mina_u32 m = b & 0x7FFFFFUL;
    int e = (int)((b >> 23) & 0xFFu);
    double v;
    if (e == 0xFF) {
        /* Inf/NaN: clamp to a finite value rather than emit a trap value. */
        v = m ? 0.0 : 3.4028234663852886e38;
    } else if (e == 0) {
        v = ldexp((double)m, -149);
    } else {
        v = ldexp((double)(m | 0x800000UL), e - 150);
    }
    return (b & 0x80000000UL) ? (float)-v : (float)v;
}
static float mina_f64_from_bits(mina_u32 hi, mina_u32 lo) {
    mina_u32 mh = hi & 0xFFFFFUL;
    int e = (int)((hi >> 20) & 0x7FFu);
    double v;
    double frac = (double)mh * 4294967296.0 + (double)lo;  /* 52-bit mantissa */
    if (e == 0x7FF) {
        v = (mh || lo) ? 0.0 : 1.7976931348623157e308;
    } else if (e == 0) {
        v = ldexp(frac, -1074);
    } else {
        v = ldexp(frac + 4503599627370496.0, e - 1075);
    }
    return (float)((hi & 0x80000000UL) ? -v : v);
}
static mina_u32 mina_f32_to_bits(float f) {
    double d = (double)f;
    mina_u32 sign = 0, mant;
    int e;
    if (d < 0.0) { sign = 0x80000000UL; d = -d; }
    if (d == 0.0) return sign;
    if (d >= 3.402823669209385e38) return sign | 0x7F800000UL;
    {
        double frac = frexp(d, &e);           /* 0.5 <= frac < 1 */
        int be = e - 1 + 127;
        if (be <= 0) {                        /* subnormal */
            mant = (mina_u32)(ldexp(d, 149) + 0.5);
            return sign | (mant & 0x7FFFFFUL);
        }
        mant = (mina_u32)(ldexp(frac, 24) + 0.5) & 0x7FFFFFUL;
        if (ldexp(frac, 24) + 0.5 >= 16777216.0) { mant = 0; be++; }
        if (be >= 255) return sign | 0x7F800000UL;
        return sign | ((mina_u32)be << 23) | mant;
    }
}

/* ------------------------------------------------------------------ */
/* MSB-first bit reader (MPEG, FLAC)                                   */
/* ------------------------------------------------------------------ */
#if !defined(MINA_NO_FLAC) || !defined(MINA_NO_MP3)
typedef struct {
    const mina_u8 *p;
    size_t size;    /* bytes available */
    size_t pos;     /* bit position */
    int    err;
} mina_br;

static void mina_br_init(mina_br *b, const mina_u8 *p, size_t size) {
    b->p = p; b->size = size; b->pos = 0; b->err = 0;
}
static size_t mina_br_left(const mina_br *b) {
    size_t total = b->size * 8;
    return b->pos >= total ? 0 : total - b->pos;
}

/* Read n (0..32) bits MSB-first. Sets err and returns 0 past the end. */
static mina_u32 mina_br_u(mina_br *b, unsigned n) {
    mina_u32 v = 0;
    size_t byte;
    unsigned bit;
    if (n == 0) return 0;
    if (n > 32 || mina_br_left(b) < n) { b->err = 1; b->pos += n; return 0; }
    byte = b->pos >> 3;
    bit  = (unsigned)(b->pos & 7u);
    /* fast path: the field plus its bit offset fits one 32-bit load */
    if (bit + n <= 32 && byte + 4 <= b->size) {
        v = ((mina_u32)b->p[byte] << 24) | ((mina_u32)b->p[byte + 1] << 16) |
            ((mina_u32)b->p[byte + 2] << 8) | (mina_u32)b->p[byte + 3];
        v <<= bit;
        v >>= (32u - n);
        b->pos += n;
        return v;
    }
    while (n) {
        unsigned take, mask;
        byte = b->pos >> 3;
        bit  = (unsigned)(b->pos & 7u);
        take = 8u - bit;
        if (take > n) take = n;
        mask = (unsigned)((1UL << take) - 1UL);
        v = (v << take) | (((mina_u32)b->p[byte] >> (8u - bit - take)) & mask);
        b->pos += take;
        n -= take;
    }
    return v;
}

/* Sign-extended read. */
static mina_i32 mina_br_s(mina_br *b, unsigned n) {
    mina_u32 v;
    if (n == 0) return 0;
    v = mina_br_u(b, n);
    if (n < 32 && (v & ((mina_u32)1 << (n - 1))))
        v |= ~(((mina_u32)1 << n) - 1u);
    return mina_u32_to_i32(v);
}

/* Peek without consuming; zero-pads past the end and never sets err.
 * Used by the MP3 Huffman decoder, which relies on speculative reads. */
static mina_u32 mina_br_peek(const mina_br *b, unsigned n) {
    mina_u32 v = 0;
    size_t pos = b->pos;
    unsigned left = n;
    if (n == 0 || n > 32) return 0;
    if ((pos & 7u) + n <= 32 && (pos >> 3) + 4 <= b->size) {
        v = mina_be32(b->p + (pos >> 3));
        v <<= (unsigned)(pos & 7u);
        return v >> (32u - n);
    }
    while (left) {
        size_t byte = pos >> 3;
        unsigned bit  = (unsigned)(pos & 7u);
        unsigned take = 8u - bit;
        mina_u32 got;
        if (take > left) take = left;
        if (byte >= b->size) got = 0;
        else got = ((mina_u32)b->p[byte] >> (8u - bit - take)) &
                   (mina_u32)((1UL << take) - 1UL);
        v = (v << take) | got;
        pos  += take;
        left -= take;
    }
    return v;
}
static void mina_br_skip(mina_br *b, unsigned n) { b->pos += n; }

/* Number of leading zero bits in a non-zero 32-bit word. */
static unsigned mina_clz32(mina_u32 v) {
    unsigned n = 0;
    if (!(v & 0xFFFF0000UL)) { n += 16; v <<= 16; }
    if (!(v & 0xFF000000UL)) { n += 8;  v <<= 8;  }
    if (!(v & 0xF0000000UL)) { n += 4;  v <<= 4;  }
    if (!(v & 0xC0000000UL)) { n += 2;  v <<= 2;  }
    if (!(v & 0x80000000UL)) { n += 1; }
    return n;
}

/* Count of leading zero bits, consuming the terminating 1. */
static mina_u32 mina_br_unary(mina_br *b) {
    mina_u32 u = 0;
    for (;;) {
        size_t byte = b->pos >> 3;
        unsigned bit;
        mina_u32 w;
        if (byte >= b->size) { b->err = 1; return u; }
        bit = (unsigned)(b->pos & 7u);
        if (byte + 4 <= b->size) {
            /* scan 32 bits at a time */
            w = ((mina_u32)b->p[byte] << 24) | ((mina_u32)b->p[byte + 1] << 16) |
                ((mina_u32)b->p[byte + 2] << 8) | (mina_u32)b->p[byte + 3];
            w <<= bit;
            if (w) {
                unsigned lead = mina_clz32(w);
                if (lead + bit < 32u) {
                    b->pos += lead + 1u;
                    return u + lead;
                }
            }
            u += 32u - bit;
            b->pos += 32u - bit;
        } else {
            unsigned cur = (unsigned)b->p[byte] & (0xFFu >> bit);
            if (cur) {
                unsigned lead = 0;
                while (!(cur & (0x80u >> (bit + lead)))) lead++;
                b->pos += lead + 1u;
                return u + lead;
            }
            u += 8u - bit;
            b->pos += 8u - bit;
        }
        if (u > 0x1000000UL) { b->err = 1; return u; }
    }
}

static size_t mina_br_align(mina_br *b) {
    b->pos = (b->pos + 7u) & ~(size_t)7u;
    return b->pos >> 3;
}
#endif /* MSB-first bit reader */

/* ------------------------------------------------------------------ */
/* LSB-first bit reader (Vorbis)                                       */
/* ------------------------------------------------------------------ */
#ifndef MINA_NO_VORBIS
typedef struct {
    const mina_u8 *p;
    size_t size;
    size_t pos;
    int    eof;
} mina_lbr;

static void mina_lbr_init(mina_lbr *b, const mina_u8 *p, size_t sz) {
    b->p = p; b->size = sz; b->pos = 0; b->eof = 0;
}
static mina_u32 mina_lbr_read(mina_lbr *b, unsigned n) {
    mina_u32 v = 0;
    unsigned got = 0;
    if (n == 0) return 0;
    if (n > 32) { b->eof = 1; return 0; }
    while (got < n) {
        size_t byte = (b->pos + got) >> 3;
        unsigned bit  = (unsigned)((b->pos + got) & 7u);
        unsigned take = 8u - bit;
        if (take > n - got) take = n - got;
        if (byte >= b->size) { b->eof = 1; }
        else {
            mina_u32 chunk = ((mina_u32)b->p[byte] >> bit) &
                             (mina_u32)((1UL << take) - 1UL);
            v |= chunk << got;
        }
        got += take;
    }
    b->pos += n;
    return v;
}
/* Peek n bits without consuming; zero-pads past the end. */
static mina_u32 mina_lbr_peek(const mina_lbr *b, unsigned n) {
    mina_u32 v = 0;
    unsigned got = 0;
    if (n == 0 || n > 32) return 0;
    while (got < n) {
        size_t byte = (b->pos + got) >> 3;
        unsigned bit  = (unsigned)((b->pos + got) & 7u);
        unsigned take = 8u - bit;
        if (take > n - got) take = n - got;
        if (byte < b->size) {
            mina_u32 chunk = ((mina_u32)b->p[byte] >> bit) &
                             (mina_u32)((1UL << take) - 1UL);
            v |= chunk << got;
        }
        got += take;
    }
    return v;
}
static size_t mina_lbr_left(const mina_lbr *b) {
    size_t total = b->size * 8;
    return b->pos >= total ? 0 : total - b->pos;
}
static int mina_lbr_bit(mina_lbr *b) {
    size_t byte = b->pos >> 3;
    int v;
    if (byte >= b->size) { b->eof = 1; b->pos++; return 0; }
    v = (int)((b->p[byte] >> (b->pos & 7u)) & 1u);
    b->pos++;
    return v;
}
#endif /* MINA_NO_VORBIS */

/* ------------------------------------------------------------------ */
/* CRCs and UTF-8 coded numbers (FLAC frames)                          */
/* ------------------------------------------------------------------ */
#ifndef MINA_NO_FLAC
static mina_u8 mina_crc8(const mina_u8 *d, size_t n) {
    mina_u8 c = 0;
    size_t i; int j;
    for (i = 0; i < n; i++) {
        c = (mina_u8)(c ^ d[i]);
        for (j = 0; j < 8; j++)
            c = (mina_u8)((c & 0x80u) ? (((mina_u32)c << 1) ^ 0x07u)
                                      : ((mina_u32)c << 1));
    }
    return c;
}
static mina_u16 mina_crc16(const mina_u8 *d, size_t n) {
    mina_u16 c = 0;
    size_t i; int j;
    for (i = 0; i < n; i++) {
        c = (mina_u16)(c ^ ((mina_u32)d[i] << 8));
        for (j = 0; j < 8; j++)
            c = (mina_u16)((c & 0x8000u) ? (((mina_u32)c << 1) ^ 0x8005u)
                                         : ((mina_u32)c << 1));
    }
    return c;
}

static int mina_utf8_len(mina_u8 b0) {
    if (!(b0 & 0x80u)) return 1;
    if ((b0 & 0xE0u) == 0xC0u) return 2;
    if ((b0 & 0xF0u) == 0xE0u) return 3;
    if ((b0 & 0xF8u) == 0xF0u) return 4;
    if ((b0 & 0xFCu) == 0xF8u) return 5;
    if ((b0 & 0xFEu) == 0xFCu) return 6;
    if (b0 == 0xFEu) return 7;
    return -1;  /* 0x80..0xBF is a continuation byte, never a lead */
}
#endif /* MINA_NO_FLAC */

/* ------------------------------------------------------------------ */
/* Growable interleaved float buffer                                   */
/*                                                                     */
/* Used by every decoder that streams frames; compiled out when all of  */
/* them are disabled.                                                  */
/* ------------------------------------------------------------------ */
#if !defined(MINA_NO_FLAC) || !defined(MINA_NO_MP3) || !defined(MINA_NO_VORBIS) || !defined(MINA_NO_AAC)
#define MINA_USE_FBUF 1

typedef struct {
    float *data;
    size_t len;   /* samples used     */
    size_t cap;   /* samples allocated */
    int    oom;
} mina_fbuf;

static void mina_fbuf_init(mina_fbuf *b) {
    b->data = NULL; b->len = 0; b->cap = 0; b->oom = 0;
}
static int mina_fbuf_reserve(mina_fbuf *b, size_t need) {
    if (b->oom) return 0;
    if (need > MINA_MAX_SAMPLES) { b->oom = 1; return 0; }
    if (need > b->cap) {
        size_t ncap = b->cap ? b->cap : 4096;
        float *nd;
        while (ncap < need) {
            if (ncap > MINA_MAX_SAMPLES / 2) { ncap = need; break; }
            ncap *= 2;
        }
        nd = (float *)MINA_REALLOC(b->data, ncap * sizeof(float));
        if (!nd) { b->oom = 1; return 0; }
        b->data = nd; b->cap = ncap;
    }
    return 1;
}
/* Reserve n more samples and return a zeroed write pointer, or NULL. */
static float *mina_fbuf_extend(mina_fbuf *b, size_t n) {
    float *p;
    if (!mina_fbuf_reserve(b, b->len + n)) return NULL;
    p = b->data + b->len;
    memset(p, 0, n * sizeof(float));
    b->len += n;
    return p;
}
static void mina_fbuf_free(mina_fbuf *b) {
    MINA_FREE(b->data); b->data = NULL; b->len = b->cap = 0;
}

/* Hand a finished buffer to the caller as a mina_pcm. */
static mina_result mina_fbuf_finish(mina_fbuf *b, mina_pcm *out,
                                    mina_u32 channels, mina_u32 rate,
                                    mina_result status) {
    if (b->oom) { mina_fbuf_free(b); return MINA_ERR_NOMEM; }
    out->channels = channels ? channels : 1;
    out->sample_rate = rate;
    out->frames = mina_u64_from((mina_u32)(b->len / out->channels));
    if (!b->data) {
        b->data = (float *)MINA_MALLOC(sizeof(float));
        if (!b->data) return MINA_ERR_NOMEM;
        b->data[0] = 0.0f;
        out->frames = mina_u64_from(0);
    }
    out->samples = b->data;
    b->data = NULL;
    return status;
}

#endif /* MINA_USE_FBUF */

/* ------------------------------------------------------------------ */
/* Sample conversion                                                    */
/*                                                                      */
/* Every path is defined byte-wise, so decoding a little-endian WAV on a */
/* big-endian host produces the same samples as on a little-endian one.  */
/* Where the host is known to use IEEE-754 little-endian floats we take  */
/* a memcpy fast path; otherwise the bits are assembled by hand.         */
/* ------------------------------------------------------------------ */
#include <float.h>

#if (FLT_RADIX == 2) && (FLT_MANT_DIG == 24) && (DBL_MANT_DIG == 53)
#  if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || \
      defined(_M_X64) || defined(__aarch64__) || defined(__wasm__) || \
      (defined(_WIN32) && !defined(_M_PPC) && !defined(_M_MRX000))
#    define MINA_IEEE_LE 1
#  endif
#endif

static const char *g_mina_fmt_names[MINA_FMT_COUNT] = {
    "u8", "s16", "s24", "s32", "f32", "f64", "mulaw", "alaw",
    "s16be", "s24be", "s32be", "f32be", "f64be"
};

const char *mina_format_name(mina_format fmt) {
    if ((int)fmt < 0 || (int)fmt >= MINA_FMT_COUNT) return "?";
    return g_mina_fmt_names[(int)fmt];
}

size_t mina_format_size(mina_format fmt) {
    switch (fmt) {
    case MINA_FMT_U8: case MINA_FMT_MULAW: case MINA_FMT_ALAW: return 1;
    case MINA_FMT_S16: case MINA_FMT_S16BE:                    return 2;
    case MINA_FMT_S24: case MINA_FMT_S24BE:                    return 3;
    case MINA_FMT_S32: case MINA_FMT_S32BE:
    case MINA_FMT_F32: case MINA_FMT_F32BE:                    return 4;
    case MINA_FMT_F64: case MINA_FMT_F64BE:                    return 8;
    default: return 0;
    }
}

/* ---- G.711 companding (ITU-T G.711, the libsndfile/ffmpeg convention) ---- */
static mina_i32 mina_mulaw_decode(mina_u8 v) {
    mina_u32 u = (mina_u32)(mina_u8)~v;
    mina_i32 t = (mina_i32)(((u & 0x0Fu) << 3) + 0x84u);
    t = mina_shl_i32(t, (unsigned)((u & 0x70u) >> 4));
    return (u & 0x80u) ? (0x84 - t) : (t - 0x84);
}
static mina_i32 mina_alaw_decode(mina_u8 v) {
    mina_u32 u = (mina_u32)(v ^ 0x55u);
    mina_i32 t = (mina_i32)((u & 0x0Fu) << 4);
    mina_u32 seg = (u & 0x70u) >> 4;
    if (seg == 0) t += 8;
    else { t += 0x108; t = mina_shl_i32(t, (unsigned)(seg - 1)); }
    return (u & 0x80u) ? t : -t;
}
/* Encoders follow the Sun reference g711.c exactly (the encoding every
 * other implementation is checked against). */
static mina_u8 mina_mulaw_encode(mina_i32 pcm) {
    static const mina_i32 seg_end[8] = { 0x3F, 0x7F, 0xFF, 0x1FF,
                                         0x3FF, 0x7FF, 0xFFF, 0x1FFF };
    mina_i32 mask, seg;
    mina_u8 uval;
    pcm = mina_shr_i32(pcm, 2);
    if (pcm < 0) { pcm = -pcm; mask = 0x7F; } else { mask = 0xFF; }
    if (pcm > 8159) pcm = 8159;               /* CLIP */
    pcm += (0x84 >> 2);                       /* BIAS */
    for (seg = 0; seg < 8; seg++) if (pcm <= seg_end[seg]) break;
    if (seg >= 8) return (mina_u8)(0x7F ^ mask);
    uval = (mina_u8)((seg << 4) | ((pcm >> (seg + 1)) & 0x0F));
    return (mina_u8)(uval ^ mask);
}
static mina_u8 mina_alaw_encode(mina_i32 pcm) {
    static const mina_i32 seg_end[8] = { 0x1F, 0x3F, 0x7F, 0xFF,
                                         0x1FF, 0x3FF, 0x7FF, 0xFFF };
    mina_i32 mask, seg;
    mina_u8 aval;
    pcm = mina_shr_i32(pcm, 3);
    if (pcm >= 0) mask = 0xD5; else { mask = 0x55; pcm = -pcm - 1; }
    for (seg = 0; seg < 8; seg++) if (pcm <= seg_end[seg]) break;
    if (seg >= 8) return (mina_u8)(0x7F ^ mask);
    aval = (mina_u8)(seg << 4);
    if (seg < 2) aval = (mina_u8)(aval | ((pcm >> 1) & 0x0F));
    else         aval = (mina_u8)(aval | ((pcm >> seg) & 0x0F));
    return (mina_u8)(aval ^ mask);
}

/* ---- decode ---- */
void mina_convert_to_f32(const void *src, mina_format fmt,
                         mina_u32 channels, size_t count, float *dst) {
    const mina_u8 *s = (const mina_u8 *)src;
    size_t i;
    (void)channels;
    if (!src || !dst) return;

    switch (fmt) {
    case MINA_FMT_U8:
        for (i = 0; i < count; i++)
            dst[i] = ((float)(int)s[i] - 128.0f) * (1.0f / 128.0f);
        break;

    case MINA_FMT_S16:
    case MINA_FMT_S16BE: {
        int be = (fmt == MINA_FMT_S16BE);
        for (i = 0; i < count; i++) {
            mina_u32 u = be ? (mina_u32)mina_be16(s + i * 2)
                            : (mina_u32)mina_le16(s + i * 2);
            mina_i32 v = (mina_i32)u - ((u & 0x8000UL) ? 65536L : 0L);
            dst[i] = (float)v * (1.0f / 32768.0f);
        }
        break; }

    case MINA_FMT_S24:
    case MINA_FMT_S24BE: {
        int be = (fmt == MINA_FMT_S24BE);
        for (i = 0; i < count; i++) {
            const mina_u8 *q = s + i * 3;
            mina_u32 u = be ? mina_be24(q)
                            : ((mina_u32)q[0] | ((mina_u32)q[1] << 8) |
                               ((mina_u32)q[2] << 16));
            mina_i32 v = (mina_i32)u - ((u & 0x800000UL) ? 16777216L : 0L);
            dst[i] = (float)v * (1.0f / 8388608.0f);
        }
        break; }

    case MINA_FMT_S32:
    case MINA_FMT_S32BE: {
        int be = (fmt == MINA_FMT_S32BE);
        for (i = 0; i < count; i++) {
            mina_u32 u = be ? mina_be32(s + i * 4) : mina_le32(s + i * 4);
            /* two's-complement reinterpretation without signed overflow */
            double v = (u & 0x80000000UL)
                     ? -(double)((mina_u32)(~u) + 1u)
                     :  (double)u;
            dst[i] = (float)(v * (1.0 / 2147483648.0));
        }
        break; }

    case MINA_FMT_F32:
#ifdef MINA_IEEE_LE
        memcpy(dst, s, count * 4);
        break;
#else
        for (i = 0; i < count; i++) dst[i] = mina_f32_from_bits(mina_le32(s + i * 4));
        break;
#endif
    case MINA_FMT_F32BE:
        for (i = 0; i < count; i++) dst[i] = mina_f32_from_bits(mina_be32(s + i * 4));
        break;

    case MINA_FMT_F64:
#ifdef MINA_IEEE_LE
        {
            const double *p = (const double *)src;
            double tmp;
            for (i = 0; i < count; i++) {
                memcpy(&tmp, (const mina_u8 *)p + i * 8, sizeof(double));
                dst[i] = (float)tmp;
            }
        }
        break;
#else
        for (i = 0; i < count; i++)
            dst[i] = mina_f64_from_bits(mina_le32(s + i * 8 + 4), mina_le32(s + i * 8));
        break;
#endif
    case MINA_FMT_F64BE:
        for (i = 0; i < count; i++)
            dst[i] = mina_f64_from_bits(mina_be32(s + i * 8), mina_be32(s + i * 8 + 4));
        break;

    case MINA_FMT_MULAW:
        for (i = 0; i < count; i++)
            dst[i] = (float)mina_mulaw_decode(s[i]) * (1.0f / 32768.0f);
        break;
    case MINA_FMT_ALAW:
        for (i = 0; i < count; i++)
            dst[i] = (float)mina_alaw_decode(s[i]) * (1.0f / 32768.0f);
        break;

    default:
        for (i = 0; i < count; i++) dst[i] = 0.0f;
        break;
    }
}

/* Round-half-away-from-zero, then clamp. */
static mina_i32 mina_quantise(float v, double scale, double lo, double hi) {
    double d = (double)v * scale;
    if (d != d) return 0;                     /* NaN */
    d = (d >= 0.0) ? (d + 0.5) : (d - 0.5);
    if (d > hi) d = hi;
    if (d < lo) d = lo;
    return (mina_i32)d;
}

void mina_convert_from_f32(const float *src, mina_format fmt,
                           mina_u32 channels, size_t count, void *dst) {
    mina_u8 *d = (mina_u8 *)dst;
    size_t i;
    (void)channels;
    if (!src || !dst) return;

    switch (fmt) {
    case MINA_FMT_U8:
        for (i = 0; i < count; i++)
            d[i] = (mina_u8)(mina_quantise(src[i], 127.0, -128.0, 127.0) + 128);
        break;

    case MINA_FMT_S16:
    case MINA_FMT_S16BE: {
        int be = (fmt == MINA_FMT_S16BE);
        for (i = 0; i < count; i++) {
            mina_u32 u = (mina_u32)mina_quantise(src[i], 32767.0, -32768.0, 32767.0);
            if (be) { d[i*2] = (mina_u8)(u >> 8); d[i*2+1] = (mina_u8)u; }
            else    { d[i*2] = (mina_u8)u; d[i*2+1] = (mina_u8)(u >> 8); }
        }
        break; }

    case MINA_FMT_S24:
    case MINA_FMT_S24BE: {
        int be = (fmt == MINA_FMT_S24BE);
        for (i = 0; i < count; i++) {
            mina_u32 u = (mina_u32)mina_quantise(src[i], 8388607.0,
                                                 -8388608.0, 8388607.0);
            if (be) { d[i*3] = (mina_u8)(u>>16); d[i*3+1] = (mina_u8)(u>>8); d[i*3+2] = (mina_u8)u; }
            else    { d[i*3] = (mina_u8)u; d[i*3+1] = (mina_u8)(u>>8); d[i*3+2] = (mina_u8)(u>>16); }
        }
        break; }

    case MINA_FMT_S32:
    case MINA_FMT_S32BE: {
        int be = (fmt == MINA_FMT_S32BE);
        for (i = 0; i < count; i++) {
            double v = (double)src[i] * 2147483647.0;
            mina_u32 u;
            if (v != v) v = 0.0;
            v = (v >= 0.0) ? (v + 0.5) : (v - 0.5);
            if (v >  2147483647.0) v =  2147483647.0;
            if (v < -2147483648.0) v = -2147483648.0;
            u = (v < 0.0) ? (mina_u32)(4294967296.0 + v) : (mina_u32)v;
            if (be) { d[i*4]=(mina_u8)(u>>24); d[i*4+1]=(mina_u8)(u>>16);
                      d[i*4+2]=(mina_u8)(u>>8); d[i*4+3]=(mina_u8)u; }
            else    { d[i*4]=(mina_u8)u; d[i*4+1]=(mina_u8)(u>>8);
                      d[i*4+2]=(mina_u8)(u>>16); d[i*4+3]=(mina_u8)(u>>24); }
        }
        break; }

    case MINA_FMT_F32:
#ifdef MINA_IEEE_LE
        memcpy(d, src, count * 4);
        break;
#else
        for (i = 0; i < count; i++) {
            mina_u32 u = mina_f32_to_bits(src[i]);
            d[i*4]=(mina_u8)u; d[i*4+1]=(mina_u8)(u>>8);
            d[i*4+2]=(mina_u8)(u>>16); d[i*4+3]=(mina_u8)(u>>24);
        }
        break;
#endif
    case MINA_FMT_F32BE:
        for (i = 0; i < count; i++) {
            mina_u32 u = mina_f32_to_bits(src[i]);
            d[i*4]=(mina_u8)(u>>24); d[i*4+1]=(mina_u8)(u>>16);
            d[i*4+2]=(mina_u8)(u>>8); d[i*4+3]=(mina_u8)u;
        }
        break;

    case MINA_FMT_F64:
    case MINA_FMT_F64BE: {
        int be = (fmt == MINA_FMT_F64BE);
        for (i = 0; i < count; i++) {
            /* float -> double is exact, so re-encoding through binary32 bits
             * and widening the exponent is lossless. */
            mina_u32 b = mina_f32_to_bits(src[i]);
            mina_u32 sign = b & 0x80000000UL, mant = b & 0x7FFFFFUL;
            int e = (int)((b >> 23) & 0xFFu);
            mina_u32 hi, lo;
            if (e == 0 && mant == 0) { hi = sign; lo = 0; }
            else if (e == 0xFF)      { hi = sign | 0x7FF00000UL; lo = 0; }
            else if (e == 0) {
                /* binary32 subnormal: renormalise into binary64's range */
                int sh = 0;
                while (!(mant & 0x800000UL)) { mant <<= 1; sh++; }
                mant &= 0x7FFFFFUL;
                hi = sign | ((mina_u32)(1023 - 126 - sh) << 20) | (mant >> 3);
                lo = mant << 29;
            } else {
                hi = sign | ((mina_u32)(e - 127 + 1023) << 20) | (mant >> 3);
                lo = mant << 29;
            }
            if (be) { d[i*8]=(mina_u8)(hi>>24); d[i*8+1]=(mina_u8)(hi>>16);
                      d[i*8+2]=(mina_u8)(hi>>8); d[i*8+3]=(mina_u8)hi;
                      d[i*8+4]=(mina_u8)(lo>>24); d[i*8+5]=(mina_u8)(lo>>16);
                      d[i*8+6]=(mina_u8)(lo>>8); d[i*8+7]=(mina_u8)lo; }
            else    { d[i*8]=(mina_u8)lo; d[i*8+1]=(mina_u8)(lo>>8);
                      d[i*8+2]=(mina_u8)(lo>>16); d[i*8+3]=(mina_u8)(lo>>24);
                      d[i*8+4]=(mina_u8)hi; d[i*8+5]=(mina_u8)(hi>>8);
                      d[i*8+6]=(mina_u8)(hi>>16); d[i*8+7]=(mina_u8)(hi>>24); }
        }
        break; }

    case MINA_FMT_MULAW:
        for (i = 0; i < count; i++)
            d[i] = mina_mulaw_encode(mina_quantise(src[i], 32767.0, -32768.0, 32767.0));
        break;
    case MINA_FMT_ALAW:
        for (i = 0; i < count; i++)
            d[i] = mina_alaw_encode(mina_quantise(src[i], 32767.0, -32768.0, 32767.0));
        break;

    default:
        break;
    }
}

/* ------------------------------------------------------------------ */
/* WAV (RIFF/WAVE) decoder + encoder                                    */
/* ------------------------------------------------------------------ */
#define MINA_WAVE_PCM        0x0001
#define MINA_WAVE_FLOAT      0x0003
#define MINA_WAVE_ALAW       0x0006
#define MINA_WAVE_MULAW      0x0007
#define MINA_WAVE_EXTENSIBLE 0xFFFE

typedef struct {
    mina_u16 format_tag;
    mina_u16 channels;
    mina_u32 sample_rate;
    mina_u32 byte_rate;
    mina_u16 block_align;
    mina_u16 bits;
    mina_u16 valid_bits;
    mina_u32 channel_mask;
    mina_u16 sub_format;      /* real format when tag == EXTENSIBLE */
    const mina_u8 *data_ptr;
    size_t         data_size;
    int            truncated; /* data chunk claimed more than the file has */
} mina_wav_hdr;

static mina_bool mina_wav_probe(const mina_u8 *d, size_t n) {
    if (n < 12) return 0;
    if (memcmp(d + 8, "WAVE", 4) != 0) return 0;
    return memcmp(d, "RIFF", 4) == 0 || memcmp(d, "RIFX", 4) == 0 ||
           memcmp(d, "RF64", 4) == 0 || memcmp(d, "BW64", 4) == 0;
}

static mina_result mina_wav_parse(const mina_u8 *d, size_t n, mina_wav_hdr *wi) {
    size_t off = 12;
    int big = 0;
    int got_fmt = 0;

    memset(wi, 0, sizeof(*wi));
    if (!mina_wav_probe(d, n)) return MINA_ERR_NOTFOUND;
    big = (memcmp(d, "RIFX", 4) == 0);

    while (off + 8 <= n) {
        mina_u32 sz;
        const mina_u8 *id = d + off;
        const mina_u8 *body = d + off + 8;
        size_t avail = n - (off + 8);
        size_t step;

        sz = big ? mina_be32(d + off + 4) : mina_le32(d + off + 4);

        if (memcmp(id, "fmt ", 4) == 0) {
            size_t take = (sz < avail) ? (size_t)sz : avail;
            if (take < 16) return MINA_ERR_INVALID;
            wi->format_tag  = big ? mina_be16(body)     : mina_le16(body);
            wi->channels    = big ? mina_be16(body + 2) : mina_le16(body + 2);
            wi->sample_rate = big ? mina_be32(body + 4) : mina_le32(body + 4);
            wi->byte_rate   = big ? mina_be32(body + 8) : mina_le32(body + 8);
            wi->block_align = big ? mina_be16(body +12) : mina_le16(body + 12);
            wi->bits        = big ? mina_be16(body +14) : mina_le16(body + 14);
            if (wi->format_tag == MINA_WAVE_EXTENSIBLE && take >= 40) {
                wi->valid_bits   = big ? mina_be16(body+18) : mina_le16(body + 18);
                wi->channel_mask = big ? mina_be32(body+20) : mina_le32(body + 20);
                wi->sub_format   = big ? mina_be16(body+24) : mina_le16(body + 24);
            }
            got_fmt = 1;
        } else if (memcmp(id, "data", 4) == 0) {
            wi->data_ptr = body;
            if ((size_t)sz > avail || sz == 0xFFFFFFFFUL) {
                /* truncated, or a streaming writer that never patched the
                 * size: take everything that is actually present */
                wi->data_size = avail;
                if (sz != 0xFFFFFFFFUL && sz != 0) wi->truncated = 1;
            } else {
                wi->data_size = (size_t)sz;
            }
            /* keep scanning: a later fmt chunk is malformed but harmless */
        }

        /* advance, guarding against size_t overflow and zero-progress loops */
        step = 8;
        if ((size_t)sz > n - off - 8) break;
        step += (size_t)sz;
        if (sz & 1u) {
            if (off + step + 1 > n) break;
            step += 1;
        }
        if (step == 0) break;
        off += step;
    }

    if (!got_fmt || wi->data_ptr == NULL) return MINA_ERR_INVALID;
    if (wi->channels == 0 || wi->channels > 4096) return MINA_ERR_INVALID;
    return MINA_OK;
}

static mina_format mina_wav_to_format(const mina_wav_hdr *wi, int big) {
    mina_u16 tag = wi->format_tag;
    if (tag == MINA_WAVE_EXTENSIBLE) tag = wi->sub_format ? wi->sub_format : MINA_WAVE_PCM;
    switch (tag) {
    case MINA_WAVE_PCM:
        switch (wi->bits) {
        case 8:  return MINA_FMT_U8;
        case 16: return big ? MINA_FMT_S16BE : MINA_FMT_S16;
        case 24: return big ? MINA_FMT_S24BE : MINA_FMT_S24;
        case 32: return big ? MINA_FMT_S32BE : MINA_FMT_S32;
        default: break;
        }
        return (mina_format)-1;
    case MINA_WAVE_FLOAT:
        if (wi->bits == 32) return big ? MINA_FMT_F32BE : MINA_FMT_F32;
        if (wi->bits == 64) return big ? MINA_FMT_F64BE : MINA_FMT_F64;
        return (mina_format)-1;
    case MINA_WAVE_ALAW:  return MINA_FMT_ALAW;
    case MINA_WAVE_MULAW: return MINA_FMT_MULAW;
    default: break;
    }
    return (mina_format)-1;
}

static mina_result mina_wav_decode(const mina_u8 *d, size_t n, mina_pcm *out) {
    mina_wav_hdr wi;
    mina_format fmt;
    mina_result r;
    size_t frames, ssize, stride, count;
    mina_u32 ch;
    int big;

    r = mina_wav_parse(d, n, &wi);
    if (r != MINA_OK) return r;
    big = (memcmp(d, "RIFX", 4) == 0);
    fmt = mina_wav_to_format(&wi, big);
    if ((int)fmt < 0) return MINA_ERR_UNSUPPORTED;

    ch = wi.channels;
    ssize = mina_format_size(fmt);
    if (!ssize) return MINA_ERR_UNSUPPORTED;

    /* Trust block_align only when it is consistent with the sample format;
     * a bogus value must not make us read outside the data chunk. */
    stride = (size_t)ch * ssize;
    if (wi.block_align && (size_t)wi.block_align >= stride)
        stride = (size_t)wi.block_align;

    frames = wi.data_size / stride;
    if (frames && (size_t)ch > MINA_MAX_SAMPLES / frames) return MINA_ERR_INVALID;
    count = frames * (size_t)ch;

    out->channels = ch;
    out->sample_rate = wi.sample_rate;
    out->frames = mina_u64_from((mina_u32)frames);
    out->samples = (float *)MINA_MALLOC((count ? count : 1) * sizeof(float));
    if (!out->samples) return MINA_ERR_NOMEM;
    out->samples[0] = 0.0f;

    if (stride == (size_t)ch * ssize) {
        mina_convert_to_f32(wi.data_ptr, fmt, ch, count, out->samples);
    } else {
        /* padded frames: convert one frame at a time */
        size_t i;
        for (i = 0; i < frames; i++)
            mina_convert_to_f32(wi.data_ptr + i * stride, fmt, ch, ch,
                                out->samples + i * ch);
    }
    return wi.truncated ? MINA_ERR_TRUNCATED : MINA_OK;
}

static mina_result mina_wav_info(const mina_u8 *d, size_t n, mina_fileinfo *out) {
    mina_wav_hdr wi;
    mina_result r = mina_wav_parse(d, n, &wi);
    size_t stride;
    mina_u32 frames;
    if (r != MINA_OK) return r;
    mina_strcpy_n(out->codec, sizeof(out->codec), "wav");
    out->sample_rate = wi.sample_rate;
    out->channels = wi.channels;
    out->bits_per_sample = wi.bits;
    stride = wi.block_align ? (size_t)wi.block_align
                            : (size_t)wi.channels * ((wi.bits + 7u) / 8u);
    if (!stride) stride = 1;
    frames = (mina_u32)(wi.data_size / stride);
    out->total_frames = mina_u64_from(frames);
    out->duration_seconds = wi.sample_rate ? (double)frames / (double)wi.sample_rate : 0.0;
    out->bitrate_kbps = (mina_u32)(wi.byte_rate / 125u);   /* bytes/s * 8 / 1000 */
    return MINA_OK;
}

/* ---- WAV encoder ---- */
static void mina_put_le16(mina_u8 *p, mina_u32 v) {
    p[0] = (mina_u8)v; p[1] = (mina_u8)(v >> 8);
}
static void mina_put_le32(mina_u8 *p, mina_u32 v) {
    p[0] = (mina_u8)v; p[1] = (mina_u8)(v >> 8);
    p[2] = (mina_u8)(v >> 16); p[3] = (mina_u8)(v >> 24);
}

size_t mina_wav_write(const float *samples, mina_u64 frames, mina_u32 channels,
                      mina_u32 sample_rate, int bits, mina_u8 **out_data) {
    mina_format fmt;
    size_t ssize, nframes, count, data_sz, total;
    mina_u8 *buf;
    mina_u32 tag, block_align;
    int ovf = 0;

    if (out_data) *out_data = NULL;
    if (!samples || !out_data || !channels) return 0;

    switch (bits) {
    case 8:               fmt = MINA_FMT_U8;  tag = MINA_WAVE_PCM;   break;
    case 16:              fmt = MINA_FMT_S16; tag = MINA_WAVE_PCM;   break;
    case 24:              fmt = MINA_FMT_S24; tag = MINA_WAVE_PCM;   break;
    case 32:              fmt = MINA_FMT_S32; tag = MINA_WAVE_PCM;   break;
    case MINA_WAV_FLOAT32:fmt = MINA_FMT_F32; tag = MINA_WAVE_FLOAT; bits = 32; break;
    default: return 0;
    }
    ssize = mina_format_size(fmt);
    nframes = mina_u64_size(frames, &ovf);
    if (ovf) return 0;
    if (nframes && (size_t)channels > (size_t)-1 / nframes) return 0;
    count = nframes * (size_t)channels;
    if (count && ssize > ((size_t)-1 - 44) / count) return 0;
    data_sz = count * ssize;
    total = 44 + data_sz;

    buf = (mina_u8 *)MINA_MALLOC(total ? total : 1);
    if (!buf) return 0;

    block_align = channels * (mina_u32)ssize;
    memcpy(buf, "RIFF", 4);
    mina_put_le32(buf + 4, (mina_u32)(total - 8));
    memcpy(buf + 8, "WAVE", 4);
    memcpy(buf + 12, "fmt ", 4);
    mina_put_le32(buf + 16, 16);
    mina_put_le16(buf + 20, tag);
    mina_put_le16(buf + 22, channels);
    mina_put_le32(buf + 24, sample_rate);
    mina_put_le32(buf + 28, sample_rate * block_align);
    mina_put_le16(buf + 32, block_align);
    mina_put_le16(buf + 34, (mina_u32)bits);
    memcpy(buf + 36, "data", 4);
    mina_put_le32(buf + 40, (mina_u32)data_sz);
    mina_convert_from_f32(samples, fmt, channels, count, buf + 44);

    *out_data = buf;
    return total;
}

/* ------------------------------------------------------------------ */
/* FLAC decoder                                                         */
/*                                                                      */
/* RFC 9639. All subframe types, both residual coding methods, all       */
/* stereo decorrelations, wasted bits, CRC-8 header and CRC-16 frame     */
/* verification. No fixed-size stack buffers: everything is sized from   */
/* STREAMINFO.                                                          */
/* ------------------------------------------------------------------ */
#ifndef MINA_NO_FLAC

#define MINA_FLAC_MAX_CH    8
#define MINA_FLAC_MAX_BLOCK 65536
#define MINA_FLAC_MAX_ORDER 32

typedef struct {
    mina_u64 total_samples;
    mina_u32 sample_rate;
    mina_u32 channels;
    mina_u32 bits_per_sample;
    mina_u32 max_blocksize;
} mina_flac_si;

typedef struct {
    mina_i32 *chdata;              /* channels * blockcap                */
    mina_i32 *residual;            /* blockcap                           */
    size_t    blockcap;
    mina_u32  channels;
} mina_flac_dec;

static int mina_flac_dec_init(mina_flac_dec *fd, mina_u32 channels, size_t blockcap) {
    fd->chdata = NULL; fd->residual = NULL;
    fd->blockcap = 0; fd->channels = channels;
    if (!channels || channels > MINA_FLAC_MAX_CH) return 0;
    if (!blockcap || blockcap > MINA_FLAC_MAX_BLOCK) return 0;
    fd->chdata = (mina_i32 *)MINA_MALLOC((size_t)channels * blockcap * sizeof(mina_i32));
    fd->residual = (mina_i32 *)MINA_MALLOC(blockcap * sizeof(mina_i32));
    if (!fd->chdata || !fd->residual) {
        MINA_FREE(fd->chdata); MINA_FREE(fd->residual);
        fd->chdata = NULL; fd->residual = NULL;
        return 0;
    }
    fd->blockcap = blockcap;
    return 1;
}

/* STREAMINFO's max blocksize is advisory; some encoders understate it.
 * Grow rather than reject when a frame turns out to be larger. */
static int mina_flac_dec_grow(mina_flac_dec *fd, size_t need) {
    mina_i32 *nc, *nr;
    if (need <= fd->blockcap) return 1;
    if (need > MINA_FLAC_MAX_BLOCK) return 0;
    nc = (mina_i32 *)MINA_REALLOC(fd->chdata,
             (size_t)fd->channels * need * sizeof(mina_i32));
    if (!nc) return 0;
    fd->chdata = nc;
    nr = (mina_i32 *)MINA_REALLOC(fd->residual, need * sizeof(mina_i32));
    if (!nr) return 0;
    fd->residual = nr;
    fd->blockcap = need;
    return 1;
}
static void mina_flac_dec_free(mina_flac_dec *fd) {
    MINA_FREE(fd->chdata); MINA_FREE(fd->residual);
    fd->chdata = NULL; fd->residual = NULL;
}

/* Coded block sizes: 0 = reserved, 6/7 = explicit follows. */
static int mina_flac_bsize_code(mina_u32 *bs, mina_u8 code) {
    switch (code) {
    case 1:  *bs = 192;   return 0;
    case 2:  *bs = 576;   return 0;
    case 3:  *bs = 1152;  return 0;
    case 4:  *bs = 2304;  return 0;
    case 5:  *bs = 4608;  return 0;
    case 6:  return 1;
    case 7:  return 2;
    case 8:  *bs = 256;   return 0;
    case 9:  *bs = 512;   return 0;
    case 10: *bs = 1024;  return 0;
    case 11: *bs = 2048;  return 0;
    case 12: *bs = 4096;  return 0;
    case 13: *bs = 8192;  return 0;
    case 14: *bs = 16384; return 0;
    case 15: *bs = 32768; return 0;
    default: break;
    }
    return -1;
}
static int mina_flac_srate_code(mina_u32 *sr, mina_u8 code) {
    switch (code) {
    case 0:  return 3;              /* take it from STREAMINFO */
    case 1:  *sr = 88200;  return 0;
    case 2:  *sr = 176400; return 0;
    case 3:  *sr = 192000; return 0;
    case 4:  *sr = 8000;   return 0;
    case 5:  *sr = 16000;  return 0;
    case 6:  *sr = 22050;  return 0;
    case 7:  *sr = 24000;  return 0;
    case 8:  *sr = 32000;  return 0;
    case 9:  *sr = 44100;  return 0;
    case 10: *sr = 48000;  return 0;
    case 11: *sr = 96000;  return 0;
    case 12: return 1;              /* 8-bit kHz  */
    case 13: return 2;              /* 16-bit Hz  */
    case 14: return 2;              /* 16-bit daHz */
    default: break;
    }
    return -1;                      /* 15 is invalid */
}
/* 0 means "from STREAMINFO"; 3 and 7 are reserved. */
static mina_u32 mina_flac_bps(mina_u8 code) {
    switch (code) {
    case 0: return 0;
    case 1: return 8;
    case 2: return 12;
    case 4: return 16;
    case 5: return 20;
    case 6: return 24;
    case 7: return 32;
    default: break;
    }
    return 0xFFFFFFFFUL;            /* reserved */
}

/* Partitioned-Rice residual into residual[0 .. blocksize-order). */
static void mina_flac_residual(mina_br *b, mina_u32 blocksize, mina_u32 order,
                               mina_i32 *residual) {
    mina_u32 method, param_bits, partition_order, partitions, per_partition;
    mina_u32 p, i, offset = 0;

    method = mina_br_u(b, 2);
    if (method >= 2) { b->err = 1; return; }
    param_bits = (method == 0) ? 4u : 5u;
    partition_order = mina_br_u(b, 4);
    if (partition_order > 15) { b->err = 1; return; }
    partitions = 1UL << partition_order;
    if ((blocksize & (partitions - 1u)) != 0) { b->err = 1; return; }
    per_partition = blocksize >> partition_order;
    if (per_partition < order) { b->err = 1; return; }

    for (p = 0; p < partitions; p++) {
        mina_u32 count = (p == 0) ? (per_partition - order) : per_partition;
        mina_u32 rice = mina_br_u(b, param_bits);
        if (b->err) return;
        if (rice == ((1UL << param_bits) - 1UL)) {
            mina_u32 q = mina_br_u(b, 5);
            if (q > 32) { b->err = 1; return; }
            for (i = 0; i < count; i++) residual[offset + i] = mina_br_s(b, (unsigned)q);
        } else {
            for (i = 0; i < count; i++) {
                mina_u32 u = mina_br_unary(b);
                mina_u32 rem = rice ? mina_br_u(b, (unsigned)rice) : 0u;
                mina_u32 mag;
                if (b->err) return;
                if (rice && u > (0xFFFFFFFFUL >> rice)) { b->err = 1; return; }
                mag = (u << rice) | rem;
                /* zig-zag: 0,-1,1,-2,2,... without signed overflow */
                residual[offset + i] =
                    mina_u32_to_i32((mag >> 1) ^ (mina_u32)(0u - (mag & 1u)));
            }
        }
        offset += count;
        if (b->err) return;
    }
}

static void mina_flac_subframe(mina_br *b, mina_flac_dec *fd,
                               mina_u32 blocksize, mina_u32 bps, mina_i32 *out) {
    mina_u32 hdr = mina_br_u(b, 8);
    mina_u32 i, type, wasted = 0, eff_bps;

    if (b->err) return;
    if (hdr & 0x80u) { b->err = 1; return; }
    type = (hdr >> 1) & 0x3Fu;
    if (hdr & 1u) {
        wasted = mina_br_unary(b) + 1u;
        if (b->err || wasted >= bps) { b->err = 1; return; }
    }
    eff_bps = bps - wasted;
    if (eff_bps == 0 || eff_bps > 32) { b->err = 1; return; }

    if (type == 0) {                                   /* CONSTANT */
        mina_i32 v = mina_br_s(b, (unsigned)eff_bps);
        for (i = 0; i < blocksize; i++) out[i] = v;
    } else if (type == 1) {                            /* VERBATIM */
        for (i = 0; i < blocksize; i++) out[i] = mina_br_s(b, (unsigned)eff_bps);
    } else if (type >= 8 && type <= 12) {              /* FIXED 0..4 */
        mina_u32 order = type - 8u;
        if (order > blocksize) { b->err = 1; return; }
        for (i = 0; i < order; i++) out[i] = mina_br_s(b, (unsigned)eff_bps);
        mina_flac_residual(b, blocksize, order, fd->residual);
        if (b->err) return;
        for (i = order; i < blocksize; i++) {
            mina_acc sum = mina_acc_from(fd->residual[i - order]);
            switch (order) {
            case 0: break;
            case 1: sum = mina_acc_addmul(sum, out[i-1],  1); break;
            case 2: sum = mina_acc_addmul(sum, out[i-1],  2);
                    sum = mina_acc_addmul(sum, out[i-2], -1); break;
            case 3: sum = mina_acc_addmul(sum, out[i-1],  3);
                    sum = mina_acc_addmul(sum, out[i-2], -3);
                    sum = mina_acc_addmul(sum, out[i-3],  1); break;
            default:sum = mina_acc_addmul(sum, out[i-1],  4);
                    sum = mina_acc_addmul(sum, out[i-2], -6);
                    sum = mina_acc_addmul(sum, out[i-3],  4);
                    sum = mina_acc_addmul(sum, out[i-4], -1); break;
            }
            out[i] = mina_acc_shr32(sum, 0);
        }
    } else if (type >= 32 && type <= 63) {             /* LPC 1..32 */
        mina_u32 order = type - 31u;
        mina_u32 prec;
        mina_i32 shift;
        mina_i32 coeffs[MINA_FLAC_MAX_ORDER];
        if (order > blocksize) { b->err = 1; return; }
        for (i = 0; i < order; i++) out[i] = mina_br_s(b, (unsigned)eff_bps);
        prec = mina_br_u(b, 4) + 1u;
        if (prec > 15) { b->err = 1; return; }          /* 0b1111 is invalid */
        shift = mina_br_s(b, 5);
        if (shift < 0) { b->err = 1; return; }          /* negative is invalid */
        for (i = 0; i < order; i++) coeffs[i] = mina_br_s(b, (unsigned)prec);
        mina_flac_residual(b, blocksize, order, fd->residual);
        if (b->err) return;
        for (i = order; i < blocksize; i++) {
            mina_acc sum = mina_acc_zero();
            mina_u32 j;
            for (j = 0; j < order; j++)
                sum = mina_acc_addmul(sum, coeffs[j], out[i - j - 1]);
            out[i] = mina_u32_to_i32((mina_u32)fd->residual[i - order] +
                                     (mina_u32)mina_acc_shr32(sum, (unsigned)shift));
        }
    } else {
        b->err = 1;
        return;
    }

    if (wasted)
        for (i = 0; i < blocksize; i++) out[i] = mina_shl_i32(out[i], (unsigned)wasted);
}

static mina_bool mina_flac_probe(const mina_u8 *d, size_t n) {
    return (mina_bool)(n >= 4 && memcmp(d, "fLaC", 4) == 0);
}

/* Walk the metadata blocks; fills si and returns the offset of the first
 * audio frame, or 0 on failure. */
static size_t mina_flac_metadata(const mina_u8 *d, size_t n, mina_flac_si *si) {
    size_t off = 4;
    int last = 0, seen_streaminfo = 0;

    memset(si, 0, sizeof(*si));
    si->total_samples = mina_u64_from(0);
    if (n < 4 || memcmp(d, "fLaC", 4) != 0) return 0;

    while (!last) {
        mina_u32 len;
        mina_u8 type;
        if (off + 4 > n) return 0;
        last = (d[off] & 0x80u) ? 1 : 0;
        type = (mina_u8)(d[off] & 0x7Fu);
        len = mina_be24(d + off + 1);
        off += 4;
        if (len > n - off) return 0;
        if (type == 0 && !seen_streaminfo) {
            const mina_u8 *p = d + off;
            if (len < 34) return 0;
            si->max_blocksize   = mina_be16(p + 2);
            si->sample_rate     = ((mina_u32)p[10] << 12) | ((mina_u32)p[11] << 4) |
                                  ((mina_u32)p[12] >> 4);
            si->channels        = (((mina_u32)p[12] >> 1) & 0x07u) + 1u;
            si->bits_per_sample = ((((mina_u32)p[12] & 0x01u) << 4) |
                                   ((mina_u32)p[13] >> 4)) + 1u;
            si->total_samples   = mina_u64_or(
                mina_u64_shl(mina_u64_from((mina_u32)(p[13] & 0x0Fu)), 32),
                mina_u64_from(mina_be32(p + 14)));
            seen_streaminfo = 1;
        } else if (type == 127) {
            return 0;                       /* invalid block type */
        }
        off += len;
    }
    if (!seen_streaminfo) return 0;
    if (!si->sample_rate || !si->channels || si->channels > MINA_FLAC_MAX_CH ||
        si->bits_per_sample < 4 || si->bits_per_sample > 32) return 0;
    return off;
}

static mina_result mina_flac_decode(const mina_u8 *d, size_t n, mina_pcm *out) {
    mina_flac_si si;
    mina_flac_dec fd;
    mina_fbuf fb;
    mina_br b;
    size_t off, blockcap;
    mina_result status = MINA_OK;
    mina_u64 limit;
    int have_limit;

    off = mina_flac_metadata(d, n, &si);
    if (!off) return mina_flac_probe(d, n) ? MINA_ERR_INVALID : MINA_ERR_NOTFOUND;

    blockcap = si.max_blocksize ? (size_t)si.max_blocksize : (size_t)MINA_FLAC_MAX_BLOCK;
    if (blockcap < 16 || blockcap > MINA_FLAC_MAX_BLOCK) blockcap = MINA_FLAC_MAX_BLOCK;
    if (!mina_flac_dec_init(&fd, si.channels, blockcap)) return MINA_ERR_NOMEM;

    mina_fbuf_init(&fb);
    mina_br_init(&b, d + off, n - off);
    limit = si.total_samples;
    have_limit = !mina_u64_zero(limit);

    while (mina_br_left(&b) >= 40 && !b.err) {
        const mina_u8 *fstart = b.p + (b.pos >> 3);
        mina_u32 b0, b1, bsize_code, srate_code, chan_code, bps_code;
        mina_u32 blocksize = 0, frame_rate = 0, chans, bps;
        int bsx, srx;
        mina_u32 crc8_read, i, ch;
        size_t frame_bit_start = b.pos;

        if ((b.pos & 7u) != 0) { b.err = 1; break; }

        b0 = mina_br_u(&b, 8);
        b1 = mina_br_u(&b, 8);
        if (b0 != 0xFFu || (b1 & 0xFCu) != 0xF8u) {
            /* Not a frame header. Either clean end of stream (padding) or
             * corruption: stop and report what we have. */
            b.pos = frame_bit_start;
            if (mina_br_left(&b) > 64) status = MINA_ERR_TRUNCATED;
            break;
        }

        bsize_code = mina_br_u(&b, 4);
        srate_code = mina_br_u(&b, 4);
        chan_code  = mina_br_u(&b, 4);
        bps_code   = mina_br_u(&b, 3);
        if (mina_br_u(&b, 1)) { status = MINA_ERR_TRUNCATED; break; }  /* reserved */

        {   /* UTF-8 coded frame or sample number */
            mina_u32 fb0 = mina_br_u(&b, 8);
            int ulen = mina_utf8_len((mina_u8)fb0);
            int k;
            if (ulen < 0) { status = MINA_ERR_TRUNCATED; break; }
            for (k = 1; k < ulen; k++) {
                mina_u32 cb = mina_br_u(&b, 8);
                if ((cb & 0xC0u) != 0x80u) { b.err = 1; break; }
            }
            if (b.err) { status = MINA_ERR_TRUNCATED; break; }
        }

        bsx = mina_flac_bsize_code(&blocksize, (mina_u8)bsize_code);
        srx = mina_flac_srate_code(&frame_rate, (mina_u8)srate_code);
        if (bsx < 0 || srx < 0) { status = MINA_ERR_TRUNCATED; break; }
        if (bsx == 1)      blocksize = mina_br_u(&b, 8) + 1u;
        else if (bsx == 2) blocksize = mina_br_u(&b, 16) + 1u;
        if (srx == 1)      frame_rate = mina_br_u(&b, 8) * 1000u;
        else if (srx == 2) {
            mina_u32 v = mina_br_u(&b, 16);
            frame_rate = (srate_code == 14u) ? v * 10u : v;
        } else if (srx == 3) frame_rate = si.sample_rate;

        crc8_read = mina_br_u(&b, 8);
        if (b.err) { status = MINA_ERR_TRUNCATED; break; }
        {
            size_t hdr_bytes = (b.pos >> 3) - (size_t)(fstart - b.p);
            if (mina_crc8(fstart, hdr_bytes - 1) != (mina_u8)crc8_read) {
                status = MINA_ERR_TRUNCATED;
                break;
            }
        }

        if (chan_code < 8) chans = chan_code + 1u;
        else if (chan_code <= 10) chans = 2u;
        else { status = MINA_ERR_TRUNCATED; break; }    /* 11..15 reserved */

        bps = mina_flac_bps((mina_u8)bps_code);
        if (bps == 0xFFFFFFFFUL) { status = MINA_ERR_TRUNCATED; break; }
        if (bps == 0) bps = si.bits_per_sample;

        if (!blocksize || !frame_rate || chans != si.channels ||
            bps < 4 || bps > 32) {
            status = MINA_ERR_TRUNCATED;
            break;
        }
        if (!mina_flac_dec_grow(&fd, (size_t)blocksize)) {
            status = (blocksize > MINA_FLAC_MAX_BLOCK) ? MINA_ERR_TRUNCATED
                                                       : MINA_ERR_NOMEM;
            break;
        }

        for (ch = 0; ch < chans; ch++) {
            /* the side channel of a decorrelated pair carries one extra bit */
            mina_u32 ch_bps = bps;
            if ((chan_code == 8u && ch == 1u) ||
                (chan_code == 9u && ch == 0u) ||
                (chan_code == 10u && ch == 1u)) ch_bps = bps + 1u;
            mina_flac_subframe(&b, &fd, blocksize, ch_bps,
                               fd.chdata + (size_t)ch * fd.blockcap);
            if (b.err) break;
        }
        if (b.err) { status = MINA_ERR_TRUNCATED; break; }

        /* undo stereo decorrelation */
        if (chan_code == 8u) {              /* left / side */
            mina_i32 *l = fd.chdata, *s = fd.chdata + fd.blockcap;
            for (i = 0; i < blocksize; i++)
                s[i] = mina_u32_to_i32((mina_u32)l[i] - (mina_u32)s[i]);
        } else if (chan_code == 9u) {       /* side / right */
            mina_i32 *s = fd.chdata, *r = fd.chdata + fd.blockcap;
            for (i = 0; i < blocksize; i++)
                s[i] = mina_u32_to_i32((mina_u32)s[i] + (mina_u32)r[i]);
        } else if (chan_code == 10u) {      /* mid / side */
            mina_i32 *m = fd.chdata, *s = fd.chdata + fd.blockcap;
            for (i = 0; i < blocksize; i++) {
                mina_u32 mid = (mina_u32)m[i] << 1;
                mina_i32 side = s[i];
                mid |= ((mina_u32)side & 1u);
                m[i] = mina_shr_i32(mina_u32_to_i32(mid + (mina_u32)side), 1);
                s[i] = mina_shr_i32(mina_u32_to_i32(mid - (mina_u32)side), 1);
            }
        }

        /* frame footer CRC-16 over everything from the sync code */
        mina_br_align(&b);
        {
            const mina_u8 *after = b.p + (b.pos >> 3);
            if ((size_t)(after - b.p) + 2 > b.size) { status = MINA_ERR_TRUNCATED; break; }
            if (mina_be16(after) != mina_crc16(fstart, (size_t)(after - fstart))) {
                status = MINA_ERR_TRUNCATED;
                break;
            }
            b.pos += 16;
        }

        /* emit */
        {
            mina_u32 emit = blocksize;
            float *dst;
            double scale = 1.0 / (double)((mina_u32)1 << (bps - 1));
            if (have_limit) {
                mina_u64 done = mina_u64_from((mina_u32)(fb.len / chans));
                mina_u64 room = mina_u64_lt(done, limit) ? mina_u64_sub(limit, done)
                                                         : mina_u64_from(0);
                mina_u32 r32 = mina_u64_lo(room);
                if (mina_u64_lt(room, mina_u64_from(blocksize))) emit = r32;
            }
            if (emit) {
                dst = mina_fbuf_extend(&fb, (size_t)emit * chans);
                if (!dst) break;
                for (i = 0; i < emit; i++) {
                    for (ch = 0; ch < chans; ch++)
                        dst[(size_t)i * chans + ch] =
                            (float)((double)fd.chdata[(size_t)ch * fd.blockcap + i] * scale);
                }
            }
            if (emit < blocksize) break;    /* reached the declared length */
        }
    }

    mina_flac_dec_free(&fd);
    if (fb.len == 0 && status != MINA_OK) {
        mina_fbuf_free(&fb);
        return MINA_ERR_INVALID;
    }
    return mina_fbuf_finish(&fb, out, si.channels, si.sample_rate, status);
}

static mina_result mina_flac_info(const mina_u8 *d, size_t n, mina_fileinfo *out) {
    mina_flac_si si;
    size_t off = mina_flac_metadata(d, n, &si);
    if (!off) return mina_flac_probe(d, n) ? MINA_ERR_INVALID : MINA_ERR_NOTFOUND;
    mina_strcpy_n(out->codec, sizeof(out->codec), "flac");
    out->sample_rate = si.sample_rate;
    out->channels = si.channels;
    out->bits_per_sample = si.bits_per_sample;
    out->total_frames = si.total_samples;
    out->duration_seconds = si.sample_rate
        ? mina_u64_dbl(si.total_samples) / (double)si.sample_rate : 0.0;
    out->bitrate_kbps = (out->duration_seconds > 0.0)
        ? (mina_u32)((double)n * 8.0 / out->duration_seconds / 1000.0) : 0;
    return MINA_OK;
}

#endif /* MINA_NO_FLAC */

/* ------------------------------------------------------------------ */
/* Ogg container: pages, packet assembly, codec identification          */
/* ------------------------------------------------------------------ */
typedef struct {
    const mina_u8 *page;     /* start of the page header */
    size_t   header_len;     /* 27 + segments            */
    size_t   body_len;
    mina_u8  type;           /* 0x01 continued, 0x02 bos, 0x04 eos */
    mina_u32 serial;
    mina_u32 seqno;
    mina_u64 granule;
    mina_u8  nsegs;
} mina_ogg_page;

static mina_bool mina_ogg_probe(const mina_u8 *d, size_t n) {
    return (mina_bool)(n >= 27 && memcmp(d, "OggS", 4) == 0 && d[4] == 0);
}

/* Parse the page starting at d+off. Returns its total size, or 0. */
static size_t mina_ogg_page_at(const mina_u8 *d, size_t n, size_t off,
                               mina_ogg_page *pg) {
    size_t body = 0, hdr;
    unsigned i, nsegs;
    if (off + 27 > n) return 0;
    if (memcmp(d + off, "OggS", 4) != 0 || d[off + 4] != 0) return 0;
    nsegs = d[off + 26];
    hdr = 27 + (size_t)nsegs;
    if (off + hdr > n) return 0;
    for (i = 0; i < nsegs; i++) body += d[off + 27 + i];
    if (body > n - off - hdr) return 0;
    pg->page = d + off;
    pg->header_len = hdr;
    pg->body_len = body;
    pg->type    = d[off + 5];
    pg->granule = mina_le64(d + off + 6);
    pg->serial  = mina_le32(d + off + 14);
    pg->seqno   = mina_le32(d + off + 18);
    pg->nsegs   = (mina_u8)nsegs;
    return hdr + body;
}

/* Identify the codec from the first packet of the first logical stream. */
static mina_result mina_ogg_scan(const mina_u8 *d, size_t n,
                                 char *codec, size_t codec_sz,
                                 mina_u32 *channels, mina_u32 *sample_rate,
                                 mina_u32 *serial) {
    mina_ogg_page pg;
    size_t sz;
    const mina_u8 *p;
    size_t body;

    *channels = 0; *sample_rate = 0; codec[0] = '\0';
    if (serial) *serial = 0;

    sz = mina_ogg_page_at(d, n, 0, &pg);
    if (!sz) return MINA_ERR_NOTFOUND;
    p = pg.page + pg.header_len;
    body = pg.body_len;
    if (serial) *serial = pg.serial;

    if (body >= 30 && p[0] == 0x01 && memcmp(p + 1, "vorbis", 6) == 0) {
        mina_strcpy_n(codec, codec_sz, "vorbis");
        *channels = p[11];
        *sample_rate = mina_le32(p + 12);
    } else if (body >= 19 && memcmp(p, "OpusHead", 8) == 0) {
        mina_strcpy_n(codec, codec_sz, "opus");
        *channels = p[9];
        *sample_rate = 48000;
    } else if (body >= 9 && p[0] == 0x7F && memcmp(p + 1, "FLAC", 4) == 0) {
        mina_strcpy_n(codec, codec_sz, "flac");
        if (body >= 51 && memcmp(p + 9, "fLaC", 4) == 0) {
            const mina_u8 *si = p + 13 + 4;    /* past fLaC + metadata header */
            *sample_rate = ((mina_u32)si[10] << 12) | ((mina_u32)si[11] << 4) |
                           ((mina_u32)si[12] >> 4);
            *channels = (((mina_u32)si[12] >> 1) & 0x07u) + 1u;
        }
    } else if (body >= 80 && memcmp(p, "Speex   ", 8) == 0) {
        mina_strcpy_n(codec, codec_sz, "speex");
        *sample_rate = mina_le32(p + 36);
        *channels = mina_le32(p + 48);
    } else if (body >= 7 && p[0] == 0x80 && memcmp(p + 1, "theora", 6) == 0) {
        mina_strcpy_n(codec, codec_sz, "theora");
    } else if (body >= 28 && memcmp(p, "PCM     ", 8) == 0) {
        mina_strcpy_n(codec, codec_sz, "oggpcm");
        *sample_rate = mina_be32(p + 16);
        *channels = mina_be16(p + 22);
    } else {
        mina_strcpy_n(codec, codec_sz, "ogg");
    }
    return MINA_OK;
}

/* Last granule position seen for the given serial number. */
static mina_u64 mina_ogg_last_granule(const mina_u8 *d, size_t n, mina_u32 serial) {
    mina_ogg_page pg;
    size_t off = 0, sz;
    mina_u64 last = mina_u64_from(0);
    mina_u64 none = mina_u64_sub(mina_u64_from(0), mina_u64_from(1)); /* ~0 */
    while ((sz = mina_ogg_page_at(d, n, off, &pg)) != 0) {
        if (pg.serial == serial && !mina_u64_eq(pg.granule, none))
            last = pg.granule;
        off += sz;
    }
    return last;
}

static mina_result mina_ogg_info(const mina_u8 *d, size_t n, mina_fileinfo *out) {
    char codec[16];
    mina_u32 ch = 0, sr = 0, serial = 0;
    mina_result r;

    memset(codec, 0, sizeof(codec));   /* LTO cannot see that scan fills it */
    memset(out, 0, sizeof(*out));
    r = mina_ogg_scan(d, n, codec, sizeof(codec), &ch, &sr, &serial);
    if (r != MINA_OK) return r;
    mina_strcpy_n(out->codec, sizeof(out->codec), codec);
    out->channels = ch;
    out->sample_rate = sr;
    out->total_frames = mina_ogg_last_granule(d, n, serial);
    if (sr) out->duration_seconds = mina_u64_dbl(out->total_frames) / (double)sr;
    out->bitrate_kbps = (out->duration_seconds > 0.0)
        ? (mina_u32)((double)n * 8.0 / out->duration_seconds / 1000.0) : 0;
    return MINA_OK;
}

/* ---- packet assembly (only the Vorbis decoder needs it) ---- */
#ifndef MINA_NO_VORBIS
typedef struct {
    size_t   offset;      /* into the arena */
    size_t   len;
    mina_u64 granule;     /* granule of the page the packet completed on */
    int      has_granule;
} mina_ogg_pktref;

typedef struct {
    mina_u8        *arena;
    size_t          arena_len, arena_cap;
    mina_ogg_pktref *pkt;
    size_t          npkt, pkt_cap;
    int             oom;
    int             truncated;   /* stream ended mid-packet */
} mina_ogg_packets;

static void mina_ogg_packets_free(mina_ogg_packets *ps) {
    MINA_FREE(ps->arena); MINA_FREE(ps->pkt);
    ps->arena = NULL; ps->pkt = NULL;
    ps->arena_len = ps->arena_cap = ps->npkt = ps->pkt_cap = 0;
}

static int mina_ogg_arena_push(mina_ogg_packets *ps, const mina_u8 *src, size_t len) {
    if (ps->arena_len + len > ps->arena_cap) {
        size_t ncap = ps->arena_cap ? ps->arena_cap : 8192;
        mina_u8 *na;
        while (ncap < ps->arena_len + len) {
            if (ncap > ((size_t)-1) / 2) return 0;
            ncap *= 2;
        }
        na = (mina_u8 *)MINA_REALLOC(ps->arena, ncap);
        if (!na) return 0;
        ps->arena = na; ps->arena_cap = ncap;
    }
    memcpy(ps->arena + ps->arena_len, src, len);
    ps->arena_len += len;
    return 1;
}

static int mina_ogg_pkt_push(mina_ogg_packets *ps, size_t off, size_t len,
                             mina_u64 granule, int has_granule) {
    if (ps->npkt == ps->pkt_cap) {
        size_t ncap = ps->pkt_cap ? ps->pkt_cap * 2 : 64;
        mina_ogg_pktref *np = (mina_ogg_pktref *)
            MINA_REALLOC(ps->pkt, ncap * sizeof(mina_ogg_pktref));
        if (!np) return 0;
        ps->pkt = np; ps->pkt_cap = ncap;
    }
    ps->pkt[ps->npkt].offset = off;
    ps->pkt[ps->npkt].len = len;
    ps->pkt[ps->npkt].granule = granule;
    ps->pkt[ps->npkt].has_granule = has_granule;
    ps->npkt++;
    return 1;
}

/* Collect every packet of one logical stream. Packets spanning pages are
 * reassembled; a packet left incomplete at end of stream is dropped and
 * flagged. */
static int mina_ogg_collect(const mina_u8 *d, size_t n, mina_u32 serial,
                            mina_ogg_packets *ps) {
    mina_ogg_page pg;
    size_t off = 0, sz;
    size_t pending_start = 0;
    int    pending = 0;
    mina_u64 none = mina_u64_sub(mina_u64_from(0), mina_u64_from(1));

    memset(ps, 0, sizeof(*ps));
    while ((sz = mina_ogg_page_at(d, n, off, &pg)) != 0) {
        const mina_u8 *seg = pg.page + 27;
        const mina_u8 *payload = pg.page + pg.header_len;
        unsigned i;
        if (pg.serial != serial) { off += sz; continue; }
        if (!(pg.type & 0x01u) && pending) {
            /* a fresh page that is not a continuation abandons the partial */
            ps->arena_len = pending_start;
            pending = 0;
        }
        for (i = 0; i < pg.nsegs; i++) {
            unsigned l = seg[i];
            if (!pending) { pending_start = ps->arena_len; pending = 1; }
            if (l && !mina_ogg_arena_push(ps, payload, l)) { ps->oom = 1; return 0; }
            payload += l;
            if (l < 255u) {
                int last_seg = (i + 1u == pg.nsegs);
                if (!mina_ogg_pkt_push(ps, pending_start,
                                       ps->arena_len - pending_start,
                                       pg.granule,
                                       last_seg && !mina_u64_eq(pg.granule, none))) {
                    ps->oom = 1; return 0;
                }
                pending = 0;
            }
        }
        off += sz;
    }
    if (pending) { ps->arena_len = pending_start; ps->truncated = 1; }
    if (off < n) ps->truncated = 1;
    return 1;
}
#endif /* MINA_NO_VORBIS */

/* ------------------------------------------------------------------ */
/* MP3 decoder constants.                                              */
/*                                                                      */
/* The Huffman data is stored in Mina's run-length form and expanded    */
/* into decoder-owned storage at initialization. The values themselves */
/* are codec lookup data and must not be changed.                       */
/* ------------------------------------------------------------------ */
#ifndef MINA_NO_MP3

typedef struct { mina_i16 value; mina_u8 count; } mina_mp3_tab_run;
typedef struct { mina_u8 value; mina_u8 count; } mina_mp3_byte_run;
static const mina_mp3_tab_run mina_mp3_tab_runs[] = {
    {0,32},{785,4},{784,4},{513,8},{256,16},{-255,1},{1313,1},{1298,1},
    {1282,1},{785,4},{784,4},{769,4},{256,16},{290,1},{288,1},{-255,1},
    {1313,1},{1298,1},{1282,1},{769,4},{529,8},{528,8},{512,8},{290,1},
    {288,1},{-253,1},{-318,1},{-351,1},{-367,1},{785,4},{784,4},{769,4},
    {256,16},{819,1},{818,1},{547,2},{275,4},{561,1},{560,1},{515,1},
    {546,1},{289,1},{274,1},{288,1},{258,1},{-254,1},{-287,1},{1329,1},
    {1299,1},{1314,1},{1312,1},{1057,2},{1042,2},{1026,2},{784,4},{529,8},
    {769,4},{768,4},{563,1},{560,1},{306,2},{291,1},{259,1},{-252,1},
    {-413,1},{-477,1},{-542,1},{1298,1},{-575,1},{1041,2},{784,4},{769,4},
    {256,16},{-383,1},{-399,1},{1107,1},{1092,1},{1106,1},{1061,1},{849,2},
    {789,2},{1104,1},{1091,1},{773,2},{1076,1},{1075,1},{341,1},{340,1},
    {325,1},{309,1},{834,1},{804,1},{577,2},{532,2},{516,2},{832,1},
    {818,1},{803,1},{816,1},{561,2},{531,2},{515,1},{546,1},{289,2},
    {288,1},{258,1},{-252,1},{-429,1},{-493,1},{-559,1},{1057,2},{1042,2},
    {529,8},{784,4},{769,4},{512,8},{-382,1},{1077,1},{-415,1},{1106,1},
    {1061,1},{1104,1},{849,2},{789,2},{1091,1},{1076,1},{1029,1},{1075,1},
    {834,2},{597,1},{581,1},{340,2},{339,1},{324,1},{804,1},{833,1},
    {532,2},{832,1},{772,1},{818,1},{803,1},{817,1},{787,1},{816,1},
    {771,1},{290,4},{288,1},{258,1},{-253,1},{-349,1},{-414,1},{-447,1},
    {-463,1},{1329,1},{1299,1},{-479,1},{1314,1},{1312,1},{1057,2},{1042,2},
    {1026,2},{785,4},{784,4},{769,4},{768,4},{-319,1},{851,1},{821,1},
    {-335,1},{836,1},{850,1},{805,1},{849,1},{341,1},{340,1},{325,1},
    {336,1},{533,2},{579,2},{564,2},{773,1},{832,1},{578,1},{548,1},
    {563,1},{516,1},{321,1},{276,1},{306,1},{291,1},{304,1},{259,1},
    {-251,1},{-572,1},{-733,1},{-830,1},{-863,1},{-879,1},{1041,2},{784,4},
    {769,4},{256,16},{-511,1},{-527,1},{-543,1},{1396,1},{1351,1},{1381,1},
    {1366,1},{1395,1},{1335,1},{1380,1},{-559,1},{1334,1},{1138,2},{1063,2},
    {1350,1},{1392,1},{1031,2},{1062,2},{1364,1},{1363,1},{1120,2},{1333,1},
    {1348,1},{881,4},{375,1},{374,1},{359,1},{373,1},{343,1},{358,1},
    {341,1},{325,1},{791,2},{1123,1},{1122,1},{-703,1},{1105,1},{1045,1},
    {-719,1},{865,2},{790,2},{774,2},{1104,1},{1029,1},{338,1},{293,1},
    {323,1},{308,1},{-799,1},{-815,1},{833,1},{788,1},{772,1},{818,1},
    {803,1},{816,1},{322,1},{292,1},{307,1},{320,1},{561,1},{531,1},
    {515,1},{546,1},{289,1},{274,1},{288,1},{258,1},{-251,1},{-525,1},
    {-605,1},{-685,1},{-765,1},{-831,1},{-846,1},{1298,1},{1057,2},{1312,1},
    {1282,1},{785,4},{784,4},{769,4},{512,8},{1399,1},{1398,1},{1383,1},
    {1367,1},{1382,1},{1396,1},{1351,1},{-511,1},{1381,1},{1366,1},{1139,2},
    {1079,2},{1124,2},{1364,1},{1349,1},{1363,1},{1333,1},{882,4},{807,4},
    {1094,2},{1136,2},{373,1},{341,1},{535,2},{881,1},{775,1},{867,1},
    {822,1},{774,1},{-591,1},{324,1},{338,1},{-671,1},{849,1},{550,2},
    {866,1},{864,1},{609,2},{293,1},{336,1},{534,2},{789,1},{835,1},
    {773,1},{-751,1},{834,1},{804,1},{308,1},{307,1},{833,1},{788,1},
    {832,1},{772,1},{562,2},{547,2},{305,1},{275,1},{560,1},{515,1},
    {290,2},{-252,1},{-397,1},{-477,1},{-557,1},{-622,1},{-653,1},{-719,1},
    {-735,1},{-750,1},{1329,1},{1299,1},{1314,1},{1057,2},{1042,2},{1312,1},
    {1282,1},{1024,2},{785,4},{784,4},{769,4},{-383,1},{1127,1},{1141,1},
    {1111,1},{1126,1},{1140,1},{1095,1},{1110,1},{869,2},{883,2},{1079,1},
    {1109,1},{882,2},{375,1},{374,1},{807,1},{868,1},{838,1},{881,1},
    {791,1},{-463,1},{867,1},{822,1},{368,1},{263,1},{852,1},{837,1},
    {836,1},{-543,1},{610,2},{550,2},{352,1},{336,1},{534,2},{865,1},
    {774,1},{851,1},{821,1},{850,1},{805,1},{593,1},{533,1},{579,1},
    {564,1},{773,1},{832,1},{578,2},{548,2},{577,2},{307,1},{276,1},
    {306,1},{291,1},{516,1},{560,1},{259,2},{-250,1},{-2107,1},{-2507,1},
    {-2764,1},{-2909,1},{-2974,1},{-3007,1},{-3023,1},{1041,2},{1040,2},{769,4},
    {256,16},{-767,1},{-1052,1},{-1213,1},{-1277,1},{-1358,1},{-1405,1},{-1469,1},
    {-1535,1},{-1550,1},{-1582,1},{-1614,1},{-1647,1},{-1662,1},{-1694,1},{-1726,1},
    {-1759,1},{-1774,1},{-1807,1},{-1822,1},{-1854,1},{-1886,1},{1565,1},{-1919,1},
    {-1935,1},{-1951,1},{-1967,1},{1731,1},{1730,1},{1580,1},{1717,1},{-1983,1},
    {1729,1},{1564,1},{-1999,1},{1548,1},{-2015,1},{-2031,1},{1715,1},{1595,1},
    {-2047,1},{1714,1},{-2063,1},{1610,1},{-2079,1},{1609,1},{-2095,1},{1323,2},
    {1457,2},{1307,2},{1712,1},{1547,1},{1641,1},{1700,1},{1699,1},{1594,1},
    {1685,1},{1625,1},{1442,2},{1322,2},{-780,1},{-973,1},{-910,1},{1279,1},
    {1278,1},{1277,1},{1262,1},{1276,1},{1261,1},{1275,1},{1215,1},{1260,1},
    {1229,1},{-959,1},{974,2},{989,2},{-943,1},{735,1},{478,2},{495,1},
    {463,1},{506,1},{414,1},{-1039,1},{1003,1},{958,1},{1017,1},{927,1},
    {942,1},{987,1},{957,1},{431,1},{476,1},{1272,1},{1167,1},{1228,1},
    {-1183,1},{1256,1},{-1199,1},{895,2},{941,2},{1242,1},{1227,1},{1212,1},
    {1135,1},{1014,2},{490,1},{489,1},{503,1},{487,1},{910,1},{1013,1},
    {985,1},{925,1},{863,1},{894,1},{970,1},{955,1},{1012,1},{847,1},
    {-1343,1},{831,1},{755,2},{984,1},{909,1},{428,1},{366,1},{754,1},
    {559,1},{-1391,1},{752,1},{486,1},{457,1},{924,1},{997,1},{698,2},
    {983,1},{893,1},{740,2},{908,1},{877,1},{739,2},{667,2},{953,1},
    {938,1},{497,1},{287,1},{271,2},{683,1},{606,1},{590,1},{712,1},
    {726,1},{574,1},{302,2},{738,1},{736,1},{481,1},{286,1},{526,1},
    {725,1},{605,1},{711,1},{636,1},{724,1},{696,1},{651,1},{589,1},
    {681,1},{666,1},{710,1},{364,1},{467,1},{573,1},{695,1},{466,2},
    {301,1},{465,1},{379,2},{709,1},{604,1},{665,1},{679,1},{316,2},
    {634,1},{633,1},{436,2},{464,1},{269,1},{424,1},{394,1},{452,1},
    {332,1},{438,1},{363,1},{347,1},{408,1},{393,1},{448,1},{331,1},
    {422,1},{362,1},{407,1},{392,1},{421,1},{346,1},{406,1},{391,1},
    {376,1},{375,1},{359,1},{1441,1},{1306,1},{-2367,1},{1290,1},{-2383,1},
    {1337,1},{-2399,1},{-2415,1},{1426,1},{1321,1},{-2431,1},{1411,1},{1336,1},
    {-2447,1},{-2463,1},{-2479,1},{1169,2},{1049,2},{1424,1},{1289,1},{1412,1},
    {1352,1},{1319,1},{-2495,1},{1154,2},{1064,2},{1153,2},{416,1},{390,1},
    {360,1},{404,1},{403,1},{389,1},{344,1},{374,1},{373,1},{343,1},
    {358,1},{372,1},{327,1},{357,1},{342,1},{311,1},{356,1},{326,1},
    {1395,1},{1394,1},{1137,2},{1047,2},{1365,1},{1392,1},{1287,1},{1379,1},
    {1334,1},{1364,1},{1349,1},{1378,1},{1318,1},{1363,1},{792,4},{1152,2},
    {1032,2},{1121,2},{1046,2},{1120,2},{1030,2},{-2895,1},{1106,1},{1061,1},
    {1104,1},{849,2},{789,2},{1091,1},{1076,1},{1029,1},{1090,1},{1060,1},
    {1075,1},{833,2},{309,1},{324,1},{532,2},{832,1},{772,1},{818,1},
    {803,1},{561,2},{531,1},{560,1},{515,1},{546,1},{289,1},{274,1},
    {288,1},{258,1},{-250,1},{-1179,1},{-1579,1},{-1836,1},{-1996,1},{-2124,1},
    {-2253,1},{-2333,1},{-2413,1},{-2477,1},{-2542,1},{-2574,1},{-2607,1},{-2622,1},
    {-2655,1},{1314,1},{1313,1},{1298,1},{1312,1},{1282,1},{785,4},{1040,2},
    {1025,2},{768,4},{-766,1},{-798,1},{-830,1},{-862,1},{-895,1},{-911,1},
    {-927,1},{-943,1},{-959,1},{-975,1},{-991,1},{-1007,1},{-1023,1},{-1039,1},
    {-1055,1},{-1070,1},{1724,1},{1647,1},{-1103,1},{-1119,1},{1631,1},{1767,1},
    {1662,1},{1738,1},{1708,1},{1723,1},{-1135,1},{1780,1},{1615,1},{1779,1},
    {1599,1},{1677,1},{1646,1},{1778,1},{1583,1},{-1151,1},{1777,1},{1567,1},
    {1737,1},{1692,1},{1765,1},{1722,1},{1707,1},{1630,1},{1751,1},{1661,1},
    {1764,1},{1614,1},{1736,1},{1676,1},{1763,1},{1750,1},{1645,1},{1598,1},
    {1721,1},{1691,1},{1762,1},{1706,1},{1582,1},{1761,1},{1566,1},{-1167,1},
    {1749,1},{1629,1},{767,1},{766,1},{751,1},{765,1},{494,2},{735,1},
    {764,1},{719,1},{749,1},{734,1},{763,1},{447,2},{748,1},{718,1},
    {477,1},{506,1},{431,1},{491,1},{446,1},{476,1},{461,1},{505,1},
    {415,1},{430,1},{475,1},{445,1},{504,1},{399,1},{460,1},{489,1},
    {414,1},{503,1},{383,1},{474,1},{429,1},{459,1},{502,2},{746,1},
    {752,1},{488,1},{398,1},{501,1},{473,1},{413,1},{472,1},{486,1},
    {271,1},{480,1},{270,1},{-1439,1},{-1455,1},{1357,1},{-1471,1},{-1487,1},
    {-1503,1},{1341,1},{1325,1},{-1519,1},{1489,1},{1463,1},{1403,1},{1309,1},
    {-1535,1},{1372,1},{1448,1},{1418,1},{1476,1},{1356,1},{1462,1},{1387,1},
    {-1551,1},{1475,1},{1340,1},{1447,1},{1402,1},{1386,1},{-1567,1},{1068,2},
    {1474,1},{1461,1},{455,1},{380,1},{468,1},{440,1},{395,1},{425,1},
    {410,1},{454,1},{364,1},{467,1},{466,1},{464,1},{453,1},{269,1},
    {409,1},{448,1},{268,1},{432,1},{1371,1},{1473,1},{1432,1},{1417,1},
    {1308,1},{1460,1},{1355,1},{1446,1},{1459,1},{1431,1},{1083,2},{1401,1},
    {1416,1},{1458,1},{1445,1},{1067,2},{1370,1},{1457,1},{1051,2},{1291,1},
    {1430,1},{1385,1},{1444,1},{1354,1},{1415,1},{1400,1},{1443,1},{1082,2},
    {1173,1},{1113,1},{1186,1},{1066,1},{1185,1},{1050,1},{-1967,1},{1158,1},
    {1128,1},{1172,1},{1097,1},{1171,1},{1081,1},{-1983,1},{1157,1},{1112,1},
    {416,1},{266,1},{375,1},{400,1},{1170,1},{1142,1},{1127,1},{1065,1},
    {793,2},{1169,1},{1033,1},{1156,1},{1096,1},{1141,1},{1111,1},{1155,1},
    {1080,1},{1126,1},{1140,1},{898,2},{808,2},{897,2},{792,2},{1095,1},
    {1152,1},{1032,1},{1125,1},{1110,1},{1139,1},{1079,1},{1124,1},{882,1},
    {807,1},{838,1},{881,1},{853,1},{791,1},{-2319,1},{867,1},{368,1},
    {263,1},{822,1},{852,1},{837,1},{866,1},{806,1},{865,1},{-2399,1},
    {851,1},{352,1},{262,1},{534,2},{821,1},{836,1},{594,2},{549,2},
    {593,2},{533,2},{848,1},{773,1},{579,2},{564,1},{578,1},{548,1},
    {563,1},{276,2},{577,1},{576,1},{306,1},{291,1},{516,1},{560,1},
    {305,2},{275,1},{259,1},{-251,1},{-892,1},{-2058,1},{-2620,1},{-2828,1},
    {-2957,1},{-3023,1},{-3039,1},{1041,2},{1040,2},{769,4},{256,16},{-511,1},
    {-527,1},{-543,1},{-559,1},{1530,1},{-575,1},{-591,1},{1528,1},{1527,1},
    {1407,1},{1526,1},{1391,1},{1023,4},{1525,1},{1375,1},{1268,2},{1103,2},
    {1087,2},{1039,2},{1523,1},{-604,1},{815,4},{510,1},{495,1},{509,1},
    {479,1},{508,1},{463,1},{507,1},{447,1},{431,1},{505,1},{415,1},
    {399,1},{-734,1},{-782,1},{1262,1},{-815,1},{1259,1},{1244,1},{-831,1},
    {1258,1},{1228,1},{-847,1},{-863,1},{1196,1},{-879,1},{1253,1},{987,2},
    {748,1},{-767,1},{493,2},{462,1},{477,1},{414,2},{686,1},{669,1},
    {478,1},{446,1},{461,1},{445,1},{474,1},{429,1},{487,1},{458,1},
    {412,1},{471,1},{1266,1},{1264,1},{1009,2},{799,2},{-1019,1},{-1276,1},
    {-1452,1},{-1581,1},{-1677,1},{-1757,1},{-1821,1},{-1886,1},{-1933,1},{-1997,1},
    {1257,2},{1483,1},{1468,1},{1512,1},{1422,1},{1497,1},{1406,1},{1467,1},
    {1496,1},{1421,1},{1510,1},{1134,2},{1225,2},{1466,1},{1451,1},{1374,1},
    {1405,1},{1252,2},{1358,1},{1480,1},{1164,2},{1251,2},{1238,2},{1389,1},
    {1465,1},{-1407,1},{1054,1},{1101,1},{-1423,1},{1207,1},{-1439,1},{830,2},
    {1248,1},{1038,1},{1237,1},{1117,1},{1223,1},{1148,1},{1236,1},{1208,1},
    {411,1},{426,1},{395,1},{410,1},{379,1},{269,1},{1193,1},{1222,1},
    {1132,1},{1235,1},{1221,1},{1116,1},{976,2},{1192,1},{1162,1},{1177,1},
    {1220,1},{1131,1},{1191,1},{963,2},{-1647,1},{961,1},{780,1},{-1663,1},
    {558,2},{994,1},{993,1},{437,1},{408,1},{393,1},{407,1},{829,1},
    {978,1},{813,1},{797,1},{947,1},{-1743,1},{721,2},{377,1},{392,1},
    {844,1},{950,1},{828,1},{890,1},{706,2},{812,1},{859,1},{796,1},
    {960,1},{948,1},{843,1},{934,1},{874,1},{571,2},{-1919,1},{690,1},
    {555,1},{689,1},{421,1},{346,1},{539,2},{944,1},{779,1},{918,1},
    {873,1},{932,1},{842,1},{903,1},{888,1},{570,2},{931,1},{917,1},
    {674,2},{-2575,1},{1562,1},{-2591,1},{1609,1},{-2607,1},{1654,1},{1322,2},
    {1441,2},{1696,1},{1546,1},{1683,1},{1593,1},{1669,1},{1624,1},{1426,2},
    {1321,2},{1639,1},{1680,1},{1425,2},{1305,2},{1545,1},{1668,1},{1608,1},
    {1623,1},{1667,1},{1592,1},{1638,1},{1666,1},{1320,2},{1652,1},{1607,1},
    {1409,2},{1304,2},{1288,2},{1664,1},{1637,1},{1395,2},{1335,2},{1622,1},
    {1636,1},{1394,2},{1319,2},{1606,1},{1621,1},{1392,2},{1137,4},{345,1},
    {390,1},{360,1},{375,1},{404,1},{373,1},{1047,1},{-2751,1},{-2767,1},
    {-2783,1},{1062,1},{1121,1},{1046,1},{-2799,1},{1077,1},{-2815,1},{1106,1},
    {1061,1},{789,2},{1105,1},{1104,1},{263,1},{355,1},{310,1},{340,1},
    {325,1},{354,1},{352,1},{262,1},{339,1},{324,1},{1091,1},{1076,1},
    {1029,1},{1090,1},{1060,1},{1075,1},{833,2},{788,2},{1088,1},{1028,1},
    {818,2},{803,2},{561,2},{531,2},{816,1},{771,1},{546,2},{289,1},
    {274,1},{288,1},{258,1},{-253,1},{-317,1},{-381,1},{-446,1},{-478,1},
    {-509,1},{1279,2},{-811,1},{-1179,1},{-1451,1},{-1756,1},{-1900,1},{-2028,1},
    {-2189,1},{-2253,1},{-2333,1},{-2414,1},{-2445,1},{-2511,1},{-2526,1},{1313,1},
    {1298,1},{-2559,1},{1041,2},{1040,2},{1025,2},{1024,2},{1022,1},{1007,1},
    {1021,1},{991,1},{1020,1},{975,1},{1019,1},{959,1},{687,2},{1018,1},
    {1017,1},{671,2},{655,2},{1016,1},{1015,1},{639,2},{758,2},{623,2},
    {757,1},{607,1},{756,1},{591,1},{755,1},{575,1},{754,1},{559,1},
    {543,2},{1009,1},{783,1},{-575,1},{-621,1},{-685,1},{-749,1},{496,1},
    {-590,1},{750,1},{749,1},{734,1},{748,1},{974,1},{989,1},{1003,1},
    {958,1},{988,1},{973,1},{1002,1},{942,1},{987,1},{957,1},{972,1},
    {1001,1},{926,1},{986,1},{941,1},{971,1},{956,1},{1000,1},{910,1},
    {985,1},{925,1},{999,1},{894,1},{970,1},{-1071,1},{-1087,1},{-1102,1},
    {1390,1},{-1135,1},{1436,1},{1509,1},{1451,1},{1374,1},{-1151,1},{1405,1},
    {1358,1},{1480,1},{1420,1},{-1167,1},{1507,1},{1494,1},{1389,1},{1342,1},
    {1465,1},{1435,1},{1450,1},{1326,1},{1505,1},{1310,1},{1493,1},{1373,1},
    {1479,1},{1404,1},{1492,1},{1464,1},{1419,1},{428,1},{443,1},{472,1},
    {397,1},{736,1},{526,1},{464,2},{486,1},{457,1},{442,1},{471,1},
    {484,1},{482,1},{1357,1},{1449,1},{1434,1},{1478,1},{1388,1},{1491,1},
    {1341,1},{1490,1},{1325,1},{1489,1},{1463,1},{1403,1},{1309,1},{1477,1},
    {1372,1},{1448,1},{1418,1},{1433,1},{1476,1},{1356,1},{1462,1},{1387,1},
    {-1439,1},{1475,1},{1340,1},{1447,1},{1402,1},{1474,1},{1324,1},{1461,1},
    {1371,1},{1473,1},{269,1},{448,1},{1432,1},{1417,1},{1308,1},{1460,1},
    {-1711,1},{1459,1},{-1727,1},{1441,1},{1099,2},{1446,1},{1386,1},{1431,1},
    {1401,1},{-1743,1},{1289,1},{1083,2},{1160,2},{1458,1},{1445,1},{1067,2},
    {1370,1},{1457,1},{1307,1},{1430,1},{1129,2},{1098,2},{268,1},{432,1},
    {267,1},{416,1},{266,1},{400,1},{-1887,1},{1144,1},{1187,1},{1082,1},
    {1173,1},{1113,1},{1186,1},{1066,1},{1050,1},{1158,1},{1128,1},{1143,1},
    {1172,1},{1097,1},{1171,1},{1081,1},{420,1},{391,1},{1157,1},{1112,1},
    {1170,1},{1142,1},{1127,1},{1065,1},{1169,1},{1049,1},{1156,1},{1096,1},
    {1141,1},{1111,1},{1155,1},{1080,1},{1126,1},{1154,1},{1064,1},{1153,1},
    {1140,1},{1095,1},{1048,1},{-2159,1},{1125,1},{1110,1},{1137,1},{-2175,1},
    {823,2},{1139,1},{1138,1},{807,2},{384,1},{264,1},{368,1},{263,1},
    {868,1},{838,1},{853,1},{791,1},{867,1},{822,1},{852,1},{837,1},
    {866,1},{806,1},{865,1},{790,1},{-2319,1},{851,1},{821,1},{836,1},
    {352,1},{262,1},{850,1},{805,1},{849,1},{-2399,1},{533,2},{835,1},
    {820,1},{336,1},{261,1},{578,1},{548,1},{563,1},{577,1},{532,2},
    {832,1},{772,1},{562,2},{547,2},{305,1},{275,1},{560,1},{515,1},
    {290,2},{288,1},{258,1},
};
static const size_t mina_mp3_tab_run_count = sizeof(mina_mp3_tab_runs) / sizeof(mina_mp3_tab_runs[0]);
static const mina_i16 mina_mp3_tabindex[] = {
    0,32,64,98,0,132,180,218,292,364,426,538,648,746,0,1126,
    1460,1460,1460,1460,1460,1460,1460,1460,1842,1842,1842,1842,1842,1842,1842,1842,
};
static const mina_u8 mina_mp3_linbits[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,2,3,4,6,8,10,13,4,5,6,7,8,9,11,13,
};
static const mina_mp3_byte_run mina_mp3_count1_a_runs[] = {
    {130,1},{162,1},{193,1},{209,1},{44,1},{28,1},{76,1},{140,1},
    {9,8},{190,1},{254,1},{222,1},{238,1},{126,1},{94,1},{157,2},
    {109,1},{61,1},{173,1},{205,1},
};
static const mina_mp3_byte_run mina_mp3_count1_b_runs[] = {
    {252,1},{236,1},{220,1},{204,1},{188,1},{172,1},{156,1},{140,1},
    {124,1},{108,1},{92,1},{76,1},{60,1},{44,1},{28,1},{12,1},
};
static const mina_u8 mina_mp3_scf_long[] = {
    6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54,0,
    12,12,12,12,12,12,16,20,24,28,32,40,48,56,64,76,90,2,2,2,2,2,0,
    6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54,0,
    6,6,6,6,6,6,8,10,12,14,16,18,22,26,32,38,46,54,62,70,76,36,0,
    6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54,0,
    4,4,4,4,4,4,6,6,8,8,10,12,16,20,24,28,34,42,50,54,76,158,0,
    4,4,4,4,4,4,6,6,6,8,10,12,16,18,22,28,34,40,46,54,54,192,0,
    4,4,4,4,4,4,6,6,8,10,12,16,20,24,30,38,46,56,68,84,102,26,0,
};
static const mina_u8 mina_mp3_scf_short[] = {
    4,4,4,4,4,4,4,4,4,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,24,24,24,30,30,30,40,40,40,18,18,18,0,
    8,8,8,8,8,8,8,8,8,12,12,12,16,16,16,20,20,20,24,24,24,28,28,28,36,36,36,2,2,2,2,2,2,2,2,2,26,26,26,0,
    4,4,4,4,4,4,4,4,4,6,6,6,6,6,6,8,8,8,10,10,10,14,14,14,18,18,18,26,26,26,32,32,32,42,42,42,18,18,18,0,
    4,4,4,4,4,4,4,4,4,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,24,24,24,32,32,32,44,44,44,12,12,12,0,
    4,4,4,4,4,4,4,4,4,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,24,24,24,30,30,30,40,40,40,18,18,18,0,
    4,4,4,4,4,4,4,4,4,4,4,4,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,22,22,22,30,30,30,56,56,56,0,
    4,4,4,4,4,4,4,4,4,4,4,4,6,6,6,6,6,6,10,10,10,12,12,12,14,14,14,16,16,16,20,20,20,26,26,26,66,66,66,0,
    4,4,4,4,4,4,4,4,4,4,4,4,6,6,6,8,8,8,12,12,12,16,16,16,20,20,20,26,26,26,34,34,34,42,42,42,12,12,12,0,
};
static const mina_u8 mina_mp3_scf_mixed[] = {
    6,6,6,6,6,6,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,24,24,24,30,30,30,40,40,40,18,18,18,0,0,0,0,
    12,12,12,4,4,4,8,8,8,12,12,12,16,16,16,20,20,20,24,24,24,28,28,28,36,36,36,2,2,2,2,2,2,2,2,2,26,26,26,0,
    6,6,6,6,6,6,6,6,6,6,6,6,8,8,8,10,10,10,14,14,14,18,18,18,26,26,26,32,32,32,42,42,42,18,18,18,0,0,0,0,
    6,6,6,6,6,6,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,24,24,24,32,32,32,44,44,44,12,12,12,0,0,0,0,
    6,6,6,6,6,6,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,24,24,24,30,30,30,40,40,40,18,18,18,0,0,0,0,
    4,4,4,4,4,4,6,6,4,4,4,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,22,22,22,30,30,30,56,56,56,0,0,
    4,4,4,4,4,4,6,6,4,4,4,6,6,6,6,6,6,10,10,10,12,12,12,14,14,14,16,16,16,20,20,20,26,26,26,66,66,66,0,0,
    4,4,4,4,4,4,6,6,4,4,4,6,6,6,8,8,8,12,12,12,16,16,16,20,20,20,26,26,26,34,34,34,42,42,42,12,12,12,0,0,
};
static const float mina_mp3_mdct_win[] = {
    0.99904822f,0.99144486f,0.97629601f,0.95371695f,0.92387953f,0.88701083f,0.84339145f,0.79335334f,0.73727734f,0.04361938f,0.13052619f,0.21643961f,0.3007058f,0.38268343f,0.46174861f,0.53729961f,0.60876143f,0.67559021f,
    1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,0.99144486f,0.92387953f,0.79335334f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.13052619f,0.38268343f,0.60876143f,
};
static const float mina_mp3_pow43_tab[] = {
    0.0f,-1.0f,-2.519842f,-4.326749f,-6.349604f,-8.54988f,-10.902724f,-13.390518f,-16.0f,-18.720754f,-21.544347f,-24.463781f,-27.473142f,-30.567351f,-33.741992f,-36.993181f,
    0.0f,1.0f,2.519842f,4.326749f,6.349604f,8.54988f,10.902724f,13.390518f,16.0f,18.720754f,21.544347f,24.463781f,27.473142f,30.567351f,33.741992f,36.993181f,
    40.317474f,43.711787f,47.173345f,50.699631f,54.288352f,57.937408f,61.644865f,65.408941f,69.227979f,73.100443f,77.024898f,81.0f,85.024491f,89.097188f,93.216975f,97.3828f,
    101.593667f,105.848633f,110.146801f,114.487321f,118.869381f,123.292209f,127.755065f,132.257246f,136.798076f,141.376907f,145.993119f,150.646117f,155.335327f,160.060199f,164.820202f,169.614826f,
    174.443577f,179.30598f,184.201575f,189.129918f,194.09058f,199.083145f,204.10721f,209.162385f,214.248292f,219.364564f,224.510845f,229.686789f,234.892058f,240.126328f,245.38928f,250.680604f,
    256.0f,261.347174f,266.721841f,272.123723f,277.552547f,283.008049f,288.489971f,293.99806f,299.532071f,305.091761f,310.676898f,316.287249f,321.922592f,327.582707f,333.267377f,338.976394f,
    344.70955f,350.466646f,356.247482f,362.051866f,367.879608f,373.730522f,379.604427f,385.501143f,391.420496f,397.362314f,403.326427f,409.312672f,415.320884f,421.350905f,427.402579f,433.47575f,
    439.570269f,445.685987f,451.822757f,457.980436f,464.158883f,470.35796f,476.57753f,482.817459f,489.077615f,495.357868f,501.65809f,507.978156f,514.317941f,520.677324f,527.056184f,533.454404f,
    539.871867f,546.308458f,552.764065f,559.238575f,565.731879f,572.24387f,578.77444f,585.323483f,591.890898f,598.476581f,605.080431f,611.702349f,618.342238f,625.0f,631.67554f,638.368763f,
    645.079578f,
};
static const mina_u8 mina_mp3_scfc_decode[] = {
    0,1,2,3,12,5,6,7,9,10,11,13,14,15,18,19,
};
static const mina_u8 mina_mp3_g_mod[] = {
    5,5,4,4,5,5,4,1,4,3,1,1,5,6,6,1,4,4,4,1,4,3,1,1,
};
static const mina_u8 mina_mp3_preamp[] = {
    1,1,1,1,2,2,3,3,3,2,
};
static const float mina_mp3_aa[] = {
    0.85749293f,0.881742f,0.94962865f,0.98331459f,0.99551782f,0.99916056f,0.9998992f,0.99999316f,
    0.51449576f,0.47173197f,0.31337745f,0.1819132f,0.09457419f,0.04096558f,0.01419856f,0.00369997f,
};
static const float mina_mp3_win[] = {
    -1.0f,26.0f,-31.0f,208.0f,218.0f,401.0f,-519.0f,2063.0f,2000.0f,4788.0f,-5517.0f,7134.0f,5959.0f,35640.0f,-39336.0f,74992.0f,
    -1.0f,24.0f,-35.0f,202.0f,222.0f,347.0f,-581.0f,2080.0f,1952.0f,4425.0f,-5879.0f,7640.0f,5288.0f,33791.0f,-41176.0f,74856.0f,
    -1.0f,21.0f,-38.0f,196.0f,225.0f,294.0f,-645.0f,2087.0f,1893.0f,4063.0f,-6237.0f,8092.0f,4561.0f,31947.0f,-43006.0f,74630.0f,
    -1.0f,19.0f,-41.0f,190.0f,227.0f,244.0f,-711.0f,2085.0f,1822.0f,3705.0f,-6589.0f,8492.0f,3776.0f,30112.0f,-44821.0f,74313.0f,
    -1.0f,17.0f,-45.0f,183.0f,228.0f,197.0f,-779.0f,2075.0f,1739.0f,3351.0f,-6935.0f,8840.0f,2935.0f,28289.0f,-46617.0f,73908.0f,
    -1.0f,16.0f,-49.0f,176.0f,228.0f,153.0f,-848.0f,2057.0f,1644.0f,3004.0f,-7271.0f,9139.0f,2037.0f,26482.0f,-48390.0f,73415.0f,
    -2.0f,14.0f,-53.0f,169.0f,227.0f,111.0f,-919.0f,2032.0f,1535.0f,2663.0f,-7597.0f,9389.0f,1082.0f,24694.0f,-50137.0f,72835.0f,
    -2.0f,13.0f,-58.0f,161.0f,224.0f,72.0f,-991.0f,2001.0f,1414.0f,2330.0f,-7910.0f,9592.0f,70.0f,22929.0f,-51853.0f,72169.0f,
    -2.0f,11.0f,-63.0f,154.0f,221.0f,36.0f,-1064.0f,1962.0f,1280.0f,2006.0f,-8209.0f,9750.0f,-998.0f,21189.0f,-53534.0f,71420.0f,
    -2.0f,10.0f,-68.0f,147.0f,215.0f,2.0f,-1137.0f,1919.0f,1131.0f,1692.0f,-8491.0f,9863.0f,-2122.0f,19478.0f,-55178.0f,70590.0f,
    -3.0f,9.0f,-73.0f,139.0f,208.0f,-29.0f,-1210.0f,1870.0f,970.0f,1388.0f,-8755.0f,9935.0f,-3300.0f,17799.0f,-56778.0f,69679.0f,
    -3.0f,8.0f,-79.0f,132.0f,200.0f,-57.0f,-1283.0f,1817.0f,794.0f,1095.0f,-8998.0f,9966.0f,-4533.0f,16155.0f,-58333.0f,68692.0f,
    -4.0f,7.0f,-85.0f,125.0f,189.0f,-83.0f,-1356.0f,1759.0f,605.0f,814.0f,-9219.0f,9959.0f,-5818.0f,14548.0f,-59838.0f,67629.0f,
    -4.0f,7.0f,-91.0f,117.0f,177.0f,-106.0f,-1428.0f,1698.0f,402.0f,545.0f,-9416.0f,9916.0f,-7154.0f,12980.0f,-61289.0f,66494.0f,
    -5.0f,6.0f,-97.0f,111.0f,163.0f,-127.0f,-1498.0f,1634.0f,185.0f,288.0f,-9585.0f,9838.0f,-8540.0f,11455.0f,-62684.0f,65290.0f,
};
static const float mina_mp3_sec[] = {
    10.19000816f,0.50060302f,0.50241929f,3.40760851f,0.50547093f,0.52249861f,2.05778098f,0.51544732f,
    0.56694406f,1.4841646f,0.53104258f,0.6468218f,1.16943991f,0.55310392f,0.7881546f,0.97256821f,
    0.58293498f,1.06067765f,0.83934963f,0.62250412f,1.72244716f,0.74453628f,0.67480832f,5.10114861f,
};
static const float mina_mp3_twid9[] = {
    0.73727734f,0.79335334f,0.84339145f,0.88701083f,0.92387953f,0.95371695f,0.97629601f,0.99144486f,0.99904822f,0.67559021f,0.60876143f,0.53729961f,0.46174861f,0.38268343f,0.3007058f,0.21643961f,0.13052619f,0.04361938f,
};
static const float mina_mp3_twid3[] = {
    0.79335334f,0.92387953f,0.99144486f,0.60876143f,0.38268343f,0.13052619f,
};
static const float mina_mp3_pan[] = {
    0.0f,1.0f,0.21132487f,0.78867513f,0.3660254f,0.6339746f,0.5f,0.5f,0.6339746f,0.3660254f,0.78867513f,0.21132487f,1.0f,0.0f,
};
static const float mina_mp3_expfrac[] = {
    9.31322575e-10f,7.83145814e-10f,6.58544508e-10f,5.53767716e-10f,
};static const mina_u8 mina_mp3_scf_part[] = {
    6,5,5,5,6,5,5,5,6,5,7,3,11,10,0,0,7,7,7,0,6,6,6,3,8,8,5,0,
    8,9,6,12,6,9,9,9,6,9,12,6,15,18,0,0,6,15,12,0,6,12,9,6,6,18,9,0,
    9,9,6,12,9,9,9,9,9,9,12,6,18,18,0,0,12,12,12,0,12,9,9,6,15,12,9,0,
};

#endif /* MINA_NO_MP3 */

/* ------------------------------------------------------------------ */
/* MP3: header parsing, Xing/Info/VBRI/LAME tags                        */
/* ------------------------------------------------------------------ */
static const mina_u16 g_mina_mp3_bitrate[2][3][16] = {
    /* MPEG-1  */ { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0},
                    {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384,0},
                    {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0} },
    /* MPEG-2/2.5 */ { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,0},
                       {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0},
                       {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0} }
};
static const mina_u16 g_mina_mp3_srate[3][4] = {
    {44100, 48000, 32000, 0},   /* MPEG-1   */
    {22050, 24000, 16000, 0},   /* MPEG-2   */
    {11025, 12000,  8000, 0}    /* MPEG-2.5 */
};

typedef struct {
    mina_u8  version;        /* 0 = 2.5, 1 = reserved, 2 = 2, 3 = 1 */
    mina_u8  layer;          /* 1..3 */
    mina_u8  bitrate_idx, srate_idx, pad, channel_mode, mode_ext, crc;
    mina_u16 bitrate_kbps;
    mina_u32 sample_rate;
    mina_u32 frame_len;
    mina_u32 samples_per_frame;
    mina_u8  channels;
} mina_mp3_header;

static int mina_mp3_parse_header(const mina_u8 *d, mina_mp3_header *h) {
    mina_u32 w = ((mina_u32)d[0] << 24) | ((mina_u32)d[1] << 16) |
                 ((mina_u32)d[2] << 8)  | (mina_u32)d[3];
    int vidx;
    if ((w & 0xFFE00000UL) != 0xFFE00000UL) return -1;
    h->version = (mina_u8)((w >> 19) & 3u);
    h->layer   = (mina_u8)(4u - ((w >> 17) & 3u));
    if (h->version == 1 || h->layer < 1 || h->layer > 3) return -1;
    h->bitrate_idx  = (mina_u8)((w >> 12) & 15u);
    h->srate_idx    = (mina_u8)((w >> 10) & 3u);
    h->pad          = (mina_u8)((w >> 9) & 1u);
    h->channel_mode = (mina_u8)((w >> 6) & 3u);
    h->mode_ext     = (mina_u8)((w >> 4) & 3u);
    h->crc          = (mina_u8)(((w >> 16) & 1u) ? 0u : 1u);   /* 0 = protected */
    /* free format (0) and the reserved index (15) are not supported */
    if (h->bitrate_idx == 0 || h->bitrate_idx == 15 || h->srate_idx == 3) return -1;

    vidx = (h->version == 3) ? 0 : 1;
    h->bitrate_kbps = g_mina_mp3_bitrate[vidx][h->layer - 1][h->bitrate_idx];
    h->sample_rate  = g_mina_mp3_srate[(h->version == 3) ? 0 :
                                       (h->version == 2) ? 1 : 2][h->srate_idx];
    if (!h->bitrate_kbps || !h->sample_rate) return -1;
    h->channels = (mina_u8)((h->channel_mode == 3) ? 1 : 2);

    if (h->layer == 1)      h->samples_per_frame = 384;
    else if (h->layer == 2) h->samples_per_frame = 1152;
    else                    h->samples_per_frame = (h->version == 3) ? 1152 : 576;

    if (h->layer == 1)
        h->frame_len = (((12u * h->bitrate_kbps * 1000u) / h->sample_rate) + h->pad) * 4u;
    else if (h->layer == 2)
        h->frame_len = ((144u * h->bitrate_kbps * 1000u) / h->sample_rate) + h->pad;
    else
        h->frame_len = (((h->version == 3 ? 144u : 72u) * h->bitrate_kbps * 1000u)
                        / h->sample_rate) + h->pad;
    if (h->frame_len < 24 || h->frame_len > 2880) return -1;
    return 0;
}

/* Two headers belong to the same stream. Deliberately does NOT compare the
 * channel mode: encoders write the Xing/Info header frame as plain stereo
 * even when the audio frames are joint stereo, and rejecting that would make
 * us miss the tag frame entirely. */
static int mina_mp3_same_format(const mina_mp3_header *a, const mina_mp3_header *b) {
    return a->version == b->version && a->layer == b->layer &&
           a->srate_idx == b->srate_idx && a->channels == b->channels;
}

static int mina_mp3_find_sync(const mina_u8 *d, size_t n, size_t *pos,
                              mina_mp3_header *out) {
    size_t i;
    for (i = 0; i + 4 <= n; i++) {
        mina_mp3_header h;
        if (d[i] != 0xFFu || (d[i + 1] & 0xE0u) != 0xE0u) continue;
        if (mina_mp3_parse_header(d + i, &h) != 0) continue;
        /* require a second consistent header where one should be */
        if (i + h.frame_len + 4 <= n) {
            mina_mp3_header h2;
            if (mina_mp3_parse_header(d + i + h.frame_len, &h2) != 0) continue;
            if (!mina_mp3_same_format(&h, &h2)) continue;
        }
        *pos = i;
        if (out) *out = h;
        return 0;
    }
    return -1;
}

static size_t mina_mp3_id3_skip(const mina_u8 *d, size_t n) {
    size_t off = 0;
    while (n - off >= 10 && memcmp(d + off, "ID3", 3) == 0) {
        mina_u32 sz = ((mina_u32)(d[off+6] & 0x7Fu) << 21) |
                      ((mina_u32)(d[off+7] & 0x7Fu) << 14) |
                      ((mina_u32)(d[off+8] & 0x7Fu) << 7)  |
                       (mina_u32)(d[off+9] & 0x7Fu);
        size_t step = 10 + (size_t)sz;
        if (d[off + 5] & 0x10u) step += 10;      /* footer present */
        if (step > n - off) return n;
        off += step;
    }
    return off;
}

static mina_bool mina_mp3_probe(const mina_u8 *d, size_t n) {
    size_t start, pos = 0;
    mina_mp3_header h;
    if (n < 4) return 0;
    start = mina_mp3_id3_skip(d, n);
    if (start >= n) return 0;
    /* Only look near the beginning: a sync deep inside some other format is
     * not evidence that this is an MP3. */
    {
        size_t win = n - start;
        if (win > 65536u && start == 0) win = 65536u;
        if (mina_mp3_find_sync(d + start, win, &pos, &h) != 0) return 0;
    }
    return (mina_bool)(start > 0 || pos < 4096);
}

/* ---- Xing / Info / VBRI ---- */
typedef struct {
    int      present;        /* a header frame is present and must be skipped */
    mina_u32 frames;         /* audio frames excluding the header frame       */
    mina_u32 enc_delay;      /* LAME encoder delay in samples                 */
    mina_u32 enc_padding;    /* LAME encoder padding in samples               */
    int      have_lame;
} mina_mp3_vbrtag;

static void mina_mp3_read_vbrtag(const mina_u8 *d, size_t n, size_t pos,
                                 const mina_mp3_header *h, mina_mp3_vbrtag *t) {
    static const mina_u32 xoff[2][2] = { {32, 17}, {17, 9} };
    size_t p, q, frame_end;
    mina_u32 flags;
    int lsf, mono;

    memset(t, 0, sizeof(*t));
    if (h->layer != 3) return;
    frame_end = pos + h->frame_len;
    if (frame_end > n) frame_end = n;

    lsf  = (h->version == 3) ? 0 : 1;
    mono = (h->channels == 1) ? 1 : 0;

    /* VBRI sits at a fixed 32 bytes past the header. */
    p = pos + 4 + 32;
    if (p + 26 <= frame_end && memcmp(d + p, "VBRI", 4) == 0) {
        if (mina_be16(d + p + 4) == 1) {
            t->present = 1;
            t->frames = mina_be32(d + p + 14);
        }
        return;
    }

    p = pos + 4 + (size_t)xoff[lsf][mono];
    if (p + 8 > frame_end) return;
    if (memcmp(d + p, "Xing", 4) != 0 && memcmp(d + p, "Info", 4) != 0) return;

    t->present = 1;
    flags = mina_be32(d + p + 4);
    q = p + 8;
    if (flags & 1u) { if (q + 4 > frame_end) return; t->frames = mina_be32(d + q); q += 4; }
    if (flags & 2u) { if (q + 4 > frame_end) return; q += 4; }
    if (flags & 4u) { if (q + 100 > frame_end) return; q += 100; }
    if (flags & 8u) { if (q + 4 > frame_end) return; q += 4; }

    /* LAME/Lavc/Lavf extension: 4-byte magic, then the 3-byte delay and
     * padding field 21 bytes further in. Try the position implied by the
     * flags first, then the fixed 0x78 that LAME itself always uses. */
    {
        size_t cand[2];
        int k;
        cand[0] = q;
        cand[1] = p + 0x78;
        for (k = 0; k < 2; k++) {
            size_t x = cand[k];
            if (x + 24 > frame_end) continue;
            if (memcmp(d + x, "LAME", 4) != 0 && memcmp(d + x, "Lavc", 4) != 0 &&
                memcmp(d + x, "Lavf", 4) != 0) continue;
            {
                mina_u32 v = mina_be24(d + x + 21);
                t->enc_delay   = v >> 12;
                t->enc_padding = v & 0x0FFFu;
                t->have_lame = 1;
            }
            return;
        }
    }
}

static mina_result mina_mp3_info(const mina_u8 *d, size_t n, mina_fileinfo *out) {
    size_t pos = 0, start;
    mina_mp3_header h, hf;
    mina_mp3_vbrtag tag;
    mina_u64 total;
    mina_u32 frames = 0;
    double bitsum = 0.0;

    memset(out, 0, sizeof(*out));
    mina_strcpy_n(out->codec, sizeof(out->codec), "mp3");
    out->bits_per_sample = 16;
    total = mina_u64_from(0);

    start = mina_mp3_id3_skip(d, n);
    if (start >= n) return MINA_ERR_NOTFOUND;
    if (mina_mp3_find_sync(d + start, n - start, &pos, &hf) != 0) return MINA_ERR_NOTFOUND;
    pos += start;

    out->sample_rate = hf.sample_rate;
    out->channels = hf.channels;
    if (hf.layer == 1) mina_strcpy_n(out->codec, sizeof(out->codec), "mp1");
    else if (hf.layer == 2) mina_strcpy_n(out->codec, sizeof(out->codec), "mp2");

    mina_mp3_read_vbrtag(d, n, pos, &hf, &tag);
    if (tag.present && tag.frames) {
        total = mina_u64_mul32(tag.frames, hf.samples_per_frame);
        if (tag.have_lame) {
            mina_u32 trim = tag.enc_delay + tag.enc_padding;
            mina_u64 t64 = mina_u64_from(trim);
            if (mina_u64_gt(total, t64)) total = mina_u64_sub(total, t64);
        }
        /* average bitrate from the byte count over the duration */
        {
            double secs = hf.sample_rate
                ? (double)tag.frames * (double)hf.samples_per_frame / (double)hf.sample_rate
                : 0.0;
            if (secs > 0.0)
                out->bitrate_kbps = (mina_u32)((double)(n - pos) * 8.0 / secs / 1000.0);
        }
    } else {
        size_t p = pos;
        while (p + 4 <= n) {
            if (mina_mp3_parse_header(d + p, &h) != 0) {
                size_t skip;
                if (mina_mp3_find_sync(d + p, n - p, &skip, &h) != 0) break;
                p += skip;
                continue;
            }
            if (!mina_mp3_same_format(&h, &hf)) break;
            total = mina_u64_add(total, mina_u64_from(h.samples_per_frame));
            bitsum += (double)h.bitrate_kbps;
            frames++;
            if (h.frame_len > n - p) break;
            p += h.frame_len;
        }
        if (frames) out->bitrate_kbps = (mina_u32)(bitsum / (double)frames + 0.5);
    }

    out->total_frames = total;
    out->duration_seconds = hf.sample_rate
        ? mina_u64_dbl(total) / (double)hf.sample_rate : 0.0;
    return MINA_OK;
}

#ifndef MINA_NO_MP3
/* ------------------------------------------------------------------ */
/* MP3 Layer III decoder                                                */
/*                                                                      */
/* Mina's bounded frame parser, transform chain, and synthesis state.  */
/* ------------------------------------------------------------------ */
#define MINA_MP3_MAX_CH        2
#define MINA_MP3_RESERV        511
#define MINA_MP3_MAX_FRAME     2880
#define MINA_MP3_MAINDATA (MINA_MP3_RESERV + MINA_MP3_MAX_FRAME)
#define MINA_MP3_DELAY         529     /* decoder pipeline delay, in samples */
#define MINA_MP3_HUFF_VALUES    2164
#define MINA_MP3_HUFF_NODES     4096

typedef struct {
    const mina_u8 *sfbtab;
    mina_u16 part_23_length, big_values, scalefac_compress;
    mina_u8  global_gain, block_type, mixed_block_flag, n_long_sfb, n_short_sfb;
    mina_u8  table_select[3], region_count[3], subblock_gain[3];
    mina_u8  preflag, scalefac_scale, count1_table, scfsi;
} mina_mp3_gr;

typedef struct {
    mina_i16 branch[2];
    mina_i16 value;
} mina_mp3_huff_node;

typedef struct {
    mina_u8 reservoir[MINA_MP3_RESERV];
    int     reserv_bytes;
    mina_i16 huff_values[MINA_MP3_HUFF_VALUES];
    mina_u8 count1_a[28];
    mina_u8 count1_b[16];
    mina_mp3_huff_node huff_nodes[MINA_MP3_HUFF_NODES];
    mina_i16 huff_roots[32];
    int     huff_node_count;
    float   mdct_overlap[MINA_MP3_MAX_CH][9 * 32];
    float   qmf_state[15 * 2 * 32];
} mina_mp3_st;

typedef struct {
    mina_mp3_gr gr_info[4];
    mina_u8     maindata[MINA_MP3_MAINDATA];
    float       grbuf[2][576];
    float       scf[40];
    float       syn[18 + 15][2 * 32];
    mina_u8     ist_pos[2][39];
} mina_mp3_scratch;

/* |x|^(4/3) from the normative table plus cubic interpolation */
static float mina_mp3_pow43(int x) {
    float frac;
    int sign, mult = 256;
    if (x < 129) return mina_mp3_pow43_tab[16 + x];
    if (x < 1024) { mult = 16; x <<= 3; }
    sign = 2 * x & 64;
    frac = (float)((x & 63) - sign) / (float)((x & ~63) + sign);
    return mina_mp3_pow43_tab[16 + ((x + sign) >> 6)] *
           (1.f + frac * ((4.f / 3) + frac * (2.f / 9))) * (float)mult;
}

/* y * 2^(e/4); e is always >= 0 at every call site */
static float mina_mp3_ldexp_q2(float y, int e) {
    int x;
    if (e < 0) return y;
    do {
        x = e < 120 ? e : 120;
        y *= mina_mp3_expfrac[x & 3] * (float)(1 << 30 >> (x >> 2));
    } while ((e -= x) > 0);
    return y;
}

static int mina_mp3_read_side_info(mina_br *bs, mina_mp3_gr *gr, int mpeg1,
                                   int mono, int sr_idx, mina_u32 *mdb_out) {
    mina_u32 tables = 0, scfsi = 0;
    mina_u32 main_data_begin;
    int gr_count = mono ? 1 : 2;
    if (mpeg1) {
        gr_count *= 2;
        main_data_begin = mina_br_u(bs, 9);
        scfsi = mina_br_u(bs, (unsigned)(7 + gr_count));
    } else {
        main_data_begin = mina_br_u(bs, (unsigned)(8 + gr_count)) >> gr_count;
    }
    do {
        if (mono) scfsi <<= 4;
        gr->part_23_length  = (mina_u16)mina_br_u(bs, 12);
        gr->big_values      = (mina_u16)mina_br_u(bs, 9);
        if (gr->big_values > 288) return -1;
        gr->global_gain     = (mina_u8)mina_br_u(bs, 8);
        gr->scalefac_compress = (mina_u16)mina_br_u(bs, (unsigned)(mpeg1 ? 4 : 9));
        gr->sfbtab = mina_mp3_scf_long + sr_idx * 23;
        gr->n_long_sfb = 22; gr->n_short_sfb = 0;
        if (mina_br_u(bs, 1)) {
            gr->block_type = (mina_u8)mina_br_u(bs, 2);
            if (!gr->block_type) return -1;
            gr->mixed_block_flag = (mina_u8)mina_br_u(bs, 1);
            gr->region_count[0] = 7; gr->region_count[1] = 255;
            if (gr->block_type == 2) {
                scfsi &= 0x0F0Fu;
                if (!gr->mixed_block_flag) {
                    gr->region_count[0] = 8;
                    gr->sfbtab = mina_mp3_scf_short + sr_idx * 40;
                    gr->n_long_sfb = 0; gr->n_short_sfb = 39;
                } else {
                    gr->sfbtab = mina_mp3_scf_mixed + sr_idx * 40;
                    gr->n_long_sfb = (mina_u8)(mpeg1 ? 8 : 6);
                    gr->n_short_sfb = 30;
                }
            }
            tables = mina_br_u(bs, 10);
            tables <<= 5;
            gr->subblock_gain[0] = (mina_u8)mina_br_u(bs, 3);
            gr->subblock_gain[1] = (mina_u8)mina_br_u(bs, 3);
            gr->subblock_gain[2] = (mina_u8)mina_br_u(bs, 3);
        } else {
            gr->block_type = 0; gr->mixed_block_flag = 0;
            tables = mina_br_u(bs, 15);
            gr->region_count[0] = (mina_u8)mina_br_u(bs, 4);
            gr->region_count[1] = (mina_u8)mina_br_u(bs, 3);
            gr->region_count[2] = 255;
        }
        gr->table_select[0] = (mina_u8)(tables >> 10);
        gr->table_select[1] = (mina_u8)((tables >> 5) & 31u);
        gr->table_select[2] = (mina_u8)(tables & 31u);
        gr->preflag = (mina_u8)(mpeg1 ? mina_br_u(bs, 1)
                                      : (gr->scalefac_compress >= 500));
        gr->scalefac_scale = (mina_u8)mina_br_u(bs, 1);
        gr->count1_table   = (mina_u8)mina_br_u(bs, 1);
        gr->scfsi = (mina_u8)((scfsi >> 12) & 15u);
        scfsi <<= 4;
        gr++;
    } while (--gr_count);
    if (bs->err) return -1;
    *mdb_out = main_data_begin;
    return 0;
}

static void mina_mp3_read_scalefactors(mina_u8 *scf, mina_u8 *ist_pos,
                                       const mina_u8 *scf_size,
                                       const mina_u8 *scf_count,
                                       mina_br *bs, int scfsi) {
    int i, k;
    for (i = 0; i < 4 && scf_count[i]; i++, scfsi *= 2) {
        int cnt = scf_count[i];
        if (scfsi & 8) {
            memcpy(scf, ist_pos, (size_t)cnt);
        } else {
            int bits = scf_size[i];
            if (!bits) {
                memset(scf, 0, (size_t)cnt);
                memset(ist_pos, 0, (size_t)cnt);
            } else {
                int max_scf = (scfsi < 0) ? (1 << bits) - 1 : -1;
                for (k = 0; k < cnt; k++) {
                    int s = (int)mina_br_u(bs, (unsigned)bits);
                    ist_pos[k] = (mina_u8)(s == max_scf ? 0xFF : s);
                    scf[k] = (mina_u8)s;
                }
            }
        }
        ist_pos += cnt; scf += cnt;
    }
    scf[0] = scf[1] = scf[2] = 0;
}

static void mina_mp3_decode_scalefactors(int mpeg1, mina_u8 *ist_pos, mina_br *bs,
                                         const mina_mp3_gr *gr, float *scf, int ch,
                                         int is_ist, int is_ms) {
    const mina_u8 *scf_partition =
        mina_mp3_scf_part + (!!gr->n_short_sfb + !gr->n_long_sfb) * 28;
    mina_u8 scf_size[4], iscf[40];
    int i, scf_shift = gr->scalefac_scale + 1, gain_exp, scfsi = gr->scfsi;
    float gain;

    if (mpeg1) {
        int part = mina_mp3_scfc_decode[gr->scalefac_compress & 15u];
        scf_size[1] = scf_size[0] = (mina_u8)(part >> 2);
        scf_size[3] = scf_size[2] = (mina_u8)(part & 3);
    } else {
        int k = 0, modprod, sfc, ist = is_ist && ch;
        sfc = (int)gr->scalefac_compress >> ist;
        for (k = ist * 3 * 4; sfc >= 0; sfc -= modprod, k += 4) {
            for (modprod = 1, i = 3; i >= 0; i--) {
                scf_size[i] = (mina_u8)(sfc / modprod % mina_mp3_g_mod[k + i]);
                modprod *= mina_mp3_g_mod[k + i];
            }
        }
        scf_partition += k;
        scfsi = -16;
    }
    mina_mp3_read_scalefactors(iscf, ist_pos, scf_size, scf_partition, bs, scfsi);

    if (gr->n_short_sfb) {
        int sh = 3 - scf_shift;
        for (i = 0; i < gr->n_short_sfb; i += 3) {
            iscf[gr->n_long_sfb + i + 0] =
                (mina_u8)(iscf[gr->n_long_sfb + i + 0] + (gr->subblock_gain[0] << sh));
            iscf[gr->n_long_sfb + i + 1] =
                (mina_u8)(iscf[gr->n_long_sfb + i + 1] + (gr->subblock_gain[1] << sh));
            iscf[gr->n_long_sfb + i + 2] =
                (mina_u8)(iscf[gr->n_long_sfb + i + 2] + (gr->subblock_gain[2] << sh));
        }
    } else if (gr->preflag) {
        for (i = 0; i < 10; i++)
            iscf[11 + i] = (mina_u8)(iscf[11 + i] + mina_mp3_preamp[i]);
    }

    gain_exp = (int)gr->global_gain + (-1) * 4 - 210 - (is_ms ? 2 : 0);
    gain = mina_mp3_ldexp_q2((float)(1 << (44 / 4)), 44 - gain_exp);
    for (i = 0; i < (int)(gr->n_long_sfb + gr->n_short_sfb); i++)
        scf[i] = mina_mp3_ldexp_q2(gain, (int)iscf[i] << scf_shift);
}

static int mina_mp3_new_huff_node(mina_mp3_st *st) {
    int node = st->huff_node_count++;
    if (node >= MINA_MP3_HUFF_NODES) return -1;
    st->huff_nodes[node].branch[0] = -1;
    st->huff_nodes[node].branch[1] = -1;
    st->huff_nodes[node].value = -1;
    return node;
}

static int mina_mp3_add_code(mina_mp3_st *st, int root, mina_u32 code,
                             unsigned length, mina_i16 value) {
    unsigned bitpos;
    int node = root;
    if (length > 32) return 0;
    for (bitpos = length; bitpos > 0; bitpos--) {
        int bit = (int)((code >> (bitpos - 1)) & 1u);
        int child = st->huff_nodes[node].branch[bit];
        if (st->huff_nodes[node].value >= 0) return 0;
        if (child < 0) {
            child = mina_mp3_new_huff_node(st);
            if (child < 0) return 0;
            st->huff_nodes[node].branch[bit] = (mina_i16)child;
        }
        node = child;
    }
    if (st->huff_nodes[node].value >= 0 && st->huff_nodes[node].value != value)
        return 0;
    if (st->huff_nodes[node].branch[0] >= 0 ||
        st->huff_nodes[node].branch[1] >= 0) return 0;
    st->huff_nodes[node].value = value;
    return 1;
}

static int mina_mp3_walk_book(mina_mp3_st *st, const mina_i16 *book, int root,
                              int index, mina_u32 prefix, unsigned depth,
                              unsigned selector, unsigned width) {
    mina_i16 entry = book[index];
    if (entry >= 0) {
        unsigned length = (unsigned)(entry >> 8);
        mina_u32 code;
        if (length > width || depth + length > 32) return 0;
        code = (prefix << length) |
               (length ? selector >> (width - length) : 0);
        return mina_mp3_add_code(st, root, code, depth + length, entry);
    } else {
        unsigned next_width = (unsigned)(entry & 7);
        unsigned branch;
        int base = -(entry >> 3);
        if (next_width > 8 || depth + width > 32) return 0;
        for (branch = 0; branch < (1u << next_width); branch++) {
            int child = base + (int)branch;
            if (child < 0 || !mina_mp3_walk_book(st, book, root, child,
                                                   (prefix << width) | selector,
                                                   depth + width, branch, next_width))
                return 0;
        }
    }
    return 1;
}

static int mina_mp3_build_tree(mina_mp3_st *st, const mina_i16 *book) {
    unsigned prefix;
    int root = mina_mp3_new_huff_node(st);
    if (root < 0) return -1;
    for (prefix = 0; prefix < 32u; prefix++) {
        mina_i16 entry = book[prefix];
        if (entry >= 0) {
            unsigned length = (unsigned)(entry >> 8);
            if (length > 5 || !mina_mp3_add_code(st, root,
                    length ? prefix >> (5 - length) : 0, length, entry))
                return -1;
        } else if (!mina_mp3_walk_book(st, book, root, prefix, 0, 0, prefix, 5)) {
            return -1;
        }
    }
    return root;
}

static int mina_mp3_tree_symbol(const mina_mp3_st *st, int tab_num, mina_br *bs) {
    int node = st->huff_roots[tab_num & 31];
    while (node >= 0 && st->huff_nodes[node].value < 0) {
        int bit = (int)mina_br_peek(bs, 1);
        mina_br_skip(bs, 1);
        node = st->huff_nodes[node].branch[bit];
    }
    return node >= 0 ? st->huff_nodes[node].value : 0;
}

static int mina_mp3_expand_huffman(mina_mp3_st *st) {
    size_t i, j, out = 0, count1_out;
    int tab_num;
    for (i = 0; i < mina_mp3_tab_run_count; i++) {
        const mina_mp3_tab_run *run = &mina_mp3_tab_runs[i];
        for (j = 0; j < (size_t)run->count; j++) {
            if (out >= MINA_MP3_HUFF_VALUES) return 0;
            st->huff_values[out++] = run->value;
        }
    }
    if (out != MINA_MP3_HUFF_VALUES) return 0;

    count1_out = 0;
    for (i = 0; i < sizeof(mina_mp3_count1_a_runs) / sizeof(mina_mp3_count1_a_runs[0]); i++) {
        for (j = 0; j < (size_t)mina_mp3_count1_a_runs[i].count; j++) {
            if (count1_out >= sizeof(st->count1_a)) return 0;
            st->count1_a[count1_out++] = mina_mp3_count1_a_runs[i].value;
        }
    }
    if (count1_out != sizeof(st->count1_a)) return 0;
    count1_out = 0;
    for (i = 0; i < sizeof(mina_mp3_count1_b_runs) / sizeof(mina_mp3_count1_b_runs[0]); i++) {
        for (j = 0; j < (size_t)mina_mp3_count1_b_runs[i].count; j++) {
            if (count1_out >= sizeof(st->count1_b)) return 0;
            st->count1_b[count1_out++] = mina_mp3_count1_b_runs[i].value;
        }
    }
    if (count1_out != sizeof(st->count1_b)) return 0;

    st->huff_node_count = 0;
    for (tab_num = 0; tab_num < 32; tab_num++) {
        int prior = -1, previous;
        int offset = mina_mp3_tabindex[tab_num];
        for (previous = 0; previous < tab_num; previous++) {
            if (mina_mp3_tabindex[previous] == offset) {
                prior = st->huff_roots[previous];
                break;
            }
        }
        if (prior >= 0) st->huff_roots[tab_num] = (mina_i16)prior;
        else {
            prior = mina_mp3_build_tree(st, st->huff_values + offset);
            if (prior < 0) return 0;
            st->huff_roots[tab_num] = (mina_i16)prior;
        }
    }
    return 1;
}

static void mina_mp3_huffman(float *dst, mina_br *bs, const mina_mp3_gr *gr,
                             const float *scf, size_t gr_limit,
                             const mina_mp3_st *st) {
    const float *scf_end = scf + 40;
    float *dst_end = dst + 576;
    float one = 0.0f;
    int ireg = 0, big_val_cnt = gr->big_values;
    const mina_u8 *sfb = gr->sfbtab;
    const mina_u8 *count1_table = gr->count1_table ? st->count1_b : st->count1_a;
    int np, pairs_to_decode;

    while (big_val_cnt > 0) {
        int tab_num = gr->table_select[ireg];
        int sfb_cnt = gr->region_count[ireg++];
        int linbits = mina_mp3_linbits[tab_num & 31];
        do {
            np = *sfb++ / 2;
            if (np <= 0) goto done;
            pairs_to_decode = big_val_cnt < np ? big_val_cnt : np;
            if (scf >= scf_end) goto done;
            one = *scf++;
            do {
                int j;
                int leaf;
                if (dst + 2 > dst_end) goto done;
                leaf = mina_mp3_tree_symbol(st, tab_num, bs);
                for (j = 0; j < 2; j++, dst++, leaf >>= 4) {
                    int lsb = leaf & 0x0F;
                    if (linbits && lsb == 15) {
                        int v = 15 + (int)mina_br_peek(bs, (unsigned)linbits);
                        mina_br_skip(bs, (unsigned)linbits);
                        *dst = one * mina_mp3_pow43(v) *
                               (mina_br_peek(bs, 1) ? -1.f : 1.f);
                        mina_br_skip(bs, 1);
                    } else if (lsb) {
                        *dst = one * mina_mp3_pow43(lsb) *
                               (mina_br_peek(bs, 1) ? -1.f : 1.f);
                        mina_br_skip(bs, 1);
                    } else {
                        *dst = 0.f;
                    }
                }
            } while (--pairs_to_decode);
        } while ((big_val_cnt -= np) > 0 && --sfb_cnt >= 0);
        if (ireg > 2) break;
    }

    for (np = 1 - big_val_cnt;; dst += 4) {
        int leaf = count1_table[mina_br_peek(bs, 4)];
        if (dst + 4 > dst_end) break;
        if (!(leaf & 8)) {
            int extra = leaf & 3;
            int bits = (int)mina_br_peek(bs, (unsigned)(4 + extra)) & ((1 << extra) - 1);
            leaf = count1_table[(leaf >> 3) + bits];
        }
        mina_br_skip(bs, (unsigned)(leaf & 7));
        if (bs->pos > gr_limit) break;
        if (!--np) {
            np = *sfb++ / 2;
            if (!np || scf >= scf_end) break;
            one = *scf++;
        }
        dst[0] = (leaf & 128) ? (mina_br_peek(bs, 1) ? -one : one) : 0.f;
        if (leaf & 128) mina_br_skip(bs, 1);
        dst[1] = (leaf & 64) ? (mina_br_peek(bs, 1) ? -one : one) : 0.f;
        if (leaf & 64) mina_br_skip(bs, 1);
        if (!--np) {
            np = *sfb++ / 2;
            if (!np || scf >= scf_end) break;
            one = *scf++;
        }
        dst[2] = (leaf & 32) ? (mina_br_peek(bs, 1) ? -one : one) : 0.f;
        if (leaf & 32) mina_br_skip(bs, 1);
        dst[3] = (leaf & 16) ? (mina_br_peek(bs, 1) ? -one : one) : 0.f;
        if (leaf & 16) mina_br_skip(bs, 1);
    }
done:
    bs->pos = gr_limit;
}

static void mina_mp3_midside(float *left, int n) {
    int i;
    for (i = 0; i < n; i++) {
        float a = left[i], b = left[i + 576];
        left[i] = a + b;
        left[i + 576] = a - b;
    }
}

static void mina_mp3_is_band(float *left, int n, float kl, float kr) {
    int i;
    for (i = 0; i < n; i++) {
        left[i + 576] = left[i] * kr;
        left[i] = left[i] * kl;
    }
}

static void mina_mp3_stereo_top_band(const float *right, const mina_u8 *sfb,
                                     int nbands, int max_band[3]) {
    int i, k;
    max_band[0] = max_band[1] = max_band[2] = -1;
    for (i = 0; i < nbands; i++) {
        for (k = 0; k < sfb[i]; k += 2) {
            if (right[k] != 0 || right[k + 1] != 0) { max_band[i % 3] = i; break; }
        }
        right += sfb[i];
    }
}

static void mina_mp3_stereo_process(float *left, const mina_u8 *ist_pos,
                                    const mina_u8 *sfb, int mpeg1, int ms,
                                    int max_band[3], int mpeg2_sh) {
    unsigned i, max_pos = mpeg1 ? 7u : 64u;
    for (i = 0; sfb[i]; i++) {
        unsigned ipos = ist_pos[i];
        if ((int)i > max_band[i % 3] && ipos < max_pos) {
            float kl, kr, s = ms ? 1.41421356f : 1.f;
            if (mpeg1) {
                kl = mina_mp3_pan[2 * ipos];
                kr = mina_mp3_pan[2 * ipos + 1];
            } else {
                kl = 1;
                kr = mina_mp3_ldexp_q2(1, (int)((ipos + 1) >> 1 << mpeg2_sh));
                if (ipos & 1u) { kl = kr; kr = 1; }
            }
            mina_mp3_is_band(left, sfb[i], kl * s, kr * s);
        } else if (ms) {
            mina_mp3_midside(left, sfb[i]);
        }
        left += sfb[i];
    }
}

static void mina_mp3_intensity(float *left, mina_u8 *ist_pos, const mina_mp3_gr *gr,
                               int mpeg1, int ms, int mpeg2_sh) {
    int max_band[3], n_sfb = gr->n_long_sfb + gr->n_short_sfb;
    int i, max_blocks = gr->n_short_sfb ? 3 : 1;
    mina_mp3_stereo_top_band(left + 576, gr->sfbtab, n_sfb, max_band);
    if (gr->n_long_sfb) {
        int m = max_band[0] > max_band[1] ? max_band[0] : max_band[1];
        m = m > max_band[2] ? m : max_band[2];
        max_band[0] = max_band[1] = max_band[2] = m;
    }
    for (i = 0; i < max_blocks; i++) {
        int default_pos = mpeg1 ? 3 : 0;
        int itop = n_sfb - max_blocks + i;
        int prev = itop - max_blocks;
        if (itop < 0 || itop >= 39) continue;
        ist_pos[itop] = (mina_u8)((prev < 0 || max_band[i] >= prev)
                                  ? default_pos : ist_pos[prev]);
    }
    mina_mp3_stereo_process(left, ist_pos, gr->sfbtab, mpeg1, ms, max_band, mpeg2_sh);
}

static void mina_mp3_reorder(float *grbuf, float *scratch, const mina_u8 *sfb) {
    int i, len;
    float *src = grbuf, *dst = scratch;
    for (; 0 != (len = *sfb); sfb += 3, src += 2 * len) {
        for (i = 0; i < len; i++, src++) {
            *dst++ = src[0 * len];
            *dst++ = src[1 * len];
            *dst++ = src[2 * len];
        }
    }
    memcpy(grbuf, scratch, (size_t)(dst - scratch) * sizeof(float));
}

static void mina_mp3_antialias(float *grbuf, int nbands) {
    for (; nbands > 0; nbands--, grbuf += 18) {
        int i;
        for (i = 0; i < 8; i++) {
            float u = grbuf[18 + i];
            float d = grbuf[17 - i];
            grbuf[18 + i] = u * mina_mp3_aa[i] - d * mina_mp3_aa[8 + i];
            grbuf[17 - i] = u * mina_mp3_aa[8 + i] + d * mina_mp3_aa[i];
        }
    }
}

static void mina_mp3_dct3_9(float *y) {
    float s0, s1, s2, s3, s4, s5, s6, s7, s8, t0, t2, t4;
    s0 = y[0]; s2 = y[2]; s4 = y[4]; s6 = y[6]; s8 = y[8];
    t0 = s0 + s6 * 0.5f; s0 -= s6;
    t4 = (s4 + s2) * 0.93969262f; t2 = (s8 + s2) * 0.76604444f;
    s6 = (s4 - s8) * 0.17364818f; s4 += s8 - s2;
    s2 = s0 - s4 * 0.5f; y[4] = s4 + s0;
    s8 = t0 - t2 + s6; s0 = t0 - t4 + t2; s4 = t0 + t4 - s6;
    s1 = y[1]; s3 = y[3]; s5 = y[5]; s7 = y[7];
    s3 *= 0.86602540f;
    t0 = (s5 + s1) * 0.98480775f; t4 = (s5 - s7) * 0.34202014f;
    t2 = (s1 + s7) * 0.64278761f; s1 = (s1 - s5 - s7) * 0.86602540f;
    s5 = t0 - s3 - t2; s7 = t4 - s3 - t0; s3 = t4 + s3 - t2;
    y[0] = s4 - s7; y[1] = s2 + s1; y[2] = s0 - s3; y[3] = s8 + s5;
    y[5] = s8 - s5; y[6] = s0 + s3; y[7] = s2 - s1; y[8] = s4 + s7;
}

static void mina_mp3_idct3(float x0, float x1, float x2, float *dst) {
    float m1 = x1 * 0.86602540f;
    float a1 = x0 - x2 * 0.5f;
    dst[1] = x0 + x2;
    dst[0] = a1 + m1;
    dst[2] = a1 - m1;
}

static void mina_mp3_imdct36(float *grbuf, float *overlap,
                             const float *window, int nbands) {
    int i, j;
    for (j = 0; j < nbands; j++, grbuf += 18, overlap += 9) {
        float co[9], si[9];
        co[0] = -grbuf[0];
        si[0] = grbuf[17];
        for (i = 0; i < 4; i++) {
            si[8 - 2 * i] = grbuf[4 * i + 1] - grbuf[4 * i + 2];
            co[1 + 2 * i] = grbuf[4 * i + 1] + grbuf[4 * i + 2];
            si[7 - 2 * i] = grbuf[4 * i + 4] - grbuf[4 * i + 3];
            co[2 + 2 * i] = -(grbuf[4 * i + 3] + grbuf[4 * i + 4]);
        }
        mina_mp3_dct3_9(co);
        mina_mp3_dct3_9(si);
        si[1] = -si[1]; si[3] = -si[3]; si[5] = -si[5]; si[7] = -si[7];
        for (i = 0; i < 9; i++) {
            float ovl = overlap[i];
            float sum = co[i] * mina_mp3_twid9[9 + i] + si[i] * mina_mp3_twid9[i];
            overlap[i] = co[i] * mina_mp3_twid9[i] - si[i] * mina_mp3_twid9[9 + i];
            grbuf[i] = ovl * window[i] - sum * window[9 + i];
            grbuf[17 - i] = ovl * window[9 + i] + sum * window[i];
        }
    }
}

static void mina_mp3_imdct12(float *x, float *dst, float *overlap) {
    float co[3], si[3];
    int i;
    mina_mp3_idct3(-x[0], x[6] + x[3], x[12] + x[9], co);
    mina_mp3_idct3(x[15], x[12] - x[9], x[6] - x[3], si);
    si[1] = -si[1];
    for (i = 0; i < 3; i++) {
        float ovl = overlap[i];
        float sum = co[i] * mina_mp3_twid3[3 + i] + si[i] * mina_mp3_twid3[i];
        overlap[i] = co[i] * mina_mp3_twid3[i] - si[i] * mina_mp3_twid3[3 + i];
        dst[i] = ovl * mina_mp3_twid3[2 - i] - sum * mina_mp3_twid3[5 - i];
        dst[5 - i] = ovl * mina_mp3_twid3[5 - i] + sum * mina_mp3_twid3[2 - i];
    }
}

static void mina_mp3_imdct_short(float *grbuf, float *overlap, int nbands) {
    for (; nbands > 0; nbands--, overlap += 9, grbuf += 18) {
        float tmp[18];
        memcpy(tmp, grbuf, sizeof(tmp));
        memcpy(grbuf, overlap, 6 * sizeof(float));
        mina_mp3_imdct12(tmp, grbuf + 6, overlap + 6);
        mina_mp3_imdct12(tmp + 1, grbuf + 12, overlap + 6);
        mina_mp3_imdct12(tmp + 2, overlap, overlap + 6);
    }
}

static void mina_mp3_change_sign(float *grbuf) {
    int b, i;
    for (b = 0, grbuf += 18; b < 32; b += 2, grbuf += 36)
        for (i = 1; i < 18; i += 2)
            grbuf[i] = -grbuf[i];
}

static void mina_mp3_imdct_gr(float *grbuf, float *overlap, unsigned block_type,
                              unsigned n_long_bands) {
    if (n_long_bands) {
        mina_mp3_imdct36(grbuf, overlap, mina_mp3_mdct_win, (int)n_long_bands);
        grbuf += 18 * n_long_bands;
        overlap += 9 * n_long_bands;
    }
    if (block_type == 2)
        mina_mp3_imdct_short(grbuf, overlap, 32 - (int)n_long_bands);
    else
        mina_mp3_imdct36(grbuf, overlap, mina_mp3_mdct_win + 18 * (block_type == 3),
                         32 - (int)n_long_bands);
}

static void mina_mp3_synth_pair(float *pcm, int nch, const float *z) {
    float a;
    a  = (z[14 * 64] - z[0]) * 29;
    a += (z[1 * 64] + z[13 * 64]) * 213;
    a += (z[12 * 64] - z[2 * 64]) * 459;
    a += (z[3 * 64] + z[11 * 64]) * 2037;
    a += (z[10 * 64] - z[4 * 64]) * 5153;
    a += (z[5 * 64] + z[9 * 64]) * 6574;
    a += (z[8 * 64] - z[6 * 64]) * 37489;
    a += z[7 * 64] * 75038;
    pcm[0] = a * (1.f / 32768.f);
    z += 2;
    a  = z[14 * 64] * 104;
    a += z[12 * 64] * 1567;
    a += z[10 * 64] * 9727;
    a += z[8 * 64] * 64019;
    a += z[6 * 64] * -9975;
    a += z[4 * 64] * -45;
    a += z[2 * 64] * 146;
    a += z[0 * 64] * -5;
    pcm[16 * nch] = a * (1.f / 32768.f);
}

static void mina_mp3_synth(float *xl, float *dstl, int nch, float *lins) {
    int i;
    float *xr = xl + 576 * (nch - 1);
    float *dstr = dstl + (nch - 1);
    const float *w = mina_mp3_win;
    float *zlin = lins + 15 * 64;

    zlin[4 * 15]     = xl[18 * 16];
    zlin[4 * 15 + 1] = xr[18 * 16];
    zlin[4 * 15 + 2] = xl[0];
    zlin[4 * 15 + 3] = xr[0];
    zlin[4 * 31]     = xl[1 + 18 * 16];
    zlin[4 * 31 + 1] = xr[1 + 18 * 16];
    zlin[4 * 31 + 2] = xl[1];
    zlin[4 * 31 + 3] = xr[1];

    mina_mp3_synth_pair(dstr, nch, lins + 4 * 15 + 1);
    mina_mp3_synth_pair(dstr + 32 * nch, nch, lins + 4 * 15 + 64 + 1);
    mina_mp3_synth_pair(dstl, nch, lins + 4 * 15);
    mina_mp3_synth_pair(dstl + 32 * nch, nch, lins + 4 * 15 + 64);

    for (i = 14; i >= 0; i--) {
        float a[4], b[4];
        float w0, w1, *vz, *vy;
        int j, k;

        zlin[4 * i]            = xl[18 * (31 - i)];
        zlin[4 * i + 1]        = xr[18 * (31 - i)];
        zlin[4 * i + 2]        = xl[1 + 18 * (31 - i)];
        zlin[4 * i + 3]        = xr[1 + 18 * (31 - i)];
        zlin[4 * (i + 16)]     = xl[1 + 18 * (1 + i)];
        zlin[4 * (i + 16) + 1] = xr[1 + 18 * (1 + i)];
        zlin[4 * (i - 16) + 2] = xl[18 * (1 + i)];
        zlin[4 * (i - 16) + 3] = xr[18 * (1 + i)];

        for (k = 0; k < 8; k++) {
            w0 = *w++; w1 = *w++;
            vz = &zlin[4 * i - k * 64];
            vy = &zlin[4 * i - (15 - k) * 64];
            if (k == 0) {
                for (j = 0; j < 4; j++) {
                    b[j] = vz[j] * w1 + vy[j] * w0;
                    a[j] = vz[j] * w0 - vy[j] * w1;
                }
            } else if (k & 1) {
                for (j = 0; j < 4; j++) {
                    b[j] += vz[j] * w1 + vy[j] * w0;
                    a[j] += vy[j] * w1 - vz[j] * w0;
                }
            } else {
                for (j = 0; j < 4; j++) {
                    b[j] += vz[j] * w1 + vy[j] * w0;
                    a[j] += vz[j] * w0 - vy[j] * w1;
                }
            }
        }

        dstr[(15 - i) * nch] = a[1] * (1.f / 32768.f);
        dstr[(17 + i) * nch] = b[1] * (1.f / 32768.f);
        dstl[(15 - i) * nch] = a[0] * (1.f / 32768.f);
        dstl[(17 + i) * nch] = b[0] * (1.f / 32768.f);
        dstr[(47 - i) * nch] = a[3] * (1.f / 32768.f);
        dstr[(49 + i) * nch] = b[3] * (1.f / 32768.f);
        dstl[(47 - i) * nch] = a[2] * (1.f / 32768.f);
        dstl[(49 + i) * nch] = b[2] * (1.f / 32768.f);
    }
}

static void mina_mp3_dct_ii(float *grbuf, int n) {
    int i, k;
    for (k = 0; k < n; k++) {
        float t[4][8], *x, *y = grbuf + k;
        for (x = t[0], i = 0; i < 8; i++, x++) {
            float x0 = y[i * 18];
            float x1 = y[(15 - i) * 18];
            float x2 = y[(16 + i) * 18];
            float x3 = y[(31 - i) * 18];
            float t0 = x0 + x3;
            float t1 = x1 + x2;
            float t2 = (x1 - x2) * mina_mp3_sec[3 * i + 0];
            float t3 = (x0 - x3) * mina_mp3_sec[3 * i + 1];
            x[0]  = t0 + t1;
            x[8]  = (t0 - t1) * mina_mp3_sec[3 * i + 2];
            x[16] = t3 + t2;
            x[24] = (t3 - t2) * mina_mp3_sec[3 * i + 2];
        }
        for (x = t[0], i = 0; i < 4; i++, x += 8) {
            float x0 = x[0], x1 = x[1], x2 = x[2], x3 = x[3];
            float x4 = x[4], x5 = x[5], x6 = x[6], x7 = x[7], xt;
            xt = x0 - x7; x0 += x7;
            x7 = x1 - x6; x1 += x6;
            x6 = x2 - x5; x2 += x5;
            x5 = x3 - x4; x3 += x4;
            x4 = x0 - x3; x0 += x3;
            x3 = x1 - x2; x1 += x2;
            x[0] = x0 + x1;
            x[4] = (x0 - x1) * 0.70710677f;
            x5 = x5 + x6;
            x6 = (x6 + x7) * 0.70710677f;
            x7 = x7 + xt;
            x3 = (x3 + x4) * 0.70710677f;
            x5 -= x7 * 0.198912367f;
            x7 += x5 * 0.382683432f;
            x5 -= x7 * 0.198912367f;
            x0 = xt - x6; xt += x6;
            x[1] = (xt + x7) * 0.50979561f;
            x[2] = (x4 + x3) * 0.54119611f;
            x[3] = (x0 - x5) * 0.60134488f;
            x[5] = (x0 + x5) * 0.89997619f;
            x[6] = (x4 - x3) * 1.30656302f;
            x[7] = (xt - x7) * 2.56291556f;
        }
        for (i = 0; i < 7; i++, y += 4 * 18) {
            y[0 * 18] = t[0][i];
            y[1 * 18] = t[2][i] + t[3][i] + t[3][i + 1];
            y[2 * 18] = t[1][i] + t[1][i + 1];
            y[3 * 18] = t[2][i + 1] + t[3][i] + t[3][i + 1];
        }
        y[0 * 18] = t[0][7];
        y[1 * 18] = t[2][7] + t[3][7];
        y[2 * 18] = t[1][7];
        y[3 * 18] = t[3][7];
    }
}

static void mina_mp3_synth_granule(float *qmf_state, float *grbuf, int nbands,
                                   int nch, float *pcm, float *lins) {
    int i;
    for (i = 0; i < nch; i++)
        mina_mp3_dct_ii(grbuf + 576 * i, nbands);
    memcpy(lins, qmf_state, sizeof(float) * 15 * 64);
    for (i = 0; i < nbands; i += 2)
        mina_mp3_synth(grbuf + i, pcm + 32 * nch * i, nch, lins + i * 64);
    if (nch == 1) {
        for (i = 0; i < 15 * 64; i += 2)
            qmf_state[i] = lins[nbands * 64 + i];
    } else {
        memcpy(qmf_state, lins + nbands * 64, sizeof(float) * 15 * 64);
    }
}

static void mina_mp3_save_reservoir(mina_mp3_st *st, const mina_br *bs,
                                    const mina_u8 *maindata, int maindata_bytes) {
    int pos = (int)((bs->pos + 7) >> 3);
    int remains = maindata_bytes - pos;
    if (pos < 0) { st->reserv_bytes = 0; return; }
    if (remains > MINA_MP3_RESERV) {
        pos += remains - MINA_MP3_RESERV;
        remains = MINA_MP3_RESERV;
    }
    if (remains > 0) memmove(st->reservoir, maindata + pos, (size_t)remains);
    else remains = 0;
    st->reserv_bytes = remains;
}

static int mina_mp3_restore_reservoir(mina_mp3_st *st, const mina_br *frame,
                                      mina_u8 *maindata, int *mdb,
                                      int main_data_begin) {
    int frame_bytes = (int)(frame->size - (frame->pos >> 3));
    int bytes_have = st->reserv_bytes < main_data_begin ? st->reserv_bytes
                                                        : main_data_begin;
    const mina_u8 *src = st->reservoir +
        (st->reserv_bytes > main_data_begin ? st->reserv_bytes - main_data_begin : 0);
    if (frame_bytes < 0) frame_bytes = 0;
    if (bytes_have < 0) bytes_have = 0;
    if (bytes_have + frame_bytes > MINA_MP3_MAINDATA)
        frame_bytes = MINA_MP3_MAINDATA - bytes_have;
    if (bytes_have) memcpy(maindata, src, (size_t)bytes_have);
    if (frame_bytes) memcpy(maindata + bytes_have,
                            frame->p + (frame->pos >> 3), (size_t)frame_bytes);
    *mdb = bytes_have + frame_bytes;
    return st->reserv_bytes >= main_data_begin;
}

/* Decode one frame's granules. Returns samples per channel produced. */
static int mina_mp3_decode_frame(mina_mp3_st *st, mina_mp3_scratch *sc,
                                 const mina_u8 *frame, int frame_size,
                                 int mpeg1, int mono, int nch, int sr_idx,
                                 int my_sr, int is_ms, int is_ist,
                                 float *pcm_out) {
    mina_br frame_bs, mbs;
    mina_u32 main_data_begin = 0;
    int mdb = 0, igr, ch;
    int granules = mpeg1 ? 2 : 1;

    memset(sc->ist_pos, 0, sizeof(sc->ist_pos));
    mina_br_init(&frame_bs, frame + 4, (size_t)frame_size - 4);
    if (!(frame[1] & 1u)) mina_br_u(&frame_bs, 16);   /* CRC-16 */

    if (mina_mp3_read_side_info(&frame_bs, sc->gr_info, mpeg1, mono, sr_idx,
                                &main_data_begin) < 0)
        return 0;
    if (!mina_mp3_restore_reservoir(st, &frame_bs, sc->maindata, &mdb,
                                    (int)main_data_begin))
        return 0;
    mina_br_init(&mbs, sc->maindata, (size_t)mdb);

    for (igr = 0; igr < granules; igr++, pcm_out += 576 * nch) {
        memset(sc->grbuf[0], 0, sizeof(sc->grbuf));
        for (ch = 0; ch < nch; ch++) {
            mina_mp3_gr *gr = &sc->gr_info[igr * nch + ch];
            size_t gr_limit = mbs.pos + gr->part_23_length;
            mina_mp3_decode_scalefactors(mpeg1, sc->ist_pos[ch], &mbs,
                                         gr, sc->scf,
                                         ch, is_ist, is_ms);
            mina_mp3_huffman(sc->grbuf[ch], &mbs, gr,
                             sc->scf, gr_limit, st);
        }
        if (is_ist && nch == 2) {
            /* the MPEG-2 intensity shift comes from the second channel's
             * scalefac_compress, exactly as the reference decoder does */
            mina_mp3_gr *gr = &sc->gr_info[igr * nch];
            int mpeg2_sh = gr[1].scalefac_compress & 1;
            mina_mp3_intensity(sc->grbuf[0], sc->ist_pos[1], gr,
                               mpeg1, is_ms, mpeg2_sh);
        } else if (is_ms && nch == 2) {
            mina_mp3_midside(sc->grbuf[0], 576);
        }
        for (ch = 0; ch < nch; ch++) {
            mina_mp3_gr *gr = &sc->gr_info[igr * nch + ch];
            int aa_bands = 31;
            int n_long_bands = (gr->mixed_block_flag ? 2 : 0) << (my_sr == 2);
            if (gr->n_short_sfb) {
                aa_bands = n_long_bands - 1;
                mina_mp3_reorder(sc->grbuf[ch] + n_long_bands * 18, sc->syn[0],
                                 gr->sfbtab + gr->n_long_sfb);
            }
            mina_mp3_antialias(sc->grbuf[ch], aa_bands);
            mina_mp3_imdct_gr(sc->grbuf[ch], st->mdct_overlap[ch], gr->block_type,
                              (unsigned)n_long_bands);
            mina_mp3_change_sign(sc->grbuf[ch]);
        }
        mina_mp3_synth_granule(st->qmf_state, sc->grbuf[0], 18, nch, pcm_out,
                               sc->syn[0]);
    }
    mina_mp3_save_reservoir(st, &mbs, sc->maindata, mdb);
    return granules * 576;
}

static mina_result mina_mp3_decode(const mina_u8 *d, size_t n, mina_pcm *out) {
    size_t pos;
    mina_mp3_st *st;
    mina_mp3_scratch *sc;
    mina_fbuf fb;
    mina_mp3_header first, h;
    mina_mp3_vbrtag tag;
    mina_u32 rate, channels;
    mina_result status = MINA_OK;
    size_t skip_head = 0, keep = 0;
    int have_keep = 0;

    pos = mina_mp3_id3_skip(d, n);
    if (pos >= n) return MINA_ERR_NOTFOUND;
    {
        size_t sync;
        if (mina_mp3_find_sync(d + pos, n - pos, &sync, &first) != 0)
            return MINA_ERR_NOTFOUND;
        pos += sync;
    }
    if (first.layer != 3) return MINA_ERR_UNSUPPORTED;   /* Layer I/II: info only */

    rate = first.sample_rate;
    channels = first.channels;

    mina_mp3_read_vbrtag(d, n, pos, &first, &tag);
    if (tag.present) {
        pos += first.frame_len;              /* the tag frame carries no audio */
        if (tag.have_lame) {
            skip_head = (size_t)tag.enc_delay + MINA_MP3_DELAY;
            if (tag.frames) {
                mina_u64 tot = mina_u64_mul32(tag.frames, first.samples_per_frame);
                mina_u64 trim = mina_u64_from(tag.enc_delay + tag.enc_padding);
                if (mina_u64_gt(tot, trim)) {
                    int ovf = 0;
                    keep = mina_u64_size(mina_u64_sub(tot, trim), &ovf);
                    have_keep = !ovf;
                }
            }
        }
    }

    st = (mina_mp3_st *)MINA_MALLOC(sizeof(mina_mp3_st));
    sc = (mina_mp3_scratch *)MINA_MALLOC(sizeof(mina_mp3_scratch));
    if (!st || !sc) { MINA_FREE(st); MINA_FREE(sc); return MINA_ERR_NOMEM; }
    memset(st, 0, sizeof(*st));
    if (!mina_mp3_expand_huffman(st)) {
        MINA_FREE(st); MINA_FREE(sc); return MINA_ERR_INVALID;
    }
    mina_fbuf_init(&fb);

    while (pos + 4 <= n) {
        int mpeg1, mono, nch, sr_idx, my_sr, is_ms, is_ist;
        size_t frame_size;
        float *dst;

        if (mina_mp3_parse_header(d + pos, &h) != 0 || !mina_mp3_same_format(&h, &first)) {
            size_t sync;
            mina_mp3_header hh;
            if (mina_mp3_find_sync(d + pos, n - pos, &sync, &hh) != 0)
                break;   /* trailing ID3v1/APE tag or clean end of stream */
            if (!mina_mp3_same_format(&hh, &first)) { status = MINA_ERR_TRUNCATED; break; }
            pos += sync;
            h = hh;
        }
        frame_size = (size_t)h.frame_len;
        if (frame_size < 4 || frame_size > n - pos) {
            if (frame_size > n - pos) status = MINA_ERR_TRUNCATED;
            break;
        }

        mpeg1 = (h.version == 3);
        mono  = (h.channel_mode == 3);
        nch   = mono ? 1 : 2;
        my_sr = h.srate_idx + (((h.version & 1u) + ((h.version >> 1) & 1u)) * 3);
        sr_idx = my_sr - (my_sr != 0);
        is_ms  = (h.channel_mode == 1) && ((h.mode_ext & 2u) != 0);
        is_ist = (h.channel_mode == 1) && ((h.mode_ext & 1u) != 0);
        dst = mina_fbuf_extend(&fb, (size_t)h.samples_per_frame * (size_t)nch);
        if (!dst) break;
        mina_mp3_decode_frame(st, sc, d + pos, (int)frame_size, mpeg1, mono, nch,
                              sr_idx, my_sr, is_ms, is_ist, dst);
        pos += frame_size;
    }

    MINA_FREE(st);
    MINA_FREE(sc);

    /* gapless trimming */
    if (fb.data && channels) {
        size_t total_frames = fb.len / channels;
        size_t head = skip_head < total_frames ? skip_head : total_frames;
        size_t avail = total_frames - head;
        if (have_keep && keep < avail) avail = keep;
        if (head)
            memmove(fb.data, fb.data + head * channels, avail * channels * sizeof(float));
        fb.len = avail * channels;
    }

    if (fb.len == 0 && status != MINA_OK) { mina_fbuf_free(&fb); return MINA_ERR_INVALID; }
    return mina_fbuf_finish(&fb, out, channels, rate, status);
}

#endif /* MINA_NO_MP3 */

/* ------------------------------------------------------------------ */
/* Ogg Vorbis I decoder                                                 */
/*                                                                      */
/* Follows the Vorbis I specification: setup headers, canonical Huffman  */
/* codebooks, floor 0                                                     */
/* and floor 1, residue 0/1/2, square-polar channel coupling, the        */
/* inverse MDCT and the four-case windowed overlap-add.                  */
/* ------------------------------------------------------------------ */
#ifndef MINA_NO_VORBIS

#define MINA_VORBIS_MAX_CH   255
#define MINA_VORBIS_MAX_BOOKS 1024
/* width of each codebook's direct-lookup table, in bits */
#define MINA_VORBIS_FAST_BITS 10

/* ---- complex FFT (radix-2, decimation in time) ---- */
typedef struct {
    int      n;
    int      log2n;
    double  *twr, *twi;    /* n/2 twiddles */
    int     *rev;          /* bit-reversal permutation */
} mina_fft;

static void mina_fft_free(mina_fft *f) {
    MINA_FREE(f->twr); MINA_FREE(f->twi); MINA_FREE(f->rev);
    f->twr = NULL; f->twi = NULL; f->rev = NULL; f->n = 0;
}

static int mina_fft_init(mina_fft *f, int n) {
    int i, j, k, lg = 0;
    memset(f, 0, sizeof(*f));
    if (n < 2 || (n & (n - 1))) return 0;
    while ((1 << lg) < n) lg++;
    f->n = n; f->log2n = lg;
    f->twr = (double *)MINA_MALLOC((size_t)(n / 2) * sizeof(double));
    f->twi = (double *)MINA_MALLOC((size_t)(n / 2) * sizeof(double));
    f->rev = (int *)MINA_MALLOC((size_t)n * sizeof(int));
    if (!f->twr || !f->twi || !f->rev) { mina_fft_free(f); return 0; }
    for (i = 0; i < n / 2; i++) {
        double a = -2.0 * MINA_PI * (double)i / (double)n;
        f->twr[i] = cos(a);
        f->twi[i] = sin(a);
    }
    for (i = 0; i < n; i++) {
        j = 0;
        for (k = 0; k < lg; k++) j |= ((i >> k) & 1) << (lg - 1 - k);
        f->rev[i] = j;
    }
    return 1;
}

/* In-place forward transform: Z[j] = sum_k z[k] * exp(-2*pi*i*j*k/n) */
static void mina_fft_run(const mina_fft *f, double *re, double *im) {
    int n = f->n, i, len, half, step, j, k;
    for (i = 0; i < n; i++) {
        int r = f->rev[i];
        if (r > i) {
            double t = re[i]; re[i] = re[r]; re[r] = t;
            t = im[i]; im[i] = im[r]; im[r] = t;
        }
    }
    for (len = 2; len <= n; len <<= 1) {
        half = len >> 1;
        step = n / len;
        for (i = 0; i < n; i += len) {
            for (j = 0, k = 0; j < half; j++, k += step) {
                double wr = f->twr[k], wi = f->twi[k];
                double xr = re[i + j + half], xi = im[i + j + half];
                double tr = xr * wr - xi * wi;
                double ti = xr * wi + xi * wr;
                re[i + j + half] = re[i + j] - tr;
                im[i + j + half] = im[i + j] - ti;
                re[i + j] += tr;
                im[i + j] += ti;
            }
        }
    }
}

/* ---- inverse MDCT ----
 *
 *   y[j] = sum_{k<n/2} X[k] * cos(2*pi/n * (j + 1/2 + n/4) * (k + 1/2))
 *
 * Factor the cosine into a length-n complex DFT with a pre- and a
 * post-twiddle, so the transform costs O(n log n) instead of O(n^2):
 *
 *   c[k] = X[k] * exp(-2*pi*i/n * (1/2 + n/4) * (k + 1/2))   (0 otherwise)
 *   y[j] = Re{ exp(-pi*i*j/n) * DFT_n(c)[j] }
 */
typedef struct {
    int      n;
    mina_fft fft;
    double  *prer, *prei;   /* n/2 */
    double  *postr, *posti; /* n   */
    double  *wr, *wi;       /* n scratch */
} mina_mdct;

static void mina_mdct_free(mina_mdct *m) {
    mina_fft_free(&m->fft);
    MINA_FREE(m->prer); MINA_FREE(m->prei);
    MINA_FREE(m->postr); MINA_FREE(m->posti);
    MINA_FREE(m->wr); MINA_FREE(m->wi);
    memset(m, 0, sizeof(*m));
}

static int mina_mdct_init(mina_mdct *m, int n) {
    int k;
    memset(m, 0, sizeof(*m));
    if (n < 8 || (n & (n - 1))) return 0;
    if (!mina_fft_init(&m->fft, n)) return 0;
    m->n = n;
    m->prer  = (double *)MINA_MALLOC((size_t)(n / 2) * sizeof(double));
    m->prei  = (double *)MINA_MALLOC((size_t)(n / 2) * sizeof(double));
    m->postr = (double *)MINA_MALLOC((size_t)n * sizeof(double));
    m->posti = (double *)MINA_MALLOC((size_t)n * sizeof(double));
    m->wr    = (double *)MINA_MALLOC((size_t)n * sizeof(double));
    m->wi    = (double *)MINA_MALLOC((size_t)n * sizeof(double));
    if (!m->prer || !m->prei || !m->postr || !m->posti || !m->wr || !m->wi) {
        mina_mdct_free(m); return 0;
    }
    for (k = 0; k < n / 2; k++) {
        double a = -2.0 * MINA_PI / (double)n * (0.5 + (double)n / 4.0)
                 * ((double)k + 0.5);
        m->prer[k] = cos(a);
        m->prei[k] = sin(a);
    }
    for (k = 0; k < n; k++) {
        double a = -MINA_PI * (double)k / (double)n;
        m->postr[k] = cos(a);
        m->posti[k] = sin(a);
    }
    return 1;
}

static void mina_mdct_backward(mina_mdct *m, const float *X, float *y) {
    int n = m->n, n2 = n >> 1, k;
    for (k = 0; k < n2; k++) {
        double x = (double)X[k];
        m->wr[k] = x * m->prer[k];
        m->wi[k] = x * m->prei[k];
    }
    for (k = n2; k < n; k++) { m->wr[k] = 0.0; m->wi[k] = 0.0; }
    mina_fft_run(&m->fft, m->wr, m->wi);
    for (k = 0; k < n; k++)
        y[k] = (float)(m->wr[k] * m->postr[k] - m->wi[k] * m->posti[k]);
}

/* ---- codebooks ---- */
typedef struct {
    int    dim;
    int    entries;
    int    used;            /* entries with a non-zero code length */
    int    lookup_type;
    int    sequence_p;
    int    lookup_values;
    float *valuelist;       /* entries * dim, only for lookup 1/2 */
    /* decode tree: node 0 is the root; child[2*i+bit], leaf when val >= 0 */
    int    nodes;
    mina_i32 *child;
    mina_i32 *val;
    /* direct lookup for codes no longer than fast_bits; 0 length = miss */
    int      fast_bits;
    mina_i32 *fast_val;
    mina_u8  *fast_len;
} mina_vorbis_book;

typedef struct {
    int  type;              /* 0 or 1 */
    /* floor 1 */
    int  partitions;
    int  partclass[32];
    int  class_dim[16], class_sub[16], class_master[16];
    int  sub_books[16 * 8];
    int  multiplier, rangebits, values, quant_q, range_n;
    int *xlist, *loneighbor, *hineighbor, *forward_index;
    /* floor 0 */
    int  order, rate, barkmap, amp_bits, amp_offset, nbooks;
    int  booklist[16];
} mina_vorbis_floor;

typedef struct {
    int       type;
    int       begin, end, part_size, classifications, classbook;
    mina_u32 *cascade;
    int      *books;        /* classifications * 8 */
} mina_vorbis_residue;

typedef struct {
    int  submaps;
    int  coupling_steps;
    int *couple_mag, *couple_ang;
    int *ch_mux;
    int *sub_floor, *sub_residue;
} mina_vorbis_mapping;

typedef struct { int blockflag; int mapping; } mina_vorbis_mode;

typedef struct {
    int channels;
    int sample_rate;
    int blocksize[2];
    int nbooks;   mina_vorbis_book    *books;
    int nfloors;  mina_vorbis_floor   *floors;
    int nres;     mina_vorbis_residue *residues;
    int nmap;     mina_vorbis_mapping *mappings;
    int nmodes;   mina_vorbis_mode    *modes;
    int modebits;
    /* synthesis state */
    mina_mdct mdct[2];
    float *w0, *w1;          /* half windows */
    float *obuf;             /* channels * blocksize[1] */
    float **work;            /* per channel, blocksize[1]/2 + slack */
    float *workmem;
    float *tbuf;             /* blocksize[1] imdct output */
    int   *floormem;         /* channels * (max floor posts) */
    int    posts_max;
    int    centerW;          /* 0 or 1 */
    int    prevW;            /* blockflag of the previous block */
    int    started;
} mina_vorbis_ctx;

static void mina_vorbis_free(mina_vorbis_ctx *v) {
    int i;
    if (!v) return;
    if (v->books) {
        for (i = 0; i < v->nbooks; i++) {
            MINA_FREE(v->books[i].valuelist);
            MINA_FREE(v->books[i].child);
            MINA_FREE(v->books[i].val);
            MINA_FREE(v->books[i].fast_val);
            MINA_FREE(v->books[i].fast_len);
        }
        MINA_FREE(v->books);
    }
    if (v->floors) {
        for (i = 0; i < v->nfloors; i++) {
            MINA_FREE(v->floors[i].xlist);
            MINA_FREE(v->floors[i].loneighbor);
            MINA_FREE(v->floors[i].hineighbor);
            MINA_FREE(v->floors[i].forward_index);
        }
        MINA_FREE(v->floors);
    }
    if (v->residues) {
        for (i = 0; i < v->nres; i++) {
            MINA_FREE(v->residues[i].cascade);
            MINA_FREE(v->residues[i].books);
        }
        MINA_FREE(v->residues);
    }
    if (v->mappings) {
        for (i = 0; i < v->nmap; i++) {
            MINA_FREE(v->mappings[i].couple_mag);
            MINA_FREE(v->mappings[i].couple_ang);
            MINA_FREE(v->mappings[i].ch_mux);
            MINA_FREE(v->mappings[i].sub_floor);
            MINA_FREE(v->mappings[i].sub_residue);
        }
        MINA_FREE(v->mappings);
    }
    MINA_FREE(v->modes);
    mina_mdct_free(&v->mdct[0]);
    mina_mdct_free(&v->mdct[1]);
    MINA_FREE(v->w0); MINA_FREE(v->w1);
    MINA_FREE(v->obuf);
    MINA_FREE(v->work); MINA_FREE(v->workmem);
    MINA_FREE(v->tbuf);
    MINA_FREE(v->floormem);
    MINA_FREE(v);
}

static int mina_ilog(mina_u32 v) {
    int r = 0;
    while (v) { r++; v >>= 1; }
    return r;
}

/* Vorbis float32_unpack */
static float mina_vorbis_f32u(mina_u32 u) {
    double mant = (double)(u & 0x1FFFFFUL);
    int e = (int)((u >> 21) & 0x3FFu) - 788;
    if (u & 0x80000000UL) mant = -mant;
    if (e > 200) e = 200;
    if (e < -200) e = -200;
    return (float)ldexp(mant, e);
}

/* Greatest v such that v^dim <= entries, using integer verification. */
static int mina_lookup1_values(int entries, int dim) {
    int vals;
    if (entries < 1 || dim < 1) return 0;
    vals = (int)floor(pow((double)entries, 1.0 / (double)dim));
    if (vals < 1) vals = 1;
    for (;;) {
        long acc = 1, acc1 = 1;
        int i;
        for (i = 0; i < dim; i++) {
            if (entries / vals < acc) break;
            acc *= vals;
            if (2147483647L / (vals + 1) < acc1) acc1 = 2147483647L;
            else acc1 *= (vals + 1);
        }
        if (i >= dim && acc <= entries && acc1 > entries) return vals;
        if (i < dim || acc > entries) vals--;
        else vals++;
        if (vals < 1) return 1;
    }
}

/* Canonical codeword assignment and tree construction. */
static int mina_vorbis_book_tree(mina_vorbis_book *b, const int *lens) {
    int i, j, e, n = b->entries, used = 0, nodes = 1, next = 1;
    mina_u32 *code;
    mina_u32 marker[33];

    for (i = 0; i < n; i++) if (lens[i] > 0) { used++; nodes += lens[i]; }
    b->used = used;
    b->nodes = nodes;
    if (!used) { b->child = NULL; b->val = NULL; return 1; }

    code = (mina_u32 *)MINA_MALLOC((size_t)n * sizeof(mina_u32));
    b->child = (mina_i32 *)MINA_MALLOC((size_t)nodes * 2 * sizeof(mina_i32));
    b->val   = (mina_i32 *)MINA_MALLOC((size_t)nodes * sizeof(mina_i32));
    if (!code || !b->child || !b->val) { MINA_FREE(code); return 0; }

    memset(marker, 0, sizeof(marker));
    for (i = 0; i < n; i++) {
        int length = lens[i];
        mina_u32 entry;
        if (length <= 0) { code[i] = 0; continue; }
        if (length > 32) { MINA_FREE(code); return 0; }
        entry = marker[length];
        /* an over-populated tree is not a valid codebook */
        if (length < 32 && (entry >> length)) { MINA_FREE(code); return 0; }
        code[i] = entry;
        for (j = length; j > 0; j--) {
            if (marker[j] & 1u) {
                if (j == 1) marker[1]++;
                else marker[j] = marker[j - 1] << 1;
                break;
            }
            marker[j]++;
        }
        for (j = length + 1; j < 33; j++) {
            if ((marker[j] >> 1) == entry) {
                entry = marker[j];
                marker[j] = marker[j - 1] << 1;
            } else break;
        }
    }

    for (i = 0; i < nodes * 2; i++) b->child[i] = -1;
    for (i = 0; i < nodes; i++) b->val[i] = -1;
    for (i = 0; i < n; i++) {
        int node = 0;
        if (lens[i] <= 0) continue;
        for (e = lens[i] - 1; e >= 0; e--) {
            int bit = (int)((code[i] >> e) & 1u);   /* MSB of the code first */
            int c = b->child[node * 2 + bit];
            if (c < 0) { c = next++; b->child[node * 2 + bit] = c; }
            node = c;
            if (b->val[node] >= 0) { MINA_FREE(code); return 0; } /* not prefix-free */
        }
        b->val[node] = i;
    }

    /* Direct-lookup acceleration. The stream is LSB-first while a codeword
     * is consumed MSB-first, so index the table by the bit-reversed code. */
    {
        int maxlen = 0, k;
        for (i = 0; i < n; i++) if (lens[i] > maxlen) maxlen = lens[i];
        k = maxlen < MINA_VORBIS_FAST_BITS ? maxlen : MINA_VORBIS_FAST_BITS;
        if (k > 0) {
            size_t tsize = (size_t)1 << k;
            b->fast_val = (mina_i32 *)MINA_MALLOC(tsize * sizeof(mina_i32));
            b->fast_len = (mina_u8 *)MINA_MALLOC(tsize);
            if (b->fast_val && b->fast_len) {
                b->fast_bits = k;
                memset(b->fast_len, 0, tsize);
                for (i = 0; i < n; i++) {
                    int L = lens[i];
                    mina_u32 rev = 0, j2;
                    if (L <= 0 || L > k) continue;
                    for (e = 0; e < L; e++)
                        rev |= ((code[i] >> (L - 1 - e)) & 1u) << e;
                    for (j2 = 0; j2 < ((mina_u32)1 << (k - L)); j2++) {
                        mina_u32 idx = rev | (j2 << L);
                        b->fast_val[idx] = i;
                        b->fast_len[idx] = (mina_u8)L;
                    }
                }
            } else {
                MINA_FREE(b->fast_val); MINA_FREE(b->fast_len);
                b->fast_val = NULL; b->fast_len = NULL; b->fast_bits = 0;
            }
        }
    }
    MINA_FREE(code);
    return 1;
}

static mina_i32 mina_vorbis_book_decode(const mina_vorbis_book *b, mina_lbr *bs) {
    int node = 0;
    if (!b->used || !b->child) { bs->eof = 1; return -1; }
    if (b->fast_bits && mina_lbr_left(bs) >= (size_t)b->fast_bits) {
        mina_u32 idx = mina_lbr_peek(bs, (unsigned)b->fast_bits);
        unsigned len = b->fast_len[idx];
        if (len) { bs->pos += len; return b->fast_val[idx]; }
    }
    for (;;) {
        int bit, c;
        if (b->val[node] >= 0) return b->val[node];
        bit = mina_lbr_bit(bs);
        if (bs->eof) return -1;
        c = b->child[node * 2 + bit];
        if (c < 0) { bs->eof = 1; return -1; }
        node = c;
    }
}

/* ---- floor 1 helpers ---- */
static const float mina_vorbis_fromdb_lookup[256] = {
    1.0649863e-07f,1.1341951e-07f,1.2079015e-07f,1.2863978e-07f,1.3699951e-07f,1.4590251e-07f,1.5538408e-07f,1.6548181e-07f,
    1.7623575e-07f,1.8768855e-07f,1.9988561e-07f,2.128753e-07f,2.2670913e-07f,2.4144197e-07f,2.5713223e-07f,2.7384213e-07f,
    2.9163793e-07f,3.1059021e-07f,3.3077411e-07f,3.5226968e-07f,3.7516214e-07f,3.9954229e-07f,4.255068e-07f,4.5315863e-07f,
    4.8260743e-07f,5.1396998e-07f,5.4737065e-07f,5.8294187e-07f,6.2082472e-07f,6.6116941e-07f,7.0413592e-07f,7.4989464e-07f,
    7.9862701e-07f,8.505263e-07f,9.0579828e-07f,9.6466216e-07f,1.0273513e-06f,1.0941144e-06f,1.1652161e-06f,1.2409384e-06f,
    1.3215816e-06f,1.4074654e-06f,1.4989305e-06f,1.5963394e-06f,1.7000785e-06f,1.8105592e-06f,1.9282195e-06f,2.0535261e-06f,
    2.1869758e-06f,2.3290978e-06f,2.4804557e-06f,2.6416497e-06f,2.813319e-06f,2.9961443e-06f,3.1908506e-06f,3.3982101e-06f,
    3.6190449e-06f,3.8542308e-06f,4.1047004e-06f,4.371447e-06f,4.6555282e-06f,4.9580707e-06f,5.280274e-06f,5.623416e-06f,
    5.9888572e-06f,6.3780469e-06f,6.7925283e-06f,7.2339451e-06f,7.7040476e-06f,8.2047e-06f,8.7378876e-06f,9.3057248e-06f,
    9.9104632e-06f,1.0554501e-05f,1.1240392e-05f,1.1970856e-05f,1.2748789e-05f,1.3577278e-05f,1.4459606e-05f,1.5399272e-05f,
    1.6400004e-05f,1.7465768e-05f,1.8600792e-05f,1.9809576e-05f,2.1096914e-05f,2.2467911e-05f,2.3928002e-05f,2.5482978e-05f,
    2.7139006e-05f,2.8902651e-05f,3.0780908e-05f,3.2781225e-05f,3.4911534e-05f,3.7180282e-05f,3.9596466e-05f,4.2169667e-05f,
    4.491009e-05f,4.7828601e-05f,5.0936773e-05f,5.4246931e-05f,5.7772202e-05f,6.1526565e-05f,6.5524908e-05f,6.9783085e-05f,
    7.4317983e-05f,7.9147585e-05f,8.429104e-05f,8.9768747e-05f,9.5602426e-05f,0.00010181521f,0.00010843174f,0.00011547824f,
    0.00012298267f,0.00013097477f,0.00013948625f,0.00014855085f,0.00015820453f,0.00016848555f,0.00017943469f,0.00019109536f,
    0.00020351382f,0.00021673929f,0.00023082423f,0.00024582449f,0.00026179955f,0.00027881276f,0.00029693158f,0.00031622787f,
    0.00033677814f,0.00035866388f,0.00038197188f,0.00040679456f,0.00043323036f,0.00046138411f,0.00049136745f,0.00052329927f,
    0.00055730621f,0.00059352311f,0.00063209358f,0.00067317058f,0.00071691700f,0.00076350630f,0.00081312324f,0.00086596457f,
    0.00092223983f,0.00098217216f,0.0010459992f,0.0011139742f,0.0011863665f,0.0012634633f,0.0013455702f,0.0014330129f,
    0.0015261382f,0.0016253153f,0.0017309374f,0.0018434235f,0.0019632195f,0.0020908006f,0.0022266726f,0.0023713743f,
    0.0025254795f,0.0026895994f,0.0028643847f,0.0030505286f,0.0032487691f,0.0034598925f,0.0036847358f,0.0039241906f,
    0.0041792066f,0.0044507950f,0.0047400328f,0.0050480668f,0.0053761186f,0.0057254891f,0.0060975636f,0.0064938176f,
    0.0069158225f,0.0073652516f,0.0078438871f,0.0083536271f,0.0088964928f,0.009474637f,0.010090352f,0.010746080f,
    0.011444421f,0.012188144f,0.012980198f,0.013823725f,0.014722068f,0.015678791f,0.016697687f,0.017782797f,
    0.018938423f,0.020169149f,0.021479854f,0.022875735f,0.024362330f,0.025945531f,0.027631618f,0.029427276f,
    0.031339626f,0.033376252f,0.035545228f,0.037855157f,0.040315199f,0.042935108f,0.045725273f,0.048696758f,
    0.051861348f,0.055231591f,0.058820850f,0.062643361f,0.066714279f,0.071049749f,0.075666962f,0.080584227f,
    0.085821044f,0.091398179f,0.097337747f,0.10366330f,0.11039993f,0.11757434f,0.12521498f,0.13335215f,
    0.14201813f,0.15124727f,0.16107617f,0.17154380f,0.18269168f,0.19456402f,0.20720788f,0.22067342f,
    0.23501402f,0.25028656f,0.26655159f,0.28387361f,0.30232132f,0.32196786f,0.34289114f,0.36517414f,
    0.38890521f,0.41417847f,0.44109412f,0.46975890f,0.50028648f,0.53279791f,0.56742212f,0.60429640f,
    0.64356699f,0.68538959f,0.72993007f,0.77736504f,0.82788260f,0.88168307f,0.9389798f,1.0f,
};

static float mina_vorbis_fromdb(int v) {
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    return mina_vorbis_fromdb_lookup[v];
}

static int mina_vorbis_render_point(int x0, int x1, int y0, int y1, int x) {
    int dy, adx, ady, err, off;
    y0 &= 0x7FFF;
    y1 &= 0x7FFF;
    dy = y1 - y0;
    adx = x1 - x0;
    if (adx == 0) return y0;
    ady = dy > 0 ? dy : -dy;
    err = ady * (x - x0);
    off = err / adx;
    return dy < 0 ? (y0 - off) : (y0 + off);
}

static void mina_vorbis_render_line(int n, int x0, int x1, int y0, int y1, float *d) {
    int dy = y1 - y0;
    int adx = x1 - x0;
    int ady = dy > 0 ? dy : -dy;
    int base, sy, x = x0, y = y0, err = 0;
    if (adx <= 0) return;
    base = dy / adx;
    sy = (dy < 0) ? base - 1 : base + 1;
    ady -= (base > 0 ? base : -base) * adx;
    if (n > x1) n = x1;
    if (x < n) d[x] *= mina_vorbis_fromdb(y);
    while (++x < n) {
        err += ady;
        if (err >= adx) { err -= adx; y += sy; }
        else y += base;
        d[x] *= mina_vorbis_fromdb(y);
    }
}

/* ---- setup header parsing ---- */
static int mina_vorbis_read_book(mina_vorbis_book *b, mina_lbr *bs) {
    int j, ordered, sparse = 0, ok;
    int *lens;

    if (mina_lbr_read(bs, 24) != 0x564342UL) return 0;    /* "BCV" */
    b->dim     = (int)mina_lbr_read(bs, 16);
    b->entries = (int)mina_lbr_read(bs, 24);
    if (b->dim <= 0 || b->entries <= 0 || b->dim > 256) return 0;
    if (b->entries > (1 << 24)) return 0;

    lens = (int *)MINA_MALLOC((size_t)b->entries * sizeof(int));
    if (!lens) return 0;

    ordered = (int)mina_lbr_read(bs, 1);
    if (ordered) {
        int current_entry = 0;
        int current_length = (int)mina_lbr_read(bs, 5) + 1;
        while (current_entry < b->entries) {
            int bits = mina_ilog((mina_u32)(b->entries - current_entry));
            int num = (int)mina_lbr_read(bs, (unsigned)bits);
            if (current_entry + num > b->entries) { MINA_FREE(lens); return 0; }
            for (j = 0; j < num; j++) lens[current_entry + j] = current_length;
            current_entry += num;
            current_length++;
            if (current_length > 32 && current_entry < b->entries) {
                MINA_FREE(lens); return 0;
            }
            if (bs->eof) { MINA_FREE(lens); return 0; }
        }
    } else {
        sparse = (int)mina_lbr_read(bs, 1);
        for (j = 0; j < b->entries; j++) {
            if (sparse && !mina_lbr_read(bs, 1)) { lens[j] = 0; continue; }
            lens[j] = (int)mina_lbr_read(bs, 5) + 1;
        }
        if (bs->eof) { MINA_FREE(lens); return 0; }
    }

    b->lookup_type = (int)mina_lbr_read(bs, 4);
    if (b->lookup_type > 2) { MINA_FREE(lens); return 0; }
    if (b->lookup_type) {
        float minval = mina_vorbis_f32u(mina_lbr_read(bs, 32));
        float delta  = mina_vorbis_f32u(mina_lbr_read(bs, 32));
        int value_bits = (int)mina_lbr_read(bs, 4) + 1;
        int vals, e, k;
        mina_u32 *mul;
        b->sequence_p = (int)mina_lbr_read(bs, 1);
        vals = (b->lookup_type == 1) ? mina_lookup1_values(b->entries, b->dim)
                                     : b->entries * b->dim;
        if (vals <= 0) { MINA_FREE(lens); return 0; }
        b->lookup_values = vals;
        mul = (mina_u32 *)MINA_MALLOC((size_t)vals * sizeof(mina_u32));
        b->valuelist = (float *)MINA_MALLOC((size_t)b->entries * (size_t)b->dim
                                            * sizeof(float));
        if (!mul || !b->valuelist) { MINA_FREE(mul); MINA_FREE(lens); return 0; }
        for (j = 0; j < vals; j++) mul[j] = mina_lbr_read(bs, (unsigned)value_bits);
        for (e = 0; e < b->entries; e++) {
            float last = 0.0f;
            int indexdiv = 1;
            for (k = 0; k < b->dim; k++) {
                int idx = (b->lookup_type == 1) ? ((e / indexdiv) % vals)
                                                : (e * b->dim + k);
                float val = (float)mul[idx] * delta + minval + last;
                if (b->sequence_p) last = val;
                b->valuelist[(size_t)e * b->dim + k] = val;
                if (b->lookup_type == 1) indexdiv *= vals;
            }
        }
        MINA_FREE(mul);
    }

    ok = mina_vorbis_book_tree(b, lens);
    MINA_FREE(lens);
    return ok && !bs->eof;
}

static int mina_vorbis_read_floor(mina_vorbis_ctx *v, mina_vorbis_floor *f,
                                  mina_lbr *bs) {
    int j, k;
    f->type = (int)mina_lbr_read(bs, 16);
    if (f->type == 0) {
        f->order      = (int)mina_lbr_read(bs, 8);
        f->rate       = (int)mina_lbr_read(bs, 16);
        f->barkmap    = (int)mina_lbr_read(bs, 16);
        f->amp_bits   = (int)mina_lbr_read(bs, 6);
        f->amp_offset = (int)mina_lbr_read(bs, 8);
        f->nbooks     = (int)mina_lbr_read(bs, 4) + 1;
        if (f->order < 1 || f->rate < 1 || f->barkmap < 1 || f->amp_bits > 32)
            return 0;
        for (j = 0; j < f->nbooks; j++) {
            int b = (int)mina_lbr_read(bs, 8);
            if (b < 0 || b >= v->nbooks) return 0;
            if (j < 16) f->booklist[j] = b;
        }
        if (f->nbooks > 16) return 0;
        return !bs->eof;
    }
    if (f->type != 1) return 0;

    f->partitions = (int)mina_lbr_read(bs, 5);
    if (f->partitions > 32) return 0;
    for (j = 0; j < 16; j++) {
        f->class_dim[j] = 0; f->class_sub[j] = 0; f->class_master[j] = 0;
    }
    for (j = 0; j < 16 * 8; j++) f->sub_books[j] = -1;
    {
        int maxclass = -1;
        for (j = 0; j < f->partitions; j++) {
            f->partclass[j] = (int)mina_lbr_read(bs, 4);
            if (f->partclass[j] > maxclass) maxclass = f->partclass[j];
        }
        for (j = 0; j <= maxclass; j++) {
            int cdim = (int)mina_lbr_read(bs, 3) + 1;
            int cbits = (int)mina_lbr_read(bs, 2);
            int nsub = 1 << cbits;
            f->class_dim[j] = cdim;
            f->class_sub[j] = cbits;
            if (cbits > 0) {
                f->class_master[j] = (int)mina_lbr_read(bs, 8);
                if (f->class_master[j] >= v->nbooks) return 0;
            }
            for (k = 0; k < nsub; k++) {
                int b = (int)mina_lbr_read(bs, 8) - 1;
                if (b >= v->nbooks) return 0;
                f->sub_books[j * 8 + k] = b;
            }
        }
    }
    f->multiplier = (int)mina_lbr_read(bs, 2) + 1;
    f->rangebits  = (int)mina_lbr_read(bs, 4);
    f->values = 2;
    for (j = 0; j < f->partitions; j++) f->values += f->class_dim[f->partclass[j]];
    if (f->values > 65 || f->values < 2) return 0;   /* VIF_POSIT + 2 */

    f->xlist         = (int *)MINA_MALLOC((size_t)f->values * sizeof(int));
    f->loneighbor    = (int *)MINA_MALLOC((size_t)f->values * sizeof(int));
    f->hineighbor    = (int *)MINA_MALLOC((size_t)f->values * sizeof(int));
    f->forward_index = (int *)MINA_MALLOC((size_t)f->values * sizeof(int));
    if (!f->xlist || !f->loneighbor || !f->hineighbor || !f->forward_index) return 0;

    f->xlist[0] = 0;
    f->xlist[1] = 1 << f->rangebits;
    {
        int off = 2;
        for (j = 0; j < f->partitions; j++) {
            int c = f->partclass[j];
            for (k = 0; k < f->class_dim[c]; k++)
                f->xlist[off++] = (int)mina_lbr_read(bs, (unsigned)f->rangebits);
        }
    }
    if (bs->eof) return 0;

    /* ascending-x index (insertion sort keeps ties in declaration order) */
    for (j = 0; j < f->values; j++) f->forward_index[j] = j;
    for (j = 1; j < f->values; j++) {
        int cur = f->forward_index[j], i = j - 1;
        while (i >= 0 && f->xlist[f->forward_index[i]] > f->xlist[cur]) {
            f->forward_index[i + 1] = f->forward_index[i];
            i--;
        }
        f->forward_index[i + 1] = cur;
    }
    {
        static const int qq[4] = { 256, 128, 86, 64 };
        f->quant_q = qq[f->multiplier - 1];
        f->range_n = f->xlist[1];
    }
    for (j = 0; j < f->values - 2; j++) {
        int lo = 0, hi = 1, lx = 0, hx = f->range_n, cx = f->xlist[j + 2], i;
        for (i = 0; i < j + 2; i++) {
            int x = f->xlist[i];
            if (x > lx && x < cx) { lo = i; lx = x; }
            if (x < hx && x > cx) { hi = i; hx = x; }
        }
        f->loneighbor[j] = lo;
        f->hineighbor[j] = hi;
    }
    return 1;
}

static int mina_vorbis_read_residue(mina_vorbis_ctx *v, mina_vorbis_residue *r,
                                    mina_lbr *bs) {
    int j, k;
    r->type = (int)mina_lbr_read(bs, 16);
    if (r->type > 2) return 0;
    r->begin           = (int)mina_lbr_read(bs, 24);
    r->end             = (int)mina_lbr_read(bs, 24);
    r->part_size       = (int)mina_lbr_read(bs, 24) + 1;
    r->classifications = (int)mina_lbr_read(bs, 6) + 1;
    r->classbook       = (int)mina_lbr_read(bs, 8);
    if (r->classbook >= v->nbooks || r->end < r->begin) return 0;
    r->cascade = (mina_u32 *)MINA_MALLOC((size_t)r->classifications * sizeof(mina_u32));
    r->books   = (int *)MINA_MALLOC((size_t)r->classifications * 8 * sizeof(int));
    if (!r->cascade || !r->books) return 0;
    for (j = 0; j < r->classifications * 8; j++) r->books[j] = -1;
    for (j = 0; j < r->classifications; j++) {
        mina_u32 lowbits = mina_lbr_read(bs, 3);
        mina_u32 high = 0;
        if (mina_lbr_read(bs, 1)) high = mina_lbr_read(bs, 5);
        r->cascade[j] = (high << 3) | lowbits;
    }
    for (j = 0; j < r->classifications; j++) {
        for (k = 0; k < 8; k++) {
            if (r->cascade[j] & ((mina_u32)1 << k)) {
                int b = (int)mina_lbr_read(bs, 8);
                if (b >= v->nbooks) return 0;
                r->books[j * 8 + k] = b;
            }
        }
    }
    /* the classbook must be able to address every partition word */
    {
        long need = 1;
        int d = v->books[r->classbook].dim;
        for (j = 0; j < d; j++) {
            need *= r->classifications;
            if (need > v->books[r->classbook].entries) return 0;
        }
    }
    return !bs->eof;
}

static int mina_vorbis_read_mapping(mina_vorbis_ctx *v, mina_vorbis_mapping *m,
                                    mina_lbr *bs) {
    int j;
    if (mina_lbr_read(bs, 16) != 0) return 0;
    m->submaps = mina_lbr_read(bs, 1) ? (int)mina_lbr_read(bs, 4) + 1 : 1;
    if (mina_lbr_read(bs, 1)) {
        int bits = mina_ilog((mina_u32)(v->channels - 1));
        m->coupling_steps = (int)mina_lbr_read(bs, 8) + 1;
        m->couple_mag = (int *)MINA_MALLOC((size_t)m->coupling_steps * sizeof(int));
        m->couple_ang = (int *)MINA_MALLOC((size_t)m->coupling_steps * sizeof(int));
        if (!m->couple_mag || !m->couple_ang) return 0;
        for (j = 0; j < m->coupling_steps; j++) {
            m->couple_mag[j] = (int)mina_lbr_read(bs, (unsigned)bits);
            m->couple_ang[j] = (int)mina_lbr_read(bs, (unsigned)bits);
            if (m->couple_mag[j] == m->couple_ang[j] ||
                m->couple_mag[j] >= v->channels || m->couple_ang[j] >= v->channels)
                return 0;
        }
    } else {
        m->coupling_steps = 0;
    }
    if (mina_lbr_read(bs, 2) != 0) return 0;    /* reserved */
    m->ch_mux = (int *)MINA_MALLOC((size_t)v->channels * sizeof(int));
    if (!m->ch_mux) return 0;
    if (m->submaps > 1) {
        for (j = 0; j < v->channels; j++) {
            m->ch_mux[j] = (int)mina_lbr_read(bs, 4);
            if (m->ch_mux[j] >= m->submaps) return 0;
        }
    } else {
        for (j = 0; j < v->channels; j++) m->ch_mux[j] = 0;
    }
    m->sub_floor   = (int *)MINA_MALLOC((size_t)m->submaps * sizeof(int));
    m->sub_residue = (int *)MINA_MALLOC((size_t)m->submaps * sizeof(int));
    if (!m->sub_floor || !m->sub_residue) return 0;
    for (j = 0; j < m->submaps; j++) {
        mina_lbr_read(bs, 8);                     /* unused time config */
        m->sub_floor[j]   = (int)mina_lbr_read(bs, 8);
        m->sub_residue[j] = (int)mina_lbr_read(bs, 8);
        if (m->sub_floor[j] >= v->nfloors || m->sub_residue[j] >= v->nres) return 0;
    }
    return !bs->eof;
}

static mina_vorbis_ctx *mina_vorbis_setup(const mina_u8 *idp, size_t idn,
                                          const mina_u8 *sup, size_t sun) {
    mina_vorbis_ctx *v = (mina_vorbis_ctx *)MINA_MALLOC(sizeof(mina_vorbis_ctx));
    mina_lbr bs;
    int i;
    if (!v) return NULL;
    memset(v, 0, sizeof(*v));

    if (idn < 30 || idp[0] != 1 || memcmp(idp + 1, "vorbis", 6) != 0) goto fail;
    if (mina_le32(idp + 7) != 0) goto fail;               /* version */
    v->channels    = idp[11];
    v->sample_rate = (int)mina_le32(idp + 12);
    v->blocksize[0] = 1 << (idp[28] & 0x0Fu);
    v->blocksize[1] = 1 << (idp[28] >> 4);
    if (v->channels < 1 || v->channels > MINA_VORBIS_MAX_CH) goto fail;
    if (v->sample_rate < 1) goto fail;
    if (v->blocksize[0] < 64 || v->blocksize[1] > 8192 ||
        v->blocksize[0] > v->blocksize[1]) goto fail;
    if (!(idp[29] & 1u)) goto fail;                        /* framing */

    if (sun < 8 || sup[0] != 5 || memcmp(sup + 1, "vorbis", 6) != 0) goto fail;
    mina_lbr_init(&bs, sup + 7, sun - 7);

    v->nbooks = (int)mina_lbr_read(&bs, 8) + 1;
    v->books = (mina_vorbis_book *)MINA_MALLOC((size_t)v->nbooks *
                                               sizeof(mina_vorbis_book));
    if (!v->books) goto fail;
    memset(v->books, 0, (size_t)v->nbooks * sizeof(mina_vorbis_book));
    for (i = 0; i < v->nbooks; i++)
        if (!mina_vorbis_read_book(&v->books[i], &bs)) goto fail;

    {   /* time domain transforms: all must be zero */
        int tcount = (int)mina_lbr_read(&bs, 6) + 1;
        for (i = 0; i < tcount; i++)
            if (mina_lbr_read(&bs, 16) != 0) goto fail;
    }

    v->nfloors = (int)mina_lbr_read(&bs, 6) + 1;
    v->floors = (mina_vorbis_floor *)MINA_MALLOC((size_t)v->nfloors *
                                                 sizeof(mina_vorbis_floor));
    if (!v->floors) goto fail;
    memset(v->floors, 0, (size_t)v->nfloors * sizeof(mina_vorbis_floor));
    for (i = 0; i < v->nfloors; i++)
        if (!mina_vorbis_read_floor(v, &v->floors[i], &bs)) goto fail;

    v->nres = (int)mina_lbr_read(&bs, 6) + 1;
    v->residues = (mina_vorbis_residue *)MINA_MALLOC((size_t)v->nres *
                                                     sizeof(mina_vorbis_residue));
    if (!v->residues) goto fail;
    memset(v->residues, 0, (size_t)v->nres * sizeof(mina_vorbis_residue));
    for (i = 0; i < v->nres; i++)
        if (!mina_vorbis_read_residue(v, &v->residues[i], &bs)) goto fail;

    v->nmap = (int)mina_lbr_read(&bs, 6) + 1;
    v->mappings = (mina_vorbis_mapping *)MINA_MALLOC((size_t)v->nmap *
                                                     sizeof(mina_vorbis_mapping));
    if (!v->mappings) goto fail;
    memset(v->mappings, 0, (size_t)v->nmap * sizeof(mina_vorbis_mapping));
    for (i = 0; i < v->nmap; i++)
        if (!mina_vorbis_read_mapping(v, &v->mappings[i], &bs)) goto fail;

    v->nmodes = (int)mina_lbr_read(&bs, 6) + 1;
    v->modes = (mina_vorbis_mode *)MINA_MALLOC((size_t)v->nmodes *
                                               sizeof(mina_vorbis_mode));
    if (!v->modes) goto fail;
    for (i = 0; i < v->nmodes; i++) {
        v->modes[i].blockflag = (int)mina_lbr_read(&bs, 1);
        mina_lbr_read(&bs, 16);                  /* window type   */
        mina_lbr_read(&bs, 16);                  /* transform type */
        v->modes[i].mapping = (int)mina_lbr_read(&bs, 8);
        if (v->modes[i].mapping >= v->nmap) goto fail;
    }
    if (!mina_lbr_read(&bs, 1) || bs.eof) goto fail;   /* framing */
    v->modebits = mina_ilog((mina_u32)(v->nmodes - 1));

    /* synthesis state */
    {
        int n0 = v->blocksize[0] >> 1, n1 = v->blocksize[1] >> 1, k, ch;
        if (!mina_mdct_init(&v->mdct[0], v->blocksize[0])) goto fail;
        if (!mina_mdct_init(&v->mdct[1], v->blocksize[1])) goto fail;
        v->w0 = (float *)MINA_MALLOC((size_t)n0 * sizeof(float));
        v->w1 = (float *)MINA_MALLOC((size_t)n1 * sizeof(float));
        v->obuf = (float *)MINA_MALLOC((size_t)v->channels *
                                       (size_t)v->blocksize[1] * sizeof(float));
        v->work = (float **)MINA_MALLOC((size_t)v->channels * sizeof(float *));
        v->workmem = (float *)MINA_MALLOC((size_t)v->channels *
                                          (size_t)(n1 + 256) * sizeof(float));
        v->tbuf = (float *)MINA_MALLOC((size_t)v->blocksize[1] * sizeof(float));
        v->posts_max = 65;
        v->floormem = (int *)MINA_MALLOC((size_t)v->channels *
                                         (size_t)v->posts_max * sizeof(int));
        if (!v->w0 || !v->w1 || !v->obuf || !v->work || !v->workmem ||
            !v->tbuf || !v->floormem) goto fail;
        memset(v->floormem, 0, (size_t)v->channels *
                               (size_t)v->posts_max * sizeof(int));
        for (k = 0; k < n0; k++) {
            double p = ((double)k + 0.5) / (double)n0 * MINA_PI / 2.0;
            v->w0[k] = (float)sin(MINA_PI / 2.0 * sin(p) * sin(p));
        }
        for (k = 0; k < n1; k++) {
            double p = ((double)k + 0.5) / (double)n1 * MINA_PI / 2.0;
            v->w1[k] = (float)sin(MINA_PI / 2.0 * sin(p) * sin(p));
        }
        for (ch = 0; ch < v->channels; ch++)
            v->work[ch] = v->workmem + (size_t)ch * (size_t)(n1 + 256);
        memset(v->obuf, 0, (size_t)v->channels * (size_t)v->blocksize[1] * sizeof(float));
    }
    v->centerW = 0;
    v->prevW = 0;
    v->started = 0;
    return v;

fail:
    mina_vorbis_free(v);
    return NULL;
}

/* ---- floor decode ---- */
/* Returns 1 when the floor is non-zero and y[] was filled. */
static int mina_vorbis_floor1_decode(const mina_vorbis_ctx *v,
                                     const mina_vorbis_floor *f,
                                     mina_lbr *bs, int *y) {
    int range = f->quant_q;
    int i, j, off;
    if (!mina_lbr_read(bs, 1)) return 0;
    y[0] = (int)mina_lbr_read(bs, (unsigned)mina_ilog((mina_u32)(range - 1)));
    y[1] = (int)mina_lbr_read(bs, (unsigned)mina_ilog((mina_u32)(range - 1)));
    off = 2;
    for (i = 0; i < f->partitions; i++) {
        int cls = f->partclass[i];
        int cdim = f->class_dim[cls];
        int cbits = f->class_sub[cls];
        int csub = (1 << cbits) - 1;
        int cval = 0;
        if (cbits > 0) {
            cval = (int)mina_vorbis_book_decode(&v->books[f->class_master[cls]], bs);
            if (cval < 0) return 0;
        }
        for (j = 0; j < cdim; j++) {
            int book = f->sub_books[cls * 8 + (cval & csub)];
            cval >>= cbits;
            if (book >= 0) {
                mina_i32 e = mina_vorbis_book_decode(&v->books[book], bs);
                if (e < 0) return 0;
                y[off++] = (int)e;
            } else {
                y[off++] = 0;
            }
        }
    }
    for (i = 2; i < f->values; i++) {
        int predicted = mina_vorbis_render_point(f->xlist[f->loneighbor[i - 2]],
                                                 f->xlist[f->hineighbor[i - 2]],
                                                 y[f->loneighbor[i - 2]],
                                                 y[f->hineighbor[i - 2]],
                                                 f->xlist[i]);
        int hiroom = range - predicted;
        int loroom = predicted;
        int room = (hiroom < loroom ? hiroom : loroom) << 1;
        int val = y[i];
        if (val) {
            if (val >= room) {
                if (hiroom > loroom) val = val - loroom;
                else val = -1 - (val - hiroom);
            } else {
                if (val & 1) val = -((val + 1) >> 1);
                else val >>= 1;
            }
            y[i] = (predicted + val) & 0x7FFF;
            y[f->loneighbor[i - 2]] &= 0x7FFF;
            y[f->hineighbor[i - 2]] &= 0x7FFF;
        } else {
            y[i] = predicted | 0x8000;
        }
    }
    return 1;
}

static void mina_vorbis_floor1_apply(const mina_vorbis_floor *f, const int *y,
                                     float *out, int n) {
    int j, hx = 0, lx = 0;
    int ly = y[0] * f->multiplier;
    if (ly < 0) ly = 0;
    if (ly > 255) ly = 255;
    for (j = 1; j < f->values; j++) {
        int current = f->forward_index[j];
        int hy = y[current] & 0x7FFF;
        if (hy == y[current]) {
            hx = f->xlist[current];
            hy *= f->multiplier;
            if (hy < 0) hy = 0;
            if (hy > 255) hy = 255;
            mina_vorbis_render_line(n, lx, hx, ly, hy, out);
            lx = hx; ly = hy;
        }
    }
    for (j = hx; j < n; j++) out[j] *= mina_vorbis_fromdb(ly);
}

/* floor 0: LSP coefficients decoded here, curve synthesised below.
 * Implemented from the Vorbis I specification; no released encoder emits
 * floor 0, so this path is unexercised by the reference test corpus. */
static int mina_vorbis_floor0_decode(const mina_vorbis_ctx *v,
                                     const mina_vorbis_floor *f,
                                     mina_lbr *bs, float *lsp, int *amp_out) {
    int amplitude = (int)mina_lbr_read(bs, (unsigned)f->amp_bits);
    int booknum, i, j = 0;
    const mina_vorbis_book *b;
    float last = 0.0f;
    if (amplitude <= 0) return 0;
    booknum = (int)mina_lbr_read(bs, (unsigned)mina_ilog((mina_u32)f->nbooks));
    if (booknum < 0 || booknum >= f->nbooks) return 0;
    b = &v->books[f->booklist[booknum]];
    if (b->lookup_type == 0 || !b->valuelist) return 0;
    while (j < f->order) {
        mina_i32 e = mina_vorbis_book_decode(b, bs);
        if (e < 0) return 0;
        for (i = 0; i < b->dim && j < f->order; i++, j++)
            lsp[j] = b->valuelist[(size_t)e * b->dim + i] + last;
        last = lsp[j - 1];
    }
    *amp_out = amplitude;
    return 1;
}

static void mina_vorbis_floor0_apply(const mina_vorbis_floor *f, const float *lsp,
                                     int amplitude, float *out, int n) {
    int i, j;
    double amp = (double)amplitude /
                 (double)(((mina_u32)1 << f->amp_bits) - 1u) * (double)f->amp_offset;
    double wscale = MINA_PI / (double)f->barkmap;
    int map_last = -1;
    double lastval = 0.0;
    for (j = 0; j < n; j++) {
        /* bark-scale map from the linear bin index */
        double fq = (double)j * (double)f->rate / (2.0 * (double)n);
        double bark = 13.1 * atan(0.00074 * fq) + 2.24 * atan(fq * fq * 1.85e-8)
                    + 1e-4 * fq;
        double bmax = 13.1 * atan(0.00074 * ((double)f->rate * 0.5))
                    + 2.24 * atan(((double)f->rate * 0.5) * ((double)f->rate * 0.5) * 1.85e-8)
                    + 1e-4 * ((double)f->rate * 0.5);
        int map = (int)(bark * (double)f->barkmap / bmax);
        if (map >= f->barkmap) map = f->barkmap - 1;
        if (map != map_last) {
            double w = cos(wscale * (double)map);
            double p = 0.25, q = 0.25;
            for (i = 0; i + 1 < f->order; i += 2) {
                double a = (double)lsp[i]     - w;
                double bq = (double)lsp[i + 1] - w;
                p *= a * a;
                q *= bq * bq;
            }
            if (f->order & 1) {
                double a = (double)lsp[f->order - 1] - w;
                q *= a * a;
                p *= (1.0 - w * w);
                p *= 4.0;
                q *= 4.0;
            } else {
                p *= (1.0 - w) * 4.0;
                q *= (1.0 + w) * 4.0;
            }
            {
                double den = sqrt(p + q);
                lastval = (den > 0.0) ? exp(0.11512925 * (amp / den - (double)f->amp_offset))
                                      : 0.0;
            }
            map_last = map;
        }
        out[j] *= (float)lastval;
    }
}

/* ---- residue ---- */
static void mina_vorbis_decodevs_add(const mina_vorbis_book *b, float *a,
                                     mina_lbr *bs, int n) {
    int step, i, k;
    if (!b->used || !b->valuelist || b->dim <= 0) return;
    step = n / b->dim;
    for (i = 0; i < step; i++) {
        mina_i32 entry = mina_vorbis_book_decode(b, bs);
        if (entry < 0) return;
        for (k = 0; k < b->dim; k++)
            a[i + k * step] += b->valuelist[(size_t)entry * b->dim + k];
    }
}

static void mina_vorbis_decodev_add(const mina_vorbis_book *b, float *a,
                                    mina_lbr *bs, int n) {
    int i = 0, j;
    if (!b->used || !b->valuelist || b->dim <= 0) return;
    while (i < n) {
        mina_i32 entry = mina_vorbis_book_decode(b, bs);
        if (entry < 0) return;
        for (j = 0; j < b->dim; j++)
            a[i++] += b->valuelist[(size_t)entry * b->dim + j];
    }
}

static void mina_vorbis_decodevv_add(const mina_vorbis_book *b, float **a,
                                     int offset, int ch, mina_lbr *bs, int n) {
    int i, m, chptr = 0, j;
    if (!b->used || !b->valuelist || b->dim <= 0 || ch <= 0) return;
    i = offset / ch;
    m = (offset + n) / ch;
    while (i < m) {
        mina_i32 entry = mina_vorbis_book_decode(b, bs);
        if (entry < 0) return;
        for (j = 0; j < b->dim; j++) {
            a[chptr++][i] += b->valuelist[(size_t)entry * b->dim + j];
            if (chptr == ch) { chptr = 0; i++; }
        }
    }
}

static void mina_vorbis_residue_decode(const mina_vorbis_ctx *v,
                                       const mina_vorbis_residue *r,
                                       mina_lbr *bs, float **in,
                                       const int *nonzero, int ch, int n2,
                                       int *classvec) {
    const mina_vorbis_book *cb = &v->books[r->classbook];
    int ppw = cb->dim;
    int parts = r->classifications;
    int type2 = (r->type == 2);
    int begin, end, n, partvals, partwords, s, l, k, j, i, used;
    float *compact[MINA_VORBIS_MAX_CH];

    if (ppw <= 0 || r->part_size <= 0) return;

    if (type2) {
        int max = n2 * ch;
        for (i = 0; i < ch; i++) if (nonzero[i]) break;
        if (i == ch) return;
        begin = r->begin < max ? r->begin : max;
        end   = r->end   < max ? r->end   : max;
        used = ch;
    } else {
        used = 0;
        for (i = 0; i < ch; i++) if (nonzero[i]) compact[used++] = in[i];
        if (!used) return;
        in = compact;
        begin = r->begin < n2 ? r->begin : n2;
        end   = r->end   < n2 ? r->end   : n2;
    }
    n = end - begin;
    if (n <= 0) return;
    partvals = n / r->part_size;
    if (partvals <= 0) return;
    partwords = (partvals + ppw - 1) / ppw;

    for (s = 0; s < 8; s++) {
        i = 0;
        for (l = 0; l < partwords && i < partvals; l++) {
            if (s == 0) {
                int lanes = type2 ? 1 : used;
                for (j = 0; j < lanes; j++) {
                    mina_i32 e = mina_vorbis_book_decode(cb, bs);
                    long val, mult = 1;
                    int kk;
                    if (e < 0) return;
                    val = (long)e;
                    for (kk = 0; kk < ppw; kk++) mult *= parts;
                    if (val >= mult) return;
                    mult /= parts;
                    for (kk = 0; kk < ppw; kk++) {
                        classvec[(j * partwords + l) * ppw + kk] =
                            (int)((mult > 0) ? (val / mult) % parts : 0);
                        if (mult > 0) { val %= mult; if (mult > 1) mult /= parts; }
                    }
                }
            }
            for (k = 0; k < ppw && i < partvals; k++, i++) {
                int offset = begin + i * r->part_size;
                if (type2) {
                    int cls = classvec[l * ppw + k];
                    if (cls < 0 || cls >= parts) continue;
                    if (r->cascade[cls] & ((mina_u32)1 << s)) {
                        int book = r->books[cls * 8 + s];
                        if (book >= 0)
                            mina_vorbis_decodevv_add(&v->books[book], in, offset,
                                                     ch, bs, r->part_size);
                    }
                } else {
                    for (j = 0; j < used; j++) {
                        int cls = classvec[(j * partwords + l) * ppw + k];
                        if (cls < 0 || cls >= parts) continue;
                        if (r->cascade[cls] & ((mina_u32)1 << s)) {
                            int book = r->books[cls * 8 + s];
                            if (book < 0) continue;
                            if (r->type == 1)
                                mina_vorbis_decodev_add(&v->books[book],
                                                        in[j] + offset, bs,
                                                        r->part_size);
                            else
                                mina_vorbis_decodevs_add(&v->books[book],
                                                         in[j] + offset, bs,
                                                         r->part_size);
                        }
                    }
                }
                if (bs->eof) return;
            }
        }
    }
}

/* ---- packet decode ----
 * Returns the number of frames written to out, or -1 on a bad packet. */
static int mina_vorbis_decode_packet(mina_vorbis_ctx *v, mina_lbr *bs,
                                     float *out, int *classvec, float *lspbuf) {
    int mode_num, W, n, n2, ch, i;
    mina_vorbis_mapping *m;
    int nonzero[MINA_VORBIS_MAX_CH];     /* dirtied by coupling; drives residue */
    int have_floor[MINA_VORBIS_MAX_CH];  /* the floor actually decoded a curve  */
    int floor_amp[MINA_VORBIS_MAX_CH];
    int n0 = v->blocksize[0] >> 1;
    int n1 = v->blocksize[1] >> 1;
    int thisCenter, prevCenter, lW, ret;

    if (mina_lbr_bit(bs) != 0) return -1;              /* not an audio packet */
    mode_num = (int)mina_lbr_read(bs, (unsigned)v->modebits);
    if (bs->eof || mode_num < 0 || mode_num >= v->nmodes) return -1;
    W = v->modes[mode_num].blockflag;
    m = &v->mappings[v->modes[mode_num].mapping];
    n = v->blocksize[W];
    n2 = n >> 1;
    if (W) { mina_lbr_read(bs, 1); mina_lbr_read(bs, 1); }  /* lW, nW: unused */

    /* the overlap case is decided by the previous block's size, exactly as
     * the reference decoder does (v->lW = v->W) */
    lW = v->prevW;

    for (ch = 0; ch < v->channels; ch++)
        memset(v->work[ch], 0, (size_t)n2 * sizeof(float));

    /* 1. floor */
    for (ch = 0; ch < v->channels; ch++) {
        int fi = m->sub_floor[m->ch_mux[ch]];
        const mina_vorbis_floor *f = &v->floors[fi];
        int *y = v->floormem + (size_t)ch * v->posts_max;
        floor_amp[ch] = 0;
        if (f->type == 1)
            have_floor[ch] = mina_vorbis_floor1_decode(v, f, bs, y);
        else
            have_floor[ch] = mina_vorbis_floor0_decode(v, f, bs,
                                 lspbuf + (size_t)ch * 256, &floor_amp[ch]);
        nonzero[ch] = have_floor[ch];
    }
    if (bs->eof) return -1;

    /* 2. coupling dirties the non-zero list */
    for (i = 0; i < m->coupling_steps; i++) {
        if (nonzero[m->couple_mag[i]] || nonzero[m->couple_ang[i]]) {
            nonzero[m->couple_mag[i]] = 1;
            nonzero[m->couple_ang[i]] = 1;
        }
    }

    /* 3. residue, per submap, over every channel of that submap */
    for (i = 0; i < m->submaps; i++) {
        float *bundle[MINA_VORBIS_MAX_CH];
        int    bnz[MINA_VORBIS_MAX_CH];
        int    nb = 0, c;
        for (c = 0; c < v->channels; c++) {
            if (m->ch_mux[c] != i) continue;
            bnz[nb] = nonzero[c];
            bundle[nb++] = v->work[c];
        }
        if (!nb) continue;
        mina_vorbis_residue_decode(v, &v->residues[m->sub_residue[i]], bs,
                                   bundle, bnz, nb, n2, classvec);
    }

    /* 4. inverse coupling, in reverse order */
    for (i = m->coupling_steps - 1; i >= 0; i--) {
        float *pcmM = v->work[m->couple_mag[i]];
        float *pcmA = v->work[m->couple_ang[i]];
        int k;
        for (k = 0; k < n2; k++) {
            float M = pcmM[k], A = pcmA[k];
            if (M > 0) {
                if (A > 0) { pcmM[k] = M; pcmA[k] = M - A; }
                else       { pcmA[k] = M; pcmM[k] = M + A; }
            } else {
                if (A > 0) { pcmM[k] = M; pcmA[k] = M + A; }
                else       { pcmA[k] = M; pcmM[k] = M - A; }
            }
        }
    }

    /* 5. spectral envelope. Coupling may have marked a channel non-zero for
     * the residue stage, but a channel whose floor decoded to zero is still
     * silenced here - exactly as floor1_inverse2 does with a NULL memo. */
    for (ch = 0; ch < v->channels; ch++) {
        int fi = m->sub_floor[m->ch_mux[ch]];
        const mina_vorbis_floor *f = &v->floors[fi];
        if (!have_floor[ch]) {
            memset(v->work[ch], 0, (size_t)n2 * sizeof(float));
            continue;
        }
        if (f->type == 1)
            mina_vorbis_floor1_apply(f, v->floormem + (size_t)ch * v->posts_max,
                                     v->work[ch], n2);
        else
            mina_vorbis_floor0_apply(f, lspbuf + (size_t)ch * 256, floor_amp[ch],
                                     v->work[ch], n2);
    }

    /* 6. IMDCT, window, overlap-add (the reference block.c layout) */
    thisCenter = v->centerW ? n1 : 0;
    prevCenter = v->centerW ? 0 : n1;
    for (ch = 0; ch < v->channels; ch++) {
        float *t = v->tbuf;
        float *ob = v->obuf + (size_t)ch * v->blocksize[1];
        int k;
        mina_mdct_backward(&v->mdct[W], v->work[ch], t);
        if (lW) {
            if (W) {                                   /* large / large */
                float *pcm = ob + prevCenter;
                for (k = 0; k < n1; k++)
                    pcm[k] = pcm[k] * v->w1[n1 - k - 1] + t[k] * v->w1[k];
            } else {                                   /* large / small */
                float *pcm = ob + prevCenter + n1 / 2 - n0 / 2;
                for (k = 0; k < n0; k++)
                    pcm[k] = pcm[k] * v->w0[n0 - k - 1] + t[k] * v->w0[k];
            }
        } else {
            if (W) {                                   /* small / large */
                float *pcm = ob + prevCenter;
                float *p = t + n1 / 2 - n0 / 2;
                for (k = 0; k < n0; k++)
                    pcm[k] = pcm[k] * v->w0[n0 - k - 1] + p[k] * v->w0[k];
                for (; k < n1 / 2 + n0 / 2; k++)
                    pcm[k] = p[k];
            } else {                                   /* small / small */
                float *pcm = ob + prevCenter;
                for (k = 0; k < n0; k++)
                    pcm[k] = pcm[k] * v->w0[n0 - k - 1] + t[k] * v->w0[k];
            }
        }
        {   /* the copy section */
            float *pcm = ob + thisCenter;
            for (k = 0; k < n2; k++) pcm[k] = t[n2 + k];
        }
    }

    if (!v->started) {
        ret = 0;
        v->started = 1;
    } else {
        ret = v->blocksize[lW] / 4 + v->blocksize[W] / 4;
        for (ch = 0; ch < v->channels; ch++) {
            const float *ob = v->obuf + (size_t)ch * v->blocksize[1] + prevCenter;
            int k;
            for (k = 0; k < ret; k++)
                out[(size_t)k * v->channels + ch] = ob[k];
        }
    }

    v->centerW = !v->centerW;
    v->prevW = W;
    return ret;
}

static mina_result mina_vorbis_decode(const mina_u8 *d, size_t n, mina_pcm *out) {
    mina_ogg_packets ps;
    mina_vorbis_ctx *v = NULL;
    mina_fbuf fb;
    char codec[16];
    mina_u32 ch0 = 0, sr0 = 0, serial = 0;
    size_t i;
    int *classvec = NULL;
    float *lspbuf = NULL;
    mina_result status = MINA_OK;
    mina_u64 granulepos, sample_count;
    int have_granulepos = 0;
    size_t head_trim = 0, tail_keep = 0;
    int have_tail = 0;

    if (mina_ogg_scan(d, n, codec, sizeof(codec), &ch0, &sr0, &serial) != MINA_OK)
        return MINA_ERR_NOTFOUND;
    if (strcmp(codec, "vorbis") != 0) return MINA_ERR_UNSUPPORTED;

    if (!mina_ogg_collect(d, n, serial, &ps)) {
        mina_ogg_packets_free(&ps);
        return ps.oom ? MINA_ERR_NOMEM : MINA_ERR_INVALID;
    }
    if (ps.npkt < 3) { mina_ogg_packets_free(&ps); return MINA_ERR_INVALID; }

    v = mina_vorbis_setup(ps.arena + ps.pkt[0].offset, ps.pkt[0].len,
                          ps.arena + ps.pkt[2].offset, ps.pkt[2].len);
    if (!v) { mina_ogg_packets_free(&ps); return MINA_ERR_INVALID; }

    /* worst-case classification vector: one entry per partition per channel */
    {
        int maxpw = 0, k;
        for (k = 0; k < v->nres; k++) {
            int ppw = v->books[v->residues[k].classbook].dim;
            int partvals, words;
            if (ppw < 1) ppw = 1;
            partvals = (v->blocksize[1] / 2 * v->channels) /
                       (v->residues[k].part_size > 0 ? v->residues[k].part_size : 1);
            words = (partvals + ppw) * ppw;
            if (words > maxpw) maxpw = words;
        }
        classvec = (int *)MINA_MALLOC((size_t)(maxpw + 64) *
                                      (size_t)v->channels * sizeof(int));
        lspbuf = (float *)MINA_MALLOC((size_t)v->channels * 256 * sizeof(float));
        if (!classvec || !lspbuf) {
            MINA_FREE(classvec); MINA_FREE(lspbuf);
            mina_vorbis_free(v); mina_ogg_packets_free(&ps);
            return MINA_ERR_NOMEM;
        }
    }

    mina_fbuf_init(&fb);
    granulepos = mina_u64_from(0);
    sample_count = mina_u64_from(0);

    for (i = 3; i < ps.npkt; i++) {
        const mina_u8 *pk = ps.arena + ps.pkt[i].offset;
        size_t plen = ps.pkt[i].len;
        mina_lbr bs;
        float *dst;
        int got;
        if (plen == 0) continue;
        if (pk[0] & 1) continue;                 /* header packet in-stream */

        dst = mina_fbuf_extend(&fb, (size_t)v->blocksize[1] * (size_t)v->channels);
        if (!dst) break;
        mina_lbr_init(&bs, pk, plen);
        got = mina_vorbis_decode_packet(v, &bs, dst, classvec, lspbuf);
        if (got < 0) {
            fb.len -= (size_t)v->blocksize[1] * (size_t)v->channels;
            status = MINA_ERR_TRUNCATED;
            continue;
        }
        fb.len -= ((size_t)v->blocksize[1] - (size_t)got) * (size_t)v->channels;

        /* granule bookkeeping, following the reference decoder */
        sample_count = mina_u64_add(sample_count, mina_u64_from((mina_u32)got));
        if (!have_granulepos) {
            if (ps.pkt[i].has_granule) {
                granulepos = ps.pkt[i].granule;
                have_granulepos = 1;
                if (mina_u64_gt(sample_count, granulepos)) {
                    mina_u32 extra = mina_u64_lo(mina_u64_sub(sample_count, granulepos));
                    if (i + 1 >= ps.npkt) {          /* first and last page */
                        tail_keep = mina_u64_size(granulepos, NULL);
                        have_tail = 1;
                    } else {
                        head_trim = extra;
                    }
                }
            }
        } else {
            granulepos = mina_u64_add(granulepos, mina_u64_from((mina_u32)got));
            if (ps.pkt[i].has_granule && !mina_u64_eq(granulepos, ps.pkt[i].granule)) {
                if (mina_u64_gt(granulepos, ps.pkt[i].granule)) {
                    int ovf = 0;
                    size_t keep = mina_u64_size(ps.pkt[i].granule, &ovf);
                    if (!ovf) {
                        size_t produced = fb.len / v->channels;
                        tail_keep = (keep > head_trim) ? keep - head_trim : 0;
                        if (tail_keep > produced - (head_trim < produced ? head_trim : produced))
                            tail_keep = produced - (head_trim < produced ? head_trim : produced);
                        have_tail = 1;
                    }
                }
                granulepos = ps.pkt[i].granule;
            }
        }
    }

    if (ps.truncated && status == MINA_OK) status = MINA_ERR_TRUNCATED;

    /* apply the leading and trailing trims */
    if (fb.data && v->channels) {
        size_t total = fb.len / (size_t)v->channels;
        size_t head = head_trim < total ? head_trim : total;
        size_t avail = total - head;
        if (have_tail && tail_keep < avail) avail = tail_keep;
        if (head)
            memmove(fb.data, fb.data + head * (size_t)v->channels,
                    avail * (size_t)v->channels * sizeof(float));
        fb.len = avail * (size_t)v->channels;
    }

    {
        mina_u32 rch = (mina_u32)v->channels, rsr = (mina_u32)v->sample_rate;
        MINA_FREE(classvec);
        MINA_FREE(lspbuf);
        mina_vorbis_free(v);
        mina_ogg_packets_free(&ps);
        if (fb.len == 0 && status != MINA_OK) { mina_fbuf_free(&fb); return MINA_ERR_INVALID; }
        return mina_fbuf_finish(&fb, out, rch, rsr, status);
    }
}

#endif /* MINA_NO_VORBIS */

/* Dispatch an Ogg stream to the right decoder. */
static mina_result mina_ogg_decode(const mina_u8 *d, size_t n, mina_pcm *out) {
    char codec[16];
    mina_u32 ch = 0, sr = 0, serial = 0;
#ifdef MINA_NO_VORBIS
    (void)out;
#endif
    if (mina_ogg_scan(d, n, codec, sizeof(codec), &ch, &sr, &serial) != MINA_OK)
        return MINA_ERR_NOTFOUND;
#ifndef MINA_NO_VORBIS
    if (strcmp(codec, "vorbis") == 0) return mina_vorbis_decode(d, n, out);
#endif
    return MINA_ERR_UNSUPPORTED;
}

/* Automatically generated from the AAC codebook definitions.
   Standardised MPEG-4 AAC Huffman codebooks (ISO 14496-3, table 4.6.5). */
/* codebook hcb1 (quad, 2-step) root=32 second=113 */
static const mina_u16 aac_hcb1_r_off[32] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,13,17,21,25,29,33,49
};
static const mina_u8 aac_hcb1_r_xb[32] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,4,6
};
static const mina_u8 aac_hcb1_bits[113] = {
1,5,5,5,5,5,5,5,5,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11
};
static const mina_i8 aac_hcb1_x[113] = {
0,1,-1,0,0,0,0,0,0,1,-1,0,0,0,0,1,0,-1,0,1,0,-1,0,1,0,0,0,-1,1,-1,1,-1,0,1,-1,1,0,0,0,0,1,1,0,-1,-1,-1,0,1,1,-1,-1,-1,-1,-1,-1,-1,-1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,-1,-1,1,1,1,-1,-1,1,1,-1,1,-1,-1,1,1,-1,-1,-1,1,1
};
static const mina_i8 aac_hcb1_y[113] = {
0,0,0,0,1,0,0,0,-1,-1,1,0,1,-1,0,1,0,-1,-1,0,1,0,0,0,-1,1,1,0,0,0,0,0,-1,1,1,-1,1,1,-1,-1,-1,0,1,1,0,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,-1,-1,-1,-1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,-1,-1,0,0,1,1,0,0,0,0,-1,-1,0,0,0,0,-1,1,1,-1,1,-1,1,-1,-1,1,-1,1,1,-1,-1,1
};
static const mina_i8 aac_hcb1_v[113] = {
0,0,0,0,0,0,-1,1,0,0,0,-1,-1,1,1,0,-1,0,-1,-1,0,1,1,1,0,1,0,-1,0,0,0,0,0,-1,-1,1,1,-1,1,1,-1,-1,-1,1,1,1,-1,0,0,0,0,0,0,-1,-1,-1,-1,-1,-1,-1,-1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,-1,-1,0,0,-1,-1,-1,-1,0,0,1,1,1,1,1,-1,1,-1,1,1,-1,1,-1,-1,1,1,-1,-1,-1,1
};
static const mina_i8 aac_hcb1_w[113] = {
0,0,0,-1,0,1,0,0,0,0,0,1,0,0,-1,0,-1,0,0,0,-1,0,1,0,1,0,1,0,1,-1,-1,1,-1,0,0,0,-1,1,1,-1,0,1,-1,0,-1,0,1,1,-1,-1,-1,-1,-1,0,0,0,0,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,-1,-1,-1,-1,-1,-1,1,1,-1,-1,1,1,1,1,-1,1,-1,1,1,1,-1,-1,-1,1,1,1,-1,1,-1,-1
};
/* codebook hcb2 (quad, 2-step) root=32 second=85 */
static const mina_u16 aac_hcb2_r_off[32] = {
0,0,0,0,1,1,2,3,4,5,6,7,8,9,11,13,15,17,19,21,23,25,27,29,31,33,37,41,45,53,61,69
};
static const mina_u8 aac_hcb2_r_xb[32] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,3,3,3,4
};
static const mina_u8 aac_hcb2_bits[85] = {
3,4,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9
};
static const mina_i8 aac_hcb2_x[85] = {
0,1,-1,0,0,0,0,0,0,0,-1,0,0,0,0,-1,1,1,-1,0,1,1,0,-1,0,0,-1,0,-1,1,0,0,1,0,1,-1,0,1,1,1,-1,0,1,-1,-1,-1,-1,1,1,1,1,1,-1,-1,-1,-1,-1,0,1,1,0,0,-1,-1,0,1,0,1,1,-1,-1,1,-1,-1,-1,1,-1,1,-1,-1,1,1,-1,1,1
};
static const mina_i8 aac_hcb2_y[85] = {
0,0,0,0,0,0,-1,0,1,-1,1,1,0,1,0,0,-1,0,-1,0,0,0,-1,0,1,-1,0,-1,0,1,1,0,0,1,0,1,-1,-1,1,0,1,-1,1,0,-1,0,0,-1,-1,1,1,-1,1,-1,0,-1,-1,-1,0,0,1,1,1,0,1,-1,-1,1,-1,1,1,-1,-1,1,1,1,-1,-1,1,-1,1,-1,-1,1,1
};
static const mina_i8 aac_hcb2_v[85] = {
0,0,0,0,-1,0,0,1,0,1,0,-1,1,0,-1,0,0,-1,0,-1,1,0,0,1,0,-1,0,0,-1,0,1,1,0,-1,-1,-1,1,1,0,1,1,-1,1,1,-1,-1,-1,-1,-1,-1,-1,0,0,1,1,0,0,-1,1,-1,-1,1,0,-1,1,0,1,0,1,-1,-1,-1,-1,1,1,1,1,1,-1,1,-1,-1,-1,-1,1
};
static const mina_i8 aac_hcb2_w[85] = {
0,0,0,1,0,-1,0,0,0,0,0,0,-1,-1,1,-1,0,0,0,-1,0,1,1,0,1,0,1,-1,0,0,0,1,-1,1,1,0,-1,0,-1,1,0,1,0,-1,0,1,1,0,0,0,0,1,-1,0,1,1,-1,-1,-1,-1,-1,1,1,-1,-1,-1,1,1,-1,1,1,1,-1,-1,1,1,-1,1,-1,1,-1,-1,1,1,-1
};
/* codebook hcb4 (quad, 2-step) root=32 second=184 */
static const mina_u16 aac_hcb4_r_off[32] = {
0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,11,12,13,14,15,16,20,24,32,40,56
};
static const mina_u8 aac_hcb4_r_xb[32] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,3,3,4,7
};
static const mina_u8 aac_hcb4_bits[184] = {
4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,12,12
};
static const mina_i8 aac_hcb4_x[184] = {
1,0,1,1,1,1,1,0,0,1,1,0,0,0,0,0,2,1,1,1,2,2,1,2,0,0,0,1,0,1,1,1,1,2,2,1,2,0,0,0,0,0,0,0,1,1,0,0,1,1,2,2,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,1,1,1,1,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,1,1,1,1,2,2,2,2,1,1,1,1,2,2,2,2,2,2,2,2,1,1,1,1,2,2,2,2,0,0,2,2,0,0,2,2,0,0,2,2,2,2,0,0,2,2,2,2
};
static const mina_i8 aac_hcb4_y[184] = {
1,1,1,1,0,0,1,0,0,0,0,1,0,1,0,1,1,1,2,1,1,1,2,0,1,1,1,1,2,0,2,1,0,1,0,2,0,1,2,0,1,1,2,2,0,0,0,0,0,0,0,0,0,0,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,1,1,1,1,2,2,2,2,1,1,1,1,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,1,1,1,1,2,2,2,2,1,1,2,2,2,2,2,2,0,0,0,0,2,2,0,0,2,2,2,2,2,2,2,0
};
static const mina_i8 aac_hcb4_v[184] = {
1,1,0,1,1,0,0,0,1,1,0,1,0,0,1,0,1,2,1,1,1,0,1,1,2,2,1,2,1,1,0,0,2,0,1,0,0,0,1,1,2,2,0,0,0,0,2,2,2,2,0,0,0,0,0,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,2,2,2,2,2,2,1,1,2,2,0,0,2,2,2,2,0,0,0,0,2,2,2,2,2,2,0,2
};
static const mina_i8 aac_hcb4_w[184] = {
1,1,1,0,1,0,0,0,1,0,1,0,1,1,0,0,1,1,1,2,0,1,0,1,1,1,2,0,1,2,1,2,1,0,0,0,1,2,0,2,0,0,1,1,2,2,1,1,0,0,0,0,2,2,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,1,1,1,1,2,2,2,2,1,1,1,1,2,2,2,2,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,2,2,0,0,2,2,2,2,2,2,2,2,0,0,2,2
};
/* codebook hcb6 (pair, 2-step) root=32 second=125 */
static const mina_u16 aac_hcb6_r_off[32] = {
0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,11,13,15,17,19,21,23,25,29,33,37,45,61
};
static const mina_u8 aac_hcb6_r_xb[32] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,3,4,6
};
static const mina_u8 aac_hcb6_bits[125] = {
4,4,4,4,4,4,4,4,4,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11
};
static const mina_i8 aac_hcb6_x[125] = {
0,1,0,0,-1,1,-1,1,-1,2,2,-2,-2,-2,-1,2,1,1,0,-1,0,2,-2,-2,2,-3,3,3,-1,-3,1,1,-1,3,-3,0,0,3,3,-3,-2,2,3,2,-2,-3,-3,3,3,3,-3,-3,1,-1,4,-4,-4,1,4,-1,0,-4,-4,-4,-4,-4,-4,-4,-4,2,2,2,2,-2,-2,-2,-2,-4,-4,-4,-4,4,4,4,4,4,4,4,4,-2,-2,-2,-2,4,4,4,4,2,2,2,2,0,0,0,0,-3,-3,-3,-3,3,3,4,4,3,3,4,4,-4,-4,-4,-4,4,-4,-4,4
};
static const mina_i8 aac_hcb6_y[125] = {
0,0,-1,1,0,1,1,-1,-1,-1,1,1,-1,0,2,0,-2,2,-2,-2,2,-2,2,-2,2,1,1,-1,3,-1,3,-3,-3,0,0,-3,3,2,2,-2,3,3,-2,-3,-3,2,2,3,3,-3,-3,3,-4,-4,1,1,-1,4,-1,4,-4,2,2,2,2,-2,-2,-2,-2,4,4,4,4,-4,-4,-4,-4,0,0,0,0,2,2,2,2,-2,-2,-2,-2,4,4,4,4,0,0,0,0,-4,-4,-4,-4,4,4,4,4,-4,-4,4,4,-4,-4,-3,-3,4,4,3,3,3,3,-3,-3,4,4,-4,-4
};
/* codebook hcb8 (pair, 2-step) root=32 second=83 */
static const mina_u16 aac_hcb8_r_off[32] = {
0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,7,8,9,10,11,12,13,15,17,19,21,23,27,31,35,43,51
};
static const mina_u8 aac_hcb8_r_xb[32] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,2,2,2,3,3,5
};
static const mina_u8 aac_hcb8_bits[83] = {
3,4,4,4,4,4,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,10,10,10,10
};
static const mina_i8 aac_hcb8_x[83] = {
1,2,1,1,0,2,0,2,0,3,1,3,2,3,4,1,4,2,3,0,4,3,5,5,2,1,5,3,4,5,0,4,4,2,6,6,6,1,1,3,6,5,5,6,0,4,7,7,2,6,7,1,1,1,1,5,5,5,5,3,3,3,3,6,6,7,7,6,6,4,4,0,0,7,7,7,7,6,6,5,7,0,7
};
static const mina_i8 aac_hcb8_y[83] = {
1,1,0,2,1,2,0,0,2,1,3,2,3,3,1,4,2,4,0,3,3,4,2,1,5,5,3,5,4,4,4,5,0,6,2,1,1,6,6,6,3,5,0,4,5,6,1,2,7,5,3,7,7,7,7,6,6,6,6,7,7,7,7,6,6,4,4,0,0,7,7,6,6,5,5,6,6,7,7,7,0,7,7
};
/* codebook hcb10 (pair, 2-step) root=64 second=209 */
static const mina_u16 aac_hcb10_r_off[64] = {
0,0,0,0,1,1,1,1,2,2,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,27,29,31,33,35,37,39,41,45,49,53,57,61,65,73,81,89,97,113,129,145
};
static const mina_u8 aac_hcb10_r_xb[64] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,4,4,4,6
};
static const mina_u8 aac_hcb10_bits[209] = {
4,4,4,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12
};
static const mina_i8 aac_hcb10_x[209] = {
1,1,2,2,1,0,1,3,3,2,3,2,0,2,4,1,4,0,4,3,3,0,4,2,5,1,5,5,3,5,4,6,2,6,4,6,0,1,3,5,6,4,4,6,7,3,2,5,8,7,5,7,0,8,1,8,7,4,2,6,7,1,3,8,4,5,5,8,8,5,5,7,6,9,6,6,9,3,9,2,0,8,9,4,10,1,7,8,9,7,10,5,10,2,10,3,9,6,6,8,8,4,4,7,7,11,11,7,11,10,1,11,9,0,8,10,3,5,8,11,0,11,2,7,6,10,4,1,12,9,12,11,5,12,11,12,3,6,9,10,10,12,0,4,9,12,12,12,12,2,2,2,2,8,8,8,8,9,9,1,1,11,11,12,12,7,7,5,5,6,6,10,10,8,8,12,12,0,0,7,7,11,11,10,10,11,11,11,11,0,0,11,11,9,9,10,10,12,12,8,8,12,10,9,11,12,0,12,12
};
static const mina_i8 aac_hcb10_y[209] = {
1,2,1,2,0,1,3,2,1,3,3,0,2,4,2,4,1,0,3,4,0,3,4,5,2,5,1,3,5,4,5,2,6,3,0,1,4,6,6,5,4,6,6,5,2,7,7,6,2,3,0,1,5,1,7,3,4,7,8,6,5,8,8,4,8,7,7,5,5,8,8,6,7,2,0,8,3,9,1,9,6,6,4,9,2,9,7,7,5,8,3,9,4,10,1,10,6,9,9,0,0,10,10,0,0,2,2,9,3,6,10,1,7,7,8,5,11,10,9,5,8,4,11,10,10,7,11,11,2,8,3,6,11,4,7,5,12,11,0,8,0,1,9,12,9,6,6,6,6,12,12,12,12,10,10,10,10,10,10,12,12,8,8,7,7,11,11,12,12,12,12,9,9,11,11,8,8,10,10,12,12,0,0,10,10,9,9,10,10,11,11,11,11,11,11,11,11,0,0,12,12,9,12,12,12,11,12,10,12
};
/* codebook hcb11 (pair, 2-step) root=32 second=374 */
static const mina_u16 aac_hcb11_r_off[32] = {
0,0,1,1,2,3,4,5,6,7,8,10,12,14,18,22,26,30,38,46,54,62,70,78,86,102,118,134,150,182,214,246
};
static const mina_u8 aac_hcb11_r_xb[32] = {
0,0,0,0,0,0,0,0,0,0,1,1,1,2,2,2,2,3,3,3,3,3,3,3,4,4,4,4,5,5,5,7
};
static const mina_u8 aac_hcb11_bits[374] = {
4,4,5,5,5,5,5,5,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,12,12,12,12,12,12
};
static const mina_i8 aac_hcb11_x[374] = {
0,1,16,1,0,2,1,2,1,3,3,2,2,0,3,3,4,1,4,2,4,3,3,0,5,5,2,4,1,5,3,3,5,5,4,6,2,6,6,3,1,4,3,16,16,16,6,16,4,4,0,2,5,5,16,16,16,2,7,3,6,5,6,16,7,7,16,7,1,1,4,16,7,16,8,16,6,9,2,5,10,16,8,8,3,5,16,16,11,11,7,7,4,4,6,6,7,7,0,0,8,16,12,1,8,14,5,13,3,8,7,2,8,9,9,15,4,6,6,9,5,8,7,1,10,0,10,9,9,4,2,9,3,6,10,8,10,9,11,1,7,10,7,3,5,10,4,11,13,6,13,13,2,2,16,16,5,5,11,11,11,9,7,8,0,4,0,3,11,13,13,12,2,13,8,6,10,10,14,12,1,4,11,3,1,12,7,3,5,5,14,4,11,14,12,13,12,8,11,2,9,14,6,10,15,8,9,14,10,5,11,14,2,6,1,13,0,13,7,12,7,15,12,6,2,15,15,1,9,4,14,8,13,8,5,3,10,11,12,15,15,8,15,7,9,0,9,9,9,9,9,9,9,9,12,12,12,12,14,14,14,14,10,10,10,10,14,14,14,14,12,12,12,12,6,6,6,6,7,7,7,7,9,9,15,15,11,11,11,11,1,1,10,10,10,10,13,13,13,13,11,11,11,11,8,8,14,14,13,13,12,12,15,15,14,14,10,10,12,12,9,9,0,0,12,12,11,11,12,12,10,10,13,13,0,0,14,14,15,15,15,15,11,11,14,14,13,13,0,0,13,13,15,15,15,15,12,12,14,14,14,14,13,13,12,12,14,14,0,0,15,15,0,15
};
static const mina_i8 aac_hcb11_y[374] = {
0,1,16,0,1,1,2,2,3,1,2,0,3,2,3,3,1,4,2,4,3,4,0,3,1,2,5,4,5,3,5,5,4,4,5,2,6,1,3,6,6,16,16,5,3,4,4,6,0,6,4,16,5,16,7,2,8,7,2,7,5,6,16,10,3,1,9,16,16,7,7,11,4,12,16,1,6,16,8,7,16,13,3,2,8,0,14,14,16,16,5,5,8,8,7,7,6,6,5,5,4,15,16,8,1,16,8,16,9,5,7,9,6,2,3,16,9,8,0,4,9,7,8,9,3,6,2,1,5,10,10,6,10,9,4,8,5,7,3,10,0,6,9,11,10,1,11,2,2,10,3,3,11,11,0,0,11,11,5,5,4,8,10,9,16,13,7,13,6,1,4,3,13,5,10,11,8,7,2,4,11,12,1,12,13,2,11,14,12,13,4,14,7,3,5,6,6,0,8,12,9,5,13,10,2,11,10,6,9,14,9,1,14,12,12,8,8,7,12,7,13,3,1,14,15,5,4,14,11,15,7,13,9,12,15,15,11,10,8,6,7,14,1,14,0,9,13,13,13,13,12,12,12,12,9,9,9,9,8,8,8,8,13,13,13,13,9,9,9,9,10,10,10,10,15,15,15,15,15,15,15,15,14,14,8,8,11,11,14,14,15,15,12,12,14,14,11,11,10,10,13,13,12,12,15,15,11,11,12,12,13,13,9,9,10,10,0,0,11,11,15,15,10,10,12,12,0,0,14,14,15,15,13,13,13,13,12,12,10,10,11,11,15,15,13,13,0,0,11,11,14,14,12,12,13,13,15,15,0,0,14,14,15,15,0,0,15,15,14,12,14,0,15,15
};
/* codebook hcb3 (binary) entries=161 */
static const mina_u8 aac_hcb3_leaf[161] = {
0,1,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,1,1,1,1,1,0,1,1
};
static const mina_i8 aac_hcb3_d0[161] = {
1,0,1,2,3,4,5,6,7,1,0,0,0,4,5,6,7,1,0,6,7,8,9,10,11,0,0,1,0,1,1,6,7,8,9,10,11,1,1,1,9,10,11,12,13,14,15,16,17,2,0,0,2,1,13,14,15,16,17,18,19,20,21,22,23,24,25,0,0,1,0,2,0,0,0,0,0,2,1,0,1,1,11,12,13,14,15,16,17,18,19,20,21,1,1,1,0,2,1,2,1,0,0,2,1,1,2,0,7,8,9,10,11,12,13,2,1,1,2,1,2,0,0,6,7,8,9,10,11,1,2,2,2,0,2,2,1,1,3,4,5,0,2,1,3,4,5,2,2,2,3,4,5,2,2,2,2,2,1,2,2
};
static const mina_i8 aac_hcb3_d1[161] = {
2,0,2,3,4,5,6,7,8,0,0,1,0,5,6,7,8,1,0,7,8,9,10,11,12,1,1,0,1,0,1,7,8,9,10,11,12,1,0,1,10,11,12,13,14,15,16,17,18,0,0,0,1,2,14,15,16,17,18,19,20,21,22,23,24,25,26,0,1,2,1,1,0,2,1,2,1,0,2,2,1,1,12,13,14,15,16,17,18,19,20,21,22,2,0,0,2,1,1,1,0,0,1,2,2,0,0,2,8,9,10,11,12,13,14,2,2,1,0,1,2,2,2,7,8,9,10,11,12,0,2,1,2,2,2,1,2,2,4,5,6,2,0,2,4,5,6,0,1,1,4,5,6,2,2,1,0,0,2,2,0
};
static const mina_i8 aac_hcb3_d2[161] = {
0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,1,0,1,1,0,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,0,1,1,2,1,2,0,0,1,1,1,2,2,0,0,0,0,0,0,0,0,0,0,0,0,2,2,0,1,1,0,1,2,2,1,2,0,0,2,0,0,0,0,0,0,0,0,2,0,1,2,1,2,1,0,0,0,0,0,0,2,0,2,2,2,2,2,1,2,0,0,0,0,2,0,0,0,0,2,1,0,0,0,0,2,1,2,1,0,0,0,2
};
static const mina_i8 aac_hcb3_d3[161] = {
0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,1,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,2,0,0,0,0,0,2,0,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,2,1,2,2,2,0,0,2,1,1,0,0,0,0,0,0,0,0,1,2,1,2,1,0,2,0,0,0,0,0,0,2,1,0,0,2,1,1,2,2,0,0,0,2,0,2,0,0,0,1,2,2,0,0,0,2,2,2,2,2,0,2,2
};
/* codebook hcb5 (binary) entries=161 */
static const mina_u8 aac_hcb5_leaf[161] = {
0,1,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1
};
static const mina_i8 aac_hcb5_d0[161] = {
1,0,1,2,3,4,5,6,7,-1,1,0,0,4,5,6,7,1,-1,-1,1,4,5,6,7,8,9,10,11,12,13,14,15,-2,0,2,0,12,13,14,15,16,17,18,19,20,21,22,23,-2,2,-1,1,-2,2,-1,1,-3,3,0,0,12,13,14,15,16,17,18,19,20,21,22,23,-3,1,3,-1,-3,3,1,-1,-2,2,-2,2,12,13,14,15,16,17,18,19,20,21,22,23,-3,3,-2,2,3,2,-3,-2,0,-4,4,4,12,13,14,15,16,17,18,19,20,21,22,23,-4,0,4,-1,1,-1,-4,1,3,-3,-3,-2,-4,4,2,2,3,-4,6,7,8,9,10,11,-2,4,3,-4,-4,3,-3,4,4,-3,2,3,4,-4,4,-4
};
static const mina_i8 aac_hcb5_d1[161] = {
2,0,2,3,4,5,6,7,8,0,0,1,-1,5,6,7,8,-1,1,-1,1,5,6,7,8,9,10,11,12,13,14,15,16,0,2,0,-2,13,14,15,16,17,18,19,20,21,22,23,24,-1,1,-2,2,1,-1,2,-2,0,0,-3,3,13,14,15,16,17,18,19,20,21,22,23,24,-1,3,1,-3,1,-1,-3,3,2,2,-2,-2,13,14,15,16,17,18,19,20,21,22,23,24,-2,-2,3,-3,2,3,2,-3,-4,0,1,0,13,14,15,16,17,18,19,20,21,22,23,24,-1,4,-1,-4,4,4,1,-4,-3,-3,3,4,-2,2,-4,4,3,2,7,8,9,10,11,12,-4,-2,-4,-3,3,4,4,3,-3,-4,3,4,-4,4,4,-4
};
/* codebook hcb7 (binary) entries=127 */
static const mina_u8 aac_hcb7_leaf[127] = {
0,1,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,1,1,1,1
};
static const mina_i8 aac_hcb7_d0[127] = {
1,0,1,2,3,1,0,2,3,1,3,4,5,6,7,8,9,10,11,2,1,2,0,8,9,10,11,12,13,14,15,3,1,2,3,0,11,12,13,14,15,16,17,18,19,20,21,2,3,1,4,1,5,3,2,0,4,12,13,14,15,16,17,18,19,20,21,22,23,4,2,5,0,6,5,1,4,3,3,5,2,6,1,10,11,12,13,14,15,16,17,18,19,3,0,6,4,7,4,7,5,6,2,7,6,5,4,3,5,6,7,8,9,7,0,6,5,7,4,5,7,2,3,7,6,6,7
};
static const mina_i8 aac_hcb7_d1[127] = {
2,0,2,3,4,0,1,3,4,1,4,5,6,7,8,9,10,11,12,1,2,0,2,9,10,11,12,13,14,15,16,1,3,2,0,3,12,13,14,15,16,17,18,19,20,21,22,3,2,4,1,5,1,3,4,4,0,13,14,15,16,17,18,19,20,21,22,23,24,2,5,2,5,1,0,6,3,5,4,3,6,2,7,11,12,13,14,15,16,17,18,19,20,6,6,0,4,1,5,2,4,3,7,3,4,5,6,7,6,7,8,9,10,0,7,5,6,4,7,7,5,3,4,6,6,7,7
};
/* codebook hcb9 (binary) entries=337 */
static const mina_u8 aac_hcb9_leaf[337] = {
0,1,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1
};
static const mina_i8 aac_hcb9_d0[337] = {
1,0,1,2,3,1,0,2,3,1,3,4,5,6,7,8,9,10,11,2,1,2,0,8,9,10,11,12,13,14,15,3,2,1,13,14,15,16,17,18,19,20,21,22,23,24,25,3,0,2,3,1,4,2,1,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,4,3,0,4,5,2,1,3,5,6,4,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,0,2,5,1,3,1,8,4,5,6,7,0,8,2,3,2,4,9,1,7,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,6,5,6,8,0,9,3,4,3,0,10,6,2,5,8,7,7,10,9,8,1,7,6,5,4,4,3,11,5,9,8,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,10,2,0,11,9,6,12,4,8,1,9,10,5,7,2,1,12,11,3,5,6,8,11,0,7,12,10,10,4,6,2,9,9,4,11,6,3,5,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,8,7,12,3,11,7,12,11,10,4,7,5,0,12,6,12,10,5,7,9,0,11,8,9,10,7,12,6,8,11,7,6,8,9,10,11,12,13,14,15,8,10,8,9,9,9,10,12,10,11,12,11,12,12,2,3,10,11,11,12
};
static const mina_i8 aac_hcb9_d1[337] = {
2,0,2,3,4,0,1,3,4,1,4,5,6,7,8,9,10,11,12,1,2,0,2,9,10,11,12,13,14,15,16,1,2,3,14,15,16,17,18,19,20,21,22,23,24,25,26,0,3,3,2,4,1,4,5,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,2,3,4,0,1,5,6,4,2,1,3,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,5,6,0,7,5,8,1,4,3,2,1,6,2,8,6,7,5,1,9,2,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,0,4,3,3,7,2,8,6,7,8,1,4,9,5,0,0,3,2,3,4,10,4,5,6,8,7,9,1,8,0,5,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,3,10,9,2,4,6,1,9,6,11,5,4,7,5,11,12,2,3,10,9,7,7,4,10,6,3,0,5,10,8,12,6,7,11,0,9,11,10,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,8,8,5,12,5,7,4,6,6,12,9,11,11,6,10,0,7,12,10,8,12,7,9,9,8,11,7,11,11,8,12,12,9,10,11,12,13,14,15,16,10,9,12,10,11,12,11,9,10,9,8,10,10,11,3,4,12,11,12,12
};
/* codebook hcb_sf (binary) entries=241 */
static const mina_u8 aac_hcb_sf_leaf[241] = {
0,1,0,0,0,1,0,0,0,1,1,1,0,0,0,1,1,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};
static const mina_i8 aac_hcb_sf_d0[241] = {
1,60,1,2,3,59,3,4,5,61,58,62,3,4,5,57,63,4,5,6,7,56,64,55,65,4,5,6,7,66,54,67,5,6,7,8,9,53,68,52,69,51,5,6,7,8,9,70,50,49,71,6,7,8,9,10,11,72,48,73,47,74,46,6,7,8,9,10,11,76,75,77,78,45,43,6,7,8,9,10,11,44,79,42,41,80,40,6,7,8,9,10,11,81,39,82,38,83,7,8,9,10,11,12,13,37,35,85,33,36,34,84,32,6,7,8,9,10,11,87,89,30,31,8,9,10,11,12,13,14,15,86,29,26,27,28,24,88,9,10,11,12,13,14,15,16,17,25,22,23,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,90,21,19,3,1,2,0,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,98,99,100,101,102,117,97,91,92,93,94,95,96,104,111,112,113,114,115,116,110,105,106,107,108,109,118,6,8,9,10,5,103,120,119,4,7,15,16,18,20,17,11,12,14,13
};
static const mina_i8 aac_hcb_sf_d1[241] = {
2,0,2,3,4,0,4,5,6,0,0,0,4,5,6,0,0,5,6,7,8,0,0,0,0,5,6,7,8,0,0,0,6,7,8,9,10,0,0,0,0,0,6,7,8,9,10,0,0,0,0,7,8,9,10,11,12,0,0,0,0,0,0,7,8,9,10,11,12,0,0,0,0,0,0,7,8,9,10,11,12,0,0,0,0,0,0,7,8,9,10,11,12,0,0,0,0,0,8,9,10,11,12,13,14,0,0,0,0,0,0,0,0,7,8,9,10,11,12,0,0,0,0,9,10,11,12,13,14,15,16,0,0,0,0,0,0,0,10,11,12,13,14,15,16,17,18,0,0,0,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,0,0,0,0,0,0,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* ------------------------------------------------------------------ */
/* AAC: ADTS and MP4/M4A container parsing (identification only)        */
/* ------------------------------------------------------------------ */
static const mina_u32 g_mina_aac_rates[13] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000,
    11025, 8000, 7350
};
static const mina_u8 g_mina_aac_chans[8] = { 0, 1, 2, 3, 4, 5, 6, 8 };

static int mina_aac_adts_header(const mina_u8 *d, size_t avail,
                                mina_u32 *rate, mina_u32 *ch, mina_u32 *len) {
    mina_u32 idx, cfg, flen;
    if (avail < 7) return -1;
    if (d[0] != 0xFFu || (d[1] & 0xF6u) != 0xF0u) return -1;  /* sync + layer 0 */
    idx = (mina_u32)((d[2] >> 2) & 0x0Fu);
    cfg = (mina_u32)(((d[2] & 1u) << 2) | (d[3] >> 6));
    if (idx >= 13) return -1;
    flen = ((mina_u32)(d[3] & 3u) << 11) | ((mina_u32)d[4] << 3) |
           ((mina_u32)d[5] >> 5);
    if (flen < 7) return -1;
    if (rate) *rate = g_mina_aac_rates[idx];
    if (ch) *ch = cfg < 8 ? g_mina_aac_chans[cfg] : 2;
    if (len) *len = flen;
    return 0;
}

static mina_bool mina_aac_probe(const mina_u8 *d, size_t n) {
    if (n >= 14 && mina_aac_adts_header(d, n, NULL, NULL, NULL) == 0) {
        mina_u32 flen = 0;
        mina_aac_adts_header(d, n, NULL, NULL, &flen);
        /* demand a second frame right where the first says it ends */
        if ((size_t)flen + 7 <= n)
            return (mina_bool)(mina_aac_adts_header(d + flen, n - flen,
                                                    NULL, NULL, NULL) == 0);
        return 1;
    }
    if (n >= 12 && memcmp(d + 4, "ftyp", 4) == 0) return 1;
    return 0;
}

static int mina_mp4_next_box(const mina_u8 *d, size_t n, size_t *cursor,
                             size_t end, const mina_u8 **type,
                             size_t *body, size_t *body_end)
{
    size_t off = *cursor, box, hdr = 8;
    mina_u32 z;
    if (off + 8 > end || off + 8 > n) return 0;
    z = mina_be32(d + off);
    box = z;
    if (z == 1) {
        if (off + 16 > end || off + 16 > n) return 0;
        box = mina_u64_size(mina_be64(d + off + 8), NULL);
        hdr = 16;
    } else if (z == 0) box = end - off;
    if (box < hdr || box > end - off || box > n - off) return 0;
    *type = d + off + 4;
    *body = off + hdr;
    *body_end = off + box;
    *cursor = off + box;
    return 1;
}


/* Walk MP4 boxes to find the audio sample entry. */
static int mina_mp4_find(const mina_u8 *d, size_t n, size_t off, size_t end,
                         mina_fileinfo *out, int depth) {
    static const char *containers[] = {
        "moov", "trak", "mdia", "minf", "stbl", "udta", "edts", NULL
    };
    while (off + 8 <= end) {
        mina_u32 sz32 = mina_be32(d + off);
        size_t box = (size_t)sz32, hdr = 8;
        const mina_u8 *type = d + off + 4;
        int i, is_container = 0;
        if (sz32 == 1) {
            if (off + 16 > end) return 0;
            box = mina_u64_size(mina_be64(d + off + 8), NULL);
            hdr = 16;
        } else if (sz32 == 0) {
            box = end - off;
        }
        if (box < hdr || off + box > end) return 0;

        for (i = 0; containers[i]; i++)
            if (memcmp(type, containers[i], 4) == 0) { is_container = 1; break; }

        if (memcmp(type, "mdhd", 4) == 0 && box >= hdr + 24) {
            const mina_u8 *b = d + off + hdr;
            if (b[0] == 0 && box >= hdr + 20) {
                mina_u32 ts = mina_be32(b + 12);
                mina_u32 du = mina_be32(b + 16);
                if (ts) {
                    out->duration_seconds = (double)du / (double)ts;
                    out->total_frames = mina_u64_from(du);
                }
            } else if (b[0] == 1 && box >= hdr + 32) {
                mina_u32 ts = mina_be32(b + 20);
                mina_u64 du = mina_be64(b + 24);
                if (ts) {
                    out->duration_seconds = mina_u64_dbl(du) / (double)ts;
                    out->total_frames = du;
                }
            }
        } else if (memcmp(type, "stsd", 4) == 0 && box >= hdr + 8) {
            /* version/flags(4) + entry count(4), then sample entries */
            size_t p = off + hdr + 8;
            if (p + 28 <= off + box) {
                const mina_u8 *e = d + p;
                if (memcmp(e + 4, "mp4a", 4) == 0 || memcmp(e + 4, "alac", 4) == 0 ||
                    memcmp(e + 4, "ac-3", 4) == 0 || memcmp(e + 4, "Opus", 4) == 0) {
                    out->channels = mina_be16(e + 24);
                    out->bits_per_sample = mina_be16(e + 26);
                    out->sample_rate = mina_be32(e + 32) >> 16;
                    if (memcmp(e + 4, "alac", 4) == 0)
                        mina_strcpy_n(out->codec, sizeof(out->codec), "alac");
                    else if (memcmp(e + 4, "Opus", 4) == 0)
                        mina_strcpy_n(out->codec, sizeof(out->codec), "opus");
                    return 1;
                }
            }
        } else if (is_container && depth < 8) {
            if (mina_mp4_find(d, n, off + hdr, off + box, out, depth + 1)) return 1;
        }
        off += box;
    }
    return 0;
}

static mina_result mina_aac_info(const mina_u8 *d, size_t n, mina_fileinfo *out) {
    memset(out, 0, sizeof(*out));
    mina_strcpy_n(out->codec, sizeof(out->codec), "aac");
    out->bits_per_sample = 16;

    if (n >= 7 && mina_aac_adts_header(d, n, NULL, NULL, NULL) == 0) {
        mina_u32 rate = 0, ch = 0;
        size_t pos = 0;
        mina_u32 frames = 0;
        double bytes = 0.0;
        mina_aac_adts_header(d, n, &rate, &ch, NULL);
        out->sample_rate = rate;
        out->channels = ch;
        while (pos + 7 <= n) {
            mina_u32 flen = 0;
            if (mina_aac_adts_header(d + pos, n - pos, NULL, NULL, &flen) != 0) break;
            frames++;
            bytes += (double)flen;
            if ((size_t)flen > n - pos) break;
            pos += flen;
        }
        if (frames) {
            out->total_frames = mina_u64_mul32(frames, 1024);   /* AAC-LC */
            if (rate) {
                out->duration_seconds = mina_u64_dbl(out->total_frames) / (double)rate;
                if (out->duration_seconds > 0.0)
                    out->bitrate_kbps =
                        (mina_u32)(bytes * 8.0 / out->duration_seconds / 1000.0);
            }
        }
        return MINA_OK;
    }

    if (n >= 12 && memcmp(d + 4, "ftyp", 4) == 0) {
        mina_mp4_find(d, n, 0, n, out, 0);
        if (out->duration_seconds > 0.0)
            out->bitrate_kbps = (mina_u32)((double)n * 8.0 / out->duration_seconds / 1000.0);
        return MINA_OK;
    }
    return MINA_ERR_NOTFOUND;
}

#ifndef MINA_NO_AAC
/* ------------------------------ constants ------------------------------ */

#define MINA_AAC_MAX_SFB    51
#define MINA_AAC_MAX_GROUP  8
#define MINA_AAC_MAX_SEC    120   /* 8*15 for shorts; long sequences use <=51 */
#define MINA_AAC_MAX_CH     8

/* window_sequence */
#define MINA_AAC_ONLY_LONG   0
#define MINA_AAC_LONG_START  1
#define MINA_AAC_EIGHT_SHORT 2
#define MINA_AAC_LONG_STOP   3

/* codebook ids */
#define MINA_AAC_ZERO_HCB       0
#define MINA_AAC_FIRST_PAIR_HCB 5
#define MINA_AAC_ESC_HCB        11
#define MINA_AAC_NOISE_HCB      13
#define MINA_AAC_INTENSITY_HCB2 14
#define MINA_AAC_INTENSITY_HCB  15

/* id_syn_ele (3 bits) */
#define MINA_AAC_ID_SCE 0
#define MINA_AAC_ID_CPE 1
#define MINA_AAC_ID_CCE 2
#define MINA_AAC_ID_LFE 3
#define MINA_AAC_ID_DSE 4
#define MINA_AAC_ID_PCE 5
#define MINA_AAC_ID_FIL 6
#define MINA_AAC_ID_END 7

#define MINA_AAC_LEN_SE_ID 3
#define MINA_AAC_LEN_TAG   4

/* ------------------------------ bit reader ------------------------------ */

typedef struct mina_aac_bits {
    const mina_u8 *buf;   /* raw frame bytes (frame payload after ADTS header) */
    size_t nbits;         /* total bits available                          */
    size_t bitpos;        /* current read position                          */
    int    error;         /* set to nonzero (1) on an overrun               */
} mina_aac_bits;

/* Initialise over nbytes bytes; bit 0 is the MSB of buf[0] (big-endian bit order). */
static void mina_aac_bits_init(mina_aac_bits *b, const mina_u8 *p, size_t nbytes);

/* Read n bits (n in [1,32]) MSB-first, advancing. On overrun set b->error=1
 * and return 0 (mirrors faad_getbits error behaviour). */
static mina_u32 mina_aac_bits_read(mina_aac_bits *b, int n);

/* Read a single bit. */
static int mina_aac_bits_bit(mina_aac_bits *b);

/* Lookahead of n bits WITHOUT advancing. On overrun set b->error=1, return 0. */
static mina_u32 mina_aac_bits_show(mina_aac_bits *b, int n);

/* Advance n bits (n in [1,32]). On overrun set b->error=1. */
static void mina_aac_bits_flush(mina_aac_bits *b, int n);

/* Align to the next byte boundary (bitpos = (bitpos+7)&~7). */
static void mina_aac_bits_align(mina_aac_bits *b);

/* ------------------------------ ic_stream ------------------------------ */

typedef struct mina_aac_ics {
    mina_u8  global_gain;
    mina_u8  window_sequence;
    mina_u8  window_shape;
    mina_u8  max_sfb;
    mina_u8  scale_factor_grouping;
    mina_u8  num_windows;
    mina_u8  num_window_groups;
    mina_u8  window_group_length[MINA_AAC_MAX_GROUP];
    mina_u8  num_swb;
    mina_u16 swb_offset[MINA_AAC_MAX_SFB + 1];
    mina_u16 swb_offset_max;
    mina_u16 sect_sfb_offset[MINA_AAC_MAX_GROUP][MINA_AAC_MAX_SEC];
    mina_u8  sect_cb[MINA_AAC_MAX_GROUP][MINA_AAC_MAX_SEC];
    mina_u16 sect_start[MINA_AAC_MAX_GROUP][MINA_AAC_MAX_SEC];
    mina_u16 sect_end[MINA_AAC_MAX_GROUP][MINA_AAC_MAX_SEC];
    mina_u8  sfb_cb[MINA_AAC_MAX_GROUP][MINA_AAC_MAX_SEC];
    mina_u8  num_sec[MINA_AAC_MAX_GROUP];
    mina_i16 scale_factors[MINA_AAC_MAX_GROUP][MINA_AAC_MAX_SFB];
    mina_u8  ms_mask_present;
    mina_u8  ms_used[MINA_AAC_MAX_GROUP][MINA_AAC_MAX_SFB];
    mina_u8  pulse_data_present;
    mina_u8  number_pulse;
    mina_u8  pulse_start_sfb;
    mina_u8  pulse_offset[4];
    mina_u8  pulse_amp[4];
    mina_u8  tns_data_present;
    mina_u8  tns_n_filt[8];
    mina_u8  tns_coef_res[8];
    mina_u8  tns_length[8][4];
    mina_u8  tns_order[8][4];
    mina_u8  tns_direction[8][4];
    mina_u8  tns_coef_compress[8][4];
    mina_u8  tns_coef[8][4][32];
    mina_u8  is_used;
    mina_u8  noise_used;
} mina_aac_ics;

/* ---------------------------- decoder context ---------------------------- */

typedef struct mina_aac_ctx {
    mina_u8  sf_index;          /* sample-rate index from ADTS (0..12)      */
    mina_u8  chans_cfg;         /* ADTS channel_configuration (0..7)        */
    mina_u8  alloc_chans;
    mina_u32 frame;             /* decoded-frame counter                    */
    mina_u32 random_state;

    /* window_shape_prev per output channel */
    mina_u8  window_shape_prev[MINA_AAC_MAX_CH];

    /* windows [shape][k]; shape 0 = sine, 1 = KBD
     * long window has frame_len samples (1024), short 1024/8 (128). */
    float w_long[2][1024];
    float w_short[2][128];

    /* MDCT twiddles for N = 2048 (long) and N = 256 (short). */
    float sincos_2048[512][2];
    float sincos_256[64][2];

    double cfft_work[1024];
    /* per-channel time-domain output and overlap buffers, frame_len floats */
    float *time_out[MINA_AAC_MAX_CH];
    float *fb_intermed[MINA_AAC_MAX_CH];
} mina_aac_ctx;

static mina_u8 mina_aac_pulse_decode(mina_aac_ics *ics, mina_i16 *spec,
                                     int frame_len);

/* ------------------------------------------------------------------ */
/* bit reader                                                          */
/* ------------------------------------------------------------------ */

static void mina_aac_bits_init(mina_aac_bits *b, const mina_u8 *p, size_t nbytes)
{
    b->buf = p;
    b->nbits = nbytes * 8;
    b->bitpos = 0;
    b->error = 0;
}

static mina_u32 mina_aac_bits_read(mina_aac_bits *b, int n)
{
    mina_u32 v = 0;
    int left = n;

    if (n < 1 || n > 32) { b->error = 1; return 0; }
    if (b->bitpos + (size_t)n > b->nbits) { b->error = 1; return 0; }

    while (left) {
        int bit = (int)(b->bitpos & 7);
        int take = 8 - bit;
        mina_u32 mask;
        if (take > left) take = left;
        mask = (1u << take) - 1u;
        v = (v << take) |
            ((b->buf[b->bitpos >> 3] >> (8 - bit - take)) & mask);
        b->bitpos += (size_t)take;
        left -= take;
    }
    return v;
}

static int mina_aac_bits_bit(mina_aac_bits *b)
{
    return (int)mina_aac_bits_read(b, 1);
}

static mina_u32 mina_aac_bits_show(mina_aac_bits *b, int n)
{
    size_t keep_pos = b->bitpos;
    int keep_err = b->error;
    mina_u32 v = mina_aac_bits_read(b, n);
    b->bitpos = keep_pos;
    b->error = keep_err;
    return v;
}

static void mina_aac_bits_flush(mina_aac_bits *b, int n)
{
    if (n < 0 || b->bitpos + (size_t)n > b->nbits) b->error = 1;
    else b->bitpos += (size_t)n;
}

static void mina_aac_bits_align(mina_aac_bits *b)
{
    b->bitpos = (b->bitpos + 7) & ~(size_t)7;
    if (b->bitpos > b->nbits)
        b->error = 1;
}


/* ------------------------------------------------------------------ */
/* windows: sine + KBD (ffmpeg kbd_window_init, alpha = 4)             */
/* ------------------------------------------------------------------ */

static double mina_aac_bessel_i0(double x)
{
    double v = 1.0, t = 1.0;
    int i;
    for (i = 1; i < 200; i++) {
        t *= x * x / (4.0 * i * i);
        v += t;
        if (t < 1e-14 * v)
            return v;
    }
    return v;
}

static void mina_aac_kbd_window(float *w, float alpha, int n)
{
    double sum = 0.0, tmp, scale = 0.0;
    double alpha2;
    double temp[513];
    int i;

    alpha2 = 4.0 * (alpha * MINA_PI / (double)n) * (alpha * MINA_PI / (double)n);

    for (i = 0; i <= n / 2; i++) {
        tmp = (double)i * (double)(n - i) * alpha2;
        temp[i] = mina_aac_bessel_i0(sqrt(tmp));
        scale += temp[i] * (1.0 + ((i && i < n / 2) ? 1.0 : 0.0));
    }
    scale = 1.0 / (scale + 1.0);

    for (i = 0; i <= n / 2; i++) {
        sum += temp[i];
        w[i] = (float)sqrt(sum * scale);
    }
    for (; i < n; i++) {
        sum += temp[n - i];
        w[i] = (float)sqrt(sum * scale);
    }
}

static void mina_aac_build_windows(mina_aac_ctx *d)
{
    int k;

    /* AAC long window spans 2048 samples (half stored), short spans 256. */
    for (k = 0; k < 1024; k++)
        d->w_long[0][k] = (float)sin((k + 0.5) * MINA_PI / 2048.0);
    mina_aac_kbd_window(d->w_long[1], 4.0f, 1024);

    for (k = 0; k < 128; k++)
        d->w_short[0][k] = (float)sin((k + 0.5) * MINA_PI / 256.0);
    mina_aac_kbd_window(d->w_short[1], 6.0f, 128);

    for (k = 0; k < 512; k++) {
        d->sincos_2048[k][0] = (float)(sqrt(2.0 / 2048.0) * cos(2.0 * MINA_PI * (k + 1.0 / 8.0) / 2048.0));
        d->sincos_2048[k][1] = (float)(sqrt(2.0 / 2048.0) * sin(2.0 * MINA_PI * (k + 1.0 / 8.0) / 2048.0));
    }
    for (k = 0; k < 64; k++) {
        d->sincos_256[k][0] = (float)(sqrt(2.0 / 256.0) * cos(2.0 * MINA_PI * (k + 1.0 / 8.0) / 256.0));
        d->sincos_256[k][1] = (float)(sqrt(2.0 / 256.0) * sin(2.0 * MINA_PI * (k + 1.0 / 8.0) / 256.0));
    }
}

static int mina_aac_fb_init(mina_aac_ctx *d)
{
    int ch, nch = d->chans_cfg ? g_mina_aac_chans[d->chans_cfg] : MINA_AAC_MAX_CH;

    for (ch = 0; ch < nch; ch++) {
        d->time_out[ch] = (float *)MINA_MALLOC(1024 * sizeof(float));
        d->fb_intermed[ch] = (float *)MINA_MALLOC(1024 * sizeof(float));
        if (!d->time_out[ch] || !d->fb_intermed[ch]) return 0;
        memset(d->time_out[ch], 0, 1024 * sizeof(float));
        memset(d->fb_intermed[ch], 0, 1024 * sizeof(float));
    }

    d->alloc_chans = (mina_u8)nch;
    mina_aac_build_windows(d);
    return 1;
}

static void mina_aac_fb_free(mina_aac_ctx *d)
{
    int ch;
    for (ch = 0; ch < MINA_AAC_MAX_CH; ch++) {
        MINA_FREE(d->time_out[ch]);
        MINA_FREE(d->fb_intermed[ch]);
        d->time_out[ch] = NULL;
        d->fb_intermed[ch] = NULL;
    }
}

static void mina_aac_cifft(double *re, double *im, int n)
{
    int i, j, bit, len;
    for (i = 1, j = 0; i < n; i++) {
        bit = n >> 1;
        while (j & bit) { j ^= bit; bit >>= 1; }
        j ^= bit;
        if (i < j) {
            double t = re[i]; re[i] = re[j]; re[j] = t;
            t = im[i]; im[i] = im[j]; im[j] = t;
        }
    }
    for (len = 2; len <= n; len <<= 1) {
        int half = len >> 1;
        double a = 2.0 * MINA_PI / (double)len;
        double wr0 = cos(a), wi0 = sin(a);
        for (i = 0; i < n; i += len) {
            double wr = 1.0, wi = 0.0;
            for (j = 0; j < half; j++) {
                int p = i + j, q = p + half;
                double tr = re[q] * wr - im[q] * wi;
                double ti = re[q] * wi + im[q] * wr;
                double nr = wr * wr0 - wi * wi0;
                im[q] = im[p] - ti;
                re[q] = re[p] - tr;
                re[p] += tr;
                im[p] += ti;
                wi = wr * wi0 + wi * wr0;
                wr = nr;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* IMDCT: pre/post rotation, inverse FFT and FAAD reordering.           */
/* ------------------------------------------------------------------ */

static void mina_aac_imdct(mina_aac_ctx *d, int n, const float *in, float *out)
{
    int n2 = n >> 1, n4 = n >> 2, n8 = n >> 3;
    float *sincos = (n == 2048) ? &d->sincos_2048[0][0] : &d->sincos_256[0][0];
    double *zre = &d->cfft_work[0];
    double *zim = zre + n4;
    int k;

    /* pre-IFFT complex multiplication */
    for (k = 0; k < n4; k++) {
        float x1 = in[2 * k];
        float x2 = in[n2 - 1 - 2 * k];
        float c1 = sincos[2 * k];
        float c2 = sincos[2 * k + 1];
        zim[k] = x1 * c1 + x2 * c2;
        zre[k] = x2 * c1 - x1 * c2;
    }

    mina_aac_cifft(zre, zim, n4);
    for (k = 0; k < n4; k++) {
        double x_in_re = zre[k];
        double x_in_im = zim[k];
        float c1 = sincos[2 * k];
        float c2 = sincos[2 * k + 1];
        zre[k] = x_in_re * c1 - x_in_im * c2;
        zim[k] = x_in_im * c1 + x_in_re * c2;
    }

    /* reordering */
    for (k = 0; k < n8; k += 2) {
        out[2 * k]                   = zim[n8 + k];
        out[2 + 2 * k]               = zim[n8 + 1 + k];
        out[1 + 2 * k]               = -zre[n8 - 1 - k];
        out[3 + 2 * k]               = -zre[n8 - 2 - k];
        out[n4 + 2 * k]              =  zre[k];
        out[n4 + 2 + 2 * k]          =  zre[1 + k];
        out[n4 + 1 + 2 * k]          = -zim[n4 - 1 - k];
        out[n4 + 3 + 2 * k]          = -zim[n4 - 2 - k];
        out[n2 + 2 * k]              =  zre[n8 + k];
        out[n2 + 2 + 2 * k]          =  zre[n8 + 1 + k];
        out[n2 + 1 + 2 * k]          = -zim[n8 - 1 - k];
        out[n2 + 3 + 2 * k]          = -zim[n8 - 2 - k];
        out[n2 + n4 + 2 * k]         = -zim[k];
        out[n2 + n4 + 2 + 2 * k]     = -zim[1 + k];
        out[n2 + n4 + 1 + 2 * k]     =  zre[n4 - 1 - k];
        out[n2 + n4 + 3 + 2 * k]     =  zre[n4 - 2 - k];
    }
}

/* ------------------------------------------------------------------ */
/* inverse filter bank (LC, frame_len = 1024)                          */
/* ------------------------------------------------------------------ */

static void mina_aac_ifilter_bank(mina_aac_ctx *d, mina_u8 win_seq,
                                  mina_u8 shape, mina_u8 shape_prev,
                                  const float *freq_in, float *time_out,
                                  float *overlap)
{
    const int frame_len = 1024;
    const int nshort = frame_len / 8;
    const int trans = nshort / 2;
    const int nflat_ls = (frame_len - nshort) / 2;
    float transf_buf[2048];
    const float *window_long;
    const float *window_long_prev;
    const float *window_short;
    const float *window_short_prev;
    int i;

    memset(transf_buf, 0, sizeof(transf_buf));

    window_long       = d->w_long[shape];
    window_long_prev  = d->w_long[shape_prev];
    window_short      = d->w_short[shape];
    window_short_prev = d->w_short[shape_prev];

    switch (win_seq) {
    case MINA_AAC_ONLY_LONG:
    case MINA_AAC_LONG_START:
        mina_aac_imdct(d, 2048, freq_in, transf_buf);

        for (i = 0; i < frame_len; i += 4) {
            time_out[i]     = overlap[i]     + transf_buf[i]     * window_long_prev[i];
            time_out[i + 1] = overlap[i + 1] + transf_buf[i + 1] * window_long_prev[i + 1];
            time_out[i + 2] = overlap[i + 2] + transf_buf[i + 2] * window_long_prev[i + 2];
            time_out[i + 3] = overlap[i + 3] + transf_buf[i + 3] * window_long_prev[i + 3];
        }
        if (win_seq == MINA_AAC_ONLY_LONG) {
            for (i = 0; i < frame_len; i += 4) {
                overlap[i]     = transf_buf[frame_len + i]     * window_long[frame_len - 1 - i];
                overlap[i + 1] = transf_buf[frame_len + i + 1] * window_long[frame_len - 2 - i];
                overlap[i + 2] = transf_buf[frame_len + i + 2] * window_long[frame_len - 3 - i];
                overlap[i + 3] = transf_buf[frame_len + i + 3] * window_long[frame_len - 4 - i];
            }
        } else {
            for (i = 0; i < nflat_ls; i++)
                overlap[i] = transf_buf[frame_len + i];
            for (i = 0; i < nshort; i++)
                overlap[nflat_ls + i] = transf_buf[frame_len + nflat_ls + i] *
                                        window_short[nshort - i - 1];
            for (i = 0; i < nflat_ls; i++)
                overlap[nflat_ls + nshort + i] = 0;
        }
        break;

    case MINA_AAC_EIGHT_SHORT: {
        int j;
        for (j = 0; j < 8; j++)
            mina_aac_imdct(d, 256, freq_in + j * nshort, transf_buf + 2 * nshort * j);

        for (i = 0; i < nflat_ls; i++)
            time_out[i] = overlap[i];
        for (i = 0; i < nshort; i++) {
            time_out[nflat_ls + i] = overlap[nflat_ls + i] +
                transf_buf[nshort * 0 + i] * window_short_prev[i];
            time_out[nflat_ls + nshort + i] = overlap[nflat_ls + nshort + i] +
                transf_buf[nshort * 1 + i] * window_short[nshort - 1 - i] +
                transf_buf[nshort * 2 + i] * window_short[i];
            time_out[nflat_ls + 2 * nshort + i] = overlap[nflat_ls + 2 * nshort + i] +
                transf_buf[nshort * 3 + i] * window_short[nshort - 1 - i] +
                transf_buf[nshort * 4 + i] * window_short[i];
            time_out[nflat_ls + 3 * nshort + i] = overlap[nflat_ls + 3 * nshort + i] +
                transf_buf[nshort * 5 + i] * window_short[nshort - 1 - i] +
                transf_buf[nshort * 6 + i] * window_short[i];
            if (i < trans)
                time_out[nflat_ls + 4 * nshort + i] = overlap[nflat_ls + 4 * nshort + i] +
                    transf_buf[nshort * 7 + i] * window_short[nshort - 1 - i] +
                    transf_buf[nshort * 8 + i] * window_short[i];
        }

        for (i = 0; i < nshort; i++) {
            if (i >= trans)
                overlap[nflat_ls + 4 * nshort + i - frame_len] =
                    transf_buf[nshort * 7 + i] * window_short[nshort - 1 - i] +
                    transf_buf[nshort * 8 + i] * window_short[i];
            overlap[nflat_ls + 5 * nshort + i - frame_len] =
                transf_buf[nshort * 9 + i] * window_short[nshort - 1 - i] +
                transf_buf[nshort * 10 + i] * window_short[i];
            overlap[nflat_ls + 6 * nshort + i - frame_len] =
                transf_buf[nshort * 11 + i] * window_short[nshort - 1 - i] +
                transf_buf[nshort * 12 + i] * window_short[i];
            overlap[nflat_ls + 7 * nshort + i - frame_len] =
                transf_buf[nshort * 13 + i] * window_short[nshort - 1 - i] +
                transf_buf[nshort * 14 + i] * window_short[i];
            overlap[nflat_ls + 8 * nshort + i - frame_len] =
                transf_buf[nshort * 15 + i] * window_short[nshort - 1 - i];
        }
        for (i = 0; i < nflat_ls; i++)
            overlap[nflat_ls + nshort + i] = 0;
        break;
    }

    case MINA_AAC_LONG_STOP:
        mina_aac_imdct(d, 2048, freq_in, transf_buf);

        for (i = 0; i < nflat_ls; i++)
            time_out[i] = overlap[i];
        for (i = 0; i < nshort; i++)
            time_out[nflat_ls + i] = overlap[nflat_ls + i] +
                transf_buf[nflat_ls + i] * window_short_prev[i];
        for (i = 0; i < nflat_ls; i++)
            time_out[nflat_ls + nshort + i] = overlap[nflat_ls + nshort + i] +
                transf_buf[nflat_ls + nshort + i];

        for (i = 0; i < frame_len; i++)
            overlap[i] = transf_buf[frame_len + i] * window_long[frame_len - 1 - i];
        break;
    }
}

/* ------------------------------------------------------------------ */
/* scalefactor-band tables (from FAAD2 specrec.c)                      */
/* ------------------------------------------------------------------ */

static const mina_u8 mina_aac_num_swb_1024_window[12] = {
    41, 41, 47, 49, 49, 51, 47, 47, 43, 43, 43, 40
};
static const mina_u8 mina_aac_num_swb_128_window[12] = {
    12, 12, 12, 14, 14, 14, 15, 15, 15, 15, 15, 15
};

static const mina_u16 mina_aac_swb_1024_96[42] = {
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
    64, 72, 80, 88, 96, 108, 120, 132, 144, 156, 172, 188, 212, 240,
    276, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1024
};
static const mina_u16 mina_aac_swb_1024_64[49] = {
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
    64, 72, 80, 88, 100, 112, 124, 140, 156, 172, 192, 216, 240, 268,
    304, 344, 384, 424, 464, 504, 544, 584, 624, 664, 704, 744, 784, 824,
    864, 904, 944, 984, 1024
};
static const mina_u16 mina_aac_swb_1024_48[50] = {
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
    80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
    320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
    768, 800, 832, 864, 896, 928, 1024
};
static const mina_u16 mina_aac_swb_1024_32[52] = {
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
    80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
    320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
    768, 800, 832, 864, 896, 928, 960, 992, 1024
};
static const mina_u16 mina_aac_swb_1024_24[48] = {
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68,
    76, 84, 92, 100, 108, 116, 124, 136, 148, 160, 172, 188, 204, 220,
    240, 260, 284, 308, 336, 364, 396, 432, 468, 508, 552, 600, 652, 704,
    768, 832, 896, 960, 1024
};
static const mina_u16 mina_aac_swb_1024_16[44] = {
    0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 100, 112, 124,
    136, 148, 160, 172, 184, 196, 212, 228, 244, 260, 280, 300, 320, 344,
    368, 396, 424, 456, 492, 532, 572, 616, 664, 716, 772, 832, 896, 960, 1024
};
static const mina_u16 mina_aac_swb_1024_8[41] = {
    0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172,
    188, 204, 220, 236, 252, 268, 288, 308, 328, 348, 372, 396, 420, 448,
    476, 508, 544, 580, 620, 664, 712, 764, 820, 880, 944, 1024
};

static const mina_u16 mina_aac_swb_128_96[13] = {
    0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128
};
static const mina_u16 mina_aac_swb_128_64[13] = {
    0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128
};
static const mina_u16 mina_aac_swb_128_48[15] = {
    0, 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112, 128
};
static const mina_u16 mina_aac_swb_128_24[16] = {
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 128
};
static const mina_u16 mina_aac_swb_128_16[16] = {
    0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 128
};
static const mina_u16 mina_aac_swb_128_8[16] = {
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 108, 128
};

static const mina_u16 *mina_aac_swb_1024_window[12] = {
    mina_aac_swb_1024_96, mina_aac_swb_1024_96, mina_aac_swb_1024_64,
    mina_aac_swb_1024_48, mina_aac_swb_1024_48, mina_aac_swb_1024_32,
    mina_aac_swb_1024_24, mina_aac_swb_1024_24, mina_aac_swb_1024_16,
    mina_aac_swb_1024_16, mina_aac_swb_1024_16, mina_aac_swb_1024_8
};
static const mina_u16 *mina_aac_swb_128_window[12] = {
    mina_aac_swb_128_96, mina_aac_swb_128_96, mina_aac_swb_128_64,
    mina_aac_swb_128_48, mina_aac_swb_128_48, mina_aac_swb_128_48,
    mina_aac_swb_128_24, mina_aac_swb_128_24, mina_aac_swb_128_16,
    mina_aac_swb_128_16, mina_aac_swb_128_16, mina_aac_swb_128_8
};

#define MINA_AAC_BIT_SET(A, B) ((A) & (1 << (B)))

static mina_u8 mina_aac_window_grouping(mina_aac_ctx *d, mina_aac_ics *ics)
{
    mina_u8 i, g;
    const mina_u8 sf_index = d->sf_index;
    const int frame_len = 1024;

    if (sf_index >= 12)
        return 32;

    switch (ics->window_sequence) {
    case MINA_AAC_ONLY_LONG:
    case MINA_AAC_LONG_START:
    case MINA_AAC_LONG_STOP:
        ics->num_windows = 1;
        ics->num_window_groups = 1;
        ics->window_group_length[0] = 1;
        ics->num_swb = mina_aac_num_swb_1024_window[sf_index];

        if (ics->max_sfb > ics->num_swb)
            return 32;

        for (i = 0; i < ics->num_swb; i++) {
            ics->sect_sfb_offset[0][i] = mina_aac_swb_1024_window[sf_index][i];
            ics->swb_offset[i] = mina_aac_swb_1024_window[sf_index][i];
        }
        ics->sect_sfb_offset[0][ics->num_swb] = (mina_u16)frame_len;
        ics->swb_offset[ics->num_swb] = (mina_u16)frame_len;
        ics->swb_offset_max = (mina_u16)frame_len;

        if (ics->num_swb > 0 &&
            ics->swb_offset[ics->num_swb] < ics->swb_offset[ics->num_swb - 1])
            return 32;
        return 0;

    case MINA_AAC_EIGHT_SHORT:
        ics->num_windows = 8;
        ics->num_window_groups = 1;
        ics->window_group_length[0] = 1;
        ics->num_swb = mina_aac_num_swb_128_window[sf_index];

        if (ics->max_sfb > ics->num_swb)
            return 32;

        for (i = 0; i < ics->num_swb; i++)
            ics->swb_offset[i] = mina_aac_swb_128_window[sf_index][i];
        ics->swb_offset[ics->num_swb] = (mina_u16)(frame_len / 8);
        ics->swb_offset_max = (mina_u16)(frame_len / 8);

        for (i = 0; i < ics->num_windows - 1; i++) {
            if (MINA_AAC_BIT_SET(ics->scale_factor_grouping, 6 - i) == 0) {
                ics->num_window_groups += 1;
                ics->window_group_length[ics->num_window_groups - 1] = 1;
            } else {
                ics->window_group_length[ics->num_window_groups - 1] += 1;
            }
        }

        /* seven increments over a base of one; assert it so the bound is
         * visible to the compiler at every sfb_cb write below */
        if (ics->num_window_groups > MINA_AAC_MAX_GROUP)
            return 32;

        for (g = 0; g < ics->num_window_groups; g++) {
            mina_u16 offset = 0;
            mina_u8 sect_sfb = 0;
            for (i = 0; i < ics->num_swb; i++) {
                mina_u16 width;
                if (i + 1 == ics->num_swb) {
                    width = (mina_u16)((frame_len / 8) -
                                     mina_aac_swb_128_window[sf_index][i]);
                } else {
                    width = (mina_u16)(mina_aac_swb_128_window[sf_index][i + 1] -
                                     mina_aac_swb_128_window[sf_index][i]);
                }
                width = (mina_u16)(width * ics->window_group_length[g]);
                ics->sect_sfb_offset[g][sect_sfb++] = offset;
                offset = (mina_u16)(offset + width);
            }
            ics->sect_sfb_offset[g][sect_sfb] = offset;
        }
        return 0;

    default:
        return 32;
    }
}

/* ------------------------------------------------------------------ */
/* Huffman: scale factors (binary), spectral books (2-step / binary)   */
/* ------------------------------------------------------------------ */

static mina_i16 mina_aac_huff_sf(mina_aac_bits *b)
{
    int node = 0;
    for (;;) {
        if (aac_hcb_sf_leaf[node]) {
            return (mina_i16)aac_hcb_sf_d0[node];
        }
        if (mina_aac_bits_bit(b))
            node += (int)aac_hcb_sf_d1[node];
        else
            node += (int)aac_hcb_sf_d0[node];
    }
}

static void mina_aac_huff_sign(mina_aac_bits *b, mina_i16 *sp, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (sp[i]) {
            if (mina_aac_bits_bit(b))
                sp[i] = (mina_i16)(-sp[i]);
        }
    }
}

static mina_u8 mina_aac_huff_escape(mina_aac_bits *b, mina_i16 *sp)
{
    int neg = 0;
    int i;
    mina_i16 x = *sp;
    int j, off;

    if (x < 0) {
        if (x != -16) return 0;
        neg = 1;
    } else {
        if (x != 16) return 0;
        neg = 0;
    }

    for (i = 4; i < 16; i++) {
        if (mina_aac_bits_bit(b) == 0)
            break;
    }
    if (i >= 16)
        return 10;

    off = (int)mina_aac_bits_read(b, i);
    j = off | (1 << i);
    if (neg)
        j = -j;
    *sp = (mina_i16)j;
    return 0;
}

static mina_u8 mina_aac_huff_2step(mina_aac_bits *b, int root_bits,
                                   const mina_u16 *r_off, const mina_u8 *r_xb,
                                   const mina_u8 *bits, mina_i16 *sp, int inc,
                                   const mina_i8 *x, const mina_i8 *y,
                                   const mina_i8 *v, const mina_i8 *w,
                                   int has_vw)
{
    mina_u32 cw;
    int off, extra;
    (void)inc;

    cw = mina_aac_bits_show(b, root_bits);
    off = (int)r_off[cw];
    extra = (int)r_xb[cw];

    if (extra) {
        mina_aac_bits_flush(b, root_bits);
        off += (int)mina_aac_bits_show(b, extra);
        mina_aac_bits_flush(b, (int)bits[off] - root_bits);
    } else {
        mina_aac_bits_flush(b, (int)bits[off]);
    }

    sp[0] = (mina_i16)x[off];
    sp[1] = (mina_i16)y[off];
    if (has_vw) {
        sp[2] = (mina_i16)v[off];
        sp[3] = (mina_i16)w[off];
    }
    return 0;
}

static mina_u8 mina_aac_huff_binary_quad(mina_aac_bits *b, mina_i16 *sp)
{
    int node = 0;
    for (;;) {
        if (aac_hcb3_leaf[node]) {
            sp[0] = (mina_i16)aac_hcb3_d0[node];
            sp[1] = (mina_i16)aac_hcb3_d1[node];
            sp[2] = (mina_i16)aac_hcb3_d2[node];
            sp[3] = (mina_i16)aac_hcb3_d3[node];
            return 0;
        }
        if (mina_aac_bits_bit(b))
            node += (int)aac_hcb3_d1[node];
        else
            node += (int)aac_hcb3_d0[node];
    }
}

static mina_u8 mina_aac_huff_binary_pair(mina_aac_bits *b,
                                         const mina_u8 *leaf,
                                         const mina_i8 *d0, const mina_i8 *d1,
                                         mina_i16 *sp)
{
    int node = 0;
    for (;;) {
        if (leaf[node]) {
            sp[0] = (mina_i16)d0[node];
            sp[1] = (mina_i16)d1[node];
            return 0;
        }
        if (mina_aac_bits_bit(b))
            node += (int)d1[node];
        else
            node += (int)d0[node];
    }
}

static mina_u8 mina_aac_spectral_huff(mina_u8 cb, mina_aac_bits *b, mina_i16 *sp)
{
    switch (cb) {
    case 1:
        return mina_aac_huff_2step(b, 5, aac_hcb1_r_off, aac_hcb1_r_xb,
            aac_hcb1_bits, sp, 4, aac_hcb1_x, aac_hcb1_y, aac_hcb1_v,
            aac_hcb1_w, 1);
    case 2:
        return mina_aac_huff_2step(b, 5, aac_hcb2_r_off, aac_hcb2_r_xb,
            aac_hcb2_bits, sp, 4, aac_hcb2_x, aac_hcb2_y, aac_hcb2_v,
            aac_hcb2_w, 1);
    case 3: {
        mina_u8 err = mina_aac_huff_binary_quad(b, sp);
        if (err) return err;
        mina_aac_huff_sign(b, sp, 4);
        return 0;
    }
    case 4: {
        mina_u8 err = mina_aac_huff_2step(b, 5, aac_hcb4_r_off, aac_hcb4_r_xb,
            aac_hcb4_bits, sp, 4, aac_hcb4_x, aac_hcb4_y, aac_hcb4_v,
            aac_hcb4_w, 1);
        if (err) return err;
        mina_aac_huff_sign(b, sp, 4);
        return 0;
    }
    case 5:
        return mina_aac_huff_binary_pair(b, aac_hcb5_leaf, aac_hcb5_d0,
            aac_hcb5_d1, sp);
    case 6:
        return mina_aac_huff_2step(b, 5, aac_hcb6_r_off, aac_hcb6_r_xb,
            aac_hcb6_bits, sp, 2, aac_hcb6_x, aac_hcb6_y, NULL, NULL, 0);
    case 7: {
        mina_u8 err = mina_aac_huff_binary_pair(b, aac_hcb7_leaf, aac_hcb7_d0,
            aac_hcb7_d1, sp);
        if (err) return err;
        mina_aac_huff_sign(b, sp, 2);
        return 0;
    }
    case 8: {
        mina_u8 err = mina_aac_huff_2step(b, 5, aac_hcb8_r_off, aac_hcb8_r_xb,
            aac_hcb8_bits, sp, 2, aac_hcb8_x, aac_hcb8_y, NULL, NULL, 0);
        if (err) return err;
        mina_aac_huff_sign(b, sp, 2);
        return 0;
    }
    case 9: {
        mina_u8 err = mina_aac_huff_binary_pair(b, aac_hcb9_leaf, aac_hcb9_d0,
            aac_hcb9_d1, sp);
        if (err) return err;
        mina_aac_huff_sign(b, sp, 2);
        return 0;
    }
    case 10: {
        mina_u8 err = mina_aac_huff_2step(b, 6, aac_hcb10_r_off, aac_hcb10_r_xb,
            aac_hcb10_bits, sp, 2, aac_hcb10_x, aac_hcb10_y, NULL, NULL, 0);
        if (err) return err;
        mina_aac_huff_sign(b, sp, 2);
        return 0;
    }
    case 11: {
        mina_u8 err = mina_aac_huff_2step(b, 5, aac_hcb11_r_off, aac_hcb11_r_xb,
            aac_hcb11_bits, sp, 2, aac_hcb11_x, aac_hcb11_y, NULL, NULL, 0);
        if (err) return err;
        mina_aac_huff_sign(b, sp, 2);
        if (!err)
            err = mina_aac_huff_escape(b, &sp[0]);
        if (!err)
            err = mina_aac_huff_escape(b, &sp[1]);
        return err;
    }
    default:
        return 11;
    }
}

/* ------------------------------------------------------------------ */
/* ics_info()                                                          */
/* ------------------------------------------------------------------ */

static mina_u8 mina_aac_ics_info(mina_aac_ctx *d, mina_aac_ics *ics,
                                 mina_aac_bits *b, mina_u8 common_window)
{
    mina_u8 ret;

    if (mina_aac_bits_bit(b) != 0)      /* ics_reserved_bit */
        return 32;
    ics->window_sequence = (mina_u8)mina_aac_bits_read(b, 2);
    ics->window_shape = (mina_u8)mina_aac_bits_bit(b);

    if (ics->window_sequence == MINA_AAC_EIGHT_SHORT) {
        ics->max_sfb = (mina_u8)mina_aac_bits_read(b, 4);
        ics->scale_factor_grouping = (mina_u8)mina_aac_bits_read(b, 7);
    } else {
        ics->max_sfb = (mina_u8)mina_aac_bits_read(b, 6);
    }

    if ((ret = mina_aac_window_grouping(d, ics)) > 0)
        return ret;

    if (ics->max_sfb > ics->num_swb)
        return 16;

    if (ics->window_sequence != MINA_AAC_EIGHT_SHORT)
        (void)mina_aac_bits_bit(b);

    (void)common_window;
    return 0;
}

/* ------------------------------------------------------------------ */
/* section_data() (Table 4.4.25)                                       */
/* ------------------------------------------------------------------ */

static mina_u8 mina_aac_section_data(mina_aac_ctx *d, mina_aac_ics *ics,
                                     mina_aac_bits *b)
{
    mina_u8 g;
    mina_u8 sect_esc_val, sect_bits;
    mina_u8 sect_lim;
    (void)d;

    if (ics->window_sequence == MINA_AAC_EIGHT_SHORT) {
        sect_bits = 3;
        sect_lim = 8 * 15;
    } else {
        sect_bits = 5;
        sect_lim = MINA_AAC_MAX_SFB;
    }
    sect_esc_val = (mina_u8)((1u << sect_bits) - 1u);

    for (g = 0; g < ics->num_window_groups && g < MINA_AAC_MAX_GROUP; g++) {
        mina_u8 k = 0;
        mina_u8 i = 0;

        while (k < ics->max_sfb) {
            mina_u8 sect_len_incr;
            mina_u8 sect_len = 0;
            mina_u8 sfb;

            if (b->error != 0)
                return 14;
            if (i >= sect_lim)
                return 15;

            ics->sect_cb[g][i] = (mina_u8)mina_aac_bits_read(b, 4);
            if (ics->sect_cb[g][i] == 12)
                return 32;

            if (ics->sect_cb[g][i] == MINA_AAC_NOISE_HCB)
                ics->noise_used = 1;
            if (ics->sect_cb[g][i] == MINA_AAC_INTENSITY_HCB2 ||
                ics->sect_cb[g][i] == MINA_AAC_INTENSITY_HCB)
                ics->is_used = 1;

            sect_len_incr = (mina_u8)mina_aac_bits_read(b, sect_bits);
            while (sect_len_incr == sect_esc_val) {
                sect_len = (mina_u8)(sect_len + sect_len_incr);
                if (sect_len > sect_lim)
                    return 15;
                sect_len_incr = (mina_u8)mina_aac_bits_read(b, sect_bits);
            }
            sect_len = (mina_u8)(sect_len + sect_len_incr);

            ics->sect_start[g][i] = k;
            ics->sect_end[g][i] = (mina_u16)(k + sect_len);

            if (sect_len > sect_lim)
                return 15;
            if ((mina_u16)k + sect_len > sect_lim)
                return 15;

            for (sfb = k; sfb < k + sect_len; sfb++)
                ics->sfb_cb[g][sfb] = ics->sect_cb[g][i];

            k = (mina_u8)(k + sect_len);
            i++;
        }
        ics->num_sec[g] = i;

        if (k != ics->max_sfb)
            return 32;
        if (b->error)
            return 32;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* decode_scale_factors()                                              */
/* ------------------------------------------------------------------ */

static mina_u8 mina_aac_scale_factors(mina_aac_ics *ics, mina_aac_bits *b)
{
    mina_u8 g, sfb;
    mina_i16 t;
    mina_i16 scale_factor = (mina_i16)ics->global_gain;
    mina_i16 is_position = 0;
    int noise_pcm_flag = 1;
    mina_i16 noise_energy = (mina_i16)(ics->global_gain - 90);

    for (g = 0; g < ics->num_window_groups; g++) {
        for (sfb = 0; sfb < ics->max_sfb; sfb++) {
            switch (ics->sfb_cb[g][sfb]) {
            case MINA_AAC_ZERO_HCB:
                ics->scale_factors[g][sfb] = 0;
                break;
            case MINA_AAC_INTENSITY_HCB:
            case MINA_AAC_INTENSITY_HCB2:
                t = mina_aac_huff_sf(b);
                t = (mina_i16)(t - 60);
                is_position += t;
                ics->scale_factors[g][sfb] = is_position;
                break;
            case MINA_AAC_NOISE_HCB:
                if (noise_pcm_flag) {
                    noise_pcm_flag = 0;
                    t = (mina_i16)((int)mina_aac_bits_read(b, 9) - 256);
                } else {
                    t = mina_aac_huff_sf(b);
                    t = (mina_i16)(t - 60);
                }
                noise_energy += t;
                ics->scale_factors[g][sfb] = noise_energy;
                break;
            default:
                ics->scale_factors[g][sfb] = 0;
                t = mina_aac_huff_sf(b);
                scale_factor += (mina_i16)(t - 60);
                if (scale_factor < 0 || scale_factor > 255)
                    return 4;
                ics->scale_factors[g][sfb] = (scale_factor < 255) ? scale_factor : 255;
                break;
            }
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* tns_data() (Table 4.4.27)                                           */
/* ------------------------------------------------------------------ */

static void mina_aac_tns_data(mina_aac_ics *ics, mina_aac_bits *b)
{
    mina_u8 w, filt, i, coef_bits;
    mina_u8 n_filt_bits = 2;
    mina_u8 length_bits = 6;
    mina_u8 order_bits = 5;

    if (ics->window_sequence == MINA_AAC_EIGHT_SHORT) {
        n_filt_bits = 1;
        length_bits = 4;
        order_bits = 3;
    }

    for (w = 0; w < ics->num_windows; w++) {
        mina_u8 start_coef_bits = 3;
        ics->tns_n_filt[w] = (mina_u8)mina_aac_bits_read(b, n_filt_bits);

        if (ics->tns_n_filt[w]) {
            ics->tns_coef_res[w] = (mina_u8)mina_aac_bits_bit(b);
            if (ics->tns_coef_res[w] & 1)
                start_coef_bits = 4;
        }

        for (filt = 0; filt < ics->tns_n_filt[w]; filt++) {
            ics->tns_length[w][filt] = (mina_u8)mina_aac_bits_read(b, length_bits);
            ics->tns_order[w][filt] = (mina_u8)mina_aac_bits_read(b, order_bits);
            if (ics->tns_order[w][filt]) {
                ics->tns_direction[w][filt] = (mina_u8)mina_aac_bits_bit(b);
                ics->tns_coef_compress[w][filt] = (mina_u8)mina_aac_bits_bit(b);

                coef_bits = (mina_u8)(start_coef_bits - ics->tns_coef_compress[w][filt]);
                for (i = 0; i < ics->tns_order[w][filt]; i++)
                    ics->tns_coef[w][filt][i] = (mina_u8)mina_aac_bits_read(b, coef_bits);
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* pulse_data() (Table 4.4.7)                                          */
/* ------------------------------------------------------------------ */

static mina_u8 mina_aac_pulse_data(mina_aac_ics *ics, mina_aac_bits *b)
{
    mina_u8 i;

    ics->number_pulse = (mina_u8)mina_aac_bits_read(b, 2);
    ics->pulse_start_sfb = (mina_u8)mina_aac_bits_read(b, 6);

    if (ics->pulse_start_sfb > ics->num_swb)
        return 16;

    for (i = 0; i < ics->number_pulse + 1; i++) {
        ics->pulse_offset[i] = (mina_u8)mina_aac_bits_read(b, 5);
        ics->pulse_amp[i] = (mina_u8)mina_aac_bits_read(b, 4);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* spectral_data (Table 4.4.29)                                        */
/* ------------------------------------------------------------------ */

static mina_u8 mina_aac_spectral_data(mina_aac_ctx *d, mina_aac_ics *ics,
                                      mina_aac_bits *b, mina_i16 *spec)
{
    const int frame_len = 1024;
    mina_i8 i;
    mina_u8 g;
    mina_u16 inc, k, p = 0;
    mina_u8 groups = 0;
    mina_u8 result;
    mina_u16 nshort = (mina_u16)(frame_len / 8);
    (void)d;

    for (g = 0; g < ics->num_window_groups; g++) {
        p = (mina_u16)(groups * nshort);

        for (i = 0; i < (mina_i8)ics->num_sec[g]; i++) {
            mina_u8 sect_cb = ics->sect_cb[g][i];

            inc = (sect_cb >= MINA_AAC_FIRST_PAIR_HCB) ? 2 : 4;

            switch (sect_cb) {
            case MINA_AAC_ZERO_HCB:
            case MINA_AAC_NOISE_HCB:
            case MINA_AAC_INTENSITY_HCB:
            case MINA_AAC_INTENSITY_HCB2:
                p += (mina_u16)(ics->sect_sfb_offset[g][ics->sect_end[g][i]] -
                    ics->sect_sfb_offset[g][ics->sect_start[g][i]]);
                break;
            default:
                for (k = ics->sect_sfb_offset[g][ics->sect_start[g][i]];
                     k < ics->sect_sfb_offset[g][ics->sect_end[g][i]]; k += inc) {
                    if ((result = mina_aac_spectral_huff(sect_cb, b, &spec[p])) > 0)
                        return result;
                    p += inc;
                }
                break;
            }
        }
        groups += ics->window_group_length[g];
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* individual_channel_stream()                                         */
/* ------------------------------------------------------------------ */

static mina_u8 mina_aac_channel_decode(mina_aac_ctx *d, mina_aac_ics *ics,
                                       mina_aac_bits *b, mina_u8 common_window,
                                       mina_i16 *spec, int frame_len)
{
    mina_u8 ret;

    ics->global_gain = (mina_u8)mina_aac_bits_read(b, 8);
    if (b->error)
        return 32;

    if (!common_window) {
            if ((ret = mina_aac_ics_info(d, ics, b, 0)) > 0) {
                return ret;
            }
        }

        if ((ret = mina_aac_section_data(d, ics, b)) > 0) {
            return ret;
        }
        if ((ret = mina_aac_scale_factors(ics, b)) > 0) {
            return ret;
        }

    ics->pulse_data_present = (mina_u8)mina_aac_bits_bit(b);
    if (ics->pulse_data_present) {
        if ((ret = mina_aac_pulse_data(ics, b)) > 0)
            return ret;
    }

    ics->tns_data_present = (mina_u8)mina_aac_bits_bit(b);
    if (ics->tns_data_present)
        mina_aac_tns_data(ics, b);


    if (mina_aac_bits_bit(b))          /* gain_control_data_present */
        return 1;

    if ((ret = mina_aac_spectral_data(d, ics, b, spec)) > 0)
        return ret;


    if (ics->pulse_data_present) {
        if (ics->window_sequence != MINA_AAC_EIGHT_SHORT) {
            if ((ret = mina_aac_pulse_decode(ics, spec, frame_len)) > 0)
                return ret;
        } else {
            return 2;
        }
    }
    return 0;
}

/* ------------------------------ helpers ------------------------------ */

#define MINA_AAC_TNS_MAX_ORDER 20

#define MINA_AAC_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MINA_AAC_MAX(a, b) (((a) > (b)) ? (a) : (b))

/* PNS band classifiers (is.h / pns.h). */
static mina_u8 mina_aac_is_noise(mina_aac_ics *ics, mina_u8 g, mina_u8 sfb)
{
    return (ics->sfb_cb[g][sfb] == MINA_AAC_NOISE_HCB) ? (mina_u8)1 : (mina_u8)0;
}

static mina_i8 mina_aac_is_intensity(mina_aac_ics *ics, mina_u8 g, mina_u8 sfb)
{
    if (ics->sfb_cb[g][sfb] == MINA_AAC_INTENSITY_HCB)
        return 1;
    if (ics->sfb_cb[g][sfb] == MINA_AAC_INTENSITY_HCB2)
        return -1;
    return 0;
}

static mina_i8 mina_aac_invert_intensity(mina_aac_ics *ics, mina_u8 g, mina_u8 sfb)
{
    if (ics->ms_mask_present == 1)
        return (mina_i8)(1 - 2 * ics->ms_used[g][sfb]);
    return 1;
}

static mina_i32 mina_aac_lcg_random(mina_u32 *state)
{
    *state = *state * 1664525u + 1013904223u;
    return (mina_i32)*state;
}

/* max_tns_sfb (common.c), LC object only. */
static mina_u8 mina_aac_max_tns_sfb(mina_u8 sr_index, mina_u8 is_short)
{
    static const mina_u8 tns_sbf_max[13][2] = {
        {31,  9}, {31,  9}, {34, 10}, {40, 14}, {42, 14}, {51, 14},
        {46, 14}, {46, 14}, {42, 14}, {42, 14}, {42, 14}, {39, 14}, {39, 14}
    };
    if (sr_index < 13)
        return tns_sbf_max[sr_index][is_short ? 1 : 0];
    return 0;
}

/* ------------------------------ iquant ------------------------------ */

/* iquant(q) = sign(q) * |q|^(4/3), float path (specrec.c). */
static float mina_aac_iquant(mina_i16 q)
{
    static const float p43[17] = {
        0.0f, 1.0f, 2.5198421f, 4.3267487f, 6.3496042f, 8.5498797f,
        10.902724f, 13.390518f, 16.0f, 18.720755f, 21.544348f,
        24.463781f, 27.473141f, 30.567350f, 33.741993f, 36.993183f,
        40.317474f
    };
    int a = q < 0 ? -q : q;
    float v = a <= 16 ? p43[a] : (float)pow((double)a, 4.0 / 3.0);
    return q < 0 ? -v : v;
}

/* ---------------------------- quant_to_spec ---------------------------- */

/* Dequantise + scale, incl. short-block deinterleaving (specrec.c float). */
static mina_u8 mina_aac_quant_to_spec(mina_aac_ctx *d, mina_aac_ics *ics,
                                      const mina_i16 *quant, float *spec,
                                      int frame_len)
{
    static const float pow2_table[4] = {
        1.0f,
        1.1892071150027210667f,   /* 2^0.25 */
        1.4142135623730950488f,   /* 2^0.5  */
        1.6817928305074290861f    /* 2^0.75 */
    };
    mina_u8 g, sfb, win;
    mina_u16 width, bin, k, gindex;
    mina_u8 error = 0;
    float scf;

    (void)d;

    k = 0;
    gindex = 0;

    memset(spec, 0, (size_t)frame_len * sizeof(float));

    for (g = 0; g < ics->num_window_groups; g++)
    {
        mina_u16 j = 0;
        mina_u16 gincrease = 0;
        mina_u16 win_inc = ics->swb_offset[ics->num_swb];

        for (sfb = 0; sfb < ics->num_swb; sfb++)
        {
            mina_u16 wa = (mina_u16)(gindex + j);
            mina_u16 wb;
            mina_i16 scale_factor = ics->scale_factors[g][sfb];
            int exp, frac;

            width = (mina_u16)(ics->swb_offset[sfb + 1] - ics->swb_offset[sfb]);
            if (width + 3 >= 1024)
            {
                error = 17;
                continue;
            }

            /* IS / PNS bands use their scale factor differently. */
            if (mina_aac_is_intensity(ics, g, sfb) || mina_aac_is_noise(ics, g, sfb))
                scale_factor = 0;

            exp = scale_factor >> 2;
            frac = scale_factor & 3;

            /* scf = 2^(exp-25) * 2^(frac/4); ldexp is exact for the 2^exp part. */
            scf = (float)ldexp((double)pow2_table[frac], exp - 25);

            for (win = 0; win < ics->window_group_length[g]; win++)
            {
                for (bin = 0; bin < width; bin += 4)
                {
                    wb = (mina_u16)(wa + bin);
                    spec[wb + 0] = mina_aac_iquant(quant[k + 0]) * scf;
                    spec[wb + 1] = mina_aac_iquant(quant[k + 1]) * scf;
                    spec[wb + 2] = mina_aac_iquant(quant[k + 2]) * scf;
                    spec[wb + 3] = mina_aac_iquant(quant[k + 3]) * scf;

                    gincrease = (mina_u16)(gincrease + 4);
                    k = (mina_u16)(k + 4);
                }
                wa = (mina_u16)(wa + win_inc);
            }
            j = (mina_u16)(j + width);
        }
        gindex = (mina_u16)(gindex + gincrease);
    }

    return error;
}

/* ------------------------------ PNS ------------------------------ */

/* gen_rand_vector (pns.c float path). */
static void mina_aac_gen_rand_vector(float *spec, mina_i16 scale_factor,
                                     mina_u16 size, mina_u32 *state)
{
    mina_u16 i;
    float energy = 0.0f;
    float scale;

    if (scale_factor < -120)
        scale_factor = -120;
    else if (scale_factor > 120)
        scale_factor = 120;

    for (i = 0; i < size; i++)
    {
        float tmp = (float)mina_aac_lcg_random(state);
        spec[i] = tmp;
        energy += tmp * tmp;
    }

    if (energy > 0)
    {
        scale = 1.0f / (float)sqrt((double)energy);
        scale *= (float)pow(2.0, 0.25 * (double)scale_factor);
        for (i = 0; i < size; i++)
            spec[i] *= scale;
    }
}

static void mina_aac_pns_decode_channel(mina_aac_ctx *d, mina_aac_ics *ics,
                                        float *spec)
{
    mina_u8 g, sfb, b;
    mina_u16 begin, end, base;
    mina_u16 group = 0;
    mina_u16 nshort = 128;

    for (g = 0; g < ics->num_window_groups; g++)
    {
        for (b = 0; b < ics->window_group_length[g]; b++)
        {
            base = (mina_u16)(group * nshort);

            for (sfb = 0; sfb < ics->max_sfb; sfb++)
            {
                if (mina_aac_is_noise(ics, g, sfb))
                {
                    begin = (mina_u16)(base + MINA_AAC_MIN(ics->swb_offset[sfb],
                                                           ics->swb_offset_max));
                    end = (mina_u16)(base + MINA_AAC_MIN(ics->swb_offset[sfb + 1],
                                                         ics->swb_offset_max));

                    mina_aac_gen_rand_vector(&spec[begin], ics->scale_factors[g][sfb],
                                             (mina_u16)(end - begin), &d->random_state);
                }
            }
            group++;
        }
    }
}

/* ------------------------------ MS ------------------------------ */

static void mina_aac_ms_decode(mina_aac_ics *ics1, mina_aac_ics *ics2,
                               float *s1, float *s2, int frame_len)
{
    mina_u8 g, b, sfb;
    mina_u16 group = 0;
    mina_u16 nshort = (mina_u16)(frame_len >> 3);
    mina_u16 i, k;
    float tmp;

    if (ics1->ms_mask_present >= 1)
    {
        for (g = 0; g < ics1->num_window_groups; g++)
        {
            for (b = 0; b < ics1->window_group_length[g]; b++)
            {
                for (sfb = 0; sfb < ics1->max_sfb; sfb++)
                {
                    if ((ics1->ms_used[g][sfb] || ics1->ms_mask_present == 2) &&
                        !mina_aac_is_intensity(ics2, g, sfb) &&
                        !mina_aac_is_noise(ics1, g, sfb))
                    {
                        for (i = ics1->swb_offset[sfb];
                             i < MINA_AAC_MIN(ics1->swb_offset[sfb + 1],
                                              ics1->swb_offset_max); i++)
                        {
                            k = (mina_u16)((group * nshort) + i);
                            tmp = s1[k] - s2[k];
                            s1[k] = s1[k] + s2[k];
                            s2[k] = tmp;
                        }
                    }
                }
                group++;
            }
        }
    }
}

/* ------------------------------ IS ------------------------------ */

static void mina_aac_is_decode(mina_aac_ics *ics1, mina_aac_ics *ics2,
                               float *s1, float *s2, int frame_len)
{
    mina_u8 g, sfb, b;
    mina_u16 i;
    float scale;
    float lv;
    mina_u16 nshort = (mina_u16)(frame_len >> 3);
    mina_u16 group = 0;
    mina_i16 scale_factor;

    for (g = 0; g < ics2->num_window_groups; g++)
    {
        for (b = 0; b < ics2->window_group_length[g]; b++)
        {
            for (sfb = 0; sfb < ics2->max_sfb; sfb++)
            {
                if (mina_aac_is_intensity(ics2, g, sfb))
                {
                    scale_factor = ics2->scale_factors[g][sfb];
                    if (scale_factor < -120)
                        scale_factor = -120;
                    else if (scale_factor > 120)
                        scale_factor = 120;

                    scale = (float)pow(0.5, 0.25 * (double)scale_factor);

                    for (i = ics2->swb_offset[sfb];
                         i < MINA_AAC_MIN(ics2->swb_offset[sfb + 1],
                                          ics1->swb_offset_max); i++)
                    {
                        lv = s1[(mina_u16)((group * nshort) + i)];
                        s2[(mina_u16)((group * nshort) + i)] = lv * scale;
                        if (mina_aac_is_intensity(ics2, g, sfb) !=
                            mina_aac_invert_intensity(ics1, g, sfb))
                            s2[(mina_u16)((group * nshort) + i)] =
                                -s2[(mina_u16)((group * nshort) + i)];
                    }
                }
            }
            group++;
        }
    }
}

/* ------------------------------ TNS ------------------------------ */

static const float mina_aac_tns_coef_0_3[16] = {
    0.0f, 0.4338837391f, 0.7818314825f, 0.9749279122f,
    -0.9848077530f, -0.8660254038f, -0.6427876097f, -0.3420201433f,
    -0.4338837391f, -0.7818314825f, -0.9749279122f, -0.9749279122f,
    -0.9848077530f, -0.8660254038f, -0.6427876097f, -0.3420201433f
};
static const float mina_aac_tns_coef_0_4[16] = {
    0.0f, 0.2079116908f, 0.4067366431f, 0.5877852523f,
    0.7431448255f, 0.8660254038f, 0.9510565163f, 0.9945218954f,
    -0.9957341763f, -0.9618256432f, -0.8951632914f, -0.7980172273f,
    -0.6736956436f, -0.5264321629f, -0.3612416662f, -0.1837495178f
};
static const float mina_aac_tns_coef_1_3[16] = {
    0.0f, 0.4338837391f, -0.6427876097f, -0.3420201433f,
    0.9749279122f, 0.7818314825f, -0.6427876097f, -0.3420201433f,
    -0.4338837391f, -0.7818314825f, -0.6427876097f, -0.3420201433f,
    -0.7818314825f, -0.4338837391f, -0.6427876097f, -0.3420201433f
};
static const float mina_aac_tns_coef_1_4[16] = {
    0.0f, 0.2079116908f, 0.4067366431f, 0.5877852523f,
    -0.6736956436f, -0.5264321629f, -0.3612416662f, -0.1837495178f,
    0.9945218954f, 0.9510565163f, 0.8660254038f, 0.7431448255f,
    -0.6736956436f, -0.5264321629f, -0.3612416662f, -0.1837495178f
};
static const float *const mina_aac_all_tns_coefs[4] = {
    mina_aac_tns_coef_0_3, mina_aac_tns_coef_0_4,
    mina_aac_tns_coef_1_3, mina_aac_tns_coef_1_4
};

/* tns_decode_coef (tns.c); returns exp (always 0 in the float path). */
static mina_u8 mina_aac_tns_decode_coef(mina_u8 order, mina_u8 coef_res_bits,
                                        mina_u8 coef_compress,
                                        const mina_u8 *coef, float *a)
{
    mina_u8 i, m, table_index;
    float tmp2[MINA_AAC_TNS_MAX_ORDER + 1];
    float b[MINA_AAC_TNS_MAX_ORDER + 1];
    const float *tns_coef_ptr;

    table_index = (mina_u8)(2 * (coef_compress != 0) + (coef_res_bits != 3));
    tns_coef_ptr = mina_aac_all_tns_coefs[table_index];

    for (i = 0; i < order; i++)
        tmp2[i] = tns_coef_ptr[coef[i]];

    a[0] = 1.0f;
    for (m = 1; m <= order; m++)
    {
        a[m] = tmp2[m - 1];
        for (i = 1; i < m; i++)
            b[i] = a[i] + (a[m] * a[m - i]);

        for (i = 1; i < m; i++)
            a[i] = b[i];
    }

    return 0;
}

/* tns_ar_filter (tns.c), float path with double-ringbuffer state. */
static void mina_aac_tns_ar_filter(float *spectrum, mina_u16 size, mina_i8 inc,
                                   const float *lpc, mina_u8 order, mina_u8 exp)
{
    mina_u8 j;
    mina_u16 i;
    mina_i8 state_index = 0;
    float state[2 * MINA_AAC_TNS_MAX_ORDER];
    float y;

    (void)exp;
    memset(state, 0, sizeof(state));

    for (i = 0; i < size; i++)
    {
        y = 0.0f;
        for (j = 0; j < order; j++)
            y += state[state_index + j] * lpc[j + 1];
        y = *spectrum - y;

        state_index--;
        if (state_index < 0)
            state_index = (mina_i8)(order - 1);
        state[state_index] = y;
        state[state_index + order] = y;

        *spectrum = y;
        spectrum += inc;
    }
}

/* tns_decode_frame (tns.c float path). */
static void mina_aac_tns_decode_frame(mina_aac_ctx *d, mina_aac_ics *ics,
                                      float *spec, int frame_len)
{
    mina_u8 w, f, tns_order, exp, tns_sfb;
    mina_i8 inc;
    mina_i16 size;
    mina_u16 bottom, top, start, end;
    mina_u16 nshort = (mina_u16)(frame_len >> 3);
    float lpc[MINA_AAC_TNS_MAX_ORDER + 1];

    if (!ics->tns_data_present)
        return;

    for (w = 0; w < ics->num_windows; w++)
    {
        bottom = ics->num_swb;

        for (f = 0; f < ics->tns_n_filt[w]; f++)
        {
            top = bottom;
            bottom = MINA_AAC_MAX((mina_u16)(top - ics->tns_length[w][f]),
                                  (mina_u16)0);
            tns_order = ics->tns_order[w][f];
            if (tns_order > MINA_AAC_TNS_MAX_ORDER)
                tns_order = MINA_AAC_TNS_MAX_ORDER;
            if (!tns_order)
                continue;

            exp = mina_aac_tns_decode_coef(tns_order,
                                           (mina_u8)(ics->tns_coef_res[w] + 3),
                                           ics->tns_coef_compress[w][f],
                                           ics->tns_coef[w][f], lpc);

            tns_sfb = mina_aac_max_tns_sfb(d->sf_index,
                                           (ics->window_sequence ==
                                            MINA_AAC_EIGHT_SHORT) ? (mina_u8)1
                                                                   : (mina_u8)0);

            start = MINA_AAC_MIN(bottom, tns_sfb);
            start = MINA_AAC_MIN(start, ics->max_sfb);
            start = MINA_AAC_MIN(ics->swb_offset[start], ics->swb_offset_max);

            end = MINA_AAC_MIN(top, tns_sfb);
            end = MINA_AAC_MIN(end, ics->max_sfb);
            end = MINA_AAC_MIN(ics->swb_offset[end], ics->swb_offset_max);

            size = (mina_i16)(end - start);
            if (size <= 0)
                continue;

            if (ics->tns_direction[w][f])
            {
                inc = -1;
                start = (mina_u16)(end - 1);
            }
            else
            {
                inc = 1;
            }

            mina_aac_tns_ar_filter(&spec[(mina_u16)((w * nshort) + start)],
                                   (mina_u16)size, inc, lpc, tns_order, exp);
        }
    }
}

/* ------------------------------ PULSE ------------------------------ */

static mina_u8 mina_aac_pulse_decode(mina_aac_ics *ics, mina_i16 *spec,
                                     int frame_len)
{
    mina_u8 i;
    mina_u16 k;

    k = (mina_u16)MINA_AAC_MIN(ics->swb_offset[ics->pulse_start_sfb],
                               ics->swb_offset_max);

    for (i = 0; i <= ics->number_pulse; i++)
    {
        k = (mina_u16)(k + ics->pulse_offset[i]);

        if (k >= (mina_u16)frame_len)
            return 15;

        if (spec[k] > 0)
            spec[k] = (mina_i16)(spec[k] + ics->pulse_amp[i]);
        else
            spec[k] = (mina_i16)(spec[k] - ics->pulse_amp[i]);
    }

    return 0;
}

/* --------------------------- reconstruct --------------------------- */

static mina_u8 mina_aac_reconstruct_single(mina_aac_ctx *d, mina_aac_ics *ics,
                                           mina_i16 *spec, int ch)
{
    mina_u8 retval;
    float spec_coef[1024];

    retval = mina_aac_quant_to_spec(d, ics, spec, spec_coef, 1024);
    if (retval != 0)
        return retval;

    mina_aac_pns_decode_channel(d, ics, spec_coef);
    mina_aac_tns_decode_frame(d, ics, spec_coef, 1024);

    mina_aac_ifilter_bank(d, ics->window_sequence, ics->window_shape,
                          d->window_shape_prev[ch], spec_coef,
                          d->time_out[ch], d->fb_intermed[ch]);

    d->window_shape_prev[ch] = ics->window_shape;

    return 0;
}

static mina_u8 mina_aac_reconstruct_pair(mina_aac_ctx *d, mina_aac_ics *ics1,
                                         mina_aac_ics *ics2, mina_i16 *s1,
                                         mina_i16 *s2, int ch0, int ch1)
{
    mina_u8 retval;
    float spec_coef1[1024];
    float spec_coef2[1024];

    retval = mina_aac_quant_to_spec(d, ics1, s1, spec_coef1, 1024);
    if (retval != 0)
        return retval;
    retval = mina_aac_quant_to_spec(d, ics2, s2, spec_coef2, 1024);
    if (retval != 0)
        return retval;

    mina_aac_pns_decode_channel(d, ics1, spec_coef1);
    mina_aac_pns_decode_channel(d, ics2, spec_coef2);
    mina_aac_ms_decode(ics1, ics2, spec_coef1, spec_coef2, 1024);
    mina_aac_is_decode(ics1, ics2, spec_coef1, spec_coef2, 1024);
    mina_aac_tns_decode_frame(d, ics1, spec_coef1, 1024);
    mina_aac_tns_decode_frame(d, ics2, spec_coef2, 1024);

    mina_aac_ifilter_bank(d, ics1->window_sequence, ics1->window_shape,
                          d->window_shape_prev[ch0], spec_coef1,
                          d->time_out[ch0], d->fb_intermed[ch0]);
    mina_aac_ifilter_bank(d, ics2->window_sequence, ics2->window_shape,
                          d->window_shape_prev[ch1], spec_coef2,
                          d->time_out[ch1], d->fb_intermed[ch1]);

    d->window_shape_prev[ch0] = ics1->window_shape;
    d->window_shape_prev[ch1] = ics2->window_shape;

    return 0;
}
/* ------------------------------------------------------------------ */
/* ADTS fixed header (ISO/IEC 14496-3, Table 1.A / 4.4.31)             */
/*                                                                     */
/*   byte0: syncword (0xFF)                                            */
/*   byte1: syncword[3:0]=1111 | ID(1) | layer(2) | protection_absent  */
/*   byte2: profile(2) | sampling_frequency_index(4) | private(1)      */
/*          | channel_configuration(1)                                 */
/*   byte3: channel_configuration(2 bits) | original(1) | home(1)      */
/*          | copyright_id(1) | copyright_start(1) | frame_length(2)   */
/*   byte4: frame_length(8)                                            */
/*   byte5: frame_length(3) | buffer_fullness(5)                       */
/*   byte6: buffer_fullness(6) | num_raw_data_blocks_in_frame(2)       */
/* ------------------------------------------------------------------ */

/* output sampling-rate table indexed by sf_index (0..12) */
typedef struct mina_aac_adts {
    mina_u8  sf_index;    /* sampling frequency index                 */
    mina_u8  chans_cfg;   /* channel_configuration (0..7)             */
    mina_u16 frame_len;   /* total frame length incl. header + CRC    */
    int      have_crc;    /* 1 when protection_absent == 0 (2 CRC b)  */
} mina_aac_adts;

/* Parse a complete ADTS header at data[pos].  Returns 0 and fills *h on
 * success, -1 on an invalid header (caller may then resync). */
static int mina_aac_adts_frame(const mina_u8 *data, size_t n, size_t pos,
                               mina_aac_adts *h)
{
    mina_u16 header_len = 7;
    mina_u32 sfi;
    mina_u16 flen;
    const mina_u8 *d;

    if (pos + 7 > n)
        return -1;

    d = data + pos;

    /* syncword, MPEG-4 (ID=0) Audio, layer=0 (ADTS) */
    if (d[0] != 0xFFu)
        return -1;
    if ((d[1] & 0xF6u) != 0xF0u)
        return -1;
    if ((d[2] >> 6) != 1u)
        return -1;

    sfi = (mina_u32)((d[2] >> 2) & 0x0Fu);
    if (sfi > 12u)
        return -1;

    h->sf_index  = (mina_u8)sfi;
    h->chans_cfg = (mina_u8)(((d[2] & 0x01u) << 2) | (d[3] >> 6));
    if (h->chans_cfg > 7u)
        return -1;

    h->have_crc = ((d[1] & 0x01u) == 0u) ? 1 : 0;
    if (h->have_crc)
        header_len = 9;

    flen = (mina_u16)(((d[3] & 0x03u) << 11) |
                      ((mina_u16)d[4] << 3) |
                      ((mina_u16)(d[5] >> 5)));
    if (flen < header_len)
        return -1;
    if (pos + (size_t)flen > n)
        return -1;

    h->frame_len = flen;
    return 0;
}

/* ------------------------------------------------------------------ */
/* program_config_element(): replicate consumption (Table 4.4.3.1)     */
/* ------------------------------------------------------------------ */
static void mina_aac_pce(mina_aac_bits *b)
{
    mina_u8 num_front, num_side, num_back, num_lfe, num_assoc, num_cc;
    mina_u8 i;

    (void)mina_aac_bits_read(b, 4);              /* element_instance_tag */
    (void)mina_aac_bits_read(b, 2);              /* object_type          */
    (void)mina_aac_bits_read(b, 4);              /* sampling_frequency   */
    num_front = (mina_u8)mina_aac_bits_read(b, 4);
    num_side  = (mina_u8)mina_aac_bits_read(b, 4);
    num_back  = (mina_u8)mina_aac_bits_read(b, 4);
    num_lfe   = (mina_u8)mina_aac_bits_read(b, 2);
    num_assoc = (mina_u8)mina_aac_bits_read(b, 3);
    num_cc    = (mina_u8)mina_aac_bits_read(b, 4);

    if (mina_aac_bits_bit(b)) {                  /* mono_mixdown_present */
        (void)mina_aac_bits_read(b, 4);
    }
    if (mina_aac_bits_bit(b)) {                  /* stereo_mixdown_present */
        (void)mina_aac_bits_read(b, 4);
    }
    if (mina_aac_bits_bit(b)) {                  /* matrix_mixdown_idx_present */
        (void)mina_aac_bits_read(b, 2);
        (void)mina_aac_bits_bit(b);
    }

    for (i = 0; i < num_front; i++) {
        (void)mina_aac_bits_bit(b);              /* front_element_is_cpe */
        (void)mina_aac_bits_read(b, 4);          /* tag */
    }
    for (i = 0; i < num_side; i++) {
        (void)mina_aac_bits_bit(b);              /* side_element_is_cpe */
        (void)mina_aac_bits_read(b, 4);          /* tag */
    }
    for (i = 0; i < num_back; i++) {
        (void)mina_aac_bits_bit(b);              /* back_element_is_cpe */
        (void)mina_aac_bits_read(b, 4);          /* tag */
    }
    for (i = 0; i < num_lfe; i++) {
        (void)mina_aac_bits_read(b, 4);          /* lfe_element_tag */
    }
    for (i = 0; i < num_assoc; i++) {
        (void)mina_aac_bits_read(b, 4);          /* assoc_data_element_tag */
    }
    for (i = 0; i < num_cc; i++) {
        (void)mina_aac_bits_bit(b);              /* cc_element_is_ind_sw */
        (void)mina_aac_bits_read(b, 4);          /* tag */
    }

    mina_aac_bits_align(b);
    {
        mina_u8 comment_field_bytes = (mina_u8)mina_aac_bits_read(b, 8);
        for (i = 0; i < comment_field_bytes; i++)
            (void)mina_aac_bits_read(b, 8);
    }
}

/* ------------------------------------------------------------------ */
/* data_stream_element(): skip (Table 4.4.10)                          */
/* ------------------------------------------------------------------ */
static void mina_aac_dse(mina_aac_bits *b)
{
    mina_u16 count;
    int byte_aligned;

    (void)mina_aac_bits_read(b, MINA_AAC_LEN_TAG);   /* element_instance_tag */
    byte_aligned = mina_aac_bits_bit(b);
    count = (mina_u16)mina_aac_bits_read(b, 8);
    if (count == 255)
        count = (mina_u16)(count + mina_aac_bits_read(b, 8));

    if (byte_aligned)
        mina_aac_bits_align(b);

    {
        mina_u16 i;
        for (i = 0; i < count; i++)
            (void)mina_aac_bits_read(b, 8);
    }
}

/* ------------------------------------------------------------------ */
/* fill_element(): consume the fill payload (Table 4.4.11).  SBR       */
/* extension data is rejected (LC-only decoder, error 24 like FAAD     */
/* when sbr_ele == INVALID_SBR_ELEMENT).                               */
/* ------------------------------------------------------------------ */
static mina_u8 mina_aac_fill(mina_aac_bits *b)
{
    mina_u16 count;

    count = (mina_u16)mina_aac_bits_read(b, 4);
    if (count == 15)
        count = (mina_u16)(count + mina_aac_bits_read(b, 8) - 1);

    if (count > 0)
        mina_aac_bits_flush(b, (int)(count * 8));
    return 0;
}

/* ------------------------------------------------------------------ */
/* raw_data_block(): Table 4.4.3.  Decodes the element sequence into   */
/* d->time_out[0..nch) and returns MINA_OK or an error code.           */
/* ------------------------------------------------------------------ */
static mina_u8 mina_aac_raw_data_block(mina_aac_ctx *d, mina_aac_bits *b)
{
    const int frame_len = 1024;
    mina_u8 nch = 0;
    mina_u8 ele_this_frame = 0;
    mina_u8 id_syn_ele;

    for (;;) {
        id_syn_ele = (mina_u8)mina_aac_bits_read(b, MINA_AAC_LEN_SE_ID);
        if (b->error) {
            return 32;
        }
        if (id_syn_ele == MINA_AAC_ID_END)
            break;

        switch (id_syn_ele) {
        case MINA_AAC_ID_SCE:
        case MINA_AAC_ID_LFE: {
            mina_aac_ics ics;
            mina_i16 spec[1024];
            mina_u8 ret;

            if ((mina_u16)nch + 1 > d->alloc_chans)
                return 12;

            memset(&ics, 0, sizeof(ics));
            memset(spec, 0, sizeof(spec));

            (void)mina_aac_bits_read(b, MINA_AAC_LEN_TAG);

            ret = mina_aac_channel_decode(d, &ics, b, 0, spec, frame_len);
            if (ret != 0) {
                return ret;
            }

            /* IS not allowed in a single channel element */
            if (ics.is_used)
                return 32;

            ret = mina_aac_reconstruct_single(d, &ics, spec, (int)nch);
            if (ret != 0)
                return ret;

            nch++;
            ele_this_frame++;
            break;
        }
        case MINA_AAC_ID_CPE: {
            mina_aac_ics ics1, ics2;
            mina_i16 s1[1024], s2[1024];
            mina_u8 ret;
            mina_u8 common_window;

            if ((mina_u16)nch + 2 > d->alloc_chans)
                return 12;

            memset(&ics1, 0, sizeof(ics1));
            memset(&ics2, 0, sizeof(ics2));
            memset(s1, 0, sizeof(s1));
            memset(s2, 0, sizeof(s2));

            (void)mina_aac_bits_read(b, MINA_AAC_LEN_TAG);

            common_window = (mina_u8)mina_aac_bits_bit(b);
            if (common_window) {
                mina_u8 g, sfb;

                ret = mina_aac_ics_info(d, &ics1, b, 1);
                if (ret != 0)
                    return ret;

                ics1.ms_mask_present = (mina_u8)mina_aac_bits_read(b, 2);
                if (ics1.ms_mask_present == 3)
                    return 32;
                if (ics1.ms_mask_present == 1) {
                    for (g = 0; g < ics1.num_window_groups; g++) {
                        for (sfb = 0; sfb < ics1.max_sfb; sfb++) {
                            ics1.ms_used[g][sfb] = (mina_u8)mina_aac_bits_bit(b);
                        }
                    }
                }
                memcpy(&ics2, &ics1, sizeof(mina_aac_ics));
            } else {
                ics1.ms_mask_present = 0;
            }

            ret = mina_aac_channel_decode(d, &ics1, b, common_window, s1,
                                          frame_len);
            if (ret != 0) {
                return ret;
            }
            ret = mina_aac_channel_decode(d, &ics2, b, common_window, s2,
                                          frame_len);
            if (ret != 0) {
                return ret;
            }

            ret = mina_aac_reconstruct_pair(d, &ics1, &ics2, s1, s2,
                                            (int)nch, (int)nch + 1);
            if (ret != 0)
                return ret;

            nch += 2;
            ele_this_frame++;
            break;
        }
        case MINA_AAC_ID_CCE:
            return 6;

        case MINA_AAC_ID_DSE:
            mina_aac_dse(b);
            ele_this_frame++;
            break;

        case MINA_AAC_ID_PCE:
            if (ele_this_frame != 0)
                return 31;
            ele_this_frame++;
            mina_aac_pce(b);
            break;

        case MINA_AAC_ID_FIL: {
            mina_u8 ret = mina_aac_fill(b);
            if (ret != 0)
                return ret;
            ele_this_frame++;
            break;
        }
        default:
            return 32;
        }

        if (b->error)
            return 32;
    }

    /* end of data streams */
    if (!b->error)
        mina_aac_bits_align(b);
    if (b->error)
        return 32;

    return (nch == 0) ? 32 : 0;
}

typedef struct mina_mp4_aac_track {
    const mina_u8 *stsz, *stsc, *stco, *co64;
    size_t stsz_n, stsc_n, stco_n, co64_n;
    mina_u32 rate, channels, object_type;
} mina_mp4_aac_track;

static int mina_mp4_direct_box(const mina_u8 *d, size_t n, size_t begin,
                               size_t end, const char *wanted,
                               size_t *body, size_t *body_end)
{
    size_t cursor = begin, b, e;
    const mina_u8 *type;
    while (mina_mp4_next_box(d, n, &cursor, end, &type, &b, &e)) {
        if (memcmp(type, wanted, 4) == 0) {
            *body = b; *body_end = e; return 1;
        }
    }
    return 0;
}

static int mina_mp4_desc_size(const mina_u8 **pp, const mina_u8 *end, size_t *size)
{
    const mina_u8 *p = *pp;
    size_t v = 0;
    int i;
    for (i = 0; i < 4; i++) {
        mina_u8 b;
        if (p >= end) return 0;
        b = *p++;
        v = (v << 7) | (b & 0x7fu);
        if (!(b & 0x80u)) {
            if (v > (size_t)(end - p)) return 0;
            *pp = p; *size = v; return 1;
        }
    }
    return 0;
}

static mina_u32 mina_mp4_esds_object_type(const mina_u8 *p, size_t n)
{
    const mina_u8 *end = p + n, *lim;
    size_t z;
    mina_u8 flags;
    if (n < 5) return 0;
    p += 4;
    if (p >= end || *p++ != 3 || !mina_mp4_desc_size(&p, end, &z)) return 0;
    lim = p + z;
    if (lim > end || lim - p < 3) return 0;
    p += 2; flags = *p++;
    if (flags & 0x80u) { if (lim - p < 2) return 0; p += 2; }
    if (flags & 0x40u) { mina_u8 u; if (p >= lim) return 0; u = *p++; if (u > lim - p) return 0; p += u; }
    if (flags & 0x20u) { if (lim - p < 2) return 0; p += 2; }
    if (p >= lim || *p++ != 4 || !mina_mp4_desc_size(&p, lim, &z) || z < 13) return 0;
    lim = p + z;
    if (lim > end || *p != 0x40u) return 0;
    p += 13;
    if (p >= lim || *p++ != 5 || !mina_mp4_desc_size(&p, lim, &z) || !z) return 0;
    return (mina_u32)(p[0] >> 3);
}

static int mina_mp4_aac_track_find(const mina_u8 *d, size_t n,
                                   mina_mp4_aac_track *t)
{
    size_t moov, moov_end, cursor, trak, trak_end;
    const mina_u8 *type;
    memset(t, 0, sizeof(*t));
    if (!mina_mp4_direct_box(d, n, 0, n, "moov", &moov, &moov_end)) return 0;
    cursor = moov;
    while (mina_mp4_next_box(d, n, &cursor, moov_end, &type, &trak, &trak_end)) {
        size_t mdia, mdia_end, hdlr, hdlr_end, minf, minf_end;
        size_t stbl, stbl_end, p, e, stsd, stsd_end, entry_off, entry_end;
        size_t child_off, esds, esds_end;
        const mina_u8 *entry;
        mina_u32 entry_size;
        mina_u16 version;
        if (memcmp(type, "trak", 4) != 0) continue;
        if (!mina_mp4_direct_box(d, n, trak, trak_end, "mdia", &mdia, &mdia_end)) continue;
        if (!mina_mp4_direct_box(d, n, mdia, mdia_end, "hdlr", &hdlr, &hdlr_end)) continue;
        if (hdlr_end - hdlr < 12 || memcmp(d + hdlr + 8, "soun", 4) != 0) continue;
        if (!mina_mp4_direct_box(d, n, mdia, mdia_end, "minf", &minf, &minf_end)) continue;
        if (!mina_mp4_direct_box(d, n, minf, minf_end, "stbl", &stbl, &stbl_end)) continue;
        if (!mina_mp4_direct_box(d, n, stbl, stbl_end, "stsd", &stsd, &stsd_end)) continue;
        if (stsd_end - stsd < 44) continue;
        entry_off = stsd + 8;
        entry = d + entry_off;
        if (memcmp(entry + 4, "mp4a", 4) != 0) continue;
        entry_size = mina_be32(entry);
        if (entry_size < 36 || entry_size > stsd_end - entry_off) continue;
        entry_end = entry_off + entry_size;
        version = mina_be16(entry + 16);
        child_off = entry_off + 36 + (version == 1 ? 16 : version == 2 ? 36 : 0);
        if (child_off > entry_end ||
            !mina_mp4_direct_box(d, n, child_off, entry_end, "esds", &esds, &esds_end)) continue;
        t->object_type = mina_mp4_esds_object_type(d + esds, esds_end - esds);
        if (!t->object_type) continue;
        t->channels = mina_be16(entry + 24);
        t->rate = mina_be32(entry + 32) >> 16;
        if (!mina_mp4_direct_box(d, n, stbl, stbl_end, "stsz", &p, &e)) continue;
        t->stsz = d + p; t->stsz_n = e - p;
        if (!mina_mp4_direct_box(d, n, stbl, stbl_end, "stsc", &p, &e)) continue;
        t->stsc = d + p; t->stsc_n = e - p;
        if (mina_mp4_direct_box(d, n, stbl, stbl_end, "stco", &p, &e)) {
            t->stco = d + p; t->stco_n = e - p;
        } else if (mina_mp4_direct_box(d, n, stbl, stbl_end, "co64", &p, &e)) {
            t->co64 = d + p; t->co64_n = e - p;
        } else continue;
        return t->rate != 0 && t->channels != 0;
    }
    return 0;
}

static mina_result mina_aac_decode(const mina_u8 *data, size_t n,
                                   mina_pcm *out)
{
    mina_aac_ctx d;
    mina_fbuf fb;
    size_t pos = 0;
    mina_u32 rate = 0;
    mina_u32 nch = 0;
    mina_result result = MINA_OK;
    mina_aac_adts first_header;

    if (n >= 12 && memcmp(data + 4, "ftyp", 4) == 0) {
        mina_fileinfo info;
        mina_mp4_aac_track track;
        const mina_u8 *stsz, *stsc, *offsets;
        size_t stsz_n, stsc_n, offsets_n;
        mina_u32 sample_size, sample_count, rate_index = 13;
        mina_u32 stsc_count, chunk_count, sample = 0, sc = 0;
        size_t total = 0, dst = 0;
        mina_u8 *adts;
        mina_u32 i, chunk;
        int use64;

        mina_aac_info(data, n, &info);
        if (!mina_mp4_aac_track_find(data, n, &track))
            return MINA_ERR_UNSUPPORTED;
        stsz = track.stsz; stsz_n = track.stsz_n;
        stsc = track.stsc; stsc_n = track.stsc_n;
        use64 = track.co64 != NULL;
        offsets = use64 ? track.co64 : track.stco;
        offsets_n = use64 ? track.co64_n : track.stco_n;
        if (stsz_n < 12 || stsc_n < 8 || offsets_n < 8)
            return MINA_ERR_INVALID;
        sample_size = mina_be32(stsz + 4);
        sample_count = mina_be32(stsz + 8);
        if (!sample_size && ((size_t)sample_count > (stsz_n - 12) / 4))
            return MINA_ERR_INVALID;
        stsc_count = mina_be32(stsc + 4);
        chunk_count = mina_be32(offsets + 4);
        if (!stsc_count || (size_t)stsc_count > (stsc_n - 8) / 12 ||
            (size_t)chunk_count > (offsets_n - 8) / (use64 ? 8u : 4u))
            return MINA_ERR_INVALID;
        for (i = 0; i < 13; i++) if (g_mina_aac_rates[i] == track.rate) rate_index = i;
        if (rate_index >= 13 || track.channels > 7 || track.object_type != 2)
            return MINA_ERR_UNSUPPORTED;
        for (i = 0; i < sample_count; i++) {
            mina_u32 z = sample_size ? sample_size : mina_be32(stsz + 12 + (size_t)i * 4);
            if (z > 8184 || total > (size_t)-1 - z - 7)
                return MINA_ERR_INVALID;
            total += z + 7;
        }
        adts = (mina_u8 *)MINA_MALLOC(total ? total : 1);
        if (!adts) return MINA_ERR_NOMEM;
        for (chunk = 0; chunk < chunk_count && sample < sample_count; chunk++) {
            mina_u32 spc, j;
            size_t off;
            while (sc + 1 < stsc_count &&
                   mina_be32(stsc + 8 + (size_t)(sc + 1) * 12) <= chunk + 1) sc++;
            if (mina_be32(stsc + 8 + (size_t)sc * 12) > chunk + 1) break;
            spc = mina_be32(stsc + 12 + (size_t)sc * 12);
            off = use64 ? mina_u64_size(mina_be64(offsets + 8 + (size_t)chunk * 8), NULL)
                        : (size_t)mina_be32(offsets + 8 + (size_t)chunk * 4);
            for (j = 0; j < spc && sample < sample_count; j++, sample++) {
                mina_u32 z = sample_size ? sample_size :
                    mina_be32(stsz + 12 + (size_t)sample * 4);
                mina_u32 flen = z + 7;
                if (off > n || z > n - off) { MINA_FREE(adts); return MINA_ERR_INVALID; }
                adts[dst] = 0xff;
                adts[dst + 1] = 0xf1;
                adts[dst + 2] = (mina_u8)(0x40u | (rate_index << 2) | (track.channels >> 2));
                adts[dst + 3] = (mina_u8)(((track.channels & 3u) << 6) | (flen >> 11));
                adts[dst + 4] = (mina_u8)(flen >> 3);
                adts[dst + 5] = (mina_u8)(((flen & 7u) << 5) | 0x1fu);
                adts[dst + 6] = 0xfcu;
                memcpy(adts + dst + 7, data + off, z);
                off += z;
                dst += z + 7;
            }
        }
        if (sample != sample_count || dst != total) { MINA_FREE(adts); return MINA_ERR_INVALID; }
        result = mina_aac_decode(adts, total, out);
        MINA_FREE(adts);
        if (out->samples && mina_u64_to_size(out->frames) > 1024) {
            size_t have = mina_u64_to_size(out->frames) - 1024;
            size_t want = mina_u64_to_size(info.total_frames);
            if (!want || want > have) want = have;
            memmove(out->samples, out->samples + 1024 * out->channels,
                    want * out->channels * sizeof(float));
            out->frames = mina_u64_from((mina_u32)want);
        }
        return result;
    }

    if (n < 7 || mina_aac_adts_frame(data, n, 0, &first_header) != 0)
        return MINA_ERR_UNSUPPORTED;

    memset(&d, 0, sizeof(d));
    d.random_state = 0x1f2e3d4cu;
    d.sf_index = first_header.sf_index;
    d.chans_cfg = first_header.chans_cfg;
    nch = (mina_u32)g_mina_aac_chans[first_header.chans_cfg];
    if (nch == 0) nch = 1;
    rate = g_mina_aac_rates[first_header.sf_index];
    mina_fbuf_init(&fb);

    if (!mina_aac_fb_init(&d)) {
        mina_aac_fb_free(&d);
        return MINA_ERR_NOMEM;
    }

    while (pos + 7 <= n) {
        mina_aac_adts h;
        size_t header_off;
        size_t plen;
        mina_aac_bits bits;
        mina_u8 raw;
        int i, ch;

        if (mina_aac_adts_frame(data, n, pos, &h) != 0) {
            /* corrupted stream: scan for the next syncword */
            pos++;
            continue;
        }

        header_off = (h.have_crc) ? 9 : 7;
        plen = (size_t)h.frame_len - header_off;
        if (plen > (size_t)0x7FFFFFFF) {   /* bits->nbits is size_t; guard */
            pos += h.frame_len;
            continue;
        }

        mina_aac_bits_init(&bits, data + pos + header_off, plen);

        raw = mina_aac_raw_data_block(&d, &bits);
        if (raw != 0) {
            if (result == MINA_OK)
                result = (d.frame > 0u) ? MINA_ERR_TRUNCATED : MINA_ERR_INVALID;
            pos += h.frame_len;
            continue;
        }

        {
            size_t count = (size_t)(1024u * nch);
            float *frame_out;
            if (fb.len > MINA_MAX_SAMPLES || count > MINA_MAX_SAMPLES - fb.len ||
                !mina_fbuf_reserve(&fb, fb.len + count)) break;
            frame_out = fb.data + fb.len;
            fb.len += count;
            for (i = 0; i < 1024; i++) {
                for (ch = 0; ch < (int)nch; ch++) {
                    float v = d.time_out[ch][i] * (1.0f / 32768.0f);
                    frame_out[i * (int)nch + ch] = v;
                }
            }
        }
        d.frame++;

        pos += h.frame_len;
    }

    mina_aac_fb_free(&d);

    if (d.frame == 0u) {
        mina_fbuf_free(&fb);
        return (result != MINA_OK) ? result : MINA_ERR_INVALID;
    }

    return mina_fbuf_finish(&fb, out, (mina_u32)nch, rate, result);
}

#endif /* MINA_NO_AAC */

/* ------------------------------------------------------------------ */
/* Resampler: linear and 32-tap windowed sinc, streaming                */
/*                                                                      */
/* The sinc kernel is tabulated once at MINA_SINC_PHASES sub-sample      */
/* positions and interpolated between them, so the inner loop is 32      */
/* multiply-adds with no transcendental calls.                          */
/* ------------------------------------------------------------------ */
#define MINA_SINC_TAPS   32
#define MINA_SINC_PHASES 128

struct mina_resampler {
    mina_u32 in_rate, out_rate, channels;
    int      quality;
    double   step;
    double   frac;        /* fractional read position within the ring   */
    mina_u32 head;        /* ring write index                            */
    mina_u32 filled;      /* input frames pushed (saturating at taps)    */
    mina_u64 in_total;    /* total input frames consumed                 */
    float   *ring;        /* taps frames, interleaved                    */
    float   *table;       /* (MINA_SINC_PHASES+1) * MINA_SINC_TAPS       */
    int      taps;
    int      drain;       /* zeros still to push during flush            */
};

static void mina_resampler_prime(mina_resampler *r);

static double mina_sinc(double x) {
    if (x > -1e-9 && x < 1e-9) return 1.0;
    x *= MINA_PI;
    return sin(x) / x;
}

static int mina_resampler_build_table(mina_resampler *r) {
    int taps = r->taps, half = taps / 2, p, k;
    double cutoff = 1.0;
    if (r->in_rate > r->out_rate)
        cutoff = (double)r->out_rate / (double)r->in_rate;   /* anti-alias */
    r->table = (float *)MINA_MALLOC((size_t)(MINA_SINC_PHASES + 1) *
                                    (size_t)taps * sizeof(float));
    if (!r->table) return 0;
    for (p = 0; p <= MINA_SINC_PHASES; p++) {
        double frac = (double)p / (double)MINA_SINC_PHASES;
        double sum = 0.0;
        float *row = r->table + (size_t)p * taps;
        for (k = 0; k < taps; k++) {
            double t = (double)(k - half + 1) - frac;
            double w = 0.5 * (1.0 + cos(2.0 * MINA_PI * t / (double)taps));
            double h;
            if (t <= -(double)half || t >= (double)half) w = 0.0;
            h = mina_sinc(t * cutoff) * cutoff * w;
            row[k] = (float)h;
            sum += h;
        }
        /* normalise so a DC input passes through at unity gain */
        if (sum > 1e-12) {
            for (k = 0; k < taps; k++) row[k] = (float)((double)row[k] / sum);
        }
    }
    return 1;
}

mina_resampler *mina_resampler_create(mina_u32 in_rate, mina_u32 out_rate,
                                      mina_u32 channels, int quality) {
    mina_resampler *r;
    if (!in_rate || !out_rate || !channels || channels > 4096) return NULL;
    r = (mina_resampler *)MINA_MALLOC(sizeof(mina_resampler));
    if (!r) return NULL;
    memset(r, 0, sizeof(*r));
    r->in_rate = in_rate; r->out_rate = out_rate; r->channels = channels;
    r->quality = quality ? MINA_QUALITY_SINC : MINA_QUALITY_LINEAR;
    r->step = (double)in_rate / (double)out_rate;
    r->taps = r->quality ? MINA_SINC_TAPS : 2;
    r->ring = (float *)MINA_MALLOC((size_t)channels * (size_t)r->taps * sizeof(float));
    if (!r->ring) { MINA_FREE(r); return NULL; }
    memset(r->ring, 0, (size_t)channels * (size_t)r->taps * sizeof(float));
    if (r->quality && !mina_resampler_build_table(r)) {
        MINA_FREE(r->ring); MINA_FREE(r); return NULL;
    }
    r->in_total = mina_u64_from(0);
    mina_resampler_prime(r);
    return r;
}

void mina_resampler_reset(mina_resampler *r) {
    if (!r) return;
    memset(r->ring, 0, (size_t)r->channels * (size_t)r->taps * sizeof(float));
    r->frac = 0.0; r->head = 0; r->filled = 0; r->drain = 0;
    r->in_total = mina_u64_from(0);
    mina_resampler_prime(r);
}

void mina_resampler_free(mina_resampler *r) {
    if (!r) return;
    MINA_FREE(r->ring);
    MINA_FREE(r->table);
    MINA_FREE(r);
}

/* Push one input frame into the ring. taps is a power of two. */
static void mina_resampler_push(mina_resampler *r, const float *frame) {
    mina_u32 c, mask = (mina_u32)r->taps - 1u;
    float *slot = r->ring + (size_t)r->head * r->channels;
    if (frame) for (c = 0; c < r->channels; c++) slot[c] = frame[c];
    else       for (c = 0; c < r->channels; c++) slot[c] = 0.0f;
    r->head = (r->head + 1u) & mask;
    if (r->filled < (mina_u32)r->taps) r->filled++;
}

/* Emit one output frame at the current fractional position.
 * ring[head] is the oldest frame, i.e. window index 0. */
static void mina_resampler_emit(mina_resampler *r, float *dst) {
    int taps = r->taps, k;
    mina_u32 c, mask = (mina_u32)taps - 1u;
    float h[MINA_SINC_TAPS];

    if (!r->quality) {
        const float *a = r->ring + (size_t)r->head * r->channels;
        const float *b = r->ring + (size_t)((r->head + 1u) & mask) * r->channels;
        double f = r->frac;
        for (c = 0; c < r->channels; c++)
            dst[c] = (float)((double)a[c] + ((double)b[c] - (double)a[c]) * f);
        return;
    }
    {
        double fp = r->frac * (double)MINA_SINC_PHASES;
        int p = (int)fp;
        double mu = fp - (double)p;
        const float *r0, *r1;
        if (p < 0) { p = 0; mu = 0.0; }
        if (p >= MINA_SINC_PHASES) { p = MINA_SINC_PHASES - 1; mu = 1.0; }
        r0 = r->table + (size_t)p * taps;
        r1 = r0 + taps;
        for (k = 0; k < taps; k++)
            h[k] = (float)((double)r0[k] + ((double)r1[k] - (double)r0[k]) * mu);
    }
    for (c = 0; c < r->channels; c++) {
        double acc = 0.0;
        for (k = 0; k < taps; k++)
            acc += (double)r->ring[(size_t)((r->head + (mina_u32)k) & mask)
                                   * r->channels + c] * (double)h[k];
        dst[c] = (float)acc;
    }
}

/* Prime the delay line so output frame 0 lines up with input frame 0:
 * the sinc kernel is centred at window index taps/2 - 1 + frac. */
static void mina_resampler_prime(mina_resampler *r) {
    int i, pre = r->taps / 2 - 1;
    for (i = 0; i < pre; i++) mina_resampler_push(r, NULL);
}

mina_u32 mina_resampler_process(mina_resampler *r, const float *in,
                                mina_u32 in_frames, float *out, mina_u32 out_cap,
                                mina_u32 *in_used) {
    mina_u32 written = 0, used = 0;
    if (in_used) *in_used = 0;
    if (!r || !out) return 0;

    for (;;) {
        /* the window is complete once `taps` frames have been pushed */
        while (r->filled == (mina_u32)r->taps && r->frac < 1.0 && written < out_cap) {
            mina_resampler_emit(r, out + (size_t)written * r->channels);
            written++;
            r->frac += r->step;
        }
        if (written >= out_cap) break;
        if (r->frac >= 1.0 || r->filled < (mina_u32)r->taps) {
            if (used >= in_frames) break;
            mina_resampler_push(r, in ? in + (size_t)used * r->channels : NULL);
            used++;
            if (r->frac >= 1.0) r->frac -= 1.0;
        } else {
            break;
        }
    }
    if (in_used) *in_used = used;
    r->in_total = mina_u64_add(r->in_total, mina_u64_from(used));
    return written;
}

mina_u32 mina_resampler_flush(mina_resampler *r, float *out, mina_u32 out_cap) {
    mina_u32 written = 0;
    int guard;
    if (!r || !out) return 0;
    /* feed the tail of the delay line with silence */
    for (guard = 0; guard < r->taps && written < out_cap; guard++) {
        while (r->filled == (mina_u32)r->taps && r->frac < 1.0 && written < out_cap) {
            mina_resampler_emit(r, out + (size_t)written * r->channels);
            written++;
            r->frac += r->step;
        }
        if (written >= out_cap) break;
        mina_resampler_push(r, NULL);
        if (r->frac >= 1.0) r->frac -= 1.0;
    }
    return written;
}

mina_u64 mina_resample_buffer(const float *in, mina_u64 in_frames,
                              mina_u32 in_rate, mina_u32 out_rate,
                              mina_u32 channels, int quality, float **out) {
    mina_resampler *r;
    size_t nin, cap, got = 0;
    float *buf;
    int ovf = 0;

    if (out) *out = NULL;
    if (!in || !out || !channels || !in_rate || !out_rate) return mina_u64_from(0);
    nin = mina_u64_size(in_frames, &ovf);
    if (ovf) return mina_u64_from(0);

    cap = (size_t)((double)nin * (double)out_rate / (double)in_rate) + 64;
    buf = (float *)MINA_MALLOC((cap ? cap : 1) * (size_t)channels * sizeof(float));
    if (!buf) return mina_u64_from(0);
    r = mina_resampler_create(in_rate, out_rate, channels, quality);
    if (!r) { MINA_FREE(buf); return mina_u64_from(0); }

    {
        size_t consumed = 0;
        while (consumed < nin && got < cap) {
            mina_u32 used = 0;
            mina_u32 chunk = (mina_u32)((nin - consumed > 4096u) ? 4096u : (nin - consumed));
            mina_u32 w = mina_resampler_process(r, in + consumed * channels, chunk,
                                                buf + got * channels,
                                                (mina_u32)(cap - got), &used);
            got += w;
            consumed += used;
            if (!w && !used) break;
        }
        if (got < cap)
            got += mina_resampler_flush(r, buf + got * channels, (mina_u32)(cap - got));
    }
    mina_resampler_free(r);
    /* The delay-line flush spills a few tail samples past the ideal length;
     * trim to the exact ratio so out_frames == round(in_frames * out/in). */
    {
        size_t ideal = (size_t)((double)nin * (double)out_rate / (double)in_rate + 0.5);
        if (got > ideal) got = ideal;
    }
    *out = buf;
    return mina_u64_from((mina_u32)got);
}

/* ------------------------------------------------------------------ */
/* Mixing                                                               */
/* ------------------------------------------------------------------ */
void mina_mix_raw(float *dst, const float *src, size_t count, float gain) {
    size_t i;
    if (!dst || !src) return;
    for (i = 0; i < count; i++) dst[i] += src[i] * gain;
}

void mina_mix(float *dst, const float *src, size_t count, float gain) {
    size_t i;
    if (!dst || !src) return;
    for (i = 0; i < count; i++) {
        float v = dst[i] + src[i] * gain;
        /* soft knee above unity, hard limit at +/-1.1 */
        if (v > 1.0f)  v = 1.0f + (v - 1.0f) * 0.1f;
        if (v < -1.0f) v = -1.0f + (v + 1.0f) * 0.1f;
        if (v > 1.1f)  v = 1.1f;
        if (v < -1.1f) v = -1.1f;
        dst[i] = v;
    }
}

/* ------------------------------------------------------------------ */
/* Small synthesiser                                                    */
/* ------------------------------------------------------------------ */
struct mina_synth {
    mina_u32 sample_rate;
    float    freq, velocity, phase;
    int      wave;
    mina_u32 noise_state;
};

mina_synth *mina_synth_create(mina_u32 sample_rate) {
    mina_synth *s = (mina_synth *)MINA_MALLOC(sizeof(mina_synth));
    if (!s) return NULL;
    memset(s, 0, sizeof(*s));
    s->sample_rate = sample_rate ? sample_rate : 44100u;
    s->freq = 440.0f;
    s->wave = MINA_WAVE_SINE;
    s->noise_state = 0x12345678UL;
    return s;
}
void mina_synth_free(mina_synth *s) { MINA_FREE(s); }
void mina_synth_note_on(mina_synth *s, float freq, float velocity) {
    if (!s) return;
    s->freq = freq; s->velocity = velocity;
}
void mina_synth_note_off(mina_synth *s) { if (s) s->velocity = 0.0f; }
void mina_synth_set_wave(mina_synth *s, int wave) {
    if (s) s->wave = (wave < 0 || wave > MINA_WAVE_NOISE) ? MINA_WAVE_SINE : wave;
}

mina_u32 mina_synth_render(mina_synth *s, float *out, mina_u32 frames) {
    mina_u32 i;
    float inc;
    if (!s || !out) return 0;
    inc = s->freq / (float)s->sample_rate;
    for (i = 0; i < frames; i++) {
        float ph = s->phase, v;
        switch (s->wave) {
        case MINA_WAVE_SINE:   v = (float)sin(2.0 * MINA_PI * (double)ph); break;
        case MINA_WAVE_SQUARE: v = (ph < 0.5f) ? 1.0f : -1.0f; break;
        case MINA_WAVE_SAW:    v = 2.0f * ph - 1.0f; break;
        case MINA_WAVE_TRI:    v = (ph < 0.5f) ? (4.0f * ph - 1.0f) : (3.0f - 4.0f * ph); break;
        default:
            s->noise_state = s->noise_state * 1664525UL + 1013904223UL;
            v = ((float)(s->noise_state >> 8) / 8388608.0f) - 1.0f;
            break;
        }
        out[i] = v * s->velocity;
        s->phase += inc;
        while (s->phase >= 1.0f) s->phase -= 1.0f;
        while (s->phase < 0.0f)  s->phase += 1.0f;
    }
    return frames;
}

/* ------------------------------------------------------------------ */
/* Output devices                                                       */
/*                                                                      */
/* Every backend is compile-time guarded and none needs its headers or   */
/* import libraries present at build time: ALSA and PulseAudio are       */
/* reached through dlopen, OSS through a device node, DOS hardware       */
/* through port I/O. Opening never hard-fails - an unavailable backend   */
/* falls through to the next candidate and finally to the null sink.     */
/* ------------------------------------------------------------------ */
#ifndef MINA_NO_DEVICES

#if defined(MINA_LINUX) || defined(MINA_BSD) || defined(MINA_UNIX) || \
    defined(MINA_POSIX_GENERIC)
#define MINA_HAVE_POSIX_AUDIO 1
#include <fcntl.h>
#include <unistd.h>
#endif

struct mina_device {
    char      backend[16];
    char      path[256];
    mina_u32  sample_rate;
    mina_u32  channels;
    int       fd;
    int       is_file;
    int       is_real;
    void     *impl;
#ifndef MINA_NO_STDIO
    FILE     *fp;
    mina_u64  file_bytes;
#endif
    mina_i16 *scratch;
    size_t    scratch_cap;
};

/* Grow the per-device conversion scratch buffer. */
static mina_i16 *mina_dev_scratch(mina_device *d, size_t samples) {
    if (samples > d->scratch_cap) {
        mina_i16 *p = (mina_i16 *)MINA_REALLOC(d->scratch, samples * sizeof(mina_i16));
        if (!p) return NULL;
        d->scratch = p;
        d->scratch_cap = samples;
    }
    return d->scratch;
}

/* ---------------- POSIX: ALSA + PulseAudio through dlopen ------------- */
#ifdef MINA_HAVE_POSIX_AUDIO
#include <dlfcn.h>

typedef struct {
    void *h;
    void *pcm;
    int (*writei)(void *, const void *, unsigned long);
    int (*recover)(void *, int, int);
    int (*drain)(void *);
    int (*close)(void *);
} mina_alsa_ctx;

/* libasound writes its configuration complaints to stderr; a library has no
 * business doing that on someone else's behalf, so silence it. */
static void mina_alsa_quiet(const char *file, int line, const char *fn,
                            int err, const char *fmt, ...) {
    (void)file; (void)line; (void)fn; (void)err; (void)fmt;
}

static void *mina_alsa_open(mina_u32 rate, mina_u32 channels) {
    void *h;
    mina_alsa_ctx *c;
    int (*pcm_open)(void **, const char *, int, int);
    /* snd_pcm_set_params(pcm, format, access, channels, rate, resample, us) */
    int (*set_params)(void *, int, int, unsigned int, unsigned int, int, unsigned int);
    int (*set_error_handler)(void (*)(const char *, int, const char *, int,
                                      const char *, ...));

    h = dlopen("libasound.so.2", RTLD_LAZY);
    if (!h) h = dlopen("libasound.so", RTLD_LAZY);
    if (!h) return NULL;
    *(void **)(&set_error_handler) = dlsym(h, "snd_lib_error_set_handler");
    if (set_error_handler) set_error_handler(mina_alsa_quiet);
    *(void **)(&pcm_open)   = dlsym(h, "snd_pcm_open");
    *(void **)(&set_params) = dlsym(h, "snd_pcm_set_params");
    if (!pcm_open || !set_params) { dlclose(h); return NULL; }

    c = (mina_alsa_ctx *)MINA_MALLOC(sizeof(mina_alsa_ctx));
    if (!c) { dlclose(h); return NULL; }
    memset(c, 0, sizeof(*c));
    c->h = h;
    *(void **)(&c->writei)  = dlsym(h, "snd_pcm_writei");
    *(void **)(&c->recover) = dlsym(h, "snd_pcm_recover");
    *(void **)(&c->drain)   = dlsym(h, "snd_pcm_drain");
    *(void **)(&c->close)   = dlsym(h, "snd_pcm_close");
    /* stream 0 = SND_PCM_STREAM_PLAYBACK, mode 0 = blocking */
    if (pcm_open(&c->pcm, "default", 0, 0) < 0) {
        dlclose(h); MINA_FREE(c); return NULL;
    }
    /* format 2 = SND_PCM_FORMAT_S16_LE, access 3 = RW_INTERLEAVED */
    if (set_params(c->pcm, 2, 3, channels, rate, 1, 100000) < 0) {
        if (c->close) c->close(c->pcm);
        dlclose(h); MINA_FREE(c); return NULL;
    }
    if (!c->writei) {
        if (c->close) c->close(c->pcm);
        dlclose(h); MINA_FREE(c); return NULL;
    }
    return c;
}

static mina_result mina_alsa_write(mina_device *d, const float *s, mina_u32 frames) {
    mina_alsa_ctx *c = (mina_alsa_ctx *)d->impl;
    size_t n = (size_t)frames * d->channels;
    mina_i16 *tmp = mina_dev_scratch(d, n);
    mina_u32 done = 0;
    if (!tmp) return MINA_ERR_NOMEM;
    mina_convert_from_f32(s, MINA_FMT_S16, d->channels, n, tmp);
    while (done < frames) {
        int r = c->writei(c->pcm, tmp + (size_t)done * d->channels, frames - done);
        if (r < 0) {
            if (c->recover && c->recover(c->pcm, r, 1) == 0) continue;
            return MINA_ERR_DEVICE;
        }
        if (r == 0) break;
        done += (mina_u32)r;
    }
    return MINA_OK;
}

static void mina_alsa_close(mina_device *d) {
    mina_alsa_ctx *c = (mina_alsa_ctx *)d->impl;
    if (!c) return;
    if (c->drain) c->drain(c->pcm);
    if (c->close) c->close(c->pcm);
    if (c->h) dlclose(c->h);
    MINA_FREE(c);
    d->impl = NULL;
}

/* PulseAudio simple API. pa_sample_spec is { int format; uint32 rate;
 * uint8 channels; } - build it with the compiler's own layout rather than
 * guessing at padding. */
typedef struct { int format; mina_u32 rate; mina_u8 channels; } mina_pa_spec;
typedef struct {
    void *h;
    void *pa;
    int (*simple_write)(void *, const void *, size_t, int *);
    int (*simple_drain)(void *, int *);
    void (*simple_free)(void *);
} mina_pulse_ctx;

static void *mina_pulse_open(mina_u32 rate, mina_u32 channels) {
    void *h;
    mina_pulse_ctx *c;
    void *(*simple_new)(const char *, const char *, int, const char *,
                        const char *, const mina_pa_spec *, const void *,
                        const void *, int *);
    mina_pa_spec spec;
    int err = 0;

    h = dlopen("libpulse-simple.so.0", RTLD_LAZY);
    if (!h) h = dlopen("libpulse-simple.so", RTLD_LAZY);
    if (!h) return NULL;
    *(void **)(&simple_new) = dlsym(h, "pa_simple_new");
    if (!simple_new) { dlclose(h); return NULL; }

    c = (mina_pulse_ctx *)MINA_MALLOC(sizeof(mina_pulse_ctx));
    if (!c) { dlclose(h); return NULL; }
    memset(c, 0, sizeof(*c));
    c->h = h;
    *(void **)(&c->simple_write) = dlsym(h, "pa_simple_write");
    *(void **)(&c->simple_drain) = dlsym(h, "pa_simple_drain");
    *(void **)(&c->simple_free)  = dlsym(h, "pa_simple_free");

    spec.format = 3;                     /* PA_SAMPLE_S16LE */
    spec.rate = rate;
    spec.channels = (mina_u8)(channels > 255 ? 255 : channels);
    /* direction 1 = PA_STREAM_PLAYBACK */
    c->pa = simple_new(NULL, "mina", 1, NULL, "playback", &spec, NULL, NULL, &err);
    if (!c->pa || !c->simple_write) {
        if (c->pa && c->simple_free) c->simple_free(c->pa);
        dlclose(h); MINA_FREE(c); return NULL;
    }
    return c;
}

static mina_result mina_pulse_write(mina_device *d, const float *s, mina_u32 frames) {
    mina_pulse_ctx *c = (mina_pulse_ctx *)d->impl;
    size_t n = (size_t)frames * d->channels;
    mina_i16 *tmp = mina_dev_scratch(d, n);
    int err = 0;
    if (!tmp) return MINA_ERR_NOMEM;
    mina_convert_from_f32(s, MINA_FMT_S16, d->channels, n, tmp);
    if (c->simple_write(c->pa, tmp, n * sizeof(mina_i16), &err) < 0)
        return MINA_ERR_DEVICE;
    return MINA_OK;
}

static void mina_pulse_close(mina_device *d) {
    mina_pulse_ctx *c = (mina_pulse_ctx *)d->impl;
    if (!c) return;
    if (c->simple_drain) { int e = 0; c->simple_drain(c->pa, &e); }
    if (c->simple_free) c->simple_free(c->pa);
    if (c->h) dlclose(c->h);
    MINA_FREE(c);
    d->impl = NULL;
}

/* OSS: /dev/dsp, configured through ioctl when the constants are known. */
static int mina_oss_open(mina_device *d) {
    int fd = open("/dev/dsp", O_WRONLY);
    if (fd < 0) fd = open("/dev/dsp0", O_WRONLY);
    if (fd < 0) fd = open("/dev/audio", O_WRONLY);
    if (fd < 0) return 0;
#ifdef SNDCTL_DSP_SPEED
    {
        int fmt = 16 /* AFMT_S16_LE */, ch = (int)d->channels, sr = (int)d->sample_rate;
        ioctl(fd, SNDCTL_DSP_SETFMT, &fmt);
        ioctl(fd, SNDCTL_DSP_CHANNELS, &ch);
        ioctl(fd, SNDCTL_DSP_SPEED, &sr);
    }
#endif
    d->fd = fd;
    return 1;
}

static mina_result mina_oss_write(mina_device *d, const float *s, mina_u32 frames) {
    size_t n = (size_t)frames * d->channels, off = 0, bytes;
    mina_i16 *tmp = mina_dev_scratch(d, n);
    if (!tmp) return MINA_ERR_NOMEM;
    mina_convert_from_f32(s, MINA_FMT_S16, d->channels, n, tmp);
    bytes = n * sizeof(mina_i16);
    while (off < bytes) {
        long w = (long)write(d->fd, (const char *)tmp + off, bytes - off);
        if (w <= 0) return MINA_ERR_DEVICE;
        off += (size_t)w;
    }
    return MINA_OK;
}
#endif /* MINA_HAVE_POSIX_AUDIO */

/* ---------------- Windows: WinMM waveOut ------------------------------ */
#if defined(MINA_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#if defined(_MSC_VER)
#pragma comment(lib, "winmm.lib")
#endif

#define MINA_WINMM_BUFFERS 4
#define MINA_WINMM_FRAMES  4096

typedef struct {
    HWAVEOUT h;
    WAVEHDR  hdr[MINA_WINMM_BUFFERS];
    mina_i16 *buf[MINA_WINMM_BUFFERS];
    int      next;
    int      prepared[MINA_WINMM_BUFFERS];
    mina_u32 channels;
    HANDLE   done;      /* signalled by waveOut as each buffer completes */
} mina_winmm_ctx;

/* Block until the given header is free again. waveOutOpen is asked for
 * CALLBACK_EVENT, which every Windows since 95 supports; a driver that
 * refuses it leaves `done` NULL and we fall back to polling. */
static void mina_winmm_await(mina_winmm_ctx *c, WAVEHDR *hdr) {
    while (!(hdr->dwFlags & WHDR_DONE)) {
        if (c->done) WaitForSingleObject(c->done, 100);
        else         Sleep(1);
    }
}

static void *mina_winmm_open(mina_u32 rate, mina_u32 channels) {
    WAVEFORMATEX wf;
    mina_winmm_ctx *c;
    int i;
    c = (mina_winmm_ctx *)MINA_MALLOC(sizeof(mina_winmm_ctx));
    if (!c) return NULL;
    memset(c, 0, sizeof(*c));
    c->channels = channels;
    memset(&wf, 0, sizeof(wf));
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = (WORD)channels;
    wf.nSamplesPerSec = rate;
    wf.wBitsPerSample = 16;
    wf.nBlockAlign = (WORD)(channels * 2);
    wf.nAvgBytesPerSec = rate * channels * 2;
    c->done = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (waveOutOpen(&c->h, WAVE_MAPPER, &wf, (DWORD_PTR)c->done, 0,
                    c->done ? CALLBACK_EVENT : CALLBACK_NULL) != MMSYSERR_NOERROR) {
        if (c->done) {
            CloseHandle(c->done);
            c->done = NULL;
            if (waveOutOpen(&c->h, WAVE_MAPPER, &wf, 0, 0,
                            CALLBACK_NULL) != MMSYSERR_NOERROR) {
                MINA_FREE(c);
                return NULL;
            }
        } else {
            MINA_FREE(c);
            return NULL;
        }
    }
    for (i = 0; i < MINA_WINMM_BUFFERS; i++) {
        c->buf[i] = (mina_i16 *)MINA_MALLOC((size_t)MINA_WINMM_FRAMES * channels * 2);
        if (!c->buf[i]) {
            int j;
            for (j = 0; j < i; j++) MINA_FREE(c->buf[j]);
            waveOutClose(c->h);
            MINA_FREE(c);
            return NULL;
        }
    }
    return c;
}

static mina_result mina_winmm_write(mina_device *d, const float *s, mina_u32 frames) {
    mina_winmm_ctx *c = (mina_winmm_ctx *)d->impl;
    mina_u32 done = 0;
    while (done < frames) {
        mina_u32 chunk = frames - done;
        WAVEHDR *hdr;
        int idx = c->next;
        if (chunk > (mina_u32)MINA_WINMM_FRAMES) chunk = (mina_u32)MINA_WINMM_FRAMES;
        hdr = &c->hdr[idx];
        if (c->prepared[idx]) {
            mina_winmm_await(c, hdr);
            waveOutUnprepareHeader(c->h, hdr, sizeof(WAVEHDR));
            c->prepared[idx] = 0;
        }
        mina_convert_from_f32(s + (size_t)done * d->channels, MINA_FMT_S16,
                              d->channels, (size_t)chunk * d->channels, c->buf[idx]);
        memset(hdr, 0, sizeof(WAVEHDR));
        hdr->lpData = (LPSTR)c->buf[idx];
        hdr->dwBufferLength = chunk * d->channels * 2;
        if (waveOutPrepareHeader(c->h, hdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
            return MINA_ERR_DEVICE;
        c->prepared[idx] = 1;
        if (waveOutWrite(c->h, hdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
            return MINA_ERR_DEVICE;
        c->next = (idx + 1) % MINA_WINMM_BUFFERS;
        done += chunk;
    }
    return MINA_OK;
}

static void mina_winmm_close(mina_device *d) {
    mina_winmm_ctx *c = (mina_winmm_ctx *)d->impl;
    int i;
    if (!c) return;
    for (i = 0; i < MINA_WINMM_BUFFERS; i++) {
        if (c->prepared[i]) {
            mina_winmm_await(c, &c->hdr[i]);
            waveOutUnprepareHeader(c->h, &c->hdr[i], sizeof(WAVEHDR));
        }
        MINA_FREE(c->buf[i]);
    }
    waveOutClose(c->h);
    if (c->done) CloseHandle(c->done);
    MINA_FREE(c);
    d->impl = NULL;
}
#endif /* MINA_WINDOWS */

/* ---------------- macOS: CoreAudio -----------------------------------
 * AudioUnit output is a pull model: the unit calls us. A lock-free ring
 * bridges the caller's push-style writes to the render callback. */
#if defined(MINA_MACOS)
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>

#define MINA_CA_RING_FRAMES 16384

typedef struct {
    AudioUnit au;
    float    *ring;
    mina_u32  frames;      /* ring capacity in frames */
    mina_u32  channels;
    volatile mina_u32 rd, wr;
    int       started;
} mina_ca_ctx;

static OSStatus mina_ca_render(void *ref, AudioUnitRenderActionFlags *flags,
                               const AudioTimeStamp *ts, UInt32 bus,
                               UInt32 nframes, AudioBufferList *io) {
    mina_ca_ctx *c = (mina_ca_ctx *)ref;
    float *dst = (float *)io->mBuffers[0].mData;
    mina_u32 i, ch = c->channels;
    (void)flags; (void)ts; (void)bus;
    for (i = 0; i < nframes; i++) {
        mina_u32 rd = c->rd;
        if (rd == c->wr) {
            memset(dst + (size_t)i * ch, 0, (size_t)(nframes - i) * ch * sizeof(float));
            break;
        }
        memcpy(dst + (size_t)i * ch, c->ring + (size_t)rd * ch, ch * sizeof(float));
        c->rd = (rd + 1u) % c->frames;
    }
    return noErr;
}

static void *mina_coreaudio_open(mina_u32 rate, mina_u32 channels) {
    AudioComponentDescription desc;
    AudioComponent comp;
    AudioStreamBasicDescription fmt;
    AURenderCallbackStruct cb;
    mina_ca_ctx *c;

    c = (mina_ca_ctx *)MINA_MALLOC(sizeof(mina_ca_ctx));
    if (!c) return NULL;
    memset(c, 0, sizeof(*c));
    c->channels = channels;
    c->frames = MINA_CA_RING_FRAMES;
    c->ring = (float *)MINA_MALLOC((size_t)c->frames * channels * sizeof(float));
    if (!c->ring) { MINA_FREE(c); return NULL; }

    memset(&desc, 0, sizeof(desc));
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    comp = AudioComponentFindNext(NULL, &desc);
    if (!comp) { MINA_FREE(c->ring); MINA_FREE(c); return NULL; }
    if (AudioComponentInstanceNew(comp, &c->au) != noErr) {
        MINA_FREE(c->ring); MINA_FREE(c); return NULL;
    }
    memset(&fmt, 0, sizeof(fmt));
    fmt.mSampleRate = (Float64)rate;
    fmt.mFormatID = kAudioFormatLinearPCM;
    fmt.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    fmt.mBitsPerChannel = 32;
    fmt.mChannelsPerFrame = channels;
    fmt.mFramesPerPacket = 1;
    fmt.mBytesPerFrame = channels * 4;
    fmt.mBytesPerPacket = channels * 4;
    if (AudioUnitSetProperty(c->au, kAudioUnitProperty_StreamFormat,
                             kAudioUnitScope_Input, 0, &fmt, sizeof(fmt)) != noErr)
        goto fail;
    memset(&cb, 0, sizeof(cb));
    cb.inputProc = mina_ca_render;
    cb.inputProcRefCon = c;
    if (AudioUnitSetProperty(c->au, kAudioUnitProperty_SetRenderCallback,
                             kAudioUnitScope_Input, 0, &cb, sizeof(cb)) != noErr)
        goto fail;
    if (AudioUnitInitialize(c->au) != noErr) goto fail;
    if (AudioOutputUnitStart(c->au) != noErr) goto fail;
    c->started = 1;
    return c;
fail:
    AudioComponentInstanceDispose(c->au);
    MINA_FREE(c->ring);
    MINA_FREE(c);
    return NULL;
}

static mina_result mina_coreaudio_write(mina_device *d, const float *s,
                                        mina_u32 frames) {
    mina_ca_ctx *c = (mina_ca_ctx *)d->impl;
    mina_u32 i;
    for (i = 0; i < frames; i++) {
        mina_u32 nw = (c->wr + 1u) % c->frames;
        int spins = 0;
        while (nw == c->rd) {
            usleep(1000);
            if (++spins > 5000) return MINA_ERR_DEVICE;
        }
        memcpy(c->ring + (size_t)c->wr * c->channels, s + (size_t)i * c->channels,
               c->channels * sizeof(float));
        c->wr = nw;
    }
    return MINA_OK;
}

static void mina_coreaudio_close(mina_device *d) {
    mina_ca_ctx *c = (mina_ca_ctx *)d->impl;
    int spins = 0;
    if (!c) return;
    while (c->rd != c->wr && spins++ < 5000) usleep(1000);
    if (c->started) AudioOutputUnitStop(c->au);
    AudioUnitUninitialize(c->au);
    AudioComponentInstanceDispose(c->au);
    MINA_FREE(c->ring);
    MINA_FREE(c);
    d->impl = NULL;
}
#endif /* MINA_MACOS */

/* ---------------- DOS: PC speaker, AdLib OPL2, MPU-401 ---------------- */
#if defined(MINA_DOS)
#if defined(__GNUC__)
static int mina_inb(int port) {
    unsigned char v;
    __asm__ __volatile__ ("inb %w1, %b0" : "=a"(v) : "Nd"(port));
    return (int)v;
}
static void mina_outb(int port, int val) {
    __asm__ __volatile__ ("outb %b0, %w1" : : "a"((unsigned char)val), "Nd"(port));
}
#elif defined(__WATCOMC__)
int  mina_inb(int port);
void mina_outb(int port, int val);
#pragma aux mina_inb  = "in al, dx" parm [dx] value [al] modify [al];
#pragma aux mina_outb = "out dx, al" parm [dx] [al];
#else
#include <conio.h>
static int  mina_inb(int port) { return inp((unsigned short)port); }
static void mina_outb(int port, int val) { outp((unsigned short)port, val); }
#endif

/* Drive the 8253 channel 2 at the dominant frequency of the block. */
static void mina_pcspeaker_write_impl(mina_device *d, const float *s,
                                      mina_u32 frames) {
    mina_u32 i;
    double energy = 0.0;
    mina_u32 zero_cross = 0;
    int prev = 0, cur;
    mina_u32 ch = d->channels ? d->channels : 1;
    mina_u32 nf = frames;

    if (!nf) return;
    for (i = 0; i < nf; i++) {
        float v = s[(size_t)i * ch];
        energy += (double)v * (double)v;
    }
    prev = (s[0] >= 0.0f);
    for (i = 1; i < nf; i++) {
        cur = (s[(size_t)i * ch] >= 0.0f);
        if (cur != prev) zero_cross++;
        prev = cur;
    }
    if (energy / (double)nf < 1e-4) {
        mina_outb(0x61, mina_inb(0x61) & 0xFC);       /* gate off */
        return;
    }
    {
        double freq = (double)zero_cross * (double)d->sample_rate / (2.0 * (double)nf);
        mina_u32 divisor;
        if (freq < 40.0) freq = 40.0;
        divisor = (mina_u32)(1193180.0 / freq);
        if (divisor < 1) divisor = 1;
        if (divisor > 65535u) divisor = 65535u;
        mina_outb(0x43, 0xB6);                        /* ch 2, mode 3, LSB/MSB */
        mina_outb(0x42, (int)(divisor & 0xFFu));
        mina_outb(0x42, (int)((divisor >> 8) & 0xFFu));
        mina_outb(0x61, mina_inb(0x61) | 0x03);       /* gate on */
    }
}

static void mina_adlib_write_reg(int reg, int val) {
    int i;
    mina_outb(0x388, reg);
    for (i = 0; i < 6; i++) (void)mina_inb(0x388);     /* address delay */
    mina_outb(0x389, val);
    for (i = 0; i < 35; i++) (void)mina_inb(0x388);    /* data delay */
}
static void mina_adlib_reset(void) {
    int i;
    for (i = 1; i <= 0xF5; i++) mina_adlib_write_reg(i, 0);
    mina_adlib_write_reg(0x01, 0x20);                  /* enable waveform select */
}
static void mina_adlib_note(double freq) {
    static const int fnum_base = 0;
    int block = 0;
    double f = freq;
    int fnum;
    (void)fnum_base;
    while (f >= 976.0 && block < 7) { f /= 2.0; block++; }
    fnum = (int)(f * 1048576.0 / 49716.0);
    if (fnum > 1023) fnum = 1023;
    mina_adlib_write_reg(0x20, 0x01);                  /* modulator: mult 1 */
    mina_adlib_write_reg(0x40, 0x10);                  /* modulator level  */
    mina_adlib_write_reg(0x60, 0xF0);                  /* attack/decay     */
    mina_adlib_write_reg(0x80, 0x77);                  /* sustain/release  */
    mina_adlib_write_reg(0x23, 0x01);                  /* carrier: mult 1  */
    mina_adlib_write_reg(0x43, 0x00);                  /* carrier level    */
    mina_adlib_write_reg(0x63, 0xF0);
    mina_adlib_write_reg(0x83, 0x77);
    mina_adlib_write_reg(0xA0, fnum & 0xFF);
    mina_adlib_write_reg(0xB0, 0x20 | ((block & 7) << 2) | ((fnum >> 8) & 3));
}
static void mina_adlib_silence(void) { mina_adlib_write_reg(0xB0, 0x00); }

static int mina_mpu_ready(void) {
    int i;
    for (i = 0; i < 10000; i++)
        if (!(mina_inb(0x331) & 0x40)) return 1;
    return 0;
}
static void mina_midi_send(int b) {
    if (mina_mpu_ready()) mina_outb(0x330, b);
}
static void mina_midi_reset(void) {
    int i;
    for (i = 0; i < 10000; i++) {
        if (!(mina_inb(0x331) & 0x40)) { mina_outb(0x331, 0xFF); break; }
    }
    mina_midi_send(0xB0); mina_midi_send(0x7B); mina_midi_send(0x00);  /* all notes off */
    mina_midi_send(0xC0); mina_midi_send(0x50);                        /* lead synth   */
}
#endif /* MINA_DOS */

/* ---------------- backend selection ---------------------------------- */
static int mina_backend_available(const char *name) {
    if (!strcmp(name, "null") || !strcmp(name, "wavfile")) return 1;
#ifdef MINA_HAVE_POSIX_AUDIO
    if (!strcmp(name, "alsa") || !strcmp(name, "pulse") || !strcmp(name, "oss")) return 1;
#endif
#ifdef MINA_WINDOWS
    if (!strcmp(name, "winmm")) return 1;
#endif
#ifdef MINA_MACOS
    if (!strcmp(name, "coreaudio")) return 1;
#endif
#ifdef MINA_DOS
    if (!strcmp(name, "pcspeaker") || !strcmp(name, "adlib") ||
        !strcmp(name, "midi")) return 1;
#endif
    return 0;
}

/* Try to bring up one named backend. Returns 1 on success. */
static int mina_device_try(mina_device *d, const char *name) {
    if (!mina_backend_available(name)) return 0;

    if (!strcmp(name, "null")) { d->is_real = 0; return 1; }

    if (!strcmp(name, "wavfile")) {
#ifdef MINA_NO_STDIO
        return 0;
#else
        mina_u8 hdr[44];
        mina_u32 br = d->sample_rate * d->channels * 2u;
        d->fp = fopen(d->path[0] ? d->path : "mina_out.wav", "wb");
        if (!d->fp) return 0;
        memset(hdr, 0, sizeof(hdr));
        memcpy(hdr, "RIFF", 4);
        memcpy(hdr + 8, "WAVE", 4);
        memcpy(hdr + 12, "fmt ", 4);
        mina_put_le32(hdr + 16, 16);
        mina_put_le16(hdr + 20, MINA_WAVE_PCM);
        mina_put_le16(hdr + 22, d->channels);
        mina_put_le32(hdr + 24, d->sample_rate);
        mina_put_le32(hdr + 28, br);
        mina_put_le16(hdr + 32, d->channels * 2u);
        mina_put_le16(hdr + 34, 16);
        memcpy(hdr + 36, "data", 4);
        if (fwrite(hdr, 1, 44, d->fp) != 44) { fclose(d->fp); d->fp = NULL; return 0; }
        d->is_file = 1;
        d->is_real = 1;
        return 1;
#endif
    }

#ifdef MINA_HAVE_POSIX_AUDIO
    if (!strcmp(name, "alsa")) {
        d->impl = mina_alsa_open(d->sample_rate, d->channels);
        if (!d->impl) return 0;
        d->is_real = 1; return 1;
    }
    if (!strcmp(name, "pulse")) {
        d->impl = mina_pulse_open(d->sample_rate, d->channels);
        if (!d->impl) return 0;
        d->is_real = 1; return 1;
    }
    if (!strcmp(name, "oss")) {
        if (!mina_oss_open(d)) return 0;
        d->is_real = 1; return 1;
    }
#endif
#ifdef MINA_WINDOWS
    if (!strcmp(name, "winmm")) {
        d->impl = mina_winmm_open(d->sample_rate, d->channels);
        if (!d->impl) return 0;
        d->is_real = 1; return 1;
    }
#endif
#ifdef MINA_MACOS
    if (!strcmp(name, "coreaudio")) {
        d->impl = mina_coreaudio_open(d->sample_rate, d->channels);
        if (!d->impl) return 0;
        d->is_real = 1; return 1;
    }
#endif
#ifdef MINA_DOS
    if (!strcmp(name, "pcspeaker")) { d->is_real = 1; return 1; }
    if (!strcmp(name, "adlib"))     { mina_adlib_reset(); d->is_real = 1; return 1; }
    if (!strcmp(name, "midi"))      { mina_midi_reset();  d->is_real = 1; return 1; }
#endif
    return 0;
}

mina_device *mina_device_open(const char *backend, mina_u32 sample_rate,
                              mina_u32 channels) {
    static const char *order[] = {
#if defined(MINA_LINUX) || defined(MINA_BSD) || defined(MINA_UNIX) || \
    defined(MINA_POSIX_GENERIC)
        "alsa", "pulse", "oss",
#endif
#if defined(MINA_MACOS)
        "coreaudio",
#endif
#if defined(MINA_WINDOWS)
        "winmm",
#endif
#if defined(MINA_DOS)
        "pcspeaker", "adlib", "midi",
#endif
        "null", NULL
    };
    mina_device *d = (mina_device *)MINA_MALLOC(sizeof(mina_device));
    int i;
    if (!d) return NULL;
    memset(d, 0, sizeof(*d));
    d->sample_rate = sample_rate ? sample_rate : 44100u;
    d->channels = channels ? channels : 2u;
    d->fd = -1;

    if (backend && backend[0]) {
        if (mina_device_try(d, backend)) {
            mina_strcpy_n(d->backend, sizeof(d->backend), backend);
            return d;
        }
        /* an explicitly requested backend that will not open still falls
         * through, so playback never becomes a hard failure */
    }
    for (i = 0; order[i]; i++) {
        if (backend && backend[0] && !strcmp(order[i], backend)) continue;
        if (mina_device_try(d, order[i])) {
            mina_strcpy_n(d->backend, sizeof(d->backend), order[i]);
            return d;
        }
    }
    mina_strcpy_n(d->backend, sizeof(d->backend), "null");
    d->is_real = 0;
    return d;
}

void mina_device_set_path(mina_device *d, const char *path) {
    if (d && path) mina_strcpy_n(d->path, sizeof(d->path), path);
}
const char *mina_device_backend(const mina_device *d) {
    return d ? d->backend : "";
}
mina_bool mina_device_is_real(const mina_device *d) {
    return d ? (mina_bool)d->is_real : 0;
}

mina_result mina_device_write(mina_device *d, const float *interleaved,
                              mina_u32 frames) {
    if (!d) return MINA_ERR_PARAM;
    if (!frames) return MINA_OK;
    if (!interleaved) return MINA_ERR_PARAM;

#ifndef MINA_NO_STDIO
    if (d->is_file && d->fp) {
        size_t n = (size_t)frames * d->channels;
        mina_i16 *tmp = mina_dev_scratch(d, n);
        if (!tmp) return MINA_ERR_NOMEM;
        mina_convert_from_f32(interleaved, MINA_FMT_S16, d->channels, n, tmp);
        if (fwrite(tmp, sizeof(mina_i16), n, d->fp) != n) return MINA_ERR_IO;
        d->file_bytes = mina_u64_add(d->file_bytes,
                                     mina_u64_from((mina_u32)(n * sizeof(mina_i16))));
        return MINA_OK;
    }
#endif
#ifdef MINA_HAVE_POSIX_AUDIO
    if (!strcmp(d->backend, "alsa")  && d->impl) return mina_alsa_write(d, interleaved, frames);
    if (!strcmp(d->backend, "pulse") && d->impl) return mina_pulse_write(d, interleaved, frames);
    if (!strcmp(d->backend, "oss")   && d->fd >= 0) return mina_oss_write(d, interleaved, frames);
#endif
#ifdef MINA_WINDOWS
    if (!strcmp(d->backend, "winmm") && d->impl) return mina_winmm_write(d, interleaved, frames);
#endif
#ifdef MINA_MACOS
    if (!strcmp(d->backend, "coreaudio") && d->impl)
        return mina_coreaudio_write(d, interleaved, frames);
#endif
#ifdef MINA_DOS
    if (!strcmp(d->backend, "pcspeaker")) {
        mina_pcspeaker_write_impl(d, interleaved, frames);
        return MINA_OK;
    }
    if (!strcmp(d->backend, "adlib")) {
        mina_u32 i, zc = 0; int prev, cur;
        double energy = 0.0;
        for (i = 0; i < frames; i++) {
            float v = interleaved[(size_t)i * d->channels];
            energy += (double)v * (double)v;
        }
        prev = (interleaved[0] >= 0.0f);
        for (i = 1; i < frames; i++) {
            cur = (interleaved[(size_t)i * d->channels] >= 0.0f);
            if (cur != prev) zc++;
            prev = cur;
        }
        if (energy / (double)(frames ? frames : 1) < 1e-4) mina_adlib_silence();
        else mina_adlib_note((double)zc * (double)d->sample_rate / (2.0 * (double)frames));
        return MINA_OK;
    }
    if (!strcmp(d->backend, "midi")) {
        mina_u32 i, zc = 0; int prev, cur, note;
        double energy = 0.0, freq;
        for (i = 0; i < frames; i++) {
            float v = interleaved[(size_t)i * d->channels];
            energy += (double)v * (double)v;
        }
        prev = (interleaved[0] >= 0.0f);
        for (i = 1; i < frames; i++) {
            cur = (interleaved[(size_t)i * d->channels] >= 0.0f);
            if (cur != prev) zc++;
            prev = cur;
        }
        if (energy / (double)(frames ? frames : 1) < 1e-4) {
            mina_midi_send(0xB0); mina_midi_send(0x7B); mina_midi_send(0x00);
            return MINA_OK;
        }
        freq = (double)zc * (double)d->sample_rate / (2.0 * (double)frames);
        if (freq < 20.0) freq = 20.0;
        note = (int)(69.0 + 12.0 * log(freq / 440.0) / log(2.0));
        if (note < 0) note = 0;
        if (note > 127) note = 127;
        mina_midi_send(0x90); mina_midi_send(note); mina_midi_send(100);
        return MINA_OK;
    }
#endif
    return MINA_OK;      /* null sink */
}

void mina_device_close(mina_device *d) {
    if (!d) return;
#ifndef MINA_NO_STDIO
    if (d->is_file && d->fp) {
        long data_bytes = ftell(d->fp) - 44;
        mina_u8 b[4];
        if (data_bytes < 0) data_bytes = 0;
        if (fseek(d->fp, 4, SEEK_SET) == 0) {
            mina_put_le32(b, (mina_u32)(data_bytes + 36));
            fwrite(b, 1, 4, d->fp);
        }
        if (fseek(d->fp, 40, SEEK_SET) == 0) {
            mina_put_le32(b, (mina_u32)data_bytes);
            fwrite(b, 1, 4, d->fp);
        }
        fclose(d->fp);
        d->fp = NULL;
    }
#endif
#ifdef MINA_HAVE_POSIX_AUDIO
    if (!strcmp(d->backend, "alsa")  && d->impl) mina_alsa_close(d);
    if (!strcmp(d->backend, "pulse") && d->impl) mina_pulse_close(d);
    if (d->fd >= 0) { close(d->fd); d->fd = -1; }
#endif
#ifdef MINA_WINDOWS
    if (!strcmp(d->backend, "winmm") && d->impl) mina_winmm_close(d);
#endif
#ifdef MINA_MACOS
    if (!strcmp(d->backend, "coreaudio") && d->impl) mina_coreaudio_close(d);
#endif
#ifdef MINA_DOS
    if (!strcmp(d->backend, "pcspeaker")) mina_outb(0x61, mina_inb(0x61) & 0xFC);
    if (!strcmp(d->backend, "adlib")) mina_adlib_silence();
    if (!strcmp(d->backend, "midi")) {
        mina_midi_send(0xB0); mina_midi_send(0x7B); mina_midi_send(0x00);
    }
#endif
    MINA_FREE(d->scratch);
    MINA_FREE(d);
}

#else  /* MINA_NO_DEVICES */

struct mina_device { int dummy; };
mina_device *mina_device_open(const char *backend, mina_u32 sample_rate,
                              mina_u32 channels) {
    (void)backend; (void)sample_rate; (void)channels; return NULL;
}
mina_result mina_device_write(mina_device *d, const float *s, mina_u32 f) {
    (void)d; (void)s; (void)f; return MINA_ERR_DEVICE;
}
void mina_device_close(mina_device *d) { (void)d; }
const char *mina_device_backend(const mina_device *d) { (void)d; return ""; }
mina_bool mina_device_is_real(const mina_device *d) { (void)d; return 0; }
void mina_device_set_path(mina_device *d, const char *p) { (void)d; (void)p; }

#endif /* MINA_NO_DEVICES */

/* ------------------------------------------------------------------ */
/* Codec registry + dispatch                                            */
/* ------------------------------------------------------------------ */
static mina_codec *g_mina_codecs = NULL;

void mina_codec_register(mina_codec *c) {
    if (!c) return;
    c->next = g_mina_codecs;
    g_mina_codecs = c;
}

mina_codec *mina_codec_first(void) { return g_mina_codecs; }

static mina_codec g_mina_wav_codec = {
    "wav", "WAV (RIFF/WAVE)",
    mina_wav_probe, mina_wav_decode, mina_wav_info, NULL
};
#ifndef MINA_NO_FLAC
static mina_codec g_mina_flac_codec = {
    "flac", "Free Lossless Audio Codec",
    mina_flac_probe, mina_flac_decode, mina_flac_info, NULL
};
#endif
static mina_codec g_mina_ogg_codec = {
    "ogg", "Ogg container (Vorbis/Opus/FLAC/Speex/Theora)",
    mina_ogg_probe, mina_ogg_decode, mina_ogg_info, NULL
};
static mina_codec g_mina_aac_codec = {
    "aac", "AAC (ADTS) / MP4-M4A container",
    mina_aac_probe,
#ifndef MINA_NO_AAC
    mina_aac_decode,
#else
    NULL,
#endif
    mina_aac_info, NULL
};
static mina_codec g_mina_mp3_codec = {
    "mp3", "MPEG-1/2/2.5 Layer III",
    mina_mp3_probe,
#ifndef MINA_NO_MP3
    mina_mp3_decode,
#else
    NULL,
#endif
    mina_mp3_info, NULL
};

static int g_mina_registered = 0;

static void mina_register_builtin_codecs(void) {
    if (g_mina_registered) return;
    g_mina_registered = 1;
    /* registration prepends, so register in reverse probe order.
     * MP3's sync pattern is the loosest, so it is probed last. */
    mina_codec_register(&g_mina_mp3_codec);
    mina_codec_register(&g_mina_aac_codec);
    mina_codec_register(&g_mina_ogg_codec);
#ifndef MINA_NO_FLAC
    mina_codec_register(&g_mina_flac_codec);
#endif
    mina_codec_register(&g_mina_wav_codec);
}

static mina_codec *mina_find_codec(const mina_u8 *d, size_t n) {
    mina_codec *c;
    for (c = g_mina_codecs; c; c = c->next)
        if (c->probe && c->probe(d, n)) return c;
    return NULL;
}

mina_result mina_decode(const void *data, size_t size, mina_pcm *out) {
    mina_codec *c;
    if (!out) return MINA_ERR_PARAM;
    out->samples = NULL;
    out->frames = mina_u64_from(0);
    out->channels = 0;
    out->sample_rate = 0;
    if (!data || !size) return MINA_ERR_PARAM;
    mina_register_builtin_codecs();
    c = mina_find_codec((const mina_u8 *)data, size);
    if (!c) return MINA_ERR_NOTFOUND;
    if (!c->decode) return MINA_ERR_UNSUPPORTED;
    return c->decode((const mina_u8 *)data, size, out);
}

mina_result mina_info(const void *data, size_t size, mina_fileinfo *out) {
    mina_codec *c;
    if (!out) return MINA_ERR_PARAM;
    memset(out, 0, sizeof(*out));
    out->total_frames = mina_u64_from(0);
    if (!data || !size) return MINA_ERR_PARAM;
    mina_register_builtin_codecs();
    c = mina_find_codec((const mina_u8 *)data, size);
    if (!c) return MINA_ERR_NOTFOUND;
    if (!c->info) return MINA_ERR_UNSUPPORTED;
    return c->info((const mina_u8 *)data, size, out);
}

mina_u64 mina_u64_of(mina_u32 v)       { return mina_u64_from(v); }
double   mina_u64_to_double(mina_u64 v) { return mina_u64_dbl(v); }
size_t   mina_u64_to_size(mina_u64 v)   { return mina_u64_size(v, NULL); }

void mina_pcm_free(mina_pcm *pcm) {
    if (!pcm) return;
    MINA_FREE(pcm->samples);
    pcm->samples = NULL;
    pcm->frames = mina_u64_from(0);
    pcm->channels = 0;
    pcm->sample_rate = 0;
}


/* ------------------------------------------------------------------ */
/* Threads                                                              */
/*                                                                      */
/* Win32 down to Windows 95: CRITICAL_SECTION, CreateThread and an       */
/* auto-reset event. Deliberately not CONDITION_VARIABLE (Vista) or      */
/* TryEnterCriticalSection (NT only) - neither exists on 9x. POSIX       */
/* threads everywhere else, and nothing at all on DOS, where             */
/* mina_have_threads() reports 0 and the engine stays pump-driven.       */
/*                                                                      */
/* Every wait re-tests its predicate and times out, so a lost wake-up    */
/* costs one poll instead of a hang - which matters on 9x, where the     */
/* event primitives are the only ones available.                         */
/* ------------------------------------------------------------------ */
#if !defined(MINA_NO_THREADS)
#  if defined(MINA_WINDOWS)
#    define MINA_HAVE_THREADS 1
#    ifndef WIN32_LEAN_AND_MEAN
#      define WIN32_LEAN_AND_MEAN
#    endif
#    include <windows.h>
#  elif defined(MINA_LINUX) || defined(MINA_MACOS) || defined(MINA_BSD) || \
        defined(MINA_UNIX)
#    define MINA_HAVE_THREADS 1
#    include <pthread.h>
#    include <sys/time.h>
#    include <time.h>
#  endif
#endif

#ifdef MINA_HAVE_THREADS
#if defined(MINA_WINDOWS)

typedef CRITICAL_SECTION mina_lock;
typedef HANDLE           mina_cond;
typedef HANDLE           mina_thread;
#define MINA_THREADFN(name, arg) static DWORD WINAPI name(LPVOID arg)
#define MINA_THREADRET 0
typedef DWORD (WINAPI *mina_threadfn)(LPVOID);

static void mina_lock_init(mina_lock *l) { InitializeCriticalSection(l); }
static void mina_lock_free(mina_lock *l) { DeleteCriticalSection(l); }
static void mina_lock_take(mina_lock *l) { EnterCriticalSection(l); }
static void mina_lock_drop(mina_lock *l) { LeaveCriticalSection(l); }

static int  mina_cond_init(mina_cond *c) {
    *c = CreateEventA(NULL, FALSE, FALSE, NULL);
    return *c != NULL;
}
static void mina_cond_free(mina_cond *c) { if (*c) CloseHandle(*c); *c = NULL; }
static void mina_cond_wake(mina_cond *c) { if (*c) SetEvent(*c); }
static void mina_cond_wait(mina_cond *c, mina_lock *l, mina_u32 ms) {
    LeaveCriticalSection(l);
    if (*c) WaitForSingleObject(*c, (DWORD)ms);
    else    Sleep((DWORD)ms);
    EnterCriticalSection(l);
}
static mina_u32 mina_now_ms(void) { return (mina_u32)GetTickCount(); }

static int mina_thread_start(mina_thread *t, mina_threadfn fn, void *arg) {
    DWORD id = 0;
    *t = CreateThread(NULL, 0, fn, arg, 0, &id);
    return *t != NULL;
}
static void mina_thread_join(mina_thread *t) {
    if (!*t) return;
    WaitForSingleObject(*t, INFINITE);
    CloseHandle(*t);
    *t = NULL;
}

#else  /* POSIX */

typedef pthread_mutex_t mina_lock;
typedef pthread_cond_t  mina_cond;
typedef pthread_t       mina_thread;
#define MINA_THREADFN(name, arg) static void *name(void *arg)
#define MINA_THREADRET NULL
typedef void *(*mina_threadfn)(void *);

static void mina_lock_init(mina_lock *l) { pthread_mutex_init(l, NULL); }
static void mina_lock_free(mina_lock *l) { pthread_mutex_destroy(l); }
static void mina_lock_take(mina_lock *l) { pthread_mutex_lock(l); }
static void mina_lock_drop(mina_lock *l) { pthread_mutex_unlock(l); }

static int  mina_cond_init(mina_cond *c) { return pthread_cond_init(c, NULL) == 0; }
static void mina_cond_free(mina_cond *c) { pthread_cond_destroy(c); }
static void mina_cond_wake(mina_cond *c) { pthread_cond_signal(c); }
static void mina_cond_wait(mina_cond *c, mina_lock *l, mina_u32 ms) {
    struct timespec ts;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ts.tv_sec  = tv.tv_sec + (time_t)(ms / 1000u);
    ts.tv_nsec = (long)tv.tv_usec * 1000L + (long)(ms % 1000u) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) { ts.tv_sec += 1; ts.tv_nsec -= 1000000000L; }
    pthread_cond_timedwait(c, l, &ts);
}
static mina_u32 mina_now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (mina_u32)((mina_u32)tv.tv_sec * 1000u + (mina_u32)(tv.tv_usec / 1000));
}

static int mina_thread_start(mina_thread *t, mina_threadfn fn, void *arg) {
    return pthread_create(t, NULL, fn, arg) == 0;
}
static void mina_thread_join(mina_thread *t) { pthread_join(*t, NULL); }

#endif
#else  /* no threading layer: the loader still needs a clock */

#include <time.h>
static mina_u32 mina_now_ms(void) {
    return (mina_u32)((double)clock() * 1000.0 / (double)CLOCKS_PER_SEC);
}

#endif /* MINA_HAVE_THREADS */

mina_bool mina_have_threads(void) {
#ifdef MINA_HAVE_THREADS
    return 1;
#else
    return 0;
#endif
}

/* ------------------------------------------------------------------ */
/* Streaming decode                                                     */
/*                                                                      */
/* One pull interface over four back ends: a WAV chunk walker, the MP3   */
/* frame loop, the Vorbis packet loop, and - for anything else - the     */
/* whole-buffer decoder run once at open. Each incremental back end is   */
/* the body of the matching mina_*_decode loop with its iteration state  */
/* lifted into the stream, so the samples and the head/tail trims are    */
/* the ones mina_decode would have produced.                            */
/* ------------------------------------------------------------------ */
#define MINA_ST_BUF 0
#define MINA_ST_WAV 1
#define MINA_ST_MP3 2
#define MINA_ST_VRB 3
#define MINA_STREAM_CHUNK 4096u   /* frames one WAV step hands back */

struct mina_stream {
    const mina_u8 *d; size_t n; void *owned;
    int kind; char codec[16];
    mina_u32 ch, sr; mina_u64 total;
    float *pend; size_t pcap, pn, ppos;   /* decoded, not yet read          */
    mina_u64 emitted;                     /* frames handed to the caller    */
    size_t head;                          /* frames still to drop up front  */
    mina_u64 limit; int have_limit;       /* hard cap on emitted frames     */
    int eof;
    mina_wav_hdr wav; mina_format wfmt; size_t wstride, wpos, wframes;
#ifndef MINA_NO_MP3
    mina_mp3_st *mst; mina_mp3_scratch *msc; mina_mp3_header mfirst;
    size_t mpos;
#endif
#ifndef MINA_NO_VORBIS
    mina_ogg_packets ops; mina_vorbis_ctx *v;
    int *vclass; float *vlsp; size_t vpkt; mina_u64 vcount; int vgran;
#endif
};

/* Point the pending buffer at room for `frames` frames and empty it. */
static float *mina_stream_pend(mina_stream *s, size_t frames) {
    size_t want = frames * (size_t)s->ch;
    s->pn = 0; s->ppos = 0;
    if (!want || want > MINA_MAX_SAMPLES) return NULL;
    if (want > s->pcap) {
        float *p = (float *)MINA_REALLOC(s->pend, want * sizeof(float));
        if (!p) return NULL;
        s->pend = p; s->pcap = want;
    }
    return s->pend;
}

#ifndef MINA_NO_MP3
/* Decode one MP3 frame into the pending buffer. 0 = end of stream. */
static int mina_stream_step_mp3(mina_stream *s) {
    mina_mp3_header h;
    while (s->mpos + 4 <= s->n) {
        int mpeg1, mono, nch, sr_idx, my_sr, is_ms, is_ist;
        size_t fsz;
        float *dst;
        if (mina_mp3_parse_header(s->d + s->mpos, &h) != 0 ||
            !mina_mp3_same_format(&h, &s->mfirst)) {
            size_t sync; mina_mp3_header hh;
            if (mina_mp3_find_sync(s->d + s->mpos, s->n - s->mpos, &sync, &hh) != 0)
                return 0;
            if (!mina_mp3_same_format(&hh, &s->mfirst)) return 0;
            s->mpos += sync; h = hh;
        }
        fsz = (size_t)h.frame_len;
        if (fsz < 4 || fsz > s->n - s->mpos) return 0;
        mpeg1 = (h.version == 3);
        mono  = (h.channel_mode == 3);
        nch   = mono ? 1 : 2;
        my_sr = h.srate_idx + (((h.version & 1u) + ((h.version >> 1) & 1u)) * 3);
        sr_idx = my_sr - (my_sr != 0);
        is_ms  = (h.channel_mode == 1) && ((h.mode_ext & 2u) != 0);
        is_ist = (h.channel_mode == 1) && ((h.mode_ext & 1u) != 0);
        dst = mina_stream_pend(s, (size_t)h.samples_per_frame);
        if (!dst) return 0;
        memset(dst, 0, (size_t)h.samples_per_frame * (size_t)nch * sizeof(float));
        mina_mp3_decode_frame(s->mst, s->msc, s->d + s->mpos, (int)fsz, mpeg1,
                              mono, nch, sr_idx, my_sr, is_ms, is_ist, dst);
        s->mpos += fsz;
        s->pn = (size_t)h.samples_per_frame;
        return 1;
    }
    return 0;
}
#endif

#ifndef MINA_NO_VORBIS
/* Decode one Vorbis audio packet into the pending buffer. */
static int mina_stream_step_vorbis(mina_stream *s) {
    while (s->vpkt < s->ops.npkt) {
        const mina_u8 *pk = s->ops.arena + s->ops.pkt[s->vpkt].offset;
        size_t plen = s->ops.pkt[s->vpkt].len;
        size_t idx = s->vpkt++;
        mina_lbr bs;
        float *dst;
        int got;
        if (plen == 0 || (pk[0] & 1)) continue;
        dst = mina_stream_pend(s, (size_t)s->v->blocksize[1]);
        if (!dst) return 0;
        mina_lbr_init(&bs, pk, plen);
        got = mina_vorbis_decode_packet(s->v, &bs, dst, s->vclass, s->vlsp);
        if (got <= 0) continue;
        s->pn = (size_t)got;
        /* The first granule says how many samples precede position zero. */
        s->vcount = mina_u64_add(s->vcount, mina_u64_of((mina_u32)got));
        if (!s->vgran && s->ops.pkt[idx].has_granule) {
            mina_u64 g = s->ops.pkt[idx].granule;
            s->vgran = 1;
            if (mina_u64_gt(s->vcount, g) && idx + 1 < s->ops.npkt)
                s->head += (size_t)mina_u64_lo(mina_u64_sub(s->vcount, g));
        }
        return 1;
    }
    return 0;
}
#endif

/* Refill the pending buffer. 0 once the stream is exhausted. */
static int mina_stream_step(mina_stream *s) {
    switch (s->kind) {
    case MINA_ST_WAV: {
        size_t take = s->wframes - s->wpos, i;
        float *dst;
        if (!take) return 0;
        if (take > MINA_STREAM_CHUNK) take = MINA_STREAM_CHUNK;
        dst = mina_stream_pend(s, take);
        if (!dst) return 0;
        if (s->wstride == (size_t)s->ch * mina_format_size(s->wfmt)) {
            mina_convert_to_f32(s->wav.data_ptr + s->wpos * s->wstride, s->wfmt,
                                s->ch, take * (size_t)s->ch, dst);
        } else {
            for (i = 0; i < take; i++)
                mina_convert_to_f32(s->wav.data_ptr + (s->wpos + i) * s->wstride,
                                    s->wfmt, s->ch, s->ch,
                                    dst + i * (size_t)s->ch);
        }
        s->wpos += take; s->pn = take;
        return 1;
    }
#ifndef MINA_NO_MP3
    case MINA_ST_MP3: return mina_stream_step_mp3(s);
#endif
#ifndef MINA_NO_VORBIS
    case MINA_ST_VRB: return mina_stream_step_vorbis(s);
#endif
    default: return 0;
    }
}

static int mina_stream_init_wav(mina_stream *s) {
    size_t ssize;
    if (mina_wav_parse(s->d, s->n, &s->wav) != MINA_OK) return 0;
    s->wfmt = mina_wav_to_format(&s->wav, memcmp(s->d, "RIFX", 4) == 0);
    if ((int)s->wfmt < 0) return 0;
    ssize = mina_format_size(s->wfmt);
    if (!ssize || !s->wav.channels) return 0;
    s->wstride = (size_t)s->wav.channels * ssize;
    if (s->wav.block_align && (size_t)s->wav.block_align >= s->wstride)
        s->wstride = (size_t)s->wav.block_align;
    s->wframes = s->wav.data_size / s->wstride;
    s->wpos = 0;
    s->ch = s->wav.channels; s->sr = s->wav.sample_rate;
    s->total = mina_u64_of((mina_u32)s->wframes);
    s->kind = MINA_ST_WAV;
    mina_strcpy_n(s->codec, sizeof(s->codec), "wav");
    return 1;
}

#ifndef MINA_NO_MP3
/* Set up the incremental MP3 back end. 0 falls back to a whole decode. */
static int mina_stream_init_mp3(mina_stream *s) {
    mina_mp3_vbrtag tag;
    size_t pos = mina_mp3_id3_skip(s->d, s->n), sync;
    if (pos >= s->n) return 0;
    if (mina_mp3_find_sync(s->d + pos, s->n - pos, &sync, &s->mfirst) != 0) return 0;
    pos += sync;
    if (s->mfirst.layer != 3) return 0;
    mina_mp3_read_vbrtag(s->d, s->n, pos, &s->mfirst, &tag);
    if (tag.present) {
        pos += s->mfirst.frame_len;          /* the tag frame carries no audio */
        if (tag.have_lame) {
            s->head = (size_t)tag.enc_delay + MINA_MP3_DELAY;
            if (tag.frames) {
                mina_u64 tot = mina_u64_mul32(tag.frames, s->mfirst.samples_per_frame);
                mina_u64 trim = mina_u64_of(tag.enc_delay + tag.enc_padding);
                if (mina_u64_gt(tot, trim)) {
                    s->limit = mina_u64_sub(tot, trim);
                    s->have_limit = 1;
                }
            }
        }
    }
    s->mst = (mina_mp3_st *)MINA_MALLOC(sizeof(mina_mp3_st));
    s->msc = (mina_mp3_scratch *)MINA_MALLOC(sizeof(mina_mp3_scratch));
    if (!s->mst || !s->msc) return 0;
    memset(s->mst, 0, sizeof(*s->mst));
    if (!mina_mp3_expand_huffman(s->mst)) return 0;
    s->mpos = pos;
    s->ch = s->mfirst.channels; s->sr = s->mfirst.sample_rate;
    s->total = s->have_limit ? s->limit : mina_u64_of(0);
    s->kind = MINA_ST_MP3;
    mina_strcpy_n(s->codec, sizeof(s->codec), "mp3");
    return 1;
}
#endif

#ifndef MINA_NO_VORBIS
/* Set up the incremental Vorbis back end. 0 falls back. */
static int mina_stream_init_vorbis(mina_stream *s) {
    mina_u32 serial = 0, c0 = 0, r0 = 0;
    char codec[16];
    int maxpw = 0, k;
    if (mina_ogg_scan(s->d, s->n, codec, sizeof(codec), &c0, &r0, &serial) != MINA_OK)
        return 0;
    if (strcmp(codec, "vorbis") != 0) return 0;
    if (!mina_ogg_collect(s->d, s->n, serial, &s->ops) || s->ops.npkt < 3) return 0;
    s->v = mina_vorbis_setup(s->ops.arena + s->ops.pkt[0].offset, s->ops.pkt[0].len,
                             s->ops.arena + s->ops.pkt[2].offset, s->ops.pkt[2].len);
    if (!s->v) return 0;
    for (k = 0; k < s->v->nres; k++) {
        int ppw = s->v->books[s->v->residues[k].classbook].dim, partvals, words;
        if (ppw < 1) ppw = 1;
        partvals = (s->v->blocksize[1] / 2 * s->v->channels) /
                   (s->v->residues[k].part_size > 0 ? s->v->residues[k].part_size : 1);
        words = (partvals + ppw) * ppw;
        if (words > maxpw) maxpw = words;
    }
    s->vclass = (int *)MINA_MALLOC((size_t)(maxpw + 64) *
                                   (size_t)s->v->channels * sizeof(int));
    s->vlsp = (float *)MINA_MALLOC((size_t)s->v->channels * 256 * sizeof(float));
    if (!s->vclass || !s->vlsp) return 0;
    s->vpkt = 3;
    s->vcount = mina_u64_of(0);
    s->ch = (mina_u32)s->v->channels; s->sr = (mina_u32)s->v->sample_rate;
    s->limit = s->total = mina_ogg_last_granule(s->d, s->n, serial);
    s->have_limit = !mina_u64_zero(s->limit);
    s->kind = MINA_ST_VRB;
    mina_strcpy_n(s->codec, sizeof(s->codec), "vorbis");
    return 1;
}
#endif

/* Release whatever back end is live, leaving the stream reusable. */
static void mina_stream_teardown(mina_stream *s) {
#ifndef MINA_NO_MP3
    MINA_FREE(s->mst); s->mst = NULL;
    MINA_FREE(s->msc); s->msc = NULL;
#endif
#ifndef MINA_NO_VORBIS
    if (s->v) { mina_vorbis_free(s->v); s->v = NULL; }
    MINA_FREE(s->vclass); s->vclass = NULL;
    MINA_FREE(s->vlsp);   s->vlsp = NULL;
    if (s->ops.arena || s->ops.pkt) mina_ogg_packets_free(&s->ops);
    memset(&s->ops, 0, sizeof(s->ops));
#endif
}

/* Bring a stream to its opening state without re-reading the image. */
static int mina_stream_start(mina_stream *s) {
    mina_pcm pcm;
    mina_fileinfo fi;
    mina_result r;
    int got = 0;
    s->pn = s->ppos = 0; s->head = 0; s->eof = 0;
    s->emitted = mina_u64_of(0);
    s->limit = mina_u64_of(0); s->have_limit = 0;
    s->total = mina_u64_of(0);
#ifndef MINA_NO_VORBIS
    s->vgran = 0;
#endif
    mina_strcpy_n(s->codec, sizeof(s->codec), "pcm");
    if (mina_wav_probe(s->d, s->n)) got = mina_stream_init_wav(s);
#ifndef MINA_NO_MP3
    else if (mina_mp3_probe(s->d, s->n)) got = mina_stream_init_mp3(s);
#endif
#ifndef MINA_NO_VORBIS
    else if (mina_ogg_probe(s->d, s->n)) got = mina_stream_init_vorbis(s);
#endif
    if (got) return 1;

    /* No incremental back end, or it failed: decode the whole image once. */
    mina_stream_teardown(s);
    MINA_FREE(s->pend); s->pend = NULL; s->pcap = 0;
    memset(&pcm, 0, sizeof(pcm));
    r = mina_decode(s->d, s->n, &pcm);
    if ((r != MINA_OK && r != MINA_ERR_TRUNCATED) || !pcm.samples || !pcm.channels) {
        mina_pcm_free(&pcm);
        return 0;
    }
    if (mina_info(s->d, s->n, &fi) == MINA_OK)
        mina_strcpy_n(s->codec, sizeof(s->codec), fi.codec);
    s->ch = pcm.channels; s->sr = pcm.sample_rate; s->total = pcm.frames;
    s->pend = pcm.samples; pcm.samples = NULL;
    s->pn = mina_u64_to_size(pcm.frames);
    s->pcap = s->pn * (size_t)s->ch;
    s->ppos = 0; s->kind = MINA_ST_BUF;
    return 1;
}

mina_stream *mina_stream_open(const void *data, size_t size, int own) {
    mina_stream *s;
    if (!data || !size) return NULL;
    s = (mina_stream *)MINA_MALLOC(sizeof(mina_stream));
    if (!s) return NULL;
    memset(s, 0, sizeof(*s));
    s->d = (const mina_u8 *)data;
    s->n = size;
    if (own) {
        union { const void *c; void *v; } u;
        u.c = data;
        s->owned = u.v;
    }
    mina_register_builtin_codecs();
    if (!mina_stream_start(s)) { mina_stream_close(s); return NULL; }
    return s;
}

void mina_stream_close(mina_stream *s) {
    if (!s) return;
    mina_stream_teardown(s);
    MINA_FREE(s->pend);
    MINA_FREE(s->owned);
    MINA_FREE(s);
}

int mina_stream_rewind(mina_stream *s) {
    if (!s) return 0;
    if (s->kind == MINA_ST_BUF) {
        s->ppos = 0; s->eof = 0;
        s->emitted = mina_u64_of(0);
        return 1;
    }
    mina_stream_teardown(s);
    return mina_stream_start(s);
}

mina_u32 mina_stream_channels(const mina_stream *s) { return s ? s->ch : 0; }
mina_u32 mina_stream_rate    (const mina_stream *s) { return s ? s->sr : 0; }
mina_u64 mina_stream_frames  (const mina_stream *s) {
    return s ? s->total : mina_u64_of(0);
}
mina_bool mina_stream_incremental(const mina_stream *s) {
    return (mina_bool)((s && s->kind != MINA_ST_BUF) ? 1 : 0);
}
const char *mina_stream_codec(const mina_stream *s) { return s ? s->codec : ""; }

mina_u32 mina_stream_read(mina_stream *s, float *out, mina_u32 frames) {
    mina_u32 done = 0;
    if (!s || !out || !frames || !s->ch) return 0;
    while (done < frames && !s->eof) {
        size_t avail, take;
        if (s->ppos >= s->pn) {
            if (!mina_stream_step(s)) { s->eof = 1; break; }
            continue;
        }
        avail = s->pn - s->ppos;
        if (s->head) {                       /* encoder delay, dropped here */
            take = s->head < avail ? s->head : avail;
            s->ppos += take; s->head -= take;
            continue;
        }
        take = (size_t)(frames - done);
        if (take > avail) take = avail;
        if (s->have_limit) {                 /* gapless / granule tail trim */
            mina_u64 room;
            if (!mina_u64_gt(s->limit, s->emitted)) { s->eof = 1; break; }
            room = mina_u64_sub(s->limit, s->emitted);
            if (mina_u64_lt(room, mina_u64_of((mina_u32)take)))
                take = (size_t)mina_u64_lo(room);
            if (!take) { s->eof = 1; break; }
        }
        memcpy(out + (size_t)done * (size_t)s->ch,
               s->pend + s->ppos * (size_t)s->ch,
               take * (size_t)s->ch * sizeof(float));
        s->ppos += take;
        done += (mina_u32)take;
        s->emitted = mina_u64_add(s->emitted, mina_u64_of((mina_u32)take));
    }
    return done;
}

/* ------------------------------------------------------------------ */
/* Playback engine                                                      */
/*                                                                      */
/* A fixed-capacity bank of named voices mixed into one interleaved      */
/* block. Everything a voice needs is allocated when it starts, so       */
/* mina_engine_render does no allocation and takes no branch that        */
/* depends on the host. Clips are decoded, channel-mapped and resampled  */
/* to the engine's format once and shared by reference count; streams    */
/* are pulled a chunk at a time through the same conversion chain.       */
/* ------------------------------------------------------------------ */
#define MINA_NAME_MAX  48
#define MINA_PATH_MAX  512
#define MINA_RAW_FRAMES 1024u   /* frames pulled from a stream per refill */
#ifndef MINA_CACHE_LIMIT
#define MINA_CACHE_LIMIT ((size_t)64 << 20)
#endif

typedef struct {
    char path[MINA_PATH_MAX];
    mina_clip *c;
    size_t bytes;
    mina_u32 used;          /* cache clock stamp, for eviction */
} mina_cache_ent;

typedef struct {
    mina_u32 slot, gen;     /* the reservation this load belongs to */
    int stream, keep;       /* keep: never dropped for missing its deadline */
    mina_u32 queued;
    char path[MINA_PATH_MAX];
} mina_loadreq;

struct mina_clip {
    int refs;
    mina_u32 channels;
    size_t frames;
    float *s;
};

typedef struct {
    char name[MINA_NAME_MAX];
    int used;
    mina_clip *clip;
    mina_stream *st;
    mina_source_params p;
    double cur, step;
    float vol, vol_to, vol_ramp;   /* per-frame linear ramp */
    int stop_at_ramp;
    /* stream chain: raw (stream format) -> rq (engine channels, stream rate)
     * -> sb (engine channels, engine rate). `cur` indexes sb. */
    float *raw; mina_u32 rawch;
    float *rq;  mina_u32 rqcap, rqn, rqpos;
    float *sb;  mina_u32 sbcap, sbn;
    mina_resampler *rs;
    int drained;
    int loading;            /* reserved, waiting on the loader thread */
    mina_u32 gen;           /* stamps this reservation */
} mina_voice;

struct mina_engine {
    mina_device *dev;
    mina_u32 rate, channels, block, nvoice;
    mina_voice *v;
    float *mixbuf;
    float master;
    float lx, ly, lz, rx, ry, rz;   /* listener position and right vector */
    void (*lock)(void *);
    void (*unlock)(void *);
    void *lock_user;
    /* asynchronous loading */
    mina_u32 nextgen;
    mina_loader_fn loader; void *loader_user;
    int quality;
    mina_u32 deadline_ms;
    mina_loadreq *lq; mina_u32 lqn, lqcap;
    /* decoded-clip cache, keyed by path */
    mina_cache_ent *cache; mina_u32 ncache, cache_cap, cache_clock;
    size_t cache_bytes, cache_limit;
    int quit, dev_failed;
#ifdef MINA_HAVE_THREADS
    mina_lock lk; mina_cond cv;
    mina_thread th_mix, th_load;
    int running, mixing, have_lock;
#endif
};

#define MINA_ELOCK(e)   do { if ((e)->lock)   (e)->lock((e)->lock_user);   } while (0)
#define MINA_EUNLOCK(e) do { if ((e)->unlock) (e)->unlock((e)->lock_user); } while (0)

void mina_source_params_init(mina_source_params *p) {
    if (!p) return;
    p->loop = 0; p->spatial = 0; p->stream = 0;
    p->x = p->y = p->z = 0.0f;
    p->min_distance = 1.0f; p->max_distance = 16.0f; p->rolloff = 1.0f;
}

/* Map `frames` interleaved frames from sc channels to dc. Extra destination
 * channels repeat the last source channel; extra source channels are
 * dropped, so 5.1 into stereo keeps the front pair. */
static void mina_chmap(const float *src, mina_u32 sc, float *dst, mina_u32 dc,
                       size_t frames) {
    size_t i;
    mina_u32 j;
    if (sc == dc) { memcpy(dst, src, frames * (size_t)dc * sizeof(float)); return; }
    for (i = 0; i < frames; i++) {
        const float *s = src + i * (size_t)sc;
        float *d = dst + i * (size_t)dc;
        for (j = 0; j < dc; j++) d[j] = s[j < sc ? j : sc - 1];
    }
}

/* ---- clips ---- */

mina_clip *mina_clip_retain(mina_clip *c) { if (c) c->refs++; return c; }

void mina_clip_release(mina_clip *c) {
    if (!c || --c->refs > 0) return;
    MINA_FREE(c->s);
    MINA_FREE(c);
}

mina_u64 mina_clip_frames(const mina_clip *c) {
    return mina_u64_of(c ? (mina_u32)c->frames : 0);
}

mina_clip *mina_clip_from_pcm(mina_engine *e, const float *in, mina_u64 frames,
                              mina_u32 channels, mina_u32 rate) {
    size_t nf = mina_u64_to_size(frames);
    mina_clip *c;
    float *mapped;
    if (!e || !in || !nf || !channels || !rate) return NULL;
    if (nf > MINA_MAX_SAMPLES / e->channels) return NULL;
    mapped = (float *)MINA_MALLOC(nf * (size_t)e->channels * sizeof(float));
    if (!mapped) return NULL;
    mina_chmap(in, channels, mapped, e->channels, nf);
    if (rate != e->rate) {
        float *res = NULL;
        mina_u64 got = mina_resample_buffer(mapped, mina_u64_of((mina_u32)nf), rate,
                                            e->rate, e->channels,
                                            e->quality, &res);
        MINA_FREE(mapped);
        if (!res) return NULL;
        nf = mina_u64_to_size(got);
        if (!nf || nf > MINA_MAX_SAMPLES / e->channels) { MINA_FREE(res); return NULL; }
        mapped = res;
    }
    c = (mina_clip *)MINA_MALLOC(sizeof(mina_clip));
    if (!c) { MINA_FREE(mapped); return NULL; }
    c->refs = 1; c->channels = e->channels; c->frames = nf; c->s = mapped;
    return c;
}

mina_clip *mina_clip_decode(mina_engine *e, const void *data, size_t size) {
    mina_pcm pcm;
    mina_result r;
    mina_clip *c;
    if (!e || !data || !size) return NULL;
    memset(&pcm, 0, sizeof(pcm));
    r = mina_decode(data, size, &pcm);
    if ((r != MINA_OK && r != MINA_ERR_TRUNCATED) || !pcm.samples ||
        !pcm.channels || !pcm.sample_rate) {
        mina_pcm_free(&pcm);
        return NULL;
    }
    c = mina_clip_from_pcm(e, pcm.samples, pcm.frames, pcm.channels, pcm.sample_rate);
    mina_pcm_free(&pcm);
    return c;
}

#ifndef MINA_NO_STDIO
/* The default loader: read the whole file. */
static mina_u8 *mina_default_loader(const char *path, size_t *size, void *user) {
    FILE *f;
    mina_u8 *b;
    long n;
    (void)user;
    f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    n = ftell(f);
    if (n <= 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    b = (mina_u8 *)MINA_MALLOC((size_t)n);
    if (!b) { fclose(f); return NULL; }
    if (fread(b, 1, (size_t)n, f) != (size_t)n) { MINA_FREE(b); fclose(f); return NULL; }
    fclose(f);
    *size = (size_t)n;
    return b;
}
#endif

/* ---- engine lifetime ---- */

mina_engine *mina_engine_create(const char *backend, mina_u32 rate,
                                mina_u32 channels, mina_u32 block,
                                mina_u32 max_voices) {
    mina_engine *e;
    if (!rate) rate = 44100;
    if (!channels) channels = 2;
    if (!block) block = 1024;
    if (!max_voices) max_voices = 64;
    if (channels > 64 || block > 1u << 20) return NULL;
    e = (mina_engine *)MINA_MALLOC(sizeof(mina_engine));
    if (!e) return NULL;
    memset(e, 0, sizeof(*e));
    e->rate = rate; e->channels = channels; e->block = block;
    e->nvoice = max_voices; e->master = 1.0f;
    e->rx = 1.0f;                       /* +X right, -Z forward, +Y up */
    e->quality = MINA_QUALITY_SINC;
    e->deadline_ms = 250;
    e->cache_limit = MINA_CACHE_LIMIT;
#ifndef MINA_NO_STDIO
    e->loader = mina_default_loader;
#endif
    e->v = (mina_voice *)MINA_MALLOC((size_t)max_voices * sizeof(mina_voice));
    e->mixbuf = (float *)MINA_MALLOC((size_t)block * channels * sizeof(float));
    if (!e->v || !e->mixbuf) { mina_engine_destroy(e); return NULL; }
    memset(e->v, 0, (size_t)max_voices * sizeof(mina_voice));
    if (backend && backend[0] == '\0') e->dev = NULL;
    else e->dev = mina_device_open(backend, rate, channels);
    return e;
}

static void mina_voice_clear(mina_voice *v) {
    mina_clip_release(v->clip);
    mina_stream_close(v->st);
    mina_resampler_free(v->rs);
    MINA_FREE(v->raw); MINA_FREE(v->rq); MINA_FREE(v->sb);
    memset(v, 0, sizeof(*v));
}

void mina_engine_destroy(mina_engine *e) {
    mina_u32 i;
    if (!e) return;
    mina_engine_stop_threads(e);
    for (i = 0; i < e->ncache; i++) mina_clip_release(e->cache[i].c);
    MINA_FREE(e->cache);
    MINA_FREE(e->lq);
    if (e->v) for (i = 0; i < e->nvoice; i++) if (e->v[i].used) mina_voice_clear(&e->v[i]);
    MINA_FREE(e->v);
    MINA_FREE(e->mixbuf);
    mina_device_close(e->dev);
    MINA_FREE(e);
}

mina_bool mina_engine_is_real(const mina_engine *e) {
    return (mina_bool)(e && e->dev && mina_device_is_real(e->dev) ? 1 : 0);
}
const char *mina_engine_backend(const mina_engine *e) {
    return (e && e->dev) ? mina_device_backend(e->dev) : "";
}
mina_u32 mina_engine_rate    (const mina_engine *e) { return e ? e->rate : 0; }
mina_u32 mina_engine_channels(const mina_engine *e) { return e ? e->channels : 0; }
mina_u32 mina_engine_block   (const mina_engine *e) { return e ? e->block : 0; }

void mina_engine_set_lock(mina_engine *e, void (*lock)(void *),
                          void (*unlock)(void *), void *user) {
    if (!e) return;
    e->lock = lock; e->unlock = unlock; e->lock_user = user;
}

void mina_engine_set_master(mina_engine *e, float gain) {
    if (!e) return;
    MINA_ELOCK(e);
    e->master = gain < 0.0f ? 0.0f : gain;
    MINA_EUNLOCK(e);
}

void mina_engine_listener(mina_engine *e, float px, float py, float pz,
                          float fx, float fy, float fz,
                          float ux, float uy, float uz) {
    float rx, ry, rz, len;
    if (!e) return;
    /* right = normalize(forward x up); degenerate input keeps +X */
    rx = fy * uz - fz * uy;
    ry = fz * ux - fx * uz;
    rz = fx * uy - fy * ux;
    len = (float)sqrt((double)(rx * rx + ry * ry + rz * rz));
    MINA_ELOCK(e);
    e->lx = px; e->ly = py; e->lz = pz;
    if (len > 1.0e-6f) { e->rx = rx / len; e->ry = ry / len; e->rz = rz / len; }
    MINA_EUNLOCK(e);
}

/* ---- voice lookup and control ---- */

/* Drop const for the locking query helpers without tripping -Wcast-qual. */
static mina_engine *mina_engine_rw(const mina_engine *e) {
    union { const mina_engine *c; mina_engine *m; } u;
    u.c = e;
    return u.m;
}

static mina_voice *mina_engine_find(mina_engine *e, const char *name) {
    mina_u32 i;
    for (i = 0; i < e->nvoice; i++)
        if (e->v[i].used && strcmp(e->v[i].name, name) == 0) return &e->v[i];
    return NULL;
}

/* Free the named voice if live, then hand back an empty slot, or NULL. */
static mina_voice *mina_engine_slot(mina_engine *e, const char *name) {
    mina_voice *v = mina_engine_find(e, name);
    mina_u32 i;
    if (v) { mina_voice_clear(v); return v; }
    for (i = 0; i < e->nvoice; i++) if (!e->v[i].used) return &e->v[i];
    return NULL;
}

static void mina_voice_begin(mina_engine *e, mina_voice *v, const char *name,
                             const mina_source_params *p, float volume,
                             float pitch) {
    mina_source_params dflt;
    mina_source_params_init(&dflt);
    v->used = 1;
    v->gen = ++e->nextgen;
    mina_strcpy_n(v->name, sizeof(v->name), name);
    v->p = p ? *p : dflt;
    if (v->p.max_distance < 0.0f) v->p.max_distance = 0.0f;
    if (v->p.min_distance < 0.0f) v->p.min_distance = 0.0f;
    if (v->p.rolloff <= 0.0f) v->p.rolloff = 1.0f;
    v->vol = v->vol_to = volume < 0.0f ? 0.0f : volume;
    v->vol_ramp = 0.0f; v->stop_at_ramp = 0;
    v->cur = 0.0;
    if (!(pitch > 0.0f)) pitch = 1.0f;
    if (pitch < 0.01f) pitch = 0.01f;
    if (pitch > 100.0f) pitch = 100.0f;
    v->step = (double)pitch;
}

int mina_engine_play_clip(mina_engine *e, const char *name, mina_clip *c,
                          const mina_source_params *p, float volume, float pitch) {
    mina_voice *v;
    if (!e || !name || !name[0] || !c || !c->frames) return 0;
    MINA_ELOCK(e);
    v = mina_engine_slot(e, name);
    if (v) {
        mina_voice_begin(e, v, name, p, volume, pitch);
        v->clip = mina_clip_retain(c);
    }
    MINA_EUNLOCK(e);
    return v != NULL;
}

int mina_engine_play_memory(mina_engine *e, const char *name,
                            const void *data, size_t size,
                            const mina_source_params *p, float volume, float pitch) {
    mina_clip *c = mina_clip_decode(e, data, size);
    int ok;
    if (!c) return 0;
    ok = mina_engine_play_clip(e, name, c, p, volume, pitch);
    mina_clip_release(c);
    return ok;
}

/* Give a voice its streaming chain and hand it the stream. On failure the
 * voice is cleared, which closes the stream; the caller must not free it. */
static int mina_voice_attach_stream(mina_engine *e, mina_voice *v, mina_stream *s) {
    mina_u32 sc = mina_stream_channels(s), sr = mina_stream_rate(s), sbcap;
    v->st = s;
    if (!sc || !sr) { mina_voice_clear(v); return 0; }
    /* sized so one raw chunk always fits after rate conversion */
    sbcap = (mina_u32)((double)MINA_RAW_FRAMES * (double)e->rate / (double)sr) + 64u;
    if (sbcap < 2048u) sbcap = 2048u;
    v->rawch = sc;
    v->rqcap = MINA_RAW_FRAMES;
    v->sbcap = sbcap;
    v->raw = (float *)MINA_MALLOC((size_t)MINA_RAW_FRAMES * sc * sizeof(float));
    v->rq  = (float *)MINA_MALLOC((size_t)v->rqcap * e->channels * sizeof(float));
    v->sb  = (float *)MINA_MALLOC((size_t)sbcap * e->channels * sizeof(float));
    if (sr != e->rate)
        v->rs = mina_resampler_create(sr, e->rate, e->channels, e->quality);
    if (!v->raw || !v->rq || !v->sb || (sr != e->rate && !v->rs)) {
        mina_voice_clear(v);
        return 0;
    }
    return 1;
}

int mina_engine_play_stream(mina_engine *e, const char *name, mina_stream *s,
                            const mina_source_params *p, float volume, float pitch) {
    mina_voice *v;
    int ok = 0;
    if (!e || !name || !name[0] || !s) { mina_stream_close(s); return 0; }
    MINA_ELOCK(e);
    v = mina_engine_slot(e, name);
    if (v) {
        mina_voice_begin(e, v, name, p, volume, pitch);
        ok = mina_voice_attach_stream(e, v, s);
    } else {
        mina_stream_close(s);
    }
    MINA_EUNLOCK(e);
    return ok;
}

void mina_engine_stop(mina_engine *e, const char *name) {
    mina_voice *v;
    if (!e || !name) return;
    MINA_ELOCK(e);
    v = mina_engine_find(e, name);
    if (v) mina_voice_clear(v);
    MINA_EUNLOCK(e);
}

void mina_engine_stop_all(mina_engine *e) {
    mina_u32 i;
    if (!e) return;
    MINA_ELOCK(e);
    for (i = 0; i < e->nvoice; i++) if (e->v[i].used) mina_voice_clear(&e->v[i]);
    MINA_EUNLOCK(e);
}

/* Set a linear volume ramp, optionally stopping the voice when it lands. */
static void mina_voice_ramp(mina_engine *e, mina_voice *v, float target,
                            float seconds, int stop) {
    if (target < 0.0f) target = 0.0f;
    if (!(seconds > 0.0f)) {
        v->vol = v->vol_to = target;
        v->vol_ramp = 0.0f;
        v->stop_at_ramp = 0;
        if (stop) mina_voice_clear(v);
        return;
    }
    v->vol_to = target;
    v->vol_ramp = (target - v->vol) / (seconds * (float)e->rate);
    if (v->vol_ramp == 0.0f) v->vol_ramp = target > v->vol ? 1.0e-7f : -1.0e-7f;
    v->stop_at_ramp = stop;
}

void mina_engine_stop_fade(mina_engine *e, const char *name, float seconds) {
    mina_voice *v;
    if (!e || !name) return;
    MINA_ELOCK(e);
    v = mina_engine_find(e, name);
    if (v) mina_voice_ramp(e, v, 0.0f, seconds, 1);
    MINA_EUNLOCK(e);
}

int mina_engine_fade(mina_engine *e, const char *name, float volume, float seconds) {
    mina_voice *v;
    if (!e || !name) return 0;
    MINA_ELOCK(e);
    v = mina_engine_find(e, name);
    if (v) mina_voice_ramp(e, v, volume, seconds, 0);
    MINA_EUNLOCK(e);
    return v != NULL;
}

mina_bool mina_engine_playing(const mina_engine *e, const char *name) {
    mina_engine *m = mina_engine_rw(e);
    mina_bool r;
    if (!e || !name) return 0;
    MINA_ELOCK(m);
    r = (mina_bool)(mina_engine_find(m, name) != NULL);
    MINA_EUNLOCK(m);
    return r;
}

mina_u32 mina_engine_voices(const mina_engine *e) {
    mina_engine *m = mina_engine_rw(e);
    mina_u32 i, n = 0;
    if (!e) return 0;
    MINA_ELOCK(m);
    for (i = 0; i < m->nvoice; i++) if (m->v[i].used) n++;
    MINA_EUNLOCK(m);
    return n;
}

int mina_engine_set_volume(mina_engine *e, const char *name, float volume) {
    mina_voice *v;
    if (!e || !name) return 0;
    MINA_ELOCK(e);
    v = mina_engine_find(e, name);
    if (v) {
        v->vol = v->vol_to = volume < 0.0f ? 0.0f : volume;
        v->vol_ramp = 0.0f; v->stop_at_ramp = 0;
    }
    MINA_EUNLOCK(e);
    return v != NULL;
}

int mina_engine_set_pitch(mina_engine *e, const char *name, float pitch) {
    mina_voice *v;
    if (!e || !name) return 0;
    if (!(pitch > 0.01f)) pitch = 0.01f;
    if (pitch > 100.0f) pitch = 100.0f;
    MINA_ELOCK(e);
    v = mina_engine_find(e, name);
    if (v) v->step = (double)pitch;
    MINA_EUNLOCK(e);
    return v != NULL;
}

int mina_engine_set_position(mina_engine *e, const char *name,
                             float x, float y, float z) {
    mina_voice *v;
    if (!e || !name) return 0;
    MINA_ELOCK(e);
    v = mina_engine_find(e, name);
    if (v) { v->p.x = x; v->p.y = y; v->p.z = z; }
    MINA_EUNLOCK(e);
    return v != NULL;
}

int mina_engine_set_loop(mina_engine *e, const char *name, int loop) {
    mina_voice *v;
    if (!e || !name) return 0;
    MINA_ELOCK(e);
    v = mina_engine_find(e, name);
    if (v) v->p.loop = loop;
    MINA_EUNLOCK(e);
    return v != NULL;
}

/* ---- rendering ---- */

/* Distance attenuation and constant-power pan for one voice. */
static void mina_voice_spatial(const mina_engine *e, const mina_voice *v,
                               float *gain, float *gl, float *gr) {
    float dx = v->p.x - e->lx, dy = v->p.y - e->ly, dz = v->p.z - e->lz;
    float d = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
    float g, pan, th;
    if (d >= v->p.max_distance || v->p.max_distance <= v->p.min_distance) {
        g = d >= v->p.max_distance ? 0.0f : 1.0f;
    } else if (d <= v->p.min_distance) {
        g = 1.0f;
    } else {
        g = (v->p.max_distance - d) / (v->p.max_distance - v->p.min_distance);
        if (v->p.rolloff != 1.0f) g = (float)pow((double)g, (double)v->p.rolloff);
    }
    pan = d > 1.0e-4f ? (dx * e->rx + dy * e->ry + dz * e->rz) / d : 0.0f;
    if (pan < -1.0f) pan = -1.0f;
    if (pan >  1.0f) pan =  1.0f;
    th = (pan + 1.0f) * 0.78539816339744831f;   /* -1 -> 0, +1 -> pi/2 */
    *gain = g;
    *gl = (float)cos((double)th);
    *gr = (float)sin((double)th);
}

/* Make sure at least two frames are readable at v->cur. 0 = voice is done. */
static int mina_voice_fill(mina_engine *e, mina_voice *v) {
    mina_u32 ec = e->channels;
    for (;;) {
        mina_u32 i = (mina_u32)v->cur;
        mina_u32 got, used;
        if (v->sbn > i && v->sbn - i >= 2) return 1;
        if (v->drained) return v->sbn > i;
        /* Drop what the cursor has passed. A cursor pitched far enough to
         * run past the whole buffer keeps its remainder and drops again on
         * the next pass, so the skipped frames are still skipped. */
        if (i) {
            mina_u32 drop = i < v->sbn ? i : v->sbn;
            if (drop) {
                memmove(v->sb, v->sb + (size_t)drop * ec,
                        (size_t)(v->sbn - drop) * ec * sizeof(float));
                v->sbn -= drop;
                v->cur -= (double)drop;
            }
        }
        if (v->rqpos >= v->rqn) {                 /* pull the next raw chunk */
            got = mina_stream_read(v->st, v->raw, MINA_RAW_FRAMES);
            if (!got) {
                if (v->p.loop && mina_stream_rewind(v->st))
                    got = mina_stream_read(v->st, v->raw, MINA_RAW_FRAMES);
                if (!got) {                       /* flush the resampler tail */
                    v->drained = 1;
                    if (v->rs && v->sbn < v->sbcap)
                        v->sbn += mina_resampler_flush(v->rs,
                                      v->sb + (size_t)v->sbn * ec,
                                      v->sbcap - v->sbn);
                    continue;
                }
            }
            mina_chmap(v->raw, v->rawch, v->rq, ec, got);
            v->rqn = got; v->rqpos = 0;
        }
        if (v->rs) {
            used = 0;
            got = mina_resampler_process(v->rs, v->rq + (size_t)v->rqpos * ec,
                                         v->rqn - v->rqpos,
                                         v->sb + (size_t)v->sbn * ec,
                                         v->sbcap - v->sbn, &used);
            v->rqpos += used; v->sbn += got;
            if (!got && !used) { v->drained = 1; continue; }
        } else {
            got = v->rqn - v->rqpos;
            if (got > v->sbcap - v->sbn) got = v->sbcap - v->sbn;
            if (!got) { v->drained = 1; continue; }
            memcpy(v->sb + (size_t)v->sbn * ec, v->rq + (size_t)v->rqpos * ec,
                   (size_t)got * ec * sizeof(float));
            v->rqpos += got; v->sbn += got;
        }
    }
}

/* Interleaved linear interpolation between frame i and i+1. */
static void mina_lerp_frame(const float *src, size_t i, size_t last, double f,
                            mina_u32 ec, float *dst) {
    const float *a = src + i * (size_t)ec;
    const float *b = src + (i < last ? i + 1 : last) * (size_t)ec;
    mina_u32 j;
    for (j = 0; j < ec; j++) dst[j] = a[j] + (float)((double)(b[j] - a[j]) * f);
}

/* Mix one voice. Returns 0 when the voice finished and should be freed. */
static int mina_voice_render(mina_engine *e, mina_voice *v, float *out,
                             mina_u32 frames) {
    mina_u32 ec = e->channels, n, j;
    float smp[64], gain = 1.0f, gl = 0.70710678f, gr = 0.70710678f;
    if (v->p.spatial) mina_voice_spatial(e, v, &gain, &gl, &gr);

    /* An unpitched, unpanned clip playing at a steady volume - which is what
     * most voices are - is a plain gain-and-add over a contiguous run, with
     * no interpolation and no per-frame branching. Take that run whole. */
    while (!v->p.spatial && v->clip && v->step == 1.0 && v->vol_ramp == 0.0f &&
           v->cur == (double)(size_t)v->cur && frames) {
        size_t idx = (size_t)v->cur, run;
        const float *src;
        float g = v->vol;
        if (idx >= v->clip->frames) {
            if (!v->p.loop) return 0;
            v->cur = 0.0;
            continue;
        }
        run = v->clip->frames - idx;
        if (run > (size_t)frames) run = (size_t)frames;
        src = v->clip->s + idx * (size_t)ec;
        for (n = 0; n < (mina_u32)run * ec; n++) out[n] += src[n] * g;
        out += run * (size_t)ec;
        frames -= (mina_u32)run;
        v->cur += (double)run;
    }
    if (!frames) return 1;

    for (n = 0; n < frames; n++) {
        const float *src;
        size_t idx, last;
        float g;
        if (v->clip) {
            if (v->cur >= (double)v->clip->frames) {
                if (!v->p.loop) return 0;
                v->cur = fmod(v->cur, (double)v->clip->frames);
            }
            src = v->clip->s;
            last = v->clip->frames - 1;
            idx = (size_t)v->cur;
        } else {
            if (!mina_voice_fill(e, v)) return 0;
            src = v->sb;
            last = v->sbn ? (size_t)v->sbn - 1 : 0;
            idx = (size_t)v->cur;
            if (idx > last) return 0;
        }
        mina_lerp_frame(src, idx, last, v->cur - (double)idx, ec, smp);
        if (v->vol_ramp != 0.0f) {              /* advance the volume ramp */
            v->vol += v->vol_ramp;
            if ((v->vol_ramp > 0.0f && v->vol >= v->vol_to) ||
                (v->vol_ramp < 0.0f && v->vol <= v->vol_to)) {
                v->vol = v->vol_to;
                v->vol_ramp = 0.0f;
                if (v->stop_at_ramp) return 0;
            }
        }
        g = v->vol * gain;
        if (v->p.spatial && ec >= 2) {
            float mono = 0.0f;
            for (j = 0; j < ec; j++) mono += smp[j];
            mono /= (float)ec;
            out[(size_t)n * ec]     += mono * g * gl;
            out[(size_t)n * ec + 1] += mono * g * gr;
        } else {
            for (j = 0; j < ec; j++) out[(size_t)n * ec + j] += smp[j] * g;
        }
        v->cur += v->step;
    }
    return 1;
}

mina_u32 mina_engine_render(mina_engine *e, float *out, mina_u32 frames) {
    mina_u32 i, k, count;
    if (!e || !out || !frames) return 0;
    if (frames > e->block) frames = e->block;
    count = frames * e->channels;
    memset(out, 0, (size_t)count * sizeof(float));
    MINA_ELOCK(e);
    for (i = 0; i < e->nvoice; i++) {
        if (!e->v[i].used || e->v[i].loading) continue;
        if (!mina_voice_render(e, &e->v[i], out, frames)) mina_voice_clear(&e->v[i]);
    }
    for (k = 0; k < count; k++) {               /* master gain and soft clip */
        float x = out[k] * e->master;
        if (x >  1.0f) x =  1.0f + (x - 1.0f) * 0.1f;
        if (x < -1.0f) x = -1.0f + (x + 1.0f) * 0.1f;
        if (x >  1.1f) x =  1.1f;
        if (x < -1.1f) x = -1.1f;
        out[k] = x;
    }
    MINA_EUNLOCK(e);
    return frames;
}

mina_result mina_engine_pump(mina_engine *e) {
    if (!e) return MINA_ERR_PARAM;
    if (!e->dev) return MINA_ERR_UNSUPPORTED;
    mina_engine_render(e, e->mixbuf, e->block);
    return mina_device_write(e->dev, e->mixbuf, e->block);
}

/* ------------------------------------------------------------------ */
/* Engine: clip cache, file loading, and the background threads         */
/*                                                                      */
/* mina_engine_play_file reserves the named voice immediately and hands  */
/* the path to the loader, so the caller never blocks on a disk read or  */
/* a decoder. The reservation carries a generation stamp; the finished   */
/* load is attached only if that exact reservation is still live, which  */
/* makes "play, then at once stop or replace" resolve correctly without  */
/* the loader itself having to be cancellable.                           */
/* ------------------------------------------------------------------ */

/* The engine's own lock, installed over the same hooks a caller may use. */
#ifdef MINA_HAVE_THREADS
static void mina_engine_lock_hook  (void *u) { mina_lock_take(&((mina_engine *)u)->lk); }
static void mina_engine_unlock_hook(void *u) { mina_lock_drop(&((mina_engine *)u)->lk); }
#endif

void mina_engine_set_loader(mina_engine *e, mina_loader_fn fn, void *user) {
    if (!e) return;
    MINA_ELOCK(e);
    e->loader = fn;
    e->loader_user = user;
    MINA_EUNLOCK(e);
}

void mina_engine_set_quality(mina_engine *e, int quality) {
    if (e) e->quality = (quality == MINA_QUALITY_LINEAR) ? MINA_QUALITY_LINEAR
                                                         : MINA_QUALITY_SINC;
}

void mina_engine_set_load_deadline(mina_engine *e, mina_u32 milliseconds) {
    if (e) e->deadline_ms = milliseconds;
}

/* ---- clip cache, keyed by path ---- */

static size_t mina_clip_bytes(const mina_clip *c) {
    return c->frames * (size_t)c->channels * sizeof(float);
}

/* Evict the least recently used clips no voice is holding, until it fits. */
static void mina_cache_trim(mina_engine *e) {
    while (e->cache_bytes > e->cache_limit && e->ncache) {
        mina_u32 i, victim = e->ncache, oldest = 0;
        for (i = 0; i < e->ncache; i++) {
            if (e->cache[i].c->refs != 1) continue;      /* still playing */
            if (victim == e->ncache || e->cache[i].used < oldest) {
                victim = i; oldest = e->cache[i].used;
            }
        }
        if (victim == e->ncache) break;                  /* all of it is in use */
        e->cache_bytes -= e->cache[victim].bytes;
        mina_clip_release(e->cache[victim].c);
        e->cache[victim] = e->cache[--e->ncache];
    }
}

static mina_clip *mina_cache_get(mina_engine *e, const char *path) {
    mina_u32 i;
    for (i = 0; i < e->ncache; i++) {
        if (strcmp(e->cache[i].path, path) == 0) {
            e->cache[i].used = ++e->cache_clock;
            return mina_clip_retain(e->cache[i].c);
        }
    }
    return NULL;
}

static void mina_cache_put(mina_engine *e, const char *path, mina_clip *c) {
    size_t bytes = mina_clip_bytes(c);
    if (!e->cache_limit || bytes > e->cache_limit) return;
    if (e->ncache == e->cache_cap) {
        mina_u32 ncap = e->cache_cap ? e->cache_cap * 2 : 32;
        mina_cache_ent *p = (mina_cache_ent *)MINA_REALLOC(e->cache,
                                (size_t)ncap * sizeof(mina_cache_ent));
        if (!p) return;
        e->cache = p; e->cache_cap = ncap;
    }
    mina_strcpy_n(e->cache[e->ncache].path, MINA_PATH_MAX, path);
    e->cache[e->ncache].c = mina_clip_retain(c);
    e->cache[e->ncache].bytes = bytes;
    e->cache[e->ncache].used = ++e->cache_clock;
    e->ncache++;
    e->cache_bytes += bytes;
    mina_cache_trim(e);
}

void mina_engine_set_cache_limit(mina_engine *e, size_t bytes) {
    if (!e) return;
    MINA_ELOCK(e);
    e->cache_limit = bytes;
    mina_cache_trim(e);
    MINA_EUNLOCK(e);
}

void mina_engine_clear_cache(mina_engine *e) {
    mina_u32 i;
    if (!e) return;
    MINA_ELOCK(e);
    for (i = 0; i < e->ncache; i++) mina_clip_release(e->cache[i].c);
    e->ncache = 0;
    e->cache_bytes = 0;
    MINA_EUNLOCK(e);
}

size_t mina_engine_cache_bytes(const mina_engine *e) {
    return e ? e->cache_bytes : 0;
}

/* ---- servicing one load ---- */

/* Give the finished audio to the reservation that asked for it, or throw it
 * away if that reservation is gone. Either way the slot stops loading. */
static void mina_engine_attach(mina_engine *e, const mina_loadreq *rq,
                               mina_clip *c, mina_stream *st) {
    mina_voice *v = &e->v[rq->slot];
    MINA_ELOCK(e);
    if (!v->used || !v->loading || v->gen != rq->gen) {
        MINA_EUNLOCK(e);                       /* superseded while we worked */
        mina_clip_release(c);
        mina_stream_close(st);
        return;
    }
    v->loading = 0;
    if (c) {
        v->clip = c;
    } else if (st) {
        if (!mina_voice_attach_stream(e, v, st)) st = NULL;   /* cleared the voice */
        st = NULL;
    } else {
        memset(v, 0, sizeof(*v));              /* nothing to play */
    }
    MINA_EUNLOCK(e);
    mina_stream_close(st);
}

/* Read, decode and attach one request. Holds no lock while it decodes. */
static void mina_engine_service(mina_engine *e, const mina_loadreq *rq) {
    mina_clip *c = NULL;
    mina_stream *st = NULL;
    mina_u8 *img = NULL;
    size_t n = 0;
    mina_loader_fn fn;
    void *user;
    int live;

    MINA_ELOCK(e);
    fn = e->loader;
    user = e->loader_user;
    live = !e->quit && e->v[rq->slot].used && e->v[rq->slot].loading &&
           e->v[rq->slot].gen == rq->gen;
    /* A short effect that waited out its deadline is dropped rather than
     * played late: a backlog should not come back as an echo. */
    if (live && e->deadline_ms && !rq->keep &&
        (mina_u32)(mina_now_ms() - rq->queued) > e->deadline_ms) live = 0;
    if (live && !rq->stream) c = mina_cache_get(e, rq->path);
    MINA_EUNLOCK(e);

    if (live && !c) {
        if (fn) img = fn(rq->path, &n, user);
        if (img && n) {
            if (rq->stream) {
                st = mina_stream_open(img, n, 1);      /* the stream owns img */
                img = NULL;
            } else {
                c = mina_clip_decode(e, img, n);
                MINA_FREE(img);
                img = NULL;
                if (c) {
                    MINA_ELOCK(e);
                    mina_cache_put(e, rq->path, c);
                    MINA_EUNLOCK(e);
                }
            }
        }
    }
    MINA_FREE(img);
    mina_engine_attach(e, rq, c, st);
}

/* ---- queueing ---- */

/* Reserve the named voice and queue its load. The caller holds the lock. */
static int mina_engine_queue(mina_engine *e, const char *name, const char *path,
                             const mina_source_params *p, float volume,
                             float pitch) {
    mina_voice *v;
    mina_loadreq *rq;
    if (strlen(path) + 1 > MINA_PATH_MAX) return 0;
    if (e->lqn == e->lqcap) {
        mina_u32 ncap = e->lqcap ? e->lqcap * 2 : 64;
        mina_loadreq *q;
        if (ncap > 4096) return 0;                     /* the queue is the limit */
        q = (mina_loadreq *)MINA_REALLOC(e->lq, (size_t)ncap * sizeof(mina_loadreq));
        if (!q) return 0;
        e->lq = q; e->lqcap = ncap;
    }
    v = mina_engine_slot(e, name);
    if (!v) return 0;
    mina_voice_begin(e, v, name, p, volume, pitch);
    v->loading = 1;
    rq = &e->lq[e->lqn++];
    rq->slot = (mina_u32)(v - e->v);
    rq->gen = v->gen;
    rq->stream = v->p.stream;
    rq->keep = v->p.loop || v->p.stream;
    rq->queued = mina_now_ms();
    mina_strcpy_n(rq->path, MINA_PATH_MAX, path);
    return 1;
}

void mina_engine_service_pending(mina_engine *e) {
    if (!e) return;
    for (;;) {
        mina_loadreq rq;
        MINA_ELOCK(e);
        if (!e->lqn) { MINA_EUNLOCK(e); return; }
        rq = e->lq[0];
        memmove(e->lq, e->lq + 1, (size_t)(--e->lqn) * sizeof(mina_loadreq));
        MINA_EUNLOCK(e);
        mina_engine_service(e, &rq);
    }
}

int mina_engine_play_file(mina_engine *e, const char *name, const char *path,
                          const mina_source_params *p, float volume, float pitch) {
    int ok, threaded;
    if (!e || !name || !name[0] || !path || !path[0]) return 0;
    MINA_ELOCK(e);
    ok = mina_engine_queue(e, name, path, p, volume, pitch);
    MINA_EUNLOCK(e);
    threaded = 0;
#ifdef MINA_HAVE_THREADS
    if (e->running) { mina_cond_wake(&e->cv); threaded = 1; }
#endif
    if (ok && !threaded) mina_engine_service_pending(e);
    return ok;
}

/* ---- threads ---- */

#ifdef MINA_HAVE_THREADS

MINA_THREADFN(mina_engine_mix_thread, arg) {
    mina_engine *e = (mina_engine *)arg;
    for (;;) {
        int quit;
        MINA_ELOCK(e);
        quit = e->quit;
        MINA_EUNLOCK(e);
        if (quit) break;
        mina_engine_render(e, e->mixbuf, e->block);
        /* The device paces this loop: the write blocks until a buffer frees. */
        if (mina_device_write(e->dev, e->mixbuf, e->block) != MINA_OK) {
            MINA_ELOCK(e);
            e->dev_failed = 1;
            MINA_EUNLOCK(e);
            break;
        }
    }
    return MINA_THREADRET;
}

MINA_THREADFN(mina_engine_load_thread, arg) {
    mina_engine *e = (mina_engine *)arg;
    for (;;) {
        mina_loadreq rq;
        mina_lock_take(&e->lk);
        while (!e->quit && !e->lqn) mina_cond_wait(&e->cv, &e->lk, 50);
        if (e->quit) { mina_lock_drop(&e->lk); break; }
        rq = e->lq[0];
        memmove(e->lq, e->lq + 1, (size_t)(--e->lqn) * sizeof(mina_loadreq));
        mina_lock_drop(&e->lk);
        mina_engine_service(e, &rq);
    }
    return MINA_THREADRET;
}

int mina_engine_start(mina_engine *e) {
    if (!e || e->running) return 0;
    mina_lock_init(&e->lk);
    if (!mina_cond_init(&e->cv)) { mina_lock_free(&e->lk); return 0; }
    e->have_lock = 1;
    e->lock = mina_engine_lock_hook;
    e->unlock = mina_engine_unlock_hook;
    e->lock_user = e;
    e->quit = 0;
    if (!mina_thread_start(&e->th_load, mina_engine_load_thread, e)) {
        e->lock = NULL; e->unlock = NULL; e->lock_user = NULL;
        e->have_lock = 0;
        mina_cond_free(&e->cv);
        mina_lock_free(&e->lk);
        return 0;
    }
    e->running = 1;
    /* A mixing thread only makes sense with a device to pace it; without one
     * the caller drives mina_engine_render at whatever rate it likes. */
    if (e->dev && mina_thread_start(&e->th_mix, mina_engine_mix_thread, e))
        e->mixing = 1;
    return 1;
}

void mina_engine_stop_threads(mina_engine *e) {
    if (!e || !e->running) return;
    MINA_ELOCK(e);
    e->quit = 1;
    MINA_EUNLOCK(e);
    mina_cond_wake(&e->cv);
    if (e->mixing) { mina_thread_join(&e->th_mix); e->mixing = 0; }
    mina_thread_join(&e->th_load);
    e->running = 0;
    e->lock = NULL; e->unlock = NULL; e->lock_user = NULL;
    e->have_lock = 0;
    mina_cond_free(&e->cv);
    mina_lock_free(&e->lk);
    e->quit = 0;
}

mina_bool mina_engine_running(const mina_engine *e) {
    return (mina_bool)((e && e->running) ? 1 : 0);
}

#else  /* no threading layer on this platform */

int  mina_engine_start(mina_engine *e) { (void)e; return 0; }
void mina_engine_stop_threads(mina_engine *e) { (void)e; }
mina_bool mina_engine_running(const mina_engine *e) { (void)e; return 0; }

#endif /* MINA_HAVE_THREADS */

mina_bool mina_engine_device_failed(const mina_engine *e) {
    mina_engine *m = mina_engine_rw(e);
    mina_bool r;
    if (!e) return 0;
    MINA_ELOCK(m);
    r = (mina_bool)(m->dev_failed ? 1 : 0);
    MINA_EUNLOCK(m);
    return r;
}
#endif /* MINA_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif

/* ------------------------------------------------------------------ */
/* License                                                              */
/* ------------------------------------------------------------------ */
/*
MIT License

Copyright (c) 2026 mina contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---------------------------------------------------------------------------

Codec lookup arrays contain required numeric decoder data. Mina stores its
MP3 Huffman data in a project-specific run-length representation and does not
include ISO publication text or third-party implementation source.
*/

#endif /* MINA_H */
