-- box3d: Lua port of box3d-main (https://github.com/erincatto/box3d)
-- A 3D rigid body physics engine.
-- Mirrors the C library structure: math, dynamic tree, bodies, shapes,
-- collision, contact solver, world simulation.

local box3d = {}

-- ============================================================== Math --
-- Port of box3d/math_functions.h and src/math_internal.h

local function v3(x, y, z) return { x = x or 0, y = y or 0, z = z or 0 } end
local function v3_add(a, b) return v3(a.x + b.x, a.y + b.y, a.z + b.z) end
local function v3_sub(a, b) return v3(a.x - b.x, a.y - b.y, a.z - b.z) end
local function v3_scale(a, s) return v3(a.x * s, a.y * s, a.z * s) end
local function v3_dot(a, b) return a.x * b.x + a.y * b.y + a.z * b.z end
local function v3_cross(a, b)
  return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x)
end
local function v3_len(a) return math.sqrt(v3_dot(a, a)) end
local function v3_len_sqr(a) return v3_dot(a, a) end
local function v3_normalize(a)
  local ls = v3_len_sqr(a)
  if ls <= 0 then return v3(0, 0, 0) end
  local inv = 1.0 / math.sqrt(ls)
  return v3(a.x * inv, a.y * inv, a.z * inv)
end
local function v3_min(a, b) return v3(math.min(a.x, b.x), math.min(a.y, b.y), math.min(a.z, b.z)) end
local function v3_max(a, b) return v3(math.max(a.x, b.x), math.max(a.y, b.y), math.max(a.z, b.z)) end
local function v3_clamp(a, lo, hi)
  return v3(
    math.max(lo.x, math.min(a.x, hi.x)),
    math.max(lo.y, math.min(a.y, hi.y)),
    math.max(lo.z, math.min(a.z, hi.z))
  )
end
local function v3_abs(a) return v3(math.abs(a.x), math.abs(a.y), math.abs(a.z)) end
local function v3_mul(a, b) return v3(a.x * b.x, a.y * b.y, a.z * b.z) end
local function v3_neg(a) return v3(-a.x, -a.y, -a.z) end
local function v3_mul_add(a, s, b) return v3(a.x + s * b.x, a.y + s * b.y, a.z + s * b.z) end
local function v3_mul_sub(a, s, b) return v3(a.x - s * b.x, a.y - s * b.y, a.z - s * b.z) end
local function v3_lerp(a, b, t) return v3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t) end

local function quat_identity() return { x = 0, y = 0, z = 0, w = 1 } end

-- Hamilton quaternion multiplication (b3MulQuat)
local function quat_mul(a, b)
  return {
    x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
    y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
    z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
    w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
  }
end

-- Conjugate (b3Conjugate, cheap inverse of unit quat)
local function quat_conjugate(q) return { x = -q.x, y = -q.y, z = -q.z, w = q.w } end

-- Normalize (b3NormalizeQuat, 128-bit precision variant adapted to float)
local function quat_normalize(q)
  local len = math.sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w)
  if len < 1e-9 then return quat_identity() end
  local inv = 1.0 / len
  return { x = q.x * inv, y = q.y * inv, z = q.z * inv, w = q.w * inv }
end

-- Rotate vector by quaternion (b3RotateVector)
local function quat_rotate(q, v)
  local qv = v3(q.x, q.y, q.z)
  local t = v3_scale(v3_cross(qv, v), 2.0)
  return v3_add(v3_add(v, v3_scale(t, q.w)), v3_cross(qv, t))
end

-- Inverse rotate (b3InvRotateVector)
local function quat_inv_rotate(q, v)
  return quat_rotate(quat_conjugate(q), v)
end

-- Quaternion dot product (b3DotQuat)
local function quat_dot(a, b)
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w
end

-- Relative quaternion: inv(q1) * q2 (b3InvMulQuat)
local function quat_inv_mul(q1, q2)
  local t1 = v3_cross(q2, q1)
  local t2 = v3_mul_add(t1, q1.w, q2)
  local t3 = v3_mul_sub(t2, q2.w, q1)
  return { x = t3.x, y = t3.y, z = t3.z, w = q1.w * q2.w + v3_dot(q1, q2) }
end

-- Integrate rotation: q2 = normalize(q1 + 0.5 * omega * q1)
-- Ported from b3IntegrateRotation (math_internal.h)
local function quat_integrate(q1, delta_rotation)
  local half = v3_scale(delta_rotation, 0.5)
  local qd = quat_mul({ x = half.x, y = half.y, z = half.z, w = 0 }, q1)
  local q2 = { x = q1.x + qd.x, y = q1.y + qd.y, z = q1.z + qd.z, w = q1.w + qd.w }
  return quat_normalize(q2)
end

-- 3x3 rotation matrix from quaternion (b3MakeMatrixFromQuat via quat_to_mat3)
local function quat_to_mat3(q)
  local x, y, z, w = q.x, q.y, q.z, q.w
  local x2, y2, z2 = x + x, y + y, z + z
  local xx, yy, zz = x * x2, y * y2, z * z2
  local xy, xz, yz = x * y2, x * z2, y * z2
  local wx, wy, wz = w * x2, w * y2, w * z2
  return {
    { 1 - (yy + zz), xy - wz, xz + wy },
    { xy + wz, 1 - (xx + zz), yz - wx },
    { xz - wy, yz + wx, 1 - (xx + yy) },
  }
end

-- Matrix-vector multiply (b3MulMV)
local function mat3_mul_vec(m, v)
  return v3(
    m[1][1] * v.x + m[1][2] * v.y + m[1][3] * v.z,
    m[2][1] * v.x + m[2][2] * v.y + m[2][3] * v.z,
    m[3][1] * v.x + m[3][2] * v.y + m[3][3] * v.z
  )
end

-- build rotation matrix from quat as column-vector struct (b3MakeMatrixFromQuat)
local function make_matrix_from_quat(q)
  local xx = q.x * q.x; local yy = q.y * q.y; local zz = q.z * q.z
  local xy = q.x * q.y; local xz = q.x * q.z; local xw = q.x * q.w
  local yz = q.y * q.z; local yw = q.y * q.w; local zw = q.z * q.w
  return {
    cx = v3(1 - 2 * (yy + zz), 2 * (xy + zw), 2 * (xz - yw)),
    cy = v3(2 * (xy - zw), 1 - 2 * (xx + zz), 2 * (yz + xw)),
    cz = v3(2 * (xz + yw), 2 * (yz - xw), 1 - 2 * (xx + yy)),
  }
end

-- Component-wise abs of matrix (b3AbsMatrix3)
local function mat3_abs_matrix(m)
  return { cx = v3_abs(m.cx), cy = v3_abs(m.cy), cz = v3_abs(m.cz) }
end

-- AABB helpers (port of box3d AABB functions)
local function make_aabb_from_points(points, count, radius)
  local a = { lower = v3(points[1].x, points[1].y, points[1].z),
             upper = v3(points[1].x, points[1].y, points[1].z) }
  for i = 2, count do
    a.lower = v3_min(a.lower, points[i])
    a.upper = v3_max(a.upper, points[i])
  end
  local r = v3(radius, radius, radius)
  a.lower = v3_sub(a.lower, r)
  a.upper = v3_add(a.upper, r)
  return a
end

local function aabb_contains(a, b)
  if a.lower.x > b.lower.x or b.upper.x > a.upper.x then return false end
  if a.lower.y > b.lower.y or b.upper.y > a.upper.y then return false end
  if a.lower.z > b.lower.z or b.upper.z > a.upper.z then return false end
  return true
end

local function aabb_overlaps(a, b)
  if a.upper.x < b.lower.x or a.lower.x > b.upper.x then return false end
  if a.upper.y < b.lower.y or a.lower.y > b.upper.y then return false end
  if a.upper.z < b.lower.z or a.lower.z > b.upper.z then return false end
  return true
end

local function aabb_center(a) return v3_scale(v3_add(a.upper, a.lower), 0.5) end
local function aabb_extents(a) return v3_scale(v3_sub(a.upper, a.lower), 0.5) end

local function aabb_union(a, b)
  return { lower = v3_min(a.lower, b.lower), upper = v3_max(a.upper, b.upper) }
end

local function aabb_inflate(a, e)
  local r = v3(e, e, e)
  return { lower = v3_sub(a.lower, r), upper = v3_add(a.upper, r) }
end

-- Transform AABB (b3AABB_Transform)
local function aabb_transform(tf, a)
  local center = v3_add(tf.p, quat_rotate(tf.q, aabb_center(a)))
  local m = make_matrix_from_quat(tf.q)
  local ext = mat3_mul_vec(mat3_abs_matrix(m), aabb_extents(a))
  return { lower = v3_sub(center, ext), upper = v3_add(center, ext) }
end

-- Perimeter (surface area proxy, b3Perimeter)
local function aabb_perimeter(a)
  local d = v3_sub(a.upper, a.lower)
  return 2 * (d.x * d.y + d.y * d.z + d.z * d.x)
end

-- Transform point (b3TransformPoint)
local function transform_point(t, v)
  return v3_add(quat_rotate(t.q, v), t.p)
end

-- Inverse transform point (b3InvTransformPoint)
local function inv_transform_point(t, v)
  return quat_inv_rotate(t.q, v3_sub(v, t.p))
end

-- Multiply transforms (b3MulTransforms)
local function mul_transforms(a, b)
  return { p = v3_add(quat_rotate(a.q, b.p), a.p), q = quat_mul(a.q, b.q) }
end

-- Inverse multiply transforms (b3InvMulTransforms)
local function inv_mul_transforms(a, b)
  return { p = quat_inv_rotate(a.q, v3_sub(b.p, a.p)), q = quat_inv_mul(a.q, b.q) }
end

-- Invert transform (b3InvertTransform)
local function invert_transform(t)
  return { p = quat_inv_rotate(t.q, v3_neg(t.p)), q = quat_conjugate(t.q) }
end

-- Slerp (standard, not in box3d core but needed for rendering)
local function quat_slerp(a, b, t)
  local dot = quat_dot(a, b)
  if dot < 0 then
    b = { x = -b.x, y = -b.y, z = -b.z, w = -b.w }
    dot = -dot
  end
  if dot > 0.9995 then
    local q = {
      x = a.x + (b.x - a.x) * t, y = a.y + (b.y - a.y) * t,
      z = a.z + (b.z - a.z) * t, w = a.w + (b.w - a.w) * t,
    }
    return quat_normalize(q)
  end
  local theta0 = math.acos(dot)
  local theta = theta0 * t
  local sin_theta0 = math.sin(theta0)
  local s0 = math.sin(theta0 - theta) / sin_theta0
  local s1 = math.sin(theta) / sin_theta0
  return {
    x = a.x * s0 + b.x * s1, y = a.y * s0 + b.y * s1,
    z = a.z * s0 + b.z * s1, w = a.w * s0 + b.w * s1,
  }
end

-- Quaternion to Euler degrees for rendering (not in box3d core)
local function quat_to_euler_degrees(q)
  local m = quat_to_mat3(q)
  local pitch = math.asin(math.max(-1, math.min(1, -m[2][3])))
  local roll = math.atan(m[2][1], m[2][2])
  local yaw = math.atan(m[1][3], m[3][3])
  local k = 180.0 / math.pi
  return yaw * k, pitch * k, roll * k
end

local function make_quat_from_axis_angle(axis, radians)
  local half = radians * 0.5
  local s = math.sin(half)
  return quat_normalize({ x = axis.x * s, y = axis.y * s, z = axis.z * s, w = math.cos(half) })
end

-- Public math API
box3d.v3 = v3
box3d.v3_add = v3_add; box3d.v3_sub = v3_sub; box3d.v3_scale = v3_scale
box3d.v3_dot = v3_dot; box3d.v3_cross = v3_cross; box3d.v3_len = v3_len
box3d.v3_len_sqr = v3_len_sqr; box3d.v3_normalize = v3_normalize
box3d.v3_min = v3_min; box3d.v3_max = v3_max; box3d.v3_abs = v3_abs
box3d.v3_lerp = v3_lerp; box3d.v3_mul = v3_mul; box3d.v3_neg = v3_neg
box3d.quat_identity = quat_identity; box3d.quat_mul = quat_mul
box3d.quat_normalize = quat_normalize; box3d.quat_rotate = quat_rotate
box3d.quat_inv_rotate = quat_inv_rotate; box3d.quat_to_mat3 = quat_to_mat3
box3d.quat_slerp = quat_slerp; box3d.quat_to_euler_degrees = quat_to_euler_degrees
box3d.make_quat_from_axis_angle = make_quat_from_axis_angle
box3d.transform_point = transform_point; box3d.inv_transform_point = inv_transform_point

-- ======================================================= Soft Constraint --
-- Port of b3MakeSoft (solver.h): TGS soft constraint coefficients.
-- biasRate, massScale, impulseScale for contact and joint springs.
-- Called at each world step with (hertz, dampingRatio, h).

function box3d.make_soft(hertz, zeta, h)
  if hertz == 0 then
    return { bias_rate = 0, mass_scale = 0, impulse_scale = 0 }
  end
  local omega = 2.0 * math.pi * hertz
  local a1 = 2.0 * zeta + h * omega
  local a2 = h * omega * a1
  local a3 = 1.0 / (1.0 + a2)
  return {
    bias_rate = omega / a1,
    mass_scale = a2 * a3,
    impulse_scale = a3,
  }
end

-- =========================================================== Dynamics --
-- Port of solver.c: integrate_velocities, integrate_positions

-- Apply inverse inertia in world space: R * invI_local * R^T * v
-- Port of the b3Body_GetInverseInertiaWorld path
local function apply_inv_inertia(body, v)
  local local_v = quat_inv_rotate(body.orientation, v)
  local il = body.inv_inertia_local
  local scaled = v3(
    local_v.x * il.xx + local_v.y * il.xy + local_v.z * il.xz,
    local_v.x * il.xy + local_v.y * il.yy + local_v.z * il.yz,
    local_v.x * il.xz + local_v.y * il.yz + local_v.z * il.zz
  )
  return quat_rotate(body.orientation, scaled)
end

-- Integrate angular velocity with torque and damping.
-- Ported from b3IntegrateVelocitiesTask (solver.c):
--   v2 = v1 + h * invInertia * torque
--   v2 *= 1 / (1 + h * damping)   (Pade approximation of exponential damping)
-- Torque is optional (nil for none). Damping defaults to 0.
function box3d.integrate_angular_velocity(body, torque, h, angular_damping)
  local w = body.angular_velocity
  if torque then
    local dw = v3_scale(apply_inv_inertia(body, torque), h)
    w = v3_add(w, dw)
  end
  local damping = 1.0 / (1.0 + h * (angular_damping or 0))
  body.angular_velocity = v3_scale(w, damping)
end

-- Integrate rotation: q2 = integrate(q1, h * w)
-- Ported from b3IntegratePositionsTask (solver.c):
--   deltaRotation = h * angularVelocity
--   q2 = b3IntegrateRotation(q1, deltaRotation)
function box3d.integrate_rotation(body, h)
  body.orientation = quat_integrate(body.orientation, v3_scale(body.angular_velocity, h))
end

-- ============================================================ Contact --
-- Single-contact-point solve with soft constraint, restitution, friction.
-- Ported from the scalar core of contact_solver.c normal-impulse loop.
-- Uses the original box3d signature: operates on one body against
-- a static surface (normal points into the body). The caller (main.lua)
-- manages body position externally.
-- Returns updated linear velocity; body.angular_velocity is mutated.

function box3d.solve_contact(body, body_position, point, normal, separation, velocity, softness, restitution, friction, max_bias_velocity, restitution_threshold)
  local r = v3_sub(point, body_position)
  local rn = v3_cross(r, normal)
  local inv_mass_normal = body.inv_mass + v3_dot(rn, apply_inv_inertia(body, rn))
  if inv_mass_normal <= 1e-9 then return velocity end
  local normal_mass = 1.0 / inv_mass_normal

  local point_velocity = v3_add(velocity, v3_cross(body.angular_velocity, r))
  local vn = v3_dot(point_velocity, normal)

  -- Soft constraint bias: b3MakeSoft -> biasRate * separation, capped
  local bias = math.max(softness.bias_rate * separation, -max_bias_velocity)
  local mass_scale = softness.mass_scale

  -- Restitution: if approaching fast enough, add bounce bias
  local threshold = restitution_threshold or (1.0 * (1.0 / 20.0)) -- default: 1.0 m/s in tick-time (0.05)
  if restitution > 0 and vn < -threshold then
    bias = math.min(bias, restitution * vn)
  end

  local delta_impulse = -normal_mass * (mass_scale * vn + bias)
  delta_impulse = math.max(delta_impulse, 0)

  local impulse_vec = v3_scale(normal, delta_impulse)
  velocity = v3_add(velocity, v3_scale(impulse_vec, body.inv_mass))
  body.angular_velocity = v3_add(body.angular_velocity, apply_inv_inertia(body, v3_cross(r, impulse_vec)))

  -- Coulomb friction
  if friction > 0 and delta_impulse > 0 then
    local tangent_v = v3_sub(point_velocity, v3_scale(normal, vn))
    local tangent_speed = v3_len(tangent_v)
    if tangent_speed > 1e-6 then
      local tangent = v3_scale(tangent_v, 1.0 / tangent_speed)
      local rt = v3_cross(r, tangent)
      local inv_mass_tangent = body.inv_mass + v3_dot(rt, apply_inv_inertia(body, rt))
      if inv_mass_tangent > 1e-9 then
        local tangent_mass = 1.0 / inv_mass_tangent
        local vt = v3_dot(point_velocity, tangent)
        local friction_impulse = -tangent_mass * vt
        local max_friction = friction * delta_impulse
        friction_impulse = math.max(-max_friction, math.min(friction_impulse, max_friction))
        local friction_vec = v3_scale(tangent, friction_impulse)
        velocity = v3_add(velocity, v3_scale(friction_vec, body.inv_mass))
        body.angular_velocity = v3_add(body.angular_velocity, apply_inv_inertia(body, v3_cross(r, friction_vec)))
      end
    end
  end

  return velocity
end

-- ============================================================= Shape --
-- Convenience: create a rigid body data structure for a uniform box.

function box3d.new_box(half_x, half_y, half_z, mass)
  mass = mass or 1.0
  local ix = (mass / 3.0) * (half_y * half_y + half_z * half_z)
  local iy = (mass / 3.0) * (half_x * half_x + half_z * half_z)
  local iz = (mass / 3.0) * (half_x * half_x + half_y * half_y)
  return {
    half = v3(half_x, half_y, half_z),
    inv_mass = mass > 0 and (1.0 / mass) or 0.0,
    inv_inertia_local = {
      xx = ix > 1e-9 and 1.0 / ix or 0.0,
      yy = iy > 1e-9 and 1.0 / iy or 0.0,
      zz = iz > 1e-9 and 1.0 / iz or 0.0,
      xy = 0, xz = 0, yz = 0,
    },
    orientation = quat_identity(),
    angular_velocity = v3(0, 0, 0),
  }
end

-- Invert a symmetric 3x3 matrix (used by new_voxel_body)
local function mat3_invert_sym(m)
  local det = m.xx * (m.yy * m.zz - m.yz * m.yz)
            - m.xy * (m.xy * m.zz - m.yz * m.xz)
            + m.xz * (m.xy * m.yz - m.yy * m.xz)
  if math.abs(det) < 1e-9 then
    return { xx = 0, yy = 0, zz = 0, xy = 0, xz = 0, yz = 0 }
  end
  local inv = 1.0 / det
  return {
    xx = (m.yy * m.zz - m.yz * m.yz) * inv,
    yy = (m.xx * m.zz - m.xz * m.xz) * inv,
    zz = (m.xx * m.yy - m.xy * m.xy) * inv,
    xy = (m.xz * m.yz - m.xy * m.zz) * inv,
    xz = (m.xy * m.yz - m.yy * m.xz) * inv,
    yz = (m.xy * m.xz - m.xx * m.yz) * inv,
  }
end
box3d.mat3_invert_sym = mat3_invert_sym

-- Voxel body from texture pixel data.
-- Computes per-pixel inertia based on alpha > 0 (transparency-based mass).
-- Returns (body_data, com_offset) where com_offset is the center of mass
-- relative to the texture center. The body position tracks the COM;
-- the render offset shifts the visual back to the texture center.
function box3d.new_voxel_body(pixels, width, height, scale, mass, thickness)
  mass = mass or 1.0
  thickness = thickness or (scale / width)
  local solid_count = 0
  local cx, cy = 0, 0

  for y = 0, height - 1 do
    for x = 0, width - 1 do
      local c = pixels[1 + y * width + x]
      local alpha = (c >> 24) & 0xFF
      if alpha > 0 then
        solid_count = solid_count + 1
        cx = cx + x
        cy = cy + y
      end
    end
  end

  if solid_count == 0 then
    return box3d.new_box(scale / 2, scale / 2, thickness / 2, mass), v3(0, 0, 0)
  end

  cx = cx / solid_count
  cy = cy / solid_count

  local dx = scale / width
  local dy = scale / height
  -- Texture spans [-scale/2, scale/2] in x and y.
  -- Pixel x=0 is at -scale/2, pixel x=width-1 is at scale/2 - dx.
  -- COM relative to texture center (0, 0, 0):
  local com_x = -scale / 2 + (cx + 0.5) * dx
  local com_y = scale / 2 - (cy + 0.5) * dy
  local com_offset = v3(com_x, com_y, 0)

  local voxel_mass = mass / solid_count
  local ix, iy, iz, ixy = 0, 0, 0, 0

  local i0_x = voxel_mass / 12 * (dy * dy + thickness * thickness)
  local i0_y = voxel_mass / 12 * (dx * dx + thickness * thickness)
  local i0_z = voxel_mass / 12 * (dx * dx + dy * dy)

  for y = 0, height - 1 do
    for x = 0, width - 1 do
      local c = pixels[1 + y * width + x]
      local alpha = (c >> 24) & 0xFF
      if alpha > 0 then
        -- Voxel position relative to COM
        local px = -scale / 2 + (x + 0.5) * dx - com_x
        local py = scale / 2 - (y + 0.5) * dy - com_y

        -- Parallel axis theorem: I = I_cm + m * d^2
        ix = ix + i0_x + voxel_mass * py * py
        iy = iy + i0_y + voxel_mass * px * px
        iz = iz + i0_z + voxel_mass * (px * px + py * py)
        ixy = ixy - voxel_mass * px * py
      end
    end
  end

  local inertia = { xx = ix, yy = iy, zz = iz, xy = ixy, xz = 0, yz = 0 }

  return {
    half = v3(scale / 2, scale / 2, thickness / 2),
    inv_mass = mass > 0 and (1.0 / mass) or 0.0,
    inv_inertia_local = mat3_invert_sym(inertia),
    orientation = quat_identity(),
    angular_velocity = v3(0, 0, 0),
  }, com_offset
end

-- ===================================================== AABB Slide --
-- Axis-separated swept-AABB collision resolution. This mirrors the clipping
-- used by Minecraft's own entity boxes: each axis is clamped against blocks
-- that overlap on the other two axes. Unlike testing only the final AABB, this
-- cannot tunnel through a block when the requested displacement crosses it.

local function ranges_overlap(a0, a1, b0, b1)
  return a1 > b0 and a0 < b1
end

local function translate_aabb(a, dx, dy, dz)
  a.min_x = a.min_x + dx; a.max_x = a.max_x + dx
  a.min_y = a.min_y + dy; a.max_y = a.max_y + dy
  a.min_z = a.min_z + dz; a.max_z = a.max_z + dz
end

function box3d.aabb_slide(aabb, dx, dy, dz, collisions)
  local next = {
    min_x = aabb.min_x, min_y = aabb.min_y, min_z = aabb.min_z,
    max_x = aabb.max_x, max_y = aabb.max_y, max_z = aabb.max_z,
  }

  if dy ~= 0 then
    for _, b in ipairs(collisions) do
      if ranges_overlap(next.min_x, next.max_x, b.min_x, b.max_x) and
         ranges_overlap(next.min_z, next.max_z, b.min_z, b.max_z) then
        if dy > 0 and next.max_y <= b.min_y then
          local gap = b.min_y - next.max_y
          if gap < dy then dy = gap end
        elseif dy < 0 and next.min_y >= b.max_y then
          local gap = b.max_y - next.min_y
          if gap > dy then dy = gap end
        end
      end
    end
    translate_aabb(next, 0, dy, 0)
  end

  if dx ~= 0 then
    for _, b in ipairs(collisions) do
      if ranges_overlap(next.min_y, next.max_y, b.min_y, b.max_y) and
         ranges_overlap(next.min_z, next.max_z, b.min_z, b.max_z) then
        if dx > 0 and next.max_x <= b.min_x then
          local gap = b.min_x - next.max_x
          if gap < dx then dx = gap end
        elseif dx < 0 and next.min_x >= b.max_x then
          local gap = b.max_x - next.min_x
          if gap > dx then dx = gap end
        end
      end
    end
    translate_aabb(next, dx, 0, 0)
  end

  if dz ~= 0 then
    for _, b in ipairs(collisions) do
      if ranges_overlap(next.min_x, next.max_x, b.min_x, b.max_x) and
         ranges_overlap(next.min_y, next.max_y, b.min_y, b.max_y) then
        if dz > 0 and next.max_z <= b.min_z then
          local gap = b.min_z - next.max_z
          if gap < dz then dz = gap end
        elseif dz < 0 and next.min_z >= b.max_z then
          local gap = b.max_z - next.min_z
          if gap > dz then dz = gap end
        end
      end
    end
  end

  return dx, dy, dz
end

-- Rotation can enlarge an item's projected AABB while it is already touching
-- a floor or wall. aabb_slide deliberately does not resolve pre-existing
-- overlap, so perform a small minimum-translation depenetration first. The
-- correction is capped because a dropped item should never be deeply embedded;
-- a huge correction would look like the very teleport this is intended to fix.
function box3d.aabb_depenetrate(aabb, collisions, max_iterations, max_total)
  max_iterations = max_iterations or 4
  max_total = max_total or 0.5

  local next = {
    min_x = aabb.min_x, min_y = aabb.min_y, min_z = aabb.min_z,
    max_x = aabb.max_x, max_y = aabb.max_y, max_z = aabb.max_z,
  }
  local total_x, total_y, total_z = 0, 0, 0

  for _ = 1, max_iterations do
    local best_axis, best_move, best_abs = nil, 0, math.huge

    for _, b in ipairs(collisions) do
      if ranges_overlap(next.min_x, next.max_x, b.min_x, b.max_x) and
         ranges_overlap(next.min_y, next.max_y, b.min_y, b.max_y) and
         ranges_overlap(next.min_z, next.max_z, b.min_z, b.max_z) then
        local candidates = {
          { "x", b.min_x - next.max_x },
          { "x", b.max_x - next.min_x },
          { "y", b.min_y - next.max_y },
          { "y", b.max_y - next.min_y },
          { "z", b.min_z - next.max_z },
          { "z", b.max_z - next.min_z },
        }
        for _, c in ipairs(candidates) do
          local a = math.abs(c[2])
          -- Prefer an upward correction on near-ties so floor contacts do not
          -- randomly shove an item sideways at block seams.
          local preferred = c[1] == "y" and c[2] > 0
          local best_preferred = best_axis == "y" and best_move > 0
          if a < best_abs - 1e-7 or (math.abs(a - best_abs) <= 1e-7 and preferred and not best_preferred) then
            best_axis, best_move, best_abs = c[1], c[2], a
          end
        end
      end
    end

    if not best_axis then break end

    local remaining = max_total - math.sqrt(total_x * total_x + total_y * total_y + total_z * total_z)
    if remaining <= 0 then break end
    if math.abs(best_move) > remaining then
      best_move = best_move < 0 and -remaining or remaining
    end

    if best_axis == "x" then
      total_x = total_x + best_move
      translate_aabb(next, best_move, 0, 0)
    elseif best_axis == "y" then
      total_y = total_y + best_move
      translate_aabb(next, 0, best_move, 0)
    else
      total_z = total_z + best_move
      translate_aabb(next, 0, 0, best_move)
    end
  end

  return total_x, total_y, total_z
end

return box3d
