-- Meteors: Stunning procedural meteor and comet renderer
-- Refactored with separation of concerns: config, physics, geology, rendering

local config = require("meteors.config")
local physics = require("meteors.physics.trajectory")
local geology = require("meteors.geology.generator")
local rendering = require("meteors.rendering.effects")

-- Module state
local meteors = { list = {}, nextId = 1 }
local explosions = {}
local cometTimer = 0
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
minecraft.event.register("tick", function(dt)
  cometTimer = cometTimer + dt
  if cometTimer > (nextCometTick or 3600) then
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
end)

minecraft.event.register("render", function(camera)
  if camera then
    lastCamera.x, lastCamera.y, lastCamera.z = camera.x, camera.y, camera.z
    lastCamera.yaw, lastCamera.pitch = camera.yaw or 0, camera.pitch or 0
    lastCamera.valid = true
  end
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
    local quadCount = math.floor(#vertices / 4)
    local quads = {}
    for i = 1, quadCount do
      local idx = (i - 1) * 4
      quads[#quads + 1] = {
        r = vertices[idx + 1].r, g = vertices[idx + 1].g, b = vertices[idx + 1].b, a = vertices[idx + 1].a,
        vertices = {
          { x = vertices[idx + 1].x, y = vertices[idx + 1].y, z = vertices[idx + 1].z, u = 0, v = 0 },
          { x = vertices[idx + 2].x, y = vertices[idx + 2].y, z = vertices[idx + 2].z, u = 1, v = 0 },
          { x = vertices[idx + 3].x, y = vertices[idx + 3].y, z = vertices[idx + 3].z, u = 1, v = 1 },
          { x = vertices[idx + 4].x, y = vertices[idx + 4].y, z = vertices[idx + 4].z, u = 0, v = 1 },
        }
      }
    end
    minecraft.render.quads(quads)
  end
end)

minecraft.log("info", "Meteors mod loaded (refactored)")
