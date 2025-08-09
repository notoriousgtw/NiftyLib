#include "NiftyVKCommon.h"

#if defined(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC)
namespace vk {
namespace detail {
    VULKAN_HPP_STORAGE_API DispatchLoaderDynamic defaultDispatchLoaderDynamic;
}
}
#endif
