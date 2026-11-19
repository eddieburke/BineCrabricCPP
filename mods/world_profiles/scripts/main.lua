local registry = require("scripts.registry")
local settings_screen = require("scripts.settings_screen")

local chosen = registry:selection()
local active

local SELECTION_KEY = "world_profiles:selection"

local function publish()
  local saved = {}
  registry:save(chosen, saved)
  minecraft.shared.put(SELECTION_KEY, assert(minecraft.util.json_encode(saved)))
end

publish()

local function selection_for(event)
  if event.has_world then return active end
  return chosen
end

minecraft.on("create_world", {}, function(event)
  registry:save(chosen, event.options)
end)

minecraft.on("world_open", {}, function(event)
  active = nil
  active = registry:load(event.options)
end)

local function profile_label()
  return "World Type: " .. registry:profile(chosen, 0).label
end

minecraft.on("screen_ui", {
  screen_id = minecraft.screen.ids.create_world,
  region = minecraft.screen.regions.footer,
  priority = 100,
}, function(event)
  event.ui:add_stacked_centered_button(profile_label(), function()
    registry:cycle(chosen, 0)
    publish()
  end, profile_label)
end)

settings_screen.register({
  id = "world_profiles:options",
  title = "World Profile Options",
  parent_screen = minecraft.screen.ids.create_world,
  parent_region = minecraft.screen.regions.footer,
  button_label = "Profile Options...",
  values = function() return chosen.values end,
  sliders = function() return registry:sliders(chosen) end,
  on_change = publish,
  priority = 90,
})

settings_screen.register({
  id = "world_profiles:dimensions",
  title = "Dimension Profiles",
  parent_screen = minecraft.screen.ids.create_world,
  parent_region = minecraft.screen.regions.footer,
  button_label = "Dimension Profiles...",
  values = function() return {} end,
  cycles = function()
    local controls = {}
    for _, dimension_id in ipairs(registry.dimension_ids) do
      controls[#controls + 1] = {
        label = function()
          return registry.dimensions[dimension_id].name .. ": " .. registry:profile(chosen, dimension_id).label
        end,
        click = function() registry:cycle(chosen, dimension_id) end,
      }
    end
    return controls
  end,
  on_change = publish,
  priority = 80,
})

minecraft.on("world_spawn_search", {
  resolved = false,
  mod_generation = true,
  priority = 100,
}, function(event)
  local selection = selection_for(event)
  local profile = registry:profile(selection, event.dimension_id)
  if profile.spawn == nil then return end
  local min_y, max_y = profile.spawn(registry:options(selection, event.dimension_id))
  assert(type(min_y) == "number" and (max_y == nil or max_y >= min_y), "world_profiles: invalid spawn range")
  local origin_x, origin_z = event.x, event.z
  local dx, dz, vx, vz = 0, 0, 0, -1
  for _ = 1, 256 do
    local x, z = origin_x + dx * 16, origin_z + dz * 16
    local y, err = minecraft.world.get_top_y(x, z)
    assert(y, err)
    if y >= min_y and y <= 126 and (max_y == nil or y <= max_y)
      and minecraft.world.get_block(x, y, z) == 0 and minecraft.world.get_block(x, y + 1, z) == 0 then
      event.x, event.y, event.z, event.resolved = x, y, z, true
      return
    end
    if dx == dz or (dx < 0 and dx == -dz) or (dx > 0 and dx == 1 - dz) then vx, vz = -vz, vx end
    dx, dz = dx + vx, dz + vz
  end
  error("world_profiles: no valid spawn found for " .. profile.id)
end)
