-- Audio Management System
-- Centralized audio handling with pooling and volume control

local audio = {}

--------------------------------------------------------------------------------
-- CONSTANTS
--------------------------------------------------------------------------------

local MAX_SIMULTANEOUS_SOUNDS = 32
local DEFAULT_MASTER_VOLUME = 0.8
local DEFAULT_SFX_VOLUME = 0.7
local DEFAULT_MUSIC_VOLUME = 0.5

--------------------------------------------------------------------------------
-- AUDIO SOURCE POOL
--------------------------------------------------------------------------------

local source_pool = {
    available = {},
    in_use = {},
    max_size = MAX_SIMULTANEOUS_SOUNDS
}

function audio.init_source_pool()
    for i = 1, MAX_SIMULTANEOUS_SOUNDS do
        table.insert(source_pool.available, {
            source = nil,
            sound_data = nil,
            is_3d = false,
            position = {x=0, y=0, z=0},
            volume = 1.0,
            pitch = 1.0
        })
    end
end

function audio.acquire_source()
    if #source_pool.available == 0 then
        -- Find oldest non-critical sound and stop it
        if #source_pool.in_use > 0 then
            local oldest = table.remove(source_pool.in_use, 1)
            if oldest.source then
                oldest.source:stop()
            end
            table.insert(source_pool.available, oldest)
        else
            return nil
        end
    end
    
    local source = table.remove(source_pool.available)
    table.insert(source_pool.in_use, source)
    return source
end

function audio.release_source(source_wrapper)
    for i, s in ipairs(source_pool.in_use) do
        if s == source_wrapper then
            table.remove(source_pool.in_use, i)
            if source_wrapper.source then
                source_wrapper.source:stop()
            end
            table.insert(source_pool.available, source_wrapper)
            break
        end
    end
end

--------------------------------------------------------------------------------
-- VOLUME CONTROL
--------------------------------------------------------------------------------

local volumes = {
    master = DEFAULT_MASTER_VOLUME,
    sfx = DEFAULT_SFX_VOLUME,
    music = DEFAULT_MUSIC_VOLUME,
    ambient = 0.6
}

function audio.set_volume(category, value)
    if volumes[category] ~= nil then
        volumes[category] = math.max(0, math.min(1, value))
        audio.update_all_volumes()
    end
end

function audio.get_volume(category)
    return volumes[category] or 0
end

function audio.set_master_volume(value)
    volumes.master = math.max(0, math.min(1, value))
    audio.update_all_volumes()
end

function audio.update_all_volumes()
    -- Update all active sources with new volume settings
    for _, wrapper in ipairs(source_pool.in_use) do
        if wrapper.source then
            local category_volume = volumes[wrapper.category] or 1.0
            wrapper.source:setVolume(wrapper.base_volume * category_volume * volumes.master)
        end
    end
end

--------------------------------------------------------------------------------
-- SOUND LOADING AND PLAYBACK
--------------------------------------------------------------------------------

local loaded_sounds = {}

function audio.load_sound(name, path)
    if loaded_sounds[name] then
        return loaded_sounds[name]
    end
    
    local ok, sound_data = pcall(love.audio.newSource, path, "static")
    if not ok then
        minetest.log("error", "Failed to load sound: " .. name .. " at " .. path)
        return nil
    end
    
    loaded_sounds[name] = sound_data
    return sound_data
end

function audio.load_music(name, path)
    if loaded_sounds[name] then
        return loaded_sounds[name]
    end
    
    local ok, sound_data = pcall(love.audio.newSource, path, "stream")
    if not ok then
        minetest.log("error", "Failed to load music: " .. name .. " at " .. path)
        return nil
    end
    
    loaded_sounds[name] = sound_data
    return sound_data
end

function audio.play_sound(name, options)
    options = options or {}
    local category = options.category or "sfx"
    
    local sound_data = loaded_sounds[name]
    if not sound_data then
        minetest.log("warning", "Attempted to play unloaded sound: " .. name)
        return nil
    end
    
    local wrapper = audio.acquire_source()
    if not wrapper then
        minetest.log("warning", "No available audio sources")
        return nil
    end
    
    -- Clone the source
    wrapper.source = sound_data:clone()
    wrapper.category = category
    wrapper.base_volume = options.volume or 1.0
    wrapper.pitch = options.pitch or 1.0
    wrapper.is_3d = options.is_3d or false
    
    if wrapper.is_3d and options.position then
        wrapper.position = options.position
    end
    
    -- Apply volume (with category and master)
    local category_volume = volumes[category] or 1.0
    wrapper.source:setVolume(wrapper.base_volume * category_volume * volumes.master)
    wrapper.source:setPitch(wrapper.pitch)
    
    -- Set position for 3D sounds
    if wrapper.is_3d and options.listener_pos then
        local dx = wrapper.position.x - options.listener_pos.x
        local dy = wrapper.position.y - options.listener_pos.y
        local dz = wrapper.position.z - options.listener_pos.z
        local dist = math.sqrt(dx*dx + dy*dy + dz*dz)
        
        -- Distance attenuation
        local max_dist = options.max_distance or 50.0
        if dist > max_dist then
            audio.release_source(wrapper)
            return nil
        end
        
        local atten = 1.0 - (dist / max_dist)
        wrapper.source:setVolume(wrapper.source:getVolume() * atten)
        
        -- Stereo panning (simplified)
        local pan = dx / max_dist
        wrapper.source:setPan(pan)
    end
    
    wrapper.source:play()
    
    return wrapper
end

function audio.stop_sound(wrapper)
    if wrapper and wrapper.source then
        wrapper.source:stop()
        audio.release_source(wrapper)
    end
end

function audio.stop_all_sounds()
    for i = #source_pool.in_use, 1, -1 do
        local wrapper = source_pool.in_use[i]
        audio.stop_sound(wrapper)
    end
end

--------------------------------------------------------------------------------
-- MUSIC CONTROL
--------------------------------------------------------------------------------

local current_music = nil
local music_playlist = {}
local playlist_index = 1
local music_loop = true

function audio.play_music(name, fade_in)
    if current_music then
        if fade_in then
            current_music:setVolume(0)
            -- Fade out logic would go here
        end
        current_music:stop()
    end
    
    local music_data = loaded_sounds[name]
    if not music_data then
        minetest.log("error", "Music not found: " .. name)
        return false
    end
    
    current_music = music_data:clone()
    current_music:setVolume(volumes.music * volumes.master)
    current_music:setLoop(music_loop)
    current_music:play()
    
    if fade_in then
        -- Fade in logic would go here
    end
    
    return true
end

function audio.stop_music(fade_out)
    if current_music then
        if fade_out then
            -- Fade out logic would go here
        end
        current_music:stop()
        current_music = nil
    end
end

function audio.add_to_playlist(name)
    table.insert(music_playlist, name)
end

function audio.clear_playlist()
    music_playlist = {}
    playlist_index = 1
end

function audio.next_track()
    if #music_playlist == 0 then return end
    
    playlist_index = playlist_index + 1
    if playlist_index > #music_playlist then
        playlist_index = 1
    end
    
    audio.play_music(music_playlist[playlist_index])
end

--------------------------------------------------------------------------------
-- UPDATE LOOP
--------------------------------------------------------------------------------

function audio.update(dt)
    -- Clean up finished sounds
    for i = #source_pool.in_use, 1, -1 do
        local wrapper = source_pool.in_use[i]
        if wrapper.source and not wrapper.source:isPlaying() then
            audio.release_source(wrapper)
        end
    end
    
    -- Auto-advance playlist
    if current_music and not current_music:isPlaying() and music_loop and #music_playlist > 0 then
        audio.next_track()
    end
end

function audio.get_stats()
    return {
        active_sounds = #source_pool.in_use,
        available_sources = #source_pool.available,
        loaded_sounds = 0,
        total_loaded = 0
    }
end

-- Initialize on module load
audio.init_source_pool()

return audio
