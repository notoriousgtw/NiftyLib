#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec4 frag_color;
layout (location = 1) in vec2 frag_texture_coord;
layout (location = 2) in vec3 frag_world_pos;
layout (location = 3) in vec3 frag_normal;

layout (location = 0) out vec4 out_color;

// Set 1: Texture array for all textures
layout (set = 1, binding = 0) uniform sampler2D textures[32]; // Array of textures

// Push constants for per-object material properties
layout (push_constant) uniform MaterialPushConstants {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float specular_highlights;
    uint diffuse_texture_index;     // Index into texture array
    uint ambient_texture_index;     // Index into texture array  
    uint specular_texture_index;    // Index into texture array
    uint padding;
} material;

void main() {

    
    vec3 diffuse_color = material.diffuse;
    if (material.diffuse_texture_index < 32) {
        diffuse_color = texture(textures[material.diffuse_texture_index], frag_texture_coord).rgb;
    }
    
    vec3 ambient_color = material.ambient;
    if (material.ambient_texture_index < 32) {
        ambient_color = texture(textures[material.ambient_texture_index], frag_texture_coord).rgb;
    }
    
    vec3 specular_color = material.specular;
    if (material.specular_texture_index < 32) {
        specular_color = texture(textures[material.specular_texture_index], frag_texture_coord).rgb;
    }
    
    // Simple lighting calculation
    vec3 light_pos = vec3(2.0, 4.0, 2.0);
    vec3 light_color = vec3(1.0, 1.0, 1.0);
    
    vec3 normal = normalize(frag_normal);
    vec3 light_dir = normalize(light_pos - frag_world_pos);
    
    // Ambient component
    vec3 ambient = ambient_color;
    
    // Diffuse component
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = diffuse_color * diff * light_color;
    
    // Simple specular (you can enhance this with proper view direction)
    vec3 specular = specular_color * material.specular_highlights * 0.1;
    
    // Combine all components
    vec3 final_color = ambient + diffuse + specular;
    
    out_color = vec4(final_color, 1);
}