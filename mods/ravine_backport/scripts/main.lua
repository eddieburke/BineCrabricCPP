-- ================================================================
-- MOD: ravine_backport
-- Standardized structure with separated config
-- ================================================================

local config = require("ravine_backport.config")

local AIR = 0
local current_chunk = nil

local function chunk_coord(value)
  return value % 16
end

local function carve_column(x, z, top_y, depth)
  if not current_chunk then return end
  for y = top_y, top_y - depth, -1 do
    if y < 1 then
      break
    end
    current_chunk:set_block(x, y, z, AIR)
  end
end

minecraft.on(minecraft.events.chunk_generation, {
  stage = minecraft.generation.stages.carver,
  moment = minecraft.generation.moments.after,
  when = minecraft.util.real_world,
}, function(event)
  current_chunk = event.chunk
  local seed = math.floor(event.world_seed or 0)
  local chunk_x = event.chunk_x or 0
  local chunk_z = event.chunk_z or 0
  if (seed + chunk_x * 7342871 + chunk_z * 912931) % 35 ~= 0 then
    current_chunk = nil
    return
  end
  local base_x = chunk_x * 16 + 8
  local base_z = chunk_z * 16 + 8
  for step = 0, 12 do
    local world_x = base_x + step * 2
    local world_z = base_z + math.floor(math.sin(step * 0.7) * 3.0)
    local x = chunk_coord(world_x)
    local z = chunk_coord(world_z)
    local top = current_chunk:get_height(x, z)
    if top > 8 then
      carve_column(x, z, top - 2, 10 + step)
    end
  end
  current_chunk = nil
end)
