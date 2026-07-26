-- Meteors: Stunning procedural meteor and comet renderer
-- Refactored with separation of concerns: config, physics, geology, rendering

local config = require("config")
local physics = require("physics.trajectory")
local geology = require("geology.generator")
local rendering = require("rendering.effects")

-- Module state
local meteors = { list = {}, nextId = 1 }
local explosions = {}
local cometTimer = 0
local nextCometTick = 3600
local lastCamera = { x = 0, y = 70, z = 0, yaw = 0, pitch = 0, valid = false }

-- Register settings and keybinds
config.register()

-- Keybind handlers
local function spawnMeteor()
  if not lastCamera.valid then return end
  local meteor = physics.spawn("meteor", lastCamera.x, lastCamera.y, lastCamera.z, lastCamera.yaw, lastCamera.pitch)
  meteor.id = meteors.nextId
  meteors.nextId = meteors.nextId + 1
  meteors.list[#meteors.list + 1] = meteor
end

local function spawnComet()
  if not lastCamera.valid then return end
  local meteor = physics.spawn("comet", lastCamera.x, lastCamera.y, lastCamera.z, lastCamera.yaw, lastCamera.pitch)
  meteor.id = meteors.nextId
  meteors.nextId = meteors.nextId + 1
  meteors.list[#meteors.list + 1] = meteor
end

-- Tick handler
minecraft.on("client_tick", { after_world = true }, function(event)
  if event.paused then return event end
  local dt = 0.05
  cometTimer = cometTimer + dt
  if cometTimer > nextCometTick then
    spawnComet()
    cometTimer = 0
    nextCometTick = 3600 + math.random() * 3600
  end
  
  local max_meteors = config.get("max_meteors") or 50
  local i = #meteors.list
  while i >= 1 do
    local meteor = meteors.list[i]
    if meteor.alive then
      local status = physics.update(meteor, dt, lastCamera.y)
      
      if status == "impact" then
        geology.createImpactSite(meteor.x, 0, meteor.z, meteor.size, meteor.kind)
        explosions[#explosions + 1] = {
          x = meteor.x, y = 0, z = meteor.z,
          size = meteor.size,
          age = 0,
        }
        
        for _, fragment in ipairs(meteor.fragments) do
          fragment.id = meteors.nextId
          meteors.nextId = meteors.nextId + 1
          meteors.list[#meteors.list + 1] = fragment
        end
      end
    else
      table.remove(meteors.list, i)
    end
    i = i - 1
  end
  
  while #meteors.list > max_meteors do
    table.remove(meteors.list, 1)
  end
  
  i = #explosions
  while i >= 1 do
    explosions[i].age = explosions[i].age + dt
    if explosions[i].age > 0.5 then
      table.remove(explosions, i)
    end
    i = i - 1
  end
  
  if minecraft.keybinds.consume("spawn_meteor") then
    spawnMeteor()
  end
  if minecraft.keybinds.consume("spawn_comet") then
    spawnComet()
  end
  return event
end)

minecraft.on("world_render", {
  stage = "entities",
  moment = "after",
  priority = 120,
}, function(event)
  if event.shadow_pass or not event.has_world then return event end
  local camera = {
    x = event.camera_x,
    y = event.camera_y,
    z = event.camera_z,
    yaw = event.camera_yaw,
    pitch = event.camera_pitch,
  }
  lastCamera.x, lastCamera.y, lastCamera.z = camera.x, camera.y, camera.z
  lastCamera.yaw, lastCamera.pitch = camera.yaw, camera.pitch
  lastCamera.valid = true
  local vertices = {}
  
  -- Render all meteors
  for _, meteor in ipairs(meteors.list) do
    if meteor.alive then
      local meteorVerts = rendering.renderMeteor(meteor, camera, geology)
      for _, v in ipairs(meteorVerts) do
        vertices[#vertices + 1] = v
      end
      
      if meteor.kind == "comet" then
        local cometVerts = rendering.renderCometEffects(meteor, camera, 0.016)
        for _, v in ipairs(cometVerts) do
          vertices[#vertices + 1] = v
        end
      end
    end
  end
  
  -- Render explosions
  for _, explosion in ipairs(explosions) do
    local expVerts = rendering.renderExplosion(explosion.x, explosion.y, explosion.z,
      explosion.size, explosion.age, camera)
    for _, v in ipairs(expVerts) do
      vertices[#vertices + 1] = v
    end
  end
  
  -- Submit quads for rendering if any
  if #vertices >= 4 then
    local count = math.floor(#vertices / 4) * 4
    local packed = {}
    for i = 1, count do
      local vertex = vertices[i]
      local base = (i - 1) * 9
      packed[base + 1] = vertex.x
      packed[base + 2] = vertex.y
      packed[base + 3] = vertex.z
      packed[base + 4] = 0
      packed[base + 5] = 0
      packed[base + 6] = vertex.r
      packed[base + 7] = vertex.g
      packed[base + 8] = vertex.b
      packed[base + 9] = vertex.a
    end
    minecraft.render.quads({
      world_space = true,
      blend = true,
      cull = false,
      depth_test = true,
      depth_write = false,
      packed = packed,
    })
  end
  return event
end)

minecraft.log("info", "Meteors mod loaded (refactored)")
