#include "graphics/renderer.h"
#include "vk/geometry.h"	// For GeometryBatcher
#include "vk/surface.h"
#include "vk/handler.h"
#include "graphics/mesh.h"  // For graphics::Vertex
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <thread>

namespace nft::graphics
{

void Renderer::DrawFrame()
{
	if (!surface || !scene)
		return;

	// Upload current scene data to bindless buffers before rendering
	UploadSceneDataToBindlessBuffers();

	// Begin frame rendering
	vk::CommandBuffer command_buffer = surface->BeginFrame();
	if (!command_buffer) {
		// Frame acquisition failed (likely due to swapchain recreation)
		return;
	}

	// CRITICAL FIX: Get the correct framebuffer for the acquired swapchain image
	uint32_t current_image_index = surface->GetCurrentImageIndex();
	vk::Framebuffer current_framebuffer = surface->GetFramebuffer(current_image_index);
	
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (logger) {
		logger->Debug(std::format("Using framebuffer for swapchain image {} in render pass", current_image_index), "VKRender");
	}

	// Begin render pass with the correct framebuffer
	vk::RenderPassBeginInfo render_pass_info = vk::RenderPassBeginInfo()
		.setRenderPass(surface->GetSwapChainRenderPass().vk_render_pass)
		.setFramebuffer(current_framebuffer)  // Use the correct framebuffer for the acquired image
		.setRenderArea(vk::Rect2D().setOffset({0, 0}).setExtent(surface->GetExtent()));

	// Clear values for color and depth
	std::vector<vk::ClearValue> clear_values = {
		vk::ClearValue().setColor(vk::ClearColorValue(std::array<float, 4>{0.1f, 0.1f, 0.3f, 1.0f})),  // Dark blue background
		vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue(1.0f, 0))
	};
	render_pass_info.setClearValueCount(clear_values.size()).setPClearValues(clear_values.data());

	command_buffer.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
	if (logger) {
		logger->Debug(std::format("Started render pass for image {}", current_image_index), "VKRender");
	}

	// Record direct draw commands
	for (auto& pipeline : managed_pipelines) {
		pipeline->RecordDrawCommandsWithEntities(command_buffer, surface->GetCurrentFrameIndex(), entity_handles);
	}

	// End render pass
	command_buffer.endRenderPass();
	if (logger) {
		logger->Debug(std::format("Ended render pass for image {}", current_image_index), "VKRender");
	}

	// End frame rendering
	surface->EndFrame();
}

void Renderer::UploadSceneDataToBindlessBuffers()
{
	auto* global_manager = vulkan::VulkanHandler::GetGlobalResourceManager();
	if (!global_manager || !scene)
		return;

	const auto& renderable_entities = scene->GetRenderableEntities();
	
	// Clear previous entity data
	entity_handles.clear();
	
	// If no entities to render, skip drawing entirely (no fallback triangle)
	if (renderable_entities.empty())
	{
		// Use app's logger through VulkanHandler
		auto* app = vulkan::VulkanHandler::GetApp();
		if (app && app->GetLogger()) {
			app->GetLogger()->Debug("No entities found - skipping rendering", "VKRender");
		}
		return;
	}
	
	// Only allocate buffers once, then reuse them
	static bool buffers_allocated = false;
	if (!buffers_allocated && !renderable_entities.empty())
	{
		auto* app = vulkan::VulkanHandler::GetApp();
		auto* logger = app ? app->GetLogger() : nullptr;
		
		if (logger) {
			logger->Debug(std::format("First-time allocation of buffers for {} entities", renderable_entities.size()), "VKRender");
		}
		
		// Collect all vertex data, index data, transforms, and materials from entities
		std::vector<graphics::Vertex> all_vertices;
		std::vector<uint32_t> all_indices;
		std::vector<glm::mat4> all_transforms;
		std::vector<vulkan::GPUMaterial> all_materials;
		
		for (const auto& entity : renderable_entities)
		{
			if (!entity.render_comp.visible || !entity.render_comp.mesh)
				continue;
			
			const auto& mesh = entity.render_comp.mesh;
			const auto& vertices = mesh->GetVertices();
			const auto& faces = mesh->GetFaces();
			
			if (vertices.empty() || faces.empty())
				continue;
			
			vulkan::EntityRenderData render_data;
			render_data.vertex_offset = static_cast<uint32_t>(all_vertices.size());
			render_data.vertex_count = static_cast<uint32_t>(vertices.size());
			render_data.index_offset = static_cast<uint32_t>(all_indices.size());
			render_data.transform_index = static_cast<uint32_t>(all_transforms.size());
			render_data.material_index = static_cast<uint32_t>(all_materials.size());
			
			// Add vertices
			all_vertices.insert(all_vertices.end(), vertices.begin(), vertices.end());
			
			// DEBUG: Print the vertices we're adding (only for first entity to reduce spam)
			if (logger && entity_handles.empty()) {
				logger->Debug(std::format("Adding {} vertices for entity {}", vertices.size(), entity_handles.size()), "VKRender");
				for (size_t v = 0; v < std::min(vertices.size(), size_t(3)); ++v) { // Only log first 3 vertices
					const auto& vertex = vertices[v];
					logger->Debug(std::format("  Vertex {}: pos=({:.3f},{:.3f},{:.3f},{:.3f}), color=({:.3f},{:.3f},{:.3f},{:.3f})",
						v, vertex.position.x, vertex.position.y, vertex.position.z, vertex.position.w,
						vertex.color.x, vertex.color.y, vertex.color.z, vertex.color.w), "VKRender");
				}
			}
			
			// Add indices from faces - IMPORTANT: Use absolute indices, not relative to vertex_offset
			for (const auto& face : faces)
			{
				const auto& face_indices = face.GetVertexIndices();
				if (logger && entity_handles.empty()) { // Only log for first entity
					logger->Debug(std::format("Face with {} indices", face_indices.size()), "VKRender");
				}
				for (uint32_t index : face_indices)
				{
					// Index should be absolute in the global vertex buffer
					uint32_t absolute_index = render_data.vertex_offset + index;
					all_indices.push_back(absolute_index);
				}
			}
			
			render_data.index_count = static_cast<uint32_t>(all_indices.size()) - render_data.index_offset;
			
			// Add transform matrix
			all_transforms.push_back(entity.transform_comp.GetModelMatrix());
			
			// Add material (bright red so we can see it clearly)
			vulkan::GPUMaterial material = {{1.0f, 0.0f, 0.0f}, 0, {1.0f, 0.0f, 0.0f}, 0, {1.0f, 1.0f, 1.0f}, 0, 32.0f, {0.0f, 0.0f, 0.0f}};
			all_materials.push_back(material);
			
			entity_handles.push_back(render_data);
			
			if (logger) {
				logger->Debug(std::format("Entity {} - vertices: {} (offset {}), indices: {} (offset {}), transform: {}, material: {}",
					entity_handles.size() - 1, render_data.vertex_count, render_data.vertex_offset, 
					render_data.index_count, render_data.index_offset, 
					render_data.transform_index, render_data.material_index), "VKRender");
			}
		}
		
		// Allocate buffers ONCE
		if (!all_vertices.empty())
		{
			if (logger) {
				logger->Debug(std::format("Allocating static buffers: {} vertices, {} indices, {} transforms, {} materials", 
					all_vertices.size(), all_indices.size(), all_transforms.size(), all_materials.size()), "VKRender");
			}
			
			vertex_handle = global_manager->AllocateVertexData(all_vertices);
			index_handle = global_manager->AllocateIndexData(all_indices);
			transform_handle = global_manager->AllocateObjectTransforms(all_transforms);
			material_handle = global_manager->AllocateMaterials(all_materials);
			
			if (vertex_handle.IsValid() && index_handle.IsValid() && transform_handle.IsValid() && material_handle.IsValid())
			{
				if (logger) {
					logger->Debug("Successfully allocated all static buffers", "VKRender");
				}
				buffers_allocated = true;
			}
			else
			{
				if (logger) {
					logger->Error("Failed to allocate static buffers!", "VKRender");
				}
			}
		}
	}
	else if (buffers_allocated)
	{
		// Buffers are already allocated, just update the entity handles for drawing
		for (const auto& entity : renderable_entities)
		{
			if (!entity.render_comp.visible || !entity.render_comp.mesh)
				continue;
			
			const auto& mesh = entity.render_comp.mesh;
			const auto& vertices = mesh->GetVertices();
			const auto& faces = mesh->GetFaces();
			
			if (vertices.empty() || faces.empty())
				continue;
			
			vulkan::EntityRenderData render_data;
			render_data.vertex_offset = 0; // Static allocation starts at 0
			render_data.vertex_count = static_cast<uint32_t>(vertices.size());
			render_data.index_offset = 0;  // Static allocation starts at 0
			render_data.index_count = static_cast<uint32_t>(faces.size() * 3); // Assume triangles
			render_data.transform_index = 0; // First transform
			render_data.material_index = 0;  // First material
			
			entity_handles.push_back(render_data);
			break; // Only process first entity for now since we have static allocation
		}
	}
	
	// Update camera data (this is lightweight)
	UpdateCameraData(surface->GetCurrentFrameIndex());
}

void Renderer::UpdateCameraData(uint32_t frame_index)
{
	auto* global_manager = vulkan::VulkanHandler::GetGlobalResourceManager();
	if (!global_manager)
		return;

	// CRITICAL FIX: Always ensure frame index is properly bounded
	uint32_t safe_frame_index = frame_index % vulkan::MAX_FRAMES;
	
	// Only log occasionally to reduce spam
	static int debug_counter = 0;
	auto* app = vulkan::VulkanHandler::GetApp();
	auto* logger = app ? app->GetLogger() : nullptr;
	
	if (logger && debug_counter % 120 == 0) { // Log every 120 frames (2 seconds at 60fps)
		logger->Debug(std::format("UpdateCameraData called with frame_index={}, using safe_frame_index={} (MAX_FRAMES={})", 
			frame_index, safe_frame_index, vulkan::MAX_FRAMES), "VKRender");
	}

	// Create camera data with proper Vulkan conventions
	vulkan::CameraData camera_data;
	
	// Camera setup - position camera to see the geometry clearly
	glm::vec3 eye = glm::vec3(0.0f, 0.0f, 3.0f);  // Move camera further back
	glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);  // Look at origin
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);  // Y-up
	
	// Use GLM's lookAt function (right-handed by default)
	camera_data.view = glm::lookAt(eye, center, up);
	
	// Create perspective projection matrix with reasonable settings
	float aspect = static_cast<float>(surface->GetExtent().width) / static_cast<float>(surface->GetExtent().height);
	camera_data.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);  // Standard 45 degree FOV
	
	// Vulkan clip space conventions: Y is flipped, Z range is [0,1] instead of [-1,1]
	// Fix Y coordinate (Vulkan's Y points down in clip space)
	camera_data.proj[1][1] *= -1.0f;
	
	camera_data.pos = eye;

	// Update camera data for current frame using the safe index
	global_manager->UpdateCameraData(safe_frame_index, camera_data);
	
	// DEBUG: Print camera matrices occasionally
	if (logger && debug_counter % 120 == 0) {  // Print every 120 frames
		logger->Debug(std::format("Camera [frame {}->{}]: eye=({:.2f},{:.2f},{:.2f}), aspect={:.2f}", 
			frame_index, safe_frame_index, eye.x, eye.y, eye.z, aspect), "VKRender");
		logger->Debug(std::format("View matrix: [{:.2f} {:.2f} {:.2f} {:.2f}] [{:.2f} {:.2f} {:.2f} {:.2f}] [{:.2f} {:.2f} {:.2f} {:.2f}] [{:.2f} {:.2f} {:.2f} {:.2f}]", 
			camera_data.view[0][0], camera_data.view[0][1], camera_data.view[0][2], camera_data.view[0][3],
			camera_data.view[1][0], camera_data.view[1][1], camera_data.view[1][2], camera_data.view[1][3],
			camera_data.view[2][0], camera_data.view[2][1], camera_data.view[2][2], camera_data.view[2][3],
			camera_data.view[3][0], camera_data.view[3][1], camera_data.view[3][2], camera_data.view[3][3]), "VKRender");
	}
	debug_counter++;
}

void Renderer::InitializeTestGeometry()
{
	auto* global_manager = vulkan::VulkanHandler::GetGlobalResourceManager();
	if (!global_manager)
		return;

	// Create a simple triangle
	std::vector<graphics::Vertex> triangle_vertices = {
		{{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},  // Bottom left - red
		{{ 0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},  // Bottom right - green
		{{ 0.0f,  0.5f, 0.0f, 1.0f}, {0.5f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}   // Top center - blue
	};

	std::vector<uint32_t> triangle_indices = { 0, 1, 2 };

	// Upload vertex data to bindless buffers
	vertex_handle = global_manager->AllocateVertexData(triangle_vertices);
	index_handle = global_manager->AllocateIndexData(triangle_indices);

	// Create a simple transform matrix (identity for now)
	std::vector<glm::mat4> transforms = { glm::mat4(1.0f) };
	transform_handle = global_manager->AllocateObjectTransforms(transforms);

	// Create a simple material
	std::vector<vulkan::GPUMaterial> materials = {
		{
			{0.2f, 0.2f, 0.2f}, 0, // ambient color, no ambient texture
			{0.8f, 0.8f, 0.8f}, 0, // diffuse color, no diffuse texture
			{1.0f, 1.0f, 1.0f}, 0, // specular color, no specular texture
			32.0f, {0.0f, 0.0f, 0.0f} // specular intensity, padding
		}
	};
	material_handle = global_manager->AllocateMaterials(materials);

	// Initialize camera data for all frames
	for (uint32_t i = 0; i < vulkan::MAX_FRAMES; ++i)
	{
		UpdateCameraData(i);
	}
	
	// Set up simple fallback entity
	vulkan::EntityRenderData fallback_entity;
	fallback_entity.vertex_offset = 0;
	fallback_entity.vertex_count = 3;
	fallback_entity.index_offset = 0;
	fallback_entity.index_count = 3;
	fallback_entity.transform_index = 0;
	fallback_entity.material_index = 0;
	
	entity_handles.clear();
	entity_handles.push_back(fallback_entity);
}

} // namespace nft::graphics
