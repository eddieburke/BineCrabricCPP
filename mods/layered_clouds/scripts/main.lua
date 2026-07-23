local config = require("layered_clouds.config")

local cloud_ticks = 0.0
local layers = {}
local layer_signature = nil
local cloud_vertices = {}

local function build_layers()
  layers = {}
  local base_height = 128
  local layer_count = config.layer_count or 7
  local layer_spacing = config.layer_height_spacing or 12
  for i = 1, layer_count do
    layers[i] = {
      height = base_height + (i - 1) * layer_spacing,
      opacity = (config.base_opacity or 0.7) * (1.0 - (i - 1) / layer_count),
      offset = math.random() * 100,
    }
  end
  layer_signature = string.format("%d_%.2f", layer_count, layer_spacing)
end

build_layers()

minecraft.event.register("tick", function(dt)
  cloud_ticks = cloud_ticks + dt * (config.wind_speed or 1.0)
  
  local new_sig = string.format("%d_%.2f", config.layer_count or 7, config.layer_height_spacing or 12)
  if new_sig ~= layer_signature then
    build_layers()
  end
end)

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.clouds,
  moment = minecraft.render.moments.before,
  priority = 100,
}, function(event)
  if not config.enabled then
    return event
  end
  event.cancel_vanilla = true
  
  local time = cloud_ticks + (event.tick_delta or 0) * (config.wind_speed or 1.0)
  local camera_x = event.camera_x or 0
  local camera_z = event.camera_z or 0
  local cloud_base_height = event.cloud_base_height or (128 - (event.camera_y or 0) + 0.33)
  local cloud_scale = config.cloud_scale or 1.0
  local tile_size = 32 * cloud_scale
  local radius = 256 * cloud_scale
  local texture_scale = 0.00048828125
  
  for _, layer in ipairs(layers) do
    local y = cloud_base_height + (layer.height - 128)
    local alpha = layer.opacity
    local x_offset = math.sin(time * 0.01 + layer.offset) * 20
    local z_offset = math.cos(time * 0.015 + layer.offset) * 20
    local vertex_count = 0

    for x = -radius, radius - tile_size, tile_size do
      for z = -radius, radius - tile_size, tile_size do
        local x0 = x + x_offset
        local x1 = x0 + tile_size
        local z0 = z + z_offset
        local z1 = z0 + tile_size
        local u0 = (camera_x + x0 + time * 0.03) * texture_scale
        local u1 = (camera_x + x1 + time * 0.03) * texture_scale
        local v0 = (camera_z + z0) * texture_scale
        local v1 = (camera_z + z1) * texture_scale
        cloud_vertices[vertex_count + 1] = x0
        cloud_vertices[vertex_count + 2] = y
        cloud_vertices[vertex_count + 3] = z1
        cloud_vertices[vertex_count + 4] = u0
        cloud_vertices[vertex_count + 5] = v1
        cloud_vertices[vertex_count + 6] = 1
        cloud_vertices[vertex_count + 7] = 1
        cloud_vertices[vertex_count + 8] = 1
        cloud_vertices[vertex_count + 9] = alpha
        cloud_vertices[vertex_count + 10] = x1
        cloud_vertices[vertex_count + 11] = y
        cloud_vertices[vertex_count + 12] = z1
        cloud_vertices[vertex_count + 13] = u1
        cloud_vertices[vertex_count + 14] = v1
        cloud_vertices[vertex_count + 15] = 1
        cloud_vertices[vertex_count + 16] = 1
        cloud_vertices[vertex_count + 17] = 1
        cloud_vertices[vertex_count + 18] = alpha
        cloud_vertices[vertex_count + 19] = x1
        cloud_vertices[vertex_count + 20] = y
        cloud_vertices[vertex_count + 21] = z0
        cloud_vertices[vertex_count + 22] = u1
        cloud_vertices[vertex_count + 23] = v0
        cloud_vertices[vertex_count + 24] = 1
        cloud_vertices[vertex_count + 25] = 1
        cloud_vertices[vertex_count + 26] = 1
        cloud_vertices[vertex_count + 27] = alpha
        cloud_vertices[vertex_count + 28] = x0
        cloud_vertices[vertex_count + 29] = y
        cloud_vertices[vertex_count + 30] = z0
        cloud_vertices[vertex_count + 31] = u0
        cloud_vertices[vertex_count + 32] = v0
        cloud_vertices[vertex_count + 33] = 1
        cloud_vertices[vertex_count + 34] = 1
        cloud_vertices[vertex_count + 35] = 1
        cloud_vertices[vertex_count + 36] = alpha
        vertex_count = vertex_count + 36
      end
    end
    
    minecraft.render.quads({
      texture = "/environment/clouds.png",
      blend = true,
      cull = false,
      depth_test = true,
      depth_write = false,
      packed = cloud_vertices,
    })
  end
  return event
end)

minecraft.log("info", "Layered Clouds mod loaded (refactored)")
