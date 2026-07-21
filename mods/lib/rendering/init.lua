-- Rendering Utilities
-- Efficient batch rendering and debug visualization

local render = {}
local core = require("lib.core.init")

--------------------------------------------------------------------------------
-- BATCH RENDERER
-- Minimizes draw calls by batching similar objects
--------------------------------------------------------------------------------

render.BatchRenderer = {}
render.BatchRenderer.__index = render.BatchRenderer

function render.BatchRenderer.new(max_quads)
    local self = setmetatable({}, render.BatchRenderer)
    
    self.max_quads = max_quads or 1000
    self.quad_count = 0
    self.vertices = {}
    self.colors = {}
    self.texture_coords = {}
    
    -- Pre-allocate vertex buffer
    for i = 1, self.max_quads * 4 do
        self.vertices[i * 3 - 2] = 0
        self.vertices[i * 3 - 1] = 0
        self.vertices[i * 3] = 0
        self.colors[i] = {1, 1, 1, 1}
        self.texture_coords[i] = {0, 0}
    end
    
    return self
end

function render.BatchRenderer:begin()
    self.quad_count = 0
end

function render.BatchRenderer:add_quad(x, y, z, width, height, color, uv)
    if self.quad_count >= self.max_quads then
        minetest.log("warning", "BatchRenderer: exceeded max quads")
        return
    end
    
    local base_idx = self.quad_count * 4
    
    -- Calculate vertices (counter-clockwise)
    local half_w = width / 2
    local half_h = height / 2
    
    -- Bottom-left
    self.vertices[base_idx * 3 + 1] = x - half_w
    self.vertices[base_idx * 3 + 2] = y - half_h
    self.vertices[base_idx * 3 + 3] = z
    
    -- Bottom-right
    self.vertices[base_idx * 3 + 4] = x + half_w
    self.vertices[base_idx * 3 + 5] = y - half_h
    self.vertices[base_idx * 3 + 6] = z
    
    -- Top-right
    self.vertices[base_idx * 3 + 7] = x + half_w
    self.vertices[base_idx * 3 + 8] = y + half_h
    self.vertices[base_idx * 3 + 9] = z
    
    -- Top-left
    self.vertices[base_idx * 3 + 10] = x - half_w
    self.vertices[base_idx * 3 + 11] = y + half_h
    self.vertices[base_idx * 3 + 12] = z
    
    -- Set colors
    color = color or {1, 1, 1, 1}
    for i = 0, 3 do
        self.colors[base_idx + i + 1] = {color[1], color[2], color[3], color[4] or 1}
    end
    
    -- Set texture coordinates if provided
    if uv then
        self.texture_coords[base_idx + 1] = {uv[1], uv[2]}
        self.texture_coords[base_idx + 2] = {uv[3], uv[2]}
        self.texture_coords[base_idx + 3] = {uv[3], uv[4]}
        self.texture_coords[base_idx + 4] = {uv[1], uv[4]}
    end
    
    self.quad_count = self.quad_count + 1
end

function render.BatchRenderer:add_cube(position, size, color)
    if self.quad_count + 6 > self.max_quads then
        minetest.log("warning", "BatchRenderer: not enough space for cube")
        return
    end
    
    local hw, hh, hd = size.x/2, size.y/2, size.z/2
    local x, y, z = position.x, position.y, position.z
    
    -- Front face
    self:add_quad(x, y + hh, z + hd, size.x, size.y, color)
    -- Back face
    self:add_quad(x, y + hh, z - hd, size.x, size.y, color)
    -- Left face
    self:add_quad(x - hw, y + hh, z, size.z, size.y, color)
    -- Right face
    self:add_quad(x + hw, y + hh, z, size.z, size.y, color)
    -- Top face
    self:add_quad(x, y + hh, z, size.x, size.z, color)
    -- Bottom face
    self:add_quad(x, y - hh, z, size.x, size.z, color)
end

function render.BatchRenderer:flush()
    if self.quad_count == 0 then return end
    
    -- In a real implementation, this would send data to GPU
    -- For now, we'll use immediate mode rendering
    love.graphics.setColor(1, 1, 1)
    
    for i = 0, self.quad_count - 1 do
        local base_idx = i * 4
        local r, g, b, a = unpack(self.colors[base_idx + 1])
        love.graphics.setColor(r, g, b, a)
        
        local x1, y1 = self.vertices[base_idx * 3 + 1], self.vertices[base_idx * 3 + 2]
        local x2, y2 = self.vertices[base_idx * 3 + 4], self.vertices[base_idx * 3 + 5]
        local x3, y3 = self.vertices[base_idx * 3 + 7], self.vertices[base_idx * 3 + 8]
        local x4, y4 = self.vertices[base_idx * 3 + 10], self.vertices[base_idx * 3 + 11]
        
        love.graphics.polygon("fill", x1, y1, x2, y2, x3, y3, x4, y4)
    end
    
    self:begin()
end

--------------------------------------------------------------------------------
-- DEBUG RENDERER
-- Visualization for physics debugging
--------------------------------------------------------------------------------

render.DebugRenderer = {}
render.DebugRenderer.__index = render.DebugRenderer

function render.DebugRenderer.new()
    local self = setmetatable({}, render.DebugRenderer)
    self.lines = {}
    self.boxes = {}
    self.points = {}
    self.enabled = true
    return self
end

function render.DebugRenderer:draw_line(p1, p2, color, duration)
    if not self.enabled then return end
    
    table.insert(self.lines, {
        p1 = {x=p1.x, y=p1.y, z=p1.z},
        p2 = {x=p2.x, y=p2.y, z=p2.z},
        color = color or {1, 1, 1},
        duration = duration or 0,
        age = 0
    })
end

function render.DebugRenderer:draw_box(min_v, max_v, color, duration)
    if not self.enabled then return end
    
    -- 12 edges of the box
    local corners = {
        {x=min_v.x, y=min_v.y, z=min_v.z},
        {x=max_v.x, y=min_v.y, z=min_v.z},
        {x=max_v.x, y=min_v.y, z=max_v.z},
        {x=min_v.x, y=min_v.y, z=max_v.z},
        {x=min_v.x, y=max_v.y, z=min_v.z},
        {x=max_v.x, y=max_v.y, z=min_v.z},
        {x=max_v.x, y=max_v.y, z=max_v.z},
        {x=min_v.x, y=max_v.y, z=max_v.z}
    }
    
    -- Bottom face
    self:draw_line(corners[1], corners[2], color, duration)
    self:draw_line(corners[2], corners[3], color, duration)
    self:draw_line(corners[3], corners[4], color, duration)
    self:draw_line(corners[4], corners[1], color, duration)
    
    -- Top face
    self:draw_line(corners[5], corners[6], color, duration)
    self:draw_line(corners[6], corners[7], color, duration)
    self:draw_line(corners[7], corners[8], color, duration)
    self:draw_line(corners[8], corners[5], color, duration)
    
    -- Vertical edges
    self:draw_line(corners[1], corners[5], color, duration)
    self:draw_line(corners[2], corners[6], color, duration)
    self:draw_line(corners[3], corners[7], color, duration)
    self:draw_line(corners[4], corners[8], color, duration)
end

function render.DebugRenderer:draw_point(pos, color, size, duration)
    if not self.enabled then return end
    
    table.insert(self.points, {
        pos = {x=pos.x, y=pos.y, z=pos.z},
        color = color or {1, 1, 1},
        size = size or 2,
        duration = duration or 0,
        age = 0
    })
end

function render.DebugRenderer:draw_arrow(from, to, color, duration)
    if not self.enabled then return end
    
    -- Draw line
    self:draw_line(from, to, color, duration)
    
    -- Draw arrowhead
    local dir = core.vec3_new()
    core.vec3_sub(dir, to, from)
    core.vec3_normalize(dir, dir)
    
    local arrow_len = 0.3
    local left = core.vec3_new()
    local right = core.vec3_new()
    
    -- Simple perpendicular vectors
    if math.abs(dir.x) < 0.9 then
        left.x = -dir.y
        left.y = dir.x
        left.z = 0
    else
        left.x = 0
        left.y = -dir.z
        left.z = dir.y
    end
    
    core.vec3_scale(left, left, arrow_len)
    core.vec3_scale(right, left, -1)
    
    local tip = core.vec3_new()
    core.vec3_scale(tip, dir, -arrow_len)
    
    local arrow_left = core.vec3_new()
    local arrow_right = core.vec3_new()
    core.vec3_add(arrow_left, to, left)
    core.vec3_add(arrow_right, to, right)
    
    self:draw_line(to, arrow_left, color, duration)
    self:draw_line(to, arrow_right, color, duration)
    
    core.vec3_release(dir)
    core.vec3_release(left)
    core.vec3_release(right)
    core.vec3_release(tip)
    core.vec3_release(arrow_left)
    core.vec3_release(arrow_right)
end

function render.DebugRenderer:update(dt)
    -- Update aged elements
    for i = #self.lines, 1, -1 do
        local line = self.lines[i]
        if line.duration > 0 then
            line.age = line.age + dt
            if line.age >= line.duration then
                table.remove(self.lines, i)
            end
        end
    end
    
    for i = #self.points, 1, -1 do
        local point = self.points[i]
        if point.duration > 0 then
            point.age = point.age + dt
            if point.age >= point.duration then
                table.remove(self.points, i)
            end
        end
    end
end

function render.DebugRenderer:render(camera)
    if not self.enabled then return end
    
    -- Render lines
    for _, line in ipairs(self.lines) do
        love.graphics.setColor(line.color)
        -- Project 3D to 2D (simplified)
        local x1, y1 = self:project(line.p1, camera)
        local x2, y2 = self:project(line.p2, camera)
        love.graphics.line(x1, y1, x2, y2)
    end
    
    -- Render points
    for _, point in ipairs(self.points) do
        love.graphics.setColor(point.color)
        local x, y = self:project(point.pos, camera)
        love.graphics.circle("fill", x, y, point.size)
    end
end

function render.DebugRenderer:project(pos, camera)
    -- Simplified projection - in real implementation would use view/projection matrices
    local dx = pos.x - (camera.x or 0)
    local dy = pos.y - (camera.y or 0)
    local dz = pos.z - (camera.z or 0)
    
    -- Orthographic projection for now
    local scale = camera.scale or 1
    return dx * scale, -dy * scale
end

function render.DebugRenderer:clear()
    self.lines = {}
    self.boxes = {}
    self.points = {}
end

--------------------------------------------------------------------------------
-- PARTICLE SYSTEM
-- Efficient particle rendering with pooling
--------------------------------------------------------------------------------

render.ParticleSystem = {}
render.ParticleSystem.__index = render.ParticleSystem

function render.ParticleSystem.new(max_particles)
    local self = setmetatable({}, render.ParticleSystem)
    
    self.max_particles = max_particles or 500
    self.particles = {}
    self.available = {}
    
    -- Pre-allocate particles
    for i = 1, self.max_particles do
        table.insert(self.available, {
            position = {x=0, y=0, z=0},
            velocity = {x=0, y=0, z=0},
            color = {1, 1, 1, 1},
            size = 1,
            lifetime = 0,
            max_lifetime = 1,
            active = false
        })
    end
    
    return self
end

function render.ParticleSystem:emit(options)
    options = options or {}
    
    if #self.available == 0 then return false end
    
    local particle = table.remove(self.available)
    particle.position.x = options.x or 0
    particle.position.y = options.y or 0
    particle.position.z = options.z or 0
    
    particle.velocity.x = options.vx or (math.random() - 0.5) * 10
    particle.velocity.y = options.vy or (math.random() - 0.5) * 10
    particle.velocity.z = options.vz or (math.random() - 0.5) * 10
    
    particle.color = options.color or {1, 1, 1, 1}
    particle.size = options.size or 1
    particle.lifetime = 0
    particle.max_lifetime = options.lifetime or 2
    particle.active = true
    
    if options.gravity ~= nil then
        particle.gravity = options.gravity
    else
        particle.gravity = -9.81
    end
    
    particle.drag = options.drag or 0.1
    
    table.insert(self.particles, particle)
    return true
end

function render.ParticleSystem:update(dt)
    for i = #self.particles, 1, -1 do
        local p = self.particles[i]
        
        if not p.active then
            table.remove(self.particles, i)
            table.insert(self.available, p)
            goto continue
        end
        
        p.lifetime = p.lifetime + dt
        
        if p.lifetime >= p.max_lifetime then
            p.active = false
            goto continue
        end
        
        -- Update velocity
        p.velocity.x = p.velocity.x * (1 - p.drag * dt)
        p.velocity.y = p.velocity.y * (1 - p.drag * dt) + (p.gravity or -9.81) * dt
        p.velocity.z = p.velocity.z * (1 - p.drag * dt)
        
        -- Update position
        p.position.x = p.position.x + p.velocity.x * dt
        p.position.y = p.position.y + p.velocity.y * dt
        p.position.z = p.position.z + p.velocity.z * dt
        
        -- Fade out near end of life
        local t = p.lifetime / p.max_lifetime
        p.color[4] = (1 - t) * (options.initial_alpha or 1)
        
        ::continue::
    end
end

function render.ParticleSystem:render(camera)
    for _, p in ipairs(self.particles) do
        if p.active then
            local x, y = self:project(p.position, camera)
            love.graphics.setColor(p.color)
            love.graphics.circle("fill", x, y, p.size * (1 - p.lifetime / p.max_lifetime))
        end
    end
end

function render.ParticleSystem:project(pos, camera)
    local dx = pos.x - (camera.x or 0)
    local dy = pos.y - (camera.y or 0)
    local scale = camera.scale or 1
    return dx * scale, -dy * scale
end

function render.ParticleSystem:get_stats()
    return {
        active = 0,
        available = #self.available,
        total = self.max_particles
    }
    
    for _, p in ipairs(self.particles) do
        if p.active then
            stats.active = stats.active + 1
        end
    end
    
    return stats
end

return render
