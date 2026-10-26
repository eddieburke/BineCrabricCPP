local search = {}

search.BIOMES = {
  "plains", "forest", "desert", "taiga", "swampland", "rainforest",
  "savanna", "shrubland", "seasonal_forest", "ice_desert", "tundra",
}

search.LISTS = { "origin", "require", "forbid", "prefer", "spawn" }

search.LIST_LABELS = {
  origin = "Origin",
  require = "Need",
  forbid = "Ban",
  prefer = "Prefer",
  spawn = "Spawn",
}

search.LIST_HELP = {
  origin = "Biome at 0,0 must be one of the selected",
  require = "Every selected biome must appear in the area",
  forbid = "None of the selected may appear in the area",
  prefer = "Higher score for more coverage of these",
  spawn = "Biome of the real world spawn (Exact spawn)",
}

search.GROUND = { "any", "land", "ocean" }

function fmt_seed(n)
  n = n or 0
  if math.type(n) == "integer" then
    return tostring(n)
  end
  return string.format("%.0f", n)
end

function pretty(name)
  return (tostring(name):gsub("_", " "))
end

function fmt_count(n)
  n = math.floor(n or 0)
  local s = tostring(n)
  local out = s:reverse():gsub("(%d%d%d)", "%1,"):reverse()
  if out:sub(1, 1) == "," then
    out = out:sub(2)
  end
  return out
end

function search.field_num(name, default)
  local text = minecraft.screen.field_text(name)
  if text == nil or text == "" then
    return default
  end
  return tonumber(text) or default
end

function search.field_int(name, default, lo, hi)
  local v = math.floor(search.field_num(name, default))
  return minecraft.util.clamp(v, lo, hi)
end

function search.save_fields()
  for _, f in ipairs((S.layout and S.layout.fields) or {}) do
    S.saved[f.name] = minecraft.screen.field_text(f.name)
  end
end

function search.list_names(which)
  local out = {}
  for _, name in ipairs(search.BIOMES) do
    if S.lists[which][name] then
      out[#out + 1] = name
    end
  end
  return out
end

function search.clear_lists()
  for _, which in ipairs(search.LISTS) do
    S.lists[which] = {}
  end
end

function search.build_criteria()
  local seed_start = minecraft.util.resolve_seed(minecraft.screen.field_text("seed_start") or "0")
  local seed_end = minecraft.util.resolve_seed(minecraft.screen.field_text("seed_end") or "100000")
  local c = {
    mode = S.random and "random" or "range",
    seed_start = seed_start,
    seed_end = seed_end,
    samples = search.field_int("samples", 1000000, 1, 2000000000),
    max_results = search.field_int("keep", 24, 1, 200),
    stop_after = search.field_int("stop_after", 0, 0, 1000000),
    origin_biomes = search.list_names("origin"),
  }
  if S.area_on then
    c.area = {
      radius = search.field_int("radius", 128, 16, 4096),
      step = search.field_int("step", 16, 4, 256),
      require = search.list_names("require"),
      forbid = search.list_names("forbid"),
      prefer = search.list_names("prefer"),
      min_unique = search.field_int("min_unique", 0, 0, 13),
    }
  end
  if S.spawn_on then
    c.spawn = {
      biomes = search.list_names("spawn"),
      max_distance = search.field_int("spawn_max", -1, -1, 100000),
    }
  end
  if S.ground ~= "any" then
    c.terrain = { { x = 0, z = 0, land = (S.ground == "land") } }
  end
  return c
end

function search.free_map()
  if S.map_tex ~= nil and S.map_tex.id ~= nil then
    minecraft.render.release_texture(S.map_tex.id)
  end
  S.map_tex = nil
end

function search.build_map(seed)
  search.free_map()
  if seed == nil then
    return
  end
  local grid = minecraft.seeds.map(seed, 0, 0, { radius = S.map_radius, side = 192 })
  if grid == nil then
    return
  end
  local tex = minecraft.render.create_texture({
    width = grid.side,
    height = grid.side,
    pixels = grid.channels.grass,
  })
  if tex ~= nil and tex.id ~= nil then
    S.map_tex = { id = tex.id, side = grid.side, origin_x = grid.origin_x, origin_z = grid.origin_z,
      span = grid.side * grid.step }
  end
end

function search.stop_detail()
  if S.detail_job ~= nil then
    minecraft.seeds.release(S.detail_job)
    S.detail_job = nil
  end
end

function search.select_index(i)
  local h = S.hits[i]
  if h == nil then
    return
  end
  S.selected_seed = fmt_seed(h.seed)
  S.detail = h.spawn_x ~= nil and h or nil
  S.detail_pending = false
  search.stop_detail()
  search.build_map(h.seed)
  if S.detail == nil then
    local job = minecraft.seeds.search({
      seed_start = h.seed,
      seed_end = h.seed,
      spawn = {},
    }, 1)
    if job ~= nil then
      S.detail_job = job
      S.detail_pending = true
    end
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

function search.selected_hit()
  local i = search.selected_index()
  return i ~= nil and S.hits[i] or nil
end

function search.move_selection(delta)
  if #S.hits == 0 then
    return
  end
  local i = (search.selected_index() or 0) + delta
  i = minecraft.util.clamp(i, 1, #S.hits)
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

function search.stop_job()
  if S.job ~= nil then
    minecraft.seeds.cancel(S.job)
    minecraft.seeds.release(S.job)
    S.job = nil
  end
  S.running = false
end

function search.toggle_search()
  if S.running then
    search.stop_job()
    return
  end
  search.save_fields()
  search.stop_detail()
  search.free_map()
  S.hits = {}
  S.selected_seed = nil
  S.detail = nil
  S.scroll = 0
  S.progress = nil
  S.error = nil
  local job, err = minecraft.seeds.search(search.build_criteria())
  if job == nil then
    S.error = tostring(err)
    return
  end
  S.job = job
  S.running = true
end

function search.tick()
  if S.job ~= nil then
    local p = minecraft.seeds.poll(S.job)
    if p == nil then
      S.job = nil
      S.running = false
    else
      S.progress = p
      S.hits = p.results
      if S.selected_seed == nil and #S.hits > 0 then
        search.select_index(1)
      end
      if p.state ~= "running" then
        minecraft.seeds.release(S.job)
        S.job = nil
        S.running = false
      end
    end
  end
  if S.detail_job ~= nil then
    local p = minecraft.seeds.poll(S.detail_job)
    if p == nil or p.state ~= "running" then
      if p ~= nil and p.results[1] ~= nil then
        S.detail = p.results[1]
      end
      S.detail_pending = false
      minecraft.seeds.release(S.detail_job)
      S.detail_job = nil
    end
  end
end

function search.preview_first()
  search.save_fields()
  search.stop_detail()
  local seed = minecraft.util.resolve_seed(minecraft.screen.field_text("seed_start") or "0")
  S.hits = {}
  S.hits[1] = { seed = seed, score = 0, unique = 0, origin_biome = minecraft.seeds.biome_at(seed, 0, 0) }
  S.selected_seed = nil
  search.select_index(1)
end

function search.copy_selected()
  local h = search.selected_hit()
  if h ~= nil then
    minecraft.seeds.copy(fmt_seed(h.seed))
    S.toast = "Copied " .. fmt_seed(h.seed)
    S.toast_until = (S.tick_count or 0) + 60
  end
end

function search.use_selected()
  local h = search.selected_hit()
  if h == nil then
    return
  end
  search.stop_job()
  search.stop_detail()
  search.free_map()
  minecraft.screen.open_host(minecraft.screen.ids.create_world, {
    world_name = S.world_name,
    seed = fmt_seed(h.seed),
  })
end

function search.go_back()
  search.save_fields()
  search.stop_job()
  search.stop_detail()
  search.free_map()
  minecraft.screen.open_host(minecraft.screen.ids.create_world, {
    world_name = S.world_name,
    seed = S.seed_text or "",
  })
end

function search.open_finder()
  S.hits = {}
  S.selected_seed = nil
  S.detail = nil
  S.progress = nil
  S.error = nil
  S.scroll = 0
  search.stop_job()
  search.stop_detail()
  search.free_map()
  minecraft.screen.open(S.screen_id)
end

return search
