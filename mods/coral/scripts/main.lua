local CORAL_ID = 180
local WATER_ID = minecraft.world.block_id("water") or 0
local SAND_ID = minecraft.world.block_id("sand") or 0
local DIRT_ID = minecraft.world.block_id("dirt") or 0

minecraft.register_block({
  id = CORAL_ID,
  texture = "mods/coral/coral.png",
  hardness = 1.5,
  resistance = 10.0,
  translation_key = "coral",
  material = "stone",
  model = {
    type = "full_cube",
    opaque = false,
    full_cube = false,
    requires_solid_below = false,
    varied_bounds = true,
    coordinate_color = true,
    bounds_padding = 0.0625,
    bounds_offset = 0.1,
    min_scale = 0.9,
    max_scale = 1.1,
  },
})

local function in_chunk(x, z)
  return x >= 0 and x < 16 and z >= 0 and z < 16
end

local function get_block(x, y, z)
  if not in_chunk(x, z) or y < 0 or y >= 128 then
    return 0
  end
  return minecraft.chunk.get_block(x, y, z)
end

local function set_block(x, y, z, id)
  if in_chunk(x, z) and y >= 0 and y < 128 then
    minecraft.chunk.set_block(x, y, z, id)
  end
end

local function is_water(id)
  return id == WATER_ID
end

local function is_large_water_body(x, y, z)
  if y <= 4 or not is_water(get_block(x, y, z)) then
    return false
  end
  if get_block(x, y - 4, z) == 0 then
    return false
  end
  local radius = 6
  return is_water(get_block(x + radius, y, z))
    and is_water(get_block(x - radius, y, z))
    and is_water(get_block(x, y, z + radius))
    and is_water(get_block(x, y, z - radius))
end

local function generate_reef(center_x, center_y, center_z)
  local reef_size = 30 + minecraft.world.random(30)
  for _ = 1, reef_size do
    local x = center_x + minecraft.world.random(8) - minecraft.world.random(8)
    local y = center_y + minecraft.world.random(4) - minecraft.world.random(4)
    local z = center_z + minecraft.world.random(8) - minecraft.world.random(8)
    if is_water(get_block(x, y, z)) then
      local block_below = get_block(x, y - 1, z)
      if block_below == SAND_ID or block_below == DIRT_ID then
        set_block(x, y, z, CORAL_ID)
      end
    end
  end
end

minecraft.on(minecraft.events.chunk_generation, {
  stage = minecraft.generation.stages.features,
  moment = minecraft.generation.moments.after,
  vanilla_stage_ran = true,
  when = minecraft.util.real_world,
  priority = 100,
}, function()
  if WATER_ID <= 0 or SAND_ID <= 0 or DIRT_ID <= 0 then
    return
  end
  if minecraft.world.random(16) ~= 0 then
    return
  end

  local x = 6 + minecraft.world.random(4)
  local z = 6 + minecraft.world.random(4)
  local y = 127
  while y > 0 and get_block(x, y, z) == 0 do
    y = y - 1
  end
  if is_large_water_body(x, y, z) then
    generate_reef(x, y, z)
  end
end)
