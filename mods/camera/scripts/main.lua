local config = require("config")

local TRIPOD_ITEM = 402
local TV_BLOCK = 152
local FUSE_TIME = 80
local CAPTURE_GRACE_TICKS = 10
local SIDES = {
  [0] = { x = 0, y = -1, z = 0 },
  [1] = { x = 0, y = 1, z = 0 },
  [2] = { x = 0, y = 0, z = -1 },
  [3] = { x = 0, y = 0, z = 1 },
  [4] = { x = -1, y = 0, z = 0 },
  [5] = { x = 1, y = 0, z = 0 },
}

local is_client = minecraft.is_client()
-- Models only exist client-side, but a client-side load failure must be loud:
-- model.load returns nil plus an error message, and the old
-- "is_client and load(...) or nil" idiom threw that message away, leaving the
-- block registered with no model at all.
local function load_model(path)
  if not is_client then
    return nil
  end
  return assert(minecraft.model.load(path))
end

local camera_model = load_model("models/camera/camera.json")
local tripod_bottom_model = load_model("models/camera/tripod_bottom/tripod_bottom.json")
local tripod_top_model = load_model("models/camera/tripod_top/tripod_top.json")
local tv_model = load_model("models/camera/tv/tv.json")
local captured = {}

local function on_block_interact(event)
  if event.remote then return end

  local off = SIDES[event.side] or SIDES[1]
  minecraft.entities.spawn_mod("camera:tripod", {
    x = event.x + off.x + 0.5,
    y = event.y + off.y,
    z = event.z + off.z + 0.5,
    yaw = event.player_yaw or 0,
    data = { fuse = 0, primed = false, capture_ticks = 0 },
  })
  if not config.renewable then
    event.item_count = event.item_count - 1
  end
  event.handled = true
end

local function on_entity_interact(event)
  if event.remote or not event.attack then return end
  local ent = minecraft.entities.get(event.target_id)
  if not ent or ent.registry_id ~= "camera:tripod" then return end

  local data = ent.data or {}
  if not data.primed and (data.capture_ticks or 0) == 0 then
    data.fuse = FUSE_TIME
    data.primed = true
    minecraft.entities.apply_state(ent, { data = data })
  elseif data.primed then
    minecraft.entities.remove({ id = ent.id })
    minecraft.drop_item(TRIPOD_ITEM, 1, ent.x, ent.y + 0.5, ent.z)
  end
  event.handled = true
end

local function tick_tripods()
  for _, ent in ipairs(minecraft.entities.list("camera:tripod")) do
    local data = ent.data or {}
    if data.primed and (data.fuse or 0) > 0 then
      data.fuse = data.fuse - 1
      if data.fuse <= 0 then
        data.fuse = 0
        data.primed = false
        data.capture_ticks = CAPTURE_GRACE_TICKS
      end
      minecraft.entities.apply_state(ent, { data = data })
    elseif (data.capture_ticks or 0) > 0 then
      data.capture_ticks = data.capture_ticks - 1
      if data.capture_ticks <= 0 then
        captured[ent.id] = nil
        minecraft.entities.remove({ id = ent.id })
      else
        minecraft.entities.apply_state(ent, { data = data })
      end
    end
  end
end

local function on_server_tick(event)
  if not event.paused then tick_tripods() end
end

local function on_client_tick(event)
  if event.paused then return end
  if not event.remote then tick_tripods() end
  for _, ent in ipairs(minecraft.entities.list("camera:tripod")) do
    local data = ent.data or {}
    if data.primed then
      minecraft.particles.spawn({
        x = ent.x, y = ent.y + 1.6, z = ent.z,
        scale = 2.5, r = 0.5, g = 0.5, b = 0.5, max_age = 35,
      })
    end
  end
end

local function draw_model(model, ent, y, scale, flash)
  if not model then return end
  minecraft.model.draw(model, {
    x = ent.x, y = y, z = ent.z, yaw = ent.yaw, scale = scale,
  })
  if flash then
    minecraft.model.draw(model, {
      x = ent.x, y = y, z = ent.z, yaw = ent.yaw, scale = scale,
      brightness = 1.0, a = 0.5, blend = true,
    })
  end
end

local function draw_tripod(ent, shadow_pass)
  local data = ent.data or {}
  local scale = 1.0
  local flash = false
  if data.primed and data.fuse then
    if data.fuse < 10 then
      local t = 1.0 - data.fuse / 10.0
      scale = 1.0 + t * t * t * t * 0.3
    end
    flash = not shadow_pass and math.floor(data.fuse / 5) % 2 == 0
  end

  draw_model(tripod_bottom_model, ent, ent.y, scale, flash)
  draw_model(tripod_top_model, ent, ent.y + 0.75, scale, flash)
  draw_model(camera_model, ent, ent.y + 1.5, scale, flash)
end

local function on_world_render(event)
  for _, ent in ipairs(minecraft.entities.list("camera:tripod")) do
    if ent.id ~= event.excluded_entity_id then
      draw_tripod(ent, event.shadow_pass)
    end
  end
end

local function take_tripod_screenshot(ent, tick_delta)
  local handle = minecraft.camera.create(config.screenshot_width or 640, config.screenshot_height or 480)
  if handle < 0 then return false end

  local ok = minecraft.camera.render(handle, ent.x, ent.y + 2.15, ent.z, ent.yaw, 0, 0, 70, tick_delta, ent.id)
  if ok then minecraft.camera.save_screenshot(handle) end
  minecraft.camera.destroy(handle)
  return ok
end

local function on_render_frame(event)
  local active = {}
  for _, ent in ipairs(minecraft.entities.list("camera:tripod")) do
    active[ent.id] = true
    local data = ent.data or {}
    if (data.capture_ticks or 0) > 0 and not captured[ent.id] then
      captured[ent.id] = take_tripod_screenshot(ent, event.tick_delta)
    end
  end
  for id in pairs(captured) do
    if not active[id] then captured[id] = nil end
  end
end

minecraft.register_item({
  id = TRIPOD_ITEM,
  name = "Tripod Camera",
  translation_key = "camera.tripod",
  texture = "mods/camera/tripod.png",
  max_count = 16,
})

minecraft.register_block({
  id = TV_BLOCK,
  texture = "models/camera/tv/tv_body.png",
  hardness = 1.5,
  resistance = 4.0,
  translation_key = "camera.tv",
  material = "metal",
  opaque = false,
  translucent = false,
  full_cube = false,
  model = tv_model,
  item = { texture = "mods/camera/tv_icon.png" },
})

minecraft.register_shaped_recipe({
  output_item_id = TRIPOD_ITEM,
  output_count = 1,
  pattern = { "pgp", "sps", "srs" },
  ingredients = { p = 5, g = 20, s = 280, r = 331 },
})

minecraft.on("block_interact", {
  right_click = true, has_item = true,
  when = function(event) return event.item_id == TRIPOD_ITEM end,
}, on_block_interact)

minecraft.on("entity_interact", { attack = true, has_player = true }, on_entity_interact)

if is_client then
  minecraft.on("client_tick", { after_world = true }, on_client_tick)
  minecraft.on("world_render", {
    stage = "entities",
    moment = "after",
  }, on_world_render)
  minecraft.on("render_frame", {}, on_render_frame)
else
  minecraft.on("world_tick", {}, on_server_tick)
end

minecraft.log("info", "Tripod Camera loaded")
