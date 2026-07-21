# Phase 2: Configuration & Settings Consolidation Plan

## Problem Analysis

### Current Issues:
1. **Inconsistent Config Patterns**:
   - `item_drop_physics`: Uses mod_storage + JSON file fallback
   - `realtime_sky`: Has SETTINGS_KEYS, SETTINGS_NAMES, SETTINGS_ALIASES tables
   - `sprint/meteors`: Use `minecraft.settings.register()` with slider definitions
   - `layered_clouds/camera`: Use `minecraft.config.load/save()` with aliases
   - No standardization across mods

2. **C++ Binding Dependencies**:
   - `LuaModSettingsBindings.cpp` handles settings registration
   - `ModSettingsRegistry.cpp` manages persistence in `mod_settings.txt`
   - Keybinds handled separately from settings
   - Complex alias system (internal_name ↔ file_name mapping)

3. **Code Duplication**:
   - Every mod implements clamp logic
   - Every mod implements load/save wrappers
   - Validation logic duplicated everywhere
   - Default value handling inconsistent

4. **Weird/Problematic Patterns**:
   - `judaism` mod removed ✓
   - Box3D C++ bindings removed ✓
   - But settings system still convoluted with:
     - Dual storage (registry + file)
     - Complex alias tables
     - Mixed keybind/settings handling
     - No type safety
     - String-based lookups everywhere

## Solution Architecture

### Part A: Unified Lua Configuration System

Create `/workspace/mods/lib/settings/init.lua`:
- Single API for all mods
- Automatic clamping/validation
- Built-in persistence
- Type-safe definitions
- No C++ dependencies for basic operations

### Part B: Simplify C++ Layer

Refactor C++ to:
- Remove complex alias logic
- Remove dual-storage pattern
- Make it a simple key-value store
- Let Lua handle validation/clamping
- Remove settings registry complexity

### Part C: Standardize All Mods

Each mod gets:
```lua
local settings = require("lib.settings")

local cfg = settings.define("mod_id", {
  name = "Display Name",
  fields = {
    my_slider = { type = "slider", min = 0, max = 100, default = 50, step = 1 },
    my_toggle = { type = "bool", default = true },
    my_number = { type = "number", min = 0, max = 1, default = 0.5 },
  }
})

-- Usage:
local value = cfg.my_slider  -- Auto-loaded
cfg.my_slider = 75  -- Auto-saved
```

### Part D: Remove Weirdness

- Eliminate alias tables (use consistent naming)
- Remove dual storage (single source of truth)
- Remove string-based field access (use direct property access)
- Remove manual clamp logic (handled by definition)
- Remove separate keybind handling (unified with settings)

## Implementation Steps

### Step 1: Create lib/settings Module
- [ ] Create `/workspace/mods/lib/settings/init.lua`
- [ ] Implement field definition DSL
- [ ] Implement automatic persistence
- [ ] Implement validation/clamping
- [ ] Implement type conversion

### Step 2: Simplify C++ Settings Backend
- [ ] Refactor `ModSettingsRegistry.hpp/cpp`
- [ ] Remove alias system
- [ ] Remove complex parsing
- [ ] Make it simple key-value store
- [ ] Update `LuaModSettingsBindings.cpp`

### Step 3: Migrate All Mods

#### 3a: item_drop_physics
- [ ] Replace custom JSON loading with lib.settings
- [ ] Remove config/init.lua physics database loading
- [ ] Keep physics definitions as data-only

#### 3b: realtime_sky
- [ ] Remove SETTINGS_KEYS, SETTINGS_NAMES, SETTINGS_ALIASES
- [ ] Use simple field definitions
- [ ] Remove manual save/load logic

#### 3c: sprint
- [ ] Remove manual settings registration
- [ ] Use lib.settings define() pattern
- [ ] Remove get() wrapper function

#### 3d: meteors
- [ ] Same as sprint
- [ ] Handle keybinds through settings

#### 3e: layered_clouds
- [ ] Remove aliases table
- [ ] Remove manual load_config/save_config
- [ ] Use lib.settings

#### 3f: camera
- [ ] Remove manual load/save
- [ ] Remove clamp logic duplication
- [ ] Use lib.settings

### Step 4: Cleanup
- [ ] Remove unused C++ functions
- [ ] Remove deprecated APIs
- [ ] Update documentation
- [ ] Verify no mod uses old patterns

## Success Metrics

1. **Line Reduction**: 
   - Config files reduced by 60%+
   - C++ settings code reduced by 50%+

2. **Consistency**:
   - All mods use identical pattern
   - No duplicated clamp/validation logic
   - No alias tables anywhere

3. **Simplicity**:
   - Settings defined once, used everywhere
   - No manual save/load calls
   - Direct property access (no get/set functions)

4. **Performance**:
   - Fewer string lookups
   - No redundant file I/O
   - Cached values with dirty tracking

## Files to Modify

### Create:
- `/workspace/mods/lib/settings/init.lua`
- `/workspace/mods/lib/settings/types.lua` (optional helper)

### Modify (Lua):
- `/workspace/mods/item_drop_physics/config/init.lua`
- `/workspace/mods/realtime_sky/config/init.lua`
- `/workspace/mods/sprint/config/init.lua`
- `/workspace/mods/meteors/config/init.lua`
- `/workspace/mods/layered_clouds/config/init.lua`
- `/workspace/mods/camera/config/init.lua`
- All main.lua files to use new pattern

### Modify (C++):
- `/workspace/src/net/minecraft/mod/ModSettingsRegistry.hpp`
- `/workspace/src/net/minecraft/mod/ModSettingsRegistry.cpp`
- `/workspace/src/net/minecraft/mod/runtime/LuaModSettingsBindings.cpp`
- `/workspace/src/net/minecraft/mod/runtime/LuaModSettingsBindings.hpp`

### Delete:
- Any deprecated helper files

## Timeline

Phase 2A: Core Settings Library (this session)
Phase 2B: C++ Simplification (this session)
Phase 2C: Mod Migration (this session)
Phase 2D: Testing & Cleanup (this session)
