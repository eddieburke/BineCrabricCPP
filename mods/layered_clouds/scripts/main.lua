local config = require("config")

local TWO_PI = math.pi * 2
local WIND_DIRECTION_MIN = 0.5
local WIND_DIRECTION_MAX = 2.5
local MOVEMENT_SCALE = 0.02
local INTERP_DURATION = 50000
local INCREMENT_SCALE = 0.02
local SKEW_STRENGTH = 0.00025
local REHASH_INTERVAL_MIN = 3000
local REHASH_INTERVAL_MAX = 7000
local BASE_RADIUS = 400

local cloud_ticks = 0.0
local layers = {}
local layer_signature = nil
local cloud_vertices = {}
local world_seed = 0
local last_rehash_time = 0
local next_rehash_interval = 0
local base_dir_x = 1.0
local base_dir_z = 0.0
local base_speed = 1.0

local function hash_float_1(seed)
  local hash = seed % 0x7fffffff
  hash = (hash * 0x9e3779b9) % 0xffffffff
  if hash < 0 then hash = hash + 0xffffffff end
  return (hash % 0x7fffffff) / 0x7fffffff
end

local function hash_float_2(seed)
  local hash = seed % 0x7fffffff
  hash = (hash * 0x85ebca6b) % 0xffffffff
  if hash < 0 then hash = hash + 0xffffffff end
  return (hash % 0x7fffffff) / 0x7fffffff
end

local function compute_wind_trend(seed, wind_speed)
  local wind_dir_range = WIND_DIRECTION_MAX - WIND_DIRECTION_MIN
  local wind_speed_min = 0.1 * wind_speed
  local wind_speed_max = 4.0 * wind_speed
  local wind_speed_range = wind_speed_max - wind_speed_min

  local trend_seed = seed + 99999
  local trend_angle = hash_float_1(trend_seed) * TWO_PI
  local trend_dir_speed = WIND_DIRECTION_MIN + hash_float_2(trend_seed) * wind_dir_range
  local trend_dir_x = math.cos(trend_angle) * trend_dir_speed
  local trend_dir_z = math.sin(trend_angle) * trend_dir_speed
  local trend_speed = wind_speed_min + hash_float_1(trend_seed + 1) * wind_speed_range

  local mag = math.sqrt(trend_dir_x * trend_dir_x + trend_dir_z * trend_dir_z)
  local dir_x, dir_z
  if mag > 0.0001 then
    dir_x = trend_dir_x / mag
    dir_z = trend_dir_z / mag
  else
    dir_x = 1.0
    dir_z = 0.0
  end
  return dir_x, dir_z, trend_speed
end

local function build_layers()
  layers = {}
  local base_height = 128
  local layer_count = config.layer_count or 7
  local layer_spacing = config.layer_height_spacing or 12
  local wind_speed = config.wind_speed or 1.0

  base_dir_x, base_dir_z, base_speed = compute_wind_trend(world_seed, wind_speed)

  local current_time = os.time() * 1000

  for i = 1, layer_count do
    local layer_seed = i

    local layer_seed_mix_4 = layer_seed * 98765
    local time_mix_4 = (current_time / 100) + layer_seed * 313
    local seed_scale = world_seed + layer_seed_mix_4 + time_mix_4
    local random_scale = hash_float_1(seed_scale)
    local biased_scale = 1.0 - (1.0 - random_scale) * (1.0 - random_scale)
    local min_scale = 0.5 * (config.cloud_scale or 1.0)
    local max_scale = 1.5 * (config.cloud_scale or 1.0)
    local layer_scale = min_scale + biased_scale * (max_scale - min_scale)

    local layer_seed_mix_2 = layer_seed * 111111
    local time_mix_2 = (current_time / 137) + layer_seed * 199
    local seed_skew = world_seed + layer_seed_mix_2 + time_mix_2
    local skew_angle = hash_float_1(seed_skew) * TWO_PI
    local skew_mag = hash_float_2(seed_skew) * SKEW_STRENGTH
    local skew_x = math.cos(skew_angle) * skew_mag
    local skew_z = math.sin(skew_angle) * skew_mag

    local layer_dir_x_base = base_dir_x + skew_x
    local layer_dir_z_base = base_dir_z + skew_z
    local layer_dir_mag = math.sqrt(layer_dir_x_base * layer_dir_x_base + layer_dir_z_base * layer_dir_z_base)
    local layer_start_dir_x, layer_start_dir_z
    if layer_dir_mag > 0.0001 then
      layer_start_dir_x = layer_dir_x_base / layer_dir_mag
      layer_start_dir_z = layer_dir_z_base / layer_dir_mag
    else
      layer_start_dir_x = base_dir_x
      layer_start_dir_z = base_dir_z
    end

    local layer_seed_mix_5 = layer_seed * 54321
    local time_mix_5 = (current_time / 137) + layer_seed * 401
    local seed_increment = world_seed + layer_seed_mix_5 + time_mix_5
    local random_increment = hash_float_2(seed_increment)
    local increment_scale = INCREMENT_SCALE * (0.5 + random_increment)

    local progress = (i - 1) / math.max(1, layer_count - 1)
    local opacity_min = 0.5 * (config.base_opacity or 0.7)
    local opacity_max = 1.0 * (config.base_opacity or 0.7)
    local layer_opacity = opacity_min + (opacity_max - opacity_min) * (1.0 - progress * 0.5)

    layers[i] = {
      height = base_height + (i - 1) * layer_spacing,
      opacity = layer_opacity,
      scale = layer_scale,
      dir_x = layer_start_dir_x,
      dir_z = layer_start_dir_z,
      start_dir_x = layer_start_dir_x,
      start_dir_z = layer_start_dir_z,
      target_dir_x = layer_start_dir_x,
      target_dir_z = layer_start_dir_z,
      speed = base_speed,
      skew_x = skew_x,
      skew_z = skew_z,
      interp_progress = 0.0,
      interp_start_time = current_time,
      increment_scale = increment_scale,
    }
  end

  layer_signature = string.format("%d_%.2f_%.2f_%.2f", layer_count, layer_spacing, config.base_opacity or 0.7, config.cloud_scale or 1.0)
end

build_layers()
last_rehash_time = os.time() * 1000
next_rehash_interval = REHASH_INTERVAL_MIN + math.random() * (REHASH_INTERVAL_MAX - REHASH_INTERVAL_MIN)

local function update_layer_interp(layer, current_time)
  local elapsed = current_time - layer.interp_start_time
  if elapsed < 0 then elapsed = 0 end

  local progress = elapsed / INTERP_DURATION
  if progress >= 1.0 then
    layer.start_dir_x = layer.dir_x
    layer.start_dir_z = layer.dir_z

    local seed_increment = world_seed + 12345 * 54321
    local random_increment = hash_float_2(seed_increment)
    layer.increment_scale = INCREMENT_SCALE * (0.5 + random_increment)

    local ideal_dir_x = base_dir_x + layer.skew_x
    local ideal_dir_z = base_dir_z + layer.skew_z
    local ideal_mag = math.sqrt(ideal_dir_x * ideal_dir_x + ideal_dir_z * ideal_dir_z)
    if ideal_mag > 0.0001 then
      ideal_dir_x = ideal_dir_x / ideal_mag
      ideal_dir_z = ideal_dir_z / ideal_mag
    else
      ideal_dir_x = base_dir_x
      ideal_dir_z = base_dir_z
    end

    local dir_to_ideal_x = ideal_dir_x - layer.dir_x
    local dir_to_ideal_z = ideal_dir_z - layer.dir_z
    layer.target_dir_x = layer.dir_x + dir_to_ideal_x * layer.increment_scale
    layer.target_dir_z = layer.dir_z + dir_to_ideal_z * layer.increment_scale

    local target_dir_mag = math.sqrt(layer.target_dir_x * layer.target_dir_x + layer.target_dir_z * layer.target_dir_z)
    if target_dir_mag > 0.0001 then
      layer.target_dir_x = layer.target_dir_x / target_dir_mag
      layer.target_dir_z = layer.target_dir_z / target_dir_mag
    else
      layer.target_dir_x = ideal_dir_x
      layer.target_dir_z = ideal_dir_z
    end

    local overshoot = elapsed - INTERP_DURATION
    layer.interp_start_time = current_time - overshoot
    progress = overshoot / INTERP_DURATION
  end

  progress = math.min(progress, 1.0)

  local interp_dir_x = layer.start_dir_x + (layer.target_dir_x - layer.start_dir_x) * progress
  local interp_dir_z = layer.start_dir_z + (layer.target_dir_z - layer.start_dir_z) * progress
  local interp_dir_mag = math.sqrt(interp_dir_x * interp_dir_x + interp_dir_z * interp_dir_z)

  if interp_dir_mag > 0.0001 then
    layer.dir_x = interp_dir_x / interp_dir_mag
    layer.dir_z = interp_dir_z / interp_dir_mag
  else
    local base_mag = math.sqrt(base_dir_x * base_dir_x + base_dir_z * base_dir_z)
    if base_mag > 0.0001 then
      layer.dir_x = base_dir_x / base_mag
      layer.dir_z = base_dir_z / base_mag
    else
      layer.dir_x = layer.start_dir_x
      layer.dir_z = layer.start_dir_z
    end
  end

  layer.speed = base_speed
  layer.interp_progress = progress
end

minecraft.on("client_tick", { after_world = true }, function(event)
  if event.paused then return event end
  local dt = 0.05
  cloud_ticks = cloud_ticks + dt * (config.wind_speed or 1.0)

  local new_sig = string.format("%d_%.2f_%.2f_%.2f", config.layer_count or 7, config.layer_height_spacing or 12, config.base_opacity or 0.7, config.cloud_scale or 1.0)
  if new_sig ~= layer_signature then
    build_layers()
  end

  local current_time = os.time() * 1000
  if current_time - last_rehash_time >= next_rehash_interval then
    world_seed = (world_seed + current_time) % 0x7fffffff
    base_dir_x, base_dir_z, base_speed = compute_wind_trend(world_seed, config.wind_speed or 1.0)

    last_rehash_time = current_time
    next_rehash_interval = REHASH_INTERVAL_MIN + math.random() * (REHASH_INTERVAL_MAX - REHASH_INTERVAL_MIN)
  end

  for _, layer in ipairs(layers) do
    update_layer_interp(layer, current_time)
  end

  return event
end)

minecraft.on("world_render", {
  stage = "clouds",
  moment = "before",
  priority = 100,
}, function(event)
  if not config.enabled then
    return event
  end
  event.cancel_vanilla = true

  local camera_x = event.camera_x or 0
  local camera_z = event.camera_z or 0
  local cloud_base_height = event.cloud_base_height or (128 - (event.camera_y or 0) + 0.33)
  local cloud_scale = config.cloud_scale or 1.0
  local cloud_offset_float = cloud_ticks * MOVEMENT_SCALE

  for i, layer in ipairs(layers) do
    if layer.opacity < 0.01 then
      goto continue
    end

    local y = cloud_base_height + (layer.height - 128)
    local alpha = layer.opacity
    local layer_scale = layer.scale or 1.0
    local dir_x = layer.dir_x
    local dir_z = layer.dir_z
    local layer_speed = layer.speed

    local offset_x = cloud_offset_float * layer_speed * dir_x
    local offset_z = cloud_offset_float * layer_speed * dir_z

    local radius = BASE_RADIUS * cloud_scale * layer_scale
    local vertex_count = 0

    local grid_step = 32
    if i > 5 then
      grid_step = 64
    elseif i > 2 then
      grid_step = 48
    end

    for x = -radius, radius, grid_step do
      for z = -radius, radius, grid_step do
        local x0 = x + offset_x
        local x1 = x0 + grid_step
        local z0 = z + offset_z
        local z1 = z0 + grid_step

        cloud_vertices[vertex_count + 1] = x0
        cloud_vertices[vertex_count + 2] = y
        cloud_vertices[vertex_count + 3] = z1
        cloud_vertices[vertex_count + 4] = 0
        cloud_vertices[vertex_count + 5] = 0
        cloud_vertices[vertex_count + 6] = 1
        cloud_vertices[vertex_count + 7] = 1
        cloud_vertices[vertex_count + 8] = 1
        cloud_vertices[vertex_count + 9] = alpha

        cloud_vertices[vertex_count + 10] = x1
        cloud_vertices[vertex_count + 11] = y
        cloud_vertices[vertex_count + 12] = z1
        cloud_vertices[vertex_count + 13] = 0
        cloud_vertices[vertex_count + 14] = 0
        cloud_vertices[vertex_count + 15] = 1
        cloud_vertices[vertex_count + 16] = 1
        cloud_vertices[vertex_count + 17] = 1
        cloud_vertices[vertex_count + 18] = alpha

        cloud_vertices[vertex_count + 19] = x1
        cloud_vertices[vertex_count + 20] = y
        cloud_vertices[vertex_count + 21] = z0
        cloud_vertices[vertex_count + 22] = 0
        cloud_vertices[vertex_count + 23] = 0
        cloud_vertices[vertex_count + 24] = 1
        cloud_vertices[vertex_count + 25] = 1
        cloud_vertices[vertex_count + 26] = 1
        cloud_vertices[vertex_count + 27] = alpha

        cloud_vertices[vertex_count + 28] = x0
        cloud_vertices[vertex_count + 29] = y
        cloud_vertices[vertex_count + 30] = z0
        cloud_vertices[vertex_count + 31] = 0
        cloud_vertices[vertex_count + 32] = 0
        cloud_vertices[vertex_count + 33] = 1
        cloud_vertices[vertex_count + 34] = 1
        cloud_vertices[vertex_count + 35] = 1
        cloud_vertices[vertex_count + 36] = alpha

        vertex_count = vertex_count + 36
      end
    end

    for index = #cloud_vertices, vertex_count + 1, -1 do
      cloud_vertices[index] = nil
    end

    minecraft.render.quads({
      texture = "/environment/clouds.png",
      blend = true,
      cull = false,
      depth_test = true,
      depth_write = false,
      packed = cloud_vertices,
    })

    ::continue::
  end

  return event
end)

minecraft.log("info", "Layered Clouds mod loaded")
