minecraft.register_block({
  id = 151,
  texture = "mods/simple_lantern/lantern.png",
  hardness = 0.5,
  resistance = 1.0,
  luminance = 0.9375,
  translation_key = "lantern",
  material = "metal",
  model = {
    type = "box_list",
    boxes = {
      {
        min = { 0.4375, 0.0, 0.4375 },
        max = { 0.5625, 0.625, 0.5625 },
      },
      {
        min = { 0.375, 0.625, 0.375 },
        max = { 0.625, 0.75, 0.625 },
      },
    },
    opaque = false,
    full_cube = false,
  },
})
