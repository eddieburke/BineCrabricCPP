local places = {}

local function add(name, lat, lon, time_zone_id)
  places[#places + 1] = {
    name = name,
    lat = lat,
    lon = lon,
    time_zone_id = time_zone_id or "GMT",
    search = string.lower(name .. " " .. (time_zone_id or "")),
  }
end

add("New York", 40.7128, -74.0060, "America/New_York")
add("Los Angeles", 34.0522, -118.2437, "America/Los_Angeles")
add("Chicago", 41.8781, -87.6298, "America/Chicago")
add("Toronto", 43.6532, -79.3832, "America/Toronto")
add("Mexico City", 19.4326, -99.1332, "America/Mexico_City")
add("Sao Paulo", -23.5505, -46.6333, "America/Sao_Paulo")
add("Buenos Aires", -34.6037, -58.3816, "America/Argentina/Buenos_Aires")
add("Lima", -12.0464, -77.0428, "America/Lima")
add("Bogota", 4.7110, -74.0721, "America/Bogota")
add("Reykjavik", 64.1466, -21.9426, "Atlantic/Reykjavik")

add("London", 51.5074, -0.1278, "Europe/London")
add("Paris", 48.8566, 2.3522, "Europe/Paris")
add("Berlin", 52.5200, 13.4050, "Europe/Berlin")
add("Madrid", 40.4168, -3.7038, "Europe/Madrid")
add("Rome", 41.9028, 12.4964, "Europe/Rome")
add("Athens", 37.9838, 23.7275, "Europe/Athens")
add("Moscow", 55.7558, 37.6173, "Europe/Moscow")
add("Istanbul", 41.0082, 28.9784, "Europe/Istanbul")
add("Cairo", 30.0444, 31.2357, "Africa/Cairo")
add("Johannesburg", -26.2041, 28.0473, "Africa/Johannesburg")
add("Nairobi", -1.2921, 36.8219, "Africa/Nairobi")
add("Lagos", 6.5244, 3.3792, "Africa/Lagos")
add("Casablanca", 33.5731, -7.5898, "Africa/Casablanca")

add("Dubai", 25.2048, 55.2708, "Asia/Dubai")
add("Mumbai", 19.0760, 72.8777, "Asia/Kolkata")
add("Delhi", 28.6139, 77.2090, "Asia/Kolkata")
add("Bangkok", 13.7563, 100.5018, "Asia/Bangkok")
add("Singapore", 1.3521, 103.8198, "Asia/Singapore")
add("Hong Kong", 22.3193, 114.1694, "Asia/Hong_Kong")
add("Shanghai", 31.2304, 121.4737, "Asia/Shanghai")
add("Tokyo", 35.6762, 139.6503, "Asia/Tokyo")
add("Seoul", 37.5665, 126.9780, "Asia/Seoul")
add("Beijing", 39.9042, 116.4074, "Asia/Shanghai")
add("Sydney", -33.8688, 151.2093, "Australia/Sydney")
add("Melbourne", -37.8136, 144.9631, "Australia/Melbourne")
add("Auckland", -36.8485, 174.7633, "Pacific/Auckland")
add("Perth", -31.9505, 115.8605, "Australia/Perth")
add("Anchorage", 61.2181, -149.9003, "America/Anchorage")
add("Honolulu", 21.3069, -157.8583, "Pacific/Honolulu")

add("Greenwich", 51.4769, 0.0005, "Europe/London")
add("Equator / Prime Meridian", 0.0, 0.0, "GMT")
add("North Pole", 90.0, 0.0, "GMT")
add("South Pole", -90.0, 0.0, "GMT")

table.sort(places, function(a, b)
  return a.name < b.name
end)

function places.all()
  return places
end

function places.filter(query)
  query = string.lower(query or "")
  if query == "" then
    return places
  end
  local filtered = {}
  for index = 1, #places do
    local place = places[index]
    if place.search:find(query, 1, true) then
      filtered[#filtered + 1] = place
    end
  end
  return filtered
end

return places
