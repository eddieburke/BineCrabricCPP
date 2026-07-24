-- ================================================================
-- MOD: camera
-- Entity-based tripod + camera system
-- ================================================================

local config = require("camera.config")

-- ================================================================
-- Constants
-- ================================================================

local TRIPOD_ITEM = 401
local CAMERA_ITEM = 402

local SIDE_OFFSETS = {
  [0] = {x= 0, y=-1, z= 0},
  [1] = {x= 0, y= 1, z= 0},
  [2] = {x= 0, y= 0, z=-1},
  [3] = {x= 0, y= 0, z= 1},
  [4] = {x=-1, y= 0, z= 0},
  [5] = {x= 1, y= 0, z= 0},
}

-- ================================================================
-- State
-- ================================================================

local smooth_yaw = 0
local smooth_pitch = 0
local rotation_angle = 0
local channels = {}
local viewfinder_open = false

-- ================================================================
-- Models
-- ================================================================

local camera_model = minecraft.model and minecraft.model.load("models/camera/camera.json")
local tv_model = minecraft.model and minecraft.model.load("models/camera/tv/tv.json")
local tripod_bottom_model = minecraft.model and minecraft.model.load("models/camera/tripod_bottom/tripod_bottom.json")
local tripod_top_model = minecraft.model and minecraft.model.load("models/camera/tripod_top/tripod_top.json")

-- ================================================================
-- Utility functions
-- ================================================================

local function lerp(a, b, t)
  return a + (b - a) * t
end

local function wrap_angle(angle)
  while angle > 180 do angle = angle - 360 end
  while angle < -180 do angle = angle + 360 end
  return angle
end

local function copy_data(ent)
  local data = {}
  if ent.data then
    for k, v in pairs(ent.data) do data[k] = v end
  end
  return data
end

-- ================================================================
-- Entity helpers
-- ================================================================

local function isTripod(ent)
  return ent and ent.registry_id == "camera:tripod"
end

local function isCamera(ent)
  return ent and ent.registry_id == "camera:camera"
end

local function findNearestTripod(x, y, z, maxDist)
  local best, bestDist = nil, maxDist * maxDist
  for _, ent in ipairs(minecraft.entities.list("camera:tripod")) do
    local dx, dy, dz = ent.x - x, ent.y - y, ent.z - z
    local dist = dx * dx + dy * dy + dz * dz
    if dist < bestDist then
      best, bestDist = ent, dist
    end
  end
  return best
end

local function getTripodForCamera(cam)
  if cam.data and cam.data.tripod_id and cam.data.tripod_id ~= 0 then
    return minecraft.entities.get(cam.data.tripod_id)
  end
  return nil
end

-- ================================================================
-- Event handlers: tick / fov / render
-- ================================================================

local function on_tick(dt)
  if config.auto_rotate then
    rotation_angle = rotation_angle + dt * (config.rotate_speed or 5.0)
  end
end

local function on_fov(event)
  event.fov = config.fov or 70.0
end

local function on_render(camera)
  if camera then
    local smoothness = config.smoothness or 0.1
    smooth_yaw = lerp(smooth_yaw, camera.yaw or 0, smoothness)
    smooth_pitch = lerp(smooth_pitch, camera.pitch or 0, smoothness)
  end
end

-- ================================================================
-- Event handlers: item placement
-- ================================================================

local function on_block_interact(event)
  local off = SIDE_OFFSETS[event.side] or SIDE_OFFSETS[1]
  local ex = event.x + off.x
  local ey = event.y + off.y
  local ez = event.z + off.z

  if event.item_id == TRIPOD_ITEM then
    minecraft.entities.spawn_mod("camera:tripod", {
      x = ex + 0.5, y = ey, z = ez + 0.5,
      yaw = event.player_yaw or 0,
      data = { shadow_radius = 0 },
    })
  elseif event.item_id == CAMERA_ITEM then
    local tripod = findNearestTripod(ex, ey, ez, 3.0)
    if tripod then
      local cam = minecraft.entities.spawn_mod("camera:camera", {
        x = tripod.x, y = tripod.y + 1.0, z = tripod.z,
        yaw = tripod.yaw,
        data = { tripod_id = tripod.id, pitch = 0, shadow_radius = 0 },
      })
      if cam then
        local data = copy_data(tripod)
        data.camera_id = cam.id
        data.shadow_radius = 0
        minecraft.entities.apply_state(tripod, { data = data })
      end
    else
      minecraft.entities.spawn_mod("camera:camera", {
        x = ex + 0.5, y = ey + 0.5, z = ez + 0.5,
        yaw = event.player_yaw or 0,
        data = { pitch = 0, shadow_radius = 0 },
      })
    end
  end

  event.handled = true
  event.item_count = event.item_count - 1
end

-- ================================================================
-- Event handlers: entity interaction
-- ================================================================

local function pickup_tripod(ent)
  if ent.data and ent.data.camera_id and ent.data.camera_id ~= 0 then
    local cam = minecraft.entities.get(ent.data.camera_id)
    if cam then
      minecraft.entities.remove({id = cam.id})
      minecraft.inventory.give({id = CAMERA_ITEM, count = 1})
    end
  end
  minecraft.entities.remove({id = ent.id})
  minecraft.inventory.give({id = TRIPOD_ITEM, count = 1})
end

local function rotate_tripod(ent, player_yaw)
  local data = copy_data(ent)
  data.shadow_radius = 0
  minecraft.entities.apply_state(ent, { yaw = player_yaw or ent.yaw, data = data })
end

local function pickup_camera(ent)
  if ent.data and ent.data.tripod_id then
    local tripod = minecraft.entities.get(ent.data.tripod_id)
    if tripod then
      local data = copy_data(tripod)
      data.camera_id = 0
      data.shadow_radius = 0
      minecraft.entities.apply_state(tripod, { data = data })
    end
  end
  minecraft.entities.remove({id = ent.id})
  minecraft.inventory.give({id = CAMERA_ITEM, count = 1})
end

local function rotate_camera(ent)
  local curPitch = (ent.data and ent.data.pitch) or 0
  local newPitch = curPitch + 15
  if newPitch > 90 then newPitch = -90 end
  local data = copy_data(ent)
  data.pitch = newPitch
  data.shadow_radius = 0
  minecraft.entities.apply_state(ent, { data = data })
end

local function on_entity_interact(event)
  local ent = minecraft.entities.get(event.entity_id)
  if not ent then return end

  if isTripod(ent) then
    if event.sneaking then
      pickup_tripod(ent)
    else
      rotate_tripod(ent, event.player_yaw)
    end
    event.handled = true

  elseif isCamera(ent) then
    if event.sneaking then
      pickup_camera(ent)
    else
      rotate_camera(ent)
    end
    event.handled = true
  end
end

-- ================================================================
-- Event handlers: entity rendering
-- ================================================================

local function render_tripod(ent)
  if tripod_bottom_model then
    minecraft.model.draw(tripod_bottom_model, {
      x = ent.x, y = ent.y, z = ent.z,
      yaw = ent.yaw,
    })
  end
  if tripod_top_model then
    minecraft.model.draw(tripod_top_model, {
      x = ent.x, y = ent.y + 1, z = ent.z,
      yaw = ent.yaw,
    })
  end
end

local function render_camera(ent)
  local pitch = (ent.data and ent.data.pitch) or 0
  local camY = ent.y
  local camYaw = ent.yaw
  local tripod = getTripodForCamera(ent)
  if tripod then
    camY = tripod.y + 1.0
    camYaw = tripod.yaw
  end
  if camera_model then
    minecraft.model.draw(camera_model, {
      x = ent.x, y = camY, z = ent.z,
      yaw = camYaw, pitch = pitch,
    })
  end
end

local function on_world_render(event)
  for _, ent in ipairs(minecraft.entities.list("camera:tripod")) do
    render_tripod(ent)
  end
  for _, ent in ipairs(minecraft.entities.list("camera:camera")) do
    render_camera(ent)
  end
end

-- ================================================================
-- Event handlers: camera feed (render-to-texture)
-- ================================================================

local function on_render_frame(event)
  if not event.has_world then return end
  for _, cam in ipairs(minecraft.entities.list("camera:camera")) do
    local ch = cam.data and cam.data.channel
    if ch and ch > 0 then
      local pitch = (cam.data and cam.data.pitch) or 0
      local yaw = cam.yaw
      local camY = cam.y + 0.4
      local tripod = getTripodForCamera(cam)
      if tripod then
        camY = tripod.y + 1.4
        yaw = tripod.yaw
      end
      minecraft.camera.render(ch, cam.x, camY, cam.z, yaw, pitch, 0, 70, event.tick_delta)
    end
  end
end

local function on_world_open(event)
  channels = {}
  viewfinder_open = false
end

-- ================================================================
-- Event registration
-- ================================================================

minecraft.on(minecraft.events.client_tick, { before = true }, on_tick)

minecraft.on(minecraft.events.fov, {}, on_fov)

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.entities,
  moment = minecraft.render.moments.after,
}, on_world_render)

minecraft.on(minecraft.events.render_frame, {}, on_render_frame)

minecraft.on(minecraft.events.world_open, {}, on_world_open)

minecraft.on(minecraft.events.block_interact, {
  right_click = true,
  has_item = true,
  local_player = true,
  when = function(event)
    return event.item_id == TRIPOD_ITEM or event.item_id == CAMERA_ITEM
  end,
}, on_block_interact)

minecraft.on(minecraft.events.entity_interact, {
  attack = false,
  has_player = true,
  local_player = true,
}, on_entity_interact)

-- ================================================================
-- Block / item registration
-- ================================================================

minecraft.register_block({
  id = 186,
  name = "TV",
  translation_key = "tvBlock",
  texture = "mods/camera/tv.png",
  model = tv_model,
  opaque = false,
  full_cube = false,
})

minecraft.register_item({
  id = TRIPOD_ITEM,
  name = "Tripod",
  translation_key = "camera.tripod",
  texture = "mods/camera/tripod.png",
  max_count = 16,
})

minecraft.register_item({
  id = CAMERA_ITEM,
  name = "Camera",
  translation_key = "camera.camera",
  texture = "mods/camera/camera.png",
  max_count = 1,
})

minecraft.log("info", "Camera mod loaded (entity-based)")
