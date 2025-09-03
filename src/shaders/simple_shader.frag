#version 450
#extension GL_EXT_nonuniform_qualifier : enable

// Input from vertex shader
layout (location = 0) in vec4 frag_color;

// Output color
layout (location = 0) out vec4 out_color;

void main() {
    // Just output the vertex color - solid color rendering
    out_color = frag_color;
}