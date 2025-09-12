#pragma once

#include "graphics/mesh.h"

#include <memory>

namespace nft::graphics
{

struct RenderComponent
{
	std::shared_ptr<TriMesh> mesh;
	bool					 visible = true;
	
	// Constructors
	RenderComponent() = default;
	RenderComponent(std::shared_ptr<TriMesh> mesh, bool visible = true) : mesh(mesh), visible(visible) {}
};

struct TransformComponent
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);	 // Euler angles in degrees
	glm::vec3 scale	   = glm::vec3(1.0f);
	
	// Constructors
	TransformComponent() = default;
	TransformComponent(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f))
		: position(position), rotation(rotation), scale(scale) {}
	
	glm::mat4 GetModelMatrix() const
	{
		glm::mat4 model = glm::mat4(1.0f);
		model			= glm::translate(model, position);
		model			= glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model			= glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model			= glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		model			= glm::scale(model, scale);
		
		// Debug output for transform matrix (only print once to reduce spam)
		// Note: Using proper logging would require including header for VulkanHandler, which could create circular dependency
		// This debug info is only needed for initial development, so it's commented out for production
		static bool printed = false;
		if (!printed) {
			// Convert to proper logging when needed:
			// auto* app = vulkan::VulkanHandler::GetApp();
			// if (app && app->GetLogger()) {
			//     app->GetLogger()->Debug(std::format("Transform - pos: ({:.6f}, {:.6f}, {:.6f}), scale: ({:.6f}, {:.6f}, {:.6f}), rot: ({:.6f}, {:.6f}, {:.6f})",
			//         position.x, position.y, position.z, scale.x, scale.y, scale.z, rotation.x, rotation.y, rotation.z), "Transform");
			// }
			printed = true;
		}
		
		return model;
	}
	glm::mat4 GetModelMatrixRowMajor() const
	{
		return glm::transpose(GetModelMatrix());	// Convert to row-major order before returning
	}
};

}	 // namespace nft::graphics
