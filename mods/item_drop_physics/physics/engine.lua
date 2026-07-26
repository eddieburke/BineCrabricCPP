-- ============================================================================
-- item_drop_physics: rigid-body engine
--
-- Replaces lib/box3d.lua and the old world/water split. Design notes:
--   * Body state is flat scalars on the sim table. No vector or quaternion
--     tables are allocated after spawn, so a tick does zero garbage work.
--   * Movement is a swept AABB resolved axis-by-axis against block collision
--     boxes -- one minecraft.world.get_block_collisions call per body per tick.
--   * Orientation is a quaternion integrated from angular velocity; contacts
--     couple tangential slip into spin so items roll and settle.
--   * Sleeping bodies are skipped entirely and only re-checked periodically.
-- ============================================================================

local materials = require("physics.materials")

local M = {}

local sqrt, min, max, abs = math.sqrt, math.min, math.max, math.abs
local floor, asin, random, pi = math.floor, math.asin, math.random, math.pi
local atan2 = math.atan2 or math.atan

--------------------------------------------------------------------------------
-- CONSTANTS
--------------------------------------------------------------------------------

local GRAVITY = -16.0            -- blocks/s^2, matches vanilla item fall
local MAX_SPEED = 12.0           -- blocks/s, keeps the sweep bounded
local MAX_SPIN = 14.0            -- rad/s
local MAX_SWEEP = 0.25           -- blocks per substep before subdividing
local MAX_SUBSTEPS = 4
local SKIN = 1.0e-3              -- contact separation to avoid re-penetration
local BOUNCE_CUTOFF = 1.2        -- below this impact speed, no bounce
local SLEEP_SPEED = 0.02
local SLEEP_SPIN = 0.10
local SLEEP_TICKS = 12
local SLEEP_RECHECK = 10         -- ticks between wake tests for sleeping bodies
local BUOYANCY_CLAMP = 2.5       -- limits upward accel for very light items
local WATER_DRAG = 0.80          -- per-tick retention while submerged
local FLOW_ACCEL = 4.0           -- blocks/s^2 imparted by flowing water

local WATER_STILL, WATER_FLOWING = 9, 8

-- Beta flowing-water metadata -> horizontal flow direction, flattened as
-- (dx, dz) pairs indexed by (meta % 4) * 2.
local FLOW_DIRS = { [0] = -1, [1] = 0, [2] = 1, [3] = 0, [4] = 0, [5] = -1, [6] = 0, [7] = 1 }

--------------------------------------------------------------------------------
-- QUATERNIONS (flat scalars in, flat scalars out)
--------------------------------------------------------------------------------

--- Normalized quaternion for a rotation of `angle` radians about a random axis.
local function random_quat()
  local ax, ay, az = random() - 0.5, random() - 0.5, random() - 0.5
  local len = sqrt(ax * ax + ay * ay + az * az)
  if len < 1e-6 then return 0, 0, 0, 1 end
  local angle = random() * pi * 2.0
  local s = math.sin(angle * 0.5) / len
  return ax * s, ay * s, az * s, math.cos(angle * 0.5)
end

--- Advance `q` by angular velocity `w` over `dt`, renormalizing.
local function integrate_quat(qx, qy, qz, qw, wx, wy, wz, dt)
  local h = dt * 0.5
  local nx = qx + h * (wx * qw + wy * qz - wz * qy)
  local ny = qy + h * (wy * qw + wz * qx - wx * qz)
  local nz = qz + h * (wz * qw + wx * qy - wy * qx)
  local nw = qw - h * (wx * qx + wy * qy + wz * qz)
  local len = sqrt(nx * nx + ny * ny + nz * nz + nw * nw)
  if len < 1e-9 then return 0, 0, 0, 1 end
  local inv = 1.0 / len
  return nx * inv, ny * inv, nz * inv, nw * inv
end

--- Shortest-arc interpolation, used for render smoothing between ticks.
function M.quat_slerp(ax, ay, az, aw, bx, by, bz, bw, t)
  local d = ax * bx + ay * by + az * bz + aw * bw
  if d < 0.0 then
    bx, by, bz, bw, d = -bx, -by, -bz, -bw, -d
  end
  local sa, sb
  if d > 0.9995 then
    sa, sb = 1.0 - t, t
  else
    local theta = math.acos(d)
    local inv_sin = 1.0 / math.sin(theta)
    sa = math.sin((1.0 - t) * theta) * inv_sin
    sb = math.sin(t * theta) * inv_sin
  end
  local x, y, z, w = ax * sa + bx * sb, ay * sa + by * sb, az * sa + bz * sb, aw * sa + bw * sb
  local len = sqrt(x * x + y * y + z * z + w * w)
  if len < 1e-9 then return 0, 0, 0, 1 end
  local inv = 1.0 / len
  return x * inv, y * inv, z * inv, w * inv
end

--- Quaternion -> yaw/pitch/roll in degrees, matching the renderer's convention.
function M.quat_to_euler_degrees(qx, qy, qz, qw)
  local xx, yy, zz = qx * qx, qy * qy, qz * qz
  local pitch = asin(max(-1.0, min(1.0, 2.0 * (qw * qx - qy * qz))))
  local roll = atan2(2.0 * (qx * qy + qw * qz), 1.0 - 2.0 * (xx + zz))
  local yaw = atan2(2.0 * (qx * qz + qw * qy), 1.0 - 2.0 * (xx + yy))
  local k = 180.0 / pi
  return yaw * k, pitch * k, roll * k
end

--------------------------------------------------------------------------------
-- WATER
--------------------------------------------------------------------------------

--- Water surface height inside the cell at (x, y, z), or nil if it holds no water.
local function water_surface(x, y, z)
  local id = minecraft.world.get_block(x, y, z)
  if id ~= WATER_STILL and id ~= WATER_FLOWING then return nil end
  local meta = minecraft.world.get_block_meta(x, y, z) or 0
  if id == WATER_STILL or meta >= 8 then return y + 1.0 end
  return y + (8 - meta) / 9.0
end

--- Submerged fraction of the body plus the flow it sits in.
-- Items are smaller than a block, so sampling the column the centre occupies is
-- both exact enough and O(1).
local function sample_water(s)
  local cx, cz = floor(s.x), floor(s.z)
  local bottom, top = s.y - s.hy, s.y + s.hy
  local surface = water_surface(cx, floor(top), cz) or water_surface(cx, floor(bottom), cz)
  if not surface then return 0.0, 0.0, 0.0 end

  local height = top - bottom
  local submersion = height > 1e-6 and (surface - bottom) / height or 1.0
  if submersion <= 0.0 then return 0.0, 0.0, 0.0 end
  if submersion > 1.0 then submersion = 1.0 end

  local fy = floor(bottom)
  local id = minecraft.world.get_block(cx, fy, cz)
  if id == WATER_FLOWING then
    local meta = minecraft.world.get_block_meta(cx, fy, cz) or 0
    if meta < 8 then
      local i = (meta % 4) * 2
      return submersion, -FLOW_DIRS[i], -FLOW_DIRS[i + 1]
    end
  end
  return submersion, 0.0, 0.0
end

--------------------------------------------------------------------------------
-- BLOCK COLLISION
--------------------------------------------------------------------------------

local query = { min_x = 0, min_y = 0, min_z = 0, max_x = 0, max_y = 0, max_z = 0 }

--- Block AABBs overlapping the body's swept volume. `query` is reused so the
-- only allocation per call is the result table the host hands back.
local function sweep_boxes(s, dx, dy, dz)
  local hx, hy, hz = s.hx, s.hy, s.hz
  query.min_x = s.x - hx + min(dx, 0) - SKIN
  query.min_y = s.y - hy + min(dy, 0) - SKIN
  query.min_z = s.z - hz + min(dz, 0) - SKIN
  query.max_x = s.x + hx + max(dx, 0) + SKIN
  query.max_y = s.y + hy + max(dy, 0) + SKIN
  query.max_z = s.z + hz + max(dz, 0) + SKIN
  return minecraft.world.get_block_collisions(query)
end

--- Clip `d` along one axis against `box`, given the body's extent on the other
-- two axes. Returns the clipped delta and whether contact occurred.
local function clip(d, lo, hi, box_lo, box_hi, a_lo, a_hi, b_lo, b_hi, box_a_lo, box_a_hi, box_b_lo, box_b_hi)
  if a_hi <= box_a_lo or a_lo >= box_a_hi then return d, false end
  if b_hi <= box_b_lo or b_lo >= box_b_hi then return d, false end
  if d > 0.0 and hi <= box_lo + SKIN then
    local allowed = box_lo - hi - SKIN
    if allowed < d then return max(allowed, 0.0), true end
  elseif d < 0.0 and lo >= box_hi - SKIN then
    local allowed = box_hi - lo + SKIN
    if allowed > d then return min(allowed, 0.0), true end
  end
  return d, false
end

--- Move the body by (dx, dy, dz), resolving Y then X then Z. Returns the
-- realized delta and per-axis contact flags.
local function move(s, dx, dy, dz)
  local boxes = sweep_boxes(s, dx, dy, dz)
  if not boxes or #boxes == 0 then
    s.x, s.y, s.z = s.x + dx, s.y + dy, s.z + dz
    return false, false, false
  end

  local hx, hy, hz = s.hx, s.hy, s.hz
  local x, y, z = s.x, s.y, s.z
  local hit_x, hit_y, hit_z = false, false, false
  local count = #boxes

  -- Y first: resting on ground is the common case and settling it before the
  -- horizontal axes keeps items from catching on block seams.
  for i = 1, count do
    local b = boxes[i]
    local touched
    dy, touched = clip(dy, y - hy, y + hy, b.min_y, b.max_y,
      x - hx, x + hx, z - hz, z + hz, b.min_x, b.max_x, b.min_z, b.max_z)
    hit_y = hit_y or touched
  end
  y = y + dy

  for i = 1, count do
    local b = boxes[i]
    local touched
    dx, touched = clip(dx, x - hx, x + hx, b.min_x, b.max_x,
      y - hy, y + hy, z - hz, z + hz, b.min_y, b.max_y, b.min_z, b.max_z)
    hit_x = hit_x or touched
  end
  x = x + dx

  for i = 1, count do
    local b = boxes[i]
    local touched
    dz, touched = clip(dz, z - hz, z + hz, b.min_z, b.max_z,
      x - hx, x + hx, y - hy, y + hy, b.min_x, b.max_x, b.min_y, b.max_y)
    hit_z = hit_z or touched
  end
  z = z + dz

  s.x, s.y, s.z = x, y, z
  return hit_x, hit_y, hit_z
end

--------------------------------------------------------------------------------
-- SIMULATION
--------------------------------------------------------------------------------

--- Build a simulation body for an item entity.
-- Mass comes from density * model volume, so no per-item mass table is needed.
function M.create(item, half_x, half_y, half_z)
  local mat = materials.get(item.item_id)
  local volume = 8.0 * half_x * half_y * half_z
  local qx, qy, qz, qw = random_quat()
  local vx, vy, vz = item.vx or 0.0, item.vy or 0.0, item.vz or 0.0
  local kick = min(MAX_SPIN, 2.0 + sqrt(vx * vx + vy * vy + vz * vz) * 3.0)

  return {
    id = item.id,
    item_id = item.item_id,
    item_damage = item.item_damage or 0,
    mat = mat,
    mass = max(0.02, mat.density * volume * 1000.0),
    -- Inverse inertia of a solid box about its centre, per unit mass. Only the
    -- ratio matters for the impulse response, so mass cancels out.
    inv_inertia = 3.0 / (half_x * half_x + half_y * half_y + half_z * half_z),
    hx = half_x, hy = half_y, hz = half_z,
    x = item.x, y = item.y, z = item.z,
    px = item.x, py = item.y, pz = item.z,
    vx = vx, vy = vy, vz = vz,
    qx = qx, qy = qy, qz = qz, qw = qw,
    pqx = qx, pqy = qy, pqz = qz, pqw = qw,
    wx = (random() - 0.5) * kick,
    wy = (random() - 0.5) * kick,
    wz = (random() - 0.5) * kick,
    grounded = false,
    submersion = 0.0,
    sleeping = false,
    sleep_ticks = 0,
    recheck = 0,
  }
end

--- Resolve a contact on one axis: reflect with restitution, apply Coulomb
-- friction to the tangent, and couple the slip into spin.
local function contact(s, axis, normal)
  local mat = s.mat
  local vn, t1, t2

  if axis == 2 then
    vn, t1, t2 = s.vy, s.vx, s.vz
  elseif axis == 1 then
    vn, t1, t2 = s.vx, s.vy, s.vz
  else
    vn, t1, t2 = s.vz, s.vx, s.vy
  end

  -- Normal response.
  local impact = abs(vn)
  if impact > BOUNCE_CUTOFF then
    vn = -vn * mat.restitution
  else
    vn = 0.0
  end

  -- Coulomb friction against the tangential slip.
  local slip = sqrt(t1 * t1 + t2 * t2)
  if slip > 1e-6 then
    local drop = min(slip, mat.friction * max(impact, 1.0))
    local scale = (slip - drop) / slip
    t1, t2 = t1 * scale, t2 * scale
    -- Slip that friction removed becomes spin about the contact tangents.
    local spin = drop * s.inv_inertia * 0.02
    if axis == 2 then
      s.wx = s.wx + t2 * spin * normal
      s.wz = s.wz - t1 * spin * normal
    else
      s.wy = s.wy + (t1 + t2) * spin * normal * 0.5
    end
  end

  if axis == 2 then
    s.vy, s.vx, s.vz = vn, t1, t2
  elseif axis == 1 then
    s.vx, s.vy, s.vz = vn, t1, t2
  else
    s.vz, s.vx, s.vy = vn, t1, t2
  end
end

--- Advance one body by `dt` seconds.
local function step_body(s, dt)
  local mat = s.mat

  local submersion, flow_x, flow_z = sample_water(s)
  s.submersion = submersion

  -- Gravity and buoyancy. Buoyant acceleration is (rho_water / rho_body) * g
  -- upward, scaled by how much of the body is under the surface.
  local accel = GRAVITY
  if submersion > 0.0 then
    accel = GRAVITY * max(-BUOYANCY_CLAMP, 1.0 - submersion / mat.density)
  end
  s.vy = s.vy + accel * dt

  if submersion > 0.0 then
    if flow_x ~= 0.0 or flow_z ~= 0.0 then
      s.vx = s.vx + flow_x * FLOW_ACCEL * submersion * dt
      s.vz = s.vz + flow_z * FLOW_ACCEL * submersion * dt
    end
    -- Drag blends between air and water with submersion.
    local drag = mat.drag + (WATER_DRAG - mat.drag) * submersion
    local f = drag ^ (dt * 20.0)
    s.vx, s.vy, s.vz = s.vx * f, s.vy * f, s.vz * f
    s.wx, s.wy, s.wz = s.wx * f, s.wy * f, s.wz * f
  else
    local f = mat.drag ^ (dt * 20.0)
    s.vx, s.vy, s.vz = s.vx * f, s.vy * f, s.vz * f
    s.wx, s.wy, s.wz = s.wx * 0.995, s.wy * 0.995, s.wz * 0.995
  end

  local speed = sqrt(s.vx * s.vx + s.vy * s.vy + s.vz * s.vz)
  if speed > MAX_SPEED then
    local k = MAX_SPEED / speed
    s.vx, s.vy, s.vz = s.vx * k, s.vy * k, s.vz * k
    speed = MAX_SPEED
  end

  local hit_x, hit_y, hit_z = move(s, s.vx * dt, s.vy * dt, s.vz * dt)

  s.grounded = hit_y and s.vy <= 0.0
  if hit_y then contact(s, 2, s.vy < 0.0 and 1.0 or -1.0) end
  if hit_x then contact(s, 1, s.vx < 0.0 and 1.0 or -1.0) end
  if hit_z then contact(s, 3, s.vz < 0.0 and 1.0 or -1.0) end

  -- Rolling resistance and angular damping once the item is supported.
  if s.grounded then
    local roll = 1.0 - min(0.5, mat.friction * dt * 4.0)
    s.wx, s.wy, s.wz = s.wx * roll, s.wy * roll, s.wz * roll
    s.vx, s.vz = s.vx * roll, s.vz * roll
  end

  local spin = sqrt(s.wx * s.wx + s.wy * s.wy + s.wz * s.wz)
  if spin > MAX_SPIN then
    local k = MAX_SPIN / spin
    s.wx, s.wy, s.wz = s.wx * k, s.wy * k, s.wz * k
  end

  s.qx, s.qy, s.qz, s.qw = integrate_quat(s.qx, s.qy, s.qz, s.qw, s.wx, s.wy, s.wz, dt)
end

--- Decide whether a body has come to rest, or should wake up again.
local function update_sleep(s)
  local speed = sqrt(s.vx * s.vx + s.vy * s.vy + s.vz * s.vz)
  local spin = sqrt(s.wx * s.wx + s.wy * s.wy + s.wz * s.wz)
  if speed < SLEEP_SPEED and spin < SLEEP_SPIN and s.grounded then
    s.sleep_ticks = s.sleep_ticks + 1
    if s.sleep_ticks >= SLEEP_TICKS then
      s.sleeping = true
      s.vx, s.vy, s.vz = 0.0, 0.0, 0.0
      s.wx, s.wy, s.wz = 0.0, 0.0, 0.0
    end
  else
    s.sleep_ticks = 0
  end
end

--- Cheap wake test for a sleeping body: has the ground gone away, or has water
-- moved in? Only runs every SLEEP_RECHECK ticks.
local function should_wake(s)
  local boxes = sweep_boxes(s, 0.0, -SKIN * 4.0, 0.0)
  if not boxes or #boxes == 0 then return true end
  local submersion = sample_water(s)
  return abs(submersion - s.submersion) > 0.05
end

--- Step every live body. `sims` is a map of entity id -> body.
function M.step(sims, dt)
  for _, s in pairs(sims) do
    s.px, s.py, s.pz = s.x, s.y, s.z
    s.pqx, s.pqy, s.pqz, s.pqw = s.qx, s.qy, s.qz, s.qw

    if s.sleeping then
      s.recheck = s.recheck + 1
      if s.recheck >= SLEEP_RECHECK then
        s.recheck = 0
        if should_wake(s) then
          s.sleeping = false
          s.sleep_ticks = 0
        end
      end
    else
      -- Subdivide only when the body would tunnel; most ticks run one step.
      local speed = sqrt(s.vx * s.vx + s.vy * s.vy + s.vz * s.vz)
      local steps = 1
      if speed * dt > MAX_SWEEP then
        steps = min(MAX_SUBSTEPS, floor(speed * dt / MAX_SWEEP) + 1)
      end
      local sub_dt = dt / steps
      for _ = 1, steps do step_body(s, sub_dt) end
      update_sleep(s)
    end
  end
end

return M
