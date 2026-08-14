# Far terrain lighting seam

Mode: debug

Success predicate: `minecraft_tests.exe --gtest_filter=LightingChannel.*:LightingMeshGate.*:ChunkMeshGolden.*` exits 0, then full `minecraft_tests.exe` exits 0.

## Trace

- Far lighting dirty regions were force-published after 250 ms while propagation remained busy.
- Each publication dirtied and rebuilt terrain meshes, exposing successive partial light states as whole-face luminance flicker.
- Iris delegates chunk meshing to Sodium. Sodium captures neighboring section light arrays in `ChunkRenderContext`, copies them into `LevelSlice`, then meshes from that snapshot. No distance-based deadline publishes an unfinished lighting wave.

## Candidate 1

Remove the far-publication deadline. Near-camera regions retain immediate publication; far regions publish only after the lighting queue settles.

## Result

- Candidate kept.
- Client, server, and lighting engine compiled; the wrapper later failed deleting a locked `minecraft_native.exe.build-omega-backup` after successful links.
- `LightingChannel.*:LightingMeshGate.*:ChunkMeshGolden.*`: 14/14 passed.
- `*Lighting*`: 14/14 passed, including lighting performance and invalidation-cascade coverage.
- Full suite reached one unrelated failure: `PerfTraceTargeted.ChunkArenaGrowDirectMeasurement`, where all GL uploads failed and the test measured 0/5. A later full-suite retry terminated with Windows access violation during unrelated integration coverage.
