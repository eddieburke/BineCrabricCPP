local globe_ui = {}

local DEG = math.pi / 180.0
local RAD_TO_DEG = 180.0 / math.pi
local CLEAR_COLOR = 0xFF1B2129
local LIMB_COLOR = 0xFF09131C
local GRID_COLOR = 0xFF4F89AD
local EQUATOR_COLOR = 0xFF79B5DB
local COAST_COLOR = 0xFFE8F3FA
local RIVER_COLOR = 0xFF68B7E2
local PIN_BORDER_COLOR = 0xFF6A3608
local PIN_COLOR = 0xFFFFA52A
local PIN_CENTER_COLOR = 0xFFFFF1C4

local CHUNK_POINTS = 80
local HORIZON_EPSILON = 0.006
local MIN_WORLD_SEGMENT_DOT = 0.82
local MIN_SCREEN_SEGMENT_SQ = 0.09

-- Camera distance is inverse zoom. The original 1.5 lower clamp allowed only
-- a tiny enlargement. This range supports a full globe and an approximately
-- 82x close view without letting values reach zero or destabilize projection.
local DEFAULT_GLOBE_CAMERA = 2.05
local MIN_GLOBE_CAMERA = 0.025
local MAX_GLOBE_CAMERA = 8.0
local ZOOM_STEP = 0.72

local LOD_DEFS = {
  { name = "far",    asset = "assets/globe_lod_far.txt",    epsilon_deg = 0.900 },
  { name = "medium", asset = "assets/globe_lod_medium.txt", epsilon_deg = 0.380 },
  { name = "near",   asset = "assets/globe_lod_near.txt",   epsilon_deg = 0.150 },
  { name = "ultra",  asset = "assets/globe_lod_ultra.txt",  epsilon_deg = 0.055 },
}

local levels = {}
local fallback_text = nil
local graticule_paths = nil
local geometry_generation = 0

local projection_cache = {
  key = nil,
  generation = -1,
  lod = 0,
  coast = {},
  river = {},
  grid = {},
  equator = {},
  visible_chunks = 0,
  source_points = 0,
  rebuilds = 0,
}

local sphere_cache = {
  key = nil,
  rectangles = {},
}

local function clamp(value, low, high)
  if value < low then return low end
  if value > high then return high end
  return value
end

local function round(value)
  if value >= 0.0 then return math.floor(value + 0.5) end
  return math.ceil(value - 0.5)
end

local function quantize(value, step)
  if step <= 0.0 then return value end
  return round(value / step) * step
end

local function normalize_yaw(value)
  value = (tonumber(value) or 0.0) % 360.0
  if value > 180.0 then value = value - 360.0 end
  return value
end

local function argb(red, green, blue, alpha)
  red = math.floor(clamp(red, 0.0, 255.0) + 0.5)
  green = math.floor(clamp(green, 0.0, 255.0) + 0.5)
  blue = math.floor(clamp(blue, 0.0, 255.0) + 0.5)
  alpha = math.floor(clamp(alpha or 255.0, 0.0, 255.0) + 0.5)
  return alpha * 16777216 + red * 65536 + green * 256 + blue
end

local function lat_lon_xyz(lat_deg, lon_deg)
  local lat = lat_deg * DEG
  local lon = lon_deg * DEG
  local cos_lat = math.cos(lat)
  return cos_lat * math.sin(lon),
         math.sin(lat),
         cos_lat * math.cos(lon)
end

local function xyz_to_lat_lon(x, y, z)
  local length = math.sqrt(x * x + y * y + z * z)
  if length < 1.0e-8 then return nil end
  local lat = math.deg(math.asin(clamp(y / length, -1.0, 1.0)))
  local lon = math.deg(math.atan2(x, z))
  if lon > 180.0 then lon = lon - 360.0 end
  if lon < -180.0 then lon = lon + 360.0 end
  return lat, lon
end

local function make_point(lat, lon)
  local x, y, z = lat_lon_xyz(lat, lon)
  return { x = x, y = y, z = z }
end

local function finalize_chunk(level, coordinates, point_count, kind)
  if point_count < 2 then return end

  local sx, sy, sz = 0.0, 0.0, 0.0
  for index = 1, point_count * 3, 3 do
    sx = sx + coordinates[index]
    sy = sy + coordinates[index + 1]
    sz = sz + coordinates[index + 2]
  end

  local length = math.sqrt(sx * sx + sy * sy + sz * sz)
  local cx, cy, cz, margin
  if length < 1.0e-8 then
    cx, cy, cz, margin = 0.0, 0.0, 1.0, 1.0
  else
    cx, cy, cz = sx / length, sy / length, sz / length
    local minimum_dot = 1.0
    for index = 1, point_count * 3, 3 do
      local dot = cx * coordinates[index] +
        cy * coordinates[index + 1] + cz * coordinates[index + 2]
      if dot < minimum_dot then minimum_dot = dot end
    end
    if minimum_dot <= 0.0 then
      margin = 1.0
    else
      margin = math.sqrt(math.max(0.0, 1.0 - minimum_dot * minimum_dot))
    end
  end

  level.chunks[#level.chunks + 1] = {
    coordinates = coordinates,
    count = point_count,
    kind = kind,
    cx = cx,
    cy = cy,
    cz = cz,
    margin = margin,
  }
  level.points = level.points + point_count
end

local function parse_geometry_text(text, level_name)
  local level = {
    name = level_name,
    chunks = {},
    points = 0,
    paths = 0,
  }

  for segment in tostring(text or ""):gmatch("[^|]+") do
    local kind = segment:match("^%s*([CR])%s") or "C"
    local coordinates = {}
    local point_count = 0
    local previous_x, previous_y, previous_z
    local segment_has_points = false

    for lat_text, lon_text in segment:gmatch("([%+%-]?[%d%.]+),([%+%-]?[%d%.]+)") do
      local lat = tonumber(lat_text)
      local lon = tonumber(lon_text)
      if lat and lon and lat >= -90.0 and lat <= 90.0 and lon >= -180.0 and lon <= 180.0 then
        local x, y, z = lat_lon_xyz(lat, lon)
        if point_count >= CHUNK_POINTS then
          finalize_chunk(level, coordinates, point_count, kind)
          coordinates = { previous_x, previous_y, previous_z }
          point_count = 1
        end
        coordinates[#coordinates + 1] = x
        coordinates[#coordinates + 1] = y
        coordinates[#coordinates + 1] = z
        point_count = point_count + 1
        previous_x, previous_y, previous_z = x, y, z
        segment_has_points = true
      end
    end

    if point_count >= 2 then
      finalize_chunk(level, coordinates, point_count, kind)
    end
    if segment_has_points then level.paths = level.paths + 1 end
  end

  return level
end

local function log_level(level, asset)
  if minecraft.log then
    minecraft.log("info", string.format(
      "globe LOD %s loaded: %d points in %d chunks from %s",
      level.name, level.points, #level.chunks, asset or "fallback"))
  end
end

local function read_level(index)
  if levels[index] ~= nil then return levels[index] end
  local definition = LOD_DEFS[index]
  if not definition then return nil end

  local ok, text = pcall(minecraft.read_asset, definition.asset)
  if ok and text and text ~= "" then
    local level = parse_geometry_text(text, definition.name)
    if level.points > 0 then
      levels[index] = level
      log_level(level, definition.asset)
      return level
    end
  end
  return nil
end

local function nearest_level(index)
  local level = read_level(index)
  if level then return level, index end

  for distance = 1, #LOD_DEFS do
    local lower = index - distance
    if lower >= 1 then
      level = read_level(lower)
      if level then return level, lower end
    end
    local higher = index + distance
    if higher <= #LOD_DEFS then
      level = read_level(higher)
      if level then return level, higher end
    end
  end

  if fallback_text and fallback_text ~= "" then
    level = parse_geometry_text(fallback_text, "fallback")
    fallback_text = nil
    if level.points > 0 then
      levels[2] = level
      log_level(level, nil)
      return level, 2
    end
  end
  return nil, 0
end

local function build_graticule()
  local paths = {}
  for lat = -80, 80, 20 do
    local path = { color = lat == 0 and EQUATOR_COLOR or GRID_COLOR }
    for lon = -180, 180, 4 do
      path[#path + 1] = make_point(lat, lon)
    end
    paths[#paths + 1] = path
  end
  for lon = -180, 150, 30 do
    local path = { color = GRID_COLOR }
    for lat = -88, 88, 4 do
      path[#path + 1] = make_point(lat, lon)
    end
    paths[#paths + 1] = path
  end
  return paths
end

local function ensure_geometry()
  if graticule_paths == nil then graticule_paths = build_graticule() end
end

local function globe_metrics(ui)
  local size = math.max(1, math.floor(ui.globe_size or 1))
  local camera = clamp(tonumber(ui.globe_cam) or DEFAULT_GLOBE_CAMERA,
    MIN_GLOBE_CAMERA, MAX_GLOBE_CAMERA)
  local zoom = DEFAULT_GLOBE_CAMERA / camera
  local radius = math.max(8.0, size * 0.455 * zoom)
  return ui.globe_x + size * 0.5,
         ui.globe_y + size * 0.5,
         radius,
         size
end

local function viewport_bounds(ui, size)
  local left = math.floor(ui.globe_x or 0)
  local top = math.floor(ui.globe_y or 0)
  return left, top, left + size - 1, top + size - 1
end

local function rotate_from_view(x1, y2, z2, yaw_deg, pitch_deg)
  local yaw = (tonumber(yaw_deg) or 0.0) * DEG
  local pitch = (tonumber(pitch_deg) or 0.0) * DEG
  local cy, sy = math.cos(yaw), math.sin(yaw)
  local cp, sp = math.cos(pitch), math.sin(pitch)

  local y = y2 * cp - z2 * sp
  local z1 = y2 * sp + z2 * cp
  local x = x1 * cy - z1 * sy
  local z = x1 * sy + z1 * cy
  return x, y, z
end

local function choose_lod(radius, dragging)
  -- The threshold is expressed in screen pixels.  Moving uses a cheaper level;
  -- once input stops, the same draw loop upgrades to the settled detail level.
  local pixel_error = dragging and 5.40 or 1.80
  local allowed_error_deg = pixel_error * RAD_TO_DEG / math.max(radius, 1.0)
  for index = 1, #LOD_DEFS do
    if LOD_DEFS[index].epsilon_deg <= allowed_error_deg then
      return index
    end
  end
  return #LOD_DEFS
end

local function clip_segment(x0, y0, x1, y1, left, top, right, bottom)
  local dx = x1 - x0
  local dy = y1 - y0
  local t0, t1 = 0.0, 1.0

  local function clip(p, q)
    if math.abs(p) < 1.0e-12 then return q >= 0.0 end
    local r = q / p
    if p < 0.0 then
      if r > t1 then return false end
      if r > t0 then t0 = r end
    else
      if r < t0 then return false end
      if r < t1 then t1 = r end
    end
    return true
  end

  if not clip(-dx, x0 - left) or
      not clip(dx, right - x0) or
      not clip(-dy, y0 - top) or
      not clip(dy, bottom - y0) then
    return nil
  end

  return x0 + dx * t0, y0 + dy * t0,
         x0 + dx * t1, y0 + dy * t1
end

local function append_segment(target, x0, y0, x1, y1,
    left, top, right, bottom)
  x0, y0, x1, y1 = clip_segment(x0, y0, x1, y1,
    left, top, right, bottom)
  if x0 == nil then return end
  local dx = x1 - x0
  local dy = y1 - y0
  if dx * dx + dy * dy < MIN_SCREEN_SEGMENT_SQ then return end
  target[#target + 1] = x0
  target[#target + 1] = y0
  target[#target + 1] = x1
  target[#target + 1] = y1
end

local function append_world_path(target, path, center_x, center_y, radius,
    cyaw, syaw, cpitch, spitch, left, top, right, bottom)
  local previous_x, previous_y, previous_z
  local previous_vx, previous_vy, previous_vz

  for index = 1, #path do
    local point = path[index]
    local x, y, z = point.x, point.y, point.z
    local vx = x * cyaw + z * syaw
    local z1 = -x * syaw + z * cyaw
    local vy = y * cpitch + z1 * spitch
    local vz = -y * spitch + z1 * cpitch

    if previous_x then
      local world_dot = previous_x * x + previous_y * y + previous_z * z
      if world_dot > MIN_WORLD_SEGMENT_DOT and
          (previous_vz > HORIZON_EPSILON or vz > HORIZON_EPSILON) then
        local ax, ay, az = previous_vx, previous_vy, previous_vz
        local bx, by, bz = vx, vy, vz
        if az <= HORIZON_EPSILON or bz <= HORIZON_EPSILON then
          local denominator = bz - az
          if math.abs(denominator) > 1.0e-8 then
            local t = (HORIZON_EPSILON - az) / denominator
            local ix = ax + (bx - ax) * t
            local iy = ay + (by - ay) * t
            if az <= HORIZON_EPSILON then
              ax, ay, az = ix, iy, HORIZON_EPSILON
            else
              bx, by, bz = ix, iy, HORIZON_EPSILON
            end
          end
        end
        append_segment(target,
          center_x + ax * radius, center_y - ay * radius,
          center_x + bx * radius, center_y - by * radius,
          left, top, right, bottom)
      end
    end

    previous_x, previous_y, previous_z = x, y, z
    previous_vx, previous_vy, previous_vz = vx, vy, vz
  end
end

local function append_chunk(target, chunk, center_x, center_y, radius,
    cyaw, syaw, cpitch, spitch, left, top, right, bottom)
  local coordinates = chunk.coordinates
  local previous_x, previous_y, previous_z
  local previous_vx, previous_vy, previous_vz

  for index = 1, chunk.count * 3, 3 do
    local x = coordinates[index]
    local y = coordinates[index + 1]
    local z = coordinates[index + 2]
    local vx = x * cyaw + z * syaw
    local z1 = -x * syaw + z * cyaw
    local vy = y * cpitch + z1 * spitch
    local vz = -y * spitch + z1 * cpitch

    if previous_x then
      local world_dot = previous_x * x + previous_y * y + previous_z * z
      if world_dot > MIN_WORLD_SEGMENT_DOT and
          (previous_vz > HORIZON_EPSILON or vz > HORIZON_EPSILON) then
        local ax, ay, az = previous_vx, previous_vy, previous_vz
        local bx, by, bz = vx, vy, vz
        if az <= HORIZON_EPSILON or bz <= HORIZON_EPSILON then
          local denominator = bz - az
          if math.abs(denominator) > 1.0e-8 then
            local t = (HORIZON_EPSILON - az) / denominator
            local ix = ax + (bx - ax) * t
            local iy = ay + (by - ay) * t
            if az <= HORIZON_EPSILON then
              ax, ay, az = ix, iy, HORIZON_EPSILON
            else
              bx, by, bz = ix, iy, HORIZON_EPSILON
            end
          end
        end
        append_segment(target,
          center_x + ax * radius, center_y - ay * radius,
          center_x + bx * radius, center_y - by * radius,
          left, top, right, bottom)
      end
    end

    previous_x, previous_y, previous_z = x, y, z
    previous_vx, previous_vy, previous_vz = vx, vy, vz
  end
end

local function rebuild_projection_cache(level, level_index, center_x, center_y,
    radius, yaw_deg, pitch_deg, key, left, top, right, bottom)
  local yaw = yaw_deg * DEG
  local pitch = pitch_deg * DEG
  local cyaw, syaw = math.cos(yaw), math.sin(yaw)
  local cpitch, spitch = math.cos(pitch), math.sin(pitch)

  local coast, river, grid, equator = {}, {}, {}, {}
  local visible_chunks = 0
  local source_points = 0

  for _, path in ipairs(graticule_paths) do
    append_world_path(path.color == EQUATOR_COLOR and equator or grid,
      path, center_x, center_y, radius, cyaw, syaw, cpitch, spitch,
      left, top, right, bottom)
  end

  for _, chunk in ipairs(level.chunks) do
    -- Transform the chunk bound once. At close zoom this rejects nearly all of
    -- the far side and all front-side chunks whose projected bound misses the
    -- cropped viewport, instead of projecting an entire hemisphere.
    local cvx = chunk.cx * cyaw + chunk.cz * syaw
    local cz1 = -chunk.cx * syaw + chunk.cz * cyaw
    local cvy = chunk.cy * cpitch + cz1 * spitch
    local cvz = -chunk.cy * spitch + cz1 * cpitch
    if cvz > -chunk.margin - 0.025 then
      local screen_margin = chunk.margin * radius + 3.0
      local screen_x = center_x + cvx * radius
      local screen_y = center_y - cvy * radius
      if screen_x + screen_margin >= left and
          screen_x - screen_margin <= right and
          screen_y + screen_margin >= top and
          screen_y - screen_margin <= bottom then
        visible_chunks = visible_chunks + 1
        source_points = source_points + chunk.count
        append_chunk(chunk.kind == "R" and river or coast,
          chunk, center_x, center_y, radius, cyaw, syaw, cpitch, spitch,
          left, top, right, bottom)
      end
    end
  end

  projection_cache.key = key
  projection_cache.generation = geometry_generation
  projection_cache.lod = level_index
  projection_cache.coast = coast
  projection_cache.river = river
  projection_cache.grid = grid
  projection_cache.equator = equator
  projection_cache.visible_chunks = visible_chunks
  projection_cache.source_points = source_points
  projection_cache.rebuilds = projection_cache.rebuilds + 1
end

local function projection_parameters(ui, center_x, center_y, radius, size)
  local dragging = ui.dragging == true
  local requested_lod = choose_lod(radius, dragging)
  local level, actual_lod = nearest_level(requested_lod)
  if not level then return nil end

  local pixel_angle_deg = RAD_TO_DEG / math.max(radius, 1.0)
  local angle_quantum = math.max(0.018,
    pixel_angle_deg * (dragging and 0.70 or 0.24))
  local radius_quantum = dragging and 0.75 or 0.25

  local yaw = quantize(normalize_yaw(ui.globe_yaw), angle_quantum)
  local pitch = quantize(clamp(tonumber(ui.globe_pitch) or 0.0, -89.0, 89.0), angle_quantum)
  local cached_radius = quantize(radius, radius_quantum)
  local cached_center_x = quantize(center_x, 0.25)
  local cached_center_y = quantize(center_y, 0.25)

  local key = table.concat({
    geometry_generation,
    actual_lod,
    round(cached_center_x * 4.0),
    round(cached_center_y * 4.0),
    round(cached_radius * 4.0),
    round(yaw / angle_quantum),
    round(pitch / angle_quantum),
    dragging and 1 or 0,
    size,
  }, ":")

  return level, actual_lod, cached_center_x, cached_center_y,
    cached_radius, yaw, pitch, key
end

local function fill_rect_clipped(x, y, width, height, color,
    left, top, right, bottom)
  local x0 = math.max(math.floor(x), left)
  local y0 = math.max(math.floor(y), top)
  local x1 = math.min(math.floor(x + width - 1), right)
  local y1 = math.min(math.floor(y + height - 1), bottom)
  if x1 < x0 or y1 < y0 then return end
  minecraft.gui.fill_rect(x0, y0, x1 - x0 + 1, y1 - y0 + 1, color)
end

local function draw_line(x0, y0, x1, y1, color, thickness,
    left, top, right, bottom)
  x0, y0, x1, y1 = clip_segment(x0, y0, x1, y1,
    left, top, right, bottom)
  if x0 == nil then return end
  local dx = x1 - x0
  local dy = y1 - y0
  local distance = math.max(math.abs(dx), math.abs(dy))
  local steps = math.max(1, math.ceil(distance / 1.40))
  local size = thickness or 1
  local offset = math.floor(size * 0.5)
  for step = 0, steps do
    local t = step / steps
    local x = round(x0 + dx * t) - offset
    local y = round(y0 + dy * t) - offset
    fill_rect_clipped(x, y, size, size, color, left, top, right, bottom)
  end
end

local function draw_segments(segments, color, thickness,
    left, top, right, bottom)
  for index = 1, #segments, 4 do
    draw_line(segments[index], segments[index + 1],
      segments[index + 2], segments[index + 3], color, thickness,
      left, top, right, bottom)
  end
end

local function build_sphere_cache(radius, size)
  local key = tostring(round(radius * 4.0)) .. ":" .. tostring(size)
  if sphere_cache.key == key then return end

  local rectangles = {}
  local center = size * 0.5
  local outer_radius = math.max(1.0, radius + 1.5)
  local inner_radius = math.max(1.0, radius - 0.5)
  local row_step = radius >= 120.0 and 2 or 1
  local segment_count = radius >= size * 2.0 and 9 or
    (radius >= 150.0 and 11 or 9)

  -- Only generate rows inside the viewport. Runtime work is O(viewport area),
  -- not O(the enormous off-screen globe diameter) at extreme zoom.
  for local_y = 0, size - 1, row_step do
    local dy = local_y - center
    if math.abs(dy) <= outer_radius then
      local outer_half = math.sqrt(math.max(0.0,
        outer_radius * outer_radius - dy * dy))
      local outer_x0 = math.max(0, math.ceil(center - outer_half))
      local outer_x1 = math.min(size - 1, math.floor(center + outer_half))
      if outer_x1 >= outer_x0 then
        rectangles[#rectangles + 1] = {
          x = outer_x0, y = local_y,
          width = outer_x1 - outer_x0 + 1, height = row_step,
          color = LIMB_COLOR,
        }
      end

      if math.abs(dy) <= inner_radius then
        local view_y = -dy / inner_radius
        local inner_half = math.sqrt(math.max(0.0,
          inner_radius * inner_radius - dy * dy))
        local inner_x0 = math.max(0, math.ceil(center - inner_half))
        local inner_x1 = math.min(size - 1, math.floor(center + inner_half))
        local span = inner_x1 - inner_x0 + 1
        if span > 0 then
          local segments = math.min(segment_count, span)
          for segment = 0, segments - 1 do
            local x0 = inner_x0 + math.floor(span * segment / segments)
            local x1 = inner_x0 + math.floor(span * (segment + 1) / segments) - 1
            if x1 >= x0 then
              local view_x = (((x0 + x1) * 0.5) - center) / inner_radius
              local view_z = math.sqrt(math.max(0.0,
                1.0 - view_x * view_x - view_y * view_y))
              local light = clamp(0.72 + 0.23 *
                (-0.38 * view_x + 0.48 * view_y + 0.78 * view_z), 0.62, 1.0)
              local rim = 0.80 + 0.20 * view_z
              rectangles[#rectangles + 1] = {
                x = x0, y = local_y,
                width = x1 - x0 + 1, height = row_step,
                color = argb(28.0 * light * rim,
                  92.0 * light * rim,
                  137.0 * light * rim + 8.0,
                  255),
              }
            end
          end
        end
      end
    end
  end

  sphere_cache.key = key
  sphere_cache.rectangles = rectangles
end

local function draw_sphere(ui, radius, size, left, top, right, bottom)
  build_sphere_cache(radius, size)
  for _, rectangle in ipairs(sphere_cache.rectangles) do
    fill_rect_clipped(left + rectangle.x, top + rectangle.y,
      rectangle.width, rectangle.height, rectangle.color,
      left, top, right, bottom)
  end
end

local function project_point(ui, center_x, center_y, radius, point)
  local yaw = (tonumber(ui.globe_yaw) or 0.0) * DEG
  local pitch = (tonumber(ui.globe_pitch) or 0.0) * DEG
  local cy, sy = math.cos(yaw), math.sin(yaw)
  local cp, sp = math.cos(pitch), math.sin(pitch)
  local x1 = point.x * cy + point.z * sy
  local z1 = -point.x * sy + point.z * cy
  local y2 = point.y * cp + z1 * sp
  local z2 = -point.y * sp + z1 * cp
  return center_x + x1 * radius, center_y - y2 * radius, z2
end

local function draw_pin(ui, center_x, center_y, radius, lat, lon,
    left, top, right, bottom)
  local point = make_point(tonumber(lat) or 0.0, tonumber(lon) or 0.0)
  local x, y, z = project_point(ui, center_x, center_y, radius, point)
  if z <= 0.0 or x < left - 5 or x > right + 5 or
      y < top - 5 or y > bottom + 5 then return end
  local px, py = round(x), round(y)
  fill_rect_clipped(px - 4, py - 4, 9, 9, PIN_BORDER_COLOR,
    left, top, right, bottom)
  fill_rect_clipped(px - 3, py - 3, 7, 7, PIN_COLOR,
    left, top, right, bottom)
  fill_rect_clipped(px - 1, py - 1, 3, 3, PIN_CENTER_COLOR,
    left, top, right, bottom)
end

function globe_ui.load_coastlines(text)
  levels = {}
  fallback_text = text
  geometry_generation = geometry_generation + 1
  projection_cache.key = nil
  projection_cache.generation = -1

  -- Keep startup bounded. Far/medium/near are only ~4.5 MB together. Ultra is
  -- loaded lazily only when the globe is large enough for its detail to matter.
  read_level(1)
  read_level(2)
  read_level(3)

  if levels[1] or levels[2] or levels[3] then
    fallback_text = nil
  end
end

function globe_ui.cleanup()
  -- Retain parsed LOD geometry and projected caches between screen openings.
end

function globe_ui.viewport_opts(ui, width, height)
  return {
    x = ui.globe_x,
    y = ui.globe_y,
    width = ui.globe_size,
    height = ui.globe_size,
    gui_width = width,
    gui_height = height,
    yaw_deg = ui.globe_yaw,
    pitch_deg = ui.globe_pitch,
    distance = clamp(tonumber(ui.globe_cam) or DEFAULT_GLOBE_CAMERA,
      MIN_GLOBE_CAMERA, MAX_GLOBE_CAMERA),
    fov_deg = 40.0,
    clear_color = CLEAR_COLOR,
  }
end

function globe_ui.draw(ui, width, height, pin_lat, pin_lon)
  ensure_geometry()
  local center_x, center_y, radius, size = globe_metrics(ui)
  local left, top, right, bottom = viewport_bounds(ui, size)
  local level, level_index, cached_center_x, cached_center_y,
    cached_radius, cached_yaw, cached_pitch, cache_key =
    projection_parameters(ui, center_x, center_y, radius, size)

  -- The panel is the viewport. Every primitive is explicitly clipped because
  -- this GUI API does not guarantee a hardware scissor rectangle.
  minecraft.gui.fill_rect(left, top, size, size, CLEAR_COLOR)

  if level then
    if projection_cache.key ~= cache_key or
        projection_cache.generation ~= geometry_generation then
      rebuild_projection_cache(level, level_index,
        cached_center_x, cached_center_y, cached_radius,
        cached_yaw, cached_pitch, cache_key,
        left, top, right, bottom)
    end
    center_x, center_y, radius = cached_center_x, cached_center_y, cached_radius
  end

  draw_sphere(ui, radius, size, left, top, right, bottom)
  draw_segments(projection_cache.grid, GRID_COLOR, 1, left, top, right, bottom)
  draw_segments(projection_cache.equator, EQUATOR_COLOR, 2, left, top, right, bottom)
  draw_segments(projection_cache.river, RIVER_COLOR, 1, left, top, right, bottom)
  draw_segments(projection_cache.coast, COAST_COLOR,
    radius >= 150.0 and 2 or 1, left, top, right, bottom)
  draw_pin(ui, center_x, center_y, radius, pin_lat, pin_lon,
    left, top, right, bottom)
end

function globe_ui.pick_lat_lon(ui, width, height, mouse_x, mouse_y)
  local center_x, center_y, radius, size = globe_metrics(ui)
  local left, top, right, bottom = viewport_bounds(ui, size)
  if mouse_x < left or mouse_x > right or mouse_y < top or mouse_y > bottom then
    return nil
  end
  local view_x = (mouse_x - center_x) / radius
  local view_y = -(mouse_y - center_y) / radius
  local radius_squared = view_x * view_x + view_y * view_y
  if radius_squared > 1.0 then return nil end

  local view_z = math.sqrt(math.max(0.0, 1.0 - radius_squared))
  local x, y, z = rotate_from_view(
    view_x, view_y, view_z, ui.globe_yaw, ui.globe_pitch)
  local lat, lon = xyz_to_lat_lon(x, y, z)
  if not lat then return nil end
  return { lat = lat, lon = lon }
end

function globe_ui.contains_point(ui_or_mouse_x, mouse_x_or_y, maybe_y, y, size)
  if type(ui_or_mouse_x) == "table" then
    local ui = ui_or_mouse_x
    local mouse_x = mouse_x_or_y
    local mouse_y = maybe_y
    local center_x, center_y, radius, viewport_size = globe_metrics(ui)
    local left, top, right, bottom = viewport_bounds(ui, viewport_size)
    if mouse_x < left or mouse_x > right or mouse_y < top or mouse_y > bottom then
      return false
    end
    local dx = mouse_x - center_x
    local dy = mouse_y - center_y
    return dx * dx + dy * dy <= radius * radius
  end

  -- Backward-compatible old signature: mouse_x, mouse_y, x, y, size.
  local old_mouse_x, old_mouse_y = ui_or_mouse_x, mouse_x_or_y
  local x = maybe_y
  local center_x = x + size * 0.5
  local center_y = y + size * 0.5
  local dx = old_mouse_x - center_x
  local dy = old_mouse_y - center_y
  local radius = size * 0.49
  return dx * dx + dy * dy <= radius * radius
end

function globe_ui.zoom(ui, wheel_delta)
  local camera = clamp(tonumber(ui.globe_cam) or DEFAULT_GLOBE_CAMERA,
    MIN_GLOBE_CAMERA, MAX_GLOBE_CAMERA)
  if (tonumber(wheel_delta) or 0.0) > 0.0 then
    camera = camera * ZOOM_STEP
  else
    camera = camera / ZOOM_STEP
  end
  ui.globe_cam = clamp(camera, MIN_GLOBE_CAMERA, MAX_GLOBE_CAMERA)
  return ui.globe_cam
end

function globe_ui.zoom_factor(ui)
  local camera = clamp(tonumber(ui.globe_cam) or DEFAULT_GLOBE_CAMERA,
    MIN_GLOBE_CAMERA, MAX_GLOBE_CAMERA)
  return DEFAULT_GLOBE_CAMERA / camera
end

function globe_ui.drag_degrees_per_pixel(ui)
  local _, _, radius = globe_metrics(ui)
  return clamp(RAD_TO_DEG / math.max(radius, 1.0) * 1.25, 0.0025, 1.25)
end


function globe_ui.stats()
  local definition = LOD_DEFS[projection_cache.lod]
  return {
    lod = definition and definition.name or "none",
    visible_chunks = projection_cache.visible_chunks,
    projected_source_points = projection_cache.source_points,
    coast_segments = math.floor(#projection_cache.coast / 4),
    river_segments = math.floor(#projection_cache.river / 4),
    projection_rebuilds = projection_cache.rebuilds,
  }
end

return globe_ui
