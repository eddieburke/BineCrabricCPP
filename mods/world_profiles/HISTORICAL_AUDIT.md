# Historical generator audit — 2026-11-17

The reference corpus contains **122 provider labels, 120 unique JAR SHA-256 digests, 237,599,688 client bytes and 14,387 CFR Java outputs**. Downloads and decompilation are complete for the recorded scope. This is evidence for specific generator work, not a claim that every version has been ported or behaviorally validated.

The complete local index is [references/minecraft/README.md](../../references/minecraft/README.md); [inventory.json](../../references/minecraft/inventory.json) records every URL, full digest, embedded version string, selected class and decompilation result.

## Sources and identity

- 73 clients are verified against SHA-1 and byte counts in Mojang version metadata reached through the [official manifest](https://piston-meta.mojang.com/mc/game/version_manifest_v2.json). This covers all 35 launcher old_alpha labels, all 26 old_beta labels and 12 release milestones: 1.0, 1.1, 1.2.5, 1.6.4, 1.7.10, 1.8.9, 1.12.2, 1.16.5, 1.17.1, 1.18.2, 1.20.1 and 1.21.1.
- 49 additional clients come from the primary archive indexes for [Indev](https://www.omniarchive.net/archive/java/client/indev/) and [Infdev](https://www.omniarchive.net/archive/java/client/infdev/). The archived June 18 Infdev binary also matches the official launcher copy; it is counted once.
- The February 27 1433 and April 15 Infdev JARs independently match [BetaCraft archive](https://files.betacraft.uk/launcher/assets/versions/) copies byte for byte. [Comparison digests](../../references/minecraft/archive-crosschecks.json) are retained. Their SHA-256 values are `507cecfad21caa2e137a00c57d63ac6ebc299cb1191e703f1c49274d53e7fef3` and `bef3eadbe765604f0b578bfc757adc5c73fb4d449ac1a02355c9f7bd1af97e47` respectively.
- A provider label does not prove a distinct binary or a release date. `a1.2.1` and `a1.2.1_01` have the same JAR digest; so do `rd-161348` and `rd-20090515`. Infdev's embedded display text does not distinguish its dated archive labels. No unavailable February 27 1414 binary is claimed.

[CFR 0.152](https://www.benf.org/other/cfr/) matches its author's published MD5. [Vineflower 1.12.0](https://github.com/Vineflower/vineflower/releases/tag/1.12.0) matches its release asset SHA-256. Six selected versions were cross-decompiled. CFR's April density output contained invalid local-variable reconstruction despite successful process completion; Vineflower, `javap` and execution of original bytecode were used for the actual behavioral checks. Generated source counts alone are not validation.

## Profile findings

| Profile | Original evidence | Result and boundary |
| --- | --- | --- |
| Default | Beta 1.7.3 `yf` overworld provider, `qn` Nether provider, `jd` Skylands provider and `xv` climate source | The host's existing Beta generator remains the base. No new claim of complete Beta chunk/population parity is made by this audit. |
| Flatlands | Java 1.1 `wo.a(byte[])`, selected by `aip.b()` for flat world type | Corrected the default grass coordinate from 4 to 3: bedrock, two dirt layers, one grass layer. The default raw layer array is compared to original bytecode. Custom heights remain supported; villages/population are omitted. |
| Highlands | This mod's surface-lifting function | Custom behavior, now labeled Custom. No historical Minecraft generator counterpart is claimed. |
| Caves | This mod's surface-lowering and pocket-carving function | Custom behavior, now labeled Custom. It is not an Indev map-type or modern cave-world reconstruction. |
| Alpha 1.1.2_01 Terrain | Official `a1.1.2_01` JAR, `nw` provider, `lp` octave sampler and `v` Perlin sampler | Corrected density and surface generation. Independent original-bytecode golden samples cover four seeds and four chunks each. Snow-covered worlds, caves, feature population and full-world behavior remain outside this check. |
| Infdev 20100227-1433 (Seeded) | Archived `net.minecraft.a.a.f`, octave sampler `net.minecraft.a.a.c.a.c` and math helper `a.a` | Original construction uses separate unseeded Random instances for the six noise groups and Math.random for flowers. The profile's saved-seed initialization and flower determinism are explicit adaptations. Under the same injected noise state, all compared non-flower blocks match the original generator method. |
| Infdev 20100415 | Archived `net.minecraft.a.a.c.a`, its noise samplers and population helpers | Terrain/surface output matches the original generator in the recorded matrix. Ore/tree population is a separately adapted chunk-local reconstruction and is not validated by those terrain results. |
| Modern Inspired (128 High) | Official mappings and generation classes from 1.18.2, 1.20.1 and 1.21.1 | The host generator is custom. Original 1.18.2 `NoiseSettings` (`cug`) uses an overworld minimum Y of -64 and height 384; modern JAR data confirms the same extent. A fixed 128-block generator is not represented as that original algorithm. |

## Concrete corrections

Alpha had retained Beta's `1.121/200` low-frequency sampling, negative-depth multiplier `0.3`, depth bias `-2`, positive divisor `8`, horizontal shortcut noise, biome-driven surfaces, five-value bedrock selection and sandstone continuation. Its original provider instead uses `1/100` scales, full 3D sampling even for single-height grids, absolute negative depth, bias `-3`, positive divisor `6`, its original surface traversal/indexing, six-value bedrock selection minus one, and no sandstone continuation. These choices now have an explicit Alpha path; Beta arithmetic is retained for the host's Beta style.

February Infdev now preserves Java single-precision intermediates, the actual float detail-scale constant, integer division in the `/5` perturbation, and Java's saturating double-to-int floor behavior. April Infdev now interpolates Y, then X, then Z in the original operation order and uses the same Java floor behavior. This includes the native integer-overflow behavior at the negative floor boundary.

Generation changed, so saved profile metadata now requires schema 3 and the package is version 3.0.0. Earlier profile metadata is rejected; no compatibility conversion silently mixes old and corrected generated chunks.

## Behavioral evidence

[Original probe metadata](../../references/minecraft/original-probes/results.json) records JAR digests, commands, coordinates and raw chunk digests. [Lua comparison results](../../references/minecraft/original-probes/lua-comparison.json) compare 41 chunks / 1,343,488 block positions with zero mismatches:

- 1 Java 1.1 flat layer array.
- 20 April Infdev terrain/surface chunks.
- 20 February Infdev chunks with controlled seeded noise state and flower block 37 normalized to air on both sides.

Infdev cases use seeds 0, 42, -7 and `0x123456789abcdef`, and chunks `(0,0)`, `(1,-1)`, `(-7,13)`, `(40,-22)` and `(1048577,-1048577)`. Thus negative coordinates, a seed above the exact floating-point integer range, and large coordinate behavior are included.

The Alpha golden corpus contains 16 original terrain/surface chunks (524,288 block positions), using the same seeds and the first four chunk coordinates. The C++ regression matches the corrected native generator against all 16 independently captured hashes. Combined with the Lua comparisons, this is 57 chunks / 1,867,776 block positions within the stated boundaries. The Release client and tests built successfully; 38 focused native tests passed with no failures. Results are recorded in [the test XML](../../build-omega/world-profiles-tests.xml).

The original April generator is unchanged in the probe. Only its final chunk lighting method, `net.minecraft.a.a.e.d.a()V`, is replaced by `return` to avoid bootstrapping lighting and world-cache side effects after the raw blocks are generated. The February generator method is unchanged; reflection injects the documented deterministic noise fields. Alpha executes its unmodified terrain and surface methods against a world allocated without game startup, with the snow-covered flag false. The flat probe invokes its original layer method with structures disabled.

These probes do not establish render parity, gameplay parity, population ordering, ore/tree parity, spawn distribution, or agreement outside the recorded cases. No live game session was used as evidence.

## Reproduce

Run from the repository root:

```text
python tools/minecraft_reference.py --phase all
python tools/minecraft_original_probe.py
python tools/compare_world_profiles.py
python tools/check-lua-mods.py
build-omega/minecraft_omega_tests.exe --gtest_filter=TerrainStyle.*:LuaGenerationHooks.*:LuaModContracts.*
```

The reference tool pins exact IDs and validates downloads; failed versions are recorded and cause a nonzero result. It does not substitute another version when a download fails. Java 23, CFR and Vineflower provenance are retained locally. The downloaded binaries and decompiled game sources remain in the ignored local reference directory.
