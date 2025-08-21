#include "gui/window.h"

#include "core/error.h"
#include "vk/handler.h"

namespace nft::vulkan
{
void Window::Init()
{
	// Initialize GLFW window
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	// glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
	if (window == NULL)
		NFT_ERROR(GLFWFatal, std::format("Failed to create window: \"{}\"", title));
	event_handler = std::make_unique<EventHandler>();
	vulkan::VulkanHandler::AddSurface(this);

	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, KeyCallbackStatic);

}

void Window::PollEvents() const
{
	glfwPollEvents();
}

void Window::KeyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods)
{

	Window* handler = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (handler)
		handler->event_handler->Notify<KeyEvent>(key, scancode, action, mods);
}

}	 // namespace nft::GUI