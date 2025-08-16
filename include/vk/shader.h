#pragma once

#include "vk/common.h"

namespace nft::vulkan
{
// Forward declarations
class Instance;
class Device;

//=============================================================================
// VULKAN SHADER CLASS
//=============================================================================
// Manages Vulkan shader modules and their associated data

class Shader
{
public:
    //=========================================================================
    // NESTED STRUCTURES
    //=========================================================================
    
    // Shader bytecode container
    struct ShaderCode
    {
        uint32_t*    data; // Pointer to SPIR-V bytecode
        const size_t size; // Size of bytecode in bytes
    };

    //=========================================================================
    // CONSTRUCTOR & DESTRUCTOR
    //=========================================================================
    Shader(Device* device, ShaderCode code);
    ~Shader();

    //=========================================================================
    // PUBLIC GETTERS (const methods for read-only access)
    //=========================================================================
    App* GetApp() const { return app; }
    Instance* GetInstance() const { return instance; }
    Device* GetDevice() const { return device; }
    
    // Shader data access (read-only)
    const ShaderCode& GetShaderCode() const { return code; }
    const vk::ShaderModule& GetShaderModule() const { return vk_shader_module; }
    const vk::ShaderModuleCreateInfo& GetShaderModuleInfo() const { return vk_shader_module_info; }

private:
    //=========================================================================
    // PRIVATE MEMBER VARIABLES
    //=========================================================================
    
    // Core references
    App*      app      = nullptr;
    Instance* instance = nullptr;
    Device*   device   = nullptr;

    // Shader data
    ShaderCode                 code;
    vk::ShaderModule           vk_shader_module;
    vk::ShaderModuleCreateInfo vk_shader_module_info;
};

} // namespace nft::vulkan
