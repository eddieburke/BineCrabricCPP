local commands = require("lib.commands")

local DAY_LENGTH = 24000
local TIME_OF_DAY = { day = 0, noon = 6000, night = 13000, midnight = 18000 }

local function player()
  local entity = minecraft.world.player()
  assert(entity ~= nil, "no player in this world")
  return entity
end

commands.register({
  name = "time",
  params = "<set|add> <ticks|day|noon|night|midnight>",
  help = "adds to or sets the world time",
  run = function(context)
    local method = (context.args[1] or ""):lower()
    local value = context.args[2]
    if value == nil or (method ~= "set" and method ~= "add") then
      return context.usage()
    end
    local now = minecraft.world.get_time()
    local named = TIME_OF_DAY[value:lower()]
    if named ~= nil then
      assert(method == "set", "named times only work with set")
      minecraft.world.set_time(now - (now % DAY_LENGTH) + named)
      context.reply("Set time to " .. value:lower())
      return
    end
    local ticks = commands.integer(value, "ticks")
    if method == "add" then
      assert(now + ticks >= 0, "time cannot go below zero")
      minecraft.world.set_time(now + ticks)
      context.reply("Added " .. ticks .. " to time")
    else
      assert(ticks >= 0, "time cannot go below zero")
      minecraft.world.set_time(ticks)
      context.reply("Set time to " .. ticks)
    end
  end,
})

commands.register({
  name = "give",
  params = "<id> [count] [damage]",
  help = "gives the player a resource",
  run = function(context)
    if context.args[1] == nil then
      return context.usage()
    end
    local id = commands.integer(context.args[1], "item id")
    local item = minecraft.items.describe(id)
    assert(item ~= nil, "there's no item with id " .. id)
    local count = context.args[2] ~= nil and commands.integer(context.args[2], "count") or 1
    local damage = context.args[3] ~= nil and commands.integer(context.args[3], "damage") or 0
    count = math.max(1, math.min(count, item.max_count))
    assert(minecraft.inventory.give({ id = id, count = count, damage = damage }), "inventory is full")
    context.reply("Given " .. count .. " of item " .. id)
  end,
})

commands.register({
  name = "tp",
  params = "<x> <y> <z>",
  aliases = { "teleport" },
  help = "moves the player to a position",
  run = function(context)
    if #context.args < 3 then
      return context.usage()
    end
    local x = commands.number(context.args[1], "x")
    local y = commands.number(context.args[2], "y")
    local z = commands.number(context.args[3], "z")
    local entity = player()
    assert(entity:teleport(x, y, z), "teleport failed")
    context.reply(string.format("Teleported to %.1f %.1f %.1f", x, y, z))
  end,
})

commands.register({
  name = "pos",
  aliases = { "where" },
  help = "prints the player position",
  run = function(context)
    local entity = player()
    context.reply(string.format("%.2f %.2f %.2f", entity.x, entity.y, entity.z))
  end,
})

commands.register({
  name = "spawn",
  params = "<entity> [count]",
  help = "spawns entities at the player",
  run = function(context)
    if context.args[1] == nil then
      return context.usage()
    end
    local entityId = context.args[1]
    local count = context.args[2] ~= nil and commands.integer(context.args[2], "count") or 1
    count = math.max(1, math.min(count, 64))
    local entity = player()
    local spawned = 0
    for _ = 1, count do
      if minecraft.world.spawn_entity(entityId, entity.x, entity.y, entity.z) then
        spawned = spawned + 1
      end
    end
    assert(spawned > 0, "there's no entity named " .. entityId)
    context.reply("Spawned " .. spawned .. " " .. entityId)
  end,
})

commands.register({
  name = "seed",
  help = "prints the world seed",
  run = function(context)
    context.reply("Seed: " .. tostring(context.event.world_seed))
  end,
})

commands.register({
  name = "list",
  help = "lists all currently connected players",
  run = function(context)
    context.reply("Connected players: " .. player().name)
  end,
})

commands.register({
  name = "me",
  params = "<action>",
  help = "broadcasts an action",
  run = function(context)
    assert(context.rest ~= "", "say what you are doing")
    context.reply("* " .. player().name .. " " .. context.rest)
  end,
})

commands.register({
  name = "say",
  params = "<message>",
  help = "broadcasts a message",
  run = function(context)
    assert(context.rest ~= "", "say something")
    context.reply(commands.PURPLE .. "[Server] " .. context.rest)
  end,
})
