local engine = require("scripts.physics.engine")
local renderer = require("scripts.rendering.item_renderer")

local TICK_SECONDS = 1.0 / 20.0
local RETIRE_TICKS = 40

local sims = {}
local tick_index = 0
local render_frame = 0

local function clear_sims()
  for id in pairs(sims) do sims[id] = nil end
end

local function valid_item(item)
  return item and item.id ~= nil and item.item_id ~= nil and item.item_id > 0
end

local function create_sim(item)
  local hx, hy, hz = renderer.half_extents(item)
  local s = engine.create(item, hx, hy, hz)
  sims[item.id] = s
  return s
end

local function get_sim(item)
  if not valid_item(item) then return nil end
  local s = sims[item.id]
  if not s or s.item_id ~= item.item_id or s.item_damage ~= (item.item_damage or 0) then
    s = create_sim(item)
  end
  return s
end

local function sync_sims(items)
  tick_index = tick_index + 1
  for i = 1, #items do
    local item = items[i]
    local s = get_sim(item)
    if s then
      s.last_seen = tick_index
      engine.sync(s, item)
    end
  end
  for id, s in pairs(sims) do
    if tick_index - (s.last_seen or tick_index) > RETIRE_TICKS then sims[id] = nil end
  end
end

minecraft.on("client_tick", { before = false, after_world = true, paused = false }, function(event)
  if not event.has_world then
    clear_sims()
    return
  end
  local items = minecraft.entities.list("Item")
  if not items then return end
  sync_sims(items)
  engine.step(sims, TICK_SECONDS)
end)

minecraft.on("entity_spawn", { entity_type = "Item" }, function(event)
  if event.entity_type ~= "Item" then return end
  local item = minecraft.entities.get(event.entity_id)
  if item then get_sim(item) end
end)

minecraft.on("entity_remove", { entity_type = "Item" }, function(event)
  if event.entity_type ~= "Item" then return end
  sims[event.entity_id] = nil
end)

minecraft.on("pre_entity_render", { entity_type = "Item" }, function(event)
  local s = sims[event.entity_id]
  if s and s.render_frame == render_frame then event.canceled = true end
  return event
end)

local function render_items(event, frame)
  local delta = math.max(0.0, math.min(1.0, event.tick_delta or 1.0))
  local items = minecraft.entities.list("Item")
  if not items then return end
  for i = 1, #items do
    local item = items[i]
    local s = get_sim(item)
    if s then
      local ok, drawn = pcall(renderer.draw, item, s, delta)
      if ok and drawn then s.render_frame = frame end
    end
  end
end

minecraft.on("world_render", { stage = "terrain_opaque", moment = "after" }, function(event)
  if event.shadow_pass then return end
  render_frame = render_frame + 1
  render_items(event, render_frame)
end)

minecraft.on("world_render", { stage = "entities", moment = "after" }, function(event)
  if event.shadow_pass then render_items(event, render_frame) end
end)

minecraft.log("info", "item_drop_physics loaded")
