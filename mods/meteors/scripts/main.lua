local config = require("config")
local mathx = require("lib.mathx")
local physics = require("scripts.physics.trajectory")
local geology = require("scripts.geology.generator")
local rendering = require("scripts.rendering.effects")

local clamp = mathx.clamp
local floor, min, random = math.floor, math.min, math.random

local SHOWER_MIN_TICKS = 700
local SHOWER_RANGE_TICKS = 1600
local FLASH_TICKS = 14
local DRAW_RANGE = 340
local COMET_CHANCE = 0.12

local meteors = {}
local flashes = {}
local camera = { x = 0, y = 70, z = 0, yaw = 0, pitch = 0, valid = false }
local effectVertices = {}
local effectFloats = 0
local nextId = 1
local showerTimer = SHOWER_MIN_TICKS

config.register()

local function spawn(kind)
  if not camera.valid then return end
  if #meteors >= (config.get("max_meteors") or 50) then return end
  local meteor = physics.spawn(kind, nextId, camera.x, camera.y, camera.z, camera.yaw, camera.pitch)
  nextId = nextId + 1
  meteors[#meteors + 1] = meteor
end

local function impactBurst(meteor)
  local count = min(26, 8 + floor(meteor.size * 12))
  local heat = clamp(meteor.heat, 0, 1)
  local r, g, b = geology.ember(0.35 + heat * 0.65)
  for i = 1, count do
    local ux, uy, uz = mathx.randomUnit(meteor.id * 31 + i, i)
    minecraft.particles.spawn({
      x = meteor.x, y = meteor.y + 0.3, z = meteor.z,
      vx = ux * 0.26, vy = math.abs(uy) * 0.40 + 0.05, vz = uz * 0.26,
      scale = 2.2 + meteor.size,
      r = r, g = g, b = b,
      max_age = 18 + i % 16,
      gravity = 0.06,
    })
  end
end

minecraft.on("client_tick", { after_world = true }, function(event)
  if event.paused or not event.has_world then return event end

  if minecraft.keybinds.consume("spawn_meteor") then spawn("meteor") end
  if minecraft.keybinds.consume("spawn_comet") then spawn("comet") end

  local cap = config.get("max_meteors") or 50
  for i = #meteors, 1, -1 do
    local meteor = meteors[i]
    local status
    status, nextId = physics.update(meteor, nextId)

    if status == "impact" then
      flashes[#flashes + 1] = {
        x = meteor.x, y = meteor.y, z = meteor.z,
        size = meteor.size, age = 0,
      }
      impactBurst(meteor)
    end

    local fragments = meteor.fragments
    for k = #fragments, 1, -1 do
      if #meteors < cap then
        meteors[#meteors + 1] = fragments[k]
      end
      fragments[k] = nil
    end

    if not meteor.alive then
      table.remove(meteors, i)
    end
  end

  for i = #flashes, 1, -1 do
    local flash = flashes[i]
    flash.age = flash.age + 1
    if flash.age > FLASH_TICKS then
      table.remove(flashes, i)
    end
  end

  showerTimer = showerTimer - 1
  if showerTimer <= 0 then
    showerTimer = SHOWER_MIN_TICKS + random(0, SHOWER_RANGE_TICKS)
    if camera.valid and minecraft.world.is_night() then
      spawn(random() < COMET_CHANCE and "comet" or "meteor")
    end
  end

  return event
end)

minecraft.on("world_render", {
  stage = "entities",
  moment = "after",
  priority = 120,
}, function(event)
  if not event.has_world then return event end

  camera.x, camera.y, camera.z = event.camera_x, event.camera_y, event.camera_z
  camera.yaw, camera.pitch = event.camera_yaw, event.camera_pitch
  camera.valid = true

  if #meteors == 0 and #flashes == 0 then return event end

  local tickDelta = clamp(event.tick_delta or 1, 0, 1)
  local quality = floor(clamp(config.get("visual_quality") or 3, 1, 3))
  local density = floor(clamp(config.get("effect_density") or 2, 1, 3))
  local sunX, sunY, sunZ = event.sun_x or 0.42, event.sun_y or 0.82, event.sun_z or -0.38
  local heatSteps = geology.heat_steps

  effectFloats = 0

  for i = 1, #meteors do
    local meteor = meteors[i]
    local hx = meteor.px + (meteor.x - meteor.px) * tickDelta
    local hy = meteor.py + (meteor.y - meteor.py) * tickDelta
    local hz = meteor.pz + (meteor.z - meteor.pz) * tickDelta
    local dx, dy, dz = hx - camera.x, hy - camera.y, hz - camera.z

    if dx * dx + dy * dy + dz * dz < DRAW_RANGE * DRAW_RANGE then
      local heat = clamp(meteor.prevHeat + (meteor.heat - meteor.prevHeat) * tickDelta, 0, 1)
      local step = floor(heat * (heatSteps - 1) + 0.5)
      minecraft.render.quads({
        world_space = true,
        x = hx, y = hy, z = hz,
        yaw = meteor.prevSpinY + (meteor.spinY - meteor.prevSpinY) * tickDelta,
        pitch = meteor.prevSpinX + (meteor.spinX - meteor.prevSpinX) * tickDelta,
        roll = meteor.prevSpinZ + (meteor.spinZ - meteor.prevSpinZ) * tickDelta,
        scale = meteor.size,
        blend = false,
        cull = true,
        depth_test = true,
        depth_write = true,
        packed = geology.body(meteor.kind, meteor.variant, quality, step),
      })
      effectFloats = rendering.meteor(effectVertices, effectFloats, meteor, camera, tickDelta,
        sunX, sunY, sunZ, quality, density)
    end
  end

  for i = 1, #flashes do
    effectFloats = rendering.flash(effectVertices, effectFloats, flashes[i], camera, tickDelta, quality)
  end

  if effectFloats >= 36 then
    for k = #effectVertices, effectFloats + 1, -1 do
      effectVertices[k] = nil
    end
    minecraft.render.quads({
      world_space = true,
      blend = true,
      cull = false,
      depth_test = true,
      depth_write = false,
      packed = effectVertices,
    })
  elseif #effectVertices > 0 then
    for k = #effectVertices, 1, -1 do
      effectVertices[k] = nil
    end
  end

  return event
end)

minecraft.log("info", "Meteors mod loaded")
