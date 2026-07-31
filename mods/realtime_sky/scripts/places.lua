local places = {}
local all_places = nil

local DEG = math.pi / 180.0

local function load_database()
  if all_places ~= nil then return all_places end

  local raw = assert(minecraft.read_asset("assets/cities.json"), "realtime_sky: missing cities database")
  local data, err = minecraft.util.json_decode(raw)
  assert(type(data) == "table", "realtime_sky: invalid cities database: " .. tostring(err))
  assert(type(data.places) == "table", "realtime_sky: missing places array")

  all_places = {}
  for _, entry in ipairs(data.places) do
    assert(type(entry.name) == "string", "realtime_sky: city name must be a string")
    assert(type(entry.country) == "string", "realtime_sky: city country must be a string")
    assert(type(entry.lat) == "number" and type(entry.lon) == "number", "realtime_sky: city coordinates must be numbers")
    assert(type(entry.tz) == "string", "realtime_sky: city time zone must be a string")
    all_places[#all_places + 1] = {
      name = entry.name,
      country = entry.country,
      lat = entry.lat,
      lon = entry.lon,
      time_zone_id = entry.tz,
    }
  end
  assert(#all_places > 0, "realtime_sky: empty cities database")

  table.sort(all_places, function(a, b)
    local an = a.name:lower()
    local bn = b.name:lower()
    if an == bn then return (a.country or "") < (b.country or "") end
    return an < bn
  end)
  return all_places
end

local function haversine_radians(lat1, lon1, lat2, lon2)
  local lat1_rad = lat1 * DEG
  local lat2_rad = lat2 * DEG
  local dlat = (lat2 - lat1) * DEG
  local dlon = (lon2 - lon1) * DEG
  local sin_dlat = math.sin(dlat * 0.5)
  local sin_dlon = math.sin(dlon * 0.5)
  local a = sin_dlat * sin_dlat +
    math.cos(lat1_rad) * math.cos(lat2_rad) * sin_dlon * sin_dlon
  return 2.0 * math.asin(math.min(1.0, math.sqrt(a)))
end

function places.all()
  return load_database()
end

function places.filter(query)
  query = tostring(query or ""):lower()
  local db = load_database()
  if query == "" then return db end
  local out = {}
  for _, place in ipairs(db) do
    local name_match = place.name:lower():find(query, 1, true)
    local country_match = (place.country or ""):lower():find(query, 1, true)
    local zone_match = (place.time_zone_id or ""):lower():find(query, 1, true)
    if name_match or country_match or zone_match then
      out[#out + 1] = place
    end
  end
  return out
end

function places.count()
  return #load_database()
end

function places.nearest(lat, lon)
  lat = tonumber(lat)
  lon = tonumber(lon)
  assert(lat ~= nil and lon ~= nil, "realtime_sky: nearest requires numeric lat/lon")

  local db = load_database()
  local best_place = nil
  local best_distance = nil
  for _, place in ipairs(db) do
    local distance = haversine_radians(lat, lon, place.lat, place.lon)
    if best_distance == nil or distance < best_distance then
      best_distance = distance
      best_place = place
    end
  end
  return best_place
end

return places
