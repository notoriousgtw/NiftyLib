#include "gui/NiftyWindow.h"

#include "core/NiftyError.h"
#include "vk/NiftyVK.h"

namespace nft::GUI
{
void Window::Init()
{
	// Initialize GLFW window
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	//glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
	if (window == NULL) NFT_ERROR(GLFWFatal, std::format("Failed to create window: \"{}\"", title));
	vulkan::VulkanHandler::AddSurface(window);
}
}	 // namespace nft::GUI