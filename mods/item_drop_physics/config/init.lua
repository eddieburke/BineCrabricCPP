-- Item Drop Physics Configuration Module
-- Handles loading and normalization of physics properties

local config = {}

--------------------------------------------------------------------------------
-- DEFAULT PHYSICS VALUES
--------------------------------------------------------------------------------

config.DEFAULT_PHYSICS = {
  material = "generic",
  density = 1.10,
  saturated_density = 1.10,
  water_absorption_rate = 0.0,
  drying_rate = 0.0,
  mass = 1.0,
  air_drag = 0.98,
  ground_drag = 0.80,
  friction = 0.60,
  restitution = 0.25,
  water_restitution = 0.06,
  water_drag = 0.80,
  water_vertical_drag = 0.76,
  air_angular_damping = 0.05,
  ground_angular_damping = 0.70,
  water_angular_damping = 1.65,
  flow_coupling = 0.90,
  buoyancy_cap = 1.35,
  rest_speed = 0.005,
  angular_rest_speed = 0.01,
  impact_rest_speed = 0.03,
  rolling_resistance = 0.015,
}

--------------------------------------------------------------------------------
-- VALUE CLAMPING WITH LOGGING
--------------------------------------------------------------------------------

local clamp_number_logged = {}

function config.clamp_number(value, fallback, lo, hi)
  value = tonumber(value)
  if value == nil or value ~= value or value == math.huge or value == -math.huge then
    local dedup = tostring(value) .. ":"
    if not clamp_number_logged[dedup] then
      clamp_number_logged[dedup] = true
      minetest.log("warning", "item_drop_physics: using fallback for invalid physics value")
    end
    value = fallback
  end
  if value < lo then return lo end
  if value > hi then return hi end
  return value
end

--------------------------------------------------------------------------------
-- PHYSICS NORMALIZATION
--------------------------------------------------------------------------------

function config.normalize_physics(source)
  source = type(source) == "table" and source or config.DEFAULT_PHYSICS
  return {
    name = tostring(source.name or "mod_or_unknown_default"),
    material = tostring(source.material or config.DEFAULT_PHYSICS.material),
    density = config.clamp_number(source.density, config.DEFAULT_PHYSICS.density, 0.01, 40.0),
    saturated_density = config.clamp_number(source.saturated_density, source.density or config.DEFAULT_PHYSICS.saturated_density, 0.01, 40.0),
    water_absorption_rate = config.clamp_number(source.water_absorption_rate, config.DEFAULT_PHYSICS.water_absorption_rate, 0.0, 1.0),
    drying_rate = config.clamp_number(source.drying_rate, config.DEFAULT_PHYSICS.drying_rate, 0.0, 1.0),
    mass = config.clamp_number(source.mass, config.DEFAULT_PHYSICS.mass, 0.02, 8.0),
    air_drag = config.clamp_number(source.air_drag, config.DEFAULT_PHYSICS.air_drag, 0.10, 0.9999),
    ground_drag = config.clamp_number(source.ground_drag, config.DEFAULT_PHYSICS.ground_drag, 0.10, 0.9999),
    friction = config.clamp_number(source.friction, config.DEFAULT_PHYSICS.friction, 0.0, 2.0),
    restitution = config.clamp_number(source.restitution, config.DEFAULT_PHYSICS.restitution, 0.0, 0.95),
    water_restitution = config.clamp_number(source.water_restitution, config.DEFAULT_PHYSICS.water_restitution, 0.0, 0.50),
    water_drag = config.clamp_number(source.water_drag, config.DEFAULT_PHYSICS.water_drag, 0.05, 0.9999),
    water_vertical_drag = config.clamp_number(source.water_vertical_drag, config.DEFAULT_PHYSICS.water_vertical_drag, 0.05, 0.9999),
    air_angular_damping = config.clamp_number(source.air_angular_damping, config.DEFAULT_PHYSICS.air_angular_damping, 0.0, 10.0),
    ground_angular_damping = config.clamp_number(source.ground_angular_damping, config.DEFAULT_PHYSICS.ground_angular_damping, 0.0, 10.0),
    water_angular_damping = config.clamp_number(source.water_angular_damping, config.DEFAULT_PHYSICS.water_angular_damping, 0.0, 12.0),
    flow_coupling = config.clamp_number(source.flow_coupling, config.DEFAULT_PHYSICS.flow_coupling, 0.0, 3.0),
    buoyancy_cap = config.clamp_number(source.buoyancy_cap, config.DEFAULT_PHYSICS.buoyancy_cap, 0.05, 2.0),
    rest_speed = config.clamp_number(source.rest_speed, config.DEFAULT_PHYSICS.rest_speed, 0.0, 0.10),
    angular_rest_speed = config.clamp_number(source.angular_rest_speed, config.DEFAULT_PHYSICS.angular_rest_speed, 0.0, 0.50),
    impact_rest_speed = config.clamp_number(source.impact_rest_speed, config.DEFAULT_PHYSICS.impact_rest_speed, 0.0, 0.20),
    rolling_resistance = config.clamp_number(source.rolling_resistance, config.DEFAULT_PHYSICS.rolling_resistance, 0.0, 0.50),
  }
end

--------------------------------------------------------------------------------
-- DATABASE LOADING
--------------------------------------------------------------------------------

local physics_database = nil
local physics_cache = {}

function config.load_database()
  local raw = minetest.get_mod_storage():get_string("item_physics_json")
  if type(raw) ~= "string" or raw == "" then
    -- Try to load from asset file
    local file = io.open(minetest.get_modpath("item_drop_physics") .. "/assets/item_physics.json", "r")
    if file then
      raw = file:read("*all")
      file:close()
    end
  end
  
  if type(raw) ~= "string" then
    return { default = config.DEFAULT_PHYSICS, blocks = {}, items = {} }
  end
  
  local ok, decoded = pcall(minetest.deserialize, raw)
  if not ok or type(decoded) ~= "table" then
    return { default = config.DEFAULT_PHYSICS, blocks = {}, items = {} }
  end
  
  decoded.blocks = type(decoded.blocks) == "table" and decoded.blocks or {}
  decoded.items = type(decoded.items) == "table" and decoded.items or {}
  decoded.default = type(decoded.default) == "table" and decoded.default or config.DEFAULT_PHYSICS
  
  return decoded
end

function config.init_database()
  physics_database = config.load_database()
  physics_cache = {}
  return physics_database
end

--------------------------------------------------------------------------------
-- PHYSICS LOOKUP
--------------------------------------------------------------------------------

function config.get_physics(item_id, item_damage)
  local key = tostring(item_id) .. ":" .. tostring(item_damage or 0)
  local cached = physics_cache[key]
  if cached then return cached end

  local collection = (item_id >= 1 and item_id <= 255)
                     and physics_database.blocks or physics_database.items
  local source = collection[tostring(item_id)] or collection[item_id]
  local result
  
  if type(source) == "table" then
    local variant = type(source.variants) == "table"
                    and (source.variants[tostring(item_damage or 0)] or source.variants[item_damage or 0])
    if type(variant) == "table" then
      local merged = {}
      for k, v in pairs(source) do merged[k] = v end
      for k, v in pairs(variant) do merged[k] = v end
      result = config.normalize_physics(merged)
    else
      result = config.normalize_physics(source)
    end
  else
    result = config.normalize_physics(physics_database.default)
  end
  
  physics_cache[key] = result
  return result
end

function config.clear_cache()
  physics_cache = {}
end

return config
