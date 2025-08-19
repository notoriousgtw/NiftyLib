#pragma once

//=============================================================================
// VULKAN COMMON HEADER
//=============================================================================
// This header contains common Vulkan setup and forward declarations
// used throughout the NiftyVK system.

// Platform-specific defines
#ifdef _WIN32
// #define WIN32_LEAN_AND_MEAN	   // Excludes rarely-used services from Windows headers
#define NOMINMAX	// Prevents min/max macro definitions
// #define NODRAWTEXT			   // Excludes DrawText and related APIs
// #define NOGDI				   // Excludes GDI APIs
// #define NOUSER				   // Excludes USER APIs
// #define NONLS				   // Excludes National Language Support APIs

#include <Windows.h>

// Undefine problematic Windows macros that conflict with Vulkan
#ifdef CreateSemaphore
#undef CreateSemaphore
#endif

#ifdef CreateEvent
#undef CreateEvent
#endif

#ifdef CreateMutex
#undef CreateMutex
#endif

#ifdef CreateFence
#undef CreateFence
#endif

#ifdef MemoryBarrier
#undef MemoryBarrier
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#endif	  // _WIN32

// Vulkan C++ bindings configuration
#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif

// Vulkan C++ bindings
#include <vulkan/vulkan.hpp>

// Platform-specific Vulkan headers for presentation support
#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#elif defined(__linux__)
#include <vulkan/vulkan_wayland.h>
#include <vulkan/vulkan_xcb.h>
#include <vulkan/vulkan_xlib.h>
#elif defined(__APPLE__)
#include <vulkan/vulkan_metal.h>
#endif

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE	   // Force depth range to [0, 1] for Vulkan compatibility

// Forward declarations for Nifty framework
namespace nft
{
class App;
class Logger;
}	 // namespace nft

namespace nft::vulkan
{
class Instance;
class Device;
struct ShaderStage;
struct VertexShaderStage;
struct FragmentShaderStage;
struct VertexInputStage;
struct InputAssemblyStage;
struct ViewportStage;
struct RasterizationStage;
struct MultisampleStage;
struct ColorBlendStage;
struct DescriptorSetLayout;
struct DescriptorPool;
struct PipelineLayout;
struct RenderPass;
class Surface;
class Shader;
class Buffer;
class BufferManager;
class Image;
class Texture;
class Scene;
class IMesh;
class SimpleMesh;
class GeometryBatcher;
}	 // namespace nft::vulkan
