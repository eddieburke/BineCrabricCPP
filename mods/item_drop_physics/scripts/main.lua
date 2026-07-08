local model_cache = {}

local function get_or_create_model(item)
  local path = item.texture_path
  if not path or path == "" then
    return nil
  end
  local atlas_idx = item.atlas_index or -1
  local key = path .. ":" .. atlas_idx
  if model_cache[key] then
    return model_cache[key]
  end

  local image = minecraft.render.get_texture_pixels(path)
  if not image or image.width <= 0 or image.height <= 0 then
    image = minecraft.render.get_texture_pixels(item.item_id)
    if not image or image.width <= 0 or image.height <= 0 then
      return nil
    end
  end

  local is_mod_tex = item.mod_texture
  local W, H = image.width, image.height
  local tileX, tileY = 0, 0
  if not is_mod_tex and atlas_idx >= 0 then
    tileX = (atlas_idx % 16) * 16
    tileY = math.floor(atlas_idx / 16) * 16
  end

  local grid = {}
  for y = 0, 15 do
    grid[y] = {}
    for x = 0, 15 do
      local px, py
      if is_mod_tex then
        px = math.floor(x * (W / 16))
        py = math.floor(y * (H / 16))
      else
        px = tileX + x
        py = tileY + y
      end
      local idx = py * W + px + 1
      local argb = image.pixels[idx] or 0
      if argb < 0 then
        argb = argb + 0x100000000
      end
      local a = math.floor(argb / 16777216) % 256
      local r = math.floor(argb / 65536) % 256
      local g = math.floor(argb / 256) % 256
      local b = argb % 256
      if a > 30 then
        grid[y][x] = { r = r / 255, g = g / 255, b = b / 255, a = a / 255 }
      else
        grid[y][x] = false
      end
    end
  end

  local scale = 1.0 / 16.0
  local thick = scale
  local vertices = {}

  local function add_face(x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, color)
    local r, g, b, a = color.r, color.g, color.b, color.a
    table.insert(vertices, { x = x1, y = y1, z = z1, r = r, g = g, b = b, a = a })
    table.insert(vertices, { x = x2, y = y2, z = z2, r = r, g = g, b = b, a = a })
    table.insert(vertices, { x = x3, y = y3, z = z3, r = r, g = g, b = b, a = a })
    table.insert(vertices, { x = x4, y = y4, z = z4, r = r, g = g, b = b, a = a })
  end

  for y = 0, 15 do
    for x = 0, 15 do
      local col = grid[y][x]
      if col then
        local minX = (x - 7.5) * scale
        local maxX = minX + scale
        local minY = (7.5 - y) * scale
        local maxY = minY + scale
        local minZ = -thick / 2
        local maxZ = thick / 2

        add_face(minX, minY, maxZ, maxX, minY, maxZ, maxX, maxY, maxZ, minX, maxY, maxZ, col)
        add_face(maxX, minY, minZ, minX, minY, minZ, minX, maxY, minZ, maxX, maxY, minZ, col)

        if x == 0 or not grid[y][x - 1] then
          add_face(minX, minY, minZ, minX, minY, maxZ, minX, maxY, maxZ, minX, maxY, minZ, col)
        end
        if x == 15 or not grid[y][x + 1] then
          add_face(maxX, minY, maxZ, maxX, minY, minZ, maxX, maxY, minZ, maxX, maxY, maxZ, col)
        end
        if y == 0 or not grid[y - 1][x] then
          add_face(minX, maxY, maxZ, maxX, maxY, maxZ, maxX, maxY, minZ, minX, maxY, minZ, col)
        end
        if y == 15 or not grid[y + 1][x] then
          add_face(minX, minY, minZ, maxX, minY, minZ, maxX, minY, maxZ, minX, minY, maxZ, col)
        end
      end
    end
  end

  model_cache[key] = vertices
  return vertices
end

minecraft.on(minecraft.events.pre_entity_render, function(event)
  if event.entity_type == "Item" then
    event.canceled = true
    return event
  end
end)

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.clouds,
  moment = minecraft.render.moments.after,
}, function(event)
  local list = minecraft.entities.list("Item")
  if not list then
    return
  end
  local time = event.world_time or 0.0
  local delta = event.tick_delta or 0.0
  local t = time + delta

  for i = 1, #list do
    local item = list[i]
    local vertices = get_or_create_model(item)
    if vertices and #vertices > 0 then
      local spin = (t * 0.05 + item.id * 0.1) % (math.pi * 2)
      local bob = math.sin(t * 0.1 + item.id * 0.5) * 0.1

      local ex = item.x
      local ey = item.y
      local ez = item.z

      local s = math.sin(spin)
      local c = math.cos(spin)

      local trans = {}
      for j = 1, #vertices do
        local v = vertices[j]
        local rx = v.x * c - v.z * s
        local rz = v.x * s + v.z * c
        trans[j] = {
          x = ex + rx,
          y = ey + 0.2 + bob + v.y,
          z = ez + rz,
          r = v.r,
          g = v.g,
          b = v.b,
          a = v.a,
        }
      end

      minecraft.render.quads({
        blend = false,
        cull = true,
        depth_test = true,
        depth_write = true,
        vertices = trans,
      })
    end
  end
end)
