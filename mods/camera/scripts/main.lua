local tripod_bottom_model = assert(minecraft.model.load("models/camera/tripod_bottom.json"))
local tripod_top_model = assert(minecraft.model.load("models/camera/tripod_top.json"))
local camera_model = assert(minecraft.model.load("models/camera/camera.json"))
local tv_model = assert(minecraft.model.load("models/tv.json"))

local TRIPOD_ITEM = 30801
local CAMERA_ITEM = 30800
local TV_BLOCK = 162
local SCREEN_ID = "camera:viewfinder"
local MODEL_SCALE = 1.0
local offsets = {
  [0] = { 0, -1, 0 },
  [1] = { 0, 1, 0 },
  [2] = { 0, 0, -1 },
  [3] = { 0, 0, 1 },
  [4] = { -1, 0, 0 },
  [5] = { 1, 0, 0 },
}

local pending_tvs = {}
local client = {
  channels = {},
  open = false,
}

local function entity_by_id(id)
  for _, entity in ipairs(minecraft.entities.list()) do
    if entity.id == id then return entity end
  end
  return nil
end

local function adjacent(x, y, z, side)
  local offset = offsets[side] or offsets[1]
  return x + offset[1], y + offset[2], z + offset[3]
end

local function facing(yaw)
  local angle = ((yaw or 0) % 360 + 360) % 360
  if angle >= 45 and angle < 135 then return 1, 0 end
  if angle >= 135 and angle < 225 then return 0, 1 end
  if angle >= 225 and angle < 315 then return -1, 0 end
  return 0, -1
end

local function camera_position(entity)
  local linked = entity.data and entity.data.on_tripod or 0
  if linked ~= 0 then
    for _, tripod in ipairs(minecraft.entities.list()) do
      if tripod.registry_id == "camera:tripod" and math.abs(tripod.x - entity.x) < 0.1 and math.abs(tripod.z - entity.z) < 0.1 then
        return tripod.x, tripod.y + 2, tripod.z
      end
    end
  end
  return entity.x, entity.y, entity.z
end

local function consume(event)
  event.item_count = math.max(0, (event.item_count or 1) - 1)
  event.handled = true
  return event
end

local function queue_tv(event)
  local x, y, z = adjacent(event.x, event.y, event.z, event.side)
  local nx, nz = facing(event.player_yaw)
  pending_tvs[x .. ":" .. y .. ":" .. z] = { x = x, y = y, z = z, nx = nx, nz = nz }
end

minecraft.register_item({
  id = TRIPOD_ITEM,
  name = "Tripod",
  translation_key = "camera.tripod",
  max_count = 16,
  texture = "mods/camera/tripod.png",
})

minecraft.register_item({
  id = CAMERA_ITEM,
  name = "Camera",
  translation_key = "camera.camera",
  max_count = 1,
  texture = "mods/camera/camera.png",
})

minecraft.register_block({
  id = TV_BLOCK,
  texture = "mods/camera/tv_body.png",
  hardness = 1.5,
  resistance = 4.0,
  translation_key = "camera.tv",
  material = "metal",
  opaque = false,
  full_cube = false,
  translucent = false,
  model = tv_model,
  tile_entity = "tv",
  item = { texture = "mods/camera/tv_icon.png" },
})

minecraft.register_shaped_recipe({
  output_item_id = TRIPOD_ITEM,
  output_count = 1,
  pattern = { "# #", " # ", " # " },
  key = "#",
  item_id = 265,
})

minecraft.register_shaped_recipe({
  output_item_id = CAMERA_ITEM,
  output_count = 1,
  pattern = { "###", "#R#", "###" },
  key = "#",
  item_id = 265,
  key2 = "R",
  item_id2 = 331,
})

minecraft.register_shaped_recipe({
  output_block_id = TV_BLOCK,
  output_count = 1,
  pattern = { "###", "#G#", "#R#" },
  key = "#",
  item_id = 265,
  key2 = "G",
  item_id2 = 20,
  key3 = "R",
  item_id3 = 331,
})

minecraft.on(minecraft.events.block_interact, { right_click = true }, function(event)
  if event.remote or event.handled or event.canceled or not event.has_item then return event end
  if event.item_id == TV_BLOCK then
    queue_tv(event)
    return event
  end
  local x, y, z = adjacent(event.x, event.y, event.z, event.side)
  if event.item_id == TRIPOD_ITEM then
    if minecraft.entities.spawn_mod("camera:tripod", {
      x = x + 0.5,
      y = y,
      z = z + 0.5,
      yaw = event.player_yaw or 0,
      data = { mounted = 0 },
    }) then
      return consume(event)
    end
  elseif event.item_id == CAMERA_ITEM then
    if minecraft.entities.spawn_mod("camera:camera", {
      x = x + 0.5,
      y = y,
      z = z + 0.5,
      yaw = ((event.player_yaw or 0) + 180) % 360,
      pitch = event.player_pitch or 0,
      data = { on_tripod = 0 },
    }) then
      return consume(event)
    end
  end
  return event
end)

minecraft.on(minecraft.events.entity_interact, { attack = false }, function(event)
  if event.remote then return event end
  local target = entity_by_id(event.target_id)
  if not target then return event end
  if target.registry_id == "camera:tripod" and event.has_item and event.item_id == CAMERA_ITEM then
    if not target.data or (target.data.mounted or 0) == 0 then
      local tripod_cam = minecraft.entities.spawn_mod("camera:camera", {
        x = target.x,
        y = target.y + 2,
        z = target.z,
        yaw = target.yaw,
        pitch = target.pitch or 0,
        data = { on_tripod = 1 },
      })
      if tripod_cam then
        target:apply_state({ data = { mounted = 1 } })
        return consume(event)
      end
    end
    return event
  end
  if event.has_item then return event end
  if event.sneaking then
    if target.registry_id == "camera:tripod" or target.registry_id == "camera:camera" then
      local yaw = (event.player_yaw or 0) % 360
      local pitch = -(event.player_pitch or 0)
      target:apply_state({ yaw = yaw, pitch = pitch })
      if target.registry_id == "camera:tripod" then
        local mounted = target.data and target.data.mounted or 0
        if mounted ~= 0 then
          for _, ent in ipairs(minecraft.entities.list()) do
            if ent.registry_id == "camera:camera" and math.abs(ent.x - target.x) < 0.1 and math.abs(ent.z - target.z) < 0.1 then
              ent:apply_state({ yaw = yaw, pitch = pitch })
            end
          end
        end
      end
      event.handled = true
    end
    return event
  end
  if target.registry_id == "camera:camera" then
    local mounted = target.data and target.data.on_tripod or 0
    if mounted ~= 0 then
      for _, ent in ipairs(minecraft.entities.list()) do
        if ent.registry_id == "camera:tripod" and math.abs(ent.x - target.x) < 0.1 and math.abs(ent.z - target.z) < 0.1 then
          ent:apply_state({ data = { mounted = 0 } })
        end
      end
    end
    target:remove()
    minecraft.inventory.give({ id = CAMERA_ITEM, count = 1 })
    event.handled = true
  elseif target.registry_id == "camera:tripod" then
    local mounted = target.data and target.data.mounted or 0
    if mounted ~= 0 then
      for _, ent in ipairs(minecraft.entities.list()) do
        if ent.registry_id == "camera:camera" and math.abs(ent.x - target.x) < 0.1 and math.abs(ent.z - target.z) < 0.1 then
          ent:remove()
          minecraft.inventory.give({ id = CAMERA_ITEM, count = 1 })
        end
      end
    end
    target:remove()
    minecraft.inventory.give({ id = TRIPOD_ITEM, count = 1 })
    event.handled = true
  end
  return event
end)

minecraft.on(minecraft.events.world_tick, { remote = false, before = false }, function(event)
  for key, pending in pairs(pending_tvs) do
    local tile = minecraft.tile_entities.get(pending.x, pending.y, pending.z)
    if tile and tile:get_id() == "camera:tv" then
      tile:set_data({ nx = pending.nx, nz = pending.nz })
      pending_tvs[key] = nil
    end
  end
  return event
end)

local function client_entities(kind)
  local result = {}
  for _, entity in ipairs(minecraft.entities.list()) do
    if entity.registry_id == kind then table.insert(result, entity) end
  end
  return result
end

local function ensure_channels()
  local cameras = {}
  for _, entity in ipairs(client_entities("camera:camera")) do
    cameras[entity.id] = true
    if not client.channels[entity.id] then
      client.channels[entity.id] = minecraft.camera.create_display_size()
    end
  end
  for id, channel in pairs(client.channels) do
    if not cameras[id] then
      minecraft.camera.destroy(channel)
      client.channels[id] = nil
    end
  end
end

minecraft.on(minecraft.events.client_tick, { paused = false }, function(event)
  ensure_channels()
  return event
end)

local function get_current_camera()
  local cameras = client_entities("camera:camera")
  if #cameras == 0 then return nil end
  table.sort(cameras, function(a, b) return a.id < b.id end)
  if client.active_camera_id then
    for _, c in ipairs(cameras) do
      if c.id == client.active_camera_id then
        return c
      end
    end
  end
  client.active_camera_id = cameras[1].id
  return cameras[1]
end

minecraft.on(minecraft.events.block_interact, { right_click = true, block_id = TV_BLOCK }, function(event)
  if not event.local_player or event.has_item then return event end
  client.open = true
  minecraft.screen.open(SCREEN_ID, { title = "Camera Viewfinder", pause = false })
  return event
end)

minecraft.on(minecraft.events.render_targets, {}, function(event)
  if not client.open then return event end
  local camera = get_current_camera()
  if not camera then return event end
  local channel = client.channels[camera.id]
  if not channel or channel < 0 then return event end
  local x, y, z = camera_position(camera)
  minecraft.camera.render(channel, x, y + 0.4, z, camera.yaw, camera.pitch or 0, 0, 70, event.tick_delta)
  return event
end)

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.entities,
  moment = minecraft.render.moments.after,
}, function(event)
  for _, entity in ipairs(client_entities("camera:tripod")) do
    minecraft.model.draw(tripod_bottom_model, { x = entity.x, y = entity.y, z = entity.z, yaw = entity.yaw, scale = MODEL_SCALE, pivot_y = 0 })
    minecraft.model.draw(tripod_top_model, { x = entity.x, y = entity.y + 1, z = entity.z, yaw = entity.yaw, scale = MODEL_SCALE, pivot_y = 0 })
  end
  for _, entity in ipairs(client_entities("camera:camera")) do
    if minecraft.camera.rendering() ~= client.channels[entity.id] then
      local x, y, z = camera_position(entity)
      minecraft.model.draw(camera_model, { x = x, y = y, z = z, yaw = (180 - entity.yaw) % 360, pitch = -(entity.pitch or 0), scale = MODEL_SCALE, pivot_y = 0 })
    end
  end

  return event
end)

minecraft.on(minecraft.events.screen_event, { screen_id = SCREEN_ID }, function(event)
  if event.phase == "init" then
    local cameras = client_entities("camera:camera")
    table.sort(cameras, function(a, b) return a.id < b.id end)
    local width = event.width or 256
    local height = event.height or 192
    local x = math.floor((width - 256) / 2)
    local y = math.floor((height - 192) / 2)
    for i, camera in ipairs(cameras) do
      local bx = x + 260
      local by = y + (i - 1) * 24
      minecraft.screen.add_button(bx, by, 50, 20, "CH " .. i, function()
        client.active_camera_id = camera.id
      end)
    end
  elseif event.phase == "render" then
    local width = event.width or 256
    local height = event.height or 192
    local x = math.floor((width - 256) / 2)
    local y = math.floor((height - 192) / 2)
    
    local cameras = client_entities("camera:camera")
    table.sort(cameras, function(a, b) return a.id < b.id end)
    for i, camera in ipairs(cameras) do
      if camera.id == client.active_camera_id then
        local bx = x + 260
        local by = y + (i - 1) * 24
        minecraft.gui.fill_rect(bx - 2, by - 2, 54, 24, 0xFF2E7D32)
      end
    end

    minecraft.gui.fill_rect(x - 4, y - 4, 264, 200, 0xFF101014)
    local camera = get_current_camera()
    local texture = camera and client.channels[camera.id] and minecraft.camera.texture(client.channels[camera.id]) or -1
    if texture and texture > 0 then
      minecraft.gui.draw_texture(texture, x, y, 256, 192)
    else
      minecraft.gui.fill_rect(x, y, 256, 192, 0xFF000000)
      minecraft.gui.draw_centered_text(x, y + 92, 256, "NO SIGNAL", 0xFF5A6270)
    end
  elseif event.phase == "close" then
    client.open = false
  end
  return event
end)

minecraft.on(minecraft.events.world_start, {}, function(event)
  minecraft.model.clear()
  client.channels = {}
  client.open = false
  client.active_camera_id = nil
  return event
end)
