#version 430 core
// Iris final writes the Minecraft backbuffer (not a colortex).
in vec2 texcoord;
uniform sampler2D colortex0;
out vec4 fragColor;
void main() {
 fragColor = texture(colortex0, texcoord);
}
