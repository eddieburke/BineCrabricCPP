local places = {}

local ALL = {
  { name = "Greenwich", lat = 51.4779, lon = -0.0015, time_zone_id = "GMT" },
  { name = "New York", lat = 40.7128, lon = -74.0060, time_zone_id = "GMT-5" },
  { name = "Los Angeles", lat = 34.0522, lon = -118.2437, time_zone_id = "GMT-8" },
  { name = "Chicago", lat = 41.8781, lon = -87.6298, time_zone_id = "GMT-6" },
  { name = "São Paulo", lat = -23.5505, lon = -46.6333, time_zone_id = "GMT-3" },
  { name = "London", lat = 51.5074, lon = -0.1278, time_zone_id = "GMT" },
  { name = "Paris", lat = 48.8566, lon = 2.3522, time_zone_id = "GMT+1" },
  { name = "Berlin", lat = 52.5200, lon = 13.4050, time_zone_id = "GMT+1" },
  { name = "Cairo", lat = 30.0444, lon = 31.2357, time_zone_id = "GMT+2" },
  { name = "Moscow", lat = 55.7558, lon = 37.6173, time_zone_id = "GMT+3" },
  { name = "Dubai", lat = 25.2048, lon = 55.2708, time_zone_id = "GMT+4" },
  { name = "Mumbai", lat = 19.0760, lon = 72.8777, time_zone_id = "GMT+5:30" },
  { name = "Bangkok", lat = 13.7563, lon = 100.5018, time_zone_id = "GMT+7" },
  { name = "Singapore", lat = 1.3521, lon = 103.8198, time_zone_id = "GMT+8" },
  { name = "Hong Kong", lat = 22.3193, lon = 114.1694, time_zone_id = "GMT+8" },
  { name = "Tokyo", lat = 35.6762, lon = 139.6503, time_zone_id = "GMT+9" },
  { name = "Sydney", lat = -33.8688, lon = 151.2093, time_zone_id = "GMT+10" },
  { name = "Auckland", lat = -36.8485, lon = 174.7633, time_zone_id = "GMT+12" },
  { name = "Reykjavik", lat = 64.1466, lon = -21.9426, time_zone_id = "GMT" },
  { name = "Cape Town", lat = -33.9249, lon = 18.4241, time_zone_id = "GMT+2" },
}

function places.all()
  return ALL
end

function places.filter(query)
  query = tostring(query or ""):lower()
  if query == "" then
    return ALL
  end
  local out = {}
  for _, place in ipairs(ALL) do
    if place.name:lower():find(query, 1, true) then
      out[#out + 1] = place
    end
  end
  return out
end

return places
