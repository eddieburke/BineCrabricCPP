local box3d = {}

-- Minimal 3D rigid-body core for oriented boxes. Positions and linear velocities
-- are center-of-mass quantities. A non-zero com_offset locates the visual/shape
-- origin relative to the center of mass.

local abs, min, max, sqrt = math.abs, math.min, math.max, math.sqrt
local floor, pi = math.floor, math.pi
local atan2 = math.atan2
if not atan2 then
  atan2 = function(y, x)
    if x > 0 then return math.atan(y / x) end
    if x < 0 then return math.atan(y / x) + (y >= 0 and pi or -pi) end
    if y > 0 then return 0.5 * pi end
    if y < 0 then return -0.5 * pi end
    return 0.0
  end
end
local EPS = 1.0e-9

local function v3(x, y, z)
  return { x = x or 0.0, y = y or 0.0, z = z or 0.0 }
end
local ZERO = { x = 0.0, y = 0.0, z = 0.0 }

local function add(a, b) return v3(a.x + b.x, a.y + b.y, a.z + b.z) end
local function sub(a, b) return v3(a.x - b.x, a.y - b.y, a.z - b.z) end
local function scale(a, s) return v3(a.x * s, a.y * s, a.z * s) end
local function neg(a) return v3(-a.x, -a.y, -a.z) end
local function dot(a, b) return a.x * b.x + a.y * b.y + a.z * b.z end
local function cross(a, b)
  return v3(
    a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x
  )
end
local function length2(a) return dot(a, a) end
local function length(a) return sqrt(length2(a)) end
local function normalize(a)
  local ls = length2(a)
  if ls <= EPS * EPS then return v3(0, 0, 0) end
  return scale(a, 1.0 / sqrt(ls))
end
local function lerp(a, b, t)
  return v3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t)
end

local function quat_identity() return { x = 0, y = 0, z = 0, w = 1 } end
local function quat_normalize(q)
  local ls = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w
  if ls <= EPS * EPS then return quat_identity() end
  local inv = 1.0 / sqrt(ls)
  return { x = q.x * inv, y = q.y * inv, z = q.z * inv, w = q.w * inv }
end
local function quat_mul(a, b)
  return {
    x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
    y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
    z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
    w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
  }
end
local function quat_rotate(q, p)
  local tx = 2.0 * (q.y * p.z - q.z * p.y)
  local ty = 2.0 * (q.z * p.x - q.x * p.z)
  local tz = 2.0 * (q.x * p.y - q.y * p.x)
  return v3(
    p.x + q.w * tx + q.y * tz - q.z * ty,
    p.y + q.w * ty + q.z * tx - q.x * tz,
    p.z + q.w * tz + q.x * ty - q.y * tx
  )
end
local function quat_inv_rotate(q, p)
  return quat_rotate({ x = -q.x, y = -q.y, z = -q.z, w = q.w }, p)
end
local function quat_integrate(q, w, h)
  local s = 0.5 * h
  return quat_normalize({
    x = q.x + s * ( w.x * q.w + w.y * q.z - w.z * q.y),
    y = q.y + s * (-w.x * q.z + w.y * q.w + w.z * q.x),
    z = q.z + s * ( w.x * q.y - w.y * q.x + w.z * q.w),
    w = q.w - s * ( w.x * q.x + w.y * q.y + w.z * q.z),
  })
end
local function quat_slerp(a, b, t)
  local d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w
  if d < 0 then
    b = { x = -b.x, y = -b.y, z = -b.z, w = -b.w }
    d = -d
  end
  if d > 0.9995 then
    return quat_normalize({
      x = a.x + (b.x - a.x) * t,
      y = a.y + (b.y - a.y) * t,
      z = a.z + (b.z - a.z) * t,
      w = a.w + (b.w - a.w) * t,
    })
  end
  d = max(-1.0, min(1.0, d))
  local a0 = math.acos(d)
  local sa0 = math.sin(a0)
  if abs(sa0) <= EPS then return a end
  local a1 = a0 * t
  local s0 = math.sin(a0 - a1) / sa0
  local s1 = math.sin(a1) / sa0
  return {
    x = a.x * s0 + b.x * s1,
    y = a.y * s0 + b.y * s1,
    z = a.z * s0 + b.z * s1,
    w = a.w * s0 + b.w * s1,
  }
end
local function quat_to_euler_degrees(q)
  local xx, yy, zz = q.x * q.x, q.y * q.y, q.z * q.z
  local pitch = math.asin(max(-1.0, min(1.0, 2.0 * (q.w * q.x - q.y * q.z))))
  local roll = atan2(2.0 * (q.x * q.y + q.w * q.z), 1.0 - 2.0 * (xx + zz))
  local yaw = atan2(2.0 * (q.x * q.z + q.w * q.y), 1.0 - 2.0 * (xx + yy))
  local k = 180.0 / math.pi
  return yaw * k, pitch * k, roll * k
end
local function make_quat_from_axis_angle(axis, radians)
  axis = normalize(axis)
  local h = 0.5 * radians
  local s = math.sin(h)
  return quat_normalize({ x = axis.x * s, y = axis.y * s, z = axis.z * s, w = math.cos(h) })
end

box3d.v3 = v3
box3d.v3_add, box3d.v3_sub, box3d.v3_scale = add, sub, scale
box3d.v3_dot, box3d.v3_cross = dot, cross
box3d.v3_len, box3d.v3_len_sqr = length, length2
box3d.v3_normalize, box3d.v3_lerp = normalize, lerp
box3d.quat_identity, box3d.quat_normalize = quat_identity, quat_normalize
box3d.quat_mul, box3d.quat_rotate, box3d.quat_inv_rotate = quat_mul, quat_rotate, quat_inv_rotate
box3d.quat_slerp, box3d.quat_to_euler_degrees = quat_slerp, quat_to_euler_degrees
box3d.make_quat_from_axis_angle = make_quat_from_axis_angle

function box3d.make_soft(hertz, damping_ratio, h)
  if hertz <= 0 or h <= 0 then
    return { bias_rate = 0, mass_scale = 1, impulse_scale = 0 }
  end
  local omega = 2.0 * math.pi * hertz
  local a1 = 2.0 * damping_ratio + h * omega
  local a2 = h * omega * a1
  local a3 = 1.0 / (1.0 + a2)
  return {
    bias_rate = omega / a1,
    mass_scale = a2 * a3,
    impulse_scale = a3,
  }
end

local function invert_symmetric(m)
  local det = m.xx * (m.yy * m.zz - m.yz * m.yz)
            - m.xy * (m.xy * m.zz - m.yz * m.xz)
            + m.xz * (m.xy * m.yz - m.yy * m.xz)
  if abs(det) <= EPS then
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

local function sync_body_cache(body)
  local q = body.orientation
  if body._qx == q.x and body._qy == q.y and body._qz == q.z and body._qw == q.w then
    return body._axes, body._inv_world
  end

  local x, y, z, w = q.x, q.y, q.z, q.w
  local xx, yy, zz = x * x, y * y, z * z
  local xy, xz, yz = x * y, x * z, y * z
  local wx, wy, wz = w * x, w * y, w * z
  local m00, m01, m02 = 1.0 - 2.0 * (yy + zz), 2.0 * (xy - wz), 2.0 * (xz + wy)
  local m10, m11, m12 = 2.0 * (xy + wz), 1.0 - 2.0 * (xx + zz), 2.0 * (yz - wx)
  local m20, m21, m22 = 2.0 * (xz - wy), 2.0 * (yz + wx), 1.0 - 2.0 * (xx + yy)

  local axes = body._axes
  if not axes then
    axes = { v3(), v3(), v3() }
    body._axes = axes
  end
  axes[1].x, axes[1].y, axes[1].z = m00, m10, m20
  axes[2].x, axes[2].y, axes[2].z = m01, m11, m21
  axes[3].x, axes[3].y, axes[3].z = m02, m12, m22

  local il = body.inv_inertia_local
  local ax, ay, az = axes[1], axes[2], axes[3]
  local iw = body._inv_world
  if not iw then
    iw = { xx = 0, yy = 0, zz = 0, xy = 0, xz = 0, yz = 0 }
    body._inv_world = iw
  end

  local ixx, iyy, izz = il.xx, il.yy, il.zz
  local ixy, ixz, iyz = il.xy, il.xz, il.yz
  local axx, axy, axz = ax.x, ax.y, ax.z
  local ayx, ayy, ayz = ay.x, ay.y, ay.z
  local azx, azy, azz = az.x, az.y, az.z
  iw.xx = ixx * axx * axx + iyy * ayx * ayx + izz * azx * azx
        + 2.0 * (ixy * axx * ayx + ixz * axx * azx + iyz * ayx * azx)
  iw.yy = ixx * axy * axy + iyy * ayy * ayy + izz * azy * azy
        + 2.0 * (ixy * axy * ayy + ixz * axy * azy + iyz * ayy * azy)
  iw.zz = ixx * axz * axz + iyy * ayz * ayz + izz * azz * azz
        + 2.0 * (ixy * axz * ayz + ixz * axz * azz + iyz * ayz * azz)
  iw.xy = ixx * axx * axy + iyy * ayx * ayy + izz * azx * azy
        + ixy * (axx * ayy + ayx * axy)
        + ixz * (axx * azy + azx * axy)
        + iyz * (ayx * azy + azx * ayy)
  iw.xz = ixx * axx * axz + iyy * ayx * ayz + izz * azx * azz
        + ixy * (axx * ayz + ayx * axz)
        + ixz * (axx * azz + azx * axz)
        + iyz * (ayx * azz + azx * ayz)
  iw.yz = ixx * axy * axz + iyy * ayy * ayz + izz * azy * azz
        + ixy * (axy * ayz + ayy * axz)
        + ixz * (axy * azz + azy * axz)
        + iyz * (ayy * azz + azy * ayz)

  body._qx, body._qy, body._qz, body._qw = q.x, q.y, q.z, q.w
  return axes, iw
end
box3d.sync_body_cache = sync_body_cache

local function apply_inv_inertia(body, world_vector)
  if not body or body.inv_mass <= 0 then return v3(0, 0, 0) end
  local _, m = sync_body_cache(body)
  return v3(
    m.xx * world_vector.x + m.xy * world_vector.y + m.xz * world_vector.z,
    m.xy * world_vector.x + m.yy * world_vector.y + m.yz * world_vector.z,
    m.xz * world_vector.x + m.yz * world_vector.y + m.zz * world_vector.z
  )
end
box3d.apply_inv_inertia = apply_inv_inertia

function box3d.new_box(half_x, half_y, half_z, mass)
  mass = max(0.0, mass or 1.0)
  local ix = (mass / 3.0) * (half_y * half_y + half_z * half_z)
  local iy = (mass / 3.0) * (half_x * half_x + half_z * half_z)
  local iz = (mass / 3.0) * (half_x * half_x + half_y * half_y)
  local body = {
    half = v3(half_x, half_y, half_z),
    mass = mass,
    inv_mass = mass > EPS and 1.0 / mass or 0.0,
    inertia_local = { xx = ix, yy = iy, zz = iz, xy = 0, xz = 0, yz = 0 },
    inv_inertia_local = {
      xx = ix > EPS and 1.0 / ix or 0,
      yy = iy > EPS and 1.0 / iy or 0,
      zz = iz > EPS and 1.0 / iz or 0,
      xy = 0, xz = 0, yz = 0,
    },
    orientation = quat_identity(),
    angular_velocity = v3(0, 0, 0),
  }
  sync_body_cache(body)
  return body
end

function box3d.new_voxel_body(pixels, width, height, scale_value, mass, thickness)
  mass = max(EPS, mass or 1.0)
  thickness = thickness or scale_value / max(width, 1)
  local count, sx, sy = 0, 0, 0
  for y = 0, height - 1 do
    for x = 0, width - 1 do
      local c = pixels[1 + y * width + x]
      if c and ((floor(c / 16777216) % 256) > 0) then
        count, sx, sy = count + 1, sx + x, sy + y
      end
    end
  end
  if count == 0 then
    return box3d.new_box(scale_value * 0.5, scale_value * 0.5, thickness * 0.5, mass), v3(0, 0, 0)
  end

  local dx, dy = scale_value / width, scale_value / height
  local cx, cy = sx / count, sy / count
  local com_x = -0.5 * scale_value + (cx + 0.5) * dx
  local com_y =  0.5 * scale_value - (cy + 0.5) * dy
  local voxel_mass = mass / count
  local ix0 = voxel_mass * (dy * dy + thickness * thickness) / 12.0
  local iy0 = voxel_mass * (dx * dx + thickness * thickness) / 12.0
  local iz0 = voxel_mass * (dx * dx + dy * dy) / 12.0
  local ix, iy, iz, ixy = 0, 0, 0, 0

  for y = 0, height - 1 do
    for x = 0, width - 1 do
      local c = pixels[1 + y * width + x]
      if c and ((floor(c / 16777216) % 256) > 0) then
        local px = -0.5 * scale_value + (x + 0.5) * dx - com_x
        local py =  0.5 * scale_value - (y + 0.5) * dy - com_y
        ix = ix + ix0 + voxel_mass * py * py
        iy = iy + iy0 + voxel_mass * px * px
        iz = iz + iz0 + voxel_mass * (px * px + py * py)
        ixy = ixy - voxel_mass * px * py
      end
    end
  end

  local inertia = { xx = ix, yy = iy, zz = iz, xy = ixy, xz = 0, yz = 0 }
  local body = {
    half = v3(scale_value * 0.5, scale_value * 0.5, thickness * 0.5),
    mass = mass,
    inv_mass = 1.0 / mass,
    inertia_local = inertia,
    inv_inertia_local = invert_symmetric(inertia),
    orientation = quat_identity(),
    angular_velocity = v3(0, 0, 0),
  }
  sync_body_cache(body)
  return body, v3(com_x, com_y, 0)
end

function box3d.integrate_rotation(body, h)
  body.orientation = quat_integrate(body.orientation, body.angular_velocity, h)
  sync_body_cache(body)
end

local function body_axes(body)
  return sync_body_cache(body)
end

local function make_body_obb(position, body, com_offset)
  com_offset = com_offset or v3(0, 0, 0)
  local axes = sync_body_cache(body)
  local ax, ay, az = axes[1], axes[2], axes[3]
  local ox, oy, oz = com_offset.x, com_offset.y, com_offset.z
  local wx = ax.x * ox + ay.x * oy + az.x * oz
  local wy = ax.y * ox + ay.y * oy + az.y * oz
  local wz = ax.z * ox + ay.z * oy + az.z * oz
  return {
    center = v3(position.x - wx, position.y - wy, position.z - wz),
    half = body.half,
    axes = axes,
  }
end

local function make_aabb_obb(aabb)
  return {
    center = v3(
      0.5 * (aabb.min_x + aabb.max_x),
      0.5 * (aabb.min_y + aabb.max_y),
      0.5 * (aabb.min_z + aabb.max_z)
    ),
    half = v3(
      0.5 * (aabb.max_x - aabb.min_x),
      0.5 * (aabb.max_y - aabb.min_y),
      0.5 * (aabb.max_z - aabb.min_z)
    ),
    axes = { v3(1, 0, 0), v3(0, 1, 0), v3(0, 0, 1) },
  }
end

local function radius_on_axis(obb, axis)
  return obb.half.x * abs(dot(obb.axes[1], axis))
       + obb.half.y * abs(dot(obb.axes[2], axis))
       + obb.half.z * abs(dot(obb.axes[3], axis))
end

local function obb_vertices(obb)
  local out = {}
  for i = 0, 7 do
    local sx = (i % 2) == 0 and -1 or 1
    local sy = (floor(i / 2) % 2) == 0 and -1 or 1
    local sz = (floor(i / 4) % 2) == 0 and -1 or 1
    local p = obb.center
    p = add(p, scale(obb.axes[1], sx * obb.half.x))
    p = add(p, scale(obb.axes[2], sy * obb.half.y))
    p = add(p, scale(obb.axes[3], sz * obb.half.z))
    out[#out + 1] = { point = p, id = i }
  end
  return out
end

local function point_inside_obb(p, obb, tolerance)
  local d = sub(p, obb.center)
  tolerance = tolerance or 0.0
  return abs(dot(d, obb.axes[1])) <= obb.half.x + tolerance
     and abs(dot(d, obb.axes[2])) <= obb.half.y + tolerance
     and abs(dot(d, obb.axes[3])) <= obb.half.z + tolerance
end

local function support(obb, direction)
  local p = obb.center
  local h = obb.half
  p = add(p, scale(obb.axes[1], dot(obb.axes[1], direction) >= 0 and h.x or -h.x))
  p = add(p, scale(obb.axes[2], dot(obb.axes[2], direction) >= 0 and h.y or -h.y))
  p = add(p, scale(obb.axes[3], dot(obb.axes[3], direction) >= 0 and h.z or -h.z))
  return p
end

local function test_sat_axis(a, b, delta, raw_axis, kind, index, margin, best)
  local ls = length2(raw_axis)
  if ls <= 1.0e-12 then return best, true end
  local axis = scale(raw_axis, 1.0 / sqrt(ls))
  local distance = dot(delta, axis)
  local separation = abs(distance) - radius_on_axis(a, axis) - radius_on_axis(b, axis)
  if separation > margin then return best, false end
  if distance < 0 then axis = neg(axis) end -- A -> B

  if not best
     or separation > best.separation + 1.0e-5
     or (abs(separation - best.separation) <= 1.0e-5 and best.kind == "edge" and kind ~= "edge") then
    best = { normal = axis, separation = separation, kind = kind, index = index }
  end
  return best, true
end

local function sat_obb(a, b, margin)
  margin = margin or 0.0
  local delta = sub(b.center, a.center)
  local best
  for i = 1, 3 do
    local ok
    best, ok = test_sat_axis(a, b, delta, a.axes[i], "face_a", i, margin, best)
    if not ok then return nil end
  end
  for i = 1, 3 do
    local ok
    best, ok = test_sat_axis(a, b, delta, b.axes[i], "face_b", i, margin, best)
    if not ok then return nil end
  end
  local edge_index = 0
  for i = 1, 3 do
    for j = 1, 3 do
      edge_index = edge_index + 1
      local ok
      best, ok = test_sat_axis(a, b, delta, cross(a.axes[i], b.axes[j]), "edge", edge_index, margin, best)
      if not ok then return nil end
    end
  end
  return best
end

local function tangent_basis(n)
  local reference = abs(n.x) < 0.57735 and v3(1, 0, 0)
                 or (abs(n.y) < 0.57735 and v3(0, 1, 0) or v3(0, 0, 1))
  local t1 = normalize(cross(n, reference))
  local t2 = cross(n, t1)
  return t1, t2
end

local function deduplicate_contacts(candidates)
  local unique = {}
  for i = 1, #candidates do
    local c = candidates[i]
    local duplicate = false
    for j = 1, #unique do
      if length2(sub(c.point, unique[j].point)) < 1.0e-8 then
        duplicate = true
        break
      end
    end
    if not duplicate then unique[#unique + 1] = c end
  end
  return unique
end

local function reduce_contacts(candidates, normal)
  candidates = deduplicate_contacts(candidates)
  if #candidates <= 4 then return candidates end
  local t1, t2 = tangent_basis(normal)
  local selected, used = {}, {}
  local function choose(sign1, sign2)
    local best_i, best_value
    for i = 1, #candidates do
      if not used[i] then
        local p = candidates[i].point
        local value = sign1 * dot(p, t1) + sign2 * dot(p, t2)
        if not best_value or value > best_value then best_i, best_value = i, value end
      end
    end
    if best_i then
      used[best_i] = true
      selected[#selected + 1] = candidates[best_i]
    end
  end
  choose( 1,  1)
  choose(-1,  1)
  choose(-1, -1)
  choose( 1, -1)
  return selected
end

local function build_manifold(a, b, sat, margin, single_contact)
  local n, separation = sat.normal, sat.separation
  local feature_base = sat.kind == "face_a" and 0 or (sat.kind == "face_b" and 8 or 16)
  if single_contact then
    local pa = support(a, n)
    local pb = support(b, neg(n))
    return {
      normal = n,
      separation = separation,
      feature = feature_base + sat.index,
      contacts = { {
        point = scale(add(pa, pb), 0.5),
        id = 24 + sat.index,
        separation = separation,
      } },
    }
  end
  local candidates = {}
  local va, vb = obb_vertices(a), obb_vertices(b)
  local support_a = dot(support(a, n), n)
  local support_b = dot(support(b, neg(n)), n)
  local band = max(0.0025, margin or 0.0) + max(0.0, -separation) * 0.25

  for i = 1, #va do
    local vertex = va[i]
    if dot(vertex.point, n) >= support_a - band and point_inside_obb(vertex.point, b, max(0.001, margin or 0)) then
      candidates[#candidates + 1] = {
        point = add(vertex.point, scale(n, 0.5 * separation)),
        id = vertex.id,
        separation = separation,
      }
    end
  end
  for i = 1, #vb do
    local vertex = vb[i]
    if dot(vertex.point, n) <= support_b + band and point_inside_obb(vertex.point, a, max(0.001, margin or 0)) then
      candidates[#candidates + 1] = {
        point = sub(vertex.point, scale(n, 0.5 * separation)),
        id = 8 + vertex.id,
        separation = separation,
      }
    end
  end

  if #candidates == 0 then
    local pa = support(a, n)
    local pb = support(b, neg(n))
    candidates[1] = {
      point = scale(add(pa, pb), 0.5),
      id = 24 + sat.index,
      separation = separation,
    }
  end

  return {
    normal = n,
    separation = separation,
    feature = feature_base + sat.index,
    contacts = reduce_contacts(candidates, n),
  }
end

function box3d.body_aabb(position, body, com_offset, inflate, out)
  local obb = make_body_obb(position, body, com_offset)
  local axes, half = obb.axes, obb.half
  local ex = half.x * abs(axes[1].x) + half.y * abs(axes[2].x) + half.z * abs(axes[3].x)
  local ey = half.x * abs(axes[1].y) + half.y * abs(axes[2].y) + half.z * abs(axes[3].y)
  local ez = half.x * abs(axes[1].z) + half.y * abs(axes[2].z) + half.z * abs(axes[3].z)
  inflate = inflate or 0.0
  out = out or {}
  out.min_x, out.max_x = obb.center.x - ex - inflate, obb.center.x + ex + inflate
  out.min_y, out.max_y = obb.center.y - ey - inflate, obb.center.y + ey + inflate
  out.min_z, out.max_z = obb.center.z - ez - inflate, obb.center.z + ez + inflate
  return out
end

function box3d.box_aabb_manifold(position, body, com_offset, aabb, margin)
  local a, b = make_body_obb(position, body, com_offset), make_aabb_obb(aabb)
  local sat = sat_obb(a, b, margin or 0.0)
  return sat and build_manifold(a, b, sat, margin or 0.0) or nil
end

function box3d.box_box_manifold(position_a, body_a, offset_a, position_b, body_b, offset_b, margin, single_contact)
  local a = make_body_obb(position_a, body_a, offset_a)
  local b = make_body_obb(position_b, body_b, offset_b)
  local sat = sat_obb(a, b, margin or 0.0)
  return sat and build_manifold(a, b, sat, margin or 0.0, single_contact) or nil
end

-- Exact translational swept-SAT for a box with fixed orientation over the sweep.
-- Rotation is handled by world sub-stepping.
function box3d.sweep_box_aabb(position, delta_position, body, com_offset, aabb, margin)
  local a, b = make_body_obb(position, body, com_offset), make_aabb_obb(aabb)
  margin = margin or 0.0
  local center_delta = sub(b.center, a.center)
  local relative_motion = neg(delta_position)
  local enter, exit = 0.0, 1.0
  local hit_normal

  local axes = { a.axes[1], a.axes[2], a.axes[3], b.axes[1], b.axes[2], b.axes[3] }
  for i = 1, 3 do
    for j = 1, 3 do axes[#axes + 1] = cross(a.axes[i], b.axes[j]) end
  end

  for i = 1, #axes do
    local raw = axes[i]
    local ls = length2(raw)
    if ls > 1.0e-12 then
      local axis = scale(raw, 1.0 / sqrt(ls))
      local radius = radius_on_axis(a, axis) + radius_on_axis(b, axis) + margin
      local d0 = dot(center_delta, axis)
      local dv = dot(relative_motion, axis)
      if abs(dv) <= EPS then
        if abs(d0) > radius then return nil end
      else
        local t0 = (-radius - d0) / dv
        local t1 = ( radius - d0) / dv
        local normal_at_entry = axis
        if t0 > t1 then
          t0, t1 = t1, t0
          normal_at_entry = neg(axis)
        else
          normal_at_entry = axis
        end
        if t0 > enter then
          enter = t0
          -- normal points from moving A toward static B
          local d_at = d0 + dv * t0
          hit_normal = d_at >= 0 and axis or neg(axis)
        end
        exit = min(exit, t1)
        if enter > exit then return nil end
      end
    end
  end

  if enter < 0 or enter > 1 then return nil end
  return enter, hit_normal
end

local function point_velocity(body, linear_velocity, r)
  return add(linear_velocity, cross(body.angular_velocity, r))
end

local function apply_impulse(body, linear_velocity, r, impulse, sign)
  if not body or body.inv_mass <= 0 then return linear_velocity end
  linear_velocity = add(linear_velocity, scale(impulse, sign * body.inv_mass))
  body.angular_velocity = add(body.angular_velocity, apply_inv_inertia(body, scale(cross(r, impulse), sign)))
  return linear_velocity
end

local function effective_mass(body_a, r_a, body_b, r_b, axis)
  local k = (body_a and body_a.inv_mass or 0) + (body_b and body_b.inv_mass or 0)
  if body_a and body_a.inv_mass > 0 then
    local ra = cross(r_a, axis)
    k = k + dot(ra, apply_inv_inertia(body_a, ra))
  end
  if body_b and body_b.inv_mass > 0 then
    local rb = cross(r_b, axis)
    k = k + dot(rb, apply_inv_inertia(body_b, rb))
  end
  return k > EPS and 1.0 / k or 0.0
end

local function prepare_points(body_a, position_a, velocity_a, body_b, position_b, velocity_b, manifolds, options, single_manifold)
  local points = {}
  local h = max(options.h or 1.0, EPS)
  local softness = options.softness or { bias_rate = 0.2 / h, mass_scale = 1, impulse_scale = 0 }
  local slop = options.slop or 0.0015
  local threshold = options.restitution_threshold or 0.05
  local max_push = options.max_push_speed or 0.15

  local manifold_count = single_manifold and 1 or #manifolds
  for mi = 1, manifold_count do
    local manifold = single_manifold and manifolds or manifolds[mi]
    local n = manifold.normal
    local t1, t2 = tangent_basis(n)
    local friction = max(0.0, manifold.friction or options.friction or 0.6)
    local restitution = max(0.0, manifold.restitution or options.restitution or 0.0)
    for ci = 1, #manifold.contacts do
      local contact = manifold.contacts[ci]
      local r_a = sub(contact.point, position_a)
      local r_b = body_b and sub(contact.point, position_b) or ZERO
      local va = point_velocity(body_a, velocity_a, r_a)
      local vb = body_b and point_velocity(body_b, velocity_b, r_b) or ZERO
      local initial_vn = dot(sub(vb, va), n)
      local separation = contact.separation or manifold.separation or 0.0
      local bias
      if separation > 0 then
        bias = separation / h -- speculative constraint
      else
        bias = max(-max_push, softness.bias_rate * min(separation + slop, 0.0))
      end
      points[#points + 1] = {
        manifold = manifold,
        contact = contact,
        n = n, t1 = t1, t2 = t2,
        r_a = r_a, r_b = r_b,
        normal_mass = effective_mass(body_a, r_a, body_b, r_b, n),
        tangent_mass1 = effective_mass(body_a, r_a, body_b, r_b, t1),
        tangent_mass2 = effective_mass(body_a, r_a, body_b, r_b, t2),
        normal_impulse = contact.normal_impulse or 0.0,
        tangent_impulse1 = contact.tangent_impulse1 or 0.0,
        tangent_impulse2 = contact.tangent_impulse2 or 0.0,
        bias = bias,
        restitution_target = initial_vn < -threshold and (-restitution * initial_vn) or 0.0,
        friction = friction,
        softness = softness,
      }
    end
  end
  return points
end

local function warm_start(body_a, velocity_a, body_b, velocity_b, points)
  for i = 1, #points do
    local p = points[i]
    local impulse = add(
      scale(p.n, p.normal_impulse),
      add(scale(p.t1, p.tangent_impulse1), scale(p.t2, p.tangent_impulse2))
    )
    velocity_a = apply_impulse(body_a, velocity_a, p.r_a, impulse, -1)
    if body_b then velocity_b = apply_impulse(body_b, velocity_b, p.r_b, impulse, 1) end
  end
  return velocity_a, velocity_b
end

local function solve_velocity(body_a, velocity_a, body_b, velocity_b, points, iterations)
  -- Box3D performs one biased solve followed by one unbiased relaxation solve
  -- per sub-step. iterations is retained as a compatibility multiplier, but
  -- callers should normally pass 1.
  iterations = max(1, iterations or 1)

  for _ = 1, iterations do
    -- Biased normal constraints only. Friction is intentionally deferred.
    for i = 1, #points do
      local p = points[i]
      local va = point_velocity(body_a, velocity_a, p.r_a)
      local vb = body_b and point_velocity(body_b, velocity_b, p.r_b) or ZERO
      local vn = dot(sub(vb, va), p.n)
      local soft = p.softness
      local delta = -p.normal_mass * (soft.mass_scale * vn + p.bias) - soft.impulse_scale * p.normal_impulse
      local next_impulse = max(0.0, p.normal_impulse + delta)
      delta = next_impulse - p.normal_impulse
      p.normal_impulse = next_impulse
      local impulse = scale(p.n, delta)
      velocity_a = apply_impulse(body_a, velocity_a, p.r_a, impulse, -1)
      if body_b then velocity_b = apply_impulse(body_b, velocity_b, p.r_b, impulse, 1) end
    end
  end

  -- Unbiased relaxation and Coulomb friction.
  for i = 1, #points do
    local p = points[i]
    local va = point_velocity(body_a, velocity_a, p.r_a)
    local vb = body_b and point_velocity(body_b, velocity_b, p.r_b) or ZERO
    local vn = dot(sub(vb, va), p.n)
    local delta = -p.normal_mass * vn
    local next_impulse = max(0.0, p.normal_impulse + delta)
    delta = next_impulse - p.normal_impulse
    p.normal_impulse = next_impulse
    local impulse = scale(p.n, delta)
    velocity_a = apply_impulse(body_a, velocity_a, p.r_a, impulse, -1)
    if body_b then velocity_b = apply_impulse(body_b, velocity_b, p.r_b, impulse, 1) end

    va = point_velocity(body_a, velocity_a, p.r_a)
    vb = body_b and point_velocity(body_b, velocity_b, p.r_b) or v3(0, 0, 0)
    local rv = sub(vb, va)
    local old1, old2 = p.tangent_impulse1, p.tangent_impulse2
    local next1 = old1 - p.tangent_mass1 * dot(rv, p.t1)
    local next2 = old2 - p.tangent_mass2 * dot(rv, p.t2)
    local limit = p.friction * p.normal_impulse
    local mag2 = next1 * next1 + next2 * next2
    if mag2 > limit * limit and mag2 > EPS then
      local f = limit / sqrt(mag2)
      next1, next2 = next1 * f, next2 * f
    end
    p.tangent_impulse1, p.tangent_impulse2 = next1, next2
    local friction_impulse = add(scale(p.t1, next1 - old1), scale(p.t2, next2 - old2))
    velocity_a = apply_impulse(body_a, velocity_a, p.r_a, friction_impulse, -1)
    if body_b then velocity_b = apply_impulse(body_b, velocity_b, p.r_b, friction_impulse, 1) end
  end

  -- Restitution is a distinct pass after the inelastic solve.
  for i = 1, #points do
    local p = points[i]
    if p.restitution_target > 0 and p.normal_impulse > 0 then
      local va = point_velocity(body_a, velocity_a, p.r_a)
      local vb = body_b and point_velocity(body_b, velocity_b, p.r_b) or ZERO
      local vn = dot(sub(vb, va), p.n)
      local delta = p.normal_mass * (p.restitution_target - vn)
      local next_impulse = max(0.0, p.normal_impulse + delta)
      delta = next_impulse - p.normal_impulse
      p.normal_impulse = next_impulse
      local impulse = scale(p.n, delta)
      velocity_a = apply_impulse(body_a, velocity_a, p.r_a, impulse, -1)
      if body_b then velocity_b = apply_impulse(body_b, velocity_b, p.r_b, impulse, 1) end
    end
  end

  for i = 1, #points do
    local p = points[i]
    p.contact.normal_impulse = p.normal_impulse
    p.contact.tangent_impulse1 = p.tangent_impulse1
    p.contact.tangent_impulse2 = p.tangent_impulse2
  end
  return velocity_a, velocity_b
end

function box3d.solve_static_manifolds(body, position, velocity, manifolds, options)
  options = options or {}
  if #manifolds == 0 then return velocity, 0.0 end
  local points = prepare_points(body, position, velocity, nil, ZERO, ZERO, manifolds, options)
  if options.warm_start ~= false then velocity = warm_start(body, velocity, nil, ZERO, points) end
  velocity = solve_velocity(body, velocity, nil, ZERO, points, options.iterations or 1)
  local total_normal = 0.0
  for i = 1, #points do total_normal = total_normal + points[i].normal_impulse end
  return velocity, total_normal
end

function box3d.solve_pair_manifold(body_a, position_a, velocity_a, body_b, position_b, velocity_b, manifold, options)
  options = options or {}
  local points = prepare_points(body_a, position_a, velocity_a, body_b, position_b, velocity_b, manifold, options, true)
  if options.warm_start ~= false then
    velocity_a, velocity_b = warm_start(body_a, velocity_a, body_b, velocity_b, points)
  end
  velocity_a, velocity_b = solve_velocity(
    body_a, velocity_a, body_b, velocity_b, points, options.iterations or 1
  )
  local total_normal = 0.0
  for i = 1, #points do total_normal = total_normal + points[i].normal_impulse end
  return velocity_a, velocity_b, total_normal
end

function box3d.correct_static_position(position, manifold, slop, percent, max_correction)
  slop = slop or 0.0015
  percent = percent or 0.75
  max_correction = max_correction or 0.15
  local penetration = max(0.0, -(manifold.separation or 0.0) - slop)
  if penetration <= 0 then return position, 0.0 end
  local correction = min(max_correction, penetration * percent)
  return sub(position, scale(manifold.normal, correction)), correction
end

function box3d.correct_pair_positions(position_a, body_a, position_b, body_b, manifold, slop, percent, max_correction)
  slop = slop or 0.0015
  percent = percent or 0.75
  max_correction = max_correction or 0.15
  local penetration = max(0.0, -(manifold.separation or 0.0) - slop)
  if penetration <= 0 then return position_a, position_b, 0.0 end
  local inv_a, inv_b = body_a.inv_mass, body_b.inv_mass
  local sum = inv_a + inv_b
  if sum <= EPS then return position_a, position_b, 0.0 end
  local correction = min(max_correction, penetration * percent)
  position_a = sub(position_a, scale(manifold.normal, correction * inv_a / sum))
  position_b = add(position_b, scale(manifold.normal, correction * inv_b / sum))
  return position_a, position_b, correction
end

return box3d
