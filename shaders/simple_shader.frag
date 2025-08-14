#version 450

layout (location = 0) in vec3 frag_color;
layout (location = 1) in vec2 frag_texture_coord;

layout (location = 0) out vec4 out_color;

layout (set = 1

void main() {
	outColor = vec4(fragColor, 1.0);
}