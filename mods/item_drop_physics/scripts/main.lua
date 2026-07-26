-- ============================================================================
-- MOD: item_drop_physics
--
-- Hides vanilla dropped-item rendering, simulates the drops with the mod's own
-- rigid-body engine, and draws them tumbling. Material response is derived from
-- physics/materials.lua; there is no per-item tuning table.
-- ============================================================================

local engine = require("scripts.physics.engine")
local renderer = require("scripts.rendering.item_renderer")

local TICK_SECONDS = 1.0 / 20.0
-- The server owns the authoritative item position. The local sim is free to
-- differ by up to this much before it gets snapped back onto the entity.
local REANCHOR_DISTANCE = 1.5

-- entity id -> body
local sims = {}
-- Bodies seen this tick, used to retire drops that despawned or were picked up.
local seen = {}

--------------------------------------------------------------------------------
-- SIMULATION
--------------------------------------------------------------------------------

local function sync_sims(items)
  for id in pairs(seen) do seen[id] = nil end

  for i = 1, #items do
    local item = items[i]
    local id = item.id
    if id and item.item_id then
      seen[id] = true
      local s = sims[id]
      if not s then
        local hx, hy, hz = renderer.half_extents(item)
        sims[id] = engine.create(item, hx, hy, hz)
      else
        -- Follow server corrections without fighting them every tick.
        local dx, dy, dz = item.x - s.x, item.y - s.y, item.z - s.z
        if dx * dx + dy * dy + dz * dz > REANCHOR_DISTANCE * REANCHOR_DISTANCE then
          s.x, s.y, s.z = item.x, item.y, item.z
          s.px, s.py, s.pz = item.x, item.y, item.z
          s.vx, s.vy, s.vz = item.vx or 0.0, item.vy or 0.0, item.vz or 0.0
          s.sleeping, s.sleep_ticks = false, 0
        end
      end
    end
  end

  for id in pairs(sims) do
    if not seen[id] then sims[id] = nil end
  end
end

minecraft.on("client_tick", { before = false, after_world = true, paused = false }, function(event)
  if not event.has_world then return end
  local items = minecraft.entities.list("Item")
  if not items then return end

  sync_sims(items)
  engine.step(sims, TICK_SECONDS)
end)

--------------------------------------------------------------------------------
-- RENDERING
--------------------------------------------------------------------------------

minecraft.on("pre_entity_render", { entity_type = "Item" }, function(event)
  event.canceled = true
  return event
end)

local function render_items(event)
  local delta = event.tick_delta or 1.0
  local items = minecraft.entities.list("Item")
  if not items then return end
  for i = 1, #items do
    local item = items[i]
    local s = sims[item.id]
    if s then renderer.draw(item, s, delta) end
  end
end

-- Opaque pass gets normal shader lighting; the entity pass only re-draws for
-- the shadow map.
minecraft.on("world_render", { stage = "terrain_opaque", moment = "after" }, function(event)
  if not event.shadow_pass then render_items(event) end
end)

minecraft.on("world_render", { stage = "entities", moment = "after" }, function(event)
  if event.shadow_pass then render_items(event) end
end)

minecraft.log("info", "item_drop_physics loaded")
