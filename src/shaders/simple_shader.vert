#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (set = 0, binding = 0) uniform UniformBaseObject {
	mat4 view;
	mat4 proj;
	vec3 pos;
} CameraData;

layout (std140, set = 0, binding = 1) readonly buffer StorageBuffer {
	mat4 transforms[];
} ObjectData;

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec4 vertex_color;
layout (location = 2) in vec2 vertex_texture_coord;
layout (location = 3) in vec3 vertex_normal;

layout (location = 0) out vec4 frag_color;
layout (location = 1) out vec2 frag_texture_coord;
layout (location = 2) out vec3 frag_world_pos;
layout (location = 3) out vec3 frag_normal;

void main() {
	vec3 debug_colors[4] = vec3[4](
		vec3(1.0, 0.0, 0.0),  // Red for instance 0
		vec3(0.0, 1.0, 0.0),  // Green for instance 1  
		vec3(0.0, 0.0, 1.0),  // Blue for instance 2
		vec3(1.0, 1.0, 0.0)   // Yellow for instance 3
	);

	mat4 model_matrix = ObjectData.transforms[gl_InstanceIndex];
	vec4 world_pos = model_matrix * vec4(vertex_position, 1.0);
	gl_Position = CameraData.proj * CameraData.view * world_pos;

	frag_color = vertex_color;
	frag_texture_coord = vertex_texture_coord;
	frag_world_pos = world_pos.xyz;
	
	// For now, assume normal is just up vector (you can enhance this later)
	frag_normal = vertex_normal;
}