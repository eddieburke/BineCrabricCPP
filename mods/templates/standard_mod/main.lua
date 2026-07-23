-- Standard Mod Template
-- Follows the consolidated structure for consistency

--------------------------------------------------------------------------------
-- SECTION 1: IMPORTS
-- Load shared libraries and dependencies first
--------------------------------------------------------------------------------

local core = require("lib.core.init")
local physics = require("lib.physics.box3d")
local ui = require("lib.ui")
local audio = require("lib.audio")
local render = require("lib.rendering")

--------------------------------------------------------------------------------
-- SECTION 2: CONSTANTS
-- Define all magic numbers and configuration values
--------------------------------------------------------------------------------

local MOD_NAME = "standard_mod"
local VERSION = "1.0.0"

-- Physics constants
local GRAVITY = -9.81
local MAX_ENTITIES = 500
local UPDATE_RATE = 60

-- Configuration defaults
local DEFAULT_CONFIG = {
    enable_physics = true,
    enable_audio = true,
    debug_mode = false,
    max_entities = 100
}

--------------------------------------------------------------------------------
-- SECTION 3: STATE MANAGEMENT
-- Centralized state for the mod
--------------------------------------------------------------------------------

local state = {
    initialized = false,
    paused = false,
    entities = {},
    entity_count = 0,
    world = nil,
    config = {},
    timers = {
        update = 0,
        render = 0
    }
}

--------------------------------------------------------------------------------
-- SECTION 4: UTILITY FUNCTIONS
-- Local helper functions (use core lib when possible)
--------------------------------------------------------------------------------

local function log(message, level)
    level = level or "info"
    minetest.log(level, "[" .. MOD_NAME .. "] " .. message)
end

local function merge_tables(target, source)
    for k, v in pairs(source) do
        if type(v) == "table" and type(target[k]) == "table" then
            merge_tables(target[k], v)
        else
            target[k] = v
        end
    end
    return target
end

--------------------------------------------------------------------------------
-- SECTION 5: ENTITY MANAGEMENT
--------------------------------------------------------------------------------

local Entity = {}
Entity.__index = Entity

function Entity.new(data)
    local self = setmetatable({}, Entity)
    
    self.id = data.id or 0
    self.type = data.type or "default"
    self.position = core.vec3_new(
        data.x or 0,
        data.y or 0,
        data.z or 0
    )
    self.velocity = core.vec3_new(0, 0, 0)
    self.size = data.size or {x=1, y=1, z=1}
    self.mass = data.mass or 1
    self.active = true
    self.render_data = data.render_data or {}
    self.audio_data = data.audio_data or {}
    
    return self
end

function Entity:update(dt)
    if not self.active then return end
    
    -- Update position from velocity
    self.position.x = self.position.x + self.velocity.x * dt
    self.position.y = self.position.y + self.velocity.y * dt
    self.position.z = self.position.z + self.velocity.z * dt
end

function Entity:render()
    if not self.active then return end
    -- Rendering handled by render system
end

function Entity:cleanup()
    core.vec3_release(self.position)
    core.vec3_release(self.velocity)
end

--------------------------------------------------------------------------------
-- SECTION 6: EVENT HANDLERS
-- Registered via core event bus
--------------------------------------------------------------------------------

local function on_world_load()
    log("World loaded")
    
    -- Initialize physics world
    if state.config.enable_physics then
        state.world = physics.World.new(10.0)
    end
    
    state.paused = false
end

local function on_world_unload()
    log("World unloading")
    
    -- Cleanup entities
    for _, entity in ipairs(state.entities) do
        entity:cleanup()
    end
    state.entities = {}
    state.entity_count = 0
    state.world = nil
end

local function on_player_join(player)
    log("Player joined: " .. tostring(player))
end

local function on_player_leave(player)
    log("Player left: " .. tostring(player))
end

local function on_chat_message(player, message)
    if message:sub(1, 1) == "/" then
        return handle_command(player, message:sub(2))
    end
end

--------------------------------------------------------------------------------
-- SECTION 7: COMMAND HANDLING
--------------------------------------------------------------------------------

function handle_command(player, command)
    local args = {}
    for arg in command:gmatch("%S+") do
        table.insert(args, arg)
    end
    
    if #args == 0 then return false end
    
    local cmd = args[1]:lower()
    
    if cmd == "spawn" then
        spawn_entity(player, args[2] or "default")
        return true
    elseif cmd == "clear" then
        clear_all_entities()
        return true
    elseif cmd == "debug" then
        state.config.debug_mode = not state.config.debug_mode
        log("Debug mode: " .. tostring(state.config.debug_mode))
        return true
    elseif cmd == "stats" then
        show_stats(player)
        return true
    end
    
    return false
end

--------------------------------------------------------------------------------
-- SECTION 8: GAME LOGIC
--------------------------------------------------------------------------------

function spawn_entity(player, entity_type)
    if state.entity_count >= state.config.max_entities then
        log("Maximum entities reached", "warning")
        return false
    end
    
    local player_pos = player:get_pos()
    local entity = Entity.new({
        id = state.entity_count + 1,
        type = entity_type,
        x = player_pos.x,
        y = player_pos.y + 2,
        z = player_pos.z,
        mass = 1,
        size = {x=0.5, y=0.5, z=0.5}
    })
    
    table.insert(state.entities, entity)
    state.entity_count = state.entity_count + 1
    
    -- Add to physics world
    if state.world and state.config.enable_physics then
        local body = physics.Body.new(
            entity.mass,
            entity.position,
            entity.size
        )
        state.world:add_body(body)
        entity.physics_body = body
    end
    
    log("Spawned entity " .. entity.id .. " of type " .. entity_type)
    return true
end

function clear_all_entities()
    for _, entity in ipairs(state.entities) do
        entity:cleanup()
    end
    state.entities = {}
    state.entity_count = 0
    
    if state.world then
        state.world = physics.World.new(10.0)
    end
    
    log("Cleared all entities")
end

function update_entities(dt)
    for i = #state.entities, 1, -1 do
        local entity = state.entities[i]
        
        if not entity.active then
            entity:cleanup()
            table.remove(state.entities, i)
            state.entity_count = state.entity_count - 1
            goto continue
        end
        
        entity:update(dt)
        
        ::continue::
    end
end

function show_stats(player)
    local stats = {
        entity_count = state.entity_count,
        max_entities = state.config.max_entities,
        physics_enabled = state.config.enable_physics,
        debug_mode = state.config.debug_mode
    }
    
    if state.world then
        local world_stats = state.world:get_stats()
        stats.physics_stats = world_stats
    end
    
    log("Stats: " .. minetest.serialize(stats))
    
    if player then
        player:send_message("Entities: " .. stats.entity_count .. "/" .. stats.max_entities)
    end
end

--------------------------------------------------------------------------------
-- SECTION 9: MAIN LOOP
--------------------------------------------------------------------------------

local function game_loop(dt)
    if state.paused then return end
    
    -- Update timers
    state.timers.update = state.timers.update + dt
    
    -- Fixed timestep for physics
    local fixed_dt = 1.0 / UPDATE_RATE
    while state.timers.update >= fixed_dt do
        -- Update physics
        if state.world and state.config.enable_physics then
            state.world:step(fixed_dt)
        end
        
        -- Update entities
        update_entities(fixed_dt)
        
        state.timers.update = state.timers.update - fixed_dt
    end
    
    -- Process queued events
    core.process_event_queue()
end

--------------------------------------------------------------------------------
-- SECTION 10: INITIALIZATION
--------------------------------------------------------------------------------

local function init_config()
    -- Load or create config
    state.config = merge_tables({}, DEFAULT_CONFIG)
    
    -- Override with saved settings if available
    local saved_config = minetest.settings_get(MOD_NAME .. "_config")
    if saved_config then
        local ok, decoded = pcall(minetest.deserialize, saved_config)
        if ok then
            merge_tables(state.config, decoded)
        end
    end
    
    log("Config loaded: max_entities=" .. state.config.max_entities)
end

local function register_events()
    core.register_event("world_load", on_world_load)
    core.register_event("world_unload", on_world_unload)
    core.register_event("player_join", on_player_join)
    core.register_event("player_leave", on_player_leave)
    core.register_event("chat_message", on_player_chat)
end

local function main_init()
    log("Initializing " .. MOD_NAME .. " v" .. VERSION)
    
    init_config()
    register_events()
    
    state.initialized = true
    
    log("Initialization complete")
end

-- Run initialization
main_init()

--------------------------------------------------------------------------------
-- SECTION 11: EXPORTS
--------------------------------------------------------------------------------

return {
    name = MOD_NAME,
    version = VERSION,
    spawn_entity = spawn_entity,
    clear_entities = clear_all_entities,
    get_stats = show_stats,
    toggle_debug = function()
        state.config.debug_mode = not state.config.debug_mode
    end
}
