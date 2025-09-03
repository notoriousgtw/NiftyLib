#include "gui/window.h"

#include "core/error.h"
#include "core/log.h"
#include "vk/handler.h"
#include "vk/surface.h"  // Add this include for complete Surface type

#include "core/event.h"

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
	event_handler = std::make_unique<Event::EventHandler>(this);
	
	// Get the shared_ptr from VulkanHandler and store it properly
	auto surface_ptr = vulkan::VulkanHandler::AddSurface(this);
	SetSurface(surface_ptr);

	glfwSetWindowUserPointer(window, this);
	glfwSetWindowSizeCallback(window, WindowResizeCallbackStatic);
	glfwSetKeyCallback(window, KeyCallbackStatic);
	glfwSetMouseButtonCallback(window, MouseButtonCallbackStatic);
	glfwSetCursorPosCallback(window, MouseMoveCallbackStatic);
}

// void Window::CreateWindowSurface()
// {
//	if (!window)
//		NFT_ERROR(GLFWFatal, "GLFW Window is null when creating window surface!");
//	surface = VulkanHandler::AddSurface(this);
//
//	//vulkan::VulkanHandler::GetPrimarySurface()->Init();
// }

void Window::PollEvents() const
{
	glfwPollEvents();
}

void Window::WindowResizeCallbackStatic(GLFWwindow* window, int width, int height)
{
	Window* window_handler = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (window_handler)
	{
		window_handler->width  = width;
		window_handler->height = height;
		window_handler->event_handler->Notify<Event::WindowResizeEvent>(window_handler->event_handler.get(), width, height);
	}
}

void Window::KeyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Window* window_handler = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (window_handler)
	{
		uint32_t u_key =
			(key < 0 || key >= GLFW_KEY_LAST) ? static_cast<uint32_t>(Event::Key::Unknown) : static_cast<uint32_t>(key);
		window_handler->event_handler->Notify<Event::KeyEvent>(window_handler->event_handler.get(), u_key, action, mods);
	}
}

void Window::MouseButtonCallbackStatic(GLFWwindow* window, int button, int action, int mods)
{

	Window* window_handler = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (window_handler)
	{
		uint32_t u_button = (button < 0 || button >= GLFW_MOUSE_BUTTON_LAST) ? static_cast<uint32_t>(Event::MouseButton::Unknown)
																			 : static_cast<uint32_t>(button);
		window_handler->event_handler->Notify<Event::MouseButtonEvent>(
			window_handler->event_handler.get(), u_button, action, mods);
	}
}

void Window::MouseMoveCallbackStatic(GLFWwindow* window, double x, double y)
{
	Window* window_handler = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (window_handler)
	{
		window_handler->event_handler->Notify<Event::MouseMoveEvent>(window_handler->event_handler.get(), x, y);
	}
}

}	 // namespace nft::vulkan