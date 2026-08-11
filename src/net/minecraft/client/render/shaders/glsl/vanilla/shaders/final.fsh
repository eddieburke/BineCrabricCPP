#version 430 core
// Iris final writes the Minecraft backbuffer (not a colortex).
in vec2 texcoord;
uniform sampler2D colortex0;
uniform float screenBrightness;
out vec4 fragColor;
void main() {
 vec4 scene = texture(colortex0, texcoord);
 vec3 lifted = 1.0 - pow(1.0 - clamp(scene.rgb, 0.0, 1.0), vec3(4.0));
 fragColor = vec4(mix(scene.rgb, lifted, clamp(screenBrightness, 0.0, 1.0)), scene.a);
}
