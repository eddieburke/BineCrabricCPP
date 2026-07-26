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

local HORIZON_EPSILON = 0.006
local MIN_WORLD_SEGMENT_DOT = 0.82
local MIN_SCREEN_SEGMENT_SQ = 0.09

local DEFAULT_GLOBE_CAMERA = 2.05
local MIN_GLOBE_CAMERA = 0.025
local MAX_GLOBE_CAMERA = 8.0
local ZOOM_STEP = 0.72

local coastline_data = nil
local graticule_paths = nil

local projection_cache = {
  key = nil,
  coast = {},
  river = {},
  grid = {},
  equator = {},
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

local function append_polyline(target, polyline, center_x, center_y, radius,
    cyaw, syaw, cpitch, spitch, left, top, right, bottom)
  local previous_x, previous_y, previous_z
  local previous_vx, previous_vy, previous_vz

  for index = 1, #polyline do
    local point = polyline[index]
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

local function rebuild_projection_cache(data, center_x, center_y,
    radius, yaw_deg, pitch_deg, key, left, top, right, bottom)
  local yaw = yaw_deg * DEG
  local pitch = pitch_deg * DEG
  local cyaw, syaw = math.cos(yaw), math.sin(yaw)
  local cpitch, spitch = math.cos(pitch), math.sin(pitch)

  local coast, river, grid, equator = {}, {}, {}, {}

  for _, path in ipairs(graticule_paths) do
    append_polyline(path.color == EQUATOR_COLOR and equator or grid,
      path, center_x, center_y, radius, cyaw, syaw, cpitch, spitch,
      left, top, right, bottom)
  end

  for _, segment in ipairs(data.coasts) do
    local polyline = {}
    for i, coord in ipairs(segment) do
      local x, y, z = lat_lon_xyz(coord[1], coord[2])
      polyline[i] = { x = x, y = y, z = z }
    end
    append_polyline(coast, polyline, center_x, center_y, radius,
      cyaw, syaw, cpitch, spitch, left, top, right, bottom)
  end

  for _, segment in ipairs(data.rivers) do
    local polyline = {}
    for i, coord in ipairs(segment) do
      local x, y, z = lat_lon_xyz(coord[1], coord[2])
      polyline[i] = { x = x, y = y, z = z }
    end
    append_polyline(river, polyline, center_x, center_y, radius,
      cyaw, syaw, cpitch, spitch, left, top, right, bottom)
  end

  projection_cache.key = key
  projection_cache.coast = coast
  projection_cache.river = river
  projection_cache.grid = grid
  projection_cache.equator = equator
end

local function projection_parameters(ui, center_x, center_y, radius, size)
  local dragging = ui.dragging == true
  if not coastline_data then return nil end

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
    round(cached_center_x * 4.0),
    round(cached_center_y * 4.0),
    round(cached_radius * 4.0),
    round(yaw / angle_quantum),
    round(pitch / angle_quantum),
    dragging and 1 or 0,
    size,
  }, ":")

  return coastline_data, cached_center_x, cached_center_y,
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

local function draw_line_segment(x0, y0, x1, y1, color, thickness,
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
    draw_line_segment(segments[index], segments[index + 1],
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
          for seg = 0, segments - 1 do
            local x0 = inner_x0 + math.floor(span * seg / segments)
            local x1 = inner_x0 + math.floor(span * (seg + 1) / segments) - 1
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

function globe_ui.load_data()
  if coastline_data ~= nil then return end
  local raw = assert(minecraft.read_asset("assets/globe.json"),
    "realtime_sky: missing globe.json")
  local data, err = minecraft.util.json_decode(raw)
  assert(type(data) == "table" and type(data.coasts) == "table",
    "realtime_sky: invalid globe.json: " .. tostring(err))
  coastline_data = data
  minecraft.log("info", string.format(
    "globe loaded: %d coast segments, %d river segments",
    #data.coasts, #data.rivers))
end

function globe_ui.cleanup()
end

function globe_ui.draw(ui, width, height, pin_lat, pin_lon)
  ensure_geometry()
  local center_x, center_y, radius, size = globe_metrics(ui)
  local left, top, right, bottom = viewport_bounds(ui, size)
  local data, cached_center_x, cached_center_y,
    cached_radius, cached_yaw, cached_pitch, cache_key =
    projection_parameters(ui, center_x, center_y, radius, size)

  minecraft.gui.fill_rect(left, top, size, size, CLEAR_COLOR)

  if data then
    if projection_cache.key ~= cache_key then
      rebuild_projection_cache(data,
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
  local yaw = (tonumber(ui.globe_yaw) or 0.0) * DEG
  local pitch = (tonumber(ui.globe_pitch) or 0.0) * DEG
  local cy, sy = math.cos(yaw), math.sin(yaw)
  local cp, sp = math.cos(pitch), math.sin(pitch)

  local x1 = view_x
  local y2 = view_y * cp + view_z * sp
  local z1 = -view_y * sp + view_z * cp
  local x = x1 * cy - z1 * sy
  local z = x1 * sy + z1 * cy
  local y = y2

  local lat, lon = xyz_to_lat_lon(x, y, z)
  if not lat then return nil end
  return { lat = lat, lon = lon }
end

function globe_ui.contains_point(ui, mouse_x, mouse_y)
  local center_x, center_y, radius, viewport_size = globe_metrics(ui)
  local left, top, right, bottom = viewport_bounds(ui, viewport_size)
  if mouse_x < left or mouse_x > right or mouse_y < top or mouse_y > bottom then
    return false
  end
  local dx = mouse_x - center_x
  local dy = mouse_y - center_y
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

return globe_ui