#pragma once

namespace seedfinder::runtime {

// Initializes block/item/entity registries required by seedfinder_probe.
// Must run on the main thread before any SearchEngine worker threads start.
void initialize();

// Process teardown hook; safe to call multiple times.
void shutdown();

} // namespace seedfinder::runtime

#ifdef __cplusplus
extern "C" {
#endif

void seedfinder_init(void);
void seedfinder_shutdown(void);

#ifdef __cplusplus
}
#endif
