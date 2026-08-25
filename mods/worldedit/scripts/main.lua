local commands = require("lib.commands")

local WAND_ID = 30000
local MAX_BLOCKS = 65536
local MAX_HISTORY = 20
local AIR = 0
local WATER = { 8, 9 }
local LAVA = { 10, 11 }

local position_one = nil
local position_two = nil
local clipboard = nil
local undo_history = {}
local redo_history = {}

minecraft.register_item({
  id = WAND_ID,
  texture = "worldedit_wand.png",
  max_count = 1,
  translation_key = "worldeditWand",
  name = "WorldEdit Wand",
})

local function floor(value)
  return math.floor(tonumber(value) or 0)
end

local function player_position()
  local player = assert(minecraft.world.player(), "no player in this world")
  return floor(player.x), floor(player.y), floor(player.z), player
end

local function set_position(which, x, y, z, notify)
  local position = { x = floor(x), y = floor(y), z = floor(z) }
  if which == 1 then
    position_one = position
  else
    position_two = position
  end
  if notify then
    commands.reply("Position " .. which .. " set to (" .. position.x .. ", " .. position.y .. ", " .. position.z .. ")")
  end
end

local function block_spec(value)
  assert(value ~= nil, "expected a block")
  local raw = tostring(value):lower()
  local base, meta = raw:match("^(.-):(-?%d+)$")
  if base == nil then
    base = raw
  end
  local id = tonumber(base)
  if id == nil then
    id = minecraft.world.block_id(base)
    assert(id ~= 0 or base == "air", "unknown block: " .. raw)
  end
  assert(id ~= nil and id >= 0, "unknown block: " .. raw)
  return math.floor(id), meta ~= nil and math.max(0, math.min(15, tonumber(meta))) or 0
end

local function contains(values, value)
  for _, candidate in ipairs(values) do
    if candidate == value then
      return true
    end
  end
  return false
end

local function selection()
  assert(position_one ~= nil and position_two ~= nil, "make a selection with the WorldEdit Wand first")
  local min_x = math.min(position_one.x, position_two.x)
  local min_y = math.max(0, math.min(position_one.y, position_two.y))
  local min_z = math.min(position_one.z, position_two.z)
  local max_x = math.max(position_one.x, position_two.x)
  local max_y = math.min(127, math.max(position_one.y, position_two.y))
  local max_z = math.max(position_one.z, position_two.z)
  local volume = (max_x - min_x + 1) * (max_y - min_y + 1) * (max_z - min_z + 1)
  assert(volume <= MAX_BLOCKS, "selection is too large (max " .. MAX_BLOCKS .. " blocks)")
  return min_x, min_y, min_z, max_x, max_y, max_z, volume
end

local function each_selection(callback)
  local min_x, min_y, min_z, max_x, max_y, max_z, volume = selection()
  for y = min_y, max_y do
    for z = min_z, max_z do
      for x = min_x, max_x do
        callback(x, y, z)
      end
    end
  end
  return min_x, min_y, min_z, max_x, max_y, max_z, volume
end

local function begin_edit(label)
  return { label = label, changes = {}, keys = {} }
end

local function change_key(x, y, z)
  return x .. ":" .. y .. ":" .. z
end

local function write(edit, x, y, z, id, meta)
  if y < 0 or y > 127 then
    return false
  end
  local key = change_key(x, y, z)
  local existing = edit.keys[key]
  local old_id = minecraft.world.get_block(x, y, z)
  local old_meta = minecraft.world.get_block_meta(x, y, z)
  if old_id == id and old_meta == meta then
    return false
  end
  if existing == nil then
    existing = { x = x, y = y, z = z, before_id = old_id, before_meta = old_meta, after_id = id, after_meta = meta }
    edit.keys[key] = existing
    edit.changes[#edit.changes + 1] = existing
  else
    existing.after_id = id
    existing.after_meta = meta
  end
  minecraft.world.set_block(x, y, z, id, meta)
  return true
end

local function finish_edit(edit)
  if #edit.changes == 0 then
    commands.reply(edit.label .. ": no blocks changed")
    return 0
  end
  undo_history[#undo_history + 1] = edit
  if #undo_history > MAX_HISTORY then
    table.remove(undo_history, 1)
  end
  redo_history = {}
  commands.reply(edit.label .. ": changed " .. #edit.changes .. " blocks")
  return #edit.changes
end

local function apply_changes(changes, before, reverse)
  if reverse then
    for index = #changes, 1, -1 do
      local change = changes[index]
      minecraft.world.set_block(change.x, change.y, change.z, before and change.before_id or change.after_id,
        before and change.before_meta or change.after_meta)
    end
  else
    for _, change in ipairs(changes) do
      minecraft.world.set_block(change.x, change.y, change.z, before and change.before_id or change.after_id,
        before and change.before_meta or change.after_meta)
    end
  end
end

local function direction(value)
  if value ~= nil then
    local name = tostring(value):lower()
    local directions = {
      north = { 0, 0, -1 }, n = { 0, 0, -1 }, south = { 0, 0, 1 }, s = { 0, 0, 1 },
      east = { 1, 0, 0 }, e = { 1, 0, 0 }, west = { -1, 0, 0 }, w = { -1, 0, 0 },
      up = { 0, 1, 0 }, u = { 0, 1, 0 }, down = { 0, -1, 0 }, d = { 0, -1, 0 },
    }
    assert(directions[name] ~= nil, "unknown direction: " .. name)
    return directions[name][1], directions[name][2], directions[name][3]
  end
  local _, _, _, player = player_position()
  local facing = math.floor((player.yaw % 360 + 45) / 90) % 4
  local directions = { { 0, 0, 1 }, { -1, 0, 0 }, { 0, 0, -1 }, { 1, 0, 0 } }
  return directions[facing + 1][1], directions[facing + 1][2], directions[facing + 1][3]
end

local function look_target()
  local hit = minecraft.raycast()
  assert(hit ~= nil and hit.type == "block", "look at a block first")
  return hit.block_x, hit.block_y, hit.block_z
end

local function selection_blocks()
  local min_x, min_y, min_z, max_x, max_y, max_z = selection()
  local blocks = {}
  for y = min_y, max_y do
    for z = min_z, max_z do
      for x = min_x, max_x do
        blocks[#blocks + 1] = {
          x = x, y = y, z = z, id = minecraft.world.get_block(x, y, z), meta = minecraft.world.get_block_meta(x, y, z),
        }
      end
    end
  end
  return blocks, min_x, min_y, min_z, max_x, max_y, max_z
end

local function copy_selection()
  local blocks, min_x, min_y, min_z, max_x, max_y, max_z = selection_blocks()
  local px, py, pz = player_position()
  clipboard = { blocks = {}, anchor = { x = px, y = py, z = pz }, width = max_x - min_x + 1, height = max_y - min_y + 1, depth = max_z - min_z + 1 }
  for _, block in ipairs(blocks) do
    clipboard.blocks[#clipboard.blocks + 1] = {
      x = block.x - px, y = block.y - py, z = block.z - pz, id = block.id, meta = block.meta,
    }
  end
  commands.reply("Copied " .. #clipboard.blocks .. " blocks")
end

local function edit_selection(label, callback)
  local edit = begin_edit(label)
  each_selection(function(x, y, z)
    callback(edit, x, y, z, minecraft.world.get_block(x, y, z), minecraft.world.get_block_meta(x, y, z))
  end)
  return finish_edit(edit)
end

commands.register({
  name = "/wand",
  aliases = { "/selwand" },
  help = "gives the dedicated WorldEdit selection wand",
  run = function()
    minecraft.inventory.give({ id = WAND_ID, count = 1 })
    commands.reply("WorldEdit Wand added to inventory")
  end,
})

commands.register({
  name = "/pos1",
  help = "sets the first selection position at your feet",
  run = function()
    local x, y, z = player_position()
    set_position(1, x, y - 1, z, true)
  end,
})

commands.register({
  name = "/pos2",
  help = "sets the second selection position at your feet",
  run = function()
    local x, y, z = player_position()
    set_position(2, x, y - 1, z, true)
  end,
})

commands.register({
  name = "/hpos1",
  help = "sets the first selection position at the targeted block",
  run = function()
    local x, y, z = look_target()
    set_position(1, x, y, z, true)
  end,
})

commands.register({
  name = "/hpos2",
  help = "sets the second selection position at the targeted block",
  run = function()
    local x, y, z = look_target()
    set_position(2, x, y, z, true)
  end,
})

commands.register({
  name = "/sel",
  params = "cuboid",
  help = "uses cuboid selections",
  run = function(context)
    local mode = (context.args[1] or "cuboid"):lower()
    assert(mode == "cuboid", "only cuboid selections are supported")
    commands.reply("Cuboid selection enabled")
  end,
})

commands.register({
  name = "/size",
  help = "shows the current selection dimensions",
  run = function()
    local min_x, min_y, min_z, max_x, max_y, max_z, volume = selection()
    commands.reply((max_x - min_x + 1) .. " x " .. (max_y - min_y + 1) .. " x " .. (max_z - min_z + 1) .. " (" .. volume .. " blocks)")
  end,
})

commands.register({
  name = "/distr",
  help = "lists block counts in the current selection",
  run = function()
    local counts = {}
    each_selection(function(x, y, z)
      local id = minecraft.world.get_block(x, y, z)
      counts[id] = (counts[id] or 0) + 1
    end)
    local entries = {}
    for id, count in pairs(counts) do
      entries[#entries + 1] = { id = id, count = count }
    end
    table.sort(entries, function(a, b) return a.count > b.count end)
    for _, entry in ipairs(entries) do
      commands.reply(entry.id .. ": " .. entry.count)
    end
  end,
})

commands.register({
  name = "/count",
  params = "<block>",
  help = "counts a block in the current selection",
  run = function(context)
    local wanted, wanted_meta = block_spec(context.args[1])
    local count = 0
    each_selection(function(x, y, z)
      if minecraft.world.get_block(x, y, z) == wanted and minecraft.world.get_block_meta(x, y, z) == wanted_meta then
        count = count + 1
      end
    end)
    commands.reply("Counted " .. count .. " blocks")
  end,
})

commands.register({
  name = "/set",
  params = "<block>",
  help = "sets every block in the selection",
  run = function(context)
    local id, meta = block_spec(context.args[1])
    edit_selection("Set", function(edit, x, y, z)
      write(edit, x, y, z, id, meta)
    end)
  end,
})

commands.register({
  name = "/replace",
  params = "[from] <to>",
  help = "replaces matching blocks in the selection",
  run = function(context)
    local from_id, from_meta, to_id, to_meta
    if context.args[2] == nil then
      to_id, to_meta = block_spec(context.args[1])
    else
      from_id, from_meta = block_spec(context.args[1])
      to_id, to_meta = block_spec(context.args[2])
    end
    edit_selection("Replace", function(edit, x, y, z, old_id, old_meta)
      if (from_id == nil and old_id ~= AIR) or (old_id == from_id and old_meta == from_meta) then
        write(edit, x, y, z, to_id, to_meta)
      end
    end)
  end,
})

commands.register({
  name = "/walls",
  params = "<block>",
  help = "builds selection walls",
  run = function(context)
    local id, meta = block_spec(context.args[1])
    local min_x, min_y, min_z, max_x, max_y, max_z = selection()
    edit_selection("Walls", function(edit, x, y, z)
      if x == min_x or x == max_x or z == min_z or z == max_z then
        write(edit, x, y, z, id, meta)
      end
    end)
  end,
})

commands.register({
  name = "/outline",
  params = "<block>",
  help = "builds walls, floor, and ceiling",
  run = function(context)
    local id, meta = block_spec(context.args[1])
    local min_x, min_y, min_z, max_x, max_y, max_z = selection()
    edit_selection("Outline", function(edit, x, y, z)
      if x == min_x or x == max_x or y == min_y or y == max_y or z == min_z or z == max_z then
        write(edit, x, y, z, id, meta)
      end
    end)
  end,
})

commands.register({
  name = "/overlay",
  params = "<block>",
  help = "places a block on top of every selected column",
  run = function(context)
    local id, meta = block_spec(context.args[1])
    local min_x, min_y, min_z, max_x, max_y, max_z = selection()
    local edit = begin_edit("Overlay")
    for z = min_z, max_z do
      for x = min_x, max_x do
        for y = max_y, min_y, -1 do
          if minecraft.world.get_block(x, y, z) ~= AIR then
            write(edit, x, y + 1, z, id, meta)
            break
          end
        end
      end
    end
    finish_edit(edit)
  end,
})

commands.register({
  name = "/copy",
  help = "copies the selection to the clipboard",
  run = copy_selection,
})

commands.register({
  name = "/cut",
  help = "copies the selection and clears it",
  run = function()
    copy_selection()
    edit_selection("Cut", function(edit, x, y, z)
      write(edit, x, y, z, AIR, 0)
    end)
  end,
})

commands.register({
  name = "/paste",
  params = "[-a]",
  help = "pastes the clipboard at your position",
  run = function(context)
    assert(clipboard ~= nil, "copy a selection first")
    local include_air = context.args[1] == "-a"
    local x, y, z = player_position()
    local edit = begin_edit("Paste")
    for _, block in ipairs(clipboard.blocks) do
      if include_air or block.id ~= AIR then
        write(edit, x + block.x, y + block.y, z + block.z, block.id, block.meta)
      end
    end
    finish_edit(edit)
  end,
})

commands.register({
  name = "/rotate",
  params = "<90|180|270>",
  help = "rotates the clipboard around the vertical axis",
  run = function(context)
    assert(clipboard ~= nil, "copy a selection first")
    local angle = tonumber(context.args[1])
    assert(angle ~= nil and angle % 90 == 0, "rotation must be a multiple of 90")
    local turns = ((angle / 90) % 4 + 4) % 4
    for _, block in ipairs(clipboard.blocks) do
      for _ = 1, turns do
        block.x, block.z = -block.z, block.x
      end
    end
    commands.reply("Clipboard rotated " .. angle .. " degrees")
  end,
})

commands.register({
  name = "/flip",
  params = "[direction]",
  help = "flips the clipboard",
  run = function(context)
    assert(clipboard ~= nil, "copy a selection first")
    local dx, dy, dz = direction(context.args[1])
    for _, block in ipairs(clipboard.blocks) do
      if dx ~= 0 then block.x = -block.x end
      if dy ~= 0 then block.y = -block.y end
      if dz ~= 0 then block.z = -block.z end
    end
    commands.reply("Clipboard flipped")
  end,
})

commands.register({
  name = "/stack",
  params = "<count> [direction]",
  help = "stacks the selection",
  run = function(context)
    local count = math.max(1, math.floor(tonumber(context.args[1]) or 0))
    local dx, dy, dz = direction(context.args[2])
    local blocks, min_x, min_y, min_z, max_x, max_y, max_z = selection_blocks()
    local width, height, depth = max_x - min_x + 1, max_y - min_y + 1, max_z - min_z + 1
    assert(#blocks * count <= MAX_BLOCKS, "stack is too large")
    local edit = begin_edit("Stack")
    for copy = 1, count do
      for _, block in ipairs(blocks) do
        write(edit, block.x + dx * width * copy, block.y + dy * height * copy, block.z + dz * depth * copy, block.id, block.meta)
      end
    end
    finish_edit(edit)
  end,
})

commands.register({
  name = "/move",
  params = "<count> [direction] [leave]",
  help = "moves the selection contents",
  run = function(context)
    local count = math.max(1, math.floor(tonumber(context.args[1]) or 0))
    local dx, dy, dz = direction(context.args[2])
    local leave_id, leave_meta = block_spec(context.args[3] or "0")
    local blocks = selection_blocks()
    local edit = begin_edit("Move")
    for _, block in ipairs(blocks) do
      write(edit, block.x, block.y, block.z, leave_id, leave_meta)
    end
    for _, block in ipairs(blocks) do
      write(edit, block.x + dx * count, block.y + dy * count, block.z + dz * count, block.id, block.meta)
    end
    finish_edit(edit)
  end,
})

commands.register({
  name = "/undo",
  params = "[count]",
  help = "undoes WorldEdit operations",
  run = function(context)
    local count = math.max(1, math.floor(tonumber(context.args[1]) or 1))
    local undone = 0
    while undone < count and #undo_history > 0 do
      local edit = table.remove(undo_history)
      apply_changes(edit.changes, true, true)
      redo_history[#redo_history + 1] = edit
      undone = undone + 1
    end
    commands.reply("Undid " .. undone .. " operation" .. (undone == 1 and "" or "s"))
  end,
})

commands.register({
  name = "/redo",
  params = "[count]",
  help = "redoes WorldEdit operations",
  run = function(context)
    local count = math.max(1, math.floor(tonumber(context.args[1]) or 1))
    local redone = 0
    while redone < count and #redo_history > 0 do
      local edit = table.remove(redo_history)
      apply_changes(edit.changes, false, false)
      undo_history[#undo_history + 1] = edit
      redone = redone + 1
    end
    commands.reply("Redid " .. redone .. " operation" .. (redone == 1 and "" or "s"))
  end,
})

commands.register({
  name = "/clearhistory",
  help = "clears WorldEdit undo history",
  run = function()
    undo_history = {}
    redo_history = {}
    commands.reply("WorldEdit history cleared")
  end,
})

local function shape_edit(label, callback)
  local edit = begin_edit(label)
  callback(function(x, y, z, id, meta)
    write(edit, x, y, z, id, meta)
  end)
  finish_edit(edit)
end

commands.register({
  name = "/cyl",
  params = "<block> <radius> [height]",
  help = "creates a solid vertical cylinder",
  run = function(context)
    local id, meta = block_spec(context.args[1])
    local radius = math.max(0, math.floor(tonumber(context.args[2]) or -1))
    local height = math.max(1, math.floor(tonumber(context.args[3]) or 1))
    assert(radius <= 64 and math.pi * radius * radius * height <= MAX_BLOCKS, "cylinder is too large")
    local cx, cy, cz = player_position()
    shape_edit("Cylinder", function(place)
      for y = cy, cy + height - 1 do
        for z = cz - radius, cz + radius do
          for x = cx - radius, cx + radius do
            if (x - cx) * (x - cx) + (z - cz) * (z - cz) <= radius * radius then
              place(x, y, z, id, meta)
            end
          end
        end
      end
    end)
  end,
})

commands.register({
  name = "/hcyl",
  params = "<block> <radius> [height]",
  help = "creates a hollow vertical cylinder",
  run = function(context)
    local id, meta = block_spec(context.args[1])
    local radius = math.max(1, math.floor(tonumber(context.args[2]) or -1))
    local height = math.max(1, math.floor(tonumber(context.args[3]) or 1))
    assert(radius <= 64, "cylinder is too large")
    local cx, cy, cz = player_position()
    shape_edit("Hollow cylinder", function(place)
      for y = cy, cy + height - 1 do
        for z = cz - radius, cz + radius do
          for x = cx - radius, cx + radius do
            local distance = (x - cx) * (x - cx) + (z - cz) * (z - cz)
            if distance <= radius * radius and distance >= (radius - 1) * (radius - 1) then
              place(x, y, z, id, meta)
            end
          end
        end
      end
    end)
  end,
})

local function sphere_command(name, hollow)
  commands.register({
    name = name,
    params = "<block> <radius> [raised]",
    help = hollow and "creates a hollow sphere" or "creates a solid sphere",
    run = function(context)
      local id, meta = block_spec(context.args[1])
      local radius = math.max(0, math.floor(tonumber(context.args[2]) or -1))
      local raised = tostring(context.args[3] or ""):lower() == "raised"
      assert(radius <= 32 and 4 * math.pi * radius * radius * radius / 3 <= MAX_BLOCKS, "sphere is too large")
      local cx, cy, cz = player_position()
      if not raised then cy = cy + radius end
      shape_edit(hollow and "Hollow sphere" or "Sphere", function(place)
        for y = cy - radius, cy + radius do
          for z = cz - radius, cz + radius do
            for x = cx - radius, cx + radius do
              local distance = (x - cx) * (x - cx) + (y - cy) * (y - cy) + (z - cz) * (z - cz)
              if distance <= radius * radius and (not hollow or distance >= math.max(0, radius - 1) * math.max(0, radius - 1)) then
                place(x, y, z, id, meta)
              end
            end
          end
        end
      end)
    end,
  })
end

sphere_command("/sphere", false)
sphere_command("/hsphere", true)

local function pyramid_command(name, hollow)
  commands.register({
    name = name,
    params = "<block> <size>",
    help = hollow and "creates a hollow pyramid" or "creates a solid pyramid",
    run = function(context)
      local id, meta = block_spec(context.args[1])
      local size = math.max(1, math.floor(tonumber(context.args[2]) or 0))
      assert(size <= 64, "pyramid is too large")
      local cx, cy, cz = player_position()
      shape_edit(hollow and "Hollow pyramid" or "Pyramid", function(place)
        for level = 0, size - 1 do
          local radius = size - level - 1
          for z = cz - radius, cz + radius do
            for x = cx - radius, cx + radius do
              if not hollow or radius == 0 or x == cx - radius or x == cx + radius or z == cz - radius or z == cz + radius then
                place(x, cy + level, z, id, meta)
              end
            end
          end
        end
      end)
    end,
  })
end

pyramid_command("/pyramid", false)
pyramid_command("/hpyramid", true)

commands.register({
  name = "/drain",
  params = "<radius>",
  help = "removes water and lava in a radius",
  run = function(context)
    local radius = math.max(1, math.floor(tonumber(context.args[1]) or 0))
    assert(radius <= 32, "radius is too large")
    local cx, cy, cz = player_position()
    shape_edit("Drain", function(place)
      for y = cy - radius, cy + radius do
        for z = cz - radius, cz + radius do
          for x = cx - radius, cx + radius do
            local dx, dy, dz = x - cx, y - cy, z - cz
            if dx * dx + dy * dy + dz * dz <= radius * radius and contains(WATER, minecraft.world.get_block(x, y, z)) or
                dx * dx + dy * dy + dz * dz <= radius * radius and contains(LAVA, minecraft.world.get_block(x, y, z)) then
              place(x, y, z, AIR, 0)
            end
          end
        end
      end
    end)
  end,
})

local function fix_liquid_command(name, liquids)
  commands.register({
    name = name,
    help = "normalizes liquid blocks in the selection",
    run = function()
      edit_selection("Fix liquid", function(edit, x, y, z, id)
        if contains(liquids, id) then
          write(edit, x, y, z, id, 0)
        end
      end)
    end,
  })
end

fix_liquid_command("/fixwater", WATER)
fix_liquid_command("/fixlava", LAVA)

local function vertical_remove_command(name, above)
  commands.register({
    name = name,
    help = above and "removes blocks above the selection" or "removes blocks below the selection",
    run = function()
      local min_x, min_y, min_z, max_x, max_y, max_z = selection()
      local edit = begin_edit(above and "Remove above" or "Remove below")
      for z = min_z, max_z do
        for x = min_x, max_x do
          local from_y, to_y = above and max_y + 1 or 0, above and 127 or min_y - 1
          local step = above and 1 or -1
          for y = from_y, to_y, step do
            write(edit, x, y, z, AIR, 0)
          end
        end
      end
      finish_edit(edit)
    end,
  })
end

vertical_remove_command("/removeabove", true)
vertical_remove_command("/removebelow", false)

commands.register({
  name = "/removenear",
  params = "<block> <radius>",
  help = "removes a block type near you",
  run = function(context)
    local wanted, wanted_meta = block_spec(context.args[1])
    local radius = math.max(1, math.floor(tonumber(context.args[2]) or 0))
    assert(radius <= 32, "radius is too large")
    local cx, cy, cz = player_position()
    shape_edit("Remove near", function(place)
      for y = cy - radius, cy + radius do
        for z = cz - radius, cz + radius do
          for x = cx - radius, cx + radius do
            local dx, dy, dz = x - cx, y - cy, z - cz
            if dx * dx + dy * dy + dz * dz <= radius * radius and minecraft.world.get_block(x, y, z) == wanted and
                minecraft.world.get_block_meta(x, y, z) == wanted_meta then
              place(x, y, z, AIR, 0)
            end
          end
        end
      end
    end)
  end,
})

commands.register({
  name = "/expand",
  params = "<amount> [direction]",
  help = "expands the selection",
  run = function(context)
    local amount = math.max(0, math.floor(tonumber(context.args[1]) or 0))
    local dx, dy, dz = direction(context.args[2])
    assert(position_one ~= nil and position_two ~= nil, "make a selection first")
    if dx > 0 then position_two.x = position_two.x + amount elseif dx < 0 then position_one.x = position_one.x - amount end
    if dy > 0 then position_two.y = position_two.y + amount elseif dy < 0 then position_one.y = position_one.y - amount end
    if dz > 0 then position_two.z = position_two.z + amount elseif dz < 0 then position_one.z = position_one.z - amount end
    commands.reply("Selection expanded")
  end,
})

commands.register({
  name = "/contract",
  params = "<amount> [direction]",
  help = "contracts the selection",
  run = function(context)
    local amount = math.max(0, math.floor(tonumber(context.args[1]) or 0))
    local dx, dy, dz = direction(context.args[2])
    assert(position_one ~= nil and position_two ~= nil, "make a selection first")
    if dx > 0 then position_two.x = position_two.x - amount elseif dx < 0 then position_one.x = position_one.x + amount end
    if dy > 0 then position_two.y = position_two.y - amount elseif dy < 0 then position_one.y = position_one.y + amount end
    if dz > 0 then position_two.z = position_two.z - amount elseif dz < 0 then position_one.z = position_one.z - amount end
    selection()
    commands.reply("Selection contracted")
  end,
})

commands.register({
  name = "/shift",
  params = "<amount> [direction]",
  help = "shifts the selection",
  run = function(context)
    local amount = math.floor(tonumber(context.args[1]) or 0)
    local dx, dy, dz = direction(context.args[2])
    assert(position_one ~= nil and position_two ~= nil, "make a selection first")
    position_one.x, position_one.y, position_one.z = position_one.x + dx * amount, position_one.y + dy * amount, position_one.z + dz * amount
    position_two.x, position_two.y, position_two.z = position_two.x + dx * amount, position_two.y + dy * amount, position_two.z + dz * amount
    commands.reply("Selection shifted")
  end,
})

local function inset_command(name, sign)
  commands.register({
    name = name,
    params = "<amount>",
    help = sign > 0 and "expands the selection in every direction" or "contracts the selection in every direction",
    run = function(context)
      local amount = math.max(0, math.floor(tonumber(context.args[1]) or 0)) * sign
      assert(position_one ~= nil and position_two ~= nil, "make a selection first")
      position_one.x, position_one.y, position_one.z = position_one.x - amount, position_one.y - amount, position_one.z - amount
      position_two.x, position_two.y, position_two.z = position_two.x + amount, position_two.y + amount, position_two.z + amount
      selection()
      commands.reply("Selection updated")
    end,
  })
end

inset_command("/outset", 1)
inset_command("/inset", -1)

local function teleport(x, y, z)
  local player = assert(minecraft.world.player(), "no player in this world")
  player:teleport(x, y, z)
end

commands.register({
  name = "/up",
  params = "<distance>",
  help = "moves you upward",
  run = function(context)
    local x, y, z = player_position()
    teleport(x, y + math.max(1, floor(context.args[1])), z)
  end,
})

commands.register({
  name = "/jumpto",
  aliases = { "/thru" },
  help = "teleports to the targeted block",
  run = function()
    local x, y, z = look_target()
    teleport(x + 0.5, y + 1, z + 0.5)
  end,
})

commands.register({
  name = "/unstuck",
  help = "moves you to the first air space above",
  run = function()
    local x, y, z = player_position()
    for target_y = y, 126 do
      if minecraft.world.get_block(x, target_y, z) == AIR and minecraft.world.get_block(x, target_y + 1, z) == AIR then
        teleport(x + 0.5, target_y, z + 0.5)
        return
      end
    end
    error("no open space found")
  end,
})

minecraft.on("mouse_button", { button = 0, pressed = true, priority = 100 }, function(event)
  local player = minecraft.world.player()
  if player == nil or player.held_item_id ~= WAND_ID then
    return
  end
  local hit = minecraft.raycast()
  if hit ~= nil and hit.type == "block" then
    set_position(1, hit.block_x, hit.block_y, hit.block_z, true)
    event.handled = true
  end
  return event
end)

minecraft.on("block_interact", { item_id = WAND_ID, right_click = true, remote = false, priority = 100 }, function(event)
  set_position(2, event.x, event.y, event.z, true)
  event.canceled = true
  event.handled = true
  return event
end)
