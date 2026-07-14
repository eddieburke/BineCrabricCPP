local M = {}

function M.load(path, defaults, options)
  options = options or {}
  local values = {}
  for k, v in pairs(defaults) do
    values[k] = v
  end
  local text = minecraft.storage.read(path)
  if text == nil or text == "" then
    return values, false
  end
  for line in text:gmatch("[^\r\n]+") do
    local raw_key, raw_value = line:match("^%s*([^#;][^:=]-)%s*[:=]%s*(.-)%s*$")
    if raw_key ~= nil then
      local key = minecraft.util.trim(raw_key)
      key = (options.aliases and options.aliases[key]) or key
      if defaults[key] ~= nil then
        local expected_type = type(defaults[key])
        if expected_type == "boolean" then
          values[key] = minecraft.util.parse_boolean(raw_value, values[key])
        elseif expected_type == "number" then
          values[key] = tonumber(raw_value) or values[key]
        else
          values[key] = raw_value ~= "" and raw_value or values[key]
        end
      end
    end
  end
  return values, true
end

function M.save(path, values, options)
  options = options or {}
  local keys = options.keys or {}
  if #keys == 0 then
    for key in pairs(values) do
      keys[#keys + 1] = key
    end
    table.sort(keys)
  end
  local separator = options.separator or "="
  local lines = {}
  for _, key in ipairs(keys) do
    local output_key = (options.names and options.names[key]) or key
    lines[#lines + 1] = output_key .. separator .. tostring(values[key])
  end
  return minecraft.storage.write(path, table.concat(lines, "\n") .. "\n")
end

return M
