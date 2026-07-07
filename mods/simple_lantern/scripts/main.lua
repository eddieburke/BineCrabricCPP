local function v(x, y, z)
  return { x, y, z }
end

local function quad(a, au, av, b, bu, bv, c, cu, cv, d, du, dv)
  minecraft.tessellator.quad({
    vertices = {
      { a[1], a[2], a[3], au, av },
      { b[1], b[2], b[3], bu, bv },
      { c[1], c[2], c[3], cu, cv },
      { d[1], d[2], d[3], du, dv },
    },
  })
end

local function draw_lantern()
  local min_core = 0.3125
  local max_core = 0.6875
  quad(v(min_core, 0.5, min_core), 5, 9, v(min_core, 0.5, max_core), 5, 15, v(max_core, 0.5, max_core), 11, 15, v(max_core, 0.5, min_core), 11, 9)
  quad(v(0, 1, min_core), 0, 0, v(1, 1, min_core), 16, 0, v(1, 0, min_core), 16, 16, v(0, 0, min_core), 0, 16)
  quad(v(1, 1, min_core), 16, 0, v(0, 1, min_core), 0, 0, v(0, 0, min_core), 0, 16, v(1, 0, min_core), 16, 16)
  quad(v(1, 1, max_core), 0, 0, v(0, 1, max_core), 16, 0, v(0, 0, max_core), 16, 16, v(1, 0, max_core), 0, 16)
  quad(v(0, 1, max_core), 16, 0, v(1, 1, max_core), 0, 0, v(1, 0, max_core), 0, 16, v(0, 0, max_core), 16, 16)
  quad(v(min_core, 1, 1), 0, 0, v(min_core, 1, 0), 16, 0, v(min_core, 0, 0), 16, 16, v(min_core, 0, 1), 0, 16)
  quad(v(min_core, 1, 0), 16, 0, v(min_core, 1, 1), 0, 0, v(min_core, 0, 1), 0, 16, v(min_core, 0, 0), 16, 16)
  quad(v(max_core, 1, 0), 0, 0, v(max_core, 1, 1), 16, 0, v(max_core, 0, 1), 16, 16, v(max_core, 0, 0), 0, 16)
  quad(v(max_core, 1, 1), 16, 0, v(max_core, 1, 0), 0, 0, v(max_core, 0, 0), 0, 16, v(max_core, 0, 1), 16, 16)
end

minecraft.register_block({
  id = 151,
  texture = "mods/simple_lantern/lantern.png",
  hardness = 0.5,
  resistance = 1.0,
  luminance = 0.9375,
  translation_key = "lantern",
  material = "metal",
  model = {
    type = "manual",
    draw = draw_lantern,
    inventory = draw_lantern,
    opaque = false,
    full_cube = false,
  },
})
