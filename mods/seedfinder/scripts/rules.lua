local rules = {}

local BLOCKS = {
  { 1, "Stone" }, { 2, "Grass" }, { 3, "Dirt" }, { 4, "Cobblestone" }, { 12, "Sand" },
  { 13, "Gravel" }, { 14, "Gold Ore" }, { 15, "Iron Ore" }, { 16, "Coal Ore" }, { 17, "Wood" },
  { 18, "Leaves" }, { 24, "Sandstone" }, { 49, "Obsidian" }, { 56, "Diamond Ore" }, { 73, "Redstone Ore" },
  { 21, "Lapis Ore" }, { 79, "Ice" }, { 78, "Snow" }, { 81, "Cactus" }, { 82, "Clay" },
  { 87, "Netherrack" }, { 88, "Soul Sand" }, { 89, "Glowstone" },
}

function rules.block_name(id)
  for _, e in ipairs(BLOCKS) do
    if e[1] == id then
      return e[2]
    end
  end
  return "block " .. tostring(id)
end

function rules.block_index(id)
  for i, e in ipairs(BLOCKS) do
    if e[1] == id then
      return i
    end
  end
  return 1
end

function rules.cycle_block(id, dir)
  local n = #BLOCKS
  local idx = rules.block_index(id) + dir
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

function rules.metric_index(m)
  for i, v in ipairs(OBJ_METRICS) do
    if v == m then
      return i
    end
  end
  return 1
end

function rules.cycle_metric(m, dir)
  local n = #OBJ_METRICS
  local idx = rules.metric_index(m) + dir
  idx = ((idx - 1) % n + n) % n + 1
  return OBJ_METRICS[idx]
end

function rules.metric_label(m)
  return OBJ_LABELS[m] or "biome variety in scan area"
end

function rules.needs_biome(m)
  return BIOME_METRICS[m] == true
end

local KIND_ORDER = { "block", "block_on_block", "biome", "objective" }

function rules.kind_index(k)
  for i, v in ipairs(KIND_ORDER) do
    if v == k then
      return i
    end
  end
  return 3
end

function rules.cycle_kind(k, dir)
  local n = #KIND_ORDER
  local idx = rules.kind_index(k) + dir
  idx = ((idx - 1) % n + n) % n + 1
  return KIND_ORDER[idx]
end

function rules.kind_label(k)
  if k == "block" then
    return "Block"
  elseif k == "block_on_block" then
    return "Block-on-Block"
  elseif k == "objective" then
    return "Objective"
  end
  return "Biome"
end

function rules.make_default(kind)
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

function rules.clone_rule(r)
  local v = {}
  for i, name in ipairs(r.values or {}) do
    v[i] = name
  end
  return { kind = r.kind, metric = r.metric, op = r.op, direction = r.direction, values = v,
    block_id = r.block_id, block_below_id = r.block_below_id, min_value = r.min_value,
    max_value = r.max_value, value = r.value, weight = r.weight }
end

function rules.clone_rules(list)
  local out = {}
  for i, r in ipairs(list or {}) do
    out[i] = rules.clone_rule(r)
  end
  return out
end

function rules.stepper_text(v)
  if v == nil or v < 0 then
    return "any"
  end
  return tostring(math.floor(v))
end

function rules.json_string_list(values)
  local parts = {}
  for i, value in ipairs(values or {}) do
    parts[i] = '"' .. tostring(value):gsub('\\', '\\\\'):gsub('"', '\\"') .. '"'
  end
  return "[" .. table.concat(parts, ",") .. "]"
end

function rules.encode_rule_node(node)
  if node.kind == "objective" then
    local out = '{"type":"objective","metric":"' .. tostring(node.metric or "") .. '","direction":"'
      .. tostring(node.direction ~= "" and node.direction or "maximize") .. '","weight":'
      .. tostring(node.weight or 1)
    if node.values ~= nil and #node.values > 0 then
      out = out .. ',"biomes":' .. rules.json_string_list(node.values)
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
    .. rules.json_string_list(node.values)
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

function rules.encode_rules_blob(rules_list)
  local rule_parts = {}
  local objective_parts = {}
  for _, node in ipairs(rules_list or {}) do
    if node.kind == "objective" then
      objective_parts[#objective_parts + 1] = rules.encode_rule_node(node)
    else
      rule_parts[#rule_parts + 1] = rules.encode_rule_node(node)
    end
  end
  return '{"rules":[' .. table.concat(rule_parts, ",") .. '],"objectives":[' .. table.concat(objective_parts, ",") .. "]}"
end

function rules.decode_rules_table(parsed)
  local result = {}
  local function push_node(raw, kind)
    local values = {}
    for i, name in ipairs(raw.biomes or raw.values or {}) do
      values[i] = name
    end
    result[#result + 1] = {
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
  return result
end

function rules.decode_rules_blob(blob)
  if blob == nil or blob == "" then
    return {}
  end
  local parsed, err = minecraft.util.json_decode(blob)
  if parsed == nil then
    minecraft.log("warn", "seedfinder: rule decode failed: " .. tostring(err))
    return {}
  end
  return rules.decode_rules_table(parsed)
end

function rules.cfg_number(cfg, ...)
  for i = 1, select("#", ...) do
    local key = select(i, ...)
    local value = cfg[key]
    if value ~= nil then
      return tonumber(value)
    end
  end
  return nil
end

function rules.biome_name(id)
  return minecraft.registry.name("biome", id or 0)
end

function rules.biome_listed(name, list)
  for _, entry in ipairs(list or {}) do
    if entry == name then
      return true
    end
  end
  return false
end

function rules.grid_biome_stats(grid)
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

function rules.coverage_for_biomes(stats, names)
  local matched = 0
  for id, n in pairs(stats.counts) do
    if rules.biome_listed(rules.biome_name(id), names) then
      matched = matched + n
    end
  end
  return stats.total > 0 and (100.0 * matched / stats.total) or 0
end

function rules.biome_area_stats(grid, names)
  local side = grid.side or 0
  local total = side * side
  local match = {}
  local matching = 0
  for i = 1, total do
    local yes = rules.biome_listed(rules.biome_name(grid.values[i] or 0), names)
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

function rules.eval_biome_rule(rule, stats, grid)
  if #(rule.values or {}) == 0 then
    return false
  end
  local op = rule.op ~= "" and rule.op or "any_of"
  local cov = rules.coverage_for_biomes(stats, rule.values)
  local matches
  if op == "none_of" then
    matches = cov <= 0.001
  elseif op == "all_of" then
    matches = true
    for _, want in ipairs(rule.values or {}) do
      local found = false
      for id, n in pairs(stats.counts) do
        if n > 0 and rules.biome_name(id) == want then
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
    local radius = rules.biome_area_stats(grid, rule.values)
    if radius < rule.value then
      return false
    end
  end
  return true
end

function rules.objective_value(rule, stats, grid)
  local metric = rule.metric or ""
  if metric == "unique_biome_count" then
    return stats.unique
  end
  if metric == "dominant_biome_percent" then
    return stats.dominant_pct
  end
  if metric == "biome_chunk_count" then
    local sample_area = math.max(1, grid.step or 1) ^ 2
    return rules.coverage_for_biomes(stats, rule.values) * stats.total * sample_area / 25600.0
  end
  if metric == "biome" or metric == "biome_coverage_pct" or metric == "biome_coverage_percent" then
    return rules.coverage_for_biomes(stats, rule.values)
  end
  if metric == "max_contiguous_biome_radius" then
    return rules.biome_area_stats(grid, rule.values)
  end
  if metric == "biome_blob_compactness_percent" then
    local _, compactness = rules.biome_area_stats(grid, rule.values)
    return compactness
  end
  return stats.unique
end

function rules.center_grid_value(grid, channel)
  local values = grid.channels and grid.channels[channel]
  if values == nil then
    return 0
  end
  local c = math.floor(grid.side / 2)
  return values[c * grid.side + c + 1] or 0
end

function rules.count_surface_pair(grid, top_id, below_id)
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

function rules.eval_block_rule(rule, grid)
  local top = grid.channels and grid.channels.surface_block or {}
  if rule.kind == "block_on_block" then
    local count = rules.count_surface_pair(grid, rule.block_id, rule.block_below_id)
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
  local actual = rules.center_grid_value(grid, "surface_block")
  if rule.op == "ne" then
    return actual ~= rule.block_id
  end
  return actual == rule.block_id
end

function rules.terrain_stats(grid)
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

function rules.score_seed(seed, cfg)
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
  local stats = rules.grid_biome_stats(grid)
  if cfg.spawn_index > 0 then
    local want = S.spawn_biomes[cfg.spawn_index]
    if rules.biome_name(stats.spawn_biome_id) ~= want then
      return nil
    end
  end
  for _, rule in ipairs(S.rules) do
    if rule.kind == "biome" and not rules.eval_biome_rule(rule, stats, grid) then
      return nil
    end
    if (rule.kind == "block" or rule.kind == "block_on_block") and not rules.eval_block_rule(rule, grid) then
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
      local value = rules.objective_value(rule, stats, grid)
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
  local avg_y, min_y, max_y, underwater = rules.terrain_stats(grid)
  return {
    seed = seed,
    score = score,
    all_hard = true,
    spawn_x = 0,
    spawn_y = cfg.terrain and rules.center_grid_value(grid, "height") or 64,
    spawn_z = 0,
    spawn_valid = true,
    spawn_biome_id = stats.spawn_biome_id,
    unique_biome_count = stats.unique,
    dominant_biome_id = stats.dominant_id,
    dominant_biome_percent = math.floor(stats.dominant_pct),
    spawn_surface_block_id = cfg.terrain and rules.center_grid_value(grid, "surface_block") or 0,
    avg_surface_y = avg_y,
    min_surface_y = min_y,
    max_surface_y = max_y,
    underwater_percent = underwater,
  }
end

return rules
