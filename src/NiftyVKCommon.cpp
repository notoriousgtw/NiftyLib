//=============================================================================
// VULKAN COMMON IMPLEMENTATION
//=============================================================================
// This file provides the implementation for common Vulkan functionality,
// primarily the dynamic dispatch loader for Vulkan C++ bindings.

#include "NiftyVKCommon.h"

//=============================================================================
// DYNAMIC DISPATCH LOADER IMPLEMENTATION
//=============================================================================
// Required for VULKAN_HPP_DISPATCH_LOADER_DYNAMIC mode

#if defined(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC)
namespace vk {
namespace detail {
    VULKAN_HPP_STORAGE_API DispatchLoaderDynamic defaultDispatchLoaderDynamic;
}
}
#endif
