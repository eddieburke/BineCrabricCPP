REALTIME SKY + SHADOW VIEWPORT STABILITY FIX V3

This package replaces both mod directories.

Changes from V2:
- Restores real-time world-clock synchronization. The nearest absolute Minecraft
  day is retained, while time-of-day is locked to the same apparent-solar tick
  that drives the visible sun and brightness.
- Removes the speculative shadow-pass pose recapture. The sampler now uses the
  exact stabilized camera pose supplied to render_shadow_orthographic.
- Uses the runtime eye_x/eye_y/eye_z fields when available. These match the GL
  model-view origin and prevent view-bobbing/camera-pitch changes from turning
  whole nearby chunks into self-shadow.
- Adds bounded derivative-aware receiver bias to stop intermittent full-scene
  shadow acne without producing the large detached-shadow offset.
- Unbinds shaders around the recursive depth render and explicitly restores the
  display framebuffer/viewport after it.
- Keeps celestial_angle as the runtime's physical radian angle and celestial as
  the normalized 0..1 phase.
- Unsafe custom post/bloom FBO capture remains disabled.

Keep existing large realtime_sky assets such as cities.json and
globe_coasts.txt when replacing the directories.
