-- Create World profiles with tunable options + generation/spawn hooks.

local AIR_ID = 0
local PROFILE_OPTION = "world_profiles:type"
local FLAT_HEIGHT_OPTION = "world_profiles:flat_height"
local HIGHLAND_BOOST_OPTION = "world_profiles:highland_boost"
local CAVE_MIN_Y_OPTION = "world_profiles:cave_min_y"
local CAVE_MAX_Y_OPTION = "world_profiles:cave_max_y"

local profiles = {
  { id = "default", label = "Default" },
  { id = "flatlands", label = "Flatlands", cancel = { carver = true, features = true } },
  { id = "highlands", label = "Highlands", spawn = { min_y = 81 } },
  { id = "caves", label = "Caves", spawn = { min_y = 40, max_y = 72 } },
}

local selected_index = 1
local active_profile = profiles[1]
local options = {
  flat_height = 4,
  highland_boost = 16,
  cave_min_y = 40,
  cave_max_y = 72,
}

local function profile_by_id(id)
  for i, profile in ipairs(profiles) do
    if profile.id == id then
      return profile, i
    end
  end
  return nil, nil
end

local function selected_profile()
  return profiles[selected_index]
end

local function selected_id()
  return selected_profile().id
end

local function profile_label()
  return "World Type: " .. selected_profile().label
end

local function cycle_profile()
  selected_index = selected_index + 1
  if selected_index > #profiles then
    selected_index = 1
  end
  minecraft.log("info", "world_profiles: selected " .. selected_id())
end

local function profile_for_event(event)
  if event ~= nil and event.has_world == false then
    return selected_profile()
  end
  return active_profile
end

local function block_id(name)
  local id = minecraft.world.block_id(name)
  return id ~= nil and id or 0
end

local function hash_coord(x, z, seed)
  local n = (x * 374761393 + z * 668265263 + seed * 1013904223) % 2147483647
  return (n * n * n * 60493) % 2147483647
end

local function tonumber_opt(map, key, fallback)
  if map == nil then
    return fallback
  end
  return tonumber(map[key]) or fallback
end

local function apply_flatlands(event)
  local grass_id = block_id("grass_block")
  local dirt_id = block_id("dirt")
  local bedrock_id = block_id("bedrock")
  if grass_id <= 0 or dirt_id <= 0 then
    return
  end
  local chunk = event.chunk
  local height = math.max(1, math.min(64, math.floor(options.flat_height + 0.5)))
  chunk:fill(0, height, 0, 15, 127, 15, AIR_ID)
  if height >= 2 then
    chunk:fill(0, 1, 0, 15, height - 1, 15, dirt_id)
  end
  chunk:fill(0, height, 0, 15, height, 15, grass_id)
  if bedrock_id > 0 then
    chunk:fill(0, 0, 0, 15, 0, 15, bedrock_id)
  end
end

local function apply_highlands(event)
  local grass_id = block_id("grass_block")
  local dirt_id = block_id("dirt")
  local stone_id = block_id("stone")
  if grass_id <= 0 or dirt_id <= 0 or stone_id <= 0 then
    return
  end
  local chunk = event.chunk
  local seed = math.floor(event.world_seed or 0)
  local boost_base = math.max(4, math.min(40, math.floor(options.highland_boost + 0.5)))
  for x = 0, 15 do
    for z = 0, 15 do
      local surface = math.max(0, chunk:get_height(x, z) - 1)
      local wx = event.chunk_x * 16 + x
      local wz = event.chunk_z * 16 + z
      local boost = boost_base + (hash_coord(wx, wz, seed) % 20)
      local top = math.min(118, surface + boost)
      if top > surface then
        chunk:fill(x, surface + 1, z, x, top, z, stone_id)
      end
      chunk:set_block(x, top, z, grass_id)
      if top - 1 > 0 then chunk:set_block(x, top - 1, z, dirt_id) end
      if top - 2 > 0 then chunk:set_block(x, top - 2, z, dirt_id) end
      if top - 3 > 0 then chunk:set_block(x, top - 3, z, dirt_id) end
    end
  end
end

local function apply_caves(event)
  local grass_id = block_id("grass_block")
  local dirt_id = block_id("dirt")
  if grass_id <= 0 or dirt_id <= 0 then
    return
  end
  local chunk = event.chunk
  local seed = math.floor(event.world_seed or 0)
  for x = 0, 15 do
    for z = 0, 15 do
      local surface = math.max(0, chunk:get_height(x, z) - 1)
      local top = math.max(48, surface - 8)
      if surface > top then
        chunk:fill(x, top + 1, z, x, surface, z, AIR_ID)
      end
      chunk:set_block(x, top, z, grass_id)
      if top - 1 > 0 then chunk:set_block(x, top - 1, z, dirt_id) end
      if top - 2 > 0 then chunk:set_block(x, top - 2, z, dirt_id) end
      if top - 3 > 0 then chunk:set_block(x, top - 3, z, dirt_id) end
      local wx = event.chunk_x * 16 + x
      local wz = event.chunk_z * 16 + z
      local pocket = hash_coord(wx, wz, seed + 17) % 11
      if pocket < 2 then
        local cy = math.max(8, top - 6 - (pocket * 3))
        for dy = -2, 2 do
          for dx = -1, 1 do
            for dz = -1, 1 do
              local lx = x + dx
              local lz = z + dz
              if lx >= 0 and lx <= 15 and lz >= 0 and lz <= 15 then
                chunk:set_block(lx, cy + dy, lz, AIR_ID)
              end
            end
          end
        end
      end
    end
  end
end

profiles[2].generate = apply_flatlands
profiles[3].generate = apply_highlands
profiles[4].generate = apply_caves

minecraft.on(minecraft.events.create_world, {}, function(event)
  event.options = event.options or {}
  event.options[PROFILE_OPTION] = selected_id()
  event.options[FLAT_HEIGHT_OPTION] = tostring(options.flat_height)
  event.options[HIGHLAND_BOOST_OPTION] = tostring(options.highland_boost)
  event.options[CAVE_MIN_Y_OPTION] = tostring(options.cave_min_y)
  event.options[CAVE_MAX_Y_OPTION] = tostring(options.cave_max_y)
  active_profile = selected_profile()
end)

minecraft.on(minecraft.events.world_open, {}, function(event)
  local id = event.options and event.options[PROFILE_OPTION] or "default"
  local profile, index = profile_by_id(id)
  if profile == nil then
    minecraft.log("warn", "world_profiles: unknown saved profile '" .. tostring(id) .. "'; using default")
    profile, index = profiles[1], 1
  end
  active_profile = profile
  options.flat_height = tonumber_opt(event.options, FLAT_HEIGHT_OPTION, options.flat_height)
  options.highland_boost = tonumber_opt(event.options, HIGHLAND_BOOST_OPTION, options.highland_boost)
  options.cave_min_y = tonumber_opt(event.options, CAVE_MIN_Y_OPTION, options.cave_min_y)
  options.cave_max_y = tonumber_opt(event.options, CAVE_MAX_Y_OPTION, options.cave_max_y)
  if event.new_world then
    selected_index = index
  end
  minecraft.log("info", "world_profiles: opened " .. tostring(event.save_name) .. " as " .. profile.id)
end)

minecraft.screen.on_ui(minecraft.screen.ids.create_world, minecraft.screen.regions.footer, function(event)
  if event.ui ~= nil then
    event.ui:add_stacked_centered_button(profile_label(), cycle_profile)
  end
  return event
end, 100)

minecraft.screen.settings({
  id = "world_profiles:options",
  title = "World Profile Options",
  parent_screen = minecraft.screen.ids.create_world,
  parent_region = minecraft.screen.regions.footer,
  button_label = "Profile Options...",
  values = function() return options end,
  sliders = {
    { key = "flat_height", label = "Flat Height", min = 1, max = 32, integer = true },
    { key = "highland_boost", label = "Highland Boost", min = 4, max = 40, integer = true },
    { key = "cave_min_y", label = "Cave Spawn Min Y", min = 8, max = 80, integer = true },
    { key = "cave_max_y", label = "Cave Spawn Max Y", min = 16, max = 100, integer = true },
  },
  priority = 90,
})

-- Default: do not resolve spawn — vanilla sand beach search runs.
-- Highlands: surface Y band. Caves: intentional underground band from options.
minecraft.on(minecraft.events.world_spawn_search, {
  resolved = false,
  is_overworld = true,
  when = minecraft.util.real_world,
  priority = 100,
}, function(event)
  local profile = profile_for_event(event)
  if profile.id == "default" or profile.id == "flatlands" then
    return
  end
  local spawn = profile.spawn
  if spawn == nil then
    return
  end
  local min_y = spawn.min_y
  local max_y = spawn.max_y
  if profile.id == "caves" then
    min_y = options.cave_min_y
    max_y = options.cave_max_y
    if min_y > max_y then
      min_y, max_y = max_y, min_y
    end
  end
  for _ = 0, 48 do
    local y = minecraft.world.get_top_y(event.x, event.z)
    if y >= min_y and (max_y == nil or y <= max_y) then
      event.y = y
      event.resolved = true
      break
    end
    event.x = event.x + 16
    event.z = event.z + 16
  end
end)

minecraft.on(minecraft.events.chunk_generation, {
  stage = {
    minecraft.generation.stages.carver,
    minecraft.generation.stages.features,
  },
  moment = minecraft.generation.moments.before,
  is_overworld = true,
  priority = 100,
  when = function(event)
    if not minecraft.util.real_world(event) then
      return false
    end
    local cancel = profile_for_event(event).cancel
    return cancel ~= nil and cancel[event.stage] == true
  end,
}, function(event)
  event.cancel_vanilla = true
end)

minecraft.on(minecraft.events.chunk_generation, {
  stage = minecraft.generation.stages.surface,
  moment = minecraft.generation.moments.after,
  when = minecraft.util.real_world,
  is_overworld = true,
  priority = 100,
}, function(event)
  local generate = profile_for_event(event).generate
  if generate ~= nil then
    generate(event)
  end
end)
