-- ================================================================
-- LIB: settings
-- Unified configuration system for all mods
-- Eliminates duplication, provides automatic persistence & validation
-- ================================================================

local settings = {}

-- Internal storage for mod configurations
local _mod_configs = {}

--------------------------------------------------------------------------------
-- TYPE VALIDATION & CLAMPING
--------------------------------------------------------------------------------

local function clamp_number(value, min_val, max_val)
  if type(value) ~= "number" or value ~= value then -- NaN check
    return min_val
  end
  if value < min_val then return min_val end
  if value > max_val then return max_val end
  return value
end

local function validate_field(field_type, value, def)
  if field_type == "slider" or field_type == "number" then
    local min_v = def.min or 0
    local max_v = def.max or 1
    local clamped = clamp_number(value, min_v, max_v)
    if def.integer then
      clamped = math.floor(clamped + 0.5)
    end
    return clamped
  elseif field_type == "bool" or field_type == "toggle" then
    return value == true or value == 1 or value == "true"
  elseif field_type == "options" or field_type == "cycle" or field_type == "enum" then
    return tostring(value or def.default or "")
  elseif field_type == "string" then
    return tostring(value or "")
  elseif field_type == "int" or field_type == "integer" then
    return math.floor(clamp_number(value, def.min or 0, def.max or 2147483647))
  end
  return value
end

--------------------------------------------------------------------------------
-- PERSISTENCE LAYER (Simple Key-Value via C++ API)
--------------------------------------------------------------------------------

local function _load_value(mod_id, key, default)
  local value = minecraft.settings.get(mod_id .. "." .. key)
  if value == nil then
    return default
  end
  return value
end

local function _save_value(mod_id, key, value)
  minecraft.settings.set(mod_id .. "." .. key, value)
end

--------------------------------------------------------------------------------
-- CONFIG DEFINITION DSL
--------------------------------------------------------------------------------

function settings.define(mod_id, definition)
  if _mod_configs[mod_id] then
    return _mod_configs[mod_id]
  end

  local mod_name = definition.name or mod_id
  local fields = definition.fields or {}
  
  -- Register with C++ settings system (for UI display)
  local settings_list = {}
  for field_key, field_def in pairs(fields) do
    local ftype = field_def.type or "slider"
    local entry = {
      key = field_def.key or field_key,
      label = field_def.label or field_key,
    }
    
    if ftype == "bool" or ftype == "toggle" then
      entry.kind = "toggle"
      entry.default = field_def.default and true or false
    elseif ftype == "options" or ftype == "cycle" or ftype == "enum" then
      entry.kind = "options"
      entry.options = field_def.options or {}
      entry.default = field_def.default
    else
      entry.kind = "slider"
      entry.min = field_def.min or 0
      entry.max = field_def.max or 1
      entry.step = field_def.step or 0
      entry.decimals = field_def.decimals or (field_def.integer and 0 or 2)
      entry.integer = field_def.integer or false
      entry.default = field_def.default or 0
    end
    
    table.insert(settings_list, entry)
  end
  
  -- Register once with C++
  if definition.native_ui ~= false and #settings_list > 0 then
    minecraft.settings.register(mod_name, settings_list)
  end

  -- Create config proxy table
  local config_proxy = {}
  local config_meta = {}
  local native_ui = definition.native_ui ~= false
  local local_values = {}

  config_meta.__newindex = function(self, key, value)
    if fields[key] then
      local validated = validate_field(fields[key].type, value, fields[key])
      if native_ui then
        _save_value(mod_id, fields[key].key or key, validated)
      else
        local_values[key] = validated
      end
    else
      rawset(self, key, value)
    end
  end

  config_meta.__index = function(self, key)
    local field = fields[key]
    if field then
      local value = native_ui and _load_value(mod_id, field.key or key, field.default) or local_values[key]
      if value == nil then value = field.default end
      return validate_field(field.type, value, field)
    end
    return rawget(self, key)
  end

  setmetatable(config_proxy, config_meta)
  _mod_configs[mod_id] = config_proxy

  return config_proxy
end

--------------------------------------------------------------------------------
-- UTILITY FUNCTIONS
--------------------------------------------------------------------------------

function settings.get_mod_config(mod_id)
  return _mod_configs[mod_id]
end

return settings
