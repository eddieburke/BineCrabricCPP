local Profiles = {}
Profiles.__index = Profiles

local PREFIX = "world_profiles:"
local SCHEMA = PREFIX .. "schema"
local DIMENSION = PREFIX .. "dim:"
local OPTION = PREFIX .. "option:"
local STAGES = { terrain = true, surface = true, carver = true, features = true }
local TERRAIN = { beta = true, alpha = true }

local function check(ok, message)
  assert(ok, "world_profiles: " .. message)
end

local function option_key(dimension_id, key)
  return tostring(dimension_id) .. ":" .. key
end

local function number_value(value, spec)
  local number = tonumber(value)
  check(number ~= nil and number == number and number >= spec.min and number <= spec.max,
    "invalid value for " .. spec.key .. ": " .. tostring(value))
  check(not spec.integer or number % 1 == 0, "expected integer for " .. spec.key)
  return number
end

function Profiles.new(definitions, dimensions)
  local self = setmetatable({ list = {}, by_id = {}, dimensions = {}, dimension_ids = {} }, Profiles)
  for _, info in ipairs(dimensions) do
    check(math.type(info.id) == "integer" and self.dimensions[info.id] == nil, "invalid or duplicate dimension")
    self.dimensions[info.id] = info
    self.dimension_ids[#self.dimension_ids + 1] = info.id
  end
  check(self.dimensions[0] ~= nil, "overworld dimension is missing")
  table.sort(self.dimension_ids)
  for _, profile in ipairs(definitions) do
    check(type(profile) == "table" and type(profile.id) == "string" and profile.id:match("^[a-z][a-z0-9_]*$"),
      "invalid profile id")
    check(self.by_id[profile.id] == nil, "duplicate profile " .. profile.id)
    check(type(profile.label) == "string" and profile.label ~= "", "missing label for " .. profile.id)
    check(type(profile.order) == "number", "missing order for " .. profile.id)
    check(profile.dimensions == "all" or type(profile.dimensions) == "table" or type(profile.dimensions) == "function",
      "missing dimensions for " .. profile.id)
    check(profile.terrain == nil or TERRAIN[profile.terrain], "invalid terrain for " .. profile.id)
    check(profile.cancel == nil and profile.generate == nil and profile.decorate == nil,
      "obsolete stage contract for " .. profile.id)
    for stage, action in pairs(profile.stages or {}) do
      check(STAGES[stage] and (action == false or type(action) == "function"), "invalid stage for " .. profile.id)
    end
    for stage, action in pairs(profile.after or {}) do
      check(STAGES[stage] and type(action) == "function", "invalid after stage for " .. profile.id)
    end
    check(profile.spawn == nil or type(profile.spawn) == "function", "invalid spawn for " .. profile.id)
    local keys = {}
    for _, spec in ipairs(profile.options or {}) do
      check(type(spec.key) == "string" and spec.key:match("^[a-z][a-z0-9_]*$") and not keys[spec.key],
        "invalid or duplicate option for " .. profile.id)
      check(type(spec.min) == "number" and type(spec.max) == "number" and spec.min < spec.max,
        "invalid option range for " .. profile.id)
      number_value(spec.default, spec)
      keys[spec.key] = true
    end
    self.by_id[profile.id] = profile
    self.list[#self.list + 1] = profile
  end
  check(self.by_id.default ~= nil and self.by_id.default.dimensions == "all", "explicit default profile is required")
  table.sort(self.list, function(a, b)
    return a.order < b.order or (a.order == b.order and a.id < b.id)
  end)
  return self
end

function Profiles:supports(profile, dimension_id)
  local info = self.dimensions[dimension_id]
  if info == nil or (profile.terrain ~= nil and not info.terrain_styles) then return false end
  if profile.dimensions == "all" then return true end
  if type(profile.dimensions) == "function" then return profile.dimensions(dimension_id, info) == true end
  for _, id in ipairs(profile.dimensions) do
    if id == dimension_id then return true end
  end
  return false
end

function Profiles:selection()
  local selection = { profiles = {}, values = {} }
  for _, dimension_id in ipairs(self.dimension_ids) do
    selection.profiles[dimension_id] = self.by_id.default
    for _, profile in ipairs(self.list) do
      if self:supports(profile, dimension_id) then
        for _, spec in ipairs(profile.options or {}) do
          local key = option_key(dimension_id, spec.key)
          check(selection.values[key] == nil, "duplicate option " .. key)
          selection.values[key] = spec.default
        end
      end
    end
  end
  return selection
end

function Profiles:profile(selection, dimension_id)
  check(selection ~= nil, "world selection has not been opened")
  local profile = selection.profiles[dimension_id]
  check(profile ~= nil and self:supports(profile, dimension_id), "invalid selection for dimension " .. tostring(dimension_id))
  return profile
end

function Profiles:cycle(selection, dimension_id)
  local current = self:profile(selection, dimension_id)
  local start
  for index, profile in ipairs(self.list) do
    if profile == current then start = index break end
  end
  for step = 1, #self.list do
    local profile = self.list[(start - 1 + step) % #self.list + 1]
    if self:supports(profile, dimension_id) then
      selection.profiles[dimension_id] = profile
      return
    end
  end
end

function Profiles:options(selection, dimension_id)
  local result = {}
  for _, spec in ipairs(self:profile(selection, dimension_id).options or {}) do
    result[spec.key] = number_value(selection.values[option_key(dimension_id, spec.key)], spec)
  end
  return result
end

function Profiles:sliders(selection)
  local result = {}
  for _, dimension_id in ipairs(self.dimension_ids) do
    for _, spec in ipairs(self:profile(selection, dimension_id).options or {}) do
      result[#result + 1] = {
        key = option_key(dimension_id, spec.key),
        label = self.dimensions[dimension_id].name .. ": " .. spec.label,
        min = spec.min,
        max = spec.max,
        integer = spec.integer,
      }
    end
  end
  return result
end

function Profiles:save(selection, saved)
  for key in pairs(saved) do
    check(key:sub(1, #PREFIX) ~= PREFIX, "creation options already contain " .. key)
  end
  saved[SCHEMA] = "3"
  for _, dimension_id in ipairs(self.dimension_ids) do
    local profile = self:profile(selection, dimension_id)
    saved[DIMENSION .. dimension_id] = profile.id
    if self.dimensions[dimension_id].terrain_styles then
      saved["terrain_style:" .. dimension_id] = profile.terrain or "beta"
    end
    for key, value in pairs(self:options(selection, dimension_id)) do
      saved[OPTION .. option_key(dimension_id, key)] = tostring(value)
    end
  end
end

function Profiles:load(saved)
  local selection = self:selection()
  local present = false
  for key in pairs(saved) do
    if key:sub(1, #PREFIX) == PREFIX then present = true end
  end
  if not present then return selection end
  check(saved[SCHEMA] == "3", "unsupported saved profile schema; expected 3")
  local accepted = { [SCHEMA] = true }
  for _, dimension_id in ipairs(self.dimension_ids) do
    local dimension_key = DIMENSION .. dimension_id
    local id = saved[dimension_key]
    local profile = self.by_id[id]
    check(profile ~= nil, "unknown saved profile " .. tostring(id) .. " for dimension " .. dimension_id)
    check(self:supports(profile, dimension_id), "profile " .. id .. " does not support dimension " .. dimension_id)
    selection.profiles[dimension_id] = profile
    accepted[dimension_key] = true
    if self.dimensions[dimension_id].terrain_styles then
      check(saved["terrain_style:" .. dimension_id] == (profile.terrain or "beta"),
        "terrain style does not match profile for dimension " .. dimension_id)
    end
    for _, spec in ipairs(profile.options or {}) do
      local key = option_key(dimension_id, spec.key)
      selection.values[key] = number_value(saved[OPTION .. key], spec)
      accepted[OPTION .. key] = true
    end
  end
  for key in pairs(saved) do
    check(key:sub(1, #PREFIX) ~= PREFIX or accepted[key], "unknown saved option " .. key)
  end
  return selection
end

return Profiles
