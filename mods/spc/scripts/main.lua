local commands = require("lib.commands")

local AIR = 0
local WATER = { [8] = true, [9] = true }
local LAVA = { [10] = true, [11] = true }
local hostile = {
  Creeper = true, Skeleton = true, Spider = true, Giant = true, Zombie = true,
  Slime = true, Ghast = true, PigZombie = true,
}
local key_codes = {
  a = 30, b = 48, c = 46, d = 32, e = 18, f = 33, g = 34, h = 35, i = 23,
  j = 36, k = 37, l = 38, m = 50, n = 49, o = 24, p = 25, q = 16, r = 19,
  s = 31, t = 20, u = 22, v = 47, w = 17, x = 45, y = 21, z = 44,
  space = 57, shift = 42, ctrl = 29, alt = 56, f1 = 59, f2 = 60, f3 = 61,
  f4 = 62, f5 = 63, f6 = 64, f7 = 65, f8 = 66, f9 = 67, f10 = 68,
  f11 = 87, f12 = 88,
}

local waypoints = {}
local aliases = {}
local bindings = {}
local state = {
  fly = false,
  noclip = false,
  speed = 1,
  freeze = false,
  instant_kill = false,
  super_punch = false,
  fire_proof = false,
  infinite_items = false,
  fall_proof = false,
  instant_mine = false,
}

local function floor(value)
  return math.floor(tonumber(value) or 0)
end

local function player()
  local value = minecraft.world.player()
  assert(value ~= nil, "no player in this world")
  return value
end

local function position()
  local value = player()
  return floor(value.x), floor(value.y), floor(value.z), value
end

local function set_player(values)
  assert(minecraft.entities.set_state(player(), values), "unable to change player state")
end

local function teleport(x, y, z)
  local value = player()
  waypoints["return"] = { x = value.x, y = value.y, z = value.z }
  assert(value:teleport(x, y, z), "teleport failed")
end

local function toggle(value, current)
  if value == nil or value == "" then
    return not current
  end
  local parsed = minecraft.util.parse_boolean(value)
  assert(parsed ~= nil, "expected on or off")
  return parsed
end

local function block_id(value)
  assert(value ~= nil, "expected a block")
  local id = tonumber(value)
  if id == nil then
    id = minecraft.world.block_id(tostring(value):lower())
    assert(id ~= 0 or tostring(value):lower() == "air", "unknown block: " .. tostring(value))
  end
  return floor(id)
end

local function facing_vector()
  local value = player()
  local angle = math.rad(value.yaw)
  return -math.sin(angle), math.cos(angle)
end

local function clear_inventory()
  for slot = 0, minecraft.inventory.slot_count() - 1 do
    minecraft.inventory.set(slot, { id = 0, count = 0 })
  end
end

local function entities(filter)
  return minecraft.entities.list(filter)
end

local function remove_entities(predicate)
  local removed = 0
  for _, value in ipairs(entities()) do
    if value.type ~= "Player" and predicate(value) and value:remove() then
      removed = removed + 1
    end
  end
  return removed
end

local function command_name(raw)
  local normalized = tostring(raw or ""):lower():gsub("[^%w_%-]", "_")
  assert(normalized ~= "", "expected a name")
  return normalized
end

local function alias_hook(event)
  if event.remote == true or event.canceled == true then
    return event
  end
  local name, rest = tostring(event.message or ""):match("^/([^%s]+)%s*(.*)$")
  if name == nil then
    return event
  end
  local target = aliases[name:lower()]
  if target ~= nil then
    event.message = target .. (rest ~= "" and " " .. rest or "")
  end
  return event
end

minecraft.on("chat_send", { priority = 1000 }, alias_hook)

minecraft.on("client_tick", {}, function()
  for _, binding in pairs(bindings) do
    if minecraft.keybinds.consume(binding.id) then
      commands.execute(binding.command)
    end
  end
  if state.fly then
    local vertical = 0
    if minecraft.is_key_down(key_codes.space) then
      vertical = 0.45
    elseif minecraft.is_key_down(key_codes.shift) then
      vertical = -0.45
    end
    set_player({ no_clip = true, fall_distance = 0, vy = vertical })
  elseif state.noclip then
    set_player({ no_clip = true, fall_distance = 0 })
  elseif state.fall_proof then
    set_player({ fall_distance = 0 })
  end
end)

minecraft.on("player_travel", { is_local_player = true }, function(event)
  event.speed_multiplier = state.speed
  return event
end)

minecraft.on("entity_tick", {}, function(event)
  if state.freeze and event.entity_type ~= "Player" then
    local value = minecraft.entities.get(event.entity_id)
    if value ~= nil then
      minecraft.entities.set_state(value, { vx = 0, vy = 0, vz = 0 })
    end
  end
  if state.fire_proof and event.entity_type == "Player" then
    local value = minecraft.entities.get(event.entity_id)
    if value ~= nil then
      minecraft.entities.set_state(value, { fire_ticks = 0 })
    end
  end
end)

minecraft.on("attack_damage", {}, function(event)
  if state.instant_kill then
    event.damage = 32767
  elseif state.super_punch then
    event.damage = math.max(1, event.damage * 10)
  end
  return event
end)

minecraft.on("block_interact", { local_player = true }, function(event)
  if state.infinite_items and event.has_item then
    event.item_count = math.max(event.item_count, 64)
    if event.item_damageable then event.item_damage = 0 end
  end
  return event
end)

minecraft.on("mouse_button", { button = 0, pressed = true, priority = 90 }, function(event)
  if not state.instant_mine then return event end
  local hit = minecraft.raycast()
  if hit ~= nil and hit.type == "block" and minecraft.world.harvest_block(hit.block_x, hit.block_y, hit.block_z) then
    event.handled = true
  end
  return event
end)

commands.register({
  name = "alias",
  params = "<name> <command>",
  help = "creates a persistent session command alias",
  run = function(context)
    local name, target = context.rest:match("^(%S+)%s+(.+)$")
    assert(name ~= nil and target ~= nil, "expected an alias name and command")
    assert(target:sub(1, 1) == "/", "the command must start with /")
    aliases[command_name(name)] = target
    context.reply("Alias " .. name .. " set")
  end,
})

commands.register({
  name = "ralias",
  params = "<name>",
  help = "removes a command alias",
  run = function(context)
    local name = command_name(context.args[1])
    assert(aliases[name] ~= nil, "unknown alias")
    aliases[name] = nil
    context.reply("Alias " .. name .. " removed")
  end,
})

commands.register({
  name = "bind",
  params = "<key> <command>",
  help = "binds a command to a Controls-menu keybind",
  run = function(context)
    local key, command = context.rest:match("^(%S+)%s+(.+)$")
    assert(key ~= nil and command ~= nil, "expected a key and command")
    local code = key_codes[key:lower()] or tonumber(key)
    assert(code ~= nil, "unknown key")
    assert(command:sub(1, 1) == "/", "the command must start with /")
    local id = "bind_" .. command_name(key)
    minecraft.keybinds.register(id, { default = code, label = "SPC: " .. key:upper() })
    bindings[id] = { id = id, command = command }
    context.reply("Bound " .. key:upper() .. " to " .. command)
  end,
})

commands.register({
  name = "unbind",
  params = "<key>",
  help = "disables an SPC command keybind",
  run = function(context)
    local id = "bind_" .. command_name(context.args[1])
    assert(bindings[id] ~= nil, "that key has no SPC command binding")
    bindings[id] = nil
    context.reply("SPC binding disabled")
  end,
})

commands.register({
  name = "set",
  params = "<name>",
  help = "stores a waypoint at your position",
  run = function(context)
    local name = command_name(context.args[1])
    local x, y, z = position()
    waypoints[name] = { x = x + 0.5, y = y, z = z + 0.5 }
    context.reply("Waypoint " .. name .. " set")
  end,
})

commands.register({
  name = "rem",
  params = "<name>",
  help = "removes a waypoint",
  run = function(context)
    local name = command_name(context.args[1])
    assert(waypoints[name] ~= nil, "unknown waypoint")
    waypoints[name] = nil
    context.reply("Waypoint " .. name .. " removed")
  end,
})

commands.register({
  name = "listwaypoints",
  aliases = { "l" },
  help = "lists stored waypoints",
  run = function(context)
    local names = {}
    for name in pairs(waypoints) do names[#names + 1] = name end
    table.sort(names)
    context.reply(#names == 0 and "No waypoints set" or "Waypoints: " .. table.concat(names, ", "))
  end,
})

commands.register({
  name = "goto",
  params = "<name>",
  help = "teleports to a waypoint",
  run = function(context)
    local value = waypoints[command_name(context.args[1])]
    assert(value ~= nil, "unknown waypoint")
    teleport(value.x, value.y, value.z)
  end,
})

commands.register({
  name = "sethome",
  help = "sets your home waypoint",
  run = function(context)
    local x, y, z = position()
    waypoints.home = { x = x + 0.5, y = y, z = z + 0.5 }
    context.reply("Home set")
  end,
})

commands.register({
  name = "home",
  help = "teleports home",
  run = function()
    local value = assert(waypoints.home, "home is not set")
    teleport(value.x, value.y, value.z)
  end,
})

commands.register({
  name = "return",
  help = "returns to your previous command teleport",
  run = function()
    local value = assert(waypoints["return"], "no return position saved")
    teleport(value.x, value.y, value.z)
  end,
})

commands.register({
  name = "jump",
  help = "teleports to the block you are looking at",
  run = function()
    local hit = assert(minecraft.raycast(), "nothing targeted")
    assert(hit.type == "block", "target a block")
    teleport(hit.block_x + 0.5, hit.block_y + 1, hit.block_z + 0.5)
  end,
})

commands.register({
  name = "ascend",
  help = "moves to the first open floor above you",
  run = function()
    local x, y, z = position()
    for target = y + 1, 126 do
      if minecraft.world.get_block(x, target, z) == AIR and minecraft.world.get_block(x, target + 1, z) == AIR and
          minecraft.world.get_block(x, target - 1, z) ~= AIR then
        teleport(x + 0.5, target, z + 0.5)
        return
      end
    end
    error("no open floor above")
  end,
})

commands.register({
  name = "descend",
  help = "moves to the first open floor below you",
  run = function()
    local x, y, z = position()
    for target = y - 1, 1, -1 do
      if minecraft.world.get_block(x, target, z) == AIR and minecraft.world.get_block(x, target + 1, z) == AIR and
          minecraft.world.get_block(x, target - 1, z) ~= AIR then
        teleport(x + 0.5, target, z + 0.5)
        return
      end
    end
    error("no open floor below")
  end,
})

commands.register({
  name = "platform",
  help = "builds a glass platform below you",
  run = function(context)
    local x, y, z = position()
    local id = context.args[1] and block_id(context.args[1]) or 20
    for dx = -1, 1 do
      for dz = -1, 1 do
        minecraft.world.set_block(x + dx, y - 1, z + dz, id)
      end
    end
    context.reply("Platform created")
  end,
})

commands.register({
  name = "moveplayer",
  params = "<distance>",
  help = "moves you forward by a distance",
  run = function(context)
    local distance = tonumber(context.args[1]) or 1
    local x, y, z = position()
    local dx, dz = facing_vector()
    teleport(x + dx * distance, y, z + dz * distance)
  end,
})

commands.register({
  name = "setspawn",
  help = "sets the world spawn at your position",
  run = function(context)
    local x, y, z = position()
    assert(minecraft.world.set_spawn(x, y, z), "unable to set world spawn")
    context.reply("World spawn set")
  end,
})

commands.register({
  name = "spawn",
  params = "<entity> [count]",
  help = "spawns entities at your position",
  run = function(context)
    local kind = assert(context.args[1], "expected an entity")
    local count = math.max(1, math.min(64, floor(context.args[2] or 1)))
    local x, y, z = position()
    local spawned = 0
    for _ = 1, count do
      if minecraft.world.spawn_entity(kind, x, y, z) then spawned = spawned + 1 end
    end
    assert(spawned > 0, "unknown entity: " .. kind)
    context.reply("Spawned " .. spawned .. " " .. kind)
  end,
})

commands.register({
  name = "bring",
  params = "<entity>",
  help = "brings matching entities to you",
  run = function(context)
    local wanted = assert(context.args[1], "expected an entity")
    local x, y, z = position()
    local moved = 0
    for _, value in ipairs(entities()) do
      if value.type:lower() == wanted:lower() and value:teleport(x, y, z) then moved = moved + 1 end
    end
    context.reply("Brought " .. moved .. " " .. wanted)
  end,
})

commands.register({
  name = "killall",
  params = "[entity]",
  help = "removes every non-player entity, or one type",
  run = function(context)
    local wanted = context.args[1] and context.args[1]:lower()
    local removed = remove_entities(function(value) return wanted == nil or value.type:lower() == wanted end)
    context.reply("Removed " .. removed .. " entities")
  end,
})

commands.register({
  name = "killnpc",
  aliases = { "exterminate" },
  params = "[entity]",
  help = "removes hostile mobs, or a named entity type",
  run = function(context)
    local wanted = context.args[1] and context.args[1]:lower()
    local removed = remove_entities(function(value)
      return wanted ~= nil and value.type:lower() == wanted or wanted == nil and hostile[value.type] == true
    end)
    context.reply("Removed " .. removed .. " entities")
  end,
})

commands.register({
  name = "defuse",
  help = "removes all primed TNT",
  run = function(context)
    local removed = remove_entities(function(value) return value.type == "Tnt" end)
    context.reply("Defused " .. removed .. " TNT")
  end,
})

commands.register({
  name = "freeze",
  params = "[on|off]",
  help = "freezes or unfreezes non-player entities",
  run = function(context)
    state.freeze = toggle(context.args[1], state.freeze)
    context.reply("Entity freeze " .. (state.freeze and "enabled" or "disabled"))
  end,
})

commands.register({
  name = "heal",
  params = "[amount]",
  help = "restores health and extinguishes fire",
  run = function(context)
    local amount = context.args[1] and floor(context.args[1]) or 20
    set_player({ health = math.max(1, math.min(20, amount)), fire_ticks = 0 })
    context.reply("Health set to " .. math.max(1, math.min(20, amount)))
  end,
})

commands.register({
  name = "health",
  help = "shows current health",
  run = function(context)
    context.reply("Health: " .. player().health)
  end,
})

commands.register({
  name = "extinguish",
  aliases = { "ext" },
  help = "extinguishes you",
  run = function(context)
    set_player({ fire_ticks = 0 })
    context.reply("Extinguished")
  end,
})

commands.register({
  name = "firedamage",
  params = "[on|off]",
  help = "toggles fire damage protection",
  run = function(context)
    state.fire_proof = not toggle(context.args[1], not state.fire_proof)
    context.reply("Fire damage " .. (state.fire_proof and "disabled" or "enabled"))
  end,
})

commands.register({
  name = "fly",
  params = "[on|off]",
  help = "toggles noclip flight; Space rises and Shift descends",
  run = function(context)
    state.fly = toggle(context.args[1], state.fly)
    if not state.fly and not state.noclip then set_player({ no_clip = false }) end
    context.reply("Flight " .. (state.fly and "enabled" or "disabled"))
  end,
})

commands.register({
  name = "noclip",
  params = "[on|off]",
  help = "toggles walking through blocks",
  run = function(context)
    state.noclip = toggle(context.args[1], state.noclip)
    set_player({ no_clip = state.noclip or state.fly, fall_distance = 0 })
    context.reply("Noclip " .. (state.noclip and "enabled" or "disabled"))
  end,
})

commands.register({
  name = "setspeed",
  aliases = { "speed" },
  params = "<multiplier>",
  help = "sets movement speed",
  run = function(context)
    state.speed = math.max(0.05, math.min(20, tonumber(context.args[1]) or 1))
    context.reply("Speed set to " .. state.speed)
  end,
})

commands.register({
  name = "instantkill",
  params = "[on|off]",
  help = "toggles one-hit kills",
  run = function(context)
    state.instant_kill = toggle(context.args[1], state.instant_kill)
    context.reply("Instant kill " .. (state.instant_kill and "enabled" or "disabled"))
  end,
})

commands.register({
  name = "superpunch",
  params = "[on|off]",
  help = "toggles powerful melee attacks",
  run = function(context)
    state.super_punch = toggle(context.args[1], state.super_punch)
    context.reply("Super punch " .. (state.super_punch and "enabled" or "disabled"))
  end,
})

commands.register({
  name = "clear",
  aliases = { "clearinventory" },
  help = "clears your inventory",
  run = function(context)
    clear_inventory()
    context.reply("Inventory cleared")
  end,
})

commands.register({
  name = "duplicate",
  help = "duplicates the selected inventory stack",
  run = function(context)
    local stack = minecraft.inventory.get(minecraft.inventory.selected_slot())
    assert(stack ~= nil and stack.id > 0, "hold an item")
    assert(minecraft.inventory.give(stack), "inventory is full")
    context.reply("Duplicated item " .. stack.id)
  end,
})

commands.register({
  name = "refill",
  help = "refills the selected inventory stack",
  run = function(context)
    local slot = minecraft.inventory.selected_slot()
    local stack = minecraft.inventory.get(slot)
    assert(stack ~= nil and stack.id > 0, "hold an item")
    stack.count = stack.max_count
    assert(minecraft.inventory.set(slot, stack), "unable to refill item")
    context.reply("Item refilled")
  end,
})

commands.register({
  name = "repair",
  help = "repairs the selected inventory item",
  run = function(context)
    local slot = minecraft.inventory.selected_slot()
    local stack = minecraft.inventory.get(slot)
    assert(stack ~= nil and stack.id > 0, "hold an item")
    stack.damage = 0
    assert(minecraft.inventory.set(slot, stack), "unable to repair item")
    context.reply("Item repaired")
  end,
})

commands.register({
  name = "item",
  aliases = { "i" },
  params = "<id> [count] [damage]",
  help = "gives an item",
  run = function(context)
    local id = floor(assert(context.args[1], "expected an item id"))
    local description = assert(minecraft.items.describe(id), "unknown item")
    local count = math.max(1, math.min(description.max_count, floor(context.args[2] or 1)))
    local damage = floor(context.args[3] or 0)
    assert(minecraft.inventory.give({ id = id, count = count, damage = damage }), "inventory is full")
    context.reply("Given " .. count .. " of item " .. id)
  end,
})

commands.register({
  name = "itemdamage",
  params = "[damage]",
  help = "shows or sets selected item damage",
  run = function(context)
    local slot = minecraft.inventory.selected_slot()
    local stack = minecraft.inventory.get(slot)
    assert(stack ~= nil and stack.id > 0, "hold an item")
    if context.args[1] == nil then
      context.reply("Item damage: " .. stack.damage)
      return
    end
    stack.damage = math.max(0, floor(context.args[1]))
    assert(minecraft.inventory.set(slot, stack), "unable to change item damage")
    context.reply("Item damage set")
  end,
})

commands.register({
  name = "itemname",
  help = "shows the selected item id",
  run = function(context)
    local stack = minecraft.inventory.get(minecraft.inventory.selected_slot())
    assert(stack ~= nil and stack.id > 0, "hold an item")
    context.reply("Item id: " .. stack.id)
  end,
})

commands.register({
  name = "helmet",
  params = "<id> [damage]",
  help = "equips an item in your helmet slot",
  run = function(context)
    local id = floor(assert(context.args[1], "expected an item id"))
    assert(minecraft.items.describe(id) ~= nil, "unknown item")
    assert(minecraft.inventory.set(minecraft.inventory.main_size() + 3, { id = id, count = 1, damage = floor(context.args[2] or 0) }), "unable to equip helmet")
    context.reply("Helmet equipped")
  end,
})

commands.register({
  name = "drops",
  aliases = { "removedrops" },
  help = "removes dropped item entities",
  run = function(context)
    local removed = remove_entities(function(value) return value.type == "Item" end)
    context.reply("Removed " .. removed .. " drops")
  end,
})

commands.register({
  name = "chest",
  help = "places a chest at the targeted block",
  run = function(context)
    local hit = assert(minecraft.raycast(), "nothing targeted")
    assert(hit.type == "block", "target a block")
    assert(minecraft.world.set_block(hit.block_x, hit.block_y + 1, hit.block_z, 54), "unable to place chest")
    context.reply("Chest placed")
  end,
})

commands.register({
  name = "clearwater",
  params = "[radius]",
  help = "removes water around you",
  run = function(context)
    local radius = math.max(1, math.min(32, floor(context.args[1] or 8)))
    local x, y, z = position()
    local removed = 0
    for dy = -radius, radius do
      for dz = -radius, radius do
        for dx = -radius, radius do
          if WATER[minecraft.world.get_block(x + dx, y + dy, z + dz)] then
            if minecraft.world.set_block(x + dx, y + dy, z + dz, AIR) then removed = removed + 1 end
          end
        end
      end
    end
    context.reply("Removed " .. removed .. " water blocks")
  end,
})

commands.register({
  name = "atlantis",
  params = "[radius]",
  help = "fills a sea-level area with water",
  run = function(context)
    local radius = math.max(1, math.min(32, floor(context.args[1] or 16)))
    local x, _, z = position()
    local changed = 0
    for dz = -radius, radius do
      for dx = -radius, radius do
        if dx * dx + dz * dz <= radius * radius and minecraft.world.get_block(x + dx, 63, z + dz) == AIR then
          if minecraft.world.set_block(x + dx, 63, z + dz, 8) then changed = changed + 1 end
        end
      end
    end
    context.reply("Atlantis filled " .. changed .. " blocks")
  end,
})

commands.register({
  name = "superheat",
  params = "[radius]",
  help = "turns nearby stone and sand into heated variants",
  run = function(context)
    local radius = math.max(1, math.min(16, floor(context.args[1] or 4)))
    local x, y, z = position()
    local changed = 0
    for dy = -radius, radius do
      for dz = -radius, radius do
        for dx = -radius, radius do
          local id = minecraft.world.get_block(x + dx, y + dy, z + dz)
          local replacement = id == 1 and 4 or id == 12 and 20 or nil
          if replacement ~= nil and minecraft.world.set_block(x + dx, y + dy, z + dz, replacement) then changed = changed + 1 end
        end
      end
    end
    context.reply("Superheated " .. changed .. " blocks")
  end,
})

commands.register({
  name = "weather",
  params = "<clear|rain|thunder>",
  help = "sets the weather",
  run = function(context)
    local value = assert(context.args[1], "expected clear, rain, or thunder")
    assert(minecraft.world.set_weather(value), "unknown weather")
    context.reply("Weather set to " .. value)
  end,
})

commands.register({
  name = "difficulty",
  params = "<peaceful|easy|normal|hard>",
  help = "sets world difficulty",
  run = function(context)
    local values = { peaceful = 0, easy = 1, normal = 2, hard = 3 }
    local value = values[tostring(context.args[1] or ""):lower()] or tonumber(context.args[1])
    assert(value ~= nil and minecraft.world.set_difficulty(value), "expected peaceful, easy, normal, or hard")
    context.reply("Difficulty set")
  end,
})

commands.register({
  name = "biome",
  help = "shows the biome at your position",
  run = function(context)
    local x, _, z = position()
    context.reply("Biome: " .. tostring(minecraft.world.get_biome(x, z)))
  end,
})

commands.register({
  name = "cannon",
  params = "[power]",
  help = "fires a primed TNT cannonball",
  run = function(context)
    local power = math.max(0.1, math.min(10, tonumber(context.args[1]) or 2))
    local x, y, z = position()
    local dx, dz = facing_vector()
    assert(minecraft.world.spawn_entity("Tnt", x + dx, y + 1, z + dz), "unable to spawn TNT")
    local latest = nil
    for _, value in ipairs(entities("Tnt")) do latest = value end
    if latest ~= nil then minecraft.entities.set_state(latest, { vx = dx * power, vy = power * 0.35, vz = dz * power }) end
    context.reply("Cannon fired")
  end,
})

commands.register({
  name = "explode",
  params = "[power]",
  help = "launches a primed TNT blast in front of you",
  run = function(context)
    commands.execute("/cannon " .. tostring(context.args[1] or 2))
  end,
})

commands.register({
  name = "falldamage",
  params = "[on|off]",
  help = "toggles fall damage",
  run = function(context)
    state.fall_proof = not toggle(context.args[1], not state.fall_proof)
    context.reply("Fall damage " .. (state.fall_proof and "disabled" or "enabled"))
  end,
})

commands.register({
  name = "instantmine",
  params = "[on|off]",
  help = "toggles instant block mining",
  run = function(context)
    state.instant_mine = toggle(context.args[1], state.instant_mine)
    context.reply("Instant mine " .. (state.instant_mine and "enabled" or "disabled"))
  end,
})

commands.register({
  name = "freecam",
  params = "[on|off]",
  help = "toggles free noclip flight controls",
  run = function(context)
    state.fly = toggle(context.args[1], state.fly)
    context.reply("Freecam flight " .. (state.fly and "enabled" or "disabled"))
  end,
})

commands.register({
  name = "time",
  params = "<day|night|set|add> [ticks]",
  help = "sets or adds world time",
  run = function(context)
    local mode = tostring(context.args[1] or ""):lower()
    local named = { day = 0, noon = 6000, night = 13000, midnight = 18000 }
    if named[mode] ~= nil then
      local now = minecraft.world.get_time()
      assert(minecraft.world.set_time(now - now % 24000 + named[mode]), "unable to set time")
      context.reply("Time set to " .. mode)
      return
    end
    local ticks = tonumber(context.args[2])
    assert((mode == "set" or mode == "add") and ticks ~= nil, "expected day, night, set <ticks>, or add <ticks>")
    if mode == "add" then ticks = minecraft.world.get_time() + ticks end
    assert(ticks >= 0 and minecraft.world.set_time(ticks), "unable to set time")
    context.reply("Time set to " .. floor(ticks))
  end,
})

commands.register({
  name = "timeschedule",
  params = "<day|night|ticks>",
  help = "sets the next scheduled world time",
  run = function(context)
    commands.execute("/time " .. assert(context.args[1], "expected a time"))
  end,
})

commands.register({
  name = "tele",
  aliases = { "t" },
  params = "<x> <y> <z>",
  help = "teleports to coordinates",
  run = function(context)
    assert(#context.args >= 3, "expected x y z")
    teleport(tonumber(context.args[1]), tonumber(context.args[2]), tonumber(context.args[3]))
  end,
})

commands.register({
  name = "p",
  help = "shows your position",
  run = function(context)
    local x, y, z = position()
    context.reply(string.format("%.2f %.2f %.2f", x, y, z))
  end,
})

commands.register({
  name = "setjump",
  help = "sets the jump return position",
  run = function(context)
    local x, y, z = position()
    waypoints.jump = { x = x + 0.5, y = y, z = z + 0.5 }
    context.reply("Jump position set")
  end,
})

commands.register({
  name = "unstuck",
  help = "moves you to the first open space above",
  run = function()
    local x, y, z = position()
    for target = y, 126 do
      if minecraft.world.get_block(x, target, z) == AIR and minecraft.world.get_block(x, target + 1, z) == AIR then
        teleport(x + 0.5, target, z + 0.5)
        return
      end
    end
    error("no open space found")
  end,
})

commands.register({
  name = "kill",
  help = "kills your player",
  run = function(context)
    set_player({ health = 0 })
    context.reply("Killed player")
  end,
})

commands.register({
  name = "damage",
  params = "<amount>",
  help = "damages your player",
  run = function(context)
    local amount = math.max(0, floor(assert(context.args[1], "expected damage")))
    set_player({ health = math.max(0, player().health - amount) })
    context.reply("Applied " .. amount .. " damage")
  end,
})

commands.register({
  name = "infiniteitems",
  params = "[on|off]",
  help = "restores used block stacks and item durability",
  run = function(context)
    state.infinite_items = toggle(context.args[1], state.infinite_items)
    context.reply("Infinite items " .. (state.infinite_items and "enabled" or "disabled"))
  end,
})

commands.register({
  name = "maxstack",
  help = "fills the selected item stack to its maximum",
  run = function(context)
    commands.execute("/refill")
  end,
})

commands.register({
  name = "drop",
  help = "drops the selected inventory stack",
  run = function(context)
    local slot = minecraft.inventory.selected_slot()
    local stack = minecraft.inventory.get(slot)
    assert(stack ~= nil and stack.id > 0, "hold an item")
    local x, y, z = position()
    assert(minecraft.drop_item(stack.id, stack.count, x, y, z), "unable to drop item")
    minecraft.inventory.set(slot, { id = 0, count = 0 })
    context.reply("Dropped item stack")
  end,
})

commands.register({
  name = "dropstore",
  help = "clears dropped items in the world",
  run = function(context)
    local removed = remove_entities(function(value) return value.type == "Item" end)
    context.reply("Stored cleanup removed " .. removed .. " drops")
  end,
})

commands.register({
  name = "light",
  help = "places a torch above the targeted block",
  run = function(context)
    local hit = assert(minecraft.raycast(), "nothing targeted")
    assert(hit.type == "block", "target a block")
    assert(minecraft.world.set_block(hit.block_x, hit.block_y + 1, hit.block_z, 50), "unable to place torch")
    context.reply("Torch placed")
  end,
})

commands.register({
  name = "grow",
  help = "fully grows the targeted crop",
  run = function(context)
    local hit = assert(minecraft.raycast(), "nothing targeted")
    assert(hit.type == "block", "target a block")
    local id = minecraft.world.get_block(hit.block_x, hit.block_y, hit.block_z)
    assert(id == 59, "target a crop")
    assert(minecraft.world.set_block(hit.block_x, hit.block_y, hit.block_z, id, 7), "unable to grow crop")
    context.reply("Crop grown")
  end,
})

commands.register({
  name = "instantplant",
  help = "grows the targeted crop immediately",
  run = function(context)
    commands.execute("/grow")
  end,
})

commands.register({
  name = "spawnportal",
  help = "builds a nether portal in front of you",
  run = function(context)
    local x, y, z = position()
    local dx, dz = facing_vector()
    local sx = floor(x + dx * 3)
    local sz = floor(z + dz * 3)
    for py = 0, 4 do
      for px = -1, 2 do
        local edge = px == -1 or px == 2 or py == 0 or py == 4
        minecraft.world.set_block(sx + px, y + py, sz, edge and 49 or 90)
      end
    end
    context.reply("Portal created")
  end,
})

commands.register({
  name = "clearwater",
  aliases = { "cw" },
  params = "[radius]",
  help = "removes water around you",
  run = function(context)
    local radius = math.max(1, math.min(32, floor(context.args[1] or 8)))
    local x, y, z = position()
    local removed = 0
    for dy = -radius, radius do
      for dz = -radius, radius do
        for dx = -radius, radius do
          if WATER[minecraft.world.get_block(x + dx, y + dy, z + dz)] and minecraft.world.set_block(x + dx, y + dy, z + dz, AIR) then
            removed = removed + 1
          end
        end
      end
    end
    context.reply("Removed " .. removed .. " water blocks")
  end,
})

commands.register({
  name = "lava",
  params = "[radius]",
  help = "removes lava around you",
  run = function(context)
    local radius = math.max(1, math.min(32, floor(context.args[1] or 8)))
    local x, y, z = position()
    local removed = 0
    for dy = -radius, radius do
      for dz = -radius, radius do
        for dx = -radius, radius do
          if LAVA[minecraft.world.get_block(x + dx, y + dy, z + dz)] and minecraft.world.set_block(x + dx, y + dy, z + dz, AIR) then
            removed = removed + 1
          end
        end
      end
    end
    context.reply("Removed " .. removed .. " lava blocks")
  end,
})

commands.register({
  name = "phelp",
  params = "[command]",
  help = "shows SPC command help",
  run = function(context)
    if context.args[1] == nil then
      commands.execute("/help")
    else
      context.reply("Use /help and look for /" .. context.args[1]:gsub("^/", ""))
    end
  end,
})

commands.register({
  name = "config",
  help = "shows active SPC toggles",
  run = function(context)
    context.reply("fly=" .. tostring(state.fly) .. " noclip=" .. tostring(state.noclip) .. " speed=" .. state.speed .. " freeze=" .. tostring(state.freeze))
  end,
})
