#pragma once

#include "core/parse_obj.h"
#include "ecs/world.h"
#include "graphics/components.h"
#include "graphics/material.h"

#include <memory>
#include <vector>

namespace nft::vulkan
{
// Forward declarations to avoid circular dependencies
class Texture;
}

namespace nft::graphics
{
// Forward declaration to avoid circular dependency
//class Renderer;

class Scene
{
  public:
	struct RenderableEntity
	{
		ecs::EntityId	   entity;
		RenderComponent	   render_comp;
		TransformComponent transform_comp;
	};

	Scene(ecs::World* world): world(world) {};

	void RegisterEntity(ecs::EntityId entity);
	void UnRegisterEntity(ecs::EntityId entity);
	void UpdateRenderableEntity();

	// Material management
	uint32_t									  AddMaterial(std::shared_ptr<Material> material);
	std::shared_ptr<Material>					  GetMaterial(uint32_t index) const;
	const std::vector<std::shared_ptr<Material>>& GetMaterials() const { return materials; }

	// Scene loading
	ecs::EntityId LoadObjScene(const std::string& file_dir, const std::string& file_name);

	// Access to renderable entities for renderer
	const std::vector<RenderableEntity>& GetRenderableEntities() const { return renderable_entities; }
	
	// Helper methods to create entities with meshes
	ecs::EntityId CreateTriangleEntity(const glm::vec3& position = glm::vec3(0.0f), 
									   const glm::vec3& scale = glm::vec3(1.0f),
									   const glm::vec3& rotation = glm::vec3(0.0f));
	ecs::EntityId CreateQuadEntity(const glm::vec3& position = glm::vec3(0.0f), 
								   const glm::vec3& scale = glm::vec3(1.0f),
								   const glm::vec3& rotation = glm::vec3(0.0f));

  private:
	ecs::World*									  world;
	//std::unique_ptr<Renderer>					  renderer;
	std::vector<RenderableEntity>				  renderable_entities;
	std::vector<std::shared_ptr<Material>>		  materials;
	std::vector<std::shared_ptr<vulkan::Texture>> textures;	   // Loaded textures
	
	// Helper methods to create basic meshes
	std::shared_ptr<TriMesh> CreateTriangleMesh();
	std::shared_ptr<TriMesh> CreateQuadMesh();
};
}	 // namespace nft::graphics