local model = assert(minecraft.model.load("models/lantern.json"))

minecraft.register_block({
  id = 151,
  texture = "mods/simple_lantern/lantern.png",
  hardness = 0.5,
  resistance = 1.0,
  luminance = 0.9375,
  translation_key = "lantern",
  material = "metal",
  opaque = false,
  full_cube = false,
  model = model,
})
