# Walking chunk scheduling, 2026-10-27

The walking hot paths rebuilt and sorted the entire distant tile grid every frame, rescanned every loaded chunk on camera-section changes and LOD coverage updates, and scanned the complete dirty-section set whenever mesh submission had room. Backlog-dependent minimum uploads/captures could bypass deadlines, and an expired shared deadline opened a fresh time slice.

## Changes

- Cache distant grid membership and stride selection within each 16-block camera cell. A conservative half-cell diagonal margin keeps all visible tiles resident at every corner of the cell. Settings, native-radius transitions and memory pressure invalidate the cache.
- Select only available distant submission slots with a partial sort; skip candidate selection while all slots are occupied. Preserve coverage-first and memory-recovery priorities. Budget tile uploads and source pinning by elapsed time.
- Enumerate the entering rows of native and transition discs when moving. Full reconciliation queries only the local disc, never all loaded world chunks. Track coverage loss separately so successful coverage uploads do not trigger admission rescans.
- Retire out-of-range native columns in a timed slice; recheck range and LOD coverage before freeing GPU buffers so rapid direction changes cannot retire terrain that reentered the view.
- Maintain pending columns as a nearest-first heap. Rebuild it in linear time when the camera section changes, then insert/pop in logarithmic time.
- Rotate through at most 512 dirty sections per capture pass. Separate membership tracking from a stable round-robin list; remove entries immediately on section release.
- Honor expired shared deadlines, allow one operation for forward progress, cap capture attempts by elapsed time and make the existing chunk update slider an actual per-phase item ceiling. Near and background uploads share the item allowance. Large backlogs no longer force 4–16 uploads before checking time.
- Add Performance Settings > Background Threads, persisted as `backgroundThreads` (0 = Auto). This limits active workers in the shared compute pool used by mesh building, lighting and loading. Park excess workers without rebuilding the pool or dropping queued jobs. Running jobs finish when lowering the limit; new jobs obey the limit. IO, networking, audio and GL compilation have separate threads.
- Rename Chunk Updates to Chunk Work Limit and Dynamic Updates to Adaptive Chunk Work. Preserve their stored keys and existing values. Adaptive mode may raise the item ceiling with backlog but never bypasses the elapsed-time budget.

For a weak dual-core machine, start with Background Threads = 1, Chunk Work Limit = 1–2, Adaptive Chunk Work = Off. This trades terrain arrival speed for rendering time. Auto already selects one shared compute worker on a four-logical-thread CPU under the existing reserved-thread policy.

## Validation

Release build completed with portable CPU settings (`-NoNativeCpu`), including compilation and linking of the test executable. Regression tests and the synthetic benchmark were not run; the user requested proceeding without tests. Tests cover circle differences across negative coordinates, diagonal movement, teleports and radius changes; expired budgets; live worker limits and queue preservation; bounded/fair dirty scans and section retirement; heap reprioritization; settings persistence; and conservative cached LOD coverage. A synthetic 900-frame walk reports grid rebuild counts and CPU time at a 1280-block view distance.

No live T520 frame-time capture or gameplay/visual comparison has been performed. Synthetic grid measurements isolate scheduling work and do not establish total FPS or eliminate unrelated generation, lighting, storage, driver or draw-call stalls.
