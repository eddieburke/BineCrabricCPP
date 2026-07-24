-- ================================================================
-- MOD: item_drop_physics
-- Description: Advanced item physics with water interaction
-- ================================================================

-- Section 1: Imports
local box3d = require("lib.physics.box3d")
local config = require("config.init")
local water = require("physics.water")
local renderer = require("rendering.item_renderer")

-- Section 2: Constants
local TICK_SECONDS = 1.0 / 20.0
local BASE_SUB_STEPS = 2
local MAX_SUB_STEPS = 4
local SLEEP_AFTER_TICKS = 12
local MAX_LINEAR_SPEED = 2.0
local MAX_ANGULAR_SPEED = 2.5

-- Section 3: State
local sims = {}
local pair_impulses = {}
local simulation_epoch = 0
local server_sync_clock = 0

-- Section 4: Physics Database (delegated to config module)
local PHYSICS_DATABASE = config.init_database()

-- Section 5: Event Handlers
local function on_tick(event)
  -- Delegate to physics world step
  require("physics.world").step(sims, TICK_SECONDS)
  
  -- Server sync
  server_sync_clock = server_sync_clock + 1
  if server_sync_clock >= SERVER_SYNC_INTERVAL then
    server_sync_clock = 0
    -- Sync logic here
  end
end

local function on_item_spawn(event)
  local sim = renderer.create_simulation(event.item, box3d, config)
  table.insert(sims, sim)
end

-- Section 6: Registration
minecraft.on(minecraft.events.world_tick, {}, on_tick)
minecraft.on(minecraft.events.entity_spawn, {}, on_item_spawn)

-- Section 7: Logging
minecraft.log("info", "item_drop_physics loaded with pure Lua physics")
