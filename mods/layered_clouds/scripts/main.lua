local SCREEN_ID = "layered_clouds:settings"
local CONFIG_FILE = "mod_LayeredClouds.cfg"
local KEY_CLOUD_SETTINGS = minecraft.key_code("k")

local CONFIG_DEFAULTS = {
  enabled = true,
  layer_count = 6,
  base_opacity = 0.7,
  cloud_scale = 1.0,
  layer_height_spacing = 12.0,
  wind_speed = 1.0,
}

local CONFIG_KEYS = {
  "enabled",
  "layer_count",
  "base_opacity",
  "cloud_scale",
  "layer_height_spacing",
  "wind_speed",
}

local CONFIG_NAMES = {
  layer_count = "layerCount",
  base_opacity = "baseOpacity",
  cloud_scale = "cloudScale",
  layer_height_spacing = "layerHeightSpacing",
  wind_speed = "windSpeedMultiplier",
}

local CONFIG_ALIASES = {}
for internal_name, file_name in pairs(CONFIG_NAMES) do
  CONFIG_ALIASES[file_name] = internal_name
end

local config = minecraft.util.copy(CONFIG_DEFAULTS)

local cloud_ticks = 0.0
local layers = {}
local layer_signature = nil

local clamp = minecraft.util.clamp

local function clamp_config()
  config.layer_count = math.floor(clamp(config.layer_count, 1, 12))
  config.base_opacity = clamp(config.base_opacity, 0.0, 1.0)
  config.cloud_scale = clamp(config.cloud_scale, 0.5, 2.0)
  config.layer_height_spacing = clamp(config.layer_height_spacing, 4.0, 32.0)
  config.wind_speed = clamp(config.wind_speed, 0.0, 5.0)
end

local function reset_config()
  config = minecraft.util.copy(CONFIG_DEFAULTS)
  clamp_config()
end

local function save_config()
  clamp_config()
  minecraft.config.save(CONFIG_FILE, config, {
    keys = CONFIG_KEYS,
    names = CONFIG_NAMES,
  })
end

local function load_config()
  local found
  config, found = minecraft.config.load(CONFIG_FILE, CONFIG_DEFAULTS, {
    aliases = CONFIG_ALIASES,
  })
  clamp_config()
  if not found then
    save_config()
  end
end

local function mod_active()
  return config.enabled
end

load_config()

local open_settings = minecraft.screen.settings({
  id = SCREEN_ID,
  title = "Cloud Settings",
  parent_screen = minecraft.screen.ids.detail_settings,
  parent_region = minecraft.screen.regions.footer,
  button_label = "Cloud Settings...",
  values = function() return config end,
  sliders = {
    { key = "layer_count", label = "Layers", min = 1, max = 12, integer = true },
    { key = "base_opacity", min = 0, max = 1,
      format = function(v) return "Opacity: " .. math.floor(v * 100) .. "%" end },
    { key = "cloud_scale", min = 0.5, max = 2,
      format = function(v) return string.format("Scale: %.2fx", v) end },
    { key = "layer_height_spacing", min = 4, max = 32,
      format = function(v) return "Spacing: " .. math.floor(v + 0.5) .. " blocks" end },
    { key = "wind_speed", min = 0, max = 5,
      format = function(v) return string.format("Wind Speed: %.1fx", v) end },
  },
  toggles = {
    { key = "enabled", label = "Enabled" },
  },
  on_change = clamp_config,
  on_reset = reset_config,
  on_save = save_config,
})

minecraft.on(minecraft.events.client_tick, {
  before = false,
  paused = false,
  priority = 100,
}, function()
  if mod_active() then
    cloud_ticks = cloud_ticks + 1.0
  end
end)

local CLOUD_TILE_SIZE = 32
local CLOUD_UV_SCALE = 0.00048828125
local TWO_PI = math.pi * 2.0
local TEXTURE_OFFSET_RANGE = 1000.0
local MOVEMENT_PER_TICK = 0.000006

local function hash_string(value)
  local hash = 2166136261
  for index = 1, #value do
    hash = (hash ~ string.byte(value, index)) * 16777619
  end
  return hash & 0x7fffffff
end

local function hash_float(seed)
  local hash = seed & 0x7fffffff
  hash = (hash ~ (hash >> 16)) * 0x45d9f3b
  hash = (hash ~ (hash >> 16)) * 0x45d9f3b
  hash = hash ~ (hash >> 16)
  return (hash & 0x7fffffff) / 0x7fffffff
end

local function rebuild_layers(world_name)
  local signature = table.concat({
    world_name or "",
    config.layer_count,
    string.format("%.3f", config.base_opacity),
    string.format("%.3f", config.cloud_scale),
    string.format("%.3f", config.wind_speed),
  }, ":")
  if signature == layer_signature then
    return
  end

  layer_signature = signature
  layers = {}
  local seed = hash_string(world_name or "")
  local trend_angle = hash_float(seed + 99999) * TWO_PI
  local base_dir_x = math.cos(trend_angle)
  local base_dir_z = math.sin(trend_angle)
  local base_speed = (0.1 + hash_float(seed + 100000) * 3.9) * config.wind_speed
  local progress_divisor = math.max(1, config.layer_count - 1)

  for index = 0, config.layer_count - 1 do
    local random_scale = hash_float(seed ~ (index * 98765 + 313))
    local biased_scale = 1.0 - (1.0 - random_scale) ^ 2
    local skew_angle = hash_float(seed ~ (index * 111111 + 199)) * TWO_PI
    local skew = hash_float(seed ~ (index * 111111 + 200)) * 0.00025
    local dir_x = base_dir_x + math.cos(skew_angle) * skew
    local dir_z = base_dir_z + math.sin(skew_angle) * skew
    local magnitude = math.sqrt(dir_x * dir_x + dir_z * dir_z)
    local gray_random = hash_float(seed + index * 77777)
    local progress = index / progress_divisor
    layers[index + 1] = {
      dir_x = dir_x / magnitude,
      dir_z = dir_z / magnitude,
      speed = base_speed,
      scale = (0.5 + biased_scale) * config.cloud_scale,
      opacity = config.base_opacity * (1.0 - progress * 0.25),
      gray = 0.6 + gray_random * 0.4,
      use_gray = gray_random > 0.5,
      texture_x = (hash_float(seed ~ (index * 11111 + 137)) - 0.5) * TEXTURE_OFFSET_RANGE,
      texture_z = (hash_float(seed ~ (index * 11111 + 138)) - 0.5) * TEXTURE_OFFSET_RANGE,
    }
  end
end

local function draw_cloud_layer(event, layer_index, height, color_r, color_g, color_b)
  local layer = layers[layer_index + 1]
  local vertices = {}
  local function vertex(x, y, z, u, v)
    vertices[#vertices + 1] = { x = x, y = y, z = z, u = u, v = v }
  end
  local grid_step = layer_index > 5 and 64 or (layer_index > 2 and 48 or 32)
  local radius = 4 * CLOUD_TILE_SIZE
  local camera_x = event.camera_x or 0.0
  local camera_z = event.camera_z or 0.0
  local movement = (cloud_ticks + (event.tick_delta or 0.0)) * MOVEMENT_PER_TICK * layer.speed
  local sample_x = (camera_x + movement * layer.dir_x) / layer.scale
  local sample_z = (camera_z + movement * layer.dir_z) / layer.scale
  sample_x = sample_x - math.floor(sample_x / 2048.0) * 2048.0
  sample_z = sample_z - math.floor(sample_z / 2048.0) * 2048.0
  local tex_x = (sample_x + layer.texture_x) * CLOUD_UV_SCALE
  local tex_z = (sample_z + layer.texture_z) * CLOUD_UV_SCALE

  for x = -radius, radius - 1, grid_step do
    for z = -radius, radius - 1, grid_step do
      local x1 = x * layer.scale
      local z1 = z * layer.scale
      local x2 = (x + grid_step) * layer.scale
      local z2 = (z + grid_step) * layer.scale
      local u1 = x * CLOUD_UV_SCALE + tex_x
      local v1 = z * CLOUD_UV_SCALE + tex_z
      local u2 = (x + grid_step) * CLOUD_UV_SCALE + tex_x
      local v2 = (z + grid_step) * CLOUD_UV_SCALE + tex_z
      vertex(x1, height, z1, u1, v1)
      vertex(x1, height, z2, u1, v2)
      vertex(x2, height, z2, u2, v2)
      vertex(x2, height, z1, u2, v1)
    end
  end
  minecraft.render.quads({
    texture = "/environment/clouds.png",
    vertices = vertices,
    r = layer.use_gray and layer.gray or color_r,
    g = layer.use_gray and layer.gray or color_g,
    b = layer.use_gray and layer.gray or color_b,
    a = 0.8 * layer.opacity,
    blend = true,
    cull = false,
    depth_test = true,
    depth_write = false,
  })
end

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.clouds,
  moment = minecraft.render.moments.before,
  priority = 100,
}, function(event)
  if not mod_active() or not event.is_overworld then
    return event
  end
  event.cancel_vanilla = true
  rebuild_layers(event.world_name)
  local base_height = event.cloud_base_height or 128.0
  local celestial = event.celestial or 0.0
  local brightness = clamp(math.cos(celestial * TWO_PI) * 2.0 + 0.5, 0.0, 1.0)
  local color_r = brightness * 0.9 + 0.1
  local color_g = brightness * 0.9 + 0.1
  local color_b = brightness * 0.85 + 0.15
  for layer = 0, config.layer_count - 1 do
    local height = base_height + layer * config.layer_height_spacing
    draw_cloud_layer(event, layer, height, color_r, color_g, color_b)
  end
  return event
end)

minecraft.on(minecraft.events.key_press, {
  key = KEY_CLOUD_SETTINGS,
  pressed = true,
  handled = false,
  priority = 100,
}, function(event)
  open_settings()
  event.handled = true
end)
