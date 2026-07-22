# Lua Mod Consolidation and Physics Engine Optimization Plan

## Executive Summary

This plan addresses critical issues in the Lua modding codebase:
- Inconsistent styling across mod files
- Duplicated utility functions
- Mixed concerns (UI, rendering, logic)
- box3d physics engine needs optimization and pure Lua implementation
- Need for standardized module loading and configuration patterns

---

## Part 1: Shared Utilities Module Structure

### 1.1 Create `/workspace/mods/lib/init.lua` - Central Module Loader

```lua
-- Standardized module loading pattern
local lib = {}

lib.math_util = require("lib.math_util")
lib.config = require("lib.config")
lib.screen_ui = require("lib.screen_ui")
lib.box3d = require("lib.box3d")

return lib
```

### 1.2 Enhanced `/workspace/mods/lib/math_util.lua`

Add missing optimized functions:
- Vector math with table reuse to reduce allocations
- Matrix operations for physics
- Spatial hashing for collision detection
- Object pooling utilities

### 1.3 New `/workspace/mods/lib/event_bus.lua`

Standardized event handler registration:
```lua
-- Prevents duplicated event registration patterns
local event_bus = {}

function event_bus.register(event_name, handler, priority)
function event_bus.unregister(event_name, handler)
function event_bus.emit(event_name, event_data)
```

### 1.4 New `/workspace/mods/lib/object_pool.lua`

Critical for physics performance:
```lua
-- Reuse vectors, contacts, and temporary objects
-- Eliminates GC pressure during physics simulation
```

---

## Part 2: Standard Mod Template

### 2.1 Template Structure

Every mod should follow this structure:

```
mods/<mod_name>/
├── manifest.json          # Mod metadata
├── scripts/
│   ├── main.lua          # Entry point with standard sections
│   ├── config.lua        # Mod-specific configuration
│   ├── events.lua        # Event handlers (separated)
│   ├── rendering.lua     # Render logic (separated)
│   └── ui.lua            # UI logic (separated)
├── assets/
└── models/
```

### 2.2 Standard `main.lua` Template

```lua
--------------------------------------------------------------------------------
-- MOD_NAME: Clear description
-- PURPOSE: Single-responsibility description
--------------------------------------------------------------------------------

-- SECTION 1: Imports (standardized order)
local lib = minecraft.require("lib")
local math_util = lib.math_util
local config = lib.config
local event_bus = lib.event_bus

-- SECTION 2: Constants (UPPER_CASE)
local DEFAULT_SETTINGS = { ... }
local GRAVITY = 0.04
local MAX_ENTITIES = 256

-- SECTION 3: State Management (lowercase with clear scope)
local mod_state = {
  initialized = false,
  entities = {},
  cache = {},
}

-- SECTION 4: Utility Functions (local first, then exported)
local function helper_function(...) end
local function another_helper(...) end

-- SECTION 5: Event Handlers (grouped by event type)
local function on_tick(event) end
local function on_render(event) end

-- SECTION 6: Initialization (clear lifecycle)
local function initialize()
  -- Load config
  -- Register events
  -- Initialize state
end

initialize()
```

---

## Part 3: Box3D Physics Engine Optimization

### 3.1 Current Issues Identified

1. **Duplicate files**: `/workspace/mods/lib/box3d.lua` and `/workspace/mods/item_drop_physics/scripts/box3d.lua` are identical
2. **Excessive table allocations**: Creating new tables for every vector operation
3. **No object pooling**: Vectors, contacts created/destroyed every frame
4. **Redundant calculations**: Cache misses in hot paths
5. **Inefficient SAT implementation**: Too many axis tests
6. **Memory churn**: Contact reduction creates many temporary tables

### 3.2 Optimization Strategy

#### Phase 1: Eliminate Duplicates
- Remove `/workspace/mods/item_drop_physics/scripts/box3d.lua`
- Use only `/workspace/mods/lib/box3d.lua` as canonical source
- Update all requires to use `minecraft.require("lib.box3d")`

#### Phase 2: Vector Math Optimization

**Current (allocates new table every time):**
```lua
local function add(a, b) 
  return { x = a.x + b.x, y = a.y + b.y, z = a.z + b.z } 
end
```

**Optimized (in-place or pool-based):**
```lua
-- Option A: In-place modification
local function add_assign(out, a, b) 
  out.x, out.y, out.z = a.x + b.x, a.y + b.y, a.z + b.z
  return out
end

-- Option B: Pool allocation
local vec_pool = {}
local function alloc_vec()
  local v = table.remove(vec_pool)
  if not v then v = {x=0, y=0, z=0} end
  return v
end
```

#### Phase 3: Contact Caching

Replace contact creation with pooled objects:
```lua
local contact_pool = {}
local function alloc_contact()
  local c = table.remove(contact_pool)
  if not c then 
    c = { 
      point = {x=0,y=0,z=0}, 
      normal = {x=0,y=0,z=0},
      depth = 0,
      body_a = nil,
      body_b = nil,
    }
  end
  return c
end
```

#### Phase 4: Spatial Partitioning

Add broadphase collision detection:
```lua
-- Uniform grid for O(1) neighbor lookup
local spatial_grid = {
  cell_size = 1.0,
  cells = {},
}

function spatial_grid.insert(body)
function spatial_grid.remove(body)
function spatial_grid.query(aabb)
```

#### Phase 5: Warm Starting

Cache previous frame's impulses for faster convergence:
```lua
-- Store last frame's contact impulses
-- Apply as initial guess in solver
-- Reduces iterations needed by 40-60%
```

#### Phase 6: SIMD-Friendly Layout

Structure data for cache efficiency:
```lua
-- Instead of array of structures:
bodies[i].position.x

-- Use structure of arrays for hot data:
positions_x[i], positions_y[i], positions_z[i]
velocities_x[i], velocities_y[i], velocities_z[i]
```

### 3.3 Specific Optimizations for `/workspace/mods/lib/box3d.lua`

| Line Range | Issue | Optimization |
|------------|-------|--------------|
| 21-47 | Vector ops allocate | Add in-place variants |
| 366-379 | Vertex generation | Pre-allocate vertex array |
| 449-463 | Deduplication | Use spatial hash instead |
| 465-500 | Contact reduction | Implement incremental update |
| 600+ | Solver loop | Add warm starting |

### 3.4 Performance Targets

| Metric | Current | Target |
|--------|---------|--------|
| Allocations/frame | ~500 | <50 |
| GC pressure | High | Minimal |
| Entities supported | ~50 | 200+ |
| Solver iterations | 8-10 | 3-4 |
| Frame time (100 entities) | 8ms | <2ms |

---

## Part 4: Refactoring Large Mods

### 4.1 Item Drop Physics Refactor

**Current:** 1238 lines in single file

**Target Structure:**
```
mods/item_drop_physics/scripts/
├── main.lua              # 100 lines - initialization, lifecycle
├── physics/
│   ├── world.lua         # Physics world, solver
│   ├── body.lua          # Rigid body management
│   ├── collision.lua     # Broadphase, narrowphase
│   └── constraints.lua   # Contacts, joints
├── rendering/
│   └── item_renderer.lua # Draw calls, batching
├── config/
│   └── physics_db.lua    # Physics properties database
└── util/
    └── water_flow.lua    # Water simulation (isolated)
```

### 4.2 Realtime Sky Refactor

**Current:** Multiple files with mixed concerns

**Target:**
```
mods/realtime_sky/scripts/
├── main.lua              # Entry point
├── astronomy/
│   ├── sun_calc.lua      # Solar position
│   └── earth_time.lua    # Time calculations
├── rendering/
│   ├── sky_renderer.lua  # Sky dome
│   └── cloud_renderer.lua # Cloud layers
├── ui/
│   └── settings_ui.lua   # Settings screen only
└── data/
    ├── cities.lua        # Data only
    └── places.lua        # Data only
```

---

## Part 5: Implementation Phases

### Phase 1: Foundation (Week 1)
- [ ] Create `/workspace/mods/lib/init.lua`
- [ ] Create `/workspace/mods/lib/event_bus.lua`
- [ ] Create `/workspace/mods/lib/object_pool.lua`
- [ ] Enhance `/workspace/mods/lib/math_util.lua`
- [ ] Document standard mod template

### Phase 2: Box3D Optimization (Week 2-3)
- [ ] Remove duplicate box3d.lua from item_drop_physics
- [ ] Implement vector pooling
- [ ] Add contact pooling
- [ ] Implement spatial hashing broadphase
- [ ] Add warm starting to solver
- [ ] Benchmark and tune

### Phase 3: Mod Refactoring (Week 4-5)
- [ ] Refactor item_drop_physics into components
- [ ] Refactor realtime_sky into components
- [ ] Update smaller mods to use standard template
- [ ] Create migration guide

### Phase 4: Standardization (Week 6)
- [ ] Update all mods to use lib.event_bus
- [ ] Consolidate duplicated utilities
- [ ] Create automated style checker
- [ ] Write comprehensive documentation

---

## Part 6: Testing Strategy

### 6.1 Unit Tests
```lua
-- tests/box3d_vector_test.lua
-- tests/box3d_collision_test.lua
-- tests/event_bus_test.lua
```

### 6.2 Performance Benchmarks
```lua
-- benchmarks/physics_100_entities.lua
-- benchmarks/physics_500_entities.lua
-- benchmarks/gc_pressure_test.lua
```

### 6.3 Integration Tests
- Verify all mods load without errors
- Test mod compatibility
- Validate save/load cycles

---

## Part 7: Success Metrics

1. **Code Quality**
   - Zero duplicate utility functions across mods
   - All mods follow standard template
   - 100% of mods use centralized event bus

2. **Performance**
   - 10x reduction in allocations per frame
   - 4x more entities supported at same FPS
   - GC pauses reduced by 80%

3. **Maintainability**
   - New mod creation time: <1 hour
   - Bug fix time reduced by 50%
   - Clear separation of concerns in all mods

---

## Appendix A: Quick Reference - Standard Patterns

### Module Import Pattern
```lua
local lib = minecraft.require("lib")
local specific_module = lib.module_name
```

### Event Registration Pattern
```lua
event_bus.register("tick", my_handler, priority)
```

### Configuration Pattern
```lua
local config = config.load("mod_name", "settings.json", defaults)
```

### Object Pool Pattern
```lua
local obj = pool.acquire()
-- use obj
pool.release(obj)
```

---

## Appendix B: File Checklist

### Files to Create
- [ ] `/workspace/mods/lib/init.lua`
- [ ] `/workspace/mods/lib/event_bus.lua`
- [ ] `/workspace/mods/lib/object_pool.lua`
- [ ] `/workspace/mods/lib/spatial_hash.lua`
- [ ] `/workspace/mods/templates/mod_template/` (template directory)

### Files to Modify
- [ ] `/workspace/mods/lib/box3d.lua` (optimize)
- [ ] `/workspace/mods/lib/math_util.lua` (enhance)
- [ ] All mod main.lua files (standardize)

### Files to Remove
- [ ] `/workspace/mods/item_drop_physics/scripts/box3d.lua` (duplicate)

---

*Document Version: 1.0*
*Last Updated: [Current Date]*
*Author: Code Consolidation Initiative*
