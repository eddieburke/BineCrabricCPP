# Judaism mod acceptance suite

The Judaism feature set is split into independently loadable Lua modules. Every module returns a table with id, entries, count, find, first, each, and summary.

## Static checks

1. Confirm mod.json points to scripts/main.lua.
2. Confirm the main script lists every optional module.
3. Confirm each module has a unique module id.
4. Confirm content identifiers are positive and do not collide with vanilla ids.
5. Confirm holiday records contain month, day, reward, and fasting fields.
6. Confirm synagogue records contain room, block, coordinates, and orientation.
7. Confirm progression records contain mitzvah category, points, and tier.
8. Confirm community records contain a service, capacity, cost, and trust score.
9. Confirm compatibility records mark optional capabilities safe.
10. Confirm every Lua file ends with a returned module table.

## Runtime checks

Load the mod in a title screen and in a world. The core Shabbat screen must open even if an optional API is absent. Optional module loading is guarded by pcall; warnings are acceptable and should identify the module name. Check that data modules can be queried without a player or world.

## Data checks

The catalog intentionally contains multiple families of ritual, holiday, community, world, synagogue, and content entries. Consumers should filter through find or first instead of depending on entry order. A future content implementation can register only records whose texture assets are installed.

## Regression checks

Toggle Shabbat settings, close and reopen the settings page, enter and leave a world, and reload the mod. No module should mutate the core settings table. A missing module must not prevent the base Shabbat rules from loading.

