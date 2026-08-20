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
  local shape = renderer.physics_shape(item)
  local s = engine.create(item, shape)
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

-- Every dropped item in the loaded world used to get a full engine draw each frame
-- (and again for the shadow pass): one geometry rebuild, one program bind, one
-- uniform upload and one GL draw call apiece, with no regard for whether it was
-- 200 blocks away or directly behind the camera.
local DRAW_RADIUS = 64.0
local DRAW_RADIUS_SQ = DRAW_RADIUS * DRAW_RADIUS
-- cos(100 degrees). Deliberately wider than any real FOV so an item never pops out
-- at the screen edge; it only rejects what is clearly behind the camera.
local BEHIND_COS = -0.17
local RAD = math.pi / 180.0

local function render_items(event, frame)
  local delta = math.max(0.0, math.min(1.0, event.tick_delta or 1.0))
  local items = minecraft.entities.list("Item")
  if not items then return end

  local cam_x, cam_y, cam_z = event.eye_x, event.eye_y, event.eye_z
  local have_camera = cam_x ~= nil and cam_y ~= nil and cam_z ~= nil
  -- Shadow casters legitimately sit outside the view frustum, so only the visible
  -- pass gets the facing test. Distance still applies to both.
  local fwd_x, fwd_y, fwd_z
  if have_camera and not event.shadow_pass and event.camera_yaw and event.camera_pitch then
    local yaw = event.camera_yaw * RAD
    local pitch = event.camera_pitch * RAD
    local cos_pitch = math.cos(pitch)
    fwd_x = -math.sin(yaw) * cos_pitch
    fwd_y = -math.sin(pitch)
    fwd_z = math.cos(yaw) * cos_pitch
  end

  local radius_sq = DRAW_RADIUS_SQ
  local render_distance = event.render_distance
  if render_distance and render_distance > 0 and render_distance < DRAW_RADIUS then
    radius_sq = render_distance * render_distance
  end

  for i = 1, #items do
    local item = items[i]
    local visible = true
    if have_camera and item.x then
      local dx, dy, dz = item.x - cam_x, item.y - cam_y, item.z - cam_z
      local dist_sq = dx * dx + dy * dy + dz * dz
      if dist_sq > radius_sq then
        visible = false
      elseif fwd_x and dist_sq > 4.0 then
        -- Items within 2 blocks stay drawn regardless: they can be under the
        -- crosshair while the eye sits inside their bounding box.
        local inv = 1.0 / math.sqrt(dist_sq)
        if (dx * fwd_x + dy * fwd_y + dz * fwd_z) * inv < BEHIND_COS then
          visible = false
        end
      end
    end
    if visible then
      local s = get_sim(item)
      if s then
        local ok, drawn = pcall(renderer.draw, item, s, delta)
        if ok and drawn then s.render_frame = frame end
      end
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
