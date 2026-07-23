-- Minetest Shared Core Library
-- Provides standardized utilities, event handling, and object pooling
-- for high-performance Lua mods

local core = {}

--------------------------------------------------------------------------------
-- MODULE LOADER
-- Standardized module loading with caching
--------------------------------------------------------------------------------

local loaded_modules = {}

function core.load_module(name, path)
    if loaded_modules[name] then
        return loaded_modules[name]
    end
    
    local ok, mod = pcall(minecraft.require, path or name)
    if not ok then
        -- Fallback to standard require
        ok, mod = pcall(require, path or name)
    end
    
    if ok then
        loaded_modules[name] = mod
        return mod
    end
    
    error("Failed to load module: " .. (path or name))
end

--------------------------------------------------------------------------------
-- EVENT BUS
-- Centralized event registration and dispatching
--------------------------------------------------------------------------------

local event_handlers = {}
local event_queue = {}

function core.register_event(event_name, handler, priority)
    priority = priority or 0
    if not event_handlers[event_name] then
        event_handlers[event_name] = {}
    end
    
    table.insert(event_handlers[event_name], {
        handler = handler,
        priority = priority
    })
    
    -- Sort by priority (higher first)
    table.sort(event_handlers[event_name], function(a, b)
        return a.priority > b.priority
    end)
end

function core.trigger_event(event_name, ...)
    local handlers = event_handlers[event_name]
    if not handlers then return end
    
    for _, h in ipairs(handlers) do
        local ok, err = pcall(h.handler, ...)
        if not ok then
            minetest.log("error", "Event handler error for " .. event_name .. ": " .. tostring(err))
        end
    end
end

function core.queue_event(event_name, ...)
    table.insert(event_queue, {event_name, {...}})
end

function core.process_event_queue()
    for _, event in ipairs(event_queue) do
        core.trigger_event(event[1], unpack(event[2]))
    end
    event_queue = {}
end

--------------------------------------------------------------------------------
-- OBJECT POOL
-- Critical for physics performance - reduces GC pressure
--------------------------------------------------------------------------------

function core.create_object_pool(factory, reset_func, initial_size)
    initial_size = initial_size or 100
    
    local pool = {
        available = {},
        in_use = {},
        factory = factory,
        reset_func = reset_func
    }
    
    -- Pre-allocate objects
    for i = 1, initial_size do
        table.insert(pool.available, factory())
    end
    
    function pool.acquire(...)
        local obj
        if #pool.available > 0 then
            obj = table.remove(pool.available)
        else
            obj = factory(...)
        end
        table.insert(pool.in_use, obj)
        return obj
    end
    
    function pool.release(obj)
        -- Find and remove from in_use
        for i, o in ipairs(pool.in_use) do
            if o == obj then
                table.remove(pool.in_use, i)
                break
            end
        end
        
        -- Reset and return to available
        if pool.reset_func then
            pool.reset_func(obj)
        end
        table.insert(pool.available, obj)
    end
    
    function pool.release_all()
        for _, obj in ipairs(pool.in_use) do
            if pool.reset_func then
                pool.reset_func(obj)
            end
            table.insert(pool.available, obj)
        end
        pool.in_use = {}
    end
    
    function pool.get_stats()
        return {
            available = #pool.available,
            in_use = #pool.in_use,
            total = #pool.available + #pool.in_use
        }
    end
    
    return pool
end

--------------------------------------------------------------------------------
-- VECTOR MATH UTILITIES
-- Optimized vector operations for physics engine
--------------------------------------------------------------------------------

core.vec3_pool = core.create_object_pool(
    function() return {x=0, y=0, z=0} end,
    function(v) v.x, v.y, v.z = 0, 0, 0 end,
    500
)

function core.vec3_new(x, y, z)
    local v = core.vec3_pool.acquire()
    v.x = x or 0
    v.y = y or 0
    v.z = z or 0
    return v
end

function core.vec3_set(v, x, y, z)
    v.x = x
    v.y = y
    v.z = z
    return v
end

function core.vec3_copy(dst, src)
    dst.x = src.x
    dst.y = src.y
    dst.z = src.z
    return dst
end

function core.vec3_add(dst, a, b)
    dst.x = a.x + b.x
    dst.y = a.y + b.y
    dst.z = a.z + b.z
    return dst
end

function core.vec3_sub(dst, a, b)
    dst.x = a.x - b.x
    dst.y = a.y - b.y
    dst.z = a.z - b.z
    return dst
end

function core.vec3_scale(dst, v, s)
    dst.x = v.x * s
    dst.y = v.y * s
    dst.z = v.z * s
    return dst
end

function core.vec3_dot(a, b)
    return a.x * b.x + a.y * b.y + a.z * b.z
end

function core.vec3_cross(dst, a, b)
    dst.x = a.y * b.z - a.z * b.y
    dst.y = a.z * b.x - a.x * b.z
    dst.z = a.x * b.y - a.y * b.x
    return dst
end

function core.vec3_length_sq(v)
    return v.x * v.x + v.y * v.y + v.z * v.z
end

function core.vec3_length(v)
    return math.sqrt(core.vec3_length_sq(v))
end

function core.vec3_normalize(dst, v)
    local len_sq = core.vec3_length_sq(v)
    if len_sq > 0 then
        local inv_len = 1.0 / math.sqrt(len_sq)
        dst.x = v.x * inv_len
        dst.y = v.y * inv_len
        dst.z = v.z * inv_len
    else
        dst.x, dst.y, dst.z = 0, 0, 0
    end
    return dst
end

function core.vec3_release(v)
    core.vec3_pool.release(v)
end

--------------------------------------------------------------------------------
-- AABB UTILITIES
-- Axis-aligned bounding box operations for collision detection
--------------------------------------------------------------------------------

function core.aabb_intersect(min1, max1, min2, max2)
    return min1.x <= max2.x and max1.x >= min2.x and
           min1.y <= max2.y and max1.y >= min2.y and
           min1.z <= max2.z and max1.z >= min2.z
end

function core.aabb_contains(min, max, point)
    return point.x >= min.x and point.x <= max.x and
           point.y >= min.y and point.y <= max.y and
           point.z >= min.z and point.z <= max.z
end

function core.aabb_expand(min, max, point)
    if point.x < min.x then min.x = point.x end
    if point.y < min.y then min.y = point.y end
    if point.z < min.z then min.z = point.z end
    if point.x > max.x then max.x = point.x end
    if point.y > max.y then max.y = point.y end
    if point.z > max.z then max.z = point.z end
end

function core.aabb_center(center, min, max)
    center.x = (min.x + max.x) * 0.5
    center.y = (min.y + max.y) * 0.5
    center.z = (min.z + max.z) * 0.5
    return center
end

--------------------------------------------------------------------------------
-- SPATIAL HASH GRID
-- Broadphase collision detection optimization
--------------------------------------------------------------------------------

function core.create_spatial_hash(cell_size)
    cell_size = cell_size or 10.0
    local grid = {}
    local inv_cell_size = 1.0 / cell_size
    
    local function hash_key(x, y, z)
        local ix = math.floor(x * inv_cell_size)
        local iy = math.floor(y * inv_cell_size)
        local iz = math.floor(z * inv_cell_size)
        return ix .. ":" .. iy .. ":" .. iz
    end
    
    return {
        cell_size = cell_size,
        inv_cell_size = inv_cell_size,
        grid = grid,
        
        insert = function(self, obj, min, max)
            local key_min_x = math.floor(min.x * self.inv_cell_size)
            local key_min_y = math.floor(min.y * self.inv_cell_size)
            local key_min_z = math.floor(min.z * self.inv_cell_size)
            local key_max_x = math.floor(max.x * self.inv_cell_size)
            local key_max_y = math.floor(max.y * self.inv_cell_size)
            local key_max_z = math.floor(max.z * self.inv_cell_size)
            
            for ix = key_min_x, key_max_x do
                for iy = key_min_y, key_max_y do
                    for iz = key_min_z, key_max_z do
                        local key = ix .. ":" .. iy .. ":" .. iz
                        if not grid[key] then
                            grid[key] = {}
                        end
                        table.insert(grid[key], obj)
                    end
                end
            end
        end,
        
        remove = function(self, obj, min, max)
            -- Simplified removal - rebuild cell if needed
            local key_min_x = math.floor(min.x * self.inv_cell_size)
            local key_min_y = math.floor(min.y * self.inv_cell_size)
            local key_min_z = math.floor(min.z * self.inv_cell_size)
            local key_max_x = math.floor(max.x * self.inv_cell_size)
            local key_max_y = math.floor(max.y * self.inv_cell_size)
            local key_max_z = math.floor(max.z * self.inv_cell_size)
            
            for ix = key_min_x, key_max_x do
                for iy = key_min_y, key_max_y do
                    for iz = key_min_z, key_max_z do
                        local key = ix .. ":" .. iy .. ":" .. iz
                        if grid[key] then
                            for i, o in ipairs(grid[key]) do
                                if o == obj then
                                    table.remove(grid[key], i)
                                    break
                                end
                            end
                        end
                    end
                end
            end
        end,
        
        query = function(self, min, max)
            local results = {}
            local seen = {}
            
            local key_min_x = math.floor(min.x * self.inv_cell_size)
            local key_min_y = math.floor(min.y * self.inv_cell_size)
            local key_min_z = math.floor(min.z * self.inv_cell_size)
            local key_max_x = math.floor(max.x * self.inv_cell_size)
            local key_max_y = math.floor(max.y * self.inv_cell_size)
            local key_max_z = math.floor(max.z * self.inv_cell_size)
            
            for ix = key_min_x, key_max_x do
                for iy = key_min_y, key_max_y do
                    for iz = key_min_z, key_max_z do
                        local key = ix .. ":" .. iy .. ":" .. iz
                        if grid[key] then
                            for _, obj in ipairs(grid[key]) do
                                if not seen[obj] then
                                    seen[obj] = true
                                    table.insert(results, obj)
                                end
                            end
                        end
                    end
                end
            end
            
            return results
        end,
        
        clear = function(self)
            grid = {}
        end
    }
end

--------------------------------------------------------------------------------
-- MATH UTILITIES
--------------------------------------------------------------------------------

function core.clamp(value, min_val, max_val)
    if value < min_val then return min_val end
    if value > max_val then return max_val end
    return value
end

function core.lerp(a, b, t)
    return a + (b - a) * t
end

function core.approach(current, target, step)
    if current < target then
        return math.min(current + step, target)
    else
        return math.max(current - step, target)
    end
end

function core.sign(x)
    if x > 0 then return 1 end
    if x < 0 then return -1 end
    return 0
end

--------------------------------------------------------------------------------
-- PERFORMANCE MONITOR
--------------------------------------------------------------------------------

local perf_stats = {}

function core.start_perf_timer(name)
    perf_stats[name] = {
        start = minetest.get_us_time(),
        count = 0,
        total = 0
    }
end

function core.stop_perf_timer(name)
    if not perf_stats[name] then return end
    local elapsed = minetest.get_us_time() - perf_stats[name].start
    perf_stats[name].count = perf_stats[name].count + 1
    perf_stats[name].total = perf_stats[name].total + elapsed
end

function core.get_perf_stats(name)
    if not perf_stats[name] or perf_stats[name].count == 0 then
        return nil
    end
    return {
        count = perf_stats[name].count,
        avg_us = perf_stats[name].total / perf_stats[name].count,
        total_us = perf_stats[name].total
    }
end

function core.reset_perf_stats()
    perf_stats = {}
end

return core
