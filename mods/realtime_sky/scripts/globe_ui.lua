local globe_ui = {}

local DEG = math.pi / 180.0
local SPHERE_RADIUS = 0.99
local COAST_RADIUS = 1.014
local GRID_RADIUS = 1.0
local PIN_RADIUS = 1.034

local coast_paths = {}
local coast_vertices = {}
local sphere_strips = nil
local graticule_primitives = nil

local function lat_lon_xyz(lat_deg, lon_deg, radius)
  local lat = lat_deg * DEG
  local lon = lon_deg * DEG
  local cos_lat = math.cos(lat)
  return cos_lat * math.sin(lon) * radius,
         math.sin(lat) * radius,
         cos_lat * math.cos(lon) * radius
end

local function parse_coast_text(text)
  local paths = {}
  for segment in text:gmatch("[^\r\n|]+") do
    local path = {}
    for token in segment:gmatch("%S+") do
      local lat_s, lon_s = token:match("([^,]+),([^,]+)")
      if lat_s and lon_s then
        path[#path + 1] = tonumber(lat_s)
        path[#path + 1] = tonumber(lon_s)
      end
    end
    if #path >= 4 then
      paths[#paths + 1] = path
    end
  end
  return paths
end

local function path_to_vertices(path, radius)
  local vertices = {}
  for index = 1, #path - 1, 2 do
    local x, y, z = lat_lon_xyz(path[index], path[index + 1], radius)
    vertices[#vertices + 1] = { x = x, y = y, z = z }
  end
  return vertices
end

local function build_sphere_strips()
  local strips = {}
  for lat = -90, 80, 10 do
    local vertices = {}
    for lon = 0, 360, 10 do
      local x1, y1, z1 = lat_lon_xyz(lat, lon, SPHERE_RADIUS)
      local x2, y2, z2 = lat_lon_xyz(lat + 10, lon, SPHERE_RADIUS)
      vertices[#vertices + 1] = { x = x1, y = y1, z = z1 }
      vertices[#vertices + 1] = { x = x2, y = y2, z = z2 }
    end
    strips[#strips + 1] = vertices
  end
  return strips
end

local function build_graticule()
  local primitives = {}
  for lat = -80, 80, 20 do
    local vertices = {}
    for lon = 0, 360, 5 do
      local x, y, z = lat_lon_xyz(lat, lon, GRID_RADIUS)
      vertices[#vertices + 1] = { x = x, y = y, z = z }
    end
    local color = (lat == 0) and 0xFF79B5DB or 0xFF3E7196
    primitives[#primitives + 1] = { mode = "line_loop", color = color, vertices = vertices }
  end
  for lon = 0, 330, 30 do
    local vertices = {}
    for lat = -90, 90, 5 do
      local x, y, z = lat_lon_xyz(lat, lon, GRID_RADIUS)
      vertices[#vertices + 1] = { x = x, y = y, z = z }
    end
    primitives[#primitives + 1] = { mode = "line_strip", color = 0xFF3E7196, vertices = vertices }
  end
  return primitives
end

local function ensure_geometry()
  if sphere_strips == nil then
    sphere_strips = build_sphere_strips()
  end
  if graticule_primitives == nil then
    graticule_primitives = build_graticule()
  end
end

local function ray_hit_sphere(ox, oy, oz, dx, dy, dz, radius)
  local len = math.sqrt(dx * dx + dy * dy + dz * dz)
  if len < 1.0e-8 then
    return nil
  end
  dx = dx / len
  dy = dy / len
  dz = dz / len
  local b = 2.0 * (ox * dx + oy * dy + oz * dz)
  local c = ox * ox + oy * oy + oz * oz - radius * radius
  local discriminant = b * b - 4.0 * c
  if discriminant < 0.0 then
    return nil
  end
  local sqrt_disc = math.sqrt(discriminant)
  local t = (-b - sqrt_disc) * 0.5
  if t < 0.0 then
    t = (-b + sqrt_disc) * 0.5
  end
  if t < 0.0 then
    return nil
  end
  return ox + dx * t, oy + dy * t, oz + dz * t
end

local function xyz_to_lat_lon(x, y, z)
  local len = math.sqrt(x * x + y * y + z * z)
  if len < 1.0e-8 then
    return nil
  end
  local lat = math.deg(math.asin(y / len))
  if lat > 90.0 then
    lat = 90.0
  elseif lat < -90.0 then
    lat = -90.0
  end
  local lon = math.deg(math.atan2(x, z))
  if lon > 180.0 then
    lon = lon - 360.0
  elseif lon < -180.0 then
    lon = lon + 360.0
  end
  return lat, lon
end

function globe_ui.load_coastlines(text)
  coast_paths = parse_coast_text(text or "")
  coast_vertices = {}
  for index, path in ipairs(coast_paths) do
    coast_vertices[index] = path_to_vertices(path, COAST_RADIUS)
  end
end

function globe_ui.cleanup()
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
    distance = ui.globe_cam,
    fov_deg = 40.0,
    clear_color = 0xFF1B2129,
  }
end

function globe_ui.draw(ui, width, height, pin_lat, pin_lon)
  ensure_geometry()
  local opts = globe_ui.viewport_opts(ui, width, height)
  minecraft.gui.begin_3d(opts)
  for _, vertices in ipairs(sphere_strips) do
    minecraft.gui.draw_3d({
      mode = "quad_strip",
      color = 0xFF173A57,
      vertices = vertices,
    })
  end
  for _, primitive in ipairs(graticule_primitives) do
    minecraft.gui.draw_3d(primitive)
  end
  for _, vertices in ipairs(coast_vertices) do
    minecraft.gui.draw_3d({
      mode = "line_strip",
      color = 0xFFE1EDF7,
      vertices = vertices,
    })
  end
  local px, py, pz = lat_lon_xyz(pin_lat, pin_lon, PIN_RADIUS)
  minecraft.gui.draw_3d({
    mode = "points",
    color = 0xFFFA9E1F,
    point_size = 6.0,
    vertices = { { x = px, y = py, z = pz } },
  })
  local sx, sy, sz = lat_lon_xyz(pin_lat, pin_lon, 1.16)
  minecraft.gui.draw_3d({
    mode = "lines",
    color = 0xFFE67314,
    vertices = {
      { x = 0.0, y = 0.0, z = 0.0 },
      { x = sx, y = sy, z = sz },
    },
  })
  minecraft.gui.end_3d()
end

function globe_ui.pick_lat_lon(ui, width, height, mouse_x, mouse_y)
  local opts = globe_ui.viewport_opts(ui, width, height)
  opts.mouse_x = mouse_x
  opts.mouse_y = mouse_y
  local ray = minecraft.gui.unproject(opts)
  if not ray then
    return nil
  end
  local hx, hy, hz = ray_hit_sphere(
    ray.origin.x, ray.origin.y, ray.origin.z,
    ray.direction.x, ray.direction.y, ray.direction.z,
    SPHERE_RADIUS)
  if not hx then
    return nil
  end
  local lat, lon = xyz_to_lat_lon(hx, hy, hz)
  if not lat then
    return nil
  end
  return { lat = lat, lon = lon }
end

function globe_ui.contains_point(mouse_x, mouse_y, x, y, size)
  local center_x = x + size / 2
  local center_y = y + size / 2
  local dx = mouse_x - center_x
  local dy = mouse_y - center_y
  local radius = size / 2
  return dx * dx + dy * dy <= radius * radius
end

return globe_ui
