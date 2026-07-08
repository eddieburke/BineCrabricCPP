local CAMERA_ITEM = 30800
local TV_BLOCK = 162
local CH_WIDTH = 256
local CH_HEIGHT = 192
local TRIPOD_TEXTURE = "mods/camera/tripod.png"
local CAMERA_BODY_TEXTURE = "mods/camera/camera_body.png"

-- Camera body cube-net atlas (camera_body.png, 2 cols x 3 rows of 16px tiles).
-- Each face samples its own tile so top/bottom/sides/lens/viewport all differ.
local U0, U1, U2 = 0.0, 0.5, 1.0
local V0, V1, V2, V3 = 0.0, 1.0 / 3.0, 2.0 / 3.0, 1.0
local CAMERA_FACES = {
  top    = { U0, V0, U1, V1 },
  bottom = { U1, V0, U2, V1 },
  front  = { U0, V1, U1, V2 }, -- +z lens
  back   = { U1, V1, U2, V2 }, -- -z viewport
  left   = { U0, V2, U1, V3 },
  right  = { U1, V2, U2, V3 },
}
local SCREEN_ID = "camera:viewfinder"
local PANEL_W = 256
local PANEL_H = 192
local FRAME = 6
local RENDER_INTERVAL_ACTIVE = 1
local RENDER_INTERVAL_PASSIVE = 2
local WATCH_RANGE_SQ = 48 * 48

local DEG2RAD = math.pi / 180.0

local tripods = {}
local channel_order = {}
local active_index = 0
local view_yaw = 0.0
local view_pitch = 0.0
local tvs = {}
local pending_tvs = {}
local tv_check_tick = 0
local viewfinder_open = false

local SIDE_OFFSETS = {
  [0] = { 0, -1, 0 },
  [1] = { 0, 1, 0 },
  [2] = { 0, 0, -1 },
  [3] = { 0, 0, 1 },
  [4] = { -1, 0, 0 },
  [5] = { 1, 0, 0 },
}

local function tv_key(x, y, z)
  return x .. "," .. y .. "," .. z
end

local function facing_toward_player()
  local yaw = ((view_yaw % 360) + 360) % 360
  if yaw >= 45 and yaw < 135 then
    return 1, 0
  elseif yaw >= 135 and yaw < 225 then
    return 0, 1
  elseif yaw >= 225 and yaw < 315 then
    return -1, 0
  end
  return 0, -1
end

local function active_channel()
  if active_index < 1 or active_index > #channel_order then
    return 0
  end
  return channel_order[active_index]
end

local function cycle_channel(direction)
  local count = #channel_order
  if count == 0 then
    active_index = 0
    return
  end
  active_index = ((active_index - 1 + direction) % count) + 1
end

local function add_channel(handle)
  channel_order[#channel_order + 1] = handle
  if active_index < 1 then
    active_index = 1
  end
end

local function remove_channel(handle)
  for index, existing in ipairs(channel_order) do
    if existing == handle then
      table.remove(channel_order, index)
      break
    end
  end
  if active_index > #channel_order then
    active_index = #channel_order
  end
end

local function place_tripod(x, y, z)
  local handle = minecraft.camera.create(CH_WIDTH, CH_HEIGHT)
  if handle < 0 then
    return false
  end
  tripods[handle] = {
    x = x + 0.5,
    y = y + 1.0,
    z = z + 0.5,
    block_x = x,
    block_y = y,
    block_z = z,
    yaw = view_yaw,
    channel = handle,
    frame_counter = 0,
  }
  add_channel(handle)
  return true
end

local function tripod_on_block(x, y, z)
  for handle, t in pairs(tripods) do
    if t.block_x == x and t.block_y == y and t.block_z == z then
      return handle
    end
  end
  return nil
end

local function pickup_tripod(handle)
  minecraft.camera.destroy(handle)
  tripods[handle] = nil
  remove_channel(handle)
  minecraft.inventory.give({ id = CAMERA_ITEM, count = 1 })
end

minecraft.register_item({
  id = CAMERA_ITEM,
  name = "Camera",
  translation_key = "camera.camera",
  max_count = 1,
  texture = "mods/camera/camera.png",
})

-- tv_body.png is a 32x32 cube-net atlas: bottom-left=side, top-right=top,
-- bottom-left(row2)=bottom. Manual UVs are in 0..16 pixel space over the full
-- image, so 16px tiles land on 0/8/16 boundaries.
local TV_SIDE = { 0, 0, 8, 8 }
local TV_TOP = { 8, 0, 16, 8 }
local TV_BOTTOM = { 0, 8, 8, 16 }

local function tv_face(a, b, c, d, uv)
  local u0, v0, u1, v1 = uv[1], uv[2], uv[3], uv[4]
  minecraft.tessellator.quad({
    vertices = {
      { a[1], a[2], a[3], u0, v0 },
      { b[1], b[2], b[3], u1, v0 },
      { c[1], c[2], c[3], u1, v1 },
      { d[1], d[2], d[3], u0, v1 },
    },
  })
  -- draw the reverse winding too so the face shows regardless of cull order
  minecraft.tessellator.quad({
    vertices = {
      { d[1], d[2], d[3], u0, v1 },
      { c[1], c[2], c[3], u1, v1 },
      { b[1], b[2], b[3], u1, v0 },
      { a[1], a[2], a[3], u0, v0 },
    },
  })
end

local function draw_tv()
  tv_face({ 0, 1, 0 }, { 1, 1, 0 }, { 1, 1, 1 }, { 0, 1, 1 }, TV_TOP)     -- top
  tv_face({ 0, 0, 1 }, { 1, 0, 1 }, { 1, 0, 0 }, { 0, 0, 0 }, TV_BOTTOM)  -- bottom
  tv_face({ 0, 1, 1 }, { 1, 1, 1 }, { 1, 0, 1 }, { 0, 0, 1 }, TV_SIDE)    -- +z
  tv_face({ 1, 1, 0 }, { 0, 1, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, TV_SIDE)    -- -z
  tv_face({ 1, 1, 1 }, { 1, 1, 0 }, { 1, 0, 0 }, { 1, 0, 1 }, TV_SIDE)    -- +x
  tv_face({ 0, 1, 0 }, { 0, 1, 1 }, { 0, 0, 1 }, { 0, 0, 0 }, TV_SIDE)    -- -x
end

minecraft.register_block({
  id = TV_BLOCK,
  texture = "mods/camera/tv_body.png",
  hardness = 1.5,
  resistance = 4.0,
  translation_key = "camera.tv",
  material = "metal",
  model = {
    type = "manual",
    draw = draw_tv,
    opaque = false,
    full_cube = false,
  },
  item = {
    texture = "mods/camera/tv_icon.png",
  },
  on_use = function(event)
    if not event.right_click then
      return
    end
    if event.has_item and event.item_id == CAMERA_ITEM then
      return
    end
    viewfinder_open = true
    minecraft.screen.open(SCREEN_ID, { title = "Camera Viewfinder" })
    event.handled = true
  end,
})

minecraft.register_shaped_recipe({
  output_item_id = CAMERA_ITEM,
  output_count = 1,
  pattern = { "###", "# #", "###" },
  key = "#",
  item_id = 265,
})

minecraft.on(minecraft.events.camera_setup, {}, function(event)
  view_yaw = event.yaw or view_yaw
  view_pitch = event.pitch or view_pitch
  return event
end)

minecraft.on(minecraft.events.block_interact, { right_click = true }, function(event)
  if event.handled or event.canceled then
    return event
  end
  local existing = tripod_on_block(event.x, event.y, event.z)
  if event.has_item and event.item_id == CAMERA_ITEM then
    if existing == nil and event.block_id ~= TV_BLOCK and place_tripod(event.x, event.y, event.z) then
      event.item_count = 0
      event.handled = true
    end
    return event
  end
  if event.has_item and event.item_id == TV_BLOCK then
    local offset = SIDE_OFFSETS[event.side]
    if offset ~= nil then
      local nx, nz = facing_toward_player()
      pending_tvs[#pending_tvs + 1] = {
        x = event.x + offset[1],
        y = event.y + offset[2],
        z = event.z + offset[3],
        nx = nx,
        nz = nz,
        ticks = 0,
      }
    end
    return event
  end
  if existing ~= nil and not event.has_item then
    pickup_tripod(existing)
    event.handled = true
  end
  return event
end)

local function settle_pending_tvs()
  for index = #pending_tvs, 1, -1 do
    local p = pending_tvs[index]
    p.ticks = p.ticks + 1
    if minecraft.world.block_id(p.x, p.y, p.z) == TV_BLOCK then
      tvs[tv_key(p.x, p.y, p.z)] = p
      table.remove(pending_tvs, index)
    elseif p.ticks > 10 then
      table.remove(pending_tvs, index)
    end
  end
end

local function validate_tvs()
  tv_check_tick = tv_check_tick + 1
  if tv_check_tick % 20 ~= 0 then
    return
  end
  for key, tv in pairs(tvs) do
    if minecraft.world.block_id(tv.x, tv.y, tv.z) ~= TV_BLOCK then
      tvs[key] = nil
    end
  end
end

minecraft.on(minecraft.events.client_tick, { paused = false }, function()
  settle_pending_tvs()
  validate_tvs()
end)

local function any_tv_watchable()
  local player = minecraft.world.player()
  if player == nil then
    return next(tvs) ~= nil
  end
  for _, tv in pairs(tvs) do
    local dx = player.x - (tv.x + 0.5)
    local dy = player.y - (tv.y + 0.5)
    local dz = player.z - (tv.z + 0.5)
    if dx * dx + dy * dy + dz * dz <= WATCH_RANGE_SQ and dx * tv.nx + dz * tv.nz > 0 then
      return true
    end
  end
  return false
end

-- Fires once per real frame. Owns all "is this feed worth rendering right
-- now" policy: which channel is shown, how watchable it is, and how often
-- to refresh it. Native only knows "render this handle now" (minecraft.camera.render).
minecraft.on(minecraft.events.render_targets, {}, function(event)
  local shown = active_channel()
  if shown <= 0 then
    return
  end
  local t = tripods[shown]
  if t == nil then
    return
  end
  if not (viewfinder_open or any_tv_watchable()) then
    return
  end
  local interval = viewfinder_open and RENDER_INTERVAL_ACTIVE or RENDER_INTERVAL_PASSIVE
  t.frame_counter = t.frame_counter + 1
  if t.frame_counter % interval ~= 0 then
    return
  end
  minecraft.camera.render(t.channel, t.x, t.y + 1.30, t.z, t.yaw + 180.0, 8.0, 0.0, 70.0, event.tick_delta)
end)

local function append_quad(list, ax, ay, az, au, av, bx, by, bz, bu, bv, cx, cy, cz, cu, cv, dx, dy, dz, du, dv)
  list[#list + 1] = { x = ax, y = ay, z = az, u = au, v = av }
  list[#list + 1] = { x = bx, y = by, z = bz, u = bu, v = bv }
  list[#list + 1] = { x = cx, y = cy, z = cz, u = cu, v = cv }
  list[#list + 1] = { x = dx, y = dy, z = dz, u = du, v = dv }
end

-- Per-face textured box: each face table is { u0, v0, u1, v1 } into the atlas.
local function append_box_faces(list, cx, cy, cz, hx, hy, hz, faces)
  local x0, x1 = cx - hx, cx + hx
  local y0, y1 = cy - hy, cy + hy
  local z0, z1 = cz - hz, cz + hz
  local t = faces.top
  append_quad(list, x0, y1, z0, t[1], t[2], x1, y1, z0, t[3], t[2], x1, y1, z1, t[3], t[4], x0, y1, z1, t[1], t[4])
  local b = faces.bottom
  append_quad(list, x0, y0, z1, b[1], b[2], x1, y0, z1, b[3], b[2], x1, y0, z0, b[3], b[4], x0, y0, z0, b[1], b[4])
  local f = faces.front
  append_quad(list, x0, y1, z1, f[1], f[2], x1, y1, z1, f[3], f[2], x1, y0, z1, f[3], f[4], x0, y0, z1, f[1], f[4])
  local k = faces.back
  append_quad(list, x1, y1, z0, k[1], k[2], x0, y1, z0, k[3], k[2], x0, y0, z0, k[3], k[4], x1, y0, z0, k[1], k[4])
  local l = faces.left
  append_quad(list, x0, y1, z0, l[1], l[2], x0, y1, z1, l[3], l[2], x0, y0, z1, l[3], l[4], x0, y0, z0, l[1], l[4])
  local r = faces.right
  append_quad(list, x1, y1, z1, r[1], r[2], x1, y1, z0, r[3], r[2], x1, y0, z0, r[3], r[4], x1, y0, z1, r[1], r[4])
end

-- Tripod legs sample the whole (clean, low-detail) tripod.png strut, 0..1 UV.
local function append_leg(list, ax, ay, az, fx, fy, fz, half)
  append_quad(list,
    ax - half, ay, az, 0.0, 0.0, ax + half, ay, az, 1.0, 0.0,
    fx + half, fy, fz, 1.0, 1.0, fx - half, fy, fz, 0.0, 1.0)
  append_quad(list,
    ax, ay, az - half, 0.0, 0.0, ax, ay, az + half, 1.0, 0.0,
    fx, fy, fz + half, 1.0, 1.0, fx, fy, fz - half, 0.0, 1.0)
end

local function build_tripod_legs(t, cam_x, cam_y, cam_z)
  local list = {}
  local bx = t.x - cam_x
  local by = t.y - cam_y
  local bz = t.z - cam_z
  local apex_y = by + 1.25
  local base_angle = t.yaw * DEG2RAD
  for k = 0, 2 do
    local angle = base_angle + k * (2.0 * math.pi / 3.0)
    local fx = bx + math.cos(angle) * 0.35
    local fz = bz + math.sin(angle) * 0.35
    append_leg(list, bx, apex_y, bz, fx, by, fz, 0.03)
  end
  return list
end

local function build_camera_body(t, cam_x, cam_y, cam_z)
  local list = {}
  local bx = t.x - cam_x
  local by = t.y - cam_y
  local bz = t.z - cam_z
  append_box_faces(list, bx, by + 1.30, bz, 0.14, 0.10, 0.14, CAMERA_FACES)
  return list
end

local SCREEN_X0 = 0.125
local SCREEN_X1 = 0.875
local SCREEN_Y0 = 0.25
local SCREEN_Y1 = 0.875
local SCREEN_LIFT = 0.004

local function build_tv_screen(tv, cam_x, cam_y, cam_z)
  local bx = tv.x - cam_x
  local by = tv.y - cam_y
  local bz = tv.z - cam_z
  local y0 = by + SCREEN_Y0
  local y1 = by + SCREEN_Y1
  local nx, nz = tv.nx, tv.nz
  local face
  if nz == 1 then
    face = { bz + 1 + SCREEN_LIFT, bx + SCREEN_X1, bx + SCREEN_X0 }
    return {
      { x = face[3], y = y1, z = face[1], u = 1, v = 1 },
      { x = face[2], y = y1, z = face[1], u = 0, v = 1 },
      { x = face[2], y = y0, z = face[1], u = 0, v = 0 },
      { x = face[3], y = y0, z = face[1], u = 1, v = 0 },
    }
  elseif nz == -1 then
    face = { bz - SCREEN_LIFT, bx + SCREEN_X0, bx + SCREEN_X1 }
    return {
      { x = face[3], y = y1, z = face[1], u = 1, v = 1 },
      { x = face[2], y = y1, z = face[1], u = 0, v = 1 },
      { x = face[2], y = y0, z = face[1], u = 0, v = 0 },
      { x = face[3], y = y0, z = face[1], u = 1, v = 0 },
    }
  elseif nx == 1 then
    face = { bx + 1 + SCREEN_LIFT, bz + SCREEN_X0, bz + SCREEN_X1 }
    return {
      { x = face[1], y = y1, z = face[3], u = 1, v = 1 },
      { x = face[1], y = y1, z = face[2], u = 0, v = 1 },
      { x = face[1], y = y0, z = face[2], u = 0, v = 0 },
      { x = face[1], y = y0, z = face[3], u = 1, v = 0 },
    }
  end
  face = { bx - SCREEN_LIFT, bz + SCREEN_X1, bz + SCREEN_X0 }
  return {
    { x = face[1], y = y1, z = face[3], u = 1, v = 1 },
    { x = face[1], y = y1, z = face[2], u = 0, v = 1 },
    { x = face[1], y = y0, z = face[2], u = 0, v = 0 },
    { x = face[1], y = y0, z = face[3], u = 1, v = 0 },
  }
end

minecraft.on(minecraft.events.world_render, {
  stage = minecraft.render.stages.clouds,
  moment = minecraft.render.moments.after,
}, function(event)
  local cam_x = event.camera_x or 0.0
  local cam_y = event.camera_y or 0.0
  local cam_z = event.camera_z or 0.0
  for _, t in pairs(tripods) do
    local dx = t.x - cam_x
    local dy = (t.y + 1.30) - cam_y
    local dz = t.z - cam_z
    if dx * dx + dy * dy + dz * dz > 0.01 then
      minecraft.render.quads({
        texture = TRIPOD_TEXTURE,
        vertices = build_tripod_legs(t, cam_x, cam_y, cam_z),
        blend = false,
        cull = false,
        depth_test = true,
        depth_write = true,
      })
      minecraft.render.quads({
        texture = CAMERA_BODY_TEXTURE,
        vertices = build_camera_body(t, cam_x, cam_y, cam_z),
        blend = false,
        cull = false,
        depth_test = true,
        depth_write = true,
      })
    end
  end
  local channel = active_channel()
  local rendering = minecraft.camera.rendering()
  local tex = (channel > 0 and channel ~= rendering) and minecraft.camera.texture(channel) or -1
  if tex and tex > 0 then
    for _, tv in pairs(tvs) do
      minecraft.render.quads({
        texture_id = tex,
        vertices = build_tv_screen(tv, cam_x, cam_y, cam_z),
        blend = false,
        cull = false,
        depth_test = true,
        depth_write = true,
      })
    end
  end
  return event
end)

local function panel_origin(event)
  local width = event.width or 320
  local height = event.height or 240
  return math.floor((width - PANEL_W) / 2), math.floor((height - PANEL_H) / 2)
end

minecraft.screen.on_lua_screen(SCREEN_ID, {
  init = function(event)
    local x, y = panel_origin(event)
    minecraft.screen.add_button(x, y + PANEL_H + 8, 60, 20, "< Prev", function()
      cycle_channel(-1)
    end)
    minecraft.screen.add_button(x + PANEL_W - 60, y + PANEL_H + 8, 60, 20, "Next >", function()
      cycle_channel(1)
    end)
    minecraft.screen.add_button(x + math.floor(PANEL_W / 2) - 30, y + PANEL_H + 8, 60, 20, "Close", function()
      minecraft.screen.close()
    end)
  end,
  render = function(event)
    local x, y = panel_origin(event)
    minecraft.gui.fill_rect(x - FRAME, y - FRAME, PANEL_W + FRAME * 2, PANEL_H + FRAME * 2, 0xFF101014)
    local channel = active_channel()
    local tex = channel > 0 and minecraft.camera.texture(channel) or -1
    if tex and tex > 0 then
      minecraft.gui.draw_texture(tex, x, y, PANEL_W, PANEL_H)
    else
      minecraft.gui.fill_rect(x, y, PANEL_W, PANEL_H, 0xFF000000)
      minecraft.gui.draw_centered_text(x, y + math.floor(PANEL_H / 2) - 4, PANEL_W, "NO SIGNAL", 0xFF5A6270)
    end
    local label = #channel_order == 0 and "No cameras" or ("CAM " .. active_index .. " / " .. #channel_order)
    minecraft.gui.draw_text(x, y - FRAME - 12, label, 0xFFB0C8E6)
  end,
  close = function()
    viewfinder_open = false
  end,
})
