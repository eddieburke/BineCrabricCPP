local config = require("config")

local TEXTURE_OFFSET_RANGE = 1000.0
local WIND_DIRECTION_MIN = 0.5
local WIND_DIRECTION_MAX = 2.5
local TWO_PI = math.pi * 2.0
local INT_MAX = 0x7fffffff
local UINT_MASK = 0xffffffff
local TEXTURE_SCALE = 1.0 / 2048.0
local OFFSET_MULT = 0.0003
local MOVEMENT_SCALE = 0.02
local OPACITY_ALPHA = 0.8
local CLOUD_HEIGHT_OFFSET = 0.33
local REHASH_INTERVAL_MIN = 3000
local REHASH_INTERVAL_MAX = 7000
local INTERP_DURATION = 50000.0
local INCREMENT_SCALE = 0.02
local SKEW_STRENGTH = 0.00025
local GRID_STEP = 32

local layers = {}
local cloud_vertices = {}
local cloud_ticks = 0
local layer_signature = nil
local active_world_name = nil
local active_world_seed = nil
local wind_seed = 0
local base_dir_x = 1.0
local base_dir_z = 0.0
local base_speed = 1.0
local last_rehash_time = 0
local next_rehash_interval = REHASH_INTERVAL_MIN

local function now_millis()
  return math.floor(minecraft.time.utc_millis())
end

local function low32(value)
  return value & UINT_MASK
end

local function hash_float_1(seed)
  local hash = low32(seed ~ (seed >> 32))
  hash = low32(hash ~ (hash >> 20) ~ (hash >> 12))
  hash = low32(hash ~ (hash >> 7) ~ (hash >> 4))
  return (hash & INT_MAX) / INT_MAX
end

local function hash_float_2(seed)
  local hash = low32(seed ~ (seed >> 32))
  hash = low32(hash * 0x9e3779b9)
  hash = low32(hash ~ (hash >> 16))
  hash = low32(hash * 0x85ebca6b)
  hash = low32(hash ~ (hash >> 13))
  hash = low32(hash * 0xc2b2ae35)
  hash = low32(hash ~ (hash >> 16))
  return (hash & INT_MAX) / INT_MAX
end

local function normalize(x, z, fallback_x, fallback_z)
  local magnitude = math.sqrt(x * x + z * z)
  if magnitude > 0.0001 then
    return x / magnitude, z / magnitude
  end
  return fallback_x, fallback_z
end

local function update_wind_trend()
  local multiplier = config.wind_speed or 1.0
  local wind_speed_min = 0.1 * multiplier
  local wind_speed_max = 4.0 * multiplier
  local trend_seed = wind_seed + 99999
  local trend_angle = hash_float_1(trend_seed) * TWO_PI
  local trend_magnitude = WIND_DIRECTION_MIN + hash_float_2(trend_seed) *
      (WIND_DIRECTION_MAX - WIND_DIRECTION_MIN)
  local trend_x = math.cos(trend_angle) * trend_magnitude
  local trend_z = math.sin(trend_angle) * trend_magnitude
  base_dir_x, base_dir_z = normalize(trend_x, trend_z, 1.0, 0.0)
  base_speed = wind_speed_min + hash_float_1(trend_seed + 1) *
      (wind_speed_max - wind_speed_min)
end

local function config_signature()
  return string.format("%d_%.3f_%.3f_%.3f_%.3f",
      config.layer_count or 6,
      config.base_opacity or 0.7,
      config.cloud_scale or 1.0,
      config.layer_height_spacing or 12.0,
      config.wind_speed or 1.0)
end

local function build_layers()
  local count = config.layer_count or 6
  local opacity = config.base_opacity or 0.7
  local scale = config.cloud_scale or 1.0
  local current_time = now_millis()
  local time_hash = current_time // 100
  local time_hash_2 = current_time // 137
  local time_hash_3 = current_time // 173

  update_wind_trend()
  last_rehash_time = current_time
  next_rehash_interval = REHASH_INTERVAL_MIN + math.floor(
      hash_float_1(wind_seed + 88888) * (REHASH_INTERVAL_MAX - REHASH_INTERVAL_MIN))
  layers = {}

  for index = 0, count - 1 do
    local seed_tex = wind_seed ~ (index * 11111) ~ (time_hash + index * 137)
    local texture_x = (hash_float_1(seed_tex) - 0.5) * TEXTURE_OFFSET_RANGE
    local texture_z = (hash_float_2(seed_tex) - 0.5) * TEXTURE_OFFSET_RANGE

    local seed_skew = wind_seed ~ (index * 111111) ~ (time_hash_2 + index * 199)
    local skew_angle = hash_float_1(seed_skew) * TWO_PI
    local skew_magnitude = hash_float_2(seed_skew) * SKEW_STRENGTH
    local skew_x = math.cos(skew_angle) * skew_magnitude
    local skew_z = math.sin(skew_angle) * skew_magnitude
    local dir_x, dir_z = normalize(base_dir_x + skew_x, base_dir_z + skew_z,
        base_dir_x, base_dir_z)

    local seed_scale = wind_seed ~ (index * 98765) ~ (time_hash + index * 313)
    local random_scale = hash_float_1(seed_scale)
    local biased_scale = 1.0 - (1.0 - random_scale) * (1.0 - random_scale)
    local layer_scale = 0.5 * scale + biased_scale * scale

    local seed_increment = wind_seed ~ (index * 54321) ~ (time_hash_2 + index * 401)
    local increment_scale = INCREMENT_SCALE * (0.5 + hash_float_2(seed_increment))

    local seed_target = wind_seed ~ (index * 22222) ~ (time_hash_3 + index * 257)
    local random_target_x = (hash_float_1(seed_target) - 0.5) * TEXTURE_OFFSET_RANGE
    local random_target_z = (hash_float_2(seed_target) - 0.5) * TEXTURE_OFFSET_RANGE
    local target_texture_x = texture_x + (random_target_x - texture_x) * increment_scale
    local target_texture_z = texture_z + (random_target_z - texture_z) * increment_scale

    local progress = index / math.max(1, count - 1)
    local layer_opacity = 0.5 * opacity + 0.5 * opacity * (1.0 - progress * 0.5)
    local gray_random = hash_float_1(wind_seed + index * 77777)

    layers[index + 1] = {
      index = index,
      dir_x = dir_x,
      dir_z = dir_z,
      start_dir_x = dir_x,
      start_dir_z = dir_z,
      target_dir_x = dir_x,
      target_dir_z = dir_z,
      texture_x = texture_x,
      texture_z = texture_z,
      start_texture_x = texture_x,
      start_texture_z = texture_z,
      target_texture_x = target_texture_x,
      target_texture_z = target_texture_z,
      speed = base_speed,
      scale = layer_scale,
      increment_scale = increment_scale,
      opacity = layer_opacity,
      gray = gray_random * 0.4 + 0.6,
      use_gray = gray_random > 0.5,
      skew_x = skew_x,
      skew_z = skew_z,
      interp_start_time = current_time,
    }
  end

  layer_signature = config_signature()
end

local function event_seed(event)
  return math.tointeger(event.world_seed or 0) or math.floor(event.world_seed or 0)
end

local function sync_world(event)
  if not event.has_world then
    active_world_name = nil
    active_world_seed = nil
    layers = {}
    return false
  end

  local name = event.world_name or ""
  local seed = event_seed(event)
  if active_world_name ~= name or active_world_seed ~= seed then
    active_world_name = name
    active_world_seed = seed
    wind_seed = seed
    cloud_ticks = 0
    build_layers()
  elseif layer_signature ~= config_signature() then
    build_layers()
  end
  return true
end

local function rehash_wind(current_time)
  local mixed = wind_seed + current_time
  local hash = low32(mixed ~ (mixed >> 32))
  hash = low32(hash ~ (hash >> 20) ~ (hash >> 12))
  hash = low32(hash ~ (hash >> 7) ~ (hash >> 4))
  wind_seed = hash & INT_MAX
  update_wind_trend()
  next_rehash_interval = REHASH_INTERVAL_MIN + math.floor(
      hash_float_1(wind_seed + 88888) * (REHASH_INTERVAL_MAX - REHASH_INTERVAL_MIN))
  last_rehash_time = current_time
end

local function update_layer(layer, current_time)
  local elapsed = math.max(0, current_time - layer.interp_start_time)
  local progress = elapsed / INTERP_DURATION

  if progress >= 1.0 then
    layer.start_dir_x = layer.dir_x
    layer.start_dir_z = layer.dir_z
    layer.start_texture_x = layer.texture_x
    layer.start_texture_z = layer.texture_z

    local time_hash = current_time // 100
    local seed_target = wind_seed ~ (layer.index * 22222) ~ (time_hash + layer.index * 257)
    local random_texture_x = (hash_float_1(seed_target) - 0.5) * TEXTURE_OFFSET_RANGE
    local random_texture_z = (hash_float_2(seed_target) - 0.5) * TEXTURE_OFFSET_RANGE
    layer.target_texture_x = layer.texture_x +
        (random_texture_x - layer.texture_x) * layer.increment_scale
    layer.target_texture_z = layer.texture_z +
        (random_texture_z - layer.texture_z) * layer.increment_scale

    local ideal_x, ideal_z = normalize(base_dir_x + layer.skew_x,
        base_dir_z + layer.skew_z, base_dir_x, base_dir_z)
    local target_x = layer.dir_x + (ideal_x - layer.dir_x) * layer.increment_scale
    local target_z = layer.dir_z + (ideal_z - layer.dir_z) * layer.increment_scale
    layer.target_dir_x, layer.target_dir_z = normalize(target_x, target_z, ideal_x, ideal_z)

    local overshoot = elapsed - INTERP_DURATION
    layer.interp_start_time = current_time - overshoot
    progress = overshoot / INTERP_DURATION
  end

  progress = math.min(progress, 1.0)
  local interp_x = layer.start_dir_x + (layer.target_dir_x - layer.start_dir_x) * progress
  local interp_z = layer.start_dir_z + (layer.target_dir_z - layer.start_dir_z) * progress
  layer.dir_x, layer.dir_z = normalize(interp_x, interp_z, base_dir_x, base_dir_z)
  layer.texture_x = layer.start_texture_x +
      (layer.target_texture_x - layer.start_texture_x) * progress
  layer.texture_z = layer.start_texture_z +
      (layer.target_texture_z - layer.start_texture_z) * progress
  layer.speed = base_speed
end

local function update_layers(current_time)
  if current_time - last_rehash_time >= next_rehash_interval then
    rehash_wind(current_time)
  end
  for _, layer in ipairs(layers) do
    update_layer(layer, current_time)
  end
end

local function vanilla_celestial_angle(world_time, tick_delta)
  local angle = ((world_time + tick_delta) % 24000.0) / 24000.0 - 0.25
  if angle < 0.0 then angle = angle + 1.0 end
  if angle > 1.0 then angle = angle - 1.0 end
  local original = angle
  angle = 1.0 - (math.cos(angle * math.pi) + 1.0) * 0.5
  return original + (angle - original) / 3.0
end

local function cloud_color(event)
  local celestial = event.celestial_angle
  if celestial == nil then
    celestial = vanilla_celestial_angle(event.world_time or 0.0, event.tick_delta or 0.0)
  end
  local brightness = math.max(0.0, math.min(1.0,
      math.cos(celestial * TWO_PI) * 2.0 + 0.5))
  local red = math.max(event.cloud_r or 1.0, brightness * 0.9 + 0.1)
  local green = math.max(event.cloud_g or 1.0, brightness * 0.9 + 0.1)
  local blue = math.max(event.cloud_b or 1.0, brightness * 0.85 + 0.15)
  return red, green, blue
end

local function append_vertex(cursor, x, y, z, u, v, red, green, blue, alpha)
  cloud_vertices[cursor + 1] = x
  cloud_vertices[cursor + 2] = y
  cloud_vertices[cursor + 3] = z
  cloud_vertices[cursor + 4] = u
  cloud_vertices[cursor + 5] = v
  cloud_vertices[cursor + 6] = red
  cloud_vertices[cursor + 7] = green
  cloud_vertices[cursor + 8] = blue
  cloud_vertices[cursor + 9] = alpha
  return cursor + 9
end

minecraft.on("client_tick", { after_world = true }, function(event)
  if not sync_world(event) then return event end
  if not event.paused then cloud_ticks = cloud_ticks + 1 end
  update_layers(now_millis())
  return event
end)

minecraft.on("world_render", {
  stage = "clouds",
  moment = "before",
  priority = 100,
}, function(event)
  if not config.enabled or event.stage_enabled == false or event.is_nether then
    return event
  end
  if not sync_world(event) then return event end

  event.cancel_vanilla = true

  local tick_delta = event.tick_delta or 0.0
  local camera_x = event.camera_x or 0.0
  local camera_z = event.camera_z or 0.0
  local eye_x = event.eye_x or camera_x
  local eye_z = event.eye_z or camera_z
  local cloud_height = (event.cloud_base_height or 128.0) + CLOUD_HEIGHT_OFFSET
  local render_distance = math.max(64.0, event.render_distance or 256.0)
  local grid_radius = math.max(1, math.ceil(render_distance / 64.0))
  local grid_start = -GRID_STEP * grid_radius
  local grid_end = GRID_STEP * grid_radius
  local cloud_offset = (cloud_ticks + tick_delta) * OFFSET_MULT
  local cloud_red, cloud_green, cloud_blue = cloud_color(event)

  for _, layer in ipairs(layers) do
    if layer.opacity >= 0.01 then
      local layer_step = GRID_STEP
      if layer.index > 5 then
        layer_step = GRID_STEP * 2
      elseif layer.index > 2 then
        layer_step = GRID_STEP + GRID_STEP // 2
      end

      local inv_scale = 1.0 / layer.scale
      local offset_x = cloud_offset * layer.speed * layer.dir_x * MOVEMENT_SCALE
      local offset_z = cloud_offset * layer.speed * layer.dir_z * MOVEMENT_SCALE
      local texture_camera_x = (camera_x + offset_x) * inv_scale
      local texture_camera_z = (camera_z + offset_z) * inv_scale
      texture_camera_x = texture_camera_x - math.floor(texture_camera_x / 2048.0) * 2048.0
      texture_camera_z = texture_camera_z - math.floor(texture_camera_z / 2048.0) * 2048.0
      local base_u = (texture_camera_x + layer.texture_x) * TEXTURE_SCALE
      local base_v = (texture_camera_z + layer.texture_z) * TEXTURE_SCALE
      local red = layer.use_gray and layer.gray or cloud_red
      local green = layer.use_gray and layer.gray or cloud_green
      local blue = layer.use_gray and layer.gray or cloud_blue
      local alpha = OPACITY_ALPHA * layer.opacity
      local y = layer.index * (config.layer_height_spacing or 12.0)
      local cursor = 0

      for x = grid_start, grid_end - 1, layer_step do
        local x0 = x * layer.scale
        local x1 = (x + layer_step) * layer.scale
        local u0 = x * TEXTURE_SCALE + base_u
        local u1 = (x + layer_step) * TEXTURE_SCALE + base_u
        for z = grid_start, grid_end - 1, layer_step do
          local z0 = z * layer.scale
          local z1 = (z + layer_step) * layer.scale
          local v0 = z * TEXTURE_SCALE + base_v
          local v1 = (z + layer_step) * TEXTURE_SCALE + base_v
          cursor = append_vertex(cursor, x0, y, z1, u0, v1, red, green, blue, alpha)
          cursor = append_vertex(cursor, x1, y, z1, u1, v1, red, green, blue, alpha)
          cursor = append_vertex(cursor, x1, y, z0, u1, v0, red, green, blue, alpha)
          cursor = append_vertex(cursor, x0, y, z0, u0, v0, red, green, blue, alpha)
        end
      end

      for index = #cloud_vertices, cursor + 1, -1 do
        cloud_vertices[index] = nil
      end

      minecraft.render.quads({
        texture = "/environment/clouds.png",
        layer = "clouds",
        x = eye_x,
        y = cloud_height,
        z = eye_z,
        world_space = true,
        blend = true,
        cull = false,
        depth_test = true,
        depth_write = false,
        packed = cloud_vertices,
      })
    end
  end

  return event
end)

minecraft.log("info", "Layered Clouds mod loaded")
