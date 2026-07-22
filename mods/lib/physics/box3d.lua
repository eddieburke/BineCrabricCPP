-- High-Performance Box3D Physics Engine (Pure Lua)
-- Optimized for minimal allocations and maximum throughput
-- Replaces C bindings with efficient Lua implementation

local core = require("lib.core.init")
local box3d = {}

--------------------------------------------------------------------------------
-- CONSTANTS
--------------------------------------------------------------------------------

local GRAVITY = -9.81
local DEFAULT_FRICTION = 0.7
local DEFAULT_RESTITUTION = 0.2
local MAX_CONTACTS = 4
local SLEEP_THRESHOLD = 0.01
local SLEEP_DELAY = 1.0

--------------------------------------------------------------------------------
-- OBJECT POOLS
--------------------------------------------------------------------------------

local contact_pool = core.create_object_pool(
    function() return {
        normal = {x=0, y=0, z=0},
        point = {x=0, y=0, z=0},
        depth = 0,
        body_a = nil,
        body_b = nil
    } end,
    function(c)
        c.normal.x, c.normal.y, c.normal.z = 0, 0, 0
        c.point.x, c.point.y, c.point.z = 0, 0, 0
        c.depth = 0
        c.body_a = nil
        c.body_b = nil
    end,
    1000
)

local manifold_pool = core.create_object_pool(
    function() return {contacts = {}, count = 0} end,
    function(m)
        m.count = 0
        for i = #m.contacts, 1, -1 do
            contact_pool.release(table.remove(m.contacts))
        end
    end,
    500
)

--------------------------------------------------------------------------------
-- RIGID BODY
--------------------------------------------------------------------------------

box3d.Body = {}
box3d.Body.__index = box3d.Body

function box3d.Body.new(mass, position, size)
    local self = setmetatable({}, box3d.Body)
    
    -- Transform
    self.position = core.vec3_new(position.x or 0, position.y or 0, position.z or 0)
    self.velocity = core.vec3_new(0, 0, 0)
    self.angular_velocity = core.vec3_new(0, 0, 0)
    
    -- Size (half-extents for AABB)
    self.half_extents = core.vec3_new(size.x/2, size.y/2, size.z/2)
    
    -- Physics properties
    self.mass = mass or 1.0
    self.inv_mass = mass > 0 and 1.0 / mass or 0
    self.friction = DEFAULT_FRICTION
    self.restitution = DEFAULT_RESTITUTION
    
    -- Inertia tensor (simplified for box)
    if mass > 0 then
        local w, h, d = size.x, size.y, size.z
        self.inertia = core.vec3_new(
            mass * (h*h + d*d) / 12,
            mass * (w*w + d*d) / 12,
            mass * (w*w + h*h) / 12
        )
        self.inv_inertia = core.vec3_new(
            1.0 / self.inertia.x,
            1.0 / self.inertia.y,
            1.0 / self.inertia.z
        )
    else
        self.inertia = core.vec3_new(0, 0, 0)
        self.inv_inertia = core.vec3_new(0, 0, 0)
    end
    
    -- State
    self.sleeping = false
    self.sleep_timer = 0
    self.enabled = true
    self.is_static = mass == 0
    
    -- Contacts
    self.contacts = {}
    
    return self
end

function box3d.Body:get_aabb(min_out, max_out)
    core.vec3_sub(min_out, self.position, self.half_extents)
    core.vec3_add(max_out, self.position, self.half_extents)
    return min_out, max_out
end

function box3d.Body:update_bounds(spatial_hash)
    if not spatial_hash then return end
    local min_v, max_v = core.vec3_new(), core.vec3_new()
    self:get_aabb(min_v, max_v)
    spatial_hash:insert(self, min_v, max_v)
    core.vec3_release(min_v)
    core.vec3_release(max_v)
end

function box3d.Body:integrate(dt)
    if self.is_static or self.sleeping then return end
    
    -- Apply gravity
    self.velocity.y = self.velocity.y + GRAVITY * dt
    
    -- Integrate velocity
    self.position.x = self.position.x + self.velocity.x * dt
    self.position.y = self.position.y + self.velocity.y * dt
    self.position.z = self.position.z + self.velocity.z * dt
    
    -- Check for sleep
    local vel_sq = core.vec3_length_sq(self.velocity)
    if vel_sq < SLEEP_THRESHOLD * SLEEP_THRESHOLD then
        self.sleep_timer = self.sleep_timer + dt
        if self.sleep_timer > SLEEP_DELAY then
            self.sleeping = true
            core.vec3_set(self.velocity, 0, 0, 0)
        end
    else
        self.sleep_timer = 0
    end
end

function box3d.Body:wake()
    self.sleeping = false
    self.sleep_timer = 0
end

function box3d.Body:apply_impulse(impulse, contact_point)
    if self.is_static then return end
    
    self:wake()
    
    -- Linear impulse
    self.velocity.x = self.velocity.x + impulse.x * self.inv_mass
    self.velocity.y = self.velocity.y + impulse.y * self.inv_mass
    self.velocity.z = self.velocity.z + impulse.z * self.inv_mass
    
    -- Angular impulse (simplified)
    local r = core.vec3_new()
    core.vec3_sub(r, contact_point, self.position)
    
    local torque = core.vec3_new()
    core.vec3_cross(torque, r, impulse)
    
    self.angular_velocity.x = self.angular_velocity.x + torque.x * self.inv_inertia.x
    self.angular_velocity.y = self.angular_velocity.y + torque.y * self.inv_inertia.y
    self.angular_velocity.z = self.angular_velocity.z + torque.z * self.inv_inertia.z
    
    core.vec3_release(r)
    core.vec3_release(torque)
end

--------------------------------------------------------------------------------
-- COLLISION DETECTION
--------------------------------------------------------------------------------

function box3d.detect_collision(body_a, body_b, manifold)
    local min_a, max_a = core.vec3_new(), core.vec3_new()
    local min_b, max_b = core.vec3_new(), core.vec3_new()
    
    body_a:get_aabb(min_a, max_a)
    body_b:get_aabb(min_b, max_b)
    
    -- Early out if AABBs don't intersect
    if not core.aabb_intersect(min_a, max_a, min_b, max_b) then
        core.vec3_release(min_a)
        core.vec3_release(max_a)
        core.vec3_release(min_b)
        core.vec3_release(max_b)
        return false
    end
    
    -- Find overlap on each axis
    local overlap_x = math.min(max_a.x, max_b.x) - math.max(min_a.x, min_b.x)
    local overlap_y = math.min(max_a.y, max_b.y) - math.max(min_a.y, min_b.y)
    local overlap_z = math.min(max_a.z, max_b.z) - math.max(min_a.z, min_b.z)
    
    -- Find minimum overlap axis
    local min_overlap = math.min(overlap_x, math.min(overlap_y, overlap_z))
    local normal
    
    if min_overlap == overlap_x then
        if body_a.position.x < body_b.position.x then
            normal = core.vec3_new(-1, 0, 0)
        else
            normal = core.vec3_new(1, 0, 0)
        end
    elseif min_overlap == overlap_y then
        if body_a.position.y < body_b.position.y then
            normal = core.vec3_new(0, -1, 0)
        else
            normal = core.vec3_new(0, 1, 0)
        end
    else
        if body_a.position.z < body_b.position.z then
            normal = core.vec3_new(0, 0, -1)
        else
            normal = core.vec3_new(0, 0, 1)
        end
    end
    
    -- Create contact
    local contact = contact_pool.acquire()
    core.vec3_copy(contact.normal, normal)
    contact.depth = min_overlap
    contact.body_a = body_a
    contact.body_b = body_b
    
    -- Calculate contact point (center of overlap region)
    local center_a, center_b = core.vec3_new(), core.vec3_new()
    core.aabb_center(center_a, min_a, max_a)
    core.aabb_center(center_b, min_b, max_b)
    core.vec3_lerp(contact.point, center_a, center_b, 0.5)
    
    table.insert(manifold.contacts, contact)
    manifold.count = manifold.count + 1
    
    -- Cleanup
    core.vec3_release(min_a)
    core.vec3_release(max_a)
    core.vec3_release(min_b)
    core.vec3_release(max_b)
    core.vec3_release(normal)
    core.vec3_release(center_a)
    core.vec3_release(center_b)
    
    return true
end

--------------------------------------------------------------------------------
-- COLLISION RESOLUTION
--------------------------------------------------------------------------------

function box3d.resolve_contact(contact)
    local body_a = contact.body_a
    local body_b = contact.body_b
    
    -- Don't resolve if both static
    if body_a.is_static and body_b.is_static then return end
    
    -- Positional correction (baumgarte stabilization)
    local baumgarte = 0.2
    local slop = 0.01
    local correction_mag = math.max(contact.depth - slop, 0) * baumgarte
    
    local total_inv_mass = body_a.inv_mass + body_b.inv_mass
    if total_inv_mass == 0 then return end
    
    local correction = core.vec3_new()
    core.vec3_scale(correction, contact.normal, correction_mag / total_inv_mass)
    
    if not body_a.is_static then
        body_a.position.x = body_a.position.x - correction.x * body_a.inv_mass
        body_a.position.y = body_a.position.y - correction.y * body_a.inv_mass
        body_a.position.z = body_a.position.z - correction.z * body_a.inv_mass
    end
    
    if not body_b.is_static then
        body_b.position.x = body_b.position.x + correction.x * body_b.inv_mass
        body_b.position.y = body_b.position.y + correction.y * body_b.inv_mass
        body_b.position.z = body_b.position.z + correction.z * body_b.inv_mass
    end
    
    core.vec3_release(correction)
    
    -- Velocity resolution
    local rel_vel = core.vec3_new()
    core.vec3_sub(rel_vel, body_b.velocity, body_a.velocity)
    
    local vel_along_normal = core.vec3_dot(rel_vel, contact.normal)
    
    -- Don't resolve if velocities are separating
    if vel_along_normal > 0 then
        core.vec3_release(rel_vel)
        return
    end
    
    -- Calculate impulse scalar
    local e = math.min(body_a.restitution, body_b.restitution)
    local j = -(1 + e) * vel_along_normal
    j = j / total_inv_mass
    
    -- Apply impulse
    local impulse = core.vec3_new()
    core.vec3_scale(impulse, contact.normal, j)
    
    body_a:apply_impulse(impulse, contact.point)
    
    -- Negate impulse for body_b
    core.vec3_scale(impulse, impulse, -1)
    body_b:apply_impulse(impulse, contact.point)
    
    -- Friction impulse (simplified)
    local tangent = core.vec3_new()
    core.vec3_copy(tangent, rel_vel)
    local tan_dot_normal = core.vec3_dot(tangent, contact.normal)
    tangent.x = tangent.x - contact.normal.x * tan_dot_normal
    tangent.y = tangent.y - contact.normal.y * tan_dot_normal
    tangent.z = tangent.z - contact.normal.z * tan_dot_normal
    
    local tan_len = core.vec3_length(tangent)
    if tan_len > 0.0001 then
        core.vec3_scale(tangent, tangent, 1.0 / tan_len)
        
        local jt = -core.vec3_dot(rel_vel, tangent) / total_inv_mass
        local mu = math.sqrt(body_a.friction * body_b.friction)
        
        -- Coulomb friction clamping
        if math.abs(jt) < j * mu then
            -- Static friction
            core.vec3_scale(impulse, tangent, jt)
        else
            -- Dynamic friction
            core.vec3_scale(impulse, tangent, -j * mu)
        end
        
        body_a:apply_impulse(impulse, contact.point)
        core.vec3_scale(impulse, impulse, -1)
        body_b:apply_impulse(impulse, contact.point)
    end
    
    core.vec3_release(rel_vel)
    core.vec3_release(impulse)
    core.vec3_release(tangent)
end

--------------------------------------------------------------------------------
-- PHYSICS WORLD
--------------------------------------------------------------------------------

box3d.World = {}
box3d.World.__index = box3d.World

function box3d.World.new(cell_size)
    local self = setmetatable({}, box3d.World)
    
    self.bodies = {}
    self.spatial_hash = core.create_spatial_hash(cell_size or 10.0)
    self.contact_manifolds = {}
    
    return self
end

function box3d.World:add_body(body)
    table.insert(self.bodies, body)
    body:update_bounds(self.spatial_hash)
end

function box3d.World:remove_body(body)
    for i, b in ipairs(self.bodies) do
        if b == body then
            table.remove(self.bodies, i)
            break
        end
    end
end

function box3d.World:step(dt)
    -- Limit dt for stability
    dt = math.min(dt, 0.05)
    
    -- Broadphase: find potential collisions
    local potential_collisions = {}
    local seen_pairs = {}
    
    for _, body in ipairs(self.bodies) do
        if body.enabled and not body.sleeping then
            local neighbors = self.spatial_hash:query(
                core.vec3_new(body.position.x - body.half_extents.x,
                             body.position.y - body.half_extents.y,
                             body.position.z - body.half_extents.z),
                core.vec3_new(body.position.x + body.half_extents.x,
                             body.position.y + body.half_extents.y,
                             body.position.z + body.half_extents.z)
            )
            
            for _, other in ipairs(neighbors) do
                if body ~= other and not other.sleeping then
                    local pair_key = body < other and (body .. ":" .. other) or (other .. ":" .. body)
                    if not seen_pairs[pair_key] then
                        seen_pairs[pair_key] = true
                        table.insert(potential_collisions, {body, other})
                    end
                end
            end
        end
    end
    
    -- Narrowphase: detect and resolve collisions
    for i = #self.contact_manifolds, 1, -1 do
        manifold_pool.release(table.remove(self.contact_manifolds))
    end
    
    for _, pair in ipairs(potential_collisions) do
        local body_a, body_b = pair[1], pair[2]
        local manifold = manifold_pool.acquire()
        
        if box3d.detect_collision(body_a, body_b, manifold) then
            table.insert(self.contact_manifolds, manifold)
        else
            manifold_pool.release(manifold)
        end
    end
    
    -- Resolve contacts
    for _, manifold in ipairs(self.contact_manifolds) do
        for _, contact in ipairs(manifold.contacts) do
            box3d.resolve_contact(contact)
        end
    end
    
    -- Integrate all bodies
    for _, body in ipairs(self.bodies) do
        if body.enabled then
            body:integrate(dt)
        end
    end
    
    -- Update spatial hash
    self.spatial_hash:clear()
    for _, body in ipairs(self.bodies) do
        if body.enabled then
            body:update_bounds(self.spatial_hash)
        end
    end
end

function box3d.World:get_stats()
    local stats = {
        body_count = #self.bodies,
        active_bodies = 0,
        sleeping_bodies = 0,
        contact_count = 0,
        pool_stats = {
            contacts = contact_pool.get_stats(),
            manifolds = manifold_pool.get_stats(),
            vectors = core.vec3_pool.get_stats()
        }
    }
    
    for _, body in ipairs(self.bodies) do
        if body.sleeping then
            stats.sleeping_bodies = stats.sleeping_bodies + 1
        else
            stats.active_bodies = stats.active_bodies + 1
        end
    end
    
    for _, manifold in ipairs(self.contact_manifolds) do
        stats.contact_count = stats.contact_count + manifold.count
    end
    
    return stats
end

return box3d
