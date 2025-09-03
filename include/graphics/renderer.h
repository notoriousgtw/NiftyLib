#pragma once

#include "graphics/scene.h"
#include "vk/handler.h"
#include "vk/pipeline.h"  // Add this to access EntityRenderData

#include "generated/simple_shader.frag.spv.h"
#include "generated/simple_shader.vert.spv.h"

namespace nft::graphics
{
class Renderer
{
  public:
	Renderer()	= default;
	~Renderer() 
	{
		// Clean up allocated resources
		auto* global_manager = vulkan::VulkanHandler::GetGlobalResourceManager();
		if (global_manager)
		{
			if (vertex_handle.IsValid())
				global_manager->FreeVertexData(vertex_handle);
			if (index_handle.IsValid())
				global_manager->FreeIndexData(index_handle);
			if (transform_handle.IsValid())
				global_manager->FreeObjectTransforms(transform_handle);
			if (material_handle.IsValid())
				global_manager->FreeMaterials(material_handle);
		}
		
		// Ensure all GPU work is complete before destroying pipelines
		if (surface && surface->GetDevice())
			surface->GetDevice()->GetDevice().waitIdle();
		// managed_pipelines will be destroyed automatically after this point
	}

	void Init(Scene* scene, std::shared_ptr<vulkan::Surface> surface)
	{
		this->scene	= scene;
		this->surface = surface;
	}

	vulkan::SwapchainRenderPipeline* AddPipeline()
	{
		return managed_pipelines
			.emplace_back(std::make_unique<vulkan::SwapchainRenderPipeline>(surface.get(), vulkan::PipelineType::Graphics))
			.get();
	}

	void CreatePipelines()
	{
		for (auto& pipeline : managed_pipelines)
		{
			pipeline->SetVertexShader(vulkan::Shader::ShaderCode { (uint32_t*)simple_shader_vert, simple_shader_vert_len });
			pipeline->SetFragmentShader(vulkan::Shader::ShaderCode { (uint32_t*)simple_shader_frag, simple_shader_frag_len });
			pipeline->Create(surface->GetSwapChainRenderPass());
			
			// Setup bindless descriptors using the global resource manager
			SetupPipelineBindlessResources(pipeline.get());
		}
		
		// REMOVED: Don't initialize test geometry here - let the scene entities handle it
		// InitializeTestGeometry();
	}

	void RecreatePipelines()
	{
		for (auto& pipeline : managed_pipelines)
		{
			pipeline->Recreate(surface->GetSwapChainRenderPass());
			
			// Re-setup bindless descriptors after recreation
			//SetupPipelineBindlessResources(pipeline.get());
		}
	}

	// Get access to the global resource manager for uploading data
	vulkan::GlobalBindlessManager* GetResourceManager() 
	{ 
		return vulkan::VulkanHandler::GetGlobalResourceManager(); 
	}

	void DrawFrame();
	
	// Get entity render data for pipeline drawing commands
	const std::vector<vulkan::EntityRenderData>& GetEntityRenderData() const { return entity_handles; }

  private:
	Scene*														  scene;
	std::shared_ptr<vulkan::Surface>							  surface;
	vulkan::GeometryBatcher*									  geometry_batcher = nullptr;	 // For batching geometry
	std::vector<std::unique_ptr<vulkan::SwapchainRenderPipeline>> managed_pipelines;
	
	// Bindless resource handles
	vulkan::ResourceHandle vertex_handle = vulkan::ResourceHandle::INVALID;
	vulkan::ResourceHandle index_handle = vulkan::ResourceHandle::INVALID;
	vulkan::ResourceHandle transform_handle = vulkan::ResourceHandle::INVALID;
	vulkan::ResourceHandle material_handle = vulkan::ResourceHandle::INVALID;
	
	// Entity render data for current frame
	std::vector<vulkan::EntityRenderData> entity_handles;
	
	// Helper methods for bindless rendering
	void UploadSceneDataToBindlessBuffers();
	void UpdateCameraData(uint32_t frame_index);
	
	// Helper method to setup bindless resources for a pipeline
	void SetupPipelineBindlessResources(vulkan::SwapchainRenderPipeline* pipeline)
	{
		// Get the global bindless descriptor layout and pool
		auto* descriptor_pool = vulkan::VulkanHandler::GetBindlessDescriptorPool();
		auto* descriptor_layout = vulkan::VulkanHandler::GetBindlessDescriptorLayout();
		auto* resource_manager = vulkan::VulkanHandler::GetGlobalResourceManager();
		
		if (!descriptor_pool || !descriptor_layout || !resource_manager)
		{
			// Log error or handle gracefully
			return;
		}
		
		// The resource manager already has its descriptor set allocated
		// Pipelines will use the shared bindless descriptor set during rendering
		// No per-pipeline setup needed since we're using a global bindless approach
	}
	
	// Initialize simple test geometry
	void InitializeTestGeometry();
};
}	 // namespace nft::graphics