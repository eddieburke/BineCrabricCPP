-- Item Renderer Module
-- Handles voxel rendering and orientation caching for item drops

local M = {}

--------------------------------------------------------------------------------
-- CONSTANTS
--------------------------------------------------------------------------------

local DRAW_SCALE = 0.25
local ICON_THICKNESS = DRAW_SCALE / 16.0
local HALF = 0.125
local HEIGHT = 0.25

--------------------------------------------------------------------------------
-- CACHES
--------------------------------------------------------------------------------

local voxel_handles = {}
local shape_cache = {}

--------------------------------------------------------------------------------
-- VOXEL HANDLING
--------------------------------------------------------------------------------

function M.get_voxel_handle(item)
  local path = item.texture_path
  if not path or path == "" then
    return nil
  end
  
  local key = path .. ":" .. (item.atlas_index or -1)
  local handle = voxel_handles[key]
  
  if handle == nil then
    handle = minecraft.model.voxel({
      texture = path,
      texture_id = item.item_id,
      atlas_index = item.atlas_index or -1,
      mod_texture = item.mod_texture or false,
    }) or false
    voxel_handles[key] = handle
  end
  
  return handle or nil
end

--------------------------------------------------------------------------------
-- SHAPE CALCULATION
--------------------------------------------------------------------------------

function M.get_shape(item)
  local key = item.item_id .. ":" .. (item.item_damage or 0)
  local shape = shape_cache[key]
  
  if shape == nil then
    local bounds = minecraft.model.item_bounds(item.item_id, item.item_damage or 0)
    if bounds then
      shape = {
        half_x = (bounds.max_x - bounds.min_x) * 0.5 * DRAW_SCALE,
        half_z = (bounds.max_z - bounds.min_z) * 0.5 * DRAW_SCALE,
        height = (bounds.max_y - bounds.min_y) * DRAW_SCALE,
      }
    else
      shape = false
    end
    shape_cache[key] = shape
  end
  
  return shape or nil
end

function M.half_extents(shape)
  local half_x = shape and shape.half_x or HALF
  local half_z = shape and shape.half_z or HALF
  local half_y = (shape and shape.height or HEIGHT) * 0.5
  return half_x, half_y, half_z
end

--------------------------------------------------------------------------------
-- SIMULATION CREATION
--------------------------------------------------------------------------------

function M.create_simulation(item, box3d, config)
  local physics = config.get_physics(item.item_id, item.item_damage or 0)
  local speed = math.sqrt(item.vx * item.vx + item.vy * item.vy + item.vz * item.vz)
  local shape = M.get_shape(item)
  local half_x, half_y, half_z = M.half_extents(shape)
  
  local body, com_offset
  local is_flat = shape == nil
  local is_cube = false
  
  if shape then
    body = box3d.new_box(half_x, half_y, half_z, physics.mass)
    com_offset = box3d.v3(0, 0, 0)
    local largest = math.max(half_x, half_y, half_z)
    local smallest = math.min(half_x, half_y, half_z)
    is_cube = largest > 1e-6 and largest - smallest <= largest * 0.02
  else
    local tex_info = minecraft.render.get_texture_pixels(item.texture_path or item.item_id)
    if tex_info and tex_info.pixels then
      body, com_offset = box3d.new_voxel_body(
        tex_info.pixels, tex_info.width, tex_info.height,
        DRAW_SCALE, physics.mass, ICON_THICKNESS
      )
    else
      body = box3d.new_box(half_x, half_y, ICON_THICKNESS * 0.5, physics.mass)
      com_offset = box3d.v3(0, 0, 0)
    end
  end
  
  -- Random initial rotation
  local axis = box3d.v3(math.random() - 0.5, math.random() - 0.5, math.random() - 0.5)
  local axis_length = box3d.v3_len(axis)
  if axis_length > 1e-6 then
    body.orientation = box3d.make_quat_from_axis_angle(
      box3d.v3_scale(axis, 1.0 / axis_length), math.random() * math.pi * 2.0
    )
  end
  
  -- Random angular velocity
  local kick = math.min(1.5, 0.30 + speed * 2.2)
  body.angular_velocity = box3d.v3(
    (math.random() - 0.5) * kick,
    (math.random() - 0.5) * kick,
    (math.random() - 0.5) * kick
  )
  
  return {
    id = item.id,
    x = item.x, y = item.y, z = item.z,
    px = item.x, py = item.y, pz = item.z,
    vx = item.vx or 0, vy = item.vy or 0, vz = item.vz or 0,
    body = body,
    com_offset = com_offset,
    prev_orientation = {
      x = body.orientation.x, y = body.orientation.y,
      z = body.orientation.z, w = body.orientation.w,
    },
    shape = shape,
    is_flat = is_flat,
    is_cube = is_cube,
    physics = physics,
    grounded = false,
    item_supported = false,
    water_fraction = 0.0,
    water_saturation = 0.0,
    sleeping = false,
    sleep_counter = 0,
    sleep_recheck = 0,
    contact_impulses = {},
    render_yaw = nil,
  }
end

--------------------------------------------------------------------------------
-- RENDER CACHING
--------------------------------------------------------------------------------

function M.cache_render_state(s)
  if s.body and s.body.orientation then
    s.render_yaw, s.render_pitch, s.render_roll = box3d.quat_to_euler_degrees(s.body.orientation)
    s.render_offset = box3d.quat_rotate(s.body.orientation, s.com_offset)
  end
end

return M
