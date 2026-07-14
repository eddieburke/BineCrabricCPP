local settings_api = minecraft.require("scripts.settings")
local settings = settings_api.values
minecraft.require("scripts.shader_settings")

local shadow = {
  target = -1,
  depth = -1,
  size = 0,
  rendering = false,
  frame = nil,
  light = nil,
  latched = nil,
  stable_yaw = nil,
}

local post = {
  scene = -1,
  bloom_a = -1,
  bloom_b = -1,
  width = 0,
  height = 0,
  bloom_width = 0,
  bloom_height = 0,
  bloom_quality = -1,
  bound = false,
}

local function clamp(value, low, high)
  if value < low then return low end
  if value > high then return high end
  return value
end

local function normalize(x, y, z)
  local length = math.sqrt(x*x + y*y + z*z)
  if length < 0.00001 then return 0.0, 1.0, 0.0 end
  return x/length, y/length, z/length
end

local function dot(ax, ay, az, bx, by, bz)
  return ax*bx + ay*by + az*bz
end

local function shadow_size()
  return 512 * (2 ^ math.floor(clamp(settings.shadow_quality, 0, 3)))
end

local function shadow_radius()
  return math.max(16.0, math.min(settings.shadow_distance, minecraft.camera.far_plane() * 0.5))
end

-- Rebuild the exact orthographic camera basis used by the fixed-function
-- shadow pass. Keeping this in one place prevents the depth pass and terrain
-- shader from drifting apart when the orientation is stabilized.
local function light_basis(yaw_deg, pitch_deg)
  local yr = (yaw_deg + 180.0) * math.pi / 180.0
  local pr = pitch_deg * math.pi / 180.0
  local cy, syaw = math.cos(yr), math.sin(yr)
  local cp, sp = math.cos(pr), math.sin(pr)
  return
    cy, 0.0, syaw,
    sp * syaw, cp, -sp * cy,
    cp * syaw, -sp, -cp * cy
end

local function build_light(event)
  -- RealTime Sky publishes one authoritative world-space vector. Prefer it so
  -- the visible sun, shadows and PBR lighting cannot independently disagree.
  local sx = tonumber(event.sun_direction_x)
  local sy = tonumber(event.sun_direction_y)
  local sz = tonumber(event.sun_direction_z)
  local has_solar_vector = sx ~= nil and sy ~= nil and sz ~= nil

  if has_solar_vector then
    sx, sy, sz = normalize(sx, sy, sz)
  else
    -- In this runtime celestial_angle is already the renderer's physical
    -- angle in radians. Only the legacy celestial field is a normalized phase.
    local angle = tonumber(event.celestial_angle)
    if angle == nil then
      angle = (tonumber(event.celestial) or 0.0) * math.pi * 2.0
    end
    local sky_yaw_deg = tonumber(event.sky_yaw_deg) or 0.0
    local yaw = sky_yaw_deg * math.pi / 180.0
    sx = math.sin(angle) * math.sin(yaw)
    sy = math.cos(angle)
    sz = math.sin(angle) * math.cos(yaw)
    sx, sy, sz = normalize(sx, sy, sz)
  end

  local sun_y = sy
  local moon = sun_y < 0.0
  local casts_shadow = not moon and sun_y > 0.01
  if moon then sx, sy, sz = -sx, -sy, -sz end

  -- Aim the orthographic camera from the light toward the ground. Near the
  -- zenith yaw is mathematically undefined, so retain the last valid yaw rather
  -- than allowing tiny floating-point changes to spin the entire shadow map.
  local fx, fy, fz = -sx, -sy, -sz
  local pitch = -math.asin(clamp(fy, -1.0, 1.0)) * 180.0 / math.pi
  local horizontal = math.sqrt(fx * fx + fz * fz)
  local raw_yaw = math.atan(-fx, fz) * 180.0 / math.pi
  if horizontal > 0.0005 then shadow.stable_yaw = raw_yaw end
  local look_yaw = shadow.stable_yaw or raw_yaw
  local rx, ry, rz, ux, uy, uz, basis_fx, basis_fy, basis_fz = light_basis(look_yaw, pitch)

  local rain = clamp(tonumber(event.rain_strength) or 0.0, 0.0, 1.0)
  local horizon = clamp((sun_y - 0.01) / 0.2, 0.0, 1.0)
  horizon = horizon * horizon * (3.0 - 2.0 * horizon)
  local illumination = moon and 0.0 or clamp(0.25 + sun_y * 1.1, 0.0, 1.0) * horizon
  illumination = illumination * (1.0 - rain * 0.55)

  return {
    x = event.camera_x,
    y = event.camera_y,
    z = event.camera_z,
    fx = basis_fx, fy = basis_fy, fz = basis_fz,
    rx = rx, ry = ry, rz = rz,
    ux = ux, uy = uy, uz = uz,
    sx = sx, sy = sy, sz = sz,
    yaw = look_yaw,
    pitch = pitch,
    illumination = illumination,
    casts_shadow = casts_shadow,
    rain = rain,
    moon = moon,
  }
end

local function stabilize_light(light, size, radius)
  -- Quantize rotation by the angle subtended by one shadow texel at the edge
  -- of the map. Directional error stays below half a texel while the depth map
  -- no longer re-rasterizes at a different sub-texel orientation every frame.
  local angular_step = (2.0 / math.max(size, 1)) * 180.0 / math.pi
  local stable_yaw = math.floor(light.yaw / angular_step + 0.5) * angular_step
  local stable_pitch = math.floor(light.pitch / angular_step + 0.5) * angular_step
  local rx, ry, rz, ux, uy, uz, fx, fy, fz = light_basis(stable_yaw, stable_pitch)

  local texel = 2.0 * radius / size
  local right = dot(light.x, light.y, light.z, rx, ry, rz)
  local up = dot(light.x, light.y, light.z, ux, uy, uz)
  local delta_right = math.floor(right / texel + 0.5) * texel - right
  local delta_up = math.floor(up / texel + 0.5) * texel - up
  local result = {}
  for key, value in pairs(light) do result[key] = value end
  result.x = light.x + rx * delta_right + ux * delta_up
  result.y = light.y + ry * delta_right + uy * delta_up
  result.z = light.z + rz * delta_right + uz * delta_up
  result.rx, result.ry, result.rz = rx, ry, rz
  result.ux, result.uy, result.uz = ux, uy, uz
  result.fx, result.fy, result.fz = fx, fy, fz
  result.yaw = stable_yaw
  result.pitch = stable_pitch
  result.radius = radius
  return result
end

local fullscreen_vertex = [[
#version 120
void main() {
  gl_Position = gl_Vertex;
  gl_TexCoord[0] = gl_MultiTexCoord0;
}
]]

local shader_sources = {
  terrain = {
    [[
#version 120
uniform vec3 lightOffset;
uniform vec3 lightRight;
uniform vec3 lightUp;
uniform vec3 lightForward;
uniform float shadowRadius;
uniform float shadowDepth;
varying vec3 shadowCoord;
varying vec3 worldRelative;
void main() {
  vec4 eye = gl_ModelViewMatrix * gl_Vertex;
  vec3 cameraLocal = (gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
  worldRelative = gl_Vertex.xyz - cameraLocal;
  vec3 q = worldRelative - lightOffset;
  shadowCoord = vec3(dot(q, lightRight) / (2.0 * shadowRadius) + 0.5,
                     dot(q, lightUp) / (2.0 * shadowRadius) + 0.5,
                     dot(q, lightForward) / (2.0 * shadowDepth) + 0.5);
  gl_FogFragCoord = abs(eye.z);
  gl_Position = gl_ProjectionMatrix * eye;
  gl_FrontColor = gl_Color;
  gl_TexCoord[0] = gl_MultiTexCoord0;
}
]],
    [[
#version 120
uniform sampler2D terrain;
uniform sampler2D shadowMap;
uniform vec2 shadowTexel;
uniform vec3 sunDirection;
uniform float shadowStrength;
uniform float shadowBias;
uniform float shadowSoftness;
uniform float pbrStrength;
uniform float ambientStrength;
uniform float sunlightStrength;
uniform float roughness;
uniform float specularStrength;
uniform float wetness;
uniform int shadowEnabled;
uniform int shadowFilter;
uniform int pbrEnabled;
varying vec3 shadowCoord;
varying vec3 worldRelative;
float shadowTap(vec2 offset, float slopeBias) {
  float sampled = texture2D(shadowMap, shadowCoord.xy + offset * shadowTexel * shadowSoftness).r;
  return shadowCoord.z - slopeBias > sampled ? 1.0 : 0.0;
}
float filteredShadow(vec3 normal, vec3 lightDir) {
  if (shadowEnabled == 0) return 0.0;
  bool inside = shadowCoord.x > 0.0 && shadowCoord.x < 1.0 &&
                shadowCoord.y > 0.0 && shadowCoord.y < 1.0 &&
                shadowCoord.z > 0.0 && shadowCoord.z < 1.0;
  if (!inside) return 0.0;
  float facing = clamp(dot(normal, lightDir), 0.0, 1.0);
  if (facing <= 0.001) return 0.0;
  // Use the receiver's actual screen-space depth gradient to suppress the
  // intermittent whole-chunk self-shadowing caused by tiny camera/model-view
  // differences. Clamp it so the correction cannot detach shadows visibly.
  float baseBias = shadowBias * (0.70 + 1.30 * (1.0 - facing));
  float derivativeBias = max(abs(dFdx(shadowCoord.z)), abs(dFdy(shadowCoord.z))) * 1.25;
  float maximumBias = shadowBias * 4.0 + 0.00005;
  float slopeBias = min(max(baseBias, derivativeBias), maximumBias);
  float result = 0.0;
  float taps = 0.0;
  if (shadowFilter <= 0) {
    result = shadowTap(vec2(0.0), slopeBias);
    taps = 1.0;
  } else {
    for (int x = -2; x <= 2; ++x) {
      for (int y = -2; y <= 2; ++y) {
        if (shadowFilter > 1 || (x >= -1 && x <= 1 && y >= -1 && y <= 1)) {
          float jx = float(x);
          float jy = float(y);
          // Fixed shadow-space rotation avoids an obvious axis-aligned grid
          // without the temporal crawling caused by gl_FragCoord noise.
          vec2 rotated = vec2(jx * 0.9238795 - jy * 0.3826834,
                              jx * 0.3826834 + jy * 0.9238795);
          result += shadowTap(rotated, slopeBias);
          taps += 1.0;
        }
      }
    }
  }
  float edge = min(min(shadowCoord.x, 1.0 - shadowCoord.x),
                   min(shadowCoord.y, 1.0 - shadowCoord.y));
  return result / max(taps, 1.0) * smoothstep(0.0, 0.045, edge);
}
void main() {
  vec4 texel = texture2D(terrain, gl_TexCoord[0].xy);
  vec4 base = texel * gl_Color;
  vec3 dx = dFdx(worldRelative);
  vec3 dy = dFdy(worldRelative);
  vec3 normal = normalize(cross(dx, dy));
  vec3 viewDirection = normalize(-worldRelative);
  if (dot(normal, viewDirection) < 0.0) normal = -normal;
  vec3 lightDirection = normalize(sunDirection);
  float shadowAmount = filteredShadow(normal, lightDirection);
  float visibility = 1.0 - shadowAmount * shadowStrength;
  float ndotl = max(dot(normal, lightDirection), 0.0);
  float blend = pbrEnabled != 0 ? pbrStrength : 0.0;
  float diffuse = ambientStrength + sunlightStrength * (0.12 + ndotl * 0.88) * visibility;
  vec3 color = base.rgb * mix(visibility, diffuse, blend);
  float surfaceRoughness = mix(roughness, 0.12, wetness);
  vec3 halfDirection = normalize(lightDirection + viewDirection);
  float exponent = mix(128.0, 4.0, surfaceRoughness);
  float fresnel = 0.06 + 0.94 * pow(1.0 - max(dot(halfDirection, viewDirection), 0.0), 5.0);
  float specular = pow(max(dot(normal, halfDirection), 0.0), exponent) *
                   specularStrength * (1.0 - surfaceRoughness * 0.6) * ndotl * visibility;
  color *= mix(1.0, 0.88, wetness * blend);
  color += vec3(specular * fresnel * sunlightStrength * blend);
  float fog = clamp((gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale, 0.0, 1.0);
  gl_FragColor = vec4(mix(gl_Fog.color.rgb, color, fog), base.a);
}
]],
  },
  extract = {
    fullscreen_vertex,
    [[
#version 120
uniform sampler2D sceneTexture;
uniform float threshold;
void main() {
  vec3 color = texture2D(sceneTexture, gl_TexCoord[0].xy).rgb;
  float brightness = max(max(color.r, color.g), color.b);
  float knee = max(0.04, threshold * 0.35);
  float soft = clamp((brightness - threshold + knee) / (2.0 * knee), 0.0, 1.0);
  soft = soft * soft * (3.0 - 2.0 * soft);
  float contribution = max(brightness - threshold, 0.0) + soft * knee;
  gl_FragColor = vec4(color * contribution / max(brightness, 0.0001), 1.0);
}
]],
  },
  blur = {
    fullscreen_vertex,
    [[
#version 120
uniform sampler2D sourceTexture;
uniform vec2 direction;
void main() {
  vec2 uv = gl_TexCoord[0].xy;
  vec3 color = texture2D(sourceTexture, uv).rgb * 0.227027;
  color += texture2D(sourceTexture, uv + direction * 1.384615).rgb * 0.316216;
  color += texture2D(sourceTexture, uv - direction * 1.384615).rgb * 0.316216;
  color += texture2D(sourceTexture, uv + direction * 3.230769).rgb * 0.070270;
  color += texture2D(sourceTexture, uv - direction * 3.230769).rgb * 0.070270;
  gl_FragColor = vec4(color, 1.0);
}
]],
  },
  composite = {
    fullscreen_vertex,
    [[
#version 120
uniform sampler2D sceneTexture;
uniform sampler2D bloomTexture;
uniform float bloomIntensity;
uniform float exposure;
uniform float saturation;
uniform float contrast;
uniform float vibrance;
uniform float vignette;
vec3 aces(vec3 value) {
  return clamp((value * (2.51 * value + 0.03)) / (value * (2.43 * value + 0.59) + 0.14), 0.0, 1.0);
}
void main() {
  vec2 uv = gl_TexCoord[0].xy;
  vec3 scene = texture2D(sceneTexture, uv).rgb;
  vec3 bloom = texture2D(bloomTexture, uv).rgb;
  vec3 color = pow(max(scene, vec3(0.0)), vec3(2.2));
  color += pow(max(bloom, vec3(0.0)), vec3(2.2)) * bloomIntensity;
  color = aces(color * exposure);
  color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));
  float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
  color = mix(vec3(luminance), color, saturation);
  float maximum = max(max(color.r, color.g), color.b);
  float minimum = min(min(color.r, color.g), color.b);
  float colorfulness = maximum - minimum;
  color = mix(color, mix(vec3(luminance), color, 1.0 + vibrance), 1.0 - colorfulness);
  color = (color - 0.5) * contrast + 0.5;
  vec2 centered = uv * 2.0 - 1.0;
  float edge = smoothstep(0.35, 1.35, dot(centered, centered));
  color *= 1.0 - edge * vignette;
  gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
]],
  },
}

-- Lazily compiled shaders, keyed by shader_sources name. Failed compiles are
-- retried on the next request, matching the old per-shader ensure functions.
local shader_ids = {}
local function shader(name)
  local id = shader_ids[name]
  if id == nil or id == -1 then
    local sources = shader_sources[name]
    id = minecraft.shader.create(sources[1], sources[2])
    shader_ids[name] = id
  end
  return id
end

-- Sets integer uniforms from `ints` and float/vector uniforms from `floats`
-- (vector values are passed as arrays).
local function apply_uniforms(id, ints, floats)
  if ints then
    for name, value in pairs(ints) do minecraft.shader.uniform_int(id, name, value) end
  end
  if floats then
    for name, value in pairs(floats) do
      if type(value) == "table" then
        minecraft.shader.uniform_float(id, name, table.unpack(value))
      else
        minecraft.shader.uniform_float(id, name, value)
      end
    end
  end
end

-- Binds target (or the current framebuffer when nil), shader, and source
-- textures by unit, then draws a fullscreen quad.
local function fullscreen_pass(target, id, textures, ints, floats, width, height)
  if target then minecraft.fbo.bind(target) end
  minecraft.shader.bind(id)
  for unit, texture in ipairs(textures) do
    minecraft.texture.bind(texture, unit - 1)
  end
  apply_uniforms(id, ints, floats)
  minecraft.fbo.fullscreen(width, height)
end

local function destroy_shadow_target()
  if shadow.target ~= -1 then minecraft.camera.destroy(shadow.target) end
  shadow.target = -1
  shadow.depth = -1
  shadow.size = 0
  shadow.latched = nil
end

local function ensure_shadow_target()
  if not settings.shadows then return false end
  local size = shadow_size()
  if shadow.target ~= -1 and shadow.size ~= size then destroy_shadow_target() end
  if shadow.target == -1 then
    shadow.target = minecraft.camera.create(size, size, 1, true)
    shadow.size = size
    if shadow.target ~= -1 then shadow.depth = minecraft.camera.depth_texture(shadow.target) end
  end
  return shadow.target ~= -1 and shadow.depth ~= -1
end

local function destroy_post_targets()
  for _, key in ipairs({ "scene", "bloom_a", "bloom_b" }) do
    if post[key] ~= -1 then minecraft.fbo.destroy(post[key]) end
    post[key] = -1
  end
  post.width = 0
  post.height = 0
  post.bloom_width = 0
  post.bloom_height = 0
  post.bloom_quality = -1
end

local function ensure_post_targets(width, height)
  if not settings.post or width < 1 or height < 1 then return false end
  local quality = math.floor(clamp(settings.bloom_quality, 0, 2))
  local divisor = quality == 0 and 4 or (quality == 1 and 2 or 1)
  local bloom_width = math.max(1, math.floor(width / divisor))
  local bloom_height = math.max(1, math.floor(height / divisor))
  if post.scene ~= -1 and (post.width ~= width or post.height ~= height or post.bloom_quality ~= quality) then
    destroy_post_targets()
  end
  if post.scene == -1 then
    post.scene = minecraft.fbo.create(width, height, 1, false)
    post.bloom_a = minecraft.fbo.create(bloom_width, bloom_height, 1, false)
    post.bloom_b = minecraft.fbo.create(bloom_width, bloom_height, 1, false)
    post.width = width
    post.height = height
    post.bloom_width = bloom_width
    post.bloom_height = bloom_height
    post.bloom_quality = quality
  end
  return post.scene ~= -1 and post.bloom_a ~= -1 and post.bloom_b ~= -1
end

local function bind_terrain_shader(event)
  if shadow.rendering or minecraft.camera.rendering() ~= -1 then return end
  if not settings.shadows and not settings.pbr then return end
  local id = shader("terrain")
  if id == -1 then return end
  local light = shadow.latched or shadow.light
  if not light then return end
  local has_shadow = settings.shadows and shadow.latched ~= nil and
                     shadow.latched.casts_shadow and shadow.depth ~= -1
  if has_shadow then minecraft.texture.bind(shadow.depth, 1) end
  -- worldRelative is reconstructed from the active GL model-view matrix. This
  -- runtime exposes eye_* even though older docs omit it; camera_* can differ by
  -- view bobbing / first-person eye translation and intermittently shifts every
  -- receiver behind its own depth value.
  local eye_x = event.eye_x or event.camera_x
  local eye_y = event.eye_y or event.camera_y
  local eye_z = event.eye_z or event.camera_z
  local texel = 1.0 / math.max(shadow.size, 1)
  minecraft.shader.bind(id)
  apply_uniforms(id, {
    terrain = 0,
    shadowMap = 1,
    shadowEnabled = has_shadow and 1 or 0,
    shadowFilter = math.floor(settings.shadow_filter + 0.5),
    pbrEnabled = settings.pbr and 1 or 0,
  }, {
    shadowTexel = { texel, texel },
    shadowStrength = settings.shadow_strength * light.illumination,
    shadowRadius = light.radius or shadow_radius(),
    shadowDepth = settings.shadow_depth,
    shadowBias = settings.shadow_bias / (2.0 * settings.shadow_depth),
    shadowSoftness = settings.shadow_softness,
    lightOffset = { light.x - eye_x, light.y - eye_y, light.z - eye_z },
    lightRight = { light.rx, light.ry, light.rz },
    lightUp = { light.ux, light.uy, light.uz },
    lightForward = { light.fx, light.fy, light.fz },
    sunDirection = { light.sx, light.sy, light.sz },
    pbrStrength = settings.pbr_strength,
    ambientStrength = settings.ambient,
    sunlightStrength = settings.sunlight * (0.25 + light.illumination * 0.75),
    roughness = settings.roughness,
    specularStrength = settings.specular,
    wetness = math.max(settings.wetness, light.rain * 0.7),
  })
end

local function finish_post()
  local scene_texture = minecraft.fbo.texture(post.scene, 0)
  local bloom_texture = scene_texture
  local extract = shader("extract")
  local blur = shader("blur")
  local composite = shader("composite")
  local shaders_ready = extract ~= -1 and blur ~= -1 and composite ~= -1
  if settings.bloom and shaders_ready then
    fullscreen_pass(post.bloom_a, extract, { scene_texture },
      { sceneTexture = 0 },
      { threshold = settings.bloom_threshold },
      post.bloom_width, post.bloom_height)
    for _ = 1, 2 do
      fullscreen_pass(post.bloom_b, blur, { minecraft.fbo.texture(post.bloom_a, 0) },
        { sourceTexture = 0 },
        { direction = { settings.bloom_radius / post.bloom_width, 0.0 } },
        post.bloom_width, post.bloom_height)
      fullscreen_pass(post.bloom_a, blur, { minecraft.fbo.texture(post.bloom_b, 0) },
        nil,
        { direction = { 0.0, settings.bloom_radius / post.bloom_height } },
        post.bloom_width, post.bloom_height)
    end
    bloom_texture = minecraft.fbo.texture(post.bloom_a, 0)
  end
  minecraft.fbo.unbind()
  post.bound = false
  if shaders_ready then
    fullscreen_pass(nil, composite, { scene_texture, bloom_texture },
      { sceneTexture = 0, bloomTexture = 1 },
      {
        bloomIntensity = settings.bloom and settings.bloom_intensity or 0.0,
        exposure = settings.exposure,
        saturation = settings.saturation,
        contrast = settings.contrast,
        vibrance = settings.vibrance,
        vignette = settings.vignette,
      },
      post.width, post.height)
  else
    minecraft.shader.unbind()
    minecraft.texture.bind(scene_texture, 0)
    minecraft.fbo.fullscreen(post.width, post.height)
  end
  minecraft.shader.unbind()
end

minecraft.on(minecraft.events.world_render, {
  stage=minecraft.render.stages.sky,
  moment=minecraft.render.moments.before,
  priority=2000,
}, function(event)
  if not shadow.rendering and minecraft.camera.rendering() == -1 and event.has_world and event.is_overworld then
    shadow.frame = {
      camera_x=event.camera_x,
      camera_y=event.camera_y,
      camera_z=event.camera_z,
      celestial_angle=event.celestial_angle,
      celestial=event.celestial,
      sky_yaw_deg=event.sky_yaw_deg,
      sun_direction_x=event.sun_direction_x,
      sun_direction_y=event.sun_direction_y,
      sun_direction_z=event.sun_direction_z,
      sun_azimuth_deg=event.sun_azimuth_deg,
      sun_altitude_deg=event.sun_altitude_deg,
      solar_day_tick=event.solar_day_tick,
      solar_time_hours=event.solar_time_hours,
      rain_strength=event.rain_strength,
    }
    shadow.light = build_light(shadow.frame)
  end
end)

minecraft.on(minecraft.events.render_targets, { priority=-1000 }, function(event)
  -- render_targets exposes tick_delta only; display width/height are not part
  -- of its supported contract. The old post path bound several differently
  -- sized FBOs here without a guaranteed viewport restore, leaking an offscreen
  -- viewport into later sky/HUD stages. Keep post/bloom disabled until the host
  -- supplies a display-sized, state-restoring post hook.
  if post.bound then
    minecraft.fbo.unbind()
    post.bound = false
  end

  if shadow.frame and shadow.light and settings.shadows and
     shadow.light.casts_shadow and shadow.light.illumination > 0.005 and
     ensure_shadow_target() then
    local radius = shadow_radius()
    local pass_light = stabilize_light(shadow.light, shadow.size, radius)
    -- Never let a terrain/fullscreen shader remain active while recursively
    -- rendering the depth target.
    minecraft.shader.unbind()
    shadow.rendering = true
    local rendered = minecraft.camera.render_shadow_orthographic(
      shadow.target,
      pass_light.x,
      pass_light.y,
      pass_light.z,
      pass_light.yaw,
      pass_light.pitch,
      0.0,
      radius,
      radius,
      -settings.shadow_depth,
      settings.shadow_depth,
      settings.entity_shadows,
      event.tick_delta)
    shadow.rendering = false
    minecraft.shader.unbind()
    -- Explicitly restore the display framebuffer/viewport. The target renderer
    -- does not reliably restore it on every return path in this build.
    minecraft.camera.unbind()
    if rendered then
      shadow.latched = pass_light
    else
      shadow.latched = nil
    end
  else
    shadow.latched = nil
  end
end)

for _, stage in ipairs({ minecraft.render.stages.terrain_opaque, minecraft.render.stages.terrain_translucent }) do
  minecraft.on(minecraft.events.world_render, {
    stage=stage,
    moment=minecraft.render.moments.before,
    priority=1000,
  }, bind_terrain_shader)
  minecraft.on(minecraft.events.world_render, {
    stage=stage,
    moment=minecraft.render.moments.after,
    priority=-1000,
  }, function()
    if not shadow.rendering and minecraft.camera.rendering() == -1 then minecraft.shader.unbind() end
  end)
end
