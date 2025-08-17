#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec4 frag_color;
layout (location = 1) in vec2 frag_texture_coord;

layout (location = 0) out vec4 out_color;

layout (set = 1, binding = 0) uniform sampler2D material;

void main() {
	out_color = texture(material, frag_texture_coord);
}