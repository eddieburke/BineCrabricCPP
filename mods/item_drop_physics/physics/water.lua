-- Water Physics Module
-- Handles fluid simulation and buoyancy for item drop physics

local core = require("lib.core.init")

local water = {}

--------------------------------------------------------------------------------
-- CONSTANTS
--------------------------------------------------------------------------------

local WATER_EPSILON = 1e-5
local WATER_CURRENT_SPEED = 0.09
local WATER_HORIZONTAL_FLOW_SIGN = -1.0
local WATER_SMALL_BODY_LIMIT = 0.26
local WATER_FLOW_TORQUE = 0.10

water.WATER_IDS = { [8] = true, [9] = true }
water.FLOWING_WATER_IDS = { [8] = true }
water.SOURCE_WATER_IDS = { [9] = true }

--------------------------------------------------------------------------------
-- WATER ALIAS REGISTRATION
--------------------------------------------------------------------------------

function water.add_alias(set, name)
  local id = minetest.registered_nodes[name] and minetest.get_content_id(name)
  if id and id > 0 then
    set[id] = true
    water.WATER_IDS[id] = true
  end
end

function water.init_aliases()
  water.add_alias(water.FLOWING_WATER_IDS, "flowing_water")
  water.add_alias(water.FLOWING_WATER_IDS, "minecraft:flowing_water")
  water.add_alias(water.FLOWING_WATER_IDS, "water_flowing")
  water.add_alias(water.SOURCE_WATER_IDS, "still_water")
  water.add_alias(water.SOURCE_WATER_IDS, "stationary_water")
  water.add_alias(water.SOURCE_WATER_IDS, "minecraft:water")
  water.add_alias(water.SOURCE_WATER_IDS, "water")
  
  -- Ensure only ID 9 is in source_water_ids if it's also in flowing
  for id in pairs(water.FLOWING_WATER_IDS) do
    if id ~= 9 then water.SOURCE_WATER_IDS[id] = nil end
  end
end

--------------------------------------------------------------------------------
-- FLUID CELL CACHING
--------------------------------------------------------------------------------

local fluid_cell_cache = {}

function water.fluid_hash(x, y, z)
  return x * 73856093 + y * 19349663 + z * 83492791
end

function water.get_block_meta(x, y, z)
  local node = minetest.get_node({x=x, y=y, z=z})
  if node then
    return node.param2 or 0
  end
  return 0
end

function water.fluid_height_from_meta(meta)
  if meta == nil or meta >= 8 then return 1.0 / 9.0 end
  return (meta + 1) / 9.0
end

water.FLOW_DIRS = {
  { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 },
}

function water.normalize_flow(fx, fy, fz)
  local ls = fx * fx + fy * fy + fz * fz
  if ls <= WATER_EPSILON * WATER_EPSILON then return 0.0, 0.0, 0.0 end
  local inv = 1.0 / math.sqrt(ls)
  return fx * inv, fy * inv, fz * inv
end

function water.fluid_cell(x, y, z, get_block_func)
  local hash = water.fluid_hash(x, y, z)
  local cached = fluid_cell_cache[hash]
  
  if cached then
    if cached.x ~= nil then
      if cached.x == x and cached.y == y and cached.z == z then return cached end
    else
      if cached.id == water.empty_id then return cached end
    end
  end
  
  local block_id = get_block_func(x, y, z)
  local result
  
  if not water.WATER_IDS[block_id] then
    result = { x = x, y = y, z = z, id = block_id, height = 0.0 }
    fluid_cell_cache[hash] = result
    return result
  end
  
  local meta = water.get_block_meta(x, y, z)
  local height = water.fluid_height_from_meta(meta)
  local is_source = water.SOURCE_WATER_IDS[block_id] == true
  local is_flowing = water.FLOWING_WATER_IDS[block_id] == true
  
  local flow_x, flow_y, flow_z = 0.0, 0.0, 0.0
  
  if is_flowing and meta ~= nil and meta < 8 then
    local dir_index = (meta % 4) + 1
    local dir = water.FLOW_DIRS[dir_index]
    flow_x = dir[1] * WATER_HORIZONTAL_FLOW_SIGN
    flow_z = dir[2] * WATER_HORIZONTAL_FLOW_SIGN
  end
  
  result = {
    x = x,
    y = y,
    z = z,
    id = block_id,
    height = height,
    is_source = is_source,
    is_flowing = is_flowing,
    flow_x = flow_x,
    flow_y = 0.0,
    flow_z = flow_z
  }
  
  fluid_cell_cache[hash] = result
  return result
end

function water.clear_fluid_cache()
  fluid_cell_cache = {}
end

--------------------------------------------------------------------------------
-- BUOYANCY CALCULATION
--------------------------------------------------------------------------------

function water.calculate_buoyancy(body, physics_props, substep_dt)
  local center_y = body.position.y
  local half_height = body.half_extents.y
  local water_level = water.find_water_level(body.position.x, body.position.y, body.position.z)
  
  if water_level == nil then
    return { buoyancy = 0, drag = 0, angular_damping = 0 }
  end
  
  local submerged_depth = math.max(0, math.min(half_height * 2, water_level - (center_y - half_height)))
  local submerged_ratio = math.min(1.0, submerged_depth / (half_height * 2))
  
  -- Buoyancy force
  local displaced_volume = (body.half_extents.x * 2) * (body.half_extents.y * 2) * (body.half_extents.z * 2) * submerged_ratio
  local buoyancy_force = displaced_volume * 1.0 * 9.81 * physics_props.buoyancy_cap
  
  -- Drag forces
  local linear_drag = 1.0 - ((1.0 - physics_props.water_drag) * submerged_ratio)
  local vertical_drag = 1.0 - ((1.0 - physics_props.water_vertical_drag) * submerged_ratio)
  local angular_damping = physics_props.water_angular_damping * submerged_ratio
  
  return {
    buoyancy = buoyancy_force,
    linear_drag = linear_drag,
    vertical_drag = vertical_drag,
    angular_damping = angular_damping,
    submerged_ratio = submerged_ratio
  }
end

function water.find_water_level(x, y, z)
  -- Search downward for water surface
  local check_y = math.floor(y)
  local max_search = 10
  
  for i = 1, max_search do
    local cell = water.fluid_cell(math.floor(x), check_y, math.floor(z))
    if cell.id and water.WATER_IDS[cell.id] then
      if cell.height > 0 then
        return check_y + cell.height
      end
    end
    check_y = check_y - 1
  end
  
  return nil
end

--------------------------------------------------------------------------------
-- FLOW COUPLING
--------------------------------------------------------------------------------

function water.apply_flow_coupling(body, physics_props, dt)
  local cell = water.fluid_cell(math.floor(body.position.x), math.floor(body.position.y), math.floor(body.position.z))
  
  if not cell.is_flowing then return end
  
  local flow_magnitude = math.sqrt(cell.flow_x * cell.flow_x + cell.flow_z * cell.flow_z)
  if flow_magnitude < WATER_EPSILON then return end
  
  local coupling = physics_props.flow_coupling * WATER_CURRENT_SPEED
  local torque = WATER_FLOW_TORQUE * coupling * flow_magnitude
  
  -- Apply force in flow direction
  body.velocity.x = body.velocity.x + cell.flow_x * coupling * dt
  body.velocity.z = body.velocity.z + cell.flow_z * coupling * dt
  
  -- Apply torque for rotation
  body.angular_velocity.x = body.angular_velocity.x + cell.flow_z * torque * dt
  body.angular_velocity.z = body.angular_velocity.z - cell.flow_x * torque * dt
end

return water
