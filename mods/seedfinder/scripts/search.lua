local search = {}
local rules = minecraft.require("scripts.rules")

function search.field_num(name, default)
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

function search.save_fields()
  for _, f in ipairs((S.layout and S.layout.fields) or {}) do
    S.saved[f.name] = minecraft.screen.field_text(f.name)
  end
end

function search.spawn_label()
  if S.spawn_index <= 0 then
    return "Any"
  end
  return pretty(S.spawn_biomes[S.spawn_index] or "Any")
end

function search.apply_import_json(json)
  local cfg, err = minecraft.util.json_decode(json)
  if cfg == nil then
    minecraft.log("error", "seedfinder: " .. tostring(err))
    return
  end
  minecraft.screen.set_field_text("seed_start", fmt_seed(rules.cfg_number(cfg, "seed_start", "seed_range_start") or 0))
  minecraft.screen.set_field_text("seed_end", fmt_seed(rules.cfg_number(cfg, "seed_end", "seed_range_end") or 100000))
  local radius = rules.cfg_number(cfg, "scan_radius_chunks")
  if radius ~= nil then
    minecraft.screen.set_field_text("radius", tostring(radius))
  end
  local top_k = rules.cfg_number(cfg, "top_k")
  if top_k ~= nil then
    minecraft.screen.set_field_text("top_k", tostring(top_k))
  end
  local min_score = rules.cfg_number(cfg, "min_score")
  if min_score ~= nil then
    minecraft.screen.set_field_text("min_score", tostring(min_score))
  end
  local unique = rules.cfg_number(cfg, "unique_biome_min")
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
    S.rules = rules.clone_rules(rules.decode_rules_blob(cfg.advanced_rule_data_nbt))
  elseif cfg.rules ~= nil or cfg.objectives ~= nil then
    S.rules = rules.clone_rules(rules.decode_rules_table({
      rules = cfg.rules or {},
      objectives = cfg.objectives or {},
    }))
  end
end

function search.selected_index()
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

function search.free_map()
  if S.map_tex ~= nil and S.map_tex.id ~= nil then
    minecraft.render.release_texture(S.map_tex.id)
  end
  S.map_tex = nil
  S.map = nil
end

function search.build_map(hit)
  search.free_map()
  if hit == nil then
    return
  end
  local radius = math.floor(minecraft.util.clamp(search.field_num("radius", 4), 1, 64))
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

function search.select_index(i)
  local h = S.hits[i]
  if h == nil then
    return
  end
  S.selected_seed = fmt_seed(h.seed)
  search.build_map(h)
end

function search.merge_hits(list)
  if list == nil then
    return
  end
  local top_k = math.floor(minecraft.util.clamp(
    (S.search_cfg and S.search_cfg.top_k) or search.field_num("top_k", 12), 1, 100))
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
    search.select_index(1)
  end
end

function search.move_selection(delta)
  if #S.hits == 0 then
    return
  end
  local i = search.selected_index() or 0
  i = i + delta
  if i < 1 then
    i = 1
  end
  if i > #S.hits then
    i = #S.hits
  end
  search.select_index(i)
  if S.layout ~= nil then
    if i - 1 < S.scroll then
      S.scroll = i - 1
    end
    if i - 1 >= S.scroll + S.layout.rows then
      S.scroll = i - S.layout.rows
    end
  end
end

function search.preview_seed()
  search.save_fields()
  local seed_text = minecraft.screen.field_text("seed_start") or "0"
  local seed = minecraft.util.resolve_seed(seed_text)
  search.build_map({ seed = seed, spawn_x = 0, spawn_z = 0 })
end

function search.search_tick()
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
    local hit = rules.score_seed(seed, cfg)
    if hit ~= nil then
      search.merge_hits({ hit })
    end
    if final_seed then
      S.searching = false
      S.search_cfg = nil
      return
    end
  end
end

function search.toggle_search()
  if S.searching then
    S.searching = false
    S.search_cfg = nil
    return
  end
  search.save_fields()
  S.hits = {}
  S.seen = {}
  S.selected_seed = nil
  S.checked = 0
  S.scroll = 0
  search.free_map()
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
    radius = math.floor(minecraft.util.clamp(search.field_num("radius", 4), 1, 64)),
    min_score = search.field_num("min_score", 0),
    unique_min = math.floor(minecraft.util.clamp(search.field_num("unique_min", -1), -1, 64)),
    top_k = math.floor(minecraft.util.clamp(search.field_num("top_k", 12), 1, 100)),
    spawn_index = S.spawn_index,
    terrain = needs_terrain,
  }
  S.searching = true
end

function search.do_import()
  local path = minecraft.files.pick({extension = "json"})
  if path == nil then
    return
  end
  local text, err = minecraft.files.read(path)
  if text == nil then
    minecraft.log("error", "seedfinder: " .. tostring(err))
    return
  end
  search.apply_import_json(text)
  minecraft.log("info", "seedfinder: imported " .. tostring(path))
end

function search.use_selected()
  local i = search.selected_index()
  if i == nil then
    return
  end
  local h = S.hits[i]
  if h == nil then
    return
  end
  S.searching = false
  search.free_map()
  minecraft.screen.open_host(minecraft.screen.ids.create_world, {
    world_name = S.world_name,
    seed = fmt_seed(h.seed),
  })
end

function search.go_back()
  S.searching = false
  search.free_map()
  minecraft.screen.open_host(minecraft.screen.ids.create_world, {
    world_name = S.world_name,
    seed = S.seed_text or "",
  })
end

function search.open_finder()
  S.mode = "finder"
  S.hits = {}
  S.seen = {}
  S.selected_seed = nil
  S.checked = 0
  S.searching = false
  S.scroll = 0
  S.preview_seed = nil
  search.free_map()
  minecraft.screen.open(SCREEN_ID)
end

return search
