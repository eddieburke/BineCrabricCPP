local config = require("config")
local settings_screen = require("scripts.settings_screen")

local PROFILE_OPTION = "world_profiles:type"
local DIMENSION_OPTION = "world_profiles:dim:"
local DIMENSION_PATTERN = "^world_profiles:dim:(-?%d+)$"
local TERRAIN_OPTION = "terrain_style:"

local profiles = {}
local options = {}
local option_specs = {}
local sliders = {}
local dimension_info = {}

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

for _, name in ipairs({
  "scripts.worldtypes.default",
  "scripts.worldtypes.flatlands",
  "scripts.worldtypes.highlands",
  "scripts.worldtypes.caves",
  "scripts.worldtypes.alpha",
  "scripts.worldtypes.modern",
  "scripts.worldtypes.infdev_20100227",
  "scripts.worldtypes.infdev_20100415",
}) do
  register_profile(require(name), name:gsub("%.", "/") .. ".lua")
end

table.sort(profiles, function(a, b)
  if a.order ~= b.order then
    return a.order < b.order
  end
  return a.id < b.id
end)

if #profiles == 0 then
  profiles[1] = { id = "default", label = "Default", order = 10, dimensions = "all" }
end

local default_profile = profiles[1]
local chosen = {}
local active = {}

local function profile_by_id(id)
  for _, profile in ipairs(profiles) do
    if profile.id == id then
      return profile
    end
  end
  return nil
end

local function refresh_dimensions()
  dimension_info = {}
  for _, info in ipairs(minecraft.world.dimensions()) do
    dimension_info[info.id] = info
  end
end

local function supports(profile, dimension_id)
  if profile == default_profile or profile.dimensions == "all" then
    return true
  end
  local info = dimension_info[dimension_id]
  if profile.terrain ~= nil and (info == nil or info.terrain_styles ~= true) then
    return false
  end
  if type(profile.dimensions) == "function" then
    return profile.dimensions(dimension_id, info) == true
  end
  if profile.dimensions == nil then
    return profile.terrain ~= nil or dimension_id == 0
  end
  for _, id in ipairs(profile.dimensions) do
    if id == dimension_id then
      return true
    end
  end
  return false
end

local function resolve(store, dimension_id)
  local profile = store[dimension_id]
  if profile ~= nil and supports(profile, dimension_id) then
    return profile
  end
  return default_profile
end

local function cycle(store, dimension_id)
  local current = resolve(store, dimension_id)
  local start = 1
  for i, profile in ipairs(profiles) do
    if profile == current then
      start = i
      break
    end
  end
  for step = 1, #profiles do
    local candidate = profiles[(start - 1 + step) % #profiles + 1]
    if supports(candidate, dimension_id) then
      store[dimension_id] = candidate
      return
    end
  end
end

local function profile_label()
  return "World Type: " .. resolve(chosen, 0).label
end

local function copy_store(store)
  local copy = {}
  for dimension_id, profile in pairs(store) do
    copy[dimension_id] = profile
  end
  return copy
end

local function profile_for_event(event)
  local dimension_id = event.dimension_id or 0
  local store = event.has_world == false and chosen or active
  return resolve(store, dimension_id)
end

refresh_dimensions()

minecraft.on("create_world", {}, function(event)
  event.options = event.options or {}
  refresh_dimensions()
  event.options[PROFILE_OPTION] = resolve(chosen, 0).id
  for dimension_id in pairs(dimension_info) do
    local profile = resolve(chosen, dimension_id)
    event.options[DIMENSION_OPTION .. dimension_id] = profile.id
    if profile.terrain ~= nil then
      event.options[TERRAIN_OPTION .. dimension_id] = profile.terrain
    end
  end
  for key, spec in pairs(option_specs) do
    event.options[spec.save_key] = tostring(options[key])
  end
  active = copy_store(chosen)
end)

minecraft.on("world_open", {}, function(event)
  local saved = event.options or {}
  active = {}
  for key, id in pairs(saved) do
    local dimension_id = key:match(DIMENSION_PATTERN)
    if dimension_id ~= nil then
      local profile = profile_by_id(id)
      if profile == nil then
        minecraft.log("warn", "world_profiles: unknown saved profile '" .. tostring(id) .. "' for dimension " .. dimension_id)
      else
        active[tonumber(dimension_id)] = profile
      end
    end
  end
  if active[0] == nil then
    local legacy = profile_by_id(saved[PROFILE_OPTION] or "default")
    if legacy == nil then
      minecraft.log("warn", "world_profiles: unknown saved profile '" .. tostring(saved[PROFILE_OPTION]) .. "'; using default")
    end
    active[0] = legacy or default_profile
  end
  for key, spec in pairs(option_specs) do
    options[key] = tonumber(saved[spec.save_key]) or options[key]
  end
  if event.new_world then
    chosen = copy_store(active)
  end
  minecraft.log("info", "world_profiles: opened " .. tostring(event.save_name) .. " as " .. active[0].id)
end)

minecraft.on("screen_ui", {
  screen_id = minecraft.screen.ids.create_world,
  region = minecraft.screen.regions.footer,
  priority = 100,
}, function(event)
  if event.ui ~= nil then
    event.ui:add_stacked_centered_button(profile_label(), function() cycle(chosen, 0) end, profile_label)
  end
  return event
end)

if #sliders > 0 then
  settings_screen.register({
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

settings_screen.register({
  id = "world_profiles:dimensions",
  title = "Dimension Profiles",
  parent_screen = minecraft.screen.ids.create_world,
  parent_region = minecraft.screen.regions.footer,
  button_label = "Dimension Profiles...",
  values = function() return {} end,
  cycles = function()
    refresh_dimensions()
    local ids = {}
    for dimension_id in pairs(dimension_info) do
      ids[#ids + 1] = dimension_id
    end
    table.sort(ids)
    local controls = {}
    for _, dimension_id in ipairs(ids) do
      local info = dimension_info[dimension_id]
      controls[#controls + 1] = {
        label = function()
          return info.name .. ": " .. resolve(chosen, dimension_id).label
        end,
        click = function() cycle(chosen, dimension_id) end,
      }
    end
    return controls
  end,
  priority = 80,
})

-- A world type without `spawn` leaves the vanilla sand beach search alone.
minecraft.on("world_spawn_search", {
  resolved = false,
  is_overworld = true,
  when = minecraft.util.real_world,
  priority = 100,
}, function(event)
  local store = event.has_world == false and chosen or active
  local spawn = resolve(store, 0).spawn
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

minecraft.on("chunk_generation", {
  stage = {
    "terrain",
    "surface",
    "carver",
    "features",
  },
  moment = "before",
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

minecraft.on("chunk_generation", {
  stage = "surface",
  moment = "after",
  when = minecraft.util.real_world,
  priority = 100,
}, function(event)
  local decorate = profile_for_event(event).decorate
  if decorate ~= nil then
    decorate(event, options)
  end
end)
