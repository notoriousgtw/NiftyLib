#include "graphics/scene.h"
#include "core/error.h"
#include "vk/handler.h"

namespace nft::graphics
{

void Scene::RegisterEntity(ecs::EntityId entity)
{
	auto* render_comp = world->GetComponent<RenderComponent>(entity);
	auto* transform_comp = world->GetComponent<TransformComponent>(entity);
	
	if (render_comp && transform_comp)
	{
		RenderableEntity renderable;
		renderable.entity = entity;
		renderable.render_comp = *render_comp;
		renderable.transform_comp = *transform_comp;
		
		renderable_entities.push_back(renderable);
	}
}

void Scene::UnRegisterEntity(ecs::EntityId entity)
{
	auto it = std::remove_if(renderable_entities.begin(), renderable_entities.end(),
		[entity](const RenderableEntity& re) { return re.entity == entity; });
	renderable_entities.erase(it, renderable_entities.end());
}

void Scene::UpdateRenderableEntity()
{
	for (auto& renderable : renderable_entities)
	{
		auto* render_comp = world->GetComponent<RenderComponent>(renderable.entity);
		auto* transform_comp = world->GetComponent<TransformComponent>(renderable.entity);
		
		if (render_comp)
			renderable.render_comp = *render_comp;
		if (transform_comp)
			renderable.transform_comp = *transform_comp;
	}
}

uint32_t Scene::AddMaterial(std::shared_ptr<Material> material)
{
	materials.push_back(material);
	return static_cast<uint32_t>(materials.size() - 1);
}

std::shared_ptr<Material> Scene::GetMaterial(uint32_t index) const
{
	if (index < materials.size())
		return materials[index];
	return nullptr;
}

ecs::EntityId Scene::LoadObjScene(const std::string& file_dir, const std::string& file_name)
{
	// TODO: Implement OBJ loading functionality
	// This is a placeholder implementation
	return ecs::INVALID_ENTITY;
}

ecs::EntityId Scene::CreateTriangleEntity(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& rotation)
{
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (logger) {
		logger->Debug(std::format("Creating triangle entity at position ({:.6f}, {:.6f}, {:.6f})", position.x, position.y, position.z), "Scene");
	}
	
	// Create entity
	ecs::EntityId entity = world->CreateEntity();
	
	// Add render component with triangle mesh
	auto triangle_mesh = CreateTriangleMesh();
	world->AddComponent<RenderComponent>(entity, triangle_mesh, true);
	
	// Add transform component
	world->AddComponent<TransformComponent>(entity, position, rotation, scale);
	
	// Register entity for rendering
	RegisterEntity(entity);
	
	if (logger) {
		logger->Debug(std::format("Created triangle entity with ID {}, registered {} total entities", entity, renderable_entities.size()), "Scene");
	}
	
	return entity;
}

ecs::EntityId Scene::CreateQuadEntity(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& rotation)
{
	// Create entity
	ecs::EntityId entity = world->CreateEntity();
	
	// Add render component with quad mesh
	auto quad_mesh = CreateQuadMesh();
	world->AddComponent<RenderComponent>(entity, quad_mesh, true);
	
	// Add transform component
	world->AddComponent<TransformComponent>(entity, position, rotation, scale);
	
	// Register entity for rendering
	RegisterEntity(entity);
	
	return entity;
}

std::shared_ptr<TriMesh> Scene::CreateTriangleMesh()
{
	auto mesh = std::make_shared<TriMesh>();
	
	// Create triangle vertices - make them much larger and more visible
	Vertex v0 = {{-0.8f, -0.6f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}};  // Bottom left - red
	Vertex v1 = {{ 0.8f, -0.6f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}};  // Bottom right - green
	Vertex v2 = {{ 0.0f,  0.6f, 0.0f, 1.0f}, {0.5f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}};  // Top center - blue
	
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (logger) {
		logger->Debug(std::format("Triangle mesh vertices: v0=({:.6f},{:.6f},{:.6f}), v1=({:.6f},{:.6f},{:.6f}), v2=({:.6f},{:.6f},{:.6f})",
			v0.position.x, v0.position.y, v0.position.z,
			v1.position.x, v1.position.y, v1.position.z,
			v2.position.x, v2.position.y, v2.position.z), "Scene");
	}
	
	// Add vertices to mesh
	mesh->AddVertex(v0);
	mesh->AddVertex(v1);
	mesh->AddVertex(v2);
	
	// Add triangle face (indices 0, 1, 2)
	std::array<uint32_t, 3> triangle_indices = {0, 1, 2};
	mesh->AddFace(triangle_indices, 0); // material index 0
	
	mesh->SetName("Triangle");
	return mesh;
}

std::shared_ptr<TriMesh> Scene::CreateQuadMesh()
{
	auto mesh = std::make_shared<TriMesh>();
	
	// Create quad vertices
	Vertex v0 = {{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}};  // Bottom left - white
	Vertex v1 = {{ 0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}};  // Bottom right - white
	Vertex v2 = {{ 0.5f,  0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}};  // Top right - white
	Vertex v3 = {{-0.5f,  0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}};  // Top left - white
	
	// Add vertices to mesh
	mesh->AddVertex(v0);
	mesh->AddVertex(v1);
	mesh->AddVertex(v2);
	mesh->AddVertex(v3);
	
	// Add two triangle faces to form a quad
	std::array<uint32_t, 3> triangle1_indices = {0, 1, 2}; // Bottom-left, bottom-right, top-right
	std::array<uint32_t, 3> triangle2_indices = {0, 2, 3}; // Bottom-left, top-right, top-left
	mesh->AddFace(triangle1_indices, 0); // material index 0
	mesh->AddFace(triangle2_indices, 0); // material index 0
	
	mesh->SetName("Quad");
	return mesh;
}

} // namespace nft::graphics
