-- Single Create World profile selector backed by generic generation hooks.

local AIR_ID = 0
local PROFILE_OPTION = "world_profiles:type"

local profiles = {
  { id = "default", label = "Default" },
  { id = "flatlands", label = "Flatlands", cancel = { carver = true, features = true } },
  { id = "highlands", label = "Highlands", spawn = { min_y = 81 } },
  { id = "caves", label = "Caves", spawn = { min_y = 40, max_y = 72 } },
}

local selected_index = 1
local active_profile = profiles[1]

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

local function apply_flatlands()
  local grass_id = block_id("grass_block")
  local dirt_id = block_id("dirt")
  local bedrock_id = block_id("bedrock")
  if grass_id <= 0 or dirt_id <= 0 then
    return
  end
  minecraft.chunk.fill(0, 4, 0, 15, 127, 15, AIR_ID)
  minecraft.chunk.fill(0, 1, 0, 15, 2, 15, dirt_id)
  minecraft.chunk.fill(0, 3, 0, 15, 3, 15, grass_id)
  if bedrock_id > 0 then
    minecraft.chunk.fill(0, 0, 0, 15, 0, 15, bedrock_id)
  end
end

local function apply_highlands(event)
  local grass_id = block_id("grass_block")
  local dirt_id = block_id("dirt")
  local stone_id = block_id("stone")
  if grass_id <= 0 or dirt_id <= 0 or stone_id <= 0 then
    return
  end
  local seed = math.floor(event.world_seed or 0)
  for x = 0, 15 do
    for z = 0, 15 do
      local surface = math.max(0, minecraft.chunk.get_height(x, z) - 1)
      local wx = event.chunk_x * 16 + x
      local wz = event.chunk_z * 16 + z
      local boost = 12 + (hash_coord(wx, wz, seed) % 20)
      local top = math.min(118, surface + boost)
      if top > surface then
        minecraft.chunk.fill(x, surface + 1, z, x, top, z, stone_id)
      end
      minecraft.chunk.set_block(x, top, z, grass_id)
      if top - 1 > 0 then
        minecraft.chunk.set_block(x, top - 1, z, dirt_id)
      end
      if top - 2 > 0 then
        minecraft.chunk.set_block(x, top - 2, z, dirt_id)
      end
      if top - 3 > 0 then
        minecraft.chunk.set_block(x, top - 3, z, dirt_id)
      end
    end
  end
end

local function apply_caves(event)
  local grass_id = block_id("grass_block")
  local dirt_id = block_id("dirt")
  if grass_id <= 0 or dirt_id <= 0 then
    return
  end
  local seed = math.floor(event.world_seed or 0)
  for x = 0, 15 do
    for z = 0, 15 do
      local surface = math.max(0, minecraft.chunk.get_height(x, z) - 1)
      local top = math.max(48, surface - 8)
      if surface > top then
        minecraft.chunk.fill(x, top + 1, z, x, surface, z, AIR_ID)
      end
      minecraft.chunk.set_block(x, top, z, grass_id)
      if top - 1 > 0 then
        minecraft.chunk.set_block(x, top - 1, z, dirt_id)
      end
      if top - 2 > 0 then
        minecraft.chunk.set_block(x, top - 2, z, dirt_id)
      end
      if top - 3 > 0 then
        minecraft.chunk.set_block(x, top - 3, z, dirt_id)
      end
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
                minecraft.chunk.set_block(lx, cy + dy, lz, AIR_ID)
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

minecraft.on(minecraft.events.create_world, function(event)
  event.options = event.options or {}
  event.options[PROFILE_OPTION] = selected_id()
  active_profile = selected_profile()
end)

minecraft.on(minecraft.events.world_open, function(event)
  local id = event.options and event.options[PROFILE_OPTION] or "default"
  local profile, index = profile_by_id(id)
  if profile == nil then
    minecraft.log("warn", "world_profiles: unknown saved profile '" .. tostring(id) .. "'; using default")
    profile, index = profiles[1], 1
  end
  active_profile = profile
  if event.new_world then
    selected_index = index
  end
  minecraft.log("info", "world_profiles: opened " .. tostring(event.save_name) .. " as " .. profile.id)
end)

minecraft.screen.on_ui(minecraft.screen.ids.create_world, minecraft.screen.regions.footer, function(event)
  if event.ui ~= nil then
    event.ui.add_stacked_centered_button(profile_label(), cycle_profile)
  end
  return event
end, 100)

minecraft.on(minecraft.events.world_spawn_search, {
  resolved = false,
  is_overworld = true,
  when = minecraft.util.real_world,
  priority = 100,
}, function(event)
  local spawn = profile_for_event(event).spawn
  if spawn == nil then
    return
  end
  for _ = 0, 48 do
    local y = minecraft.world.get_top_y(event.x, event.z)
    if y >= spawn.min_y and (spawn.max_y == nil or y <= spawn.max_y) then
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
