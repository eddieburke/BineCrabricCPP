local CRYSTAL_BLOCK = 163
local BLOCK_TEXTURE = "mods/item_audio_demo/textures/item_audio_demo.png"
local CLICK_SOUND = "item_audio_demo:click"
local LOOP_SOUND = "item_audio_demo:loop"
local loop_handle = nil

-- A placeable crystal block. Boxes are centered in x/z (0.28..0.72) so the
-- inventory model and the placed block both sit centered rather than offset.
assert(minecraft.register_block({
  id = CRYSTAL_BLOCK,
  name = "Audio Demo Crystal",
  translation_key = "item_audio_demo.crystal",
  texture = BLOCK_TEXTURE,
  hardness = 0.6,
  resistance = 1.0,
  luminance = 0.6,
  material = "glass",
  model = {
    type = "box_list",
    opaque = false,
    full_cube = false,
    boxes = {
      { min = { 0.28, 0.00, 0.28 }, max = { 0.72, 0.52, 0.72 } },
      { min = { 0.22, 0.52, 0.22 }, max = { 0.78, 0.74, 0.78 } },
      { min = { 0.38, 0.74, 0.38 }, max = { 0.62, 0.92, 0.62 } },
    },
  },
  on_use = function(event)
    if event.right_click then
      minecraft.sound.play(CLICK_SOUND, 0.85, 1.0)
    end
  end,
}))

assert(minecraft.sound.register(CLICK_SOUND, "mods/item_audio_demo/sounds/item_audio_demo.wav", "effect"))
assert(minecraft.sound.register(LOOP_SOUND, "mods/item_audio_demo/sounds/item_audio_demo.wav", "streaming"))

minecraft.on(minecraft.events.client_tick, {
  has_player = true,
  once = true,
  priority = 0,
}, function()
  minecraft.inventory.give({ id = CRYSTAL_BLOCK, count = 1 })
  minecraft.sound.play(CLICK_SOUND, 0.85, 1.0)
end)

minecraft.on(minecraft.events.key_press, {
  key = minecraft.key_code("g"),
  pressed = true,
  priority = 100,
}, function(event)
  local player = minecraft.world.player()
  if player == nil then
    return
  end
  minecraft.sound.play_at(CLICK_SOUND, player.x, player.y, player.z, 1.0, 1.0)
  event.handled = true
end)

minecraft.on(minecraft.events.key_press, {
  key = minecraft.key_code("h"),
  pressed = true,
  priority = 100,
}, function(event)
  local player = minecraft.world.player()
  if player == nil then
    return
  end
  if loop_handle ~= nil then
    minecraft.sound.stop(loop_handle)
    loop_handle = nil
  else
    loop_handle = minecraft.sound.play_loop_at(LOOP_SOUND, player.x, player.y, player.z, 0.8, 1.0)
  end
  event.handled = true
end)
