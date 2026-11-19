local registry = require("scripts.registry")

local SELECTION_KEY = "world_profiles:selection"
local chosen = registry:selection()
local chosen_text
local active

local function preview_selection()
  local text = minecraft.shared.get(SELECTION_KEY)
  if text ~= nil and text ~= chosen_text then
    chosen = registry:load(assert(minecraft.util.json_decode(text)))
    chosen_text = text
  end
  return chosen
end

local function selection_for(event)
  if event.has_world then return active end
  return preview_selection()
end

minecraft.on("world_open", {}, function(event)
  active = nil
  active = registry:load(event.options)
end)

minecraft.on("chunk_generation", {
  moment = "before",
  mod_generation = true,
  priority = 100,
}, function(event)
  local selection = selection_for(event)
  local profile = registry:profile(selection, event.dimension_id)
  local action = profile.stages and profile.stages[event.stage]
  if action == nil then return end
  assert(not event.cancel_vanilla, "world_profiles: generation stage already claimed: " .. event.stage)
  event.cancel_vanilla = true
  if action then action(event, registry:options(selection, event.dimension_id)) end
end)

minecraft.on("chunk_generation", {
  moment = "after",
  mod_generation = true,
  priority = 100,
}, function(event)
  local selection = selection_for(event)
  local profile = registry:profile(selection, event.dimension_id)
  local action = profile.after and profile.after[event.stage]
  if action then action(event, registry:options(selection, event.dimension_id)) end
end)
