
local box3d = minecraft.require("scripts.box3d")

local DEFAULT_PHYSICS = {
  material = "generic", density = 1.10, saturated_density = 1.10,
  water_absorption_rate = 0.0, drying_rate = 0.0, mass = 1.0,
  air_drag = 0.98, ground_drag = 0.80, friction = 0.60,
  restitution = 0.25, water_restitution = 0.06,
  water_drag = 0.80, water_vertical_drag = 0.76,
  air_angular_damping = 0.05, ground_angular_damping = 0.70,
  water_angular_damping = 1.65, flow_coupling = 0.90,
  buoyancy_cap = 1.35, rest_speed = 0.005,
  angular_rest_speed = 0.01, impact_rest_speed = 0.03,
  rolling_resistance = 0.015,
}

local function clamp_number(value, fallback, lo, hi)
  value = tonumber(value)
  if value == nil or value ~= value or value == math.huge or value == -math.huge then
    value = fallback
  end
  if value < lo then return lo end
  if value > hi then return hi end
  return value
end

local function normalize_physics(source)
  source = type(source) == "table" and source or DEFAULT_PHYSICS
  return {
    name = tostring(source.name or "mod_or_unknown_default"),
    material = tostring(source.material or DEFAULT_PHYSICS.material),
    density = clamp_number(source.density, DEFAULT_PHYSICS.density, 0.01, 40.0),
    saturated_density = clamp_number(source.saturated_density, source.density or DEFAULT_PHYSICS.saturated_density, 0.01, 40.0),
    water_absorption_rate = clamp_number(source.water_absorption_rate, DEFAULT_PHYSICS.water_absorption_rate, 0.0, 1.0),
    drying_rate = clamp_number(source.drying_rate, DEFAULT_PHYSICS.drying_rate, 0.0, 1.0),
    mass = clamp_number(source.mass, DEFAULT_PHYSICS.mass, 0.02, 8.0),
    air_drag = clamp_number(source.air_drag, DEFAULT_PHYSICS.air_drag, 0.10, 0.9999),
    ground_drag = clamp_number(source.ground_drag, DEFAULT_PHYSICS.ground_drag, 0.10, 0.9999),
    friction = clamp_number(source.friction, DEFAULT_PHYSICS.friction, 0.0, 2.0),
    restitution = clamp_number(source.restitution, DEFAULT_PHYSICS.restitution, 0.0, 0.95),
    water_restitution = clamp_number(source.water_restitution, DEFAULT_PHYSICS.water_restitution, 0.0, 0.50),
    water_drag = clamp_number(source.water_drag, DEFAULT_PHYSICS.water_drag, 0.05, 0.9999),
    water_vertical_drag = clamp_number(source.water_vertical_drag, DEFAULT_PHYSICS.water_vertical_drag, 0.05, 0.9999),
    air_angular_damping = clamp_number(source.air_angular_damping, DEFAULT_PHYSICS.air_angular_damping, 0.0, 10.0),
    ground_angular_damping = clamp_number(source.ground_angular_damping, DEFAULT_PHYSICS.ground_angular_damping, 0.0, 10.0),
    water_angular_damping = clamp_number(source.water_angular_damping, DEFAULT_PHYSICS.water_angular_damping, 0.0, 12.0),
    flow_coupling = clamp_number(source.flow_coupling, DEFAULT_PHYSICS.flow_coupling, 0.0, 3.0),
    buoyancy_cap = clamp_number(source.buoyancy_cap, DEFAULT_PHYSICS.buoyancy_cap, 0.05, 2.0),
    rest_speed = clamp_number(source.rest_speed, DEFAULT_PHYSICS.rest_speed, 0.0, 0.10),
    angular_rest_speed = clamp_number(source.angular_rest_speed, DEFAULT_PHYSICS.angular_rest_speed, 0.0, 0.50),
    impact_rest_speed = clamp_number(source.impact_rest_speed, DEFAULT_PHYSICS.impact_rest_speed, 0.0, 0.20),
    rolling_resistance = clamp_number(source.rolling_resistance, DEFAULT_PHYSICS.rolling_resistance, 0.0, 0.50),
  }
end

local function load_physics_database()
  local raw = minecraft.read_asset("assets/item_physics.json")
  if type(raw) ~= "string" then
    return { default = DEFAULT_PHYSICS, blocks = {}, items = {} }
  end
  local decoded = minecraft.util.json_decode(raw)
  if type(decoded) ~= "table" then
    return { default = DEFAULT_PHYSICS, blocks = {}, items = {} }
  end
  decoded.blocks = type(decoded.blocks) == "table" and decoded.blocks or {}
  decoded.items = type(decoded.items) == "table" and decoded.items or {}
  decoded.default = type(decoded.default) == "table" and decoded.default or DEFAULT_PHYSICS
  return decoded
end

local PHYSICS_DATABASE = load_physics_database()
local PHYSICS_DEFAULT = normalize_physics(PHYSICS_DATABASE.default)
local physics_cache = {}

local function physics_for(item_id, item_damage)
  local key = tostring(item_id) .. ":" .. tostring(item_damage or 0)
  local cached = physics_cache[key]
  if cached then return cached end

  local collection = item_id >= 1 and item_id <= 255
                     and PHYSICS_DATABASE.blocks or PHYSICS_DATABASE.items
  local source = collection[tostring(item_id)] or collection[item_id]
  local result
  if type(source) == "table" then
    local variant = type(source.variants) == "table"
                    and (source.variants[tostring(item_damage or 0)] or source.variants[item_damage or 0])
    if type(variant) == "table" then
      local merged = {}
      for k, v in pairs(source) do merged[k] = v end
      for k, v in pairs(variant) do merged[k] = v end
      result = normalize_physics(merged)
    else
      result = normalize_physics(source)
    end
  else
    result = PHYSICS_DEFAULT
  end
  physics_cache[key] = result
  return result
end

local HALF = 0.125
local HEIGHT = 0.25
local DRAW_SCALE = 0.25
local ICON_THICKNESS = DRAW_SCALE / 16.0
local GRAVITY = 0.04

local WATER_EPSILON = 1e-5
local WATER_CURRENT_SPEED = 0.09
local WATER_HORIZONTAL_FLOW_SIGN = -1.0
local WATER_SMALL_BODY_LIMIT = 0.26
local WATER_FLOW_TORQUE = 0.10

local TICK_SECONDS = 1.0 / 20.0
-- Box3D uses one biased solve and one relaxation solve per sub-step. The Lua
-- port now follows that schedule instead of compensating with 8x brute force.
local BASE_SUB_STEPS = 2
local MAX_SUB_STEPS = 4
local MAX_SWEEP_PER_SUBSTEP = 0.18
local VELOCITY_ITERATIONS = 1
local PAIR_VELOCITY_ITERATIONS = 1
local POSITION_ITERATIONS = 1
local CCD_MARGIN = 0.0015
local CCD_TIME_SLOP = 1.0e-4
local CCD_MIN_SWEEP = 0.035
local SPECULATIVE_MARGIN = 0.012
local WORLD_QUERY_MARGIN = 0.025
local POSITION_SLOP = 0.0015
local POSITION_CORRECTION = 0.72
local PAIR_POSITION_CORRECTION = 0.68
local MAX_POSITION_CORRECTION = 0.12
local MAX_BIAS_VELOCITY = 0.15
local RESTITUTION_THRESHOLD = 0.05
local SUPPORT_NORMAL_Y = 0.55
local SUPPORT_PROBE = 0.012
local WAKE_IMPULSE = 0.002
local GROUND_ANGULAR_DAMPING_SCALE = 0.12
local MAX_WORLD_BLOCKS = 64
local MAX_PAIR_CANDIDATES = 1024
local CACHE_RETENTION_TICKS = 3
local CACHE_PRUNE_INTERVAL = 32

local SLEEP_AFTER_TICKS = 12
local SLEEP_RECHECK_TICKS = 10
local SLEEP_LINEAR_SPEED = 0.0035
local SLEEP_ANGULAR_SPEED = 0.018
local MAX_LINEAR_SPEED = 2.0
local MAX_ANGULAR_SPEED = 2.5

local softness_cache = {}
local function contact_softness(substep_fraction)
  local key = math.floor(substep_fraction * 4096.0 + 0.5)
  local cached = softness_cache[key]
  if cached then return cached end
  local source = box3d.make_soft(30.0, 1.0, TICK_SECONDS * substep_fraction)
  cached = {
    bias_rate = source.bias_rate * TICK_SECONDS,
    mass_scale = source.mass_scale,
    impulse_scale = source.impulse_scale,
  }
  softness_cache[key] = cached
  return cached
end

local function is_finite(n)
  return n == n and n ~= math.huge and n ~= -math.huge
end

local water_ids = { [8] = true, [9] = true }
local flowing_water_ids = { [8] = true }
local source_water_ids = { [9] = true }

local function add_water_alias(set, name)
  local id = minecraft.world.block_id(name)
  if id and id > 0 then
    set[id] = true
    water_ids[id] = true
  end
end

add_water_alias(flowing_water_ids, "flowing_water")
add_water_alias(flowing_water_ids, "minecraft:flowing_water")
add_water_alias(flowing_water_ids, "water_flowing")
add_water_alias(source_water_ids, "still_water")
add_water_alias(source_water_ids, "stationary_water")
add_water_alias(source_water_ids, "minecraft:water")
add_water_alias(source_water_ids, "water")

for id in pairs(flowing_water_ids) do
  if id ~= 9 then source_water_ids[id] = nil end
end

local get_block = minecraft.world.get_block
local raw_get_block_meta = rawget(minecraft.world, "get_block_meta")
local meta_mode = type(raw_get_block_meta) == "function" and 0 or -1
local fluid_cell_cache = {}

local function is_water_id(id)
  return water_ids[id] == true
end

local function safe_block_meta(x, y, z)
  if meta_mode < 0 then return nil end
  if meta_mode == 0 then
    local ok, value = pcall(raw_get_block_meta, x, y, z)
    if not ok or type(value) ~= "number" then
      meta_mode = -1
      raw_get_block_meta = nil
      return nil
    end
    meta_mode = 1
    return math.floor(value)
  end
  return math.floor(raw_get_block_meta(x, y, z))
end

local function fluid_height_from_meta(meta)
  if meta == nil or meta >= 8 then return 1.0 / 9.0 end
  return (meta + 1) / 9.0
end

local FLOW_DIRS = {
  { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 },
}

local function fluid_hash(x, y, z)
  return x * 73856093 + y * 19349663 + z * 83492791
end

local function normalize_flow(fx, fy, fz)
  local ls = fx * fx + fy * fy + fz * fz
  if ls <= WATER_EPSILON * WATER_EPSILON then return 0.0, 0.0, 0.0 end
  local inv = 1.0 / math.sqrt(ls)
  return fx * inv, fy * inv, fz * inv
end

local function fluid_cell(x, y, z)
  local hash = fluid_hash(x, y, z)
  local cached = fluid_cell_cache[hash]
  if cached then
    if cached.x ~= nil then
      if cached.x == x and cached.y == y and cached.z == z then return cached end
    else
      for i = 1, #cached do
        local c = cached[i]
        if c.x == x and c.y == y and c.z == z then return c end
      end
    end
  end

  local function store(c)
    if not cached then
      fluid_cell_cache[hash] = c
    elseif cached.x ~= nil then
      fluid_cell_cache[hash] = { cached, c }
    else
      cached[#cached + 1] = c
    end
    return c
  end

  local id = get_block(x, y, z)
  if not is_water_id(id) then
    return store({ x = x, y = y, z = z, water = false })
  end

  local meta = safe_block_meta(x, y, z)
  local surface = is_water_id(get_block(x, y + 1, z))
                  and (y + 1.0) or (y + 1.0 - fluid_height_from_meta(meta))
  local fx, fy, fz = 0.0, 0.0, 0.0
  local current_decay = meta and (meta >= 8 and 0 or meta) or nil

  for i = 1, 4 do
    local dx, dz = FLOW_DIRS[i][1], FLOW_DIRS[i][2]
    local nid = get_block(x + dx, y, z + dz)
    if current_decay ~= nil and is_water_id(nid) then
      local nm = safe_block_meta(x + dx, y, z + dz)
      local nd = nm and (nm >= 8 and 0 or nm) or current_decay
      local difference = nd - current_decay
      fx, fz = fx + dx * difference, fz + dz * difference
    elseif current_decay == nil and source_water_ids[nid] then
      fx, fz = fx - dx, fz - dz
    elseif not is_water_id(nid) and is_water_id(get_block(x + dx, y - 1, z + dz)) then
      fx, fz, fy = fx + dx * 2.0, fz + dz * 2.0, fy - 2.0
    end
  end

  if (meta and meta >= 8) or
     (meta == nil and flowing_water_ids[id] and is_water_id(get_block(x, y + 1, z))) then
    fy = fy - 6.0
  end

  fx, fz = fx * WATER_HORIZONTAL_FLOW_SIGN, fz * WATER_HORIZONTAL_FLOW_SIGN
  fx, fy, fz = normalize_flow(fx, fy, fz)
  return store({ x = x, y = y, z = z, water = true, surface = surface, fx = fx, fy = fy, fz = fz })
end

local function half_extents(shape)
  local half_x = shape and shape.half_x or HALF
  local half_z = shape and shape.half_z or HALF
  local half_y = (shape and shape.height or HEIGHT) * 0.5
  return half_x, half_y, half_z
end

local function projected_half_extents(s)
  local q = s.body.orientation
  local hx, hy, hz = s.body.half.x, s.body.half.y, s.body.half.z
  local xx, yy, zz = q.x * q.x, q.y * q.y, q.z * q.z
  local xy, xz, yz = q.x * q.y, q.x * q.z, q.y * q.z
  local wx, wy, wz = q.w * q.x, q.w * q.y, q.w * q.z
  local m00, m01, m02 = 1 - 2 * (yy + zz), 2 * (xy - wz), 2 * (xz + wy)
  local m10, m11, m12 = 2 * (xy + wz), 1 - 2 * (xx + zz), 2 * (yz - wx)
  local m20, m21, m22 = 2 * (xz - wy), 2 * (yz + wx), 1 - 2 * (xx + yy)
  return math.abs(m00) * hx + math.abs(m01) * hy + math.abs(m02) * hz,
         math.abs(m10) * hx + math.abs(m11) * hy + math.abs(m12) * hz,
         math.abs(m20) * hx + math.abs(m21) * hy + math.abs(m22) * hz
end

local voxel_handles = {}
local function voxel_handle(item)
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

local shape_cache = {}
local function shape_for(item)
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

local sims = {}
local pair_impulses = {}
local simulation_epoch = 0
local current_items = nil

local pending_item_sync = {}
local SERVER_SYNC_INTERVAL = 2
local SERVER_SYNC_EPSILON_SQR = 0.0004
local SERVER_SYNC_MAX_DELTA_SQR = 4.0
local server_sync_clock = 0

local function seed(item)
  local physics = physics_for(item.item_id, item.item_damage or 0)
  local speed = math.sqrt(item.vx * item.vx + item.vy * item.vy + item.vz * item.vz)
  local shape = shape_for(item)
  local half_x, half_y, half_z = half_extents(shape)
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

  local axis = box3d.v3(math.random() - 0.5, math.random() - 0.5, math.random() - 0.5)
  local axis_length = box3d.v3_len(axis)
  if axis_length > 1e-6 then
    body.orientation = box3d.make_quat_from_axis_angle(
      box3d.v3_scale(axis, 1.0 / axis_length), math.random() * math.pi * 2.0
    )
  end
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

local function finite_vec3(v)
  return is_finite(v.x) and is_finite(v.y) and is_finite(v.z)
end

local function finite_quat(q)
  return is_finite(q.x) and is_finite(q.y) and is_finite(q.z) and is_finite(q.w)
end

local function position_of(s)
  local p = s._position
  if not p then p = box3d.v3(); s._position = p end
  p.x, p.y, p.z = s.x, s.y, s.z
  return p
end

local function set_position(s, p)
  s.x, s.y, s.z = p.x, p.y, p.z
  if s._position then s._position.x, s._position.y, s._position.z = p.x, p.y, p.z end
end

local function velocity_of(s)
  local v = s._velocity
  if not v then v = box3d.v3(); s._velocity = v end
  v.x, v.y, v.z = s.vx, s.vy, s.vz
  return v
end

local function set_velocity(s, v)
  s.vx, s.vy, s.vz = v.x, v.y, v.z
  if s._velocity then s._velocity.x, s._velocity.y, s._velocity.z = v.x, v.y, v.z end
end

local function make_box(s, inflate, out)
  return box3d.body_aabb(position_of(s), s.body, s.com_offset, inflate or 0.0, out)
end

local function overlap_length(a0, a1, b0, b1)
  local lo = a0 > b0 and a0 or b0
  local hi = a1 < b1 and a1 or b1
  return hi > lo and hi - lo or 0.0
end

local function sample_water_fast(s)
  local box = make_box(s)
  local height = math.max(WATER_EPSILON, box.max_y - box.min_y)
  local half_x = (box.max_x - box.min_x) * 0.5
  local half_z = (box.max_z - box.min_z) * 0.5
  local cx, cz = (box.min_x + box.max_x) * 0.5, (box.min_z + box.max_z) * 0.5
  local wide = half_x > WATER_SMALL_BODY_LIMIT or half_z > WATER_SMALL_BODY_LIMIT
  local offsets = wide and {
    { 0.0, 0.0 },
    { -half_x * 0.72, -half_z * 0.72 }, { half_x * 0.72, -half_z * 0.72 },
    { -half_x * 0.72,  half_z * 0.72 }, { half_x * 0.72,  half_z * 0.72 },
  } or nil
  local count = wide and 5 or 1
  local wet, fx, fy, fz, bx, by, bz = 0, 0, 0, 0, 0, 0, 0

  for i = 1, count do
    local ox, oz = 0, 0
    if offsets then ox, oz = offsets[i][1], offsets[i][2] end
    local px, pz = cx + ox, cz + oz
    local x, z = math.floor(px), math.floor(pz)
    local min_y = math.floor(box.min_y)
    local max_y = math.floor(box.max_y - WATER_EPSILON)
    for y = min_y, max_y do
      local c = fluid_cell(x, y, z)
      if c.water then
        local amount = overlap_length(box.min_y, box.max_y, y, c.surface)
        if amount > WATER_EPSILON then
          local weight = amount / height
          local wet_lo = box.min_y > y and box.min_y or y
          local wet_hi = box.max_y < c.surface and box.max_y or c.surface
          wet = wet + weight
          fx, fy, fz = fx + c.fx * weight, fy + c.fy * weight, fz + c.fz * weight
          bx, by, bz = bx + px * weight, by + 0.5 * (wet_lo + wet_hi) * weight, bz + pz * weight
        end
      end
    end
  end

  local fraction = math.min(1.0, wet / count)
  if wet <= WATER_EPSILON then
    return 0.0, 0.0, 0.0, 0.0, s.x, s.y, s.z
  end
  return fraction, fx / wet, fy / wet, fz / wet, bx / wet, by / wet, bz / wet
end

local function cache_sleep_render(s)
  s.render_yaw, s.render_pitch, s.render_roll = box3d.quat_to_euler_degrees(s.body.orientation)
  s.render_offset = box3d.quat_rotate(s.body.orientation, s.com_offset)
end

local function wake(s)
  s.sleeping = false
  s.sleep_counter = 0
  s.sleep_recheck = 0
  s.render_yaw, s.render_pitch, s.render_roll = nil, nil, nil
end

local function aabb_union(a, b, inflate, out)
  inflate = inflate or 0.0
  out = out or {}
  out.min_x = math.min(a.min_x, b.min_x) - inflate
  out.min_y = math.min(a.min_y, b.min_y) - inflate
  out.min_z = math.min(a.min_z, b.min_z) - inflate
  out.max_x = math.max(a.max_x, b.max_x) + inflate
  out.max_y = math.max(a.max_y, b.max_y) + inflate
  out.max_z = math.max(a.max_z, b.max_z) + inflate
  return out
end

local HASH_MOD = 134217727
local function quantize_bound(v)
  local scaled = v * 256.0
  if scaled >= 0 then return math.floor(scaled + 0.5) end
  return -math.floor(-scaled + 0.5)
end

local function block_key(b)
  local h = quantize_bound(b.min_x) * 73856093
          + quantize_bound(b.min_y) * 19349663
          + quantize_bound(b.min_z) * 83492791
          + quantize_bound(b.max_x - b.min_x) * 2654435761
          + quantize_bound(b.max_y - b.min_y) * 97531
          + quantize_bound(b.max_z - b.min_z) * 421
  h = h % HASH_MOD
  if h < 0 then h = h + HASH_MOD end
  return h
end

local function load_contact_cache(manifold, prefix, cache)
  for i = 1, #manifold.contacts do
    local c = manifold.contacts[i]
    local key = (prefix * 64 + (c.id or i)) % 8589934591
    c.cache_key = key
    local old = cache[key]
    if old and simulation_epoch - (old.stamp or 0) <= CACHE_RETENTION_TICKS then
      c.normal_impulse = old.n or 0
      c.tangent_impulse1 = old.t1 or 0
      c.tangent_impulse2 = old.t2 or 0
    end
  end
end

local function save_manifold_cache(manifold, cache)
  local contacts = manifold.contacts
  for j = 1, #contacts do
    local c = contacts[j]
    if c.cache_key then
      local entry = cache[c.cache_key]
      if not entry then
        entry = {}
        cache[c.cache_key] = entry
      end
      entry.n = c.normal_impulse or 0
      entry.t1 = c.tangent_impulse1 or 0
      entry.t2 = c.tangent_impulse2 or 0
      entry.stamp = simulation_epoch
    end
  end
end

local function save_contact_cache(manifolds, cache)
  for i = 1, #manifolds do save_manifold_cache(manifolds[i], cache) end
end

local function prune_contact_cache(cache)
  for key, entry in pairs(cache) do
    if simulation_epoch - (entry.stamp or 0) > CACHE_RETENTION_TICKS then
      cache[key] = nil
    end
  end
end

local function collect_world_blocks(s, start_position, end_position, inflate)
  s._world_start_box = box3d.body_aabb(start_position, s.body, s.com_offset, 0, s._world_start_box)
  s._world_end_box = box3d.body_aabb(end_position, s.body, s.com_offset, 0, s._world_end_box)
  s._world_query_box = aabb_union(
    s._world_start_box, s._world_end_box, inflate or WORLD_QUERY_MARGIN, s._world_query_box
  )
  return minecraft.world.get_block_collisions(s._world_query_box) or {}
end

local function world_manifolds(s, blocks, margin, restitution, cache)
  local manifolds = {}
  local position = position_of(s)
  local block_count = math.min(#blocks, MAX_WORLD_BLOCKS)
  for i = 1, block_count do
    local b = blocks[i]
    local manifold = box3d.box_aabb_manifold(position, s.body, s.com_offset, b, margin)
    if manifold then
      manifold.friction = s.physics.friction
      manifold.restitution = restitution
      manifold.block = b
      load_contact_cache(manifold, block_key(b) * 32 + manifold.feature, cache)
      manifolds[#manifolds + 1] = manifold
    end
  end
  return manifolds
end

local function correct_world_penetration(s, blocks)
  local moved = false
  for _ = 1, POSITION_ITERATIONS do
    local deepest
    local position = position_of(s)
    local block_count = math.min(#blocks, MAX_WORLD_BLOCKS)
    for i = 1, block_count do
      local manifold = box3d.box_aabb_manifold(position, s.body, s.com_offset, blocks[i], 0.0)
      if manifold and manifold.separation < -POSITION_SLOP then
        if not deepest or manifold.separation < deepest.separation then deepest = manifold end
      end
    end
    if not deepest then break end
    local corrected, amount = box3d.correct_static_position(
      position, deepest, POSITION_SLOP, POSITION_CORRECTION, MAX_POSITION_CORRECTION
    )
    if amount <= 0 then break end
    set_position(s, corrected)
    moved = true
  end
  return moved
end

local function solve_world_contacts(s, blocks, h, restitution, cache, warm_start)
  local manifolds = world_manifolds(s, blocks, SPECULATIVE_MARGIN, restitution, cache)
  if #manifolds == 0 then return false, 0.0 end
  local velocity, total_impulse = box3d.solve_static_manifolds(
    s.body, position_of(s), velocity_of(s), manifolds,
    {
      h = h,
      softness = contact_softness(h),
      max_push_speed = MAX_BIAS_VELOCITY,
      restitution_threshold = RESTITUTION_THRESHOLD,
      slop = POSITION_SLOP,
      iterations = VELOCITY_ITERATIONS,
      warm_start = warm_start,
    }
  )
  set_velocity(s, velocity)
  save_contact_cache(manifolds, cache)

  local supported = false
  for i = 1, #manifolds do
    local m = manifolds[i]
    if m.normal.y < -SUPPORT_NORMAL_Y then
      for j = 1, #m.contacts do
        if (m.contacts[j].normal_impulse or 0) > 1e-7 or m.separation <= POSITION_SLOP then
          supported = true
          break
        end
      end
    end
    if supported then break end
  end
  return supported, total_impulse
end

local function move_against_world(s, h, in_water)
  local start_position = position_of(s)
  local velocity = velocity_of(s)
  local delta = box3d.v3_scale(velocity, h)
  local end_position = box3d.v3_add(start_position, delta)
  local blocks = collect_world_blocks(s, start_position, end_position, WORLD_QUERY_MARGIN)

  if #blocks == 0 then
    set_position(s, end_position)
    return false
  end

  correct_world_penetration(s, blocks)
  start_position = position_of(s)
  velocity = velocity_of(s)
  delta = box3d.v3_scale(velocity, h)

  local earliest = 1.0
  local sweep2 = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z
  if sweep2 >= CCD_MIN_SWEEP * CCD_MIN_SWEEP then
    local block_count = math.min(#blocks, MAX_WORLD_BLOCKS)
    for i = 1, block_count do
      local toi = box3d.sweep_box_aabb(start_position, delta, s.body, s.com_offset, blocks[i], CCD_MARGIN)
      if toi and toi < earliest then earliest = toi end
    end
  end

  local travel = earliest < 1.0 and math.max(0.0, earliest - CCD_TIME_SLOP) or 1.0
  set_position(s, box3d.v3_add(start_position, box3d.v3_scale(delta, travel)))
  correct_world_penetration(s, blocks)

  local restitution = in_water and s.physics.water_restitution or s.physics.restitution
  local supported, total_impulse = solve_world_contacts(
    s, blocks, h, restitution, s.contact_impulses, true
  )

  -- Finish the sub-step using the post-impact velocity. Reuse the same broadphase
  -- query instead of launching another collision-world query/CCD loop.
  if earliest < 1.0 then
    local remaining = h * (1.0 - earliest)
    if remaining > 1.0e-5 then
      local p = position_of(s)
      local v = velocity_of(s)
      set_position(s, box3d.v3_add(p, box3d.v3_scale(v, remaining)))
      correct_world_penetration(s, blocks)
      local resting_support, resting_impulse = solve_world_contacts(
        s, blocks, remaining, 0.0, s.contact_impulses, false
      )
      supported = supported or resting_support
      total_impulse = total_impulse + resting_impulse
    end
  end

  if total_impulse > WAKE_IMPULSE then s.sleep_counter = 0 end
  return supported
end

local function has_support(s)
  local p = position_of(s)
  local blocks = collect_world_blocks(s, p, p, SUPPORT_PROBE)
  for i = 1, #blocks do
    local m = box3d.box_aabb_manifold(p, s.body, s.com_offset, blocks[i], SUPPORT_PROBE)
    if m and m.normal.y < -SUPPORT_NORMAL_Y then return true end
  end
  for _, other in pairs(sims) do
    if other ~= s then
      local m = box3d.box_box_manifold(
        p, s.body, s.com_offset,
        position_of(other), other.body, other.com_offset,
        SUPPORT_PROBE
      )
      if m and m.normal.y < -SUPPORT_NORMAL_Y then return true end
    end
  end
  return false
end

local function pair_key(a, b)
  local lo, hi = a.id, b.id
  if lo > hi then lo, hi = hi, lo end
  local h = (lo * 65599 + hi * 31337) % HASH_MOD
  if h < 0 then h = h + HASH_MOD end
  return h
end

local broadphase_entries = {}
local pair_candidates = {}
local broadphase_count = 0
local pair_candidate_count = 0

local function broadphase_less(a, b)
  if a.min_x == b.min_x then return a.s.id < b.s.id end
  return a.min_x < b.min_x
end

local function build_pair_candidates(active, margin)
  local count = #active
  for i = 1, count do
    local s = active[i]
    local entry = broadphase_entries[i]
    if not entry then entry = {}; broadphase_entries[i] = entry end
    entry.s = s
    entry.box = make_box(s, margin, entry.box)
    entry.min_x = entry.box.min_x
  end
  for i = count + 1, broadphase_count do broadphase_entries[i] = nil end
  broadphase_count = count
  table.sort(broadphase_entries, broadphase_less)

  local pair_count = 0
  local pair_limit = count > 96 and 256 or (count > 48 and 384 or (count > 24 and 640 or MAX_PAIR_CANDIDATES))
  for i = 1, count - 1 do
    local ea = broadphase_entries[i]
    local aa, a = ea.box, ea.s
    for j = i + 1, count do
      local eb = broadphase_entries[j]
      local bb = eb.box
      if bb.min_x > aa.max_x then break end
      local b = eb.s
      if not (a.sleeping and b.sleeping)
         and aa.max_y >= bb.min_y and aa.min_y <= bb.max_y
         and aa.max_z >= bb.min_z and aa.min_z <= bb.max_z then
        pair_count = pair_count + 1
        local pair = pair_candidates[pair_count]
        if not pair then pair = {}; pair_candidates[pair_count] = pair end
        pair.a, pair.b = a, b
        if pair_count >= pair_limit then break end
      end
    end
    if pair_count >= pair_limit then break end
  end
  for i = pair_count + 1, pair_candidate_count do pair_candidates[i] = nil end
  pair_candidate_count = pair_count
  return pair_count
end

local function resolve_item_collisions(active, h)
  for i = 1, #active do
    if not active[i].sleeping then active[i].item_supported = false end
  end

  local pair_count = build_pair_candidates(active, SPECULATIVE_MARGIN)
  local fast_pair_manifold = #active > 12
  local softness = contact_softness(h)
  for i = 1, pair_count do
    local pair = pair_candidates[i]
    local a, b = pair.a, pair.b
    local pa, pb = position_of(a), position_of(b)
    local manifold = box3d.box_box_manifold(
      pa, a.body, a.com_offset, pb, b.body, b.com_offset, SPECULATIVE_MARGIN, fast_pair_manifold
    )
    if manifold then
      if manifold.separation < -POSITION_SLOP then
        local corrected_a, corrected_b, amount = box3d.correct_pair_positions(
          pa, a.body, pb, b.body, manifold,
          POSITION_SLOP, PAIR_POSITION_CORRECTION, MAX_POSITION_CORRECTION
        )
        if amount > 0 then
          set_position(a, corrected_a)
          set_position(b, corrected_b)
          manifold = box3d.box_box_manifold(
            position_of(a), a.body, a.com_offset,
            position_of(b), b.body, b.com_offset,
            SPECULATIVE_MARGIN, fast_pair_manifold
          )
        end
      end

      if manifold then
        if a.sleeping and not b.sleeping then wake(a) end
        if b.sleeping and not a.sleeping then wake(b) end
        if not (a.sleeping and b.sleeping) then
          local key = pair_key(a, b) * 32 + manifold.feature
          load_contact_cache(manifold, key, pair_impulses)
          manifold.friction = math.sqrt(a.physics.friction * b.physics.friction)
          manifold.restitution = math.max(a.physics.restitution, b.physics.restitution)
          local va, vb, impulse = box3d.solve_pair_manifold(
            a.body, position_of(a), velocity_of(a),
            b.body, position_of(b), velocity_of(b),
            manifold,
            {
              h = h,
              softness = softness,
              max_push_speed = MAX_BIAS_VELOCITY,
              restitution_threshold = RESTITUTION_THRESHOLD,
              slop = POSITION_SLOP,
              iterations = PAIR_VELOCITY_ITERATIONS,
              warm_start = true,
            }
          )
          set_velocity(a, va)
          set_velocity(b, vb)
          save_manifold_cache(manifold, pair_impulses)

          if manifold.normal.y > SUPPORT_NORMAL_Y then
            b.item_supported = true
          elseif manifold.normal.y < -SUPPORT_NORMAL_Y then
            a.item_supported = true
          end
          if impulse > WAKE_IMPULSE then
            wake(a)
            wake(b)
          end
        end
      end
    end
  end
end

local function begin_step(s)
  s.px, s.py, s.pz = s.x, s.y, s.z
  local previous = s.prev_orientation
  previous.x, previous.y = s.body.orientation.x, s.body.orientation.y
  previous.z, previous.w = s.body.orientation.z, s.body.orientation.w

  if s.sleeping then
    s.sleep_recheck = s.sleep_recheck + 1
    if s.sleep_recheck < SLEEP_RECHECK_TICKS then return false end
    s.sleep_recheck = 0
    local water_fraction = sample_water_fast(s)
    if water_fraction <= WATER_EPSILON and has_support(s) then return false end
    wake(s)
  end

  local physics = s.physics
  local water_fraction, flow_x, flow_y, flow_z, buoy_x, buoy_y, buoy_z = sample_water_fast(s)
  s.water_fraction = water_fraction
  s.step_flow_x, s.step_flow_y, s.step_flow_z = flow_x, flow_y, flow_z
  s.step_buoy_x, s.step_buoy_y, s.step_buoy_z = buoy_x, buoy_y, buoy_z

  if water_fraction > WATER_EPSILON then
    local absorb = physics.water_absorption_rate * water_fraction
    s.water_saturation = math.min(1.0, s.water_saturation + absorb * (1.0 - s.water_saturation))
  elseif s.water_saturation > 0 then
    s.water_saturation = math.max(0.0, s.water_saturation - physics.drying_rate)
  end
  local density = physics.density + (physics.saturated_density - physics.density) * s.water_saturation
  s.step_buoyancy_ratio = math.min(physics.buoyancy_cap, 1.0 / math.max(0.01, density))
  return true
end

local function clamp_motion(s)
  local speed2 = s.vx * s.vx + s.vy * s.vy + s.vz * s.vz
  if not is_finite(speed2) then
    s.vx, s.vy, s.vz = 0, 0, 0
  elseif speed2 > MAX_LINEAR_SPEED * MAX_LINEAR_SPEED then
    local f = MAX_LINEAR_SPEED / math.sqrt(speed2)
    s.vx, s.vy, s.vz = s.vx * f, s.vy * f, s.vz * f
  end

  local w = s.body.angular_velocity
  local angular2 = w.x * w.x + w.y * w.y + w.z * w.z
  if not is_finite(angular2) then
    s.body.angular_velocity = box3d.v3(0, 0, 0)
  elseif angular2 > MAX_ANGULAR_SPEED * MAX_ANGULAR_SPEED then
    local f = MAX_ANGULAR_SPEED / math.sqrt(angular2)
    w.x, w.y, w.z = w.x * f, w.y * f, w.z * f
  end
end

local function step_body_sub(s, h)
  if s.sleeping then return end
  local physics = s.physics
  local water_fraction = s.water_fraction
  local in_water = water_fraction > WATER_EPSILON

  if in_water then
    s.vy = s.vy + GRAVITY * (s.step_buoyancy_ratio * water_fraction - 1.0) * h
    local current_scale = WATER_CURRENT_SPEED * physics.flow_coupling
    local target_x = s.step_flow_x * current_scale
    local target_y = s.step_flow_y * current_scale
    local target_z = s.step_flow_z * current_scale
    local side_retention = physics.water_drag ^ (h * water_fraction)
    local vertical_retention = physics.water_vertical_drag ^ (h * water_fraction)
    s.vx = target_x + (s.vx - target_x) * side_retention
    s.vy = target_y + (s.vy - target_y) * vertical_retention
    s.vz = target_z + (s.vz - target_z) * side_retention

    local w = s.body.angular_velocity
    local buoyancy_accel = GRAVITY * s.step_buoyancy_ratio * water_fraction
    w.x = w.x - (s.step_buoy_z - s.z) * buoyancy_accel * WATER_FLOW_TORQUE * h
    w.z = w.z + (s.step_buoy_x - s.x) * buoyancy_accel * WATER_FLOW_TORQUE * h
    local damping = physics.air_angular_damping + physics.water_angular_damping * water_fraction
    local retention = 1.0 / (1.0 + h * damping)
    w.x, w.y, w.z = w.x * retention, w.y * retention, w.z * retention
  else
    s.vy = s.vy - GRAVITY * h
    local damping = physics.air_angular_damping
    if s.grounded or s.item_supported then
      damping = damping + physics.ground_angular_damping * GROUND_ANGULAR_DAMPING_SCALE
    end
    local retention = 1.0 / (1.0 + h * damping)
    local w = s.body.angular_velocity
    w.x, w.y, w.z = w.x * retention, w.y * retention, w.z * retention
  end

  clamp_motion(s)
  box3d.integrate_rotation(s.body, h)
  if not finite_quat(s.body.orientation) then
    s.body.orientation = box3d.quat_identity()
    s.body.angular_velocity = box3d.v3(0, 0, 0)
  end

  s.grounded = move_against_world(s, h, in_water)

  local air_retention = physics.air_drag ^ h
  s.vx, s.vy, s.vz = s.vx * air_retention, s.vy * air_retention, s.vz * air_retention

  if (s.grounded or s.item_supported) and not in_water and physics.rolling_resistance > 0 then
    local horizontal = math.sqrt(s.vx * s.vx + s.vz * s.vz)
    if horizontal > 0 then
      local remaining = math.max(0.0, horizontal - physics.rolling_resistance * h)
      local f = remaining / horizontal
      s.vx, s.vz = s.vx * f, s.vz * f
    end
  end
  clamp_motion(s)
end

local function finish_step(s)
  if s.sleeping then return end
  local supported = s.grounded or s.item_supported
  local w = s.body.angular_velocity
  local speed2 = s.vx * s.vx + s.vy * s.vy + s.vz * s.vz
  local angular2 = w.x * w.x + w.y * w.y + w.z * w.z
  if supported and s.water_fraction <= WATER_EPSILON
     and speed2 <= SLEEP_LINEAR_SPEED * SLEEP_LINEAR_SPEED
     and angular2 <= SLEEP_ANGULAR_SPEED * SLEEP_ANGULAR_SPEED then
    s.sleep_counter = s.sleep_counter + 1
    if s.sleep_counter >= SLEEP_AFTER_TICKS then
      s.vx, s.vy, s.vz = 0, 0, 0
      s.body.angular_velocity = box3d.v3(0, 0, 0)
      s.sleeping = true
      s.sleep_recheck = 0
      cache_sleep_render(s)
    end
  else
    s.sleep_counter = 0
  end
end

local function choose_sub_steps(active, awake_count)
  if awake_count <= 0 then return 0 end
  local load_count = #active
  local steps = load_count > 12 and 1 or BASE_SUB_STEPS
  local max_steps = load_count >= 48 and 1 or (load_count > 12 and 2 or MAX_SUB_STEPS)
  for i = 1, #active do
    local s = active[i]
    if not s.sleeping then
      local speed = math.sqrt(s.vx * s.vx + s.vy * s.vy + s.vz * s.vz)
      local w = s.body.angular_velocity
      local angular_speed = math.sqrt(w.x * w.x + w.y * w.y + w.z * w.z)
      local half = s.body.half
      local radius = math.sqrt(half.x * half.x + half.y * half.y + half.z * half.z)
      local sweep = speed + angular_speed * radius
      steps = math.max(steps, math.ceil(sweep / MAX_SWEEP_PER_SUBSTEP))
    end
  end
  return math.min(max_steps, steps)
end

local function simulate(active)
  simulation_epoch = simulation_epoch + 1
  local awake_count = 0
  for i = 1, #active do
    if begin_step(active[i]) then awake_count = awake_count + 1 end
  end
  local sub_steps = choose_sub_steps(active, awake_count)
  if sub_steps > 0 then
    local h = 1.0 / sub_steps
    for _ = 1, sub_steps do
      for i = 1, #active do step_body_sub(active[i], h) end
      resolve_item_collisions(active, h)
    end
    for i = 1, #active do finish_step(active[i]) end
  end

  if simulation_epoch % CACHE_PRUNE_INTERVAL == 0 then
    prune_contact_cache(pair_impulses)
    for i = 1, #active do prune_contact_cache(active[i].contact_impulses) end
  end
end


local function reconcile_visual_sim(s, item)
  local dx, dy, dz = item.x - s.x, item.y - s.y, item.z - s.z
  local distance2 = dx * dx + dy * dy + dz * dz
  if distance2 > SERVER_SYNC_MAX_DELTA_SQR then
    s.x, s.y, s.z = item.x, item.y, item.z
    s.px, s.py, s.pz = item.x, item.y, item.z
    s.vx, s.vy, s.vz = item.vx or 0, item.vy or 0, item.vz or 0
    s.sleeping = false
    s.sleep_counter = 0
    s.sleep_recheck = 0
    s.contact_impulses = {}
    s.render_yaw, s.render_pitch, s.render_roll = nil, nil, nil
  end
end

minecraft.on(minecraft.events.pre_entity_render, { entity_type = "Item" }, function(event)
  event.canceled = true
  return event
end)

minecraft.on(minecraft.events.client_tick, { before = false, after_world = true, paused = false }, function(event)
  if not event.has_world then return end
  local list = minecraft.entities.list("Item")
  if not list then return end
  current_items = list

  fluid_cell_cache = {}
  local live, active = {}, {}
  for i = 1, #list do
    local item = list[i]
    live[item.id] = true
    local s = sims[item.id]
    if s then
      reconcile_visual_sim(s, item)
    else
      s = seed(item)
      sims[item.id] = s
    end
    active[#active + 1] = s
  end

  simulate(active)

  for i = 1, #active do
    local s = active[i]
    if not (is_finite(s.x) and is_finite(s.y) and is_finite(s.z)
            and finite_vec3(s.body.angular_velocity) and finite_quat(s.body.orientation)) then
      local item
      for j = 1, #list do
        if list[j].id == s.id then item = list[j]; break end
      end
      if item then
        sims[s.id] = seed(item)
        s = sims[s.id]
      end
      pending_item_sync[s.id] = nil
    elseif not s.sleeping then
      local target = pending_item_sync[s.id]
      if not target then target = {}; pending_item_sync[s.id] = target end
      target.x, target.y, target.z = s.x, s.y, s.z
    end
  end

  for id in pairs(sims) do
    if not live[id] then
      sims[id] = nil
      pending_item_sync[id] = nil
    end
  end
end)

minecraft.on(minecraft.events.world_tick, {
  before = false,
  remote = false,
}, function()
  server_sync_clock = server_sync_clock + 1
  if server_sync_clock < SERVER_SYNC_INTERVAL then return end
  server_sync_clock = 0

  local list = minecraft.entities.list("Item")
  if not list then return end

  for i = 1, #list do
    local item = list[i]
    local target = pending_item_sync[item.id]
    if target and item.type == "Item" then
      local dx = target.x - item.x
      local dy = target.y - item.y
      local dz = target.z - item.z
      local distance2 = dx * dx + dy * dy + dz * dz
      if distance2 > SERVER_SYNC_EPSILON_SQR and
         distance2 <= SERVER_SYNC_MAX_DELTA_SQR then
        minecraft.entities.teleport(item.id, target.x, target.y, target.z)
      end
      pending_item_sync[item.id] = nil
    end
  end
end)

local function render_items(event)
  local list = current_items or minecraft.entities.list("Item")
  if not list then return end
  local d = event.tick_delta or 1.0
  for i = 1, #list do
    local item = list[i]
    local s = sims[item.id]
    if s then
      local orientation, world_offset, yaw, pitch, roll
      if s.sleeping and s.render_yaw ~= nil then
        orientation = s.body.orientation
        world_offset = s.render_offset
        yaw, pitch, roll = s.render_yaw, s.render_pitch, s.render_roll
      else
        orientation = box3d.quat_slerp(s.prev_orientation, s.body.orientation, d)
        world_offset = box3d.quat_rotate(orientation, s.com_offset)
        yaw, pitch, roll = box3d.quat_to_euler_degrees(orientation)
      end
      local transform = {
        x = s.px + (s.x - s.px) * d - world_offset.x,
        y = s.py + (s.y - s.py) * d - world_offset.y,
        z = s.pz + (s.z - s.pz) * d - world_offset.z,
        yaw = yaw,
        pitch = pitch,
        roll = roll,
        pivot_y = 0.5,
        scale = DRAW_SCALE,
      }
      local drew_model = minecraft.model.draw_item(item.item_id, item.item_damage or 0, transform)
      if not drew_model then
        local handle = voxel_handle(item)
        if handle then
          minecraft.model.draw(handle, transform)
        end
      end
    end
  end
end

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.terrain_opaque,
  moment = minecraft.render.moments.after,
}, function(event)
  if not event.shadow_pass then render_items(event) end
end)

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.entities,
  moment = minecraft.render.moments.after,
}, function(event)
  if event.shadow_pass then render_items(event) end
end)
