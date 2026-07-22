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
  -- Update camera
  local cam = minecraft.camera.get()
  if cam then
    lastCamera.x, lastCamera.y, lastCamera.z = cam.x, cam.y, cam.z
    lastCamera.yaw, lastCamera.pitch = cam.yaw, cam.pitch
    lastCamera.valid = true
  end
  
  -- Spawn comet periodically
  cometTimer = cometTimer + dt
  if cometTimer > (nextCometTick or 3600) then
    spawnComet()
    cometTimer = 0
    nextCometTick = 3600 + math.random() * 3600
  end
  
  -- Update meteors
  local max_meteors = config.get("max_meteors") or 50
  local i = #meteors.list
  while i >= 1 do
    local meteor = meteors.list[i]
    if meteor.alive then
      local status = physics.update(meteor, dt, lastCamera.y)
      
      -- Handle impact
      if status == "impact" then
        geology.createImpactSite(meteor.x, 0, meteor.z, meteor.size, meteor.kind)
        explosions[#explosions + 1] = {
          x = meteor.x, y = 0, z = meteor.z,
          size = meteor.size,
          age = 0,
        }
        
        -- Add fragments
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
  
  -- Limit meteor count
  while #meteors.list > max_meteors do
    table.remove(meteors.list, 1)
  end
  
  -- Update explosions
  i = #explosions
  while i >= 1 do
    explosions[i].age = explosions[i].age + dt
    if explosions[i].age > 0.5 then
      table.remove(explosions, i)
    end
    i = i - 1
  end
  
  -- Keybind handling
  if minecraft.keybinds.consume("spawn_meteor") then
    spawnMeteor()
  end
  if minecraft.keybinds.consume("spawn_comet") then
    spawnComet()
  end
end)

-- Render handler
minecraft.event.register("render", function(camera)
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
  
  -- Submit vertices for rendering
  if #vertices > 0 then
    minecraft.render.begin("triangles")
    for _, v in ipairs(vertices) do
      minecraft.render.color(v.r, v.g, v.b, v.a)
      minecraft.render.vertex(v.x, v.y, v.z)
    end
    minecraft.render.end()
  end
end)

minecraft.log("info", "Meteors mod loaded (refactored)")
