-- ================================================================
-- MOD: layered_clouds
-- Standardized structure with separated config
-- ================================================================

local config = require("layered_clouds.config")

-- Layered Clouds: Multi-layer volumetric cloud rendering
-- Refactored with separation of concerns

local config_module = require("layered_clouds.config")

-- Module state
local config = {}
local cloud_ticks = 0.0
local layers = {}
local layer_signature = nil

local function fancy_clouds_enabled()
  return minecraft.options.get("clouds") == config_module.NATIVE_CLOUDS_FANCIER
end

local function set_fancy_clouds(want)
  local current = minecraft.options.get("clouds") or 0
  local desired = want and config_module.NATIVE_CLOUDS_FANCIER or 0
  if current ~= desired then
    minecraft.options.cycle("clouds", (desired - current + 4) % 4)
  end
end

-- Load configuration
config, _ = config_module.load_config()

-- Initialize layers
local function build_layers()
  layers = {}
  local base_height = 128
  for i = 1, config.layer_count do
    layers[i] = {
      height = base_height + (i - 1) * config.layer_height_spacing,
      opacity = config.base_opacity * (1.0 - (i - 1) / config.layer_count),
      offset = math.random() * 100,
    }
  end
  layer_signature = string.format("%d_%d_%.2f", config.layer_count, base_height, config.layer_height_spacing)
end

build_layers()

-- Config change handler
minecraft.event.register("config_changed", function(key)
  if key == "clouds" then
    local enabled = fancy_clouds_enabled()
    if not enabled then
      set_fancy_clouds(true)
    end
  end
end)

-- Tick handler
minecraft.event.register("tick", function(dt)
  cloud_ticks = cloud_ticks + dt * config.wind_speed
  
  -- Rebuild layers if config changed
  local new_sig = string.format("%d_%d_%.2f", config.layer_count, 128, config.layer_height_spacing)
  if new_sig ~= layer_signature then
    build_layers()
  end
end)

-- Render handler
minecraft.event.register("render_clouds", function(camera, tick_delta)
  if not fancy_clouds_enabled() then
    set_fancy_clouds(true)
    return
  end
  
  local time = cloud_ticks + (tick_delta or 0) * config.wind_speed
  
  for _, layer in ipairs(layers) do
    local y = layer.height
    local alpha = layer.opacity
    
    minecraft.render.push()
    minecraft.render.translate(0, y, 0)
    
    -- Simple cloud quad rendering (would be expanded with noise in full implementation)
    local size = 64 * config.cloud_scale
    local x_offset = math.sin(time * 0.01 + layer.offset) * 20
    local z_offset = math.cos(time * 0.015 + layer.offset) * 20
    
    minecraft.render.begin("quads")
    minecraft.render.color(1.0, 1.0, 1.0, alpha)
    minecraft.render.vertex(-size + x_offset, 0, -size + z_offset)
    minecraft.render.vertex(size + x_offset, 0, -size + z_offset)
    minecraft.render.vertex(size + x_offset, 0, size + z_offset)
    minecraft.render.vertex(-size + x_offset, 0, size + z_offset)
    minecraft.render.end()
    
    minecraft.render.pop()
  end
end)

-- Save config on unload
minecraft.event.register("unload", function()
  config_module.save_config(config)
end)

minecraft.log("info", "Layered Clouds mod loaded (refactored)")
