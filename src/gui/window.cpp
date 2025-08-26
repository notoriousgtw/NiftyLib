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
	event_handler = std::make_unique<Event::EventHandler>();
	vulkan::VulkanHandler::AddSurface(this);

	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, KeyCallbackStatic);
	glfwSetMouseButtonCallback(window, MouseButtonCallbackStatic);
	glfwSetCursorPosCallback(window, MouseMoveCallbackStatic);
}

//void Window::CreateWindowSurface()
//{
//	if (!window)
//		NFT_ERROR(GLFWFatal, "GLFW Window is null when creating window surface!");
//	surface = VulkanHandler::AddSurface(this);
//
//	//vulkan::VulkanHandler::GetPrimarySurface()->Init();
//}

void Window::PollEvents() const
{
	glfwPollEvents();
}

void Window::KeyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods)
{

	Window* handler = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (handler)
	{
		uint32_t u_key =
			(key < 0 || key >= GLFW_KEY_LAST) ? static_cast<uint32_t>(Event::Key::Unknown) : static_cast<uint32_t>(key);
		handler->event_handler->Notify<Event::KeyEvent>(handler->event_handler.get(), u_key, action, mods);
	}
}

void Window::MouseButtonCallbackStatic(GLFWwindow* window, int button, int action, int mods)
{

	Window* handler = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (handler)
	{
		uint32_t u_button = (button < 0 || button >= GLFW_MOUSE_BUTTON_LAST)
								? static_cast<uint32_t>(Event::MouseButton::Unknown)
								: static_cast<uint32_t>(button);
		handler->event_handler->Notify<Event::MouseButtonEvent>(handler->event_handler.get(), u_button, action, mods);
	}
}

void Window::MouseMoveCallbackStatic(GLFWwindow* window, double x, double y)
{
	Window* handler = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (handler)
	{
		handler->event_handler->Notify<Event::MouseMoveEvent>(handler->event_handler.get(), x, y);
	}
}

}	 // namespace nft::vulkan