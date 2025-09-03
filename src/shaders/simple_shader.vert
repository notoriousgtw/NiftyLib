#version 450
#extension GL_EXT_nonuniform_qualifier : enable

// Traditional vertex input attributes
layout (location = 0) in vec4 in_position;     // position from vertex buffer
layout (location = 1) in vec2 in_tex_coord;    // UV coordinates from vertex buffer  
layout (location = 2) in vec3 in_normal;       // normal from vertex buffer
layout (location = 3) in vec4 in_color;        // color from vertex buffer

// Output to fragment shader
layout (location = 0) out vec4 frag_color;

// Simple push constants
layout (push_constant) uniform PushConstants {
    uint frame_index;
    uint material_index;
    uint transform_index;
    uint vertex_offset;
} pc;

// Camera data from bindless system
layout (std140, set = 0, binding = 0) readonly buffer CameraBuffer {
    mat4 view;
    mat4 proj;
    vec3 pos;
} camera_data[];

// Transform data from bindless system
layout (std140, set = 0, binding = 1) readonly buffer TransformBuffer {
    mat4 transforms[];
} transform_data;

void main() {
    // Use the input vertex data directly
    vec4 world_pos = in_position;
    
    // Apply transform if available
    if (pc.transform_index < 1000) {
        world_pos = transform_data.transforms[pc.transform_index] * in_position;
    }
    
    // Apply camera transformation
    if (pc.frame_index < 3) {
        gl_Position = camera_data[pc.frame_index].proj * camera_data[pc.frame_index].view * world_pos;
    } else {
        gl_Position = world_pos;
    }
    
    // Pass vertex color to fragment shader
    frag_color = in_color;
}