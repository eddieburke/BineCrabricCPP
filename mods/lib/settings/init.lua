-- Shared settings registry. settings.define() resolves each field once -- native
-- key name, storage path and validator -- so reading config.x during a render
-- hook is a table lookup plus one native get, not a type dispatch and a concat.

local settings = {}

local configs = {}

local function clamp_number(value, minimum, maximum)
  value = tonumber(value)
  if value == nil or value ~= value then
    return minimum
  end
  if value < minimum then return minimum end
  if value > maximum then return maximum end
  return value
end

local function make_validator(field)
  local field_type = field.type
  if field_type == nil then
    return function(value) return value end
  end
  if field_type == "slider" or field_type == "number" then
    local minimum, maximum = field.min or 0, field.max or 1
    if field.integer then
      return function(value) return math.floor(clamp_number(value, minimum, maximum) + 0.5) end
    end
    return function(value) return clamp_number(value, minimum, maximum) end
  end
  if field_type == "int" or field_type == "integer" then
    local minimum, maximum = field.min or 0, field.max or 2147483647
    return function(value) return math.floor(clamp_number(value, minimum, maximum)) end
  end
  if field_type == "bool" or field_type == "toggle" then
    return function(value) return value == true or value == 1 or value == "true" end
  end
  if field_type == "options" or field_type == "cycle" or field_type == "enum" then
    local fallback = field.default or ""
    return function(value) return tostring(value or fallback) end
  end
  if field_type == "string" then
    return function(value) return tostring(value or "") end
  end
  return function(value) return value end
end

local function native_kind(field_type)
  if field_type == "bool" or field_type == "toggle" then return "toggle" end
  if field_type == "options" or field_type == "cycle" or field_type == "enum" then return "options" end
  if field_type == "string" then return nil end
  return "slider"
end

function settings.define(mod_id, definition)
  if configs[mod_id] then return configs[mod_id] end

  local fields = definition.fields or {}
  local native_ui = definition.native_ui ~= false
  local entries = {}
  local resolved = {}
  local local_values = {}

  for field_key, field in pairs(fields) do
    local field_type = field.type or "slider"
    local key = field.key or field_key
    local kind = native_ui and native_kind(field_type) or nil
    local entry = {
      native = kind ~= nil,
      name = mod_id .. "." .. key,
      path = "settings/" .. key .. ".txt",
      default = field.default,
      validate = make_validator(field),
    }
    resolved[field_key] = entry

    if entry.native then
      local native_entry = { key = key, label = field.label or field_key, kind = kind }
      if kind == "toggle" then
        native_entry.default = field.default == true
      elseif kind == "options" then
        native_entry.options = field.options or {}
        native_entry.default = field.default
      else
        local integer = field_type == "int" or field_type == "integer" or field.integer == true
        native_entry.min = field.min or 0
        native_entry.max = field.max or 1
        native_entry.step = field.step or (integer and 1 or 0)
        native_entry.decimals = integer and 0 or (field.decimals or 2)
        native_entry.integer = integer
        native_entry.default = field.default or 0
      end
      entries[#entries + 1] = native_entry
    else
      local stored = minecraft.storage.read(entry.path)
      local_values[field_key] = entry.validate(stored == nil and entry.default or stored)
    end
  end

  if #entries > 0 then
    minecraft.settings.register(definition.name or mod_id, entries, {
      hide_with_shader_pack = definition.hide_with_shader_pack == true,
    })
  end

  local config = {}
  setmetatable(config, {
    __index = function(self, key)
      local entry = resolved[key]
      if entry == nil then return rawget(self, key) end
      if not entry.native then return local_values[key] end
      local value = minecraft.settings.get(entry.name)
      if value == nil then value = entry.default end
      return entry.validate(value)
    end,
    __newindex = function(self, key, value)
      local entry = resolved[key]
      if entry == nil then
        rawset(self, key, value)
        return
      end
      local validated = entry.validate(value)
      if entry.native then
        minecraft.settings.set(entry.name, validated)
        return
      end
      local_values[key] = validated
      assert(minecraft.storage.write(entry.path, tostring(validated)))
    end,
  })

  configs[mod_id] = config
  return config
end

return settings
