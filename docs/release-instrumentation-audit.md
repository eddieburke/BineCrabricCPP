# Release instrumentation audit

Runtime profiling is disabled in Release and RelWithDebInfo by default. Debug enables it. Explicitly enable it in an optimized build with `-DMINECRAFT_ENABLE_PROFILING=ON` when configuring CMake; debug symbols alone do not enable profiling.

## Removed from ordinary Release execution

- GPU stage debug markers, including shadow-stage label formatting.
- Hang watchdog thread, heartbeat atomics, and automatic hang dumps.
- Per-packet diagnostic ID lookup, statistics-vector append, and main-thread statistics merge under the receive mutex. Packet delivery and receive backpressure still use their existing synchronization.
- Logging enqueue/write clocks and cumulative diagnostic counters. Log records still reach their handlers.
- Snapshot fallback diagnostic atomic increments.
- Audio telemetry counters, decode timing, and the additional telemetry-only loader lock. Audio request deadlines, cache eviction, and playback state still operate.
- Unused mesh capture/build timers and stale/deferred mesh counters.
- The scene-target diagnostic string stream formerly constructed on every target check, before duplicate-log suppression.

Tick duration, profiler frame times, and FPS text now collect only while the F3 HUD is visible. Repeated shader log messages use a string-view lookup before allocating their stored message.

Unbounded lighting and chunk-integration passes no longer read a start timestamp that only a configured time budget consumes. Bounded passes retain their deadlines.

The NVIDIA available-VRAM query now runs only when creating or resizing an SSBO. Previously every pack-resource check queried it, including packs with no SSBOs; those checks can occur several times per frame.

Diagnostic statistics APIs return zero counters when profiling is disabled. Packet-accounting and logging tests cover delivery with either configuration. Standalone MINA retains telemetry by default; the game selects it through the profiling build setting. The MINA switch preserves C90 syntax.

## Validation handoff

Read-only inspection of fresh Release object files with `nm -C` found no logging steady-clock references, packet-statistics symbols, watchdog state, snapshot diagnostic counter, or GPU marker references. `strings` found no scene-target diagnostic format string in the Release pipeline object. This establishes removal of those compiled instrumentation paths, not a gameplay speedup.

Another build was already compiling `build-omega` during this edit, so no competing build was started. Its output must not be assumed to contain all edits made during that build. After that build completes, run a final incremental build:

```powershell
.\toolchain\mingw64\bin\cmake.exe --build build-omega --target minecraft_native minecraft_omega_tests -j 4
.\build-omega\minecraft_omega_tests.exe --gtest_filter="LoggingDispatcherTest.*:PacketAccounting.*:MinaAudioTest.*:ColorTargetsTest.*:ShaderGlIntegrationTest.*:TerrainCapturePerf.*"
```

Also exercise a separate profiling-enabled configuration before relying on its counters. Gameplay FPS and driver synchronization cost have not been measured for these changes.

Framebuffer creation/completeness checks in color-target clears and flip resets remain a separate rendering optimization candidate. They validate real framebuffer attachments and should be cached with resource lifetime tracking, rather than disabled as profiling.
