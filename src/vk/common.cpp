//=============================================================================
// VULKAN COMMON IMPLEMENTATION
//=============================================================================
// This file provides the implementation for common Vulkan functionality,
// primarily the dynamic dispatch loader for Vulkan C++ bindings.

#include "vk/Common.h"

//=============================================================================
// DYNAMIC DISPATCH LOADER IMPLEMENTATION
//=============================================================================
// Required for VULKAN_HPP_DISPATCH_LOADER_DYNAMIC mode

// Define the default dispatch loader dynamic storage
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
