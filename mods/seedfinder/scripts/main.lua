-- Seedfinder: rules, cooperative seed search, and biome preview — all Lua on generic APIs.

local SCREEN_ID = "seedfinder:main"

-- LWJGL Beta 1.7.3 keyboard scancodes.
local KEY_ESCAPE = 1
local KEY_ENTER = 28
local KEY_UP = 200
local KEY_DOWN = 208

local DEFAULTS = {
  seed_start = "0",
  seed_end = "10000",
  radius = "4",
  top_k = "12",
  min_score = "0",
  unique_min = "",
}

local SPAWN_PICKER_BIOMES = {
  "plains", "forest", "desert", "taiga", "swampland", "rainforest", "savanna",
  "shrubland", "seasonal_forest", "ice_desert", "tundra", "hell", "sky",
}

local S = {
  mode = "finder",
  world_name = "",
  seed_text = "",
  saved = {},
  spawn_biomes = {},
  all_biomes = {},
  spawn_index = 0,
  hits = {},
  seen = {},
  selected_seed = nil,
  checked = 0,
  searching = false,
  search_cur = 0,
  search_end = 0,
  search_cfg = nil,
  scroll = 0,
  preview_seed = nil,
  map = nil,
  map_tex = nil,
  rules = {},
  rules_backup = nil,
  spec_scroll = 0,
  spec_content_height = 0,
  spec_boxes = {},
  spec_footer = {},
  view_top = 0,
  view_bottom = 0,
  dry = false,
  hitboxes = {},
  mx = -1,
  my = -1,
  layout = nil,
}

for k, v in pairs(DEFAULTS) do
  S.saved[k] = v
end

S.spawn_biomes = SPAWN_PICKER_BIOMES
S.all_biomes = minecraft.registry.list("biome")

----------------------------------------------------------------------
-- theme
----------------------------------------------------------------------

local C_BACKDROP   = 0xFF101722
local C_PANEL      = 0xFF202B3A
local C_PANEL2     = 0xFF263449
local C_HEADER     = 0xFF182231
local C_BORDER     = 0xFF46566D
local C_BORDER_HI  = 0xFF7186A5
local C_ACCENT     = 0xFF53B1FD
local C_ACCENT_DK  = 0xFF2C5C8F
local C_TEXT       = 0xFFE7ECF3
local C_DIMTEXT    = 0xFF9AA6B6
local C_MUTE       = 0xFF7F8FA5
local C_GOOD       = 0xFF6FCF82
local C_DANGER     = 0xFFD46A6A
local C_BTN        = 0xFF34445A
local C_BTN_HOV    = 0xFF415570
local C_BTN_ON     = 0xFF2C5C8F
local C_BTN_ONH    = 0xFF3A74B0
local C_ROW        = 0xFF1A2431
local C_ROW_ALT    = 0xFF1E2A39
local C_ROW_SEL    = 0xFF234461
local C_TRACK      = 0xFF0E1219
local C_THUMB      = 0xFF3A4658
local C_BEVEL_HI   = 0x18FFFFFF
local C_BEVEL_LO   = 0x26000000
local C_HOVER      = 0x22FFFFFF

----------------------------------------------------------------------
-- generic helpers
----------------------------------------------------------------------

local function fmt_seed(n)
  n = n or 0
  if math.type(n) == "integer" then
    return tostring(n)
  end
  return string.format("%.0f", n)
end

local function tw(s)
  return minecraft.gui.text_width(s)
end

local in_rect = minecraft.util.in_rect

local function rect(x, y, w, h, color)
  minecraft.gui.fill_rect(x, y, w, h, color)
end

local function border(x, y, w, h, color)
  rect(x, y, w, 1, color)
  rect(x, y + h - 1, w, 1, color)
  rect(x, y, 1, h, color)
  rect(x + w - 1, y, 1, h, color)
end

local function panel(x, y, w, h, fill, brd)
  rect(x, y, w, h, fill)
  border(x, y, w, h, brd or C_BORDER)
end

local function text(x, y, s, color)
  minecraft.gui.draw_text(x, y, s, color)
end

local function draw_text_centered(x, y, w, s, color)
  minecraft.gui.draw_centered_text(x, y, w, s, color)
end

local function text_right(x_right, y, s, color)
  minecraft.gui.draw_text(x_right - tw(s), y, s, color)
end

local function elide(s, max_w)
  if tw(s) <= max_w then
    return s
  end
  while #s > 1 and tw(s .. "..") > max_w do
    s = s:sub(1, #s - 1)
  end
  return s .. ".."
end

local function pretty(name)
  return (tostring(name):gsub("_", " "))
end

local function contains(list, name)
  for _, v in ipairs(list) do
    if v == name then
      return true
    end
  end
  return false
end

local function toggle_value(node, name)
  for i, v in ipairs(node.values) do
    if v == name then
      table.remove(node.values, i)
      return
    end
  end
  node.values[#node.values + 1] = name
end

local function map_texture_from_grid(grid)
  if grid == nil then
    return nil
  end
  local tex = minecraft.render.create_texture(grid.side, grid.side, grid.values)
  if tex == nil or tex.id == nil then
    return nil
  end
  local spawn_px, spawn_pz = minecraft.world.marker_px(grid, grid.center_x or 0, grid.center_z or 0)
  return {id = tex.id, side = grid.side, spawn_px = spawn_px, spawn_pz = spawn_pz}
end

----------------------------------------------------------------------
-- Lua-owned rule model.
----------------------------------------------------------------------

local BLOCKS = {
  { 1, "Stone" }, { 2, "Grass" }, { 3, "Dirt" }, { 4, "Cobblestone" }, { 12, "Sand" },
  { 13, "Gravel" }, { 14, "Gold Ore" }, { 15, "Iron Ore" }, { 16, "Coal Ore" }, { 17, "Wood" },
  { 18, "Leaves" }, { 24, "Sandstone" }, { 49, "Obsidian" }, { 56, "Diamond Ore" }, { 73, "Redstone Ore" },
  { 21, "Lapis Ore" }, { 79, "Ice" }, { 78, "Snow" }, { 81, "Cactus" }, { 82, "Clay" },
  { 87, "Netherrack" }, { 88, "Soul Sand" }, { 89, "Glowstone" },
}

local function block_name(id)
  for _, e in ipairs(BLOCKS) do
    if e[1] == id then
      return e[2]
    end
  end
  return "block " .. tostring(id)
end

local function block_index(id)
  for i, e in ipairs(BLOCKS) do
    if e[1] == id then
      return i
    end
  end
  return 1
end

local function cycle_block(id, dir)
  local n = #BLOCKS
  local idx = block_index(id) + dir
  idx = ((idx - 1) % n + n) % n + 1
  return BLOCKS[idx][1]
end

local OBJ_METRICS = {
  "unique_biome_count",
  "dominant_biome_percent",
  "biome_chunk_count",
  "biome_coverage_percent",
  "max_contiguous_biome_radius",
  "biome_blob_compactness_percent",
}

local OBJ_LABELS = {
  unique_biome_count = "biome variety in scan area",
  dominant_biome_percent = "dominant biome coverage %",
  biome_chunk_count = "selected biome chunks in area",
  biome_coverage_percent = "selected biome coverage %",
  max_contiguous_biome_radius = "largest contiguous biome blob",
  biome_blob_compactness_percent = "biome blob compactness %",
}
local BIOME_METRICS = {
  biome_chunk_count = true,
  biome_coverage_percent = true,
  max_contiguous_biome_radius = true,
  biome_blob_compactness_percent = true,
}

local function metric_index(m)
  for i, v in ipairs(OBJ_METRICS) do
    if v == m then
      return i
    end
  end
  return 1
end

local function cycle_metric(m, dir)
  local n = #OBJ_METRICS
  local idx = metric_index(m) + dir
  idx = ((idx - 1) % n + n) % n + 1
  return OBJ_METRICS[idx]
end

local function metric_label(m)
  return OBJ_LABELS[m] or "biome variety in scan area"
end

local function needs_biome(m)
  return BIOME_METRICS[m] == true
end

local KIND_ORDER = { "block", "block_on_block", "biome", "objective" }

local function kind_index(k)
  for i, v in ipairs(KIND_ORDER) do
    if v == k then
      return i
    end
  end
  return 3
end

local function cycle_kind(k, dir)
  local n = #KIND_ORDER
  local idx = kind_index(k) + dir
  idx = ((idx - 1) % n + n) % n + 1
  return KIND_ORDER[idx]
end

local function kind_label(k)
  if k == "block" then
    return "Block"
  elseif k == "block_on_block" then
    return "Block-on-Block"
  elseif k == "objective" then
    return "Objective"
  end
  return "Biome"
end

local function make_default(kind)
  if kind == "block" then
    return { kind = "block", metric = "spawn_surface_block_id", op = "eq", direction = "", values = {},
      block_id = 12, block_below_id = -1, min_value = -1, max_value = -1, value = -1, weight = 1 }
  elseif kind == "block_on_block" then
    return { kind = "block_on_block", metric = "block_on_block", op = "on", direction = "", values = {},
      block_id = 12, block_below_id = 3, min_value = -1, max_value = -1, value = -1, weight = 1 }
  elseif kind == "objective" then
    return { kind = "objective", metric = "unique_biome_count", op = "", direction = "maximize", values = {},
      block_id = -1, block_below_id = -1, min_value = -1, max_value = -1, value = -1, weight = 1 }
  end
  return { kind = "biome", metric = "biome", op = "any_of", direction = "", values = {},
    block_id = -1, block_below_id = -1, min_value = -1, max_value = -1, value = -1, weight = 1 }
end

local function clone_rule(r)
  local v = {}
  for i, name in ipairs(r.values or {}) do
    v[i] = name
  end
  return { kind = r.kind, metric = r.metric, op = r.op, direction = r.direction, values = v,
    block_id = r.block_id, block_below_id = r.block_below_id, min_value = r.min_value,
    max_value = r.max_value, value = r.value, weight = r.weight }
end

local function clone_rules(list)
  local out = {}
  for i, r in ipairs(list or {}) do
    out[i] = clone_rule(r)
  end
  return out
end

local function stepper_text(v)
  if v == nil or v < 0 then
    return "any"
  end
  return tostring(math.floor(v))
end

----------------------------------------------------------------------
-- config / search
----------------------------------------------------------------------

local function field_num(name, default)
  local text = minecraft.screen.field_text(name)
  if text == nil or text == "" then
    return default
  end
  local v = tonumber(text)
  if v == nil then
    return default
  end
  return v
end

local function save_fields()
  for _, f in ipairs((S.layout and S.layout.fields) or {}) do
    S.saved[f.name] = minecraft.screen.field_text(f.name)
  end
end

local function spawn_label()
  if S.spawn_index <= 0 then
    return "Any"
  end
  return pretty(S.spawn_biomes[S.spawn_index] or "Any")
end

local function json_string_list(values)
  local parts = {}
  for i, value in ipairs(values or {}) do
    parts[i] = '"' .. tostring(value):gsub('\\', '\\\\'):gsub('"', '\\"') .. '"'
  end
  return "[" .. table.concat(parts, ",") .. "]"
end

local function encode_rule_node(node)
  if node.kind == "objective" then
    local out = '{"type":"objective","metric":"' .. tostring(node.metric or "") .. '","direction":"'
      .. tostring(node.direction ~= "" and node.direction or "maximize") .. '","weight":'
      .. tostring(node.weight or 1)
    if node.values ~= nil and #node.values > 0 then
      out = out .. ',"biomes":' .. json_string_list(node.values)
    end
    return out .. "}"
  end
  if node.kind == "block" then
    return string.format(
      '{"type":"block_constraint","metric":"%s","op":"%s","block_id":%d,"min":%s,"max":%s,"value":%s}',
      node.metric ~= "" and node.metric or "spawn_surface_block_id",
      node.op ~= "" and node.op or "eq",
      node.block_id or -1,
      tostring(node.min_value or -1),
      tostring(node.max_value or -1),
      tostring(node.value or -1))
  end
  if node.kind == "block_on_block" then
    return string.format(
      '{"type":"block_on_block","op":"%s","top_block_id":%d,"bottom_block_id":%d,"min":%s}',
      node.op ~= "" and node.op or "on",
      node.block_id or -1,
      node.block_below_id or -1,
      tostring(node.min_value or -1))
  end
  local out = '{"type":"biome_constraint","op":"' .. tostring(node.op ~= "" and node.op or "any_of") .. '","biomes":'
    .. json_string_list(node.values)
  if node.min_value ~= nil and node.min_value >= 0 then
    out = out .. ',"min_coverage_percent":' .. tostring(node.min_value)
  end
  if node.max_value ~= nil and node.max_value >= 0 then
    out = out .. ',"max_coverage_percent":' .. tostring(node.max_value)
  end
  if node.value ~= nil and node.value >= 1 then
    out = out .. ',"min_contiguous_radius_chunks":' .. tostring(node.value)
  end
  return out .. "}"
end

local function encode_rules_blob(rules)
  local rule_parts = {}
  local objective_parts = {}
  for _, node in ipairs(rules or {}) do
    if node.kind == "objective" then
      objective_parts[#objective_parts + 1] = encode_rule_node(node)
    else
      rule_parts[#rule_parts + 1] = encode_rule_node(node)
    end
  end
  return '{"rules":[' .. table.concat(rule_parts, ",") .. '],"objectives":[' .. table.concat(objective_parts, ",") .. "]}"
end

local function decode_rules_table(parsed)
  local rules = {}
  local function push_node(raw, kind)
    local values = {}
    for i, name in ipairs(raw.biomes or raw.values or {}) do
      values[i] = name
    end
    rules[#rules + 1] = {
      kind = kind,
      metric = raw.metric or (kind == "biome" and "biome" or ""),
      op = raw.op or "",
      direction = raw.direction or "",
      values = values,
      block_id = raw.block_id or raw.top_block_id or -1,
      block_below_id = raw.block_below_id or raw.bottom_block_id or -1,
      min_value = raw.min_coverage_percent or raw.min or raw.min_value or -1,
      max_value = raw.max_coverage_percent or raw.max or raw.max_value or -1,
      value = raw.min_contiguous_radius_chunks or raw.value or -1,
      weight = raw.weight or 1,
    }
  end
  for _, raw in ipairs(parsed.rules or {}) do
    if raw.kind == "objective" then
      push_node(raw, "objective")
    elseif raw.kind == "block" or raw.type == "block_constraint" then
      push_node(raw, "block")
    elseif raw.kind == "block_on_block" or raw.type == "block_on_block" then
      push_node(raw, "block_on_block")
    else
      push_node(raw, "biome")
    end
  end
  for _, raw in ipairs(parsed.objectives or {}) do
    push_node(raw, "objective")
  end
  return rules
end

local function decode_rules_blob(blob)
  if blob == nil or blob == "" then
    return {}
  end
  local parsed, err = minecraft.util.json_decode(blob)
  if parsed == nil then
    minecraft.log("warn", "seedfinder: rule decode failed: " .. tostring(err))
    return {}
  end
  return decode_rules_table(parsed)
end

local function cfg_number(cfg, ...)
  for i = 1, select("#", ...) do
    local key = select(i, ...)
    local value = cfg[key]
    if value ~= nil then
      return tonumber(value)
    end
  end
  return nil
end

local function apply_import_json(json)
  local cfg, err = minecraft.util.json_decode(json)
  if cfg == nil then
    minecraft.log("error", "seedfinder: " .. tostring(err))
    return
  end
  minecraft.screen.set_field_text("seed_start", fmt_seed(cfg_number(cfg, "seed_start", "seed_range_start") or 0))
  minecraft.screen.set_field_text("seed_end", fmt_seed(cfg_number(cfg, "seed_end", "seed_range_end") or 100000))
  local radius = cfg_number(cfg, "scan_radius_chunks")
  if radius ~= nil then
    minecraft.screen.set_field_text("radius", tostring(radius))
  end
  local top_k = cfg_number(cfg, "top_k")
  if top_k ~= nil then
    minecraft.screen.set_field_text("top_k", tostring(top_k))
  end
  local min_score = cfg_number(cfg, "min_score")
  if min_score ~= nil then
    minecraft.screen.set_field_text("min_score", tostring(min_score))
  end
  local unique = cfg_number(cfg, "unique_biome_min")
  if unique ~= nil then
    minecraft.screen.set_field_text("unique_min", tostring(unique))
  end
  local spawn = cfg.spawn_biome
  if spawn ~= nil and spawn ~= "" then
    for i, name in ipairs(S.spawn_biomes) do
      if name == spawn then
        S.spawn_index = i
      end
    end
  end
  if cfg.advanced_rule_data_nbt ~= nil and cfg.advanced_rule_data_nbt ~= "" then
    S.rules = clone_rules(decode_rules_blob(cfg.advanced_rule_data_nbt))
  elseif cfg.rules ~= nil or cfg.objectives ~= nil then
    S.rules = clone_rules(decode_rules_table({
      rules = cfg.rules or {},
      objectives = cfg.objectives or {},
    }))
  end
end

----------------------------------------------------------------------
-- Lua search (minecraft.world.sample + rules)
----------------------------------------------------------------------

local function biome_name(id)
  return minecraft.registry.name("biome", id or 0)
end

local function biome_listed(name, list)
  for _, entry in ipairs(list or {}) do
    if entry == name then
      return true
    end
  end
  return false
end

local function grid_biome_stats(grid)
  local counts = {}
  local total = grid.side * grid.side
  for i = 1, total do
    local id = grid.values[i] or 0
    counts[id] = (counts[id] or 0) + 1
  end
  local unique = 0
  for _ in pairs(counts) do
    unique = unique + 1
  end
  local dominant_id, dominant_n = 0, 0
  for id, n in pairs(counts) do
    if n > dominant_n then
      dominant_id, dominant_n = id, n
    end
  end
  local center_cell = math.floor(grid.side / 2)
  local center = grid.values[center_cell * grid.side + center_cell + 1] or 0
  return {
    counts = counts,
    total = total,
    unique = unique,
    dominant_id = dominant_id,
    dominant_pct = total > 0 and (100.0 * dominant_n / total) or 0,
    spawn_biome_id = center,
  }
end

local function coverage_for_biomes(stats, names)
  local matched = 0
  for id, n in pairs(stats.counts) do
    if biome_listed(biome_name(id), names) then
      matched = matched + n
    end
  end
  return stats.total > 0 and (100.0 * matched / stats.total) or 0
end

local function biome_area_stats(grid, names)
  local side = grid.side or 0
  local total = side * side
  local match = {}
  local matching = 0
  for i = 1, total do
    local yes = biome_listed(biome_name(grid.values[i] or 0), names)
    match[i] = yes
    if yes then
      matching = matching + 1
    end
  end
  if matching == 0 then
    return 0, 0
  end
  local visited = {}
  local best_radius_cells = 0
  local best_size = 0
  local dx = { 1, -1, 0, 0 }
  local dz = { 0, 0, 1, -1 }
  for start = 1, total do
    if match[start] and not visited[start] then
      local queue = { start }
      local head = 1
      local cells = {}
      local sum_x, sum_z = 0, 0
      visited[start] = true
      while head <= #queue do
        local idx = queue[head]
        head = head + 1
        cells[#cells + 1] = idx
        local zero = idx - 1
        local x = zero % side
        local z = math.floor(zero / side)
        sum_x = sum_x + x
        sum_z = sum_z + z
        for d = 1, 4 do
          local nx, nz = x + dx[d], z + dz[d]
          if nx >= 0 and nz >= 0 and nx < side and nz < side then
            local ni = nz * side + nx + 1
            if match[ni] and not visited[ni] then
              visited[ni] = true
              queue[#queue + 1] = ni
            end
          end
        end
      end
      local cx = math.floor(sum_x / #cells)
      local cz = math.floor(sum_z / #cells)
      local radius_cells = 0
      for _, idx in ipairs(cells) do
        local zero = idx - 1
        local x = zero % side
        local z = math.floor(zero / side)
        radius_cells = math.max(radius_cells, math.max(math.abs(x - cx), math.abs(z - cz)))
      end
      if radius_cells > best_radius_cells or (radius_cells == best_radius_cells and #cells > best_size) then
        best_radius_cells = radius_cells
        best_size = #cells
      end
    end
  end
  local radius_chunks = best_radius_cells * math.max(1, grid.step or 1) / 16
  return radius_chunks, best_size * 100.0 / matching
end

local function eval_biome_rule(rule, stats, grid)
  if #(rule.values or {}) == 0 then
    return false
  end
  local op = rule.op ~= "" and rule.op or "any_of"
  local cov = coverage_for_biomes(stats, rule.values)
  local matches
  if op == "none_of" then
    matches = cov <= 0.001
  elseif op == "all_of" then
    matches = true
    for _, want in ipairs(rule.values or {}) do
      local found = false
      for id, n in pairs(stats.counts) do
        if n > 0 and biome_name(id) == want then
          found = true
          break
        end
      end
      if not found then
        matches = false
        break
      end
    end
  else
    matches = cov > 0
  end
  if not matches then
    return false
  end
  if rule.min_value ~= nil and rule.min_value >= 0 then
    if cov < rule.min_value then
      return false
    end
  end
  if rule.max_value ~= nil and rule.max_value >= 0 and cov > rule.max_value then
    return false
  end
  if rule.value ~= nil and rule.value >= 1 then
    local radius = biome_area_stats(grid, rule.values)
    if radius < rule.value then
      return false
    end
  end
  return true
end

local function objective_value(rule, stats, grid)
  local metric = rule.metric or ""
  if metric == "unique_biome_count" then
    return stats.unique
  end
  if metric == "dominant_biome_percent" then
    return stats.dominant_pct
  end
  if metric == "biome_chunk_count" then
    local sample_area = math.max(1, grid.step or 1) ^ 2
    return coverage_for_biomes(stats, rule.values) * stats.total * sample_area / 25600.0
  end
  if metric == "biome" or metric == "biome_coverage_pct" or metric == "biome_coverage_percent" then
    return coverage_for_biomes(stats, rule.values)
  end
  if metric == "max_contiguous_biome_radius" then
    return biome_area_stats(grid, rule.values)
  end
  if metric == "biome_blob_compactness_percent" then
    local _, compactness = biome_area_stats(grid, rule.values)
    return compactness
  end
  return stats.unique
end

local function center_grid_value(grid, channel)
  local values = grid.channels and grid.channels[channel]
  if values == nil then
    return 0
  end
  local c = math.floor(grid.side / 2)
  return values[c * grid.side + c + 1] or 0
end

local function count_surface_pair(grid, top_id, below_id)
  local top = grid.channels and grid.channels.surface_block or {}
  local below = grid.channels and grid.channels.surface_block_below or {}
  local count = 0
  for i = 1, grid.side * grid.side do
    if top[i] == top_id and below[i] == below_id then
      count = count + 1
    end
  end
  return count
end

local function eval_block_rule(rule, grid)
  local top = grid.channels and grid.channels.surface_block or {}
  if rule.kind == "block_on_block" then
    local count = count_surface_pair(grid, rule.block_id, rule.block_below_id)
    return count >= math.max(1, rule.min_value or -1)
  end
  if rule.metric == "block_histogram" then
    local count = 0
    for i = 1, #top do
      if top[i] == rule.block_id then
        count = count + 1
      end
    end
    if rule.min_value ~= nil and rule.min_value >= 0 and count < rule.min_value then
      return false
    end
    if rule.max_value ~= nil and rule.max_value >= 0 and count > rule.max_value then
      return false
    end
    return count > 0 or (rule.min_value ~= nil and rule.min_value == 0)
  end
  local actual = center_grid_value(grid, "surface_block")
  if rule.op == "ne" then
    return actual ~= rule.block_id
  end
  return actual == rule.block_id
end

local function terrain_stats(grid)
  local heights = grid.channels and grid.channels.height or {}
  local surface = grid.channels and grid.channels.surface_block or {}
  if #heights == 0 then
    return 64, 64, 64, 0
  end
  local sum, min_y, max_y, underwater = 0, heights[1], heights[1], 0
  for i, value in ipairs(heights) do
    sum = sum + value
    min_y = math.min(min_y, value)
    max_y = math.max(max_y, value)
    if surface[i] == 8 or surface[i] == 9 then
      underwater = underwater + 1
    end
  end
  return sum / #heights, min_y, max_y, underwater * 100.0 / #heights
end

local function score_seed(seed, cfg)
  local radius = cfg.radius
  local side = math.min(48, math.max(16, radius * 8))
  local channels = { "biome_id" }
  if cfg.terrain then
    channels[#channels + 1] = "height"
    channels[#channels + 1] = "surface_block"
    channels[#channels + 1] = "surface_block_below"
  end
  local ok, grid = pcall(minecraft.world.sample, seed, 0, 0, {
    radius_chunks = radius,
    max_side = side,
    channels = channels,
    mod_generation = true,
  })
  if not ok or grid == nil then
    return nil
  end
  local stats = grid_biome_stats(grid)
  if cfg.spawn_index > 0 then
    local want = S.spawn_biomes[cfg.spawn_index]
    if biome_name(stats.spawn_biome_id) ~= want then
      return nil
    end
  end
  for _, rule in ipairs(S.rules) do
    if rule.kind == "biome" and not eval_biome_rule(rule, stats, grid) then
      return nil
    end
    if (rule.kind == "block" or rule.kind == "block_on_block") and not eval_block_rule(rule, grid) then
      return nil
    end
  end
  if cfg.unique_min >= 0 and stats.unique < cfg.unique_min then
    return nil
  end
  local score = 0
  local objectives = 0
  for _, rule in ipairs(S.rules) do
    if rule.kind == "objective" then
      local value = objective_value(rule, stats, grid)
      local weight = rule.weight or 1
      if rule.direction == "minimize" then
        score = score - value * weight
      else
        score = score + value * weight
      end
      objectives = objectives + 1
    end
  end
  if objectives == 0 then
    score = stats.unique + stats.dominant_pct * 0.1
  end
  if score < cfg.min_score then
    return nil
  end
  local avg_y, min_y, max_y, underwater = terrain_stats(grid)
  return {
    seed = seed,
    score = score,
    all_hard = true,
    spawn_x = 0,
    spawn_y = cfg.terrain and center_grid_value(grid, "height") or 64,
    spawn_z = 0,
    spawn_valid = true,
    spawn_biome_id = stats.spawn_biome_id,
    unique_biome_count = stats.unique,
    dominant_biome_id = stats.dominant_id,
    dominant_biome_percent = math.floor(stats.dominant_pct),
    spawn_surface_block_id = cfg.terrain and center_grid_value(grid, "surface_block") or 0,
    avg_surface_y = avg_y,
    min_surface_y = min_y,
    max_surface_y = max_y,
    underwater_percent = underwater,
  }
end

local function selected_index()
  if S.selected_seed == nil then
    return nil
  end
  for i, h in ipairs(S.hits) do
    if fmt_seed(h.seed) == S.selected_seed then
      return i
    end
  end
  return nil
end

local function free_map()
  if S.map_tex ~= nil and S.map_tex.id ~= nil then
    minecraft.render.release_texture(S.map_tex.id)
  end
  S.map_tex = nil
  S.map = nil
end

local function build_map(hit)
  free_map()
  if hit == nil then
    return
  end
  local radius = math.floor(minecraft.util.clamp(field_num("radius", 4), 1, 64))
  local cx = hit.spawn_x or 0
  local cz = hit.spawn_z or 0
  S.preview_seed = hit.seed
  local grid = minecraft.world.sample(hit.seed, cx, cz, {
    radius_chunks = radius,
    max_side = 256,
    mod_generation = true,
  })
  S.map_tex = map_texture_from_grid(grid)
  local have_tex = S.map_tex ~= nil and S.map_tex.id ~= nil and S.map_tex.id > 0
  if not have_tex then
    S.map = minecraft.world.sample(hit.seed, cx, cz, {
      radius_chunks = radius,
      max_side = 48,
      mod_generation = true,
    })
  end
end

local function select_index(i)
  local h = S.hits[i]
  if h == nil then
    return
  end
  S.selected_seed = fmt_seed(h.seed)
  build_map(h)
end

local function merge_hits(list)
  if list == nil then
    return
  end
  local top_k = math.floor(minecraft.util.clamp(
    (S.search_cfg and S.search_cfg.top_k) or field_num("top_k", 12), 1, 100))
  for _, h in ipairs(list) do
    local key = fmt_seed(h.seed)
    if not S.seen[key] then
      S.seen[key] = true
      S.hits[#S.hits + 1] = h
    end
  end
  table.sort(S.hits, function(a, b)
    return a.score > b.score
  end)
  while #S.hits > top_k do
    table.remove(S.hits)
  end
  if S.selected_seed == nil and #S.hits > 0 then
    select_index(1)
  end
end

local function move_selection(delta)
  if #S.hits == 0 then
    return
  end
  local i = selected_index() or 0
  i = i + delta
  if i < 1 then
    i = 1
  end
  if i > #S.hits then
    i = #S.hits
  end
  select_index(i)
  if S.layout ~= nil then
    if i - 1 < S.scroll then
      S.scroll = i - 1
    end
    if i - 1 >= S.scroll + S.layout.rows then
      S.scroll = i - S.layout.rows
    end
  end
end

local function preview_seed()
  save_fields()
  local seed_text = minecraft.screen.field_text("seed_start") or "0"
  local seed = minecraft.util.resolve_seed(seed_text)
  build_map({ seed = seed, spawn_x = 0, spawn_z = 0 })
end

local function search_tick()
  if not S.searching or S.search_cfg == nil then
    return
  end
  local cfg = S.search_cfg
  local budget = cfg.terrain and 1 or math.max(2, math.floor(64 / math.max(1, cfg.radius)))
  for _ = 1, budget do
    if S.search_cur > S.search_end then
      S.searching = false
      S.search_cfg = nil
      return
    end
    local seed = S.search_cur
    local final_seed = seed >= S.search_end
    if not final_seed then
      S.search_cur = seed + 1
    end
    S.checked = S.checked + 1
    local hit = score_seed(seed, cfg)
    if hit ~= nil then
      merge_hits({ hit })
    end
    if final_seed then
      S.searching = false
      S.search_cfg = nil
      return
    end
  end
end

local function toggle_search()
  if S.searching then
    S.searching = false
    S.search_cfg = nil
    return
  end
  save_fields()
  S.hits = {}
  S.seen = {}
  S.selected_seed = nil
  S.checked = 0
  S.scroll = 0
  free_map()
  S.search_cur = minecraft.util.resolve_seed(minecraft.screen.field_text("seed_start") or "0")
  S.search_end = minecraft.util.resolve_seed(minecraft.screen.field_text("seed_end") or "10000")
  if S.search_cur > S.search_end then
    S.search_cur, S.search_end = S.search_end, S.search_cur
  end
  local needs_terrain = false
  for _, rule in ipairs(S.rules) do
    if rule.kind == "block" or rule.kind == "block_on_block" then
      needs_terrain = true
      break
    end
  end
  S.search_cfg = {
    radius = math.floor(minecraft.util.clamp(field_num("radius", 4), 1, 64)),
    min_score = field_num("min_score", 0),
    unique_min = math.floor(minecraft.util.clamp(field_num("unique_min", -1), -1, 64)),
    top_k = math.floor(minecraft.util.clamp(field_num("top_k", 12), 1, 100)),
    spawn_index = S.spawn_index,
    terrain = needs_terrain,
  }
  S.searching = true
end

local function do_import()
  local path = minecraft.files.pick({extension = "json"})
  if path == nil then
    return
  end
  local text, err = minecraft.files.read(path)
  if text == nil then
    minecraft.log("error", "seedfinder: " .. tostring(err))
    return
  end
  apply_import_json(text)
  minecraft.log("info", "seedfinder: imported " .. tostring(path))
end

local function use_selected()
  local i = selected_index()
  if i == nil then
    return
  end
  local h = S.hits[i]
  if h == nil then
    return
  end
  S.searching = false
  free_map()
  minecraft.screen.open_host(minecraft.screen.ids.create_world, {
    world_name = S.world_name,
    seed = fmt_seed(h.seed),
  })
end

local function go_back()
  S.searching = false
  free_map()
  minecraft.screen.open_host(minecraft.screen.ids.create_world, {
    world_name = S.world_name,
    seed = S.seed_text or "",
  })
end

local function open_finder()
  S.mode = "finder"
  S.hits = {}
  S.seen = {}
  S.selected_seed = nil
  S.checked = 0
  S.searching = false
  S.scroll = 0
  S.preview_seed = nil
  free_map()
  minecraft.screen.open(SCREEN_ID)
end

----------------------------------------------------------------------
-- spec editor (full Lua port of SeedSpecEditor)
----------------------------------------------------------------------

local MARGIN = 20
local TITLE_Y = 13
local CONTENT_TOP = 34
local FOOTER_H = 32
local CARD_PAD = 8
local HEADER_H = 22
local ROW_H = 20
local ROW_GAP = 3
local ROWADV = ROW_H + ROW_GAP
local CARD_GAP = 8
local ARROW_W = 14
local ICON_W = 16
local CHIP_H = 16

local SPEC_BG = 0xFF0C0F16
local CARD_BG = C_PANEL
local HEADER_BG = C_PANEL2
local BTN_BG = C_BTN
local VALUE_BG = 0xFF10151E
local CHIP_ON = C_ACCENT_DK
local CHIP_OFF = 0xFF2A3340
local DELETE_BG = 0xFF7E3030
local ADD_BG = C_ACCENT_DK
local DONE_BG = 0xFF2E7D4F
local LABEL_FG = C_DIMTEXT
local VALUE_FG = C_TEXT
local MUTED_FG = C_MUTE
local HOVER = C_HOVER

local function spec_visible(y, h)
  return (y + h > S.view_top) and (y < S.view_bottom)
end

local function spec_body_btn(x, y, w, h, label, bg, fg, center, fn)
  if not spec_visible(y, h) then
    return
  end
  if bg ~= nil and bg ~= 0 then
    rect(x, y, w, h, bg)
    if fn ~= nil and in_rect(S.mx, S.my, x, y, w, h) then
      rect(x, y, w, h, HOVER)
    end
  end
  if label ~= nil and label ~= "" then
    local ty = y + math.floor((h - 8) / 2)
    if center then
      draw_text_centered(x, ty, w, label, fg)
    else
      text(x, y, label, fg)
    end
  end
  if fn ~= nil then
    S.spec_boxes[#S.spec_boxes + 1] = { x = x, y = y, w = w, h = h, fn = fn }
  end
end

local function spec_body_label(x, y, label, fg)
  if (y + 10 > S.view_top) and (y < S.view_bottom) then
    text(x, y, label, fg)
  end
end

local function spec_rect_culled(x, y, w, h, color)
  if spec_visible(y, h) then
    rect(x, y, w, h, color)
  end
end

local function spec_border_culled(x, y, w, h, color)
  if spec_visible(y, h) then
    border(x, y, w, h, color)
  end
end

local function L_label(x, y, label, fg)
  if not S.dry then
    spec_body_label(x, y, label, fg)
  end
end

local function L_btn(x, y, w, h, label, bg, fg, center, fn)
  if not S.dry then
    spec_body_btn(x, y, w, h, label, bg, fg, center, fn)
  end
end

local function L_cycle(x, y, label, fg, on_left, on_right)
  local value_w = math.max(58, tw(label) + 12)
  L_btn(x, y, ARROW_W, ROW_H - 2, "<", BTN_BG, VALUE_FG, true, on_left)
  L_btn(x + ARROW_W + 1, y, value_w, ROW_H - 2, label, VALUE_BG, fg, true, nil)
  L_btn(x + ARROW_W + 1 + value_w + 1, y, ARROW_W, ROW_H - 2, ">", BTN_BG, VALUE_FG, true, on_right)
  return ARROW_W + 1 + value_w + 1 + ARROW_W
end

local function L_stepper(x, y, label, on_minus, on_plus)
  local value_w = math.max(50, tw(label) + 12)
  L_btn(x, y, ARROW_W, ROW_H - 2, "-", BTN_BG, VALUE_FG, true, on_minus)
  L_btn(x + ARROW_W + 1, y, value_w, ROW_H - 2, label, VALUE_BG, VALUE_FG, true, nil)
  L_btn(x + ARROW_W + 1 + value_w + 1, y, ARROW_W, ROW_H - 2, "+", BTN_BG, VALUE_FG, true, on_plus)
end

local function chip_section(node, inner_left, inner_right, by, single)
  local chip_x = inner_left
  for _, name in ipairs(S.all_biomes) do
    local label = pretty(name)
    local w = tw(label) + 12
    if chip_x + w > inner_right then
      chip_x = inner_left
      by = by + CHIP_H + 4
    end
    local on
    if single then
      on = (#node.values == 1 and node.values[1] == name)
    else
      on = contains(node.values, name)
    end
    L_btn(chip_x, by, w, CHIP_H, label, on and CHIP_ON or CHIP_OFF, VALUE_FG, true, function()
      if single then
        node.values = { name }
      else
        toggle_value(node, name)
      end
    end)
    chip_x = chip_x + w + 4
  end
  return by + CHIP_H + 4
end

-- Lays out (and optionally draws) one rule card body. Returns the y just past it.
local function body_layout(node, inner_left, inner_right, value_col, by)
  if node.kind == "biome" then
    L_label(inner_left, by + 5, "Match", LABEL_FG)
    L_cycle(value_col, by, node.op == "none_of" and "none of" or "any of", VALUE_FG,
      function() node.op = (node.op == "none_of") and "any_of" or "none_of" end,
      function() node.op = (node.op == "none_of") and "any_of" or "none_of" end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Min cover %", LABEL_FG)
    L_stepper(value_col, by, stepper_text(node.min_value),
      function() node.min_value = (node.min_value <= 0) and -1 or (node.min_value - 5) end,
      function()
        local v = (node.min_value < 0) and 0 or node.min_value
        v = v + 5
        if v > 100 then v = 100 end
        node.min_value = v
      end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Min radius", LABEL_FG)
    L_stepper(value_col, by, stepper_text(node.value),
      function() node.value = (node.value <= 0) and -1 or (node.value - 1) end,
      function() node.value = ((node.value < 0) and 0 or node.value) + 1 end)
    L_label(value_col + 120, by + 5, "chunks (contiguous blob)", MUTED_FG)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Max cover %", LABEL_FG)
    L_stepper(value_col, by, stepper_text(node.max_value),
      function() node.max_value = (node.max_value <= 0) and -1 or (node.max_value - 5) end,
      function()
        local v = (node.max_value < 0) and 0 or node.max_value
        v = v + 5
        if v > 100 then v = 100 end
        node.max_value = v
      end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Biomes", LABEL_FG)
    by = by + CARD_PAD + 6
    by = chip_section(node, inner_left, inner_right, by, false)
    return by
  elseif node.kind == "block" then
    local histogram = node.metric == "block_histogram"
    L_label(inner_left, by + 5, "Check", LABEL_FG)
    L_cycle(value_col, by, histogram and "block count in area" or "spawn surface block", VALUE_FG,
      function() node.metric = (node.metric == "block_histogram") and "spawn_surface_block_id" or "block_histogram" end,
      function() node.metric = (node.metric == "block_histogram") and "spawn_surface_block_id" or "block_histogram" end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Block", LABEL_FG)
    L_cycle(value_col, by, block_name(node.block_id), VALUE_FG,
      function() node.block_id = cycle_block(node.block_id, -1) end,
      function() node.block_id = cycle_block(node.block_id, 1) end)
    by = by + ROWADV
    if histogram then
      L_label(inner_left, by + 5, "Min count", LABEL_FG)
      L_stepper(value_col, by, stepper_text(node.min_value),
        function() node.min_value = (node.min_value <= 0) and -1 or (node.min_value - 1) end,
        function() node.min_value = ((node.min_value < 0) and 0 or node.min_value) + 1 end)
      by = by + ROWADV
      L_label(inner_left, by + 5, "Max count", LABEL_FG)
      L_stepper(value_col, by, stepper_text(node.max_value),
        function() node.max_value = (node.max_value <= 0) and -1 or (node.max_value - 1) end,
        function() node.max_value = ((node.max_value < 0) and 0 or node.max_value) + 1 end)
      by = by + ROWADV
    else
      L_label(inner_left, by + 5, "Must be", LABEL_FG)
      L_cycle(value_col, by, node.op == "ne" and "not this block" or "this block", VALUE_FG,
        function() node.op = (node.op == "ne") and "eq" or "ne" end,
        function() node.op = (node.op == "ne") and "eq" or "ne" end)
      by = by + ROWADV
    end
    return by
  elseif node.kind == "block_on_block" then
    L_label(inner_left, by + 5, "Top block", LABEL_FG)
    L_cycle(value_col, by, block_name(node.block_id), VALUE_FG,
      function() node.block_id = cycle_block(node.block_id, -1) end,
      function() node.block_id = cycle_block(node.block_id, 1) end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "On top of", LABEL_FG)
    L_cycle(value_col, by, block_name(node.block_below_id), VALUE_FG,
      function() node.block_below_id = cycle_block(node.block_below_id, -1) end,
      function() node.block_below_id = cycle_block(node.block_below_id, 1) end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Min count", LABEL_FG)
    L_stepper(value_col, by, stepper_text(node.min_value),
      function() node.min_value = (node.min_value <= 0) and -1 or (node.min_value - 1) end,
      function() node.min_value = ((node.min_value < 0) and 0 or node.min_value) + 1 end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Counts sampled surface columns", MUTED_FG)
    by = by + ROWADV
    return by
  else
    L_label(inner_left, by + 5, "Prefer", LABEL_FG)
    L_cycle(value_col, by, node.direction == "minimize" and "less" or "more", VALUE_FG,
      function() node.direction = (node.direction == "minimize") and "maximize" or "minimize" end,
      function() node.direction = (node.direction == "minimize") and "maximize" or "minimize" end)
    by = by + ROWADV
    L_label(inner_left, by + 5, "Measure", LABEL_FG)
    L_cycle(value_col, by, metric_label(node.metric), VALUE_FG,
      function()
        node.metric = cycle_metric(node.metric, -1)
        if not needs_biome(node.metric) then
          node.values = {}
        elseif #node.values == 0 then
          node.values = { "desert" }
        end
      end,
      function()
        node.metric = cycle_metric(node.metric, 1)
        if not needs_biome(node.metric) then
          node.values = {}
        elseif #node.values == 0 then
          node.values = { "desert" }
        end
      end)
    by = by + ROWADV
    if needs_biome(node.metric) then
      L_label(inner_left, by + 5, "Target biome", LABEL_FG)
      by = by + CARD_PAD + 6
      by = chip_section(node, inner_left, inner_right, by, true)
    end
    L_label(inner_left, by + 5, "Weight", LABEL_FG)
    local weight_text = tostring(math.floor((node.weight and node.weight > 0) and node.weight or 1))
    L_stepper(value_col, by, weight_text,
      function() node.weight = math.max(1, (node.weight or 1) - 1) end,
      function() node.weight = math.min(20, ((node.weight and node.weight > 0) and node.weight or 1) + 1) end)
    by = by + ROWADV
    return by
  end
end

local function enter_spec()
  S.rules_backup = clone_rules(S.rules)
  S.mode = "spec"
  S.spec_scroll = 0
  minecraft.screen.set_fields_visible(false)
end

local function exit_spec(keep)
  if not keep then
    S.rules = clone_rules(S.rules_backup or {})
  end
  S.rules_backup = nil
  S.mode = "finder"
  minecraft.screen.set_fields_visible(true)
end

local function footer_add(x, y, w, h, label, bg, fg, fn)
  S.spec_footer[#S.spec_footer + 1] = { x = x, y = y, w = w, h = h, text = label, bg = bg, fg = fg, fn = fn }
end

local function spec_render(event)
  local W, H = event.width, event.height
  S.mx = event.mouse_x
  S.my = event.mouse_y
  S.spec_boxes = {}
  S.spec_footer = {}
  S.view_top = CONTENT_TOP
  S.view_bottom = H - FOOTER_H

  local viewport = math.max(0, (H - FOOTER_H) - CONTENT_TOP)
  local max_scroll = math.max(0, (S.spec_content_height or 0) - viewport)
  if S.spec_scroll > max_scroll then
    S.spec_scroll = max_scroll
  end
  if S.spec_scroll < 0 then
    S.spec_scroll = 0
  end

  rect(0, 0, W, H, SPEC_BG)

  local left = MARGIN
  local card_w = W - 2 * MARGIN
  local inner_left = left + CARD_PAD
  local inner_right = left + card_w - CARD_PAD
  local value_col = inner_left + 96
  local top0 = CONTENT_TOP - S.spec_scroll
  local cy = top0

  for i, node in ipairs(S.rules) do
    local card_top = cy
    local body_start = card_top + HEADER_H + CARD_PAD
    S.dry = true
    local body_end = body_layout(node, inner_left, inner_right, value_col, body_start)
    S.dry = false
    local body_h = body_end - body_start
    local card_h = HEADER_H + body_h + CARD_PAD

    spec_rect_culled(left, card_top, card_w, card_h, CARD_BG)
    spec_rect_culled(left, card_top, card_w, HEADER_H, HEADER_BG)
    spec_rect_culled(left, card_top, 3, card_h, C_ACCENT_DK)
    spec_border_culled(left, card_top, card_w, card_h, C_BORDER)

    local hx = inner_left
    local hy = card_top + 3
    if i > 1 then
      L_btn(hx, hy, ICON_W, ICON_W, "^", BTN_BG, VALUE_FG, true, function()
        S.rules[i], S.rules[i - 1] = S.rules[i - 1], S.rules[i]
      end)
    end
    hx = hx + ICON_W + 2
    if i < #S.rules then
      L_btn(hx, hy, ICON_W, ICON_W, "v", BTN_BG, VALUE_FG, true, function()
        S.rules[i], S.rules[i + 1] = S.rules[i + 1], S.rules[i]
      end)
    end
    hx = hx + ICON_W + 6
    spec_body_label(hx, card_top + 7, "RULE " .. tostring(i), C_ACCENT)

    spec_body_btn(inner_right - ICON_W, hy, ICON_W, ICON_W, "X", DELETE_BG, VALUE_FG, true, function()
      table.remove(S.rules, i)
    end)
    local kind_text = kind_label(node.kind)
    local cycle_w = ARROW_W + 1 + math.max(58, tw(kind_text) + 12) + 1 + ARROW_W
    local cxk = inner_right - ICON_W - 6 - cycle_w
    L_cycle(cxk, card_top + 3, kind_text, VALUE_FG,
      function() S.rules[i] = make_default(cycle_kind(node.kind, -1)) end,
      function() S.rules[i] = make_default(cycle_kind(node.kind, 1)) end)

    body_layout(node, inner_left, inner_right, value_col, body_start)
    cy = card_top + card_h + CARD_GAP
  end

  spec_body_btn(left, cy, card_w, ROW_H + 4, "+ Add Rule", ADD_BG, VALUE_FG, true, function()
    S.rules[#S.rules + 1] = make_default("biome")
  end)
  cy = cy + ROW_H + 4
  S.spec_content_height = cy - top0

  local footer_y = H - FOOTER_H + 6
  local btn_w = 110
  local gap = 10
  local total_w = btn_w * 2 + gap
  local fx = math.floor((W - total_w) / 2)
  footer_add(fx, footer_y, btn_w, ROW_H, "Cancel", BTN_BG, VALUE_FG, function() exit_spec(false) end)
  footer_add(fx + btn_w + gap, footer_y, btn_w, ROW_H, "Done", DONE_BG, VALUE_FG, function() exit_spec(true) end)
  footer_add(W - MARGIN + 1, CONTENT_TOP, ICON_W, ICON_W, "^", BTN_BG, VALUE_FG, function()
    S.spec_scroll = S.spec_scroll - 40
  end)
  footer_add(W - MARGIN + 1, H - FOOTER_H - ICON_W, ICON_W, ICON_W, "v", BTN_BG, VALUE_FG, function()
    S.spec_scroll = S.spec_scroll + 40
  end)

  -- mask overflow, then header + footer on top
  rect(0, 0, W, S.view_top, SPEC_BG)
  rect(0, S.view_top - 1, W, 1, C_BORDER)
  draw_text_centered(0, TITLE_Y, W, "Seed Specification", C_TEXT)
  draw_text_centered(0, TITLE_Y + 11, W, "build rules, no typing", C_MUTE)
  if #S.rules == 0 then
    draw_text_centered(0, CONTENT_TOP + 24, W, "No rules yet. Press  + Add Rule  to start.", MUTED_FG)
  end
  rect(0, S.view_bottom, W, H - S.view_bottom, SPEC_BG)
  rect(0, S.view_bottom, W, 1, C_BORDER)
  for _, c in ipairs(S.spec_footer) do
    rect(c.x, c.y, c.w, c.h, c.bg)
    if in_rect(S.mx, S.my, c.x, c.y, c.w, c.h) then
      rect(c.x, c.y, c.w, c.h, HOVER)
    end
    border(c.x, c.y, c.w, c.h, C_BORDER)
    draw_text_centered(c.x, c.y + math.floor((c.h - 8) / 2), c.w, c.text, c.fg)
  end
end

local function spec_mouse(event)
  if event.button ~= 0 then
    return
  end
  for i = #S.spec_footer, 1, -1 do
    local b = S.spec_footer[i]
    if in_rect(event.x, event.y, b.x, b.y, b.w, b.h) then
      b.fn()
      event.handled = true
      return
    end
  end
  for i = #S.spec_boxes, 1, -1 do
    local b = S.spec_boxes[i]
    if in_rect(event.x, event.y, b.x, b.y, b.w, b.h) then
      b.fn()
      event.handled = true
      return
    end
  end
end

----------------------------------------------------------------------
-- finder layout + drawing
----------------------------------------------------------------------

local function add_hit(x, y, w, h, fn)
  S.hitboxes[#S.hitboxes + 1] = { x = x, y = y, w = w, h = h, fn = fn }
end

-- Unified themed button. opts: { variant = "accent"|"danger"|"ghost",
-- active = bool, disabled = bool, align = "left" }.
local function ui_button(x, y, w, h, label, fn, opts)
  opts = opts or {}
  local hover = in_rect(S.mx, S.my, x, y, w, h)
  local base, hov, fg = C_BTN, C_BTN_HOV, C_TEXT
  if opts.variant == "accent" then
    base, hov = C_BTN_ON, C_BTN_ONH
  elseif opts.variant == "danger" then
    base, hov = 0xFF7E3030, 0xFF9A3A3A
  elseif opts.variant == "ghost" then
    base, hov = 0xFF1B212B, C_BTN_HOV
  end
  if opts.active then
    base, hov = C_BTN_ON, C_BTN_ONH
  end
  if opts.disabled then
    base, hov, fg = 0xFF1A1F27, 0xFF1A1F27, C_MUTE
    hover = false
  end
  rect(x, y, w, h, hover and hov or base)
  rect(x, y, w, 1, C_BEVEL_HI)
  rect(x, y + h - 1, w, 1, C_BEVEL_LO)
  border(x, y, w, h, (hover or opts.active) and C_BORDER_HI or C_BORDER)
  local ty = y + math.floor((h - 8) / 2)
  if opts.align == "left" then
    text(x + 6, ty, elide(label, w - 10), fg)
  else
    draw_text_centered(x, ty, w, elide(label, w - 8), fg)
  end
  if fn ~= nil and not opts.disabled then
    add_hit(x, y, w, h, fn)
  end
end

local function layout(W, H)
  local L = { W = W, H = H }
  L.pad = 6
  L.header_h = 26
  L.footer_h = 24
  L.content_y = L.header_h + 4
  L.content_bottom = H - L.footer_h - 4

  -- left configuration panel
  L.lpx = L.pad
  L.lpw = math.max(188, math.min(210, math.floor(W * 0.42)))
  L.lpy = L.content_y
  L.lph = L.content_bottom - L.content_y

  -- right results / map panel
  L.rpx = L.lpx + L.lpw + L.pad
  L.rpw = W - L.rpx - L.pad

  -- two-column field grid (label above narrow field)
  local inner_x = L.lpx + 8
  local inner_w = L.lpw - 16
  local col_gap = 8
  local col_w = math.floor((inner_w - col_gap) / 2)
  local col0 = inner_x
  local col1 = inner_x + col_w + col_gap

  L.fields = {
    { name = "seed_start", label = "Seed start", numeric = true, signed = true },
    { name = "seed_end", label = "Seed end", numeric = true, signed = true },
    { name = "radius", label = "Radius (ch)", numeric = true },
    { name = "top_k", label = "Keep top", numeric = true },
    { name = "min_score", label = "Min score", numeric = true, signed = true, decimal = true },
    { name = "unique_min", label = "Min biomes", numeric = true },
  }

  local body_top = L.lpy + 8
  local body_bottom = L.lpy + L.lph - 8
  local total = body_bottom - body_top
  local field_rows = 3
  local ctrl_rows = 2
  local search_h = 18
  local block = total - search_h - 16
  local field_stride = math.max(22, math.min(28, math.floor((block * 0.58) / field_rows)))
  local ctrl_stride = math.max(18, math.min(24, math.floor((block * 0.42) / ctrl_rows)))

  for i, f in ipairs(L.fields) do
    local r = math.floor((i - 1) / 2)
    local c = (i - 1) % 2
    f.x = (c == 0) and col0 or col1
    f.w = col_w
    f.h = 14
    f.label_y = body_top + r * field_stride
    f.y = f.label_y + 9
  end

  L.col0 = col0
  L.col1 = col1
  L.col_w = col_w
  L.inner_x = inner_x
  L.inner_w = inner_w
  L.field_stride = field_stride
  L.ctrl_stride = ctrl_stride

  local ctrl_top = body_top + field_rows * field_stride + 6
  L.ctrl_top = ctrl_top
  L.ctrl_h = 16
  L.search_y = ctrl_top + ctrl_rows * ctrl_stride + 6
  L.search_h = search_h

  L.footer_y = H - L.footer_h
  L.use_w = 92
  L.back_w = 70

  L.list_x = L.rpx + 6
  L.list_w = L.rpw - 12
  L.list_top = L.content_y + 18
  L.row_h = 12
  local map_size = math.min(L.rpw - 12, math.max(96, math.floor((L.content_bottom - L.content_y) * 0.42)))
  L.map_size = map_size
  L.map_panel_h = map_size + 16
  L.map_panel_y = L.content_bottom - L.map_panel_h
  L.list_bottom = L.map_panel_y - 6
  L.rows = math.max(1, math.floor((L.list_bottom - L.list_top) / L.row_h))
  return L
end

local function render_screen(event)
  local L = S.layout
  if L == nil then
    L = layout(event.width, event.height)
    S.layout = L
  end
  S.mx = event.mouse_x
  S.my = event.mouse_y
  S.hitboxes = {}

  -- backdrop + header
  rect(0, 0, L.W, L.H, C_BACKDROP)
  rect(0, 0, L.W, L.header_h, C_HEADER)
  rect(0, L.header_h - 1, L.W, 1, C_ACCENT_DK)
  rect(8, 8, 3, 10, C_ACCENT)
  text(16, 9, "SEED FINDER", C_TEXT)
  local hint = "Enter search   Up/Down select   Esc back"
  text_right(L.W - 8, 9, hint, C_MUTE)

  -- panels
  panel(L.lpx, L.lpy, L.lpw, L.lph, C_PANEL, C_BORDER)
  panel(L.rpx, L.content_y, L.rpw, L.content_bottom - L.content_y, C_PANEL, C_BORDER)

  -- config fields (native widgets draw themselves; we draw the labels above)
  for _, f in ipairs(L.fields) do
    text(f.x, f.label_y, f.label, C_DIMTEXT)
  end

  -- control buttons (2x2): spawn / preview / rules / import
  local ct = L.ctrl_top
  ui_button(L.col0, ct, L.col_w, L.ctrl_h, "Spawn: " .. spawn_label(), function()
    local total = #S.spawn_biomes
    S.spawn_index = S.spawn_index + 1
    if S.spawn_index > total then
      S.spawn_index = 0
    end
  end, { align = "left" })
  ui_button(L.col1, ct, L.col_w, L.ctrl_h, "Preview start", preview_seed, { align = "left" })
  local ct2 = ct + L.ctrl_stride
  ui_button(L.col0, ct2, L.col_w, L.ctrl_h, "Rules (" .. tostring(#S.rules) .. ")", enter_spec,
    { align = "left", active = (#S.rules > 0) })
  ui_button(L.col1, ct2, L.col_w, L.ctrl_h, "Import JSON", do_import, { align = "left" })

  ui_button(L.inner_x, L.search_y, L.inner_w, L.search_h,
    S.searching and "Stop search" or "Search seeds", toggle_search,
    { variant = S.searching and "danger" or "accent" })

  rect(0, L.footer_y, L.W, L.footer_h, C_HEADER)
  rect(0, L.footer_y, L.W, 1, C_BORDER)
  local fb_y = L.footer_y + math.floor((L.footer_h - 16) / 2)
  ui_button(L.pad, fb_y, L.back_w, 16, "Back", go_back, { variant = "ghost" })
  ui_button(L.W - L.pad - L.use_w, fb_y, L.use_w, 16, "Use Seed", use_selected,
    { variant = "accent", disabled = (S.selected_seed == nil) })
  local mid = S.searching and ("Searching... " .. fmt_seed(S.checked) .. " checked")
    or (#S.hits .. " result" .. (#S.hits == 1 and "" or "s") .. "  -  " .. fmt_seed(S.checked) .. " checked")
  draw_text_centered(L.back_w + L.pad * 2, fb_y, L.W - (L.back_w + L.use_w + L.pad * 4), mid,
    S.searching and C_ACCENT or C_DIMTEXT)

  text(L.rpx + 6, L.content_y + 5, "RESULTS", C_DIMTEXT)
  text_right(L.rpx + L.rpw - 6, L.content_y + 5, "score", C_MUTE)
  rect(L.rpx + 1, L.list_top - 2, L.rpw - 2, 1, C_BORDER)

  if S.scroll > math.max(0, #S.hits - L.rows) then
    S.scroll = math.max(0, #S.hits - L.rows)
  end
  if S.scroll < 0 then
    S.scroll = 0
  end
  rect(L.list_x - 2, L.list_top, L.list_w + 4, L.list_bottom - L.list_top, C_TRACK)
  for row = 1, L.rows do
    local i = S.scroll + row
    local h = S.hits[i]
    if h ~= nil then
      local y = L.list_top + (row - 1) * L.row_h
      local key = fmt_seed(h.seed)
      local sel = (key == S.selected_seed)
      local hover = in_rect(S.mx, S.my, L.list_x, y, L.list_w, L.row_h)
      local bg = (row % 2 == 0) and C_ROW_ALT or C_ROW
      if sel then
        bg = C_ROW_SEL
      elseif hover then
        bg = C_BTN_HOV
      end
      rect(L.list_x, y, L.list_w, L.row_h - 1, bg)
      if sel then
        rect(L.list_x, y, 2, L.row_h - 1, C_ACCENT)
      end
      local biome = pretty(minecraft.registry.name("biome", h.spawn_biome_id or 0))
      local left_text = elide(key .. "  " .. biome, L.list_w - 40)
      text(L.list_x + 5, y + 2, left_text, sel and C_TEXT or C_DIMTEXT)
      text_right(L.list_x + L.list_w - 5, y + 2, string.format("%.1f", h.score or 0),
        sel and C_ACCENT or C_MUTE)
      add_hit(L.list_x, y, L.list_w, L.row_h, function()
        select_index(i)
      end)
    end
  end

  if #S.hits > L.rows then
    local track_h = L.list_bottom - L.list_top
    local bar_x = L.list_x + L.list_w - 3
    rect(bar_x, L.list_top, 3, track_h, C_TRACK)
    local thumb = math.max(12, math.floor(track_h * L.rows / #S.hits))
    local maxs = math.max(1, #S.hits - L.rows)
    local pos = math.floor((track_h - thumb) * S.scroll / maxs)
    rect(bar_x, L.list_top + pos, 3, thumb, C_THUMB)
  end

  panel(L.rpx, L.map_panel_y, L.rpw, L.map_panel_h, C_PANEL2, C_BORDER)
  text(L.rpx + 6, L.map_panel_y + 4, "BIOME MAP", C_DIMTEXT)
  local si = selected_index()
  if si ~= nil then
    local h = S.hits[si]
    text_right(L.rpx + L.rpw - 6, L.map_panel_y + 4,
      string.format("%d, %d", h.spawn_x or 0, h.spawn_z or 0), C_MUTE)
  end
  local img = L.map_size
  local img_x = L.rpx + math.floor((L.rpw - img) / 2)
  local img_y = L.map_panel_y + 14
  rect(img_x - 1, img_y - 1, img + 2, img + 2, C_BORDER)
  local drew = false
  if S.map_tex ~= nil and S.map_tex.id ~= nil and S.map_tex.id > 0 then
    minecraft.gui.draw_texture(S.map_tex.id, img_x, img_y, img, img)
    local side = S.map_tex.side or 1
    if side < 1 then side = 1 end
    local mxp = img_x + math.floor((S.map_tex.spawn_px or 0) / side * img)
    local myp = img_y + math.floor((S.map_tex.spawn_pz or 0) / side * img)
    rect(mxp - 3, myp - 1, 7, 3, 0xFFFF5050)
    rect(mxp - 1, myp - 3, 3, 7, 0xFFFF5050)
    rect(mxp, myp, 1, 1, 0xFFFFFFFF)
    drew = true
  elseif S.map ~= nil and S.map.values ~= nil and S.map.side ~= nil and S.map.side > 0 then
    local side = S.map.side
    local cell = math.max(1, math.floor(img / side))
    local mw = cell * side
    local ox = img_x + math.floor((img - mw) / 2)
    local oy = img_y + math.floor((img - mw) / 2)
    rect(img_x, img_y, img, img, 0xFF0E1219)
    for r = 0, side - 1 do
      for c = 0, side - 1 do
        local color = S.map.values[r * side + c + 1]
        if color ~= nil then
          rect(ox + c * cell, oy + r * cell, cell, cell, color)
        end
      end
    end
    local cx = ox + math.floor(mw / 2)
    local cyy = oy + math.floor(mw / 2)
    rect(cx - 1, cyy - 1, 3, 3, 0xFFFF5050)
    drew = true
  end
  if not drew then
    rect(img_x, img_y, img, img, 0xFF0E1219)
    draw_text_centered(img_x, img_y + math.floor(img / 2) - 4, img, "select a result", C_MUTE)
  end
end

----------------------------------------------------------------------
-- events
----------------------------------------------------------------------

minecraft.on(minecraft.events.screen_ui, {
  screen_id = minecraft.screen.ids.create_world,
  region = minecraft.screen.regions.footer,
  priority = 90,
}, function(event)
  if event.ui == nil then
    return event
  end
  local host = event.host_fields or {}
  S.world_name = host.world_name or minecraft.screen.host_field("world_name")
  S.seed_text = host.seed or S.seed_text or ""
  event.ui:add_stacked_centered_button("Seed tools...", function()
    S.world_name = minecraft.screen.host_field("world_name")
    S.seed_text = minecraft.screen.host_field("seed")
    open_finder()
  end)
  return event
end)

minecraft.on(minecraft.events.screen_event, { screen_id = SCREEN_ID }, function(event)
  if event.phase == "init" then
    local L = layout(event.width, event.height)
    S.layout = L
    for _, f in ipairs(L.fields) do
      minecraft.screen.add_field(f.name, f.x, f.y, f.w, f.h, {
        text = S.saved[f.name] or "",
        numeric = f.numeric or false,
        signed = f.signed or false,
        decimal = f.decimal or false,
        max_len = 20,
      })
    end
    if S.mode == "spec" then
      minecraft.screen.set_fields_visible(false)
    end
    return event
  elseif event.phase == "render" then
    if S.mode == "spec" then
      spec_render(event)
    else
      render_screen(event)
    end
    return event
  elseif event.phase == "tick" then
    search_tick()
    return event
  elseif event.phase == "mouse" then
    if S.mode == "spec" then
      spec_mouse(event)
      return event
    end
    if event.button ~= 0 then
      return event
    end
    for i = #S.hitboxes, 1, -1 do
      local box = S.hitboxes[i]
      if in_rect(event.x, event.y, box.x, box.y, box.w, box.h) then
        box.fn()
        event.handled = true
        return event
      end
    end
    return event
  elseif event.phase == "scroll" then
    if S.mode == "spec" then
      S.spec_scroll = S.spec_scroll - ((event.delta or 0) > 0 and 24 or -24)
      event.handled = true
    else
      local step = (event.delta or 0) > 0 and -1 or 1
      S.scroll = S.scroll + step
      if S.scroll < 0 then
        S.scroll = 0
      end
      event.handled = true
    end
    return event
  elseif event.phase == "key" then
    if S.mode == "spec" then
      if event.key == KEY_ESCAPE then
        exit_spec(false)
        event.handled = true
      end
      return event
    end
    if event.key == KEY_ESCAPE then
      go_back()
      event.handled = true
    elseif event.key == KEY_ENTER then
      toggle_search()
      event.handled = true
    elseif event.key == KEY_UP then
      move_selection(-1)
      event.handled = true
    elseif event.key == KEY_DOWN then
      move_selection(1)
      event.handled = true
    end
    return event
  elseif event.phase == "close" then
    S.searching = false
    S.search_cfg = nil
    free_map()
    return event
  end
end)
