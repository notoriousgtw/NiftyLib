#pragma once

#include "core/event.h"

#include "vk/common.h"
#include "vk/geometry.h"
#include "vk/image.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace nft::vulkan
{
struct ObjectData
{
	IMesh*	  mesh;
	glm::mat4 transform;
	uint32_t  material_index = 0;	 // Index of the material in the materials vector
};

class Camera
{
	public:
	Camera() = default;
	~Camera() = default;
	glm::mat4 GetViewMatrix() const { return glm::inverse(transform); }
	glm::mat4 GetProjectionMatrix(float aspect_ratio) const
	{
		return glm::perspective(glm::radians(fov), aspect_ratio, near_plane, far_plane);
	}
	void SetPosition(const glm::vec3& position) { transform[3] = glm::vec4(position, 1.0f); }
	glm::vec3 GetPosition() const { return glm::vec3(transform[3]); }
	void SetRotation(const glm::vec3& rotation)
	{
		transform = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0));
		transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0, 1, 0));
		transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0, 0, 1));
		transform[3] = glm::vec4(GetPosition(), 1.0f);	 // Preserve position
	}
	glm::vec3 GetRotation() const
	{
		// Extract Euler angles from the rotation matrix
		glm::vec3 scale;
		glm::quat  orientation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transform, scale, orientation, translation, skew, perspective);
		return glm::degrees(glm::eulerAngles(orientation));
	}
	void SetFOV(float fov_degrees) { fov = fov_degrees; }
	float GetFOV() const { return fov; }
	void SetNearPlane(float near_plane_distance) { near_plane = near_plane_distance; }
	float GetNearPlane() const { return near_plane; }
	void SetFarPlane(float far_plane_distance) { far_plane = far_plane_distance; }
	float GetFarPlane() const { return far_plane; }
  private:
	glm::mat4 transform   = glm::mat4(1.0f);	   // Camera transformation matrix
	float	  fov		 = 45.0f;				   // Field of view in degrees
	float	  near_plane   = 0.1f;				   // Near clipping plane
	float	  far_plane	  = 100.0f;					// Far clipping plane
};

class Scene: Observer
{
  public:
	// Constructor
	Scene(Surface* surface, vk::CommandBuffer main_command_buffer);
	// Destructor
	~Scene() = default;

	void Update(IEvent* source);

	// Add an object to the scene
	void AddObject(const ObjectData& object) { objects.push_back(object); }

	// Get all objects in the scene
	const std::vector<ObjectData>& GetObjects() const { return objects; }

	// Add a material to the scene
	void AddMaterial(const Material& material) { materials.push_back(material); }
	// Get all materials in the scene
	const std::vector<Material>& GetMaterials() const { return materials; }

	// Get the geometry batcher
	const GeometryBatcher* GetGeometryBatcher() { return geometry_batcher.get(); }

  private:
	Surface* surface;
	Device*	 device;	// Vulkan device

	glm::vec2 last_mouse_pos;
	uint32_t selected_obj = UINT32_MAX;
	bool	  is_rotating = false;
	bool	  is_panning = false;

	glm::mat4 camera_transforms = glm::mat4(1.0f);	  // Camera transformation matrix

	// Orbital camera variables
	glm::vec3 orbit_target			 = glm::vec3(0.0f, 1.0f, -3.0f);	   // Point to orbit around
	float	  orbit_distance		 = 5.0f;						   // Distance from target
	float	  orbit_angle_horizontal = 0.0f;						   // Horizontal rotation angle
	float	  orbit_angle_vertical	 = 0.0f;						   // Vertical rotation angle
	float	  orbit_speed			 = 2.0f;						   // Rotation speed multiplier

	// Helper function to update camera transform from orbital parameters
	void UpdateCameraFromOrbit();

	std::vector<ObjectData>			 objects;			  // List of objects in the scene
	std::unique_ptr<GeometryBatcher> geometry_batcher;	  // Geometry batcher for efficient rendering
	std::vector<IMesh*>				 meshes;
	std::vector<Texture>			 textures;
	std::vector<Material>			 materials;	   // List of materials in the scene

	friend class Surface;
};
vk::VertexInputBindingDescription				 GetVertexInputBindingDescription();
std::vector<vk::VertexInputAttributeDescription> GetVertexInputAttributeDescriptions();

}	 // namespace nft::vulkan
