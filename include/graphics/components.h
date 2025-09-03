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
		static bool printed = false;
		if (!printed) {
			printf("DEBUG: Transform - pos: (%f, %f, %f), scale: (%f, %f, %f), rot: (%f, %f, %f)\n",
				position.x, position.y, position.z, scale.x, scale.y, scale.z, rotation.x, rotation.y, rotation.z);
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
