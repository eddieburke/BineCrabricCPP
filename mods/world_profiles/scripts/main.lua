local PROFILE_OPTION = "world_profiles:type"

local profiles = {}
local options = {}
local option_specs = {}
local sliders = {}

local function register_profile(profile, source)
  if type(profile) ~= "table" or type(profile.id) ~= "string" then
    minecraft.log("warn", "world_profiles: " .. source .. " did not return a world type table")
    return
  end
  for _, existing in ipairs(profiles) do
    if existing.id == profile.id then
      minecraft.log("warn", "world_profiles: duplicate world type '" .. profile.id .. "' from " .. source)
      return
    end
  end
  profile.label = profile.label or profile.id
  profile.order = tonumber(profile.order) or 100
  profiles[#profiles + 1] = profile
  for _, spec in ipairs(profile.options or {}) do
    if option_specs[spec.key] ~= nil then
      minecraft.log("warn", "world_profiles: duplicate option key '" .. spec.key .. "' from " .. source)
    else
      spec.save_key = spec.save_key or ("world_profiles:" .. spec.key)
      option_specs[spec.key] = spec
      options[spec.key] = spec.default or spec.min or 0
      sliders[#sliders + 1] = {
        key = spec.key,
        label = spec.label or spec.key,
        min = spec.min,
        max = spec.max,
        integer = spec.integer,
      }
    end
  end
end

for _, entry in ipairs(minecraft.require_dir("scripts/worldtypes")) do
  register_profile(entry.module, "scripts/worldtypes/" .. entry.name .. ".lua")
end

table.sort(profiles, function(a, b)
  if a.order ~= b.order then
    return a.order < b.order
  end
  return a.id < b.id
end)

if #profiles == 0 then
  profiles[1] = { id = "default", label = "Default", order = 10 }
end

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

minecraft.on(minecraft.events.create_world, {}, function(event)
  event.options = event.options or {}
  event.options[PROFILE_OPTION] = selected_id()
  for key, spec in pairs(option_specs) do
    event.options[spec.save_key] = tostring(options[key])
  end
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
  for key, spec in pairs(option_specs) do
    options[key] = tonumber(event.options and event.options[spec.save_key]) or options[key]
  end
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

if #sliders > 0 then
  minecraft.screen.settings({
    id = "world_profiles:options",
    title = "World Profile Options",
    parent_screen = minecraft.screen.ids.create_world,
    parent_region = minecraft.screen.regions.footer,
    button_label = "Profile Options...",
    values = function() return options end,
    sliders = sliders,
    priority = 90,
  })
end

-- A world type without `spawn` leaves the vanilla sand beach search alone.
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
  local min_y, max_y = spawn(options)
  if min_y == nil then
    return
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
    minecraft.generation.stages.terrain,
    minecraft.generation.stages.surface,
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
  local profile = profile_for_event(event)
  event.cancel_vanilla = true
  if profile.generate ~= nil then
    profile.generate(event, options)
  end
end)

minecraft.on(minecraft.events.chunk_generation, {
  stage = minecraft.generation.stages.surface,
  moment = minecraft.generation.moments.after,
  when = minecraft.util.real_world,
  is_overworld = true,
  priority = 100,
}, function(event)
  local decorate = profile_for_event(event).decorate
  if decorate ~= nil then
    decorate(event, options)
  end
end)
