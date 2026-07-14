-- Item Drop Physics: hides the engine's dropped-item rendering and draws a
-- tumbling model driven by our own physics sim. The sim integrates gravity
-- per tick and resolves motion with box3d.aabb_slide() against the real in-game
-- block collision boxes (minecraft.world.get_block_collisions), so items
-- bounce off floors and walls and tumble from the impacts.
--
-- The engine item keeps ticking (pickup, despawn, lava) but Lua never moves
-- the visual back onto it — the sim fully owns where the item is drawn.
--
-- Draw/hitbox shape per item, in priority order:
--   1. minecraft.model.draw_item / item_bounds: the item's *real* model, i.e.
--      a custom Lua item/block's own baked model, or the vanilla/mod
--      block-cube renderer for full block items (dirt, stone, iron_bars'
--      block, ...). Same rendering path vanilla uses for dropped block items.
--   2. Fallback: a voxel cube extruded from the flat 2D icon, for plain
--      sprite items (tools, food, ...) with no 3D shape to draw.

local box3d = minecraft.require("scripts.box3d")

local HALF = 0.125       -- fallback item box half-width (engine box is 0.25 x 0.25)
local HEIGHT = 0.25
local DRAW_SCALE = 0.25
local ICON_THICKNESS = DRAW_SCALE / 16.0 -- one texel thick; sprite items are plates, not cubes
local GRAVITY = 0.04     -- matches ItemEntity::tick
local AIR_DRAG = 0.98
local GROUND_DRAG = 0.75
local FLOOR_BOUNCE = 0.55
local WALL_BOUNCE = 0.45
local BOUNCE_MIN_VY = 0.055  -- impacts slower than this come to rest
local GROUND_REST_SPEED = 0.005 -- grounded horizontal speed below this snaps to 0 instead of decaying forever
local GROUND_REST_ANGULAR_SPEED = 0.01 -- grounded angular speed below this snaps to 0 instead of decaying forever
local FLAT_SETTLE_ALIGN_EPS = 0.0005 -- once this close to upright, treat a flat item as settled instead of nudging it forever

-- Rotation is driven by a real box3d_lite rigid body (inertia tensor +
-- impulse contacts) instead of a random per-axis spin kick. Translation
-- keeps the existing tuned bounce/drag model above; the rigid body only
-- owns orientation and angular velocity, fed by an approximate contact
-- point at each collision (see step() below).
local TICK_SECONDS = 1.0 / 20.0                                   -- Minecraft tick rate
local SUB_STEPS = 4
local CONTACT_SOFTNESS = box3d.make_soft(30.0 * TICK_SECONDS, 10.0, 1.0 / SUB_STEPS) -- box3d defaults: 30Hz, zeta=10
local MAX_BIAS_VELOCITY = 3.0 * TICK_SECONDS                      -- box3d default contactSpeed
local RESTITUTION = 0.3
local RESTITUTION_THRESHOLD = 1.0 * TICK_SECONDS                  -- box3d default restitutionThreshold (1.0 m/s)
local FRICTION = 0.6
local GROUNDED_ANGULAR_DAMPING = 0.818  -- 1/(1+h*c) = 0.55 at h=1, matching the old grounded spin decay
local AIR_ANGULAR_DAMPING = 0.05        -- gentle spin decay in the air so per-bounce contact kicks can't accumulate without bound
local FLAT_SETTLE_STRENGTH = 1.35         -- grounded sprite torque toward lying on a broad face
local FLAT_SETTLE_MAX_SPEED = 0.22        -- only stabilize once translational motion is nearly finished

-- Stability clamps. A dropped item's own physics never legitimately reaches
-- these; they only fire when a contact solve overshoots (deep penetration +
-- tiny voxel inertia -> huge impulse, or a spin-coupled feedback loop). Capping
-- here keeps a bad frame from rocketing the item off-screen or into a NaN.
local MAX_LINEAR_SPEED = 1.5            -- blocks/tick
local MAX_ANGULAR_SPEED = 4.0 * math.pi -- rad/tick

local function is_finite(n)
  return n == n and n ~= math.huge and n ~= -math.huge
end

local function half_extents(shape)
  local half_x = shape and shape.half_x or HALF
  local half_z = shape and shape.half_z or HALF
  local half_y = (shape and shape.height or HEIGHT) * 0.5
  return half_x, half_y, half_z
end

local function projected_half_extents(s)
  local q = s.body.orientation
  -- The rigid body's dimensions are the single source of truth. Previously
  -- block items rendered with cube bounds while their contact solver still
  -- used a thin icon body, and fallback icons used cube-sized projected bounds.
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

-- Physics hitbox half-extents for an item, cached by item_id:damage since a
-- dropped item's shape never changes over its lifetime. Real-model items use
-- their actual bounds (model.item_bounds); everything else keeps the old
-- fixed icon-sized box.
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

-- Per-entity sim state, keyed by entity id. Pruned each tick as items leave.
local sims = {}

local function seed(item)
  local speed = math.sqrt(item.vx * item.vx + item.vy * item.vy + item.vz * item.vz)
  local kick = math.min(0.15 + speed * 1.5, 0.6)
  local shape = shape_for(item)
  local half_x, half_y, half_z = half_extents(shape)
  
  local body, com_offset
  local is_flat = shape == nil
  if shape then
    -- Real block/custom models must use the same box for broad phase, inertia,
    -- and contact points. The old code accidentally gave many cubes a thin
    -- 2D texture inertia tensor, which generated explosive collision impulses.
    body = box3d.new_box(half_x, half_y, half_z, 1.0)
    com_offset = box3d.v3(0, 0, 0)
  else
    local tex_info = minecraft.render.get_texture_pixels(item.texture_path or item.item_id)
    if tex_info and tex_info.pixels then
      body, com_offset = box3d.new_voxel_body(
        tex_info.pixels, tex_info.width, tex_info.height,
        DRAW_SCALE, 1.0, ICON_THICKNESS
      )
    else
      body = box3d.new_box(half_x, half_y, ICON_THICKNESS * 0.5, 1.0)
      com_offset = box3d.v3(0, 0, 0)
    end
  end
  
  -- Random starting orientation (uniform-ish via a random axis/angle) and a
  -- small random spin, in place of the old independent-Euler-angle kick.
  local axis = box3d.v3(math.random() - 0.5, math.random() - 0.5, math.random() - 0.5)
  local angle = math.random() * math.pi * 2
  local len = math.sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z)
  if len > 1e-6 then
    local s2 = math.sin(angle * 0.5) / len
    body.orientation = box3d.quat_normalize({ x = axis.x * s2, y = axis.y * s2, z = axis.z * s2, w = math.cos(angle * 0.5) })
  end
  body.angular_velocity = box3d.v3(
    (math.random() - 0.5) * kick,
    (math.random() - 0.5) * kick,
    (math.random() - 0.5) * kick
  )
  return {
    x = item.x, y = item.y, z = item.z,
    px = item.x, py = item.y, pz = item.z,
    vx = item.vx, vy = item.vy, vz = item.vz,
    body = body,
    com_offset = com_offset,
    prev_orientation = body.orientation,
    shape = shape,
    is_flat = is_flat,
    grounded = false,
  }
end

-- Find the actual contact point (furthest corner in collision direction) and
-- solve the contact constraint using Box3D's solver.
local function solve_axis_contact(s, normal, contact_axis, impact_velocity, half_x, half_y, half_z)
  local hx, hy, hz = s.body.half.x, s.body.half.y, s.body.half.z
  local world_offset = box3d.quat_rotate(s.body.orientation, s.com_offset)
  
  -- Find the corner furthest in the direction of the obstacle (-normal)
  local min_dot = math.huge
  local contact_r = nil
  for i = 0, 7 do
    local lx = (i % 2 == 0) and hx or -hx
    local ly = (math.floor(i / 2) % 2 == 0) and hy or -hy
    local lz = (math.floor(i / 4) % 2 == 0) and hz or -hz
    -- Shift corner by com_offset so it is relative to Center of Mass (origin of body frame)
    local local_corner = box3d.v3(lx - s.com_offset.x, ly - s.com_offset.y, lz - s.com_offset.z)
    local r = box3d.quat_rotate(s.body.orientation, local_corner)
    local dot = r.x * normal.x + r.y * normal.y + r.z * normal.z
    if dot < min_dot then
      min_dot = dot
      contact_r = r
    end
  end
  
  -- Compute penetration separation of this corner relative to the sliding AABB boundaries
  local separation = 0
  if contact_axis == "y" then
    if normal.y > 0 then
      separation = contact_r.y + world_offset.y + half_y
    else
      separation = -contact_r.y - world_offset.y + half_y
    end
  elseif contact_axis == "x" then
    if normal.x > 0 then
      separation = contact_r.x + world_offset.x + half_x
    else
      separation = -contact_r.x - world_offset.x + half_x
    end
  elseif contact_axis == "z" then
    if normal.z > 0 then
      separation = contact_r.z + world_offset.z + half_z
    else
      separation = -contact_r.z - world_offset.z + half_z
    end
  end
  
  separation = math.min(separation, 0.0)
  local point = box3d.v3(s.x + contact_r.x, s.y + contact_r.y, s.z + contact_r.z)
  
  return box3d.solve_contact(
    s.body, box3d.v3(s.x, s.y, s.z), point, normal, separation,
    impact_velocity, CONTACT_SOFTNESS, RESTITUTION, FRICTION, MAX_BIAS_VELOCITY, RESTITUTION_THRESHOLD
  )
end

local function finite_vec3(v)
  return is_finite(v.x) and is_finite(v.y) and is_finite(v.z)
end

local function finite_quat(q)
  return is_finite(q.x) and is_finite(q.y) and is_finite(q.z) and is_finite(q.w)
end

local function move_box(box, dx, dy, dz)
  box.min_x = box.min_x + dx; box.max_x = box.max_x + dx
  box.min_y = box.min_y + dy; box.max_y = box.max_y + dy
  box.min_z = box.min_z + dz; box.max_z = box.max_z + dz
end

local function make_box(s)
  local half_x, half_y, half_z = projected_half_extents(s)
  local world_offset = box3d.quat_rotate(s.body.orientation, s.com_offset)
  local cx, cy, cz = s.x - world_offset.x, s.y - world_offset.y, s.z - world_offset.z
  return {
    min_x = cx - half_x, min_y = cy - half_y, min_z = cz - half_z,
    max_x = cx + half_x, max_y = cy + half_y, max_z = cz + half_z,
  }, half_x, half_y, half_z, world_offset
end

-- Let the contact solver produce torque/friction, but do not accept its linear
-- velocity result. A single-point soft constraint has no accumulated impulse
-- manifold, so feeding its linear correction back into translation could add
-- energy and launch an item. Translation uses a bounded restitution response.
local function collide_axis(s, normal, axis, velocity, half_x, half_y, half_z, bounce)
  local old_w = box3d.v3(s.body.angular_velocity.x, s.body.angular_velocity.y, s.body.angular_velocity.z)
  solve_axis_contact(s, normal, axis, velocity, half_x, half_y, half_z)
  if not finite_vec3(s.body.angular_velocity) then
    s.body.angular_velocity = old_w
  end

  local vn = velocity.x * normal.x + velocity.y * normal.y + velocity.z * normal.z
  local out = box3d.v3(velocity.x, velocity.y, velocity.z)
  if vn < 0 then
    local correction = -(1.0 + bounce) * vn
    out.x = out.x + normal.x * correction
    out.y = out.y + normal.y * correction
    out.z = out.z + normal.z * correction
  end
  return out
end

local function stabilize_flat_item(s, h)
  if not s.is_flat then return end
  local speed2 = s.vx * s.vx + s.vy * s.vy + s.vz * s.vz
  if speed2 > FLAT_SETTLE_MAX_SPEED * FLAT_SETTLE_MAX_SPEED then return end

  -- Local Z is the icon thickness axis. Rotate whichever face is already
  -- nearest upward toward world +Y. This removes the numerically stable but
  -- visually absurd state where a paper-thin item balances forever on an edge.
  local n = box3d.quat_rotate(s.body.orientation, box3d.v3(0, 0, 1))
  if n.y < 0 then n = box3d.v3(-n.x, -n.y, -n.z) end
  if n.y > 1 - FLAT_SETTLE_ALIGN_EPS then
    -- Already lying flat: stop nudging it. Without this, floating-point noise
    -- in the cross product below keeps injecting a tiny torque every grounded
    -- substep forever, so the item never actually stops spinning in place.
    s.body.angular_velocity = box3d.v3(0, 0, 0)
    return
  end
  local axis = box3d.v3_cross(n, box3d.v3(0, 1, 0))
  local w = s.body.angular_velocity
  s.body.angular_velocity = box3d.v3(
    w.x + axis.x * FLAT_SETTLE_STRENGTH * h,
    w.y + axis.y * FLAT_SETTLE_STRENGTH * h,
    w.z + axis.z * FLAT_SETTLE_STRENGTH * h
  )
end

local function step(s)
  s.px, s.py, s.pz = s.x, s.y, s.z
  s.prev_orientation = s.body.orientation

  local h = 1.0 / SUB_STEPS

  for _ = 1, SUB_STEPS do
    -- Gravity first, in blocks per tick squared.
    s.vy = s.vy - GRAVITY * h

    -- Integrate rotation before building the collision AABB. The previous code
    -- rotated after resolving translation, so the newly enlarged projected box
    -- was allowed to appear inside the floor until the next frame.
    local old_box = make_box(s)
    local old_bottom = old_box.min_y
    local angular_damping = s.grounded and GROUNDED_ANGULAR_DAMPING or AIR_ANGULAR_DAMPING
    box3d.integrate_angular_velocity(s.body, nil, h, angular_damping)

    local w = s.body.angular_velocity
    local w2 = w.x * w.x + w.y * w.y + w.z * w.z
    if not is_finite(w2) then
      s.body.angular_velocity = box3d.v3(0, 0, 0)
    elseif w2 > MAX_ANGULAR_SPEED * MAX_ANGULAR_SPEED then
      local f = MAX_ANGULAR_SPEED / math.sqrt(w2)
      s.body.angular_velocity = box3d.v3(w.x * f, w.y * f, w.z * f)
    elseif s.grounded and w2 < GROUND_REST_ANGULAR_SPEED * GROUND_REST_ANGULAR_SPEED then
      -- Grounded angular damping only ever multiplies spin toward zero, same
      -- issue as the old linear-drag bug: it asymptotically approaches rest
      -- but never reaches it, so the item keeps imperceptibly spinning.
      s.body.angular_velocity = box3d.v3(0, 0, 0)
    end

    box3d.integrate_rotation(s.body, h)
    if not finite_quat(s.body.orientation) then
      s.body.orientation = box3d.quat_identity()
      s.body.angular_velocity = box3d.v3(0, 0, 0)
    end

    -- While supported, preserve the old floor contact as rotation changes the
    -- projected height. This makes a cube pivot upward instead of expanding
    -- downward through the block below it.
    if s.grounded then
      local rotated_box = make_box(s)
      s.y = s.y + (old_bottom - rotated_box.min_y)
    end

    local box, half_x, half_y, half_z = make_box(s)
    local req_dx, req_dy, req_dz = s.vx * h, s.vy * h, s.vz * h
    local query = {
      min_x = box.min_x + math.min(req_dx, 0) - 0.05,
      min_y = box.min_y + math.min(req_dy, 0) - 0.05,
      min_z = box.min_z + math.min(req_dz, 0) - 0.05,
      max_x = box.max_x + math.max(req_dx, 0) + 0.05,
      max_y = box.max_y + math.max(req_dy, 0) + 0.05,
      max_z = box.max_z + math.max(req_dz, 0) + 0.05,
    }
    local collisions = minecraft.world.get_block_collisions(query) or {}

    -- Resolve only shallow pre-existing overlap caused by spawn placement or
    -- rotational growth. The correction cap prevents deep-overlap teleports.
    local push_x, push_y, push_z = box3d.aabb_depenetrate(box, collisions, 4, 0.35)
    if push_x ~= 0 or push_y ~= 0 or push_z ~= 0 then
      s.x, s.y, s.z = s.x + push_x, s.y + push_y, s.z + push_z
      move_box(box, push_x, push_y, push_z)
      if push_y > 0 and s.vy < 0 then s.vy = 0 end
      if push_x * s.vx < 0 then s.vx = 0 end
      if push_z * s.vz < 0 then s.vz = 0 end
      req_dx, req_dy, req_dz = s.vx * h, s.vy * h, s.vz * h
    end

    local dx, dy, dz = box3d.aabb_slide(box, req_dx, req_dy, req_dz, collisions)
    s.x, s.y, s.z = s.x + dx, s.y + dy, s.z + dz
    move_box(box, dx, dy, dz)

    local hit_y = math.abs(dy - req_dy) > 1e-9
    local hit_x = math.abs(dx - req_dx) > 1e-9
    local hit_z = math.abs(dz - req_dz) > 1e-9
    local grounded = push_y > 0
    local new_v = box3d.v3(s.vx, s.vy, s.vz)

    if hit_y then
      local normal = box3d.v3(0, req_dy < 0 and 1 or -1, 0)
      new_v = collide_axis(s, normal, "y", new_v, half_x, half_y, half_z,
                           normal.y > 0 and FLOOR_BOUNCE or WALL_BOUNCE)
      if normal.y > 0 then
        grounded = true
        if math.abs(new_v.y) < BOUNCE_MIN_VY then new_v.y = 0 end
      elseif math.abs(new_v.y) < BOUNCE_MIN_VY then
        new_v.y = 0
      end
    end
    if hit_x then
      local normal = box3d.v3(req_dx < 0 and 1 or -1, 0, 0)
      new_v = collide_axis(s, normal, "x", new_v, half_x, half_y, half_z, WALL_BOUNCE)
      if math.abs(new_v.x) < BOUNCE_MIN_VY then new_v.x = 0 end
    end
    if hit_z then
      local normal = box3d.v3(0, 0, req_dz < 0 and 1 or -1)
      new_v = collide_axis(s, normal, "z", new_v, half_x, half_y, half_z, WALL_BOUNCE)
      if math.abs(new_v.z) < BOUNCE_MIN_VY then new_v.z = 0 end
    end

    s.vx, s.vy, s.vz = new_v.x, new_v.y, new_v.z
    s.grounded = grounded

    local drag = grounded and GROUND_DRAG or AIR_DRAG
    local sub_drag = 1.0 - (1.0 - drag) * h
    local sub_air_drag = 1.0 - (1.0 - AIR_DRAG) * h
    s.vx, s.vy, s.vz = s.vx * sub_drag, s.vy * sub_air_drag, s.vz * sub_drag

    -- Horizontal drag only ever multiplies velocity toward zero, and "grounded"
    -- can flicker false for a substep on a sub-epsilon gap (falling back to the
    -- much weaker AIR_DRAG). Without a hard floor here, resting items keep
    -- creeping indefinitely instead of coming to a stop.
    if grounded then
      if math.abs(s.vx) < GROUND_REST_SPEED then s.vx = 0 end
      if math.abs(s.vz) < GROUND_REST_SPEED then s.vz = 0 end
    end

    local sp2 = s.vx * s.vx + s.vy * s.vy + s.vz * s.vz
    if not is_finite(sp2) then
      s.vx, s.vy, s.vz = 0, 0, 0
    elseif sp2 > MAX_LINEAR_SPEED * MAX_LINEAR_SPEED then
      local f = MAX_LINEAR_SPEED / math.sqrt(sp2)
      s.vx, s.vy, s.vz = s.vx * f, s.vy * f, s.vz * f
    end

    if grounded then stabilize_flat_item(s, h) end
  end
end

-- Suppress the vanilla flat rendering of dropped items; we draw them ourselves.
minecraft.on(minecraft.events.pre_entity_render, { entity_type = "Item" }, function(event)
  event.canceled = true
  return event
end)

-- Advance the sim once per client tick (after world simulation), and prune
-- state for dead items. client_tick fires for the rendered world in both
-- singleplayer and multiplayer; world_tick's remote flag tracks
-- World::isRemote(), which is false for the singleplayer world.
minecraft.on(minecraft.events.client_tick, { before = false, after_world = true, paused = false }, function(event)
  if not event.has_world then
    return
  end
  local list = minecraft.entities.list("Item")
  if not list then
    return
  end
  local live = {}
  for i = 1, #list do
    local item = list[i]
    live[item.id] = true
    local s = sims[item.id]
    if s then
      step(s)
      -- If the sim went non-finite despite the clamps above, don't teleport the
      -- engine item to NaN (that loses the item) — reset the sim onto the real
      -- item and skip this frame.
      if not (is_finite(s.x) and is_finite(s.y) and is_finite(s.z)) then
        sims[item.id] = seed(item)
        goto continue
      end
      -- Move the invisible engine item onto the sim's position so pickup
      -- happens where the item is actually drawn, instead of wherever
      -- vanilla's own (unrendered) physics would have left it. The engine
      -- item's own tick logic (pickup, despawn, lava) keeps running against
      -- whatever position we hand it here. No-ops on a remote MP world,
      -- where only the server may move entities.
      item:teleport(s.x, s.y, s.z)
    else
      sims[item.id] = seed(item)
    end
    ::continue::
  end
  for id in pairs(sims) do
    if not live[id] then
      sims[id] = nil
    end
  end
end)

-- Draw each item as a tumbling model at the interpolated sim position: the
-- item's real 3D model (block cube or custom Lua model) when it has one,
-- otherwise the flat-icon voxel fallback.
minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.entities,
  moment = minecraft.render.moments.after,
}, function(event)
  local list = minecraft.entities.list("Item")
  if not list then
    return
  end
  local d = event.tick_delta or 1.0
  for i = 1, #list do
    local item = list[i]
    local s = sims[item.id]
    if s then
      local orientation = box3d.quat_slerp(s.prev_orientation, s.body.orientation, d)
      local world_offset = box3d.quat_rotate(orientation, s.com_offset)
      local yaw, pitch, roll = box3d.quat_to_euler_degrees(orientation)
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
end)
