# mina.h — the audio layer

One header. Strict ANSI C89. No runtime dependencies, no codec libraries, no
build system. Decodes WAV, FLAC, MP3, Ogg Vorbis and AAC, streams them, mixes
named voices with distance and panning, and plays them on whatever output the
host has.

```c
#define MINA_IMPLEMENTATION   /* in exactly one translation unit */
#include "mina.h"
```

In this repo that one translation unit is
`src/net/minecraft/client/platform/audio/AudioEngine.cpp`. There is no wrapper
class around it — `AudioEngine` calls `mina_*` directly.

---

## Provenance

Based on upstream mina 2.0.0 (`C:\Users\Eddie\Desktop\Custom Programs\mina2.0.0\mina`),
with four things that are **local to this repo and not in upstream**. They must
be re-applied on every upstream refresh:

| Local addition | Where |
|---|---|
| **AAC LC decoder** — codebooks, decoder, MP4 track walker, registry wiring | `#ifndef MINA_NO_AAC` block, `g_mina_aac_codec` |
| **Streaming decode** — `mina_stream_*` | "Streaming decode" section |
| **Playback engine** — `mina_engine_*`, `mina_clip_*`, threads, loader hook, cache | "Playback engine", "Threads", "Engine: clip cache…" sections |
| Small fixes | `mina_br_peek` fast path; `MINA_NO_AAC` in the `MINA_USE_FBUF` guard; a zeroed `codec[]` in `mina_ogg_info` and an explicit `num_window_groups <= MINA_AAC_MAX_GROUP` bound in the AAC path, both to silence LTO false positives (the invariants already held) |

Upstream 2.0.0 **only identifies** AAC — it has no sample decoder, and it is
*smaller* than this vendored file. Byte count is not a safe check for whether a
refresh lost something; diff the public symbol list instead.

---

## What is in it

### Decoders

| Format | Status |
|---|---|
| WAV | PCM 8/16/24/32, float32/64, µ-law, A-law, `WAVE_FORMAT_EXTENSIBLE`, `RIFX` big-endian, any channel count and rate. Encoder too. |
| FLAC | Every subframe type, partitioned-Rice residuals, all decorrelation modes, wasted bits, CRC-8/CRC-16 verification, all metadata blocks |
| MP3 | Full MPEG-1/2/2.5 Layer III, incl. Xing/Info/VBRI and LAME gapless trimming |
| Ogg Vorbis | Full Vorbis I. Floor 0 is implemented but has no test coverage — no released encoder emits it |
| AAC | AAC LC in ADTS and MP4/M4A (**local addition**, see the caveat under *Not verified* below) |
| Ogg container | Pages, packet reassembly, granule tracking; identifies Vorbis, Opus, FLAC-in-Ogg, Speex, Theora, OggPCM |

Plus: 13 PCM conversion formats both endians, a streaming resampler (linear and
32-tap windowed sinc), a mixer with soft clip, a small synth, and a codec
registry for adding decoders without touching the core.

### Output backends

ALSA, OSS, PulseAudio, CoreAudio, WinMM, WAV file, null, and DOS PC speaker /
AdLib OPL2 / MPU-401 MIDI. `mina_device_open(NULL, …)` walks the platform's
backends in order and falls through to the null sink, so a program still runs on
a machine with no sound card. `mina_device_is_real()` says which happened.

### Streaming decode — `mina_stream_*`

Pull-mode decode over an image in memory.

```c
mina_stream *s = mina_stream_open(data, size, /*own=*/1);
mina_u32 got = mina_stream_read(s, out, frames);   /* 0 = end */
mina_stream_rewind(s);
mina_stream_close(s);
```

WAV, MP3 and Ogg Vorbis decode **on demand**, a frame or packet at a time, so a
long track costs its encoded size plus one decoder's working set rather than its
whole decoded size — for a 3-minute stereo Ogg that is a few MB instead of about
60 MB. Everything else (FLAC, AAC, registered custom codecs) is decoded once at
open and served from that buffer; `mina_stream_incremental()` reports which.

Head and tail trims (LAME encoder delay/padding, Ogg granule) are applied
incrementally, so a stream yields **the same samples as `mina_decode`** either
way.

### Playback engine — `mina_engine_*`

A fixed-capacity bank of named voices mixed into one interleaved block.

- **Named voices.** Playing under a name that is already live replaces it.
- **Reference-counted clips** (`mina_clip_*`), decoded, channel-mapped and
  resampled to the engine's format once and shared between voices.
- **Streaming voices** — a `mina_stream` played through the same chain, with its
  own resampler when the rates differ.
- Per-voice **volume, pitch, loop, position**; linear **volume ramps** and
  `mina_engine_stop_fade` (kills the click on a music cut).
- **Distance attenuation** with `min_distance` / `max_distance` / `rolloff`, and
  **constant-power panning** off a real forward×up basis.
- Master gain and soft clip.
- **No per-block allocation.** Everything a voice needs is allocated when it
  starts.

Two ways to drive it:

```c
/* caller-driven: render into your own callback */
mina_engine_render(e, buf, frames);

/* engine-driven: mina_engine_start() spawns a loader thread, plus a mixer
   thread when the engine has a device */
mina_engine_start(e);
```

`mina_engine_start` also installs the engine's own lock, after which every call
is safe from any thread. Without it, install your own via
`mina_engine_set_lock`, or use it single-threaded.

### Asynchronous file loading

```c
mina_engine_set_loader(e, my_loader, user);   /* optional */
mina_engine_play_file(e, "BgMusic", path, &params, volume, pitch);
```

Reserves the voice and returns immediately; the read and decode happen on the
loader thread (or inline when no threads are running). The reservation carries a
generation stamp, so "play, then immediately stop or replace" resolves correctly
without the loader having to be cancellable. `mina_engine_playing()` reports 1
while a load is pending.

- **Loader hook** — a `path → buffer` function. This repo uses it to decrypt
  `.mus` files (see `loadSoundFile` in `AudioEngine.cpp`). The default reads the
  file with `<stdio.h>`.
- **Clip cache** keyed by path, 64 MB by default, LRU, and it only ever evicts
  clips no voice is holding.
- **Load deadline** — a queued short effect that has waited longer than 250 ms is
  dropped rather than played late as an echo. Looping and streaming requests are
  never dropped.

### Threads

Win32 from Windows 95 on (`CRITICAL_SECTION`, `CreateThread`, an auto-reset
event — deliberately *not* `CONDITION_VARIABLE` or `TryEnterCriticalSection`,
neither of which exists on 9x), POSIX threads everywhere else, and nothing at all
on DOS, where `mina_have_threads()` returns 0 and the engine stays caller-driven.
Every wait re-tests its predicate and times out, so a lost wake-up costs one poll
instead of a hang.

---

## Build-time switches

| Define | Effect |
|---|---|
| `MINA_NO_STDIO` | no `<stdio.h>` anywhere (drops the wavfile sink and the default loader) |
| `MINA_NO_DEVICES` | drop the output layer |
| `MINA_NO_THREADS` | drop the threading layer; the engine stays caller-driven |
| `MINA_NO_VORBIS` / `MINA_NO_MP3` / `MINA_NO_FLAC` / `MINA_NO_AAC` | drop a decoder |
| `MINA_NO_STDINT` | derive fixed-width types from `<limits.h>` |
| `MINA_FORCE_NO_64` | use the portable 32/32 arithmetic instead of a native 64-bit type |
| `MINA_MALLOC` / `MINA_REALLOC` / `MINA_FREE` | your allocator |
| `MINA_CACHE_LIMIT` | default engine clip-cache size |
| `MINA_LINUX` / `MINA_WINDOWS` / `MINA_MACOS` / `MINA_DOS` / `MINA_UNIX` | override platform detection |

---

## Confirmed to work

Everything in this section was **run in this repo** with the project's own
toolchain (`toolchain/mingw64`, gcc 15.2.0, Windows 11 x86-64) against the
164-file corpus in `Custom Programs\mina2.0.0\mina\testfiles`.

### Streaming parity — 384 / 384

Every corpus file read back through `mina_stream_*` and compared sample-for-
sample against `mina_decode` of the same image, at two chunk sizes (997 frames,
and one frame at a time). **`maxerr` 0.0 and identical frame counts on all 166
files, both chunkings.** Same 384/384 under `-DMINA_FORCE_NO_64` and under
`-DMINA_NO_STDINT -DMINA_FORCE_NO_64`, which exercises the emulated 64-bit
arithmetic the head/tail trim logic runs on.

The incremental/buffered split over that corpus is **118 incremental, 48
buffered, 0 unsupported** — i.e. every WAV, MP3 and Ogg Vorbis file takes the
on-demand path, and the FLAC and AAC files take the decode-once path, as designed.

### Engine — included in the 384 above

Silence with no voices; clip decode and frame count; play, replace-by-name,
voice count; non-looping voices retiring themselves; looping voices surviving the
end; volume/pitch setters addressing only live voices; hard-left and hard-right
panning; silence beyond `max_distance`; fade-out retiring a voice; streaming
voices through a rate conversion; the voice bank saturating rather than
overflowing; and rejection of empty names, null clips, garbage images and null
streams. Plus a pitch sweep (100× → 0.01×) that a stream voice must survive
without walking off its buffer.

### Async, threads, cache — 31 / 31 threaded, 25 / 25 without threads

Inline loading; cache hit on a second play of the same path; a missing file
leaving no voice; the loader hook being called and its output actually decoded;
streaming by path; streams not filling the cache; a looping stream still running
after 80 blocks; `stop` cancelling a pending load and the cancelled load not
resurrecting it; threads starting, stopping and restarting cleanly. And a
concurrency hammer: 200 rounds of `set_volume` / `set_pitch` / `set_position` /
`playing` / `play_file` / `stop` from the calling thread while the loader thread
runs and the mixer renders.

Separately: a looping stream voice kept alive for 4.6 s of a 0.14 s clip, with
balanced lock/unlock counts (403/403) and pitch changed to 2.5× and 0.1× mid-play.

### Compile and link matrix — all clean

`-std=c89 -pedantic-errors -Wall -Wextra -Wshadow -Wcast-qual -Wwrite-strings
-Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes`, zero warnings, at
`-O0` / `-Os` / `-O2`; also `-std=c99` and `-std=c11` with `-pedantic-errors`,
`g++ -std=c++98 -pedantic-errors` and `g++ -std=c++20 -O2`.

**Linked**, not merely compiled, in every one of these configurations: default,
`MINA_NO_STDINT`, `MINA_FORCE_NO_64`, both together, `MINA_NO_STDIO`,
`MINA_NO_DEVICES`, both together, `MINA_NO_THREADS`, `MINA_NO_THREADS` with
devices and stdio off, all four decoders off, and each decoder off individually.
`MINA_DOS` compiles clean. The header survives being included twice in one file
and from two translation units where only one defines `MINA_IMPLEMENTATION`.

Linking every configuration is worth doing on its own: a compile-only sweep
missed a real `MINA_NO_THREADS` link failure that this caught.

### The game

`build-omega.ps1 -BuildType Release` builds `minecraft_native.exe` and
`minecraft_server.exe` clean — **zero warnings, zero errors** — with the whole
mina implementation compiled into `AudioEngine.cpp` under the project's LTO.

---

## Not verified here

Be honest about the edges.

- **AAC decoder correctness.** The AAC files decode and stream without error, but
  the streaming-parity test only proves stream output equals `mina_decode`
  output, and AAC takes the buffered path where that is trivially true. Nothing
  here checks AAC samples against a reference decoder.
- **Upstream's bit-exactness numbers** (WAV/FLAC `maxerr` 0.0, MP3 ≤ 1.1e-5,
  Vorbis ≤ 3.0e-7 vs ffmpeg) are upstream's measurements, not re-run here. The
  384/384 above is stream-vs-`mina_decode`, a different claim.
- **Linux, macOS and DOS backends.** They compile under `MINA_UNIX` / `MINA_MACOS`
  / `MINA_DOS` on their real toolchains upstream, but the only backend actually
  *run* in this repo is WinMM. `MINA_UNIX` / `MINA_LINUX` / `MINA_MACOS` fail to
  cross-compile from this Windows host (mingw's `dlfcn.h` shim, no macOS SDK) —
  pristine upstream fails identically, so that is the environment, not the code.
- **POSIX threading path.** Written against pthreads and reviewed, but no POSIX
  host was available to compile or run it.
- **Big-endian.** Upstream passes its corpus on s390x under qemu; not re-checked
  after the local additions. The engine and streaming code is endian-neutral (it
  works in `float` and defers to the existing byte-wise conversion paths), but
  that is reasoning, not a measurement.
- **Long-run stability.** No soak test, and no ASan/UBSan run — the mingw
  toolchain in `toolchain/` has no sanitizer runtime.

---

## Behaviour change worth knowing

The deleted `MinaBackend` panned **backwards**. Its pan was
`(dx·−lookZ + dz·lookX)/d` applied as louder-right for positive pan; with the
listener basis the game actually passes (forward `(−lookX, 0, −lookZ)`, up
`(0,1,0)`), the right vector is `forward × up`, which puts the sign the other
way. `mina_engine` derives the right vector from the basis, so left and right
are now correct — and swapped relative to the old build.

It also used `(1−pan)` / `(1+pan)` gains, which doubled a centred source's
loudness; the engine uses constant power (`cos`/`sin`), so a centred source now
sits at 0.707 per side instead of 1.0.

---

## Layout in this repo

```
third_party/mina.h                                  the library
third_party/MINA.md                                 this file
src/net/minecraft/client/platform/audio/
    AudioEngine.hpp                                 unchanged game-facing API
    AudioEngine.cpp                                 MINA_IMPLEMENTATION lives here;
                                                    registries, .mus loader hook,
                                                    direct mina_engine_* calls
tests/mina_audio_test.cpp                           gtest: decode + resample
```

`src/net/minecraft/client/platform/audio/backend/` and `.../decode/` are gone.
Sources are globbed by CMake, so deleting them was enough; `minecraft_link_audio`
in `CMakeLists.txt` still supplies `winmm` / CoreAudio / `dl` per platform. A
POSIX build will also need pthreads linked there.

## License

MIT, at the bottom of `mina.h`.
