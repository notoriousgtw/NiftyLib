#version 450

layout(push_constant) uniform PickingPushConstants {
    uint object_id;
} push_constants;

layout(location = 0) out uvec4 out_object_id;

void main() {
    // Output the object ID as a color
    // Use R channel for object ID, G and B channels can be used for additional data
    out_object_id = uvec4(push_constants.object_id, 0, 0, 255);
}