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

local function validate(field_type, value, definition)
  if field_type == "slider" or field_type == "number" then
    local result = clamp_number(value, definition.min or 0, definition.max or 1)
    if definition.integer then result = math.floor(result + 0.5) end
    return result
  end
  if field_type == "int" or field_type == "integer" then
    return math.floor(clamp_number(value, definition.min or 0, definition.max or 2147483647))
  end
  if field_type == "bool" or field_type == "toggle" then
    return value == true or value == 1 or value == "true"
  end
  if field_type == "options" or field_type == "cycle" or field_type == "enum" then
    return tostring(value or definition.default or "")
  end
  if field_type == "string" then
    return tostring(value or "")
  end
  return value
end

local function native_kind(field_type)
  if field_type == "bool" or field_type == "toggle" then return "toggle" end
  if field_type == "options" or field_type == "cycle" or field_type == "enum" then return "options" end
  if field_type == "string" then return nil end
  return "slider"
end

local function storage_path(key)
  return "settings/" .. key .. ".txt"
end

function settings.define(mod_id, definition)
  if configs[mod_id] then return configs[mod_id] end

  local fields = definition.fields or {}
  local native_ui = definition.native_ui ~= false
  local entries = {}

  if native_ui then
    for field_key, field in pairs(fields) do
      local field_type = field.type or "slider"
      local kind = native_kind(field_type)
      if kind ~= nil then
        local entry = {
          key = field.key or field_key,
          label = field.label or field_key,
          kind = kind,
        }
        if kind == "toggle" then
          entry.default = field.default == true
        elseif kind == "options" then
          entry.options = field.options or {}
          entry.default = field.default
        else
          local integer = field_type == "int" or field_type == "integer" or field.integer == true
          entry.min = field.min or 0
          entry.max = field.max or 1
          entry.step = field.step or (integer and 1 or 0)
          entry.decimals = integer and 0 or (field.decimals or 2)
          entry.integer = integer
          entry.default = field.default or 0
        end
        entries[#entries + 1] = entry
      end
    end
    if #entries > 0 then
      minecraft.settings.register(definition.name or mod_id, entries)
    end
  end

  local local_values = {}
  for field_key, field in pairs(fields) do
    if not native_ui or native_kind(field.type or "slider") == nil then
      local stored = minecraft.storage.read(storage_path(field.key or field_key))
      local_values[field_key] = validate(field.type, stored == nil and field.default or stored, field)
    end
  end

  local config = {}
  setmetatable(config, {
    __index = function(self, key)
      local field = fields[key]
      if field == nil then return rawget(self, key) end
      if native_ui and native_kind(field.type or "slider") ~= nil then
        local value = minecraft.settings.get(mod_id .. "." .. (field.key or key))
        return validate(field.type, value == nil and field.default or value, field)
      end
      return local_values[key]
    end,
    __newindex = function(self, key, value)
      local field = fields[key]
      if field == nil then
        rawset(self, key, value)
        return
      end
      local validated = validate(field.type, value, field)
      if native_ui and native_kind(field.type or "slider") ~= nil then
        minecraft.settings.set(mod_id .. "." .. (field.key or key), validated)
        return
      end
      local_values[key] = validated
      assert(minecraft.storage.write(storage_path(field.key or key), tostring(validated)))
    end,
  })

  configs[mod_id] = config
  return config
end

function settings.get_mod_config(mod_id)
  return configs[mod_id]
end

return settings
