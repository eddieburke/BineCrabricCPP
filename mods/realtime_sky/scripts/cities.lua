local cities = {}
local fallback_places = minecraft.require("scripts.places")
local all_places = nil
local load_error = nil

local function copy_fallback(reason)
  load_error = reason
  local out = {}
  for _, place in ipairs(fallback_places.all()) do
    out[#out + 1] = {
      name = place.name,
      country = place.country or "",
      lat = place.lat,
      lon = place.lon,
      time_zone_id = place.time_zone_id or "GMT",
    }
  end
  return out
end

local function load_database()
  if all_places ~= nil then return all_places end

  local raw = minecraft.read_asset("assets/cities.json")
  if raw == nil or raw == "" then
    all_places = copy_fallback("cities.json not found; using bundled fallback places")
    return all_places
  end

  local data, err = minecraft.util.json_decode(raw)
  if data == nil then
    all_places = copy_fallback("JSON parse error: " .. tostring(err))
    return all_places
  end

  local source = data.places or data.cities
  if type(source) ~= "table" then
    all_places = copy_fallback("missing 'places' or 'cities' array in cities.json")
    return all_places
  end

  all_places = {}
  for _, entry in ipairs(source) do
    local name = entry.name or entry.city
    local lat = entry.lat or entry.latitude
    local lon = entry.lon or entry.lng or entry.longitude
    local zone = entry.tz or entry.time_zone_id or entry.timeZoneId or entry.timezone
    if type(name) == "string" and type(lat) == "number" and type(lon) == "number" then
      all_places[#all_places + 1] = {
        name = name,
        country = entry.country or entry.country_name or "",
        lat = lat,
        lon = lon,
        time_zone_id = type(zone) == "string" and zone or "GMT",
      }
    end
  end

  if #all_places == 0 then
    all_places = copy_fallback("cities.json contained no valid city records")
    return all_places
  end

  table.sort(all_places, function(a, b)
    local an = a.name:lower()
    local bn = b.name:lower()
    if an == bn then return (a.country or "") < (b.country or "") end
    return an < bn
  end)
  load_error = nil
  return all_places
end

function cities.all()
  return load_database()
end

function cities.error()
  load_database()
  return load_error
end

function cities.filter(query)
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

function cities.count()
  return #load_database()
end

return cities
