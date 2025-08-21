#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec3 in_normal;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 pos;
} ubo;

layout(set = 0, binding = 1) readonly buffer ObjectTransforms {
    mat4 transforms[];
} object_transforms;

layout(push_constant) uniform PickingPushConstants {
    uint object_id;
} push_constants;

void main() {
    mat4 model = object_transforms.transforms[gl_InstanceIndex];
    gl_Position = ubo.proj * ubo.view * model * vec4(in_position, 1.0);
}