-- Single Player Commands for Beta 1.7.3.
--
-- Flight and noclip both ride the engine's player_travel hook. event.flying selects
-- PlayerMovement::travelFlying, which has no gravity term, so the mod only has to drive
-- velocityY; setting no_clip on its own just makes you sink through the floor forever.
-- Vertical input is read from the player's own jumping/sneaking state rather than the raw
-- keyboard, because the raw keys stay live while chat is open and a typed space would
-- otherwise launch you.

local commands = require("lib.commands")

local AIR = 0
local WATER = { [8] = true, [9] = true }
local LAVA = { [10] = true, [11] = true }
local WORLD_TOP = 127
local FLIGHT_RATE = 0.42
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
local armor_slots = { helmet = 3, chestplate = 2, leggings = 1, boots = 0 }
local named_times = { day = 0, noon = 6000, night = 13000, midnight = 18000 }
local difficulties = { peaceful = 0, easy = 1, normal = 2, hard = 3 }

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

local category = "SPC"
local function register(spec)
  spec.category = spec.category or category
  return commands.register(spec)
end

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
  return value.x, value.y, value.z, value
end

local function block_pos()
  local x, y, z, value = position()
  return floor(x), floor(y), floor(z), value
end

local function set_player(values)
  assert(minecraft.entities.set_state(player(), values), "unable to change player state")
end

local function teleport(x, y, z)
  local value = player()
  waypoints["return"] = { x = value.x, y = value.y, z = value.z }
  assert(value:teleport(x, y, z), "teleport failed")
  minecraft.entities.set_state(value, { vx = 0, vy = 0, vz = 0, fall_distance = 0 })
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
    local name = tostring(value):lower()
    id = minecraft.world.block_id(name)
    assert(id ~= 0 or name == "air", "unknown block: " .. tostring(value))
  end
  return floor(id)
end

local function item_id(value)
  assert(value ~= nil, "expected an item")
  local id = tonumber(value)
  if id == nil then
    id = minecraft.world.block_id(tostring(value):lower())
    assert(id ~= 0, "unknown item: " .. tostring(value))
  end
  id = floor(id)
  assert(minecraft.items.describe(id) ~= nil, "unknown item: " .. tostring(value))
  return id
end

local function facing_vector()
  local value = player()
  local angle = math.rad(value.yaw)
  return -math.sin(angle), math.cos(angle)
end

local function held_stack()
  local slot = minecraft.inventory.selected_slot()
  local stack = minecraft.inventory.get(slot)
  assert(stack ~= nil and stack.id > 0, "hold an item")
  return slot, stack
end

local function targeted_block()
  local hit = assert(minecraft.raycast(), "nothing targeted")
  assert(hit.type == "block", "target a block")
  return hit.block_x, hit.block_y, hit.block_z, hit
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

local function plural(count, noun)
  return count .. " " .. noun .. (count == 1 and "" or "s")
end

-- Waypoints, aliases and keybinds outlive the session; SPC without that is a scratchpad.
local STORE = "spc.txt"

local function save()
  local lines = {}
  for name, point in pairs(waypoints) do
    if name ~= "return" then
      lines[#lines + 1] = string.format("waypoint %s %.3f %.3f %.3f", name, point.x, point.y, point.z)
    end
  end
  for name, target in pairs(aliases) do
    lines[#lines + 1] = "alias " .. name .. " " .. target
  end
  for _, binding in pairs(bindings) do
    lines[#lines + 1] = "bind " .. binding.key .. " " .. binding.command
  end
  table.sort(lines)
  minecraft.storage.write(STORE, table.concat(lines, "\n"))
end

local function bind_key(key, command)
  local code = key_codes[key:lower()] or tonumber(key)
  assert(code ~= nil, "unknown key: " .. key)
  local id = "bind_" .. command_name(key)
  minecraft.keybinds.register(id, { default = code, label = "SPC: " .. key:upper() })
  bindings[id] = { id = id, key = key:lower(), command = command }
  return id
end

local function load()
  local text = minecraft.storage.read(STORE)
  if text == nil then
    return
  end
  for line in text:gmatch("[^\r\n]+") do
    local kind, rest = line:match("^(%S+)%s+(.+)$")
    if kind == "waypoint" then
      local name, x, y, z = rest:match("^(%S+)%s+(%S+)%s+(%S+)%s+(%S+)$")
      if name ~= nil and tonumber(z) ~= nil then
        waypoints[name] = { x = tonumber(x), y = tonumber(y), z = tonumber(z) }
      end
    elseif kind == "alias" then
      local name, target = rest:match("^(%S+)%s+(.+)$")
      if name ~= nil then
        aliases[name] = target
      end
    elseif kind == "bind" then
      local key, command = rest:match("^(%S+)%s+(.+)$")
      if key ~= nil then
        pcall(bind_key, key, command)
      end
    end
  end
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

local function break_targeted_block()
  local hit = minecraft.raycast()
  if hit == nil or hit.type ~= "block" then
    return false
  end
  return minecraft.world.harvest_block(hit.block_x, hit.block_y, hit.block_z) == true
end

minecraft.on("client_tick", { before = true }, function(event)
  if event.has_screen == true or event.world_remote == true then
    return event
  end
  for _, binding in pairs(bindings) do
    if minecraft.keybinds.consume(binding.id) then
      commands.execute(binding.command)
    end
  end
  if state.instant_mine and minecraft.is_mouse_down(0) then
    break_targeted_block()
  end
  return event
end)

-- The whole of flight: travelFlying drops gravity, and vy is re-driven every tick because
-- travelFlyingAir damps it by the air slipperiness after each move.
minecraft.on("player_travel", { is_local_player = true }, function(event)
  event.speed_multiplier = event.speed_multiplier * state.speed
  local flying = state.fly or state.noclip
  if not flying then
    if state.fall_proof then
      minecraft.entities.set_state(player(), { fall_distance = 0 })
    end
    return event
  end
  event.flying = true
  local value = player()
  local vertical = 0
  if value.jumping then
    vertical = FLIGHT_RATE * state.speed
  elseif value.sneaking then
    vertical = -FLIGHT_RATE * state.speed
  end
  minecraft.entities.set_state(value, {
    vy = vertical,
    fall_distance = 0,
    no_clip = state.noclip,
  })
  return event
end)

minecraft.on("entity_tick", {}, function(event)
  if state.freeze and event.entity_type ~= "Player" then
    local value = minecraft.entities.get(event.entity_id)
    if value ~= nil then
      minecraft.entities.set_state(value, { vx = 0, vy = 0, vz = 0 })
    end
  elseif state.fire_proof and event.entity_type == "Player" then
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
  if state.instant_mine and break_targeted_block() then
    event.handled = true
  end
  return event
end)

--------------------------------------------------------------------------------
category = "SPC: movement"

register({
  name = "fly",
  params = "[on|off]",
  help = "flight with no gravity; jump rises, sneak drops",
  details = "blocks still stop you -- use /noclip to pass through them",
  run = function(context)
    state.fly = toggle(context.args[1], state.fly)
    if not state.fly and not state.noclip then
      set_player({ no_clip = false, fall_distance = 0 })
    end
    context.reply("Flight " .. (state.fly and "enabled" or "disabled"))
  end,
})

register({
  name = "noclip",
  aliases = { "nc" },
  params = "[on|off]",
  help = "flight that passes through blocks",
  details = "implies flight, otherwise you would sink out of the world",
  run = function(context)
    state.noclip = toggle(context.args[1], state.noclip)
    set_player({ no_clip = state.noclip, fall_distance = 0 })
    context.reply("Noclip " .. (state.noclip and "enabled" or "disabled"))
  end,
})

register({
  name = "setspeed",
  aliases = { "speed" },
  params = "<multiplier>",
  help = "scales walking and flying speed",
  run = function(context)
    state.speed = math.max(0.05, math.min(20, tonumber(context.args[1]) or 1))
    context.reply("Speed set to " .. state.speed)
  end,
})

register({
  name = "tele",
  aliases = { "t" },
  params = "<x> <y> <z>",
  help = "teleports to coordinates",
  run = function(context)
    assert(#context.args >= 3, "expected x y z")
    local x = assert(tonumber(context.args[1]), "expected a number for x")
    local y = assert(tonumber(context.args[2]), "expected a number for y")
    local z = assert(tonumber(context.args[3]), "expected a number for z")
    teleport(x, y, z)
    context.reply(string.format("Teleported to %.1f %.1f %.1f", x, y, z))
  end,
})

register({
  name = "jump",
  help = "teleports onto the block you are looking at",
  run = function(context)
    local x, y, z = targeted_block()
    teleport(x + 0.5, y + 1, z + 0.5)
    context.reply("Jumped to the targeted block")
  end,
})

register({
  name = "ascend",
  help = "teleports to the next open floor above you",
  run = function(context)
    local x, y, z = block_pos()
    for target = y + 1, WORLD_TOP - 1 do
      if minecraft.world.get_block(x, target, z) == AIR and minecraft.world.get_block(x, target + 1, z) == AIR and
          minecraft.world.get_block(x, target - 1, z) ~= AIR then
        teleport(x + 0.5, target, z + 0.5)
        context.reply("Ascended to y " .. target)
        return
      end
    end
    error("no open floor above")
  end,
})

register({
  name = "descend",
  help = "teleports to the next open floor below you",
  run = function(context)
    local x, y, z = block_pos()
    for target = y - 1, 1, -1 do
      if minecraft.world.get_block(x, target, z) == AIR and minecraft.world.get_block(x, target + 1, z) == AIR and
          minecraft.world.get_block(x, target - 1, z) ~= AIR then
        teleport(x + 0.5, target, z + 0.5)
        context.reply("Descended to y " .. target)
        return
      end
    end
    error("no open floor below")
  end,
})

register({
  name = "unstuck",
  help = "teleports out of the blocks you are inside",
  run = function(context)
    local x, y, z = block_pos()
    for target = y, WORLD_TOP - 1 do
      if minecraft.world.get_block(x, target, z) == AIR and minecraft.world.get_block(x, target + 1, z) == AIR then
        teleport(x + 0.5, target, z + 0.5)
        context.reply("Unstuck at y " .. target)
        return
      end
    end
    error("no open space found")
  end,
})

register({
  name = "moveplayer",
  params = "<distance> [up|down|forward|back|left|right]",
  help = "shoves you a set distance",
  run = function(context)
    local distance = tonumber(context.args[1]) or 1
    local direction = tostring(context.args[2] or "forward"):lower()
    local x, y, z = position()
    local dx, dz = facing_vector()
    if direction == "up" then
      teleport(x, y + distance, z)
    elseif direction == "down" then
      teleport(x, y - distance, z)
    elseif direction == "back" then
      teleport(x - dx * distance, y, z - dz * distance)
    elseif direction == "left" then
      teleport(x + dz * distance, y, z - dx * distance)
    elseif direction == "right" then
      teleport(x - dz * distance, y, z + dx * distance)
    else
      assert(direction == "forward", "expected up, down, forward, back, left, or right")
      teleport(x + dx * distance, y, z + dz * distance)
    end
    context.reply("Moved " .. distance .. " " .. direction)
  end,
})

register({
  name = "platform",
  params = "[block]",
  help = "builds a 3x3 platform under your feet",
  run = function(context)
    local x, y, z = block_pos()
    local id = context.args[1] and block_id(context.args[1]) or 20
    for dx = -1, 1 do
      for dz = -1, 1 do
        minecraft.world.set_block(x + dx, y - 1, z + dz, id)
      end
    end
    context.reply("Platform created")
  end,
})

register({
  name = "p",
  aliases = { "position" },
  help = "shows your exact position, facing, and biome",
  run = function(context)
    local x, y, z, value = position()
    local bx, _, bz = block_pos()
    context.reply(string.format("%.2f %.2f %.2f", x, y, z))
    context.reply(string.format("yaw %.1f pitch %.1f, %s", value.yaw, value.pitch,
        tostring(minecraft.world.get_biome(bx, bz))))
  end,
})

--------------------------------------------------------------------------------
category = "SPC: waypoints"

register({
  name = "set",
  params = "<name>",
  help = "stores a waypoint where you stand",
  run = function(context)
    local name = command_name(context.args[1])
    local x, y, z = position()
    waypoints[name] = { x = x, y = y, z = z }
    save()
    context.reply("Waypoint " .. name .. " set")
  end,
})

register({
  name = "rem",
  params = "<name>",
  help = "removes a waypoint",
  run = function(context)
    local name = command_name(context.args[1])
    assert(waypoints[name] ~= nil, "unknown waypoint")
    waypoints[name] = nil
    save()
    context.reply("Waypoint " .. name .. " removed")
  end,
})

register({
  name = "goto",
  params = "<name>",
  help = "teleports to a waypoint",
  run = function(context)
    local name = command_name(context.args[1])
    local value = assert(waypoints[name], "unknown waypoint")
    teleport(value.x, value.y, value.z)
    context.reply("Went to " .. name)
  end,
})

register({
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

register({
  name = "sethome",
  help = "sets your home waypoint",
  run = function(context)
    local x, y, z = position()
    waypoints.home = { x = x, y = y, z = z }
    save()
    context.reply("Home set")
  end,
})

register({
  name = "home",
  help = "teleports home",
  run = function(context)
    local value = assert(waypoints.home, "home is not set -- use /sethome")
    teleport(value.x, value.y, value.z)
    context.reply("Went home")
  end,
})

register({
  name = "setjump",
  help = "stores the jump waypoint where you stand",
  run = function(context)
    local x, y, z = position()
    waypoints.jump = { x = x, y = y, z = z }
    save()
    context.reply("Jump waypoint set")
  end,
})

register({
  name = "return",
  help = "returns to where your last teleport started",
  run = function(context)
    local value = assert(waypoints["return"], "no return position saved")
    teleport(value.x, value.y, value.z)
    context.reply("Returned")
  end,
})

--------------------------------------------------------------------------------
category = "SPC: player"

register({
  name = "heal",
  params = "[amount]",
  help = "restores health and puts out fire",
  run = function(context)
    local amount = math.max(1, math.min(20, context.args[1] and floor(context.args[1]) or 20))
    set_player({ health = amount, fire_ticks = 0 })
    context.reply("Health set to " .. amount)
  end,
})

register({
  name = "health",
  params = "[amount]",
  help = "shows or sets your health",
  run = function(context)
    if context.args[1] == nil then
      context.reply("Health: " .. player().health .. "/20")
      return
    end
    local amount = math.max(0, math.min(20, floor(context.args[1])))
    set_player({ health = amount })
    context.reply("Health set to " .. amount)
  end,
})

register({
  name = "damage",
  params = "<amount>",
  help = "takes health off you",
  run = function(context)
    local amount = math.max(0, floor(assert(context.args[1], "expected an amount")))
    set_player({ health = math.max(0, player().health - amount) })
    context.reply("Applied " .. plural(amount, "point") .. " of damage")
  end,
})

register({
  name = "kill",
  help = "kills your player",
  run = function(context)
    set_player({ health = 0 })
    context.reply("Killed player")
  end,
})

register({
  name = "extinguish",
  aliases = { "ext" },
  help = "puts you out",
  run = function(context)
    set_player({ fire_ticks = 0 })
    context.reply("Extinguished")
  end,
})

register({
  name = "falldamage",
  params = "[on|off]",
  help = "toggles fall damage",
  run = function(context)
    state.fall_proof = not toggle(context.args[1], not state.fall_proof)
    context.reply("Fall damage " .. (state.fall_proof and "disabled" or "enabled"))
  end,
})

register({
  name = "firedamage",
  params = "[on|off]",
  help = "toggles burning",
  run = function(context)
    state.fire_proof = not toggle(context.args[1], not state.fire_proof)
    context.reply("Fire damage " .. (state.fire_proof and "disabled" or "enabled"))
  end,
})

register({
  name = "instantkill",
  params = "[on|off]",
  help = "toggles one-hit kills",
  run = function(context)
    state.instant_kill = toggle(context.args[1], state.instant_kill)
    context.reply("Instant kill " .. (state.instant_kill and "enabled" or "disabled"))
  end,
})

register({
  name = "superpunch",
  params = "[on|off]",
  help = "toggles ten times melee damage",
  run = function(context)
    state.super_punch = toggle(context.args[1], state.super_punch)
    context.reply("Super punch " .. (state.super_punch and "enabled" or "disabled"))
  end,
})

register({
  name = "instantmine",
  params = "[on|off]",
  help = "breaks blocks the moment you hit them",
  run = function(context)
    state.instant_mine = toggle(context.args[1], state.instant_mine)
    context.reply("Instant mine " .. (state.instant_mine and "enabled" or "disabled"))
  end,
})

register({
  name = "infiniteitems",
  params = "[on|off]",
  help = "stops placed blocks and tool wear from using up stacks",
  run = function(context)
    state.infinite_items = toggle(context.args[1], state.infinite_items)
    context.reply("Infinite items " .. (state.infinite_items and "enabled" or "disabled"))
  end,
})

--------------------------------------------------------------------------------
category = "SPC: items"

register({
  name = "item",
  aliases = { "i" },
  params = "<id|block> [count] [damage]",
  help = "puts an item in your inventory",
  details = "block names work too: /item diamond_block 64",
  run = function(context)
    local id = item_id(context.args[1])
    local description = minecraft.items.describe(id)
    local count = math.max(1, math.min(description.max_count, floor(context.args[2] or 1)))
    local damage = floor(context.args[3] or 0)
    assert(minecraft.inventory.give({ id = id, count = count, damage = damage }), "inventory is full")
    context.reply("Given " .. count .. " of item " .. id)
  end,
})

register({
  name = "clear",
  aliases = { "clearinventory" },
  help = "empties your inventory and armour",
  run = function(context)
    for slot = 0, minecraft.inventory.slot_count() - 1 do
      minecraft.inventory.set(slot, { id = 0, count = 0 })
    end
    context.reply("Inventory cleared")
  end,
})

register({
  name = "duplicate",
  aliases = { "dupe" },
  help = "copies the stack you are holding",
  run = function(context)
    local _, stack = held_stack()
    assert(minecraft.inventory.give(stack), "inventory is full")
    context.reply("Duplicated item " .. stack.id)
  end,
})

register({
  name = "refill",
  aliases = { "maxstack" },
  help = "fills the held stack to its maximum",
  run = function(context)
    local slot, stack = held_stack()
    stack.count = stack.max_count
    assert(minecraft.inventory.set(slot, stack), "unable to refill item")
    context.reply("Refilled to " .. stack.count)
  end,
})

register({
  name = "repair",
  help = "repairs the held item",
  run = function(context)
    local slot, stack = held_stack()
    stack.damage = 0
    assert(minecraft.inventory.set(slot, stack), "unable to repair item")
    context.reply("Item repaired")
  end,
})

register({
  name = "itemdamage",
  params = "[damage]",
  help = "shows or sets the held item's damage",
  run = function(context)
    local slot, stack = held_stack()
    if context.args[1] == nil then
      context.reply("Item damage: " .. stack.damage .. "/" .. stack.max_damage)
      return
    end
    stack.damage = math.max(0, floor(context.args[1]))
    assert(minecraft.inventory.set(slot, stack), "unable to change item damage")
    context.reply("Item damage set to " .. stack.damage)
  end,
})

register({
  name = "itemname",
  help = "shows the held item's id and stack size",
  run = function(context)
    local _, stack = held_stack()
    context.reply("Item " .. stack.id .. " x" .. stack.count .. ", damage " .. stack.damage)
  end,
})

register({
  name = "armor",
  aliases = { "armour" },
  params = "<helmet|chestplate|leggings|boots> <id> [damage]",
  help = "equips an item in an armour slot",
  run = function(context)
    local piece = tostring(context.args[1] or ""):lower()
    local offset = assert(armor_slots[piece], "expected helmet, chestplate, leggings, or boots")
    local id = item_id(context.args[2])
    local slot = minecraft.inventory.main_size() + offset
    assert(minecraft.inventory.set(slot, { id = id, count = 1, damage = floor(context.args[3] or 0) }),
        "unable to equip " .. piece)
    context.reply(piece:gsub("^%l", string.upper) .. " equipped")
  end,
})

register({
  name = "helmet",
  params = "<id> [damage]",
  help = "equips an item on your head",
  run = function(context)
    commands.execute("/armor helmet " .. table.concat(context.args, " "))
  end,
})

register({
  name = "drop",
  help = "drops the stack you are holding",
  run = function(context)
    local slot, stack = held_stack()
    local x, y, z = position()
    assert(minecraft.drop_item(stack.id, stack.count, x, y + 1, z), "unable to drop item")
    minecraft.inventory.set(slot, { id = 0, count = 0 })
    context.reply("Dropped item stack")
  end,
})

register({
  name = "dropstore",
  help = "drops your whole inventory at your feet",
  run = function(context)
    local x, y, z = position()
    local dropped = 0
    for slot = 0, minecraft.inventory.slot_count() - 1 do
      local stack = minecraft.inventory.get(slot)
      if stack ~= nil and stack.id > 0 and minecraft.drop_item(stack.id, stack.count, x, y + 1, z) then
        minecraft.inventory.set(slot, { id = 0, count = 0 })
        dropped = dropped + 1
      end
    end
    context.reply("Dropped " .. plural(dropped, "stack"))
  end,
})

--------------------------------------------------------------------------------
category = "SPC: world"

register({
  name = "time",
  params = "<day|noon|night|midnight|set|add> [ticks]",
  help = "sets or advances the world time",
  run = function(context)
    local mode = tostring(context.args[1] or ""):lower()
    if named_times[mode] ~= nil then
      local now = minecraft.world.get_time()
      assert(minecraft.world.set_time(now - now % 24000 + named_times[mode]), "unable to set time")
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

register({
  name = "day",
  help = "sets the time to sunrise",
  run = function(context) commands.execute("/time day") end,
})

register({
  name = "night",
  help = "sets the time to nightfall",
  run = function(context) commands.execute("/time night") end,
})

register({
  name = "weather",
  params = "<clear|rain|thunder>",
  help = "sets the weather",
  run = function(context)
    local value = tostring(assert(context.args[1], "expected clear, rain, or thunder")):lower()
    assert(minecraft.world.set_weather(value), "expected clear, rain, or thunder")
    context.reply("Weather set to " .. value)
  end,
})

register({
  name = "sun",
  help = "clears the weather",
  run = function(context) commands.execute("/weather clear") end,
})

register({
  name = "storm",
  help = "starts a thunderstorm",
  run = function(context) commands.execute("/weather thunder") end,
})

register({
  name = "difficulty",
  params = "<peaceful|easy|normal|hard>",
  help = "sets the world difficulty",
  run = function(context)
    local value = difficulties[tostring(context.args[1] or ""):lower()] or tonumber(context.args[1])
    assert(value ~= nil and minecraft.world.set_difficulty(value), "expected peaceful, easy, normal, or hard")
    context.reply("Difficulty set")
  end,
})

register({
  name = "biome",
  help = "shows the biome you are standing in",
  run = function(context)
    local x, _, z = block_pos()
    context.reply("Biome: " .. tostring(minecraft.world.get_biome(x, z)))
  end,
})

register({
  name = "setspawn",
  help = "moves the world spawn to your position",
  run = function(context)
    local x, y, z = block_pos()
    assert(minecraft.world.set_spawn(x, y, z), "unable to set world spawn")
    context.reply("World spawn set to " .. x .. " " .. y .. " " .. z)
  end,
})

register({
  name = "worldspawn",
  help = "teleports to the world spawn",
  run = function(context)
    local spawn = assert(minecraft.world.get_spawn(), "this world has no spawn")
    teleport(spawn.x + 0.5, spawn.y, spawn.z + 0.5)
    context.reply("Went to the world spawn")
  end,
})

register({
  name = "destroy",
  help = "breaks the block you are looking at",
  run = function(context)
    local x, y, z = targeted_block()
    assert(minecraft.world.harvest_block(x, y, z), "unable to break that block")
    context.reply("Block destroyed")
  end,
})

register({
  name = "light",
  help = "places a torch on top of the targeted block",
  run = function(context)
    local x, y, z = targeted_block()
    assert(minecraft.world.set_block(x, y + 1, z, 50), "unable to place torch")
    context.reply("Torch placed")
  end,
})

register({
  name = "chest",
  help = "places a chest on top of the targeted block",
  run = function(context)
    local x, y, z = targeted_block()
    assert(minecraft.world.set_block(x, y + 1, z, 54), "unable to place chest")
    context.reply("Chest placed")
  end,
})

register({
  name = "grow",
  aliases = { "instantplant" },
  help = "brings the targeted crop to full growth",
  run = function(context)
    local x, y, z = targeted_block()
    local id = minecraft.world.get_block(x, y, z)
    assert(id == 59, "target a wheat crop")
    assert(minecraft.world.set_block(x, y, z, id, 7), "unable to grow crop")
    context.reply("Crop grown")
  end,
})

register({
  name = "search",
  params = "<block> [radius]",
  help = "finds the nearest matching block around you",
  run = function(context)
    local wanted = block_id(assert(context.args[1], "expected a block"))
    local radius = math.max(1, math.min(24, floor(context.args[2] or 16)))
    local px, py, pz = block_pos()
    local found, best, count = nil, math.huge, 0
    for dy = -radius, radius do
      for dz = -radius, radius do
        for dx = -radius, radius do
          if minecraft.world.get_block(px + dx, py + dy, pz + dz) == wanted then
            count = count + 1
            local distance = dx * dx + dy * dy + dz * dz
            if distance < best then
              best = distance
              found = { x = px + dx, y = py + dy, z = pz + dz }
            end
          end
        end
      end
    end
    assert(found ~= nil, "no block " .. wanted .. " within " .. radius .. " blocks")
    context.reply(string.format("%d found, nearest at %d %d %d (%.1f away)",
        count, found.x, found.y, found.z, math.sqrt(best)))
  end,
})

register({
  name = "clearwater",
  aliases = { "cw" },
  params = "[radius]",
  help = "removes water around you",
  run = function(context)
    local radius = math.max(1, math.min(32, floor(context.args[1] or 8)))
    local x, y, z = block_pos()
    local removed = 0
    for dy = -radius, radius do
      for dz = -radius, radius do
        for dx = -radius, radius do
          if WATER[minecraft.world.get_block(x + dx, y + dy, z + dz)] and
              minecraft.world.set_block(x + dx, y + dy, z + dz, AIR) then
            removed = removed + 1
          end
        end
      end
    end
    context.reply("Removed " .. plural(removed, "water block"))
  end,
})

register({
  name = "lava",
  params = "[radius]",
  help = "removes lava around you",
  run = function(context)
    local radius = math.max(1, math.min(32, floor(context.args[1] or 8)))
    local x, y, z = block_pos()
    local removed = 0
    for dy = -radius, radius do
      for dz = -radius, radius do
        for dx = -radius, radius do
          if LAVA[minecraft.world.get_block(x + dx, y + dy, z + dz)] and
              minecraft.world.set_block(x + dx, y + dy, z + dz, AIR) then
            removed = removed + 1
          end
        end
      end
    end
    context.reply("Removed " .. plural(removed, "lava block"))
  end,
})

register({
  name = "atlantis",
  params = "[radius]",
  help = "floods sea level around you",
  run = function(context)
    local radius = math.max(1, math.min(32, floor(context.args[1] or 16)))
    local x, _, z = block_pos()
    local changed = 0
    for dz = -radius, radius do
      for dx = -radius, radius do
        if dx * dx + dz * dz <= radius * radius and minecraft.world.get_block(x + dx, 63, z + dz) == AIR then
          if minecraft.world.set_block(x + dx, 63, z + dz, 8) then changed = changed + 1 end
        end
      end
    end
    context.reply("Flooded " .. plural(changed, "block"))
  end,
})

register({
  name = "superheat",
  params = "[radius]",
  help = "smelts nearby cobblestone, sand, ice, and snow",
  run = function(context)
    local radius = math.max(1, math.min(16, floor(context.args[1] or 4)))
    local x, y, z = block_pos()
    -- Only conversions that land on another block: ore smelts to an item, which has
    -- nowhere to go here.
    local cooked = { [4] = 1, [12] = 20, [79] = 9, [78] = AIR, [80] = AIR }
    local changed = 0
    for dy = -radius, radius do
      for dz = -radius, radius do
        for dx = -radius, radius do
          local replacement = cooked[minecraft.world.get_block(x + dx, y + dy, z + dz)]
          if replacement ~= nil and minecraft.world.set_block(x + dx, y + dy, z + dz, replacement) then
            changed = changed + 1
          end
        end
      end
    end
    context.reply("Superheated " .. plural(changed, "block"))
  end,
})

register({
  name = "spawnportal",
  help = "builds a nether portal in front of you",
  run = function(context)
    local x, y, z = block_pos()
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

--------------------------------------------------------------------------------
category = "SPC: entities"

register({
  name = "spawn",
  params = "<entity> [count]",
  help = "spawns entities where you stand",
  run = function(context)
    local kind = assert(context.args[1], "expected an entity")
    local count = math.max(1, math.min(64, floor(context.args[2] or 1)))
    local x, y, z = block_pos()
    local spawned = 0
    for _ = 1, count do
      if minecraft.world.spawn_entity(kind, x, y, z) then spawned = spawned + 1 end
    end
    assert(spawned > 0, "unknown entity: " .. kind)
    context.reply("Spawned " .. spawned .. " " .. kind)
  end,
})

register({
  name = "bring",
  params = "<entity>",
  help = "pulls matching entities to you",
  run = function(context)
    local wanted = tostring(assert(context.args[1], "expected an entity")):lower()
    local x, y, z = position()
    local moved = 0
    for _, value in ipairs(entities()) do
      if value.type:lower() == wanted and value:teleport(x, y, z) then moved = moved + 1 end
    end
    context.reply("Brought " .. moved .. " " .. wanted)
  end,
})

register({
  name = "count",
  params = "[entity]",
  help = "counts loaded entities by type",
  run = function(context)
    local wanted = context.args[1] and tostring(context.args[1]):lower()
    local tally, names = {}, {}
    for _, value in ipairs(entities()) do
      local kind = value.type
      if wanted == nil or kind:lower() == wanted then
        if tally[kind] == nil then names[#names + 1] = kind end
        tally[kind] = (tally[kind] or 0) + 1
      end
    end
    table.sort(names)
    if #names == 0 then
      context.reply("Nothing loaded")
      return
    end
    local parts = {}
    for _, name in ipairs(names) do parts[#parts + 1] = name .. " x" .. tally[name] end
    context.reply(table.concat(parts, ", "))
  end,
})

register({
  name = "killall",
  params = "[entity]",
  help = "removes every non-player entity, or one type",
  run = function(context)
    local wanted = context.args[1] and tostring(context.args[1]):lower()
    local removed = remove_entities(function(value) return wanted == nil or value.type:lower() == wanted end)
    context.reply("Removed " .. removed .. " entities")
  end,
})

register({
  name = "killnpc",
  aliases = { "exterminate" },
  params = "[entity]",
  help = "removes hostile mobs, or one named type",
  run = function(context)
    local wanted = context.args[1] and tostring(context.args[1]):lower()
    local removed = remove_entities(function(value)
      if wanted ~= nil then return value.type:lower() == wanted end
      return hostile[value.type] == true
    end)
    context.reply("Removed " .. removed .. " mobs")
  end,
})

register({
  name = "drops",
  aliases = { "removedrops" },
  help = "removes dropped items",
  run = function(context)
    local removed = remove_entities(function(value) return value.type == "Item" end)
    context.reply("Removed " .. plural(removed, "drop"))
  end,
})

register({
  name = "defuse",
  help = "removes all primed TNT",
  run = function(context)
    local removed = remove_entities(function(value) return value.type == "Tnt" end)
    context.reply("Defused " .. removed .. " TNT")
  end,
})

register({
  name = "freeze",
  params = "[on|off]",
  help = "holds every non-player entity still",
  run = function(context)
    state.freeze = toggle(context.args[1], state.freeze)
    context.reply("Entity freeze " .. (state.freeze and "enabled" or "disabled"))
  end,
})

register({
  name = "cannon",
  params = "[power]",
  help = "launches primed TNT the way you are facing",
  run = function(context)
    local power = math.max(0.1, math.min(10, tonumber(context.args[1]) or 2))
    local x, y, z = block_pos()
    local dx, dz = facing_vector()
    assert(minecraft.world.spawn_entity("Tnt", x + dx, y + 1, z + dz), "unable to spawn TNT")
    local latest = nil
    for _, value in ipairs(entities("Tnt")) do latest = value end
    if latest ~= nil then
      minecraft.entities.set_state(latest, { vx = dx * power, vy = power * 0.35, vz = dz * power })
    end
    context.reply("Cannon fired")
  end,
})

register({
  name = "explode",
  help = "drops primed TNT on the block you are looking at",
  run = function(context)
    local x, y, z = targeted_block()
    assert(minecraft.world.spawn_entity("Tnt", x + 0.5, y + 1, z + 0.5), "unable to spawn TNT")
    context.reply("TNT primed")
  end,
})

--------------------------------------------------------------------------------
category = "SPC: setup"

register({
  name = "alias",
  params = "<name> <command>",
  help = "makes one command stand for another",
  details = "/alias k /killall  then /k runs it",
  run = function(context)
    local name, target = context.rest:match("^(%S+)%s+(.+)$")
    assert(name ~= nil and target ~= nil, "expected an alias name and command")
    assert(target:sub(1, 1) == "/", "the command must start with /")
    aliases[command_name(name)] = target
    save()
    context.reply("Alias " .. name .. " set")
  end,
})

register({
  name = "ralias",
  params = "<name>",
  help = "removes an alias",
  run = function(context)
    local name = command_name(context.args[1])
    assert(aliases[name] ~= nil, "unknown alias")
    aliases[name] = nil
    save()
    context.reply("Alias " .. name .. " removed")
  end,
})

register({
  name = "listaliases",
  help = "lists your aliases",
  run = function(context)
    local names = {}
    for name in pairs(aliases) do names[#names + 1] = name end
    table.sort(names)
    if #names == 0 then
      context.reply("No aliases set")
      return
    end
    for _, name in ipairs(names) do
      context.reply(commands.GRAY .. "/" .. name .. " -> " .. aliases[name])
    end
  end,
})

register({
  name = "bind",
  params = "<key> <command>",
  help = "runs a command from a key",
  details = "rebindable afterwards in Options > Mod settings",
  run = function(context)
    local key, command = context.rest:match("^(%S+)%s+(.+)$")
    assert(key ~= nil and command ~= nil, "expected a key and command")
    assert(command:sub(1, 1) == "/", "the command must start with /")
    bind_key(key, command)
    save()
    context.reply("Bound " .. key:upper() .. " to " .. command)
  end,
})

register({
  name = "unbind",
  params = "<key>",
  help = "stops a key running its command",
  run = function(context)
    local id = "bind_" .. command_name(context.args[1])
    assert(bindings[id] ~= nil, "that key has no SPC command binding")
    bindings[id] = nil
    save()
    context.reply("Binding removed")
  end,
})

register({
  name = "listbinds",
  help = "lists your key bindings",
  run = function(context)
    local lines = {}
    for _, binding in pairs(bindings) do
      lines[#lines + 1] = commands.GRAY .. binding.key:upper() .. " -> " .. binding.command
    end
    table.sort(lines)
    if #lines == 0 then
      context.reply("No keys bound")
      return
    end
    for _, line in ipairs(lines) do context.reply(line) end
  end,
})

register({
  name = "config",
  help = "shows which SPC toggles are on",
  run = function(context)
    local names = {}
    for name in pairs(state) do names[#names + 1] = name end
    table.sort(names)
    local parts = {}
    for _, name in ipairs(names) do parts[#parts + 1] = name .. "=" .. tostring(state[name]) end
    context.reply(table.concat(parts, " "))
  end,
})

register({
  name = "phelp",
  params = "[page|command]",
  help = "shows the command help",
  run = function(context)
    commands.execute("/help " .. (context.args[1] or ""))
  end,
})

load()
minecraft.log("info", "SPC loaded")
