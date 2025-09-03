#include "core/observer.h"

#include "core/error.h"
#include "gui/window.h"  // Add this include for Window class definition

namespace nft::Event
{
EventHandler::EventHandler(vulkan::Window* window): window(window)
{
	if (!window)
		NFT_ERROR(EventFatal, "Window is null!");
}

Observer::Observer(EventHandler* event_handler)
{
	if (event_handler)
		this->event_handler = event_handler;
	else
		NFT_ERROR(EventFatal, "EventHandler is null!");
}

MouseHandler::MouseHandler(EventHandler* handler): Observer(handler), raw_mouse_supported(glfwRawMouseMotionSupported())
{
	if (!handler->GetWindow())
		NFT_ERROR(GLFWFatal, "MouseHandler requires a valid Window in EventHandler!");
	Subscribe<MouseButtonEvent>();
	Subscribe<MouseMoveEvent>();
	InitButtonStates();
}

void MouseHandler::EnableRawMouseMotion()
{
	if (raw_mouse_supported)
		glfwSetInputMode(event_handler->GetWindow()->GetGLFWWindow(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	else
		NFT_ERROR(EventError, "Raw mouse motion not supported on this system!");
}

void MouseHandler::DisableRawMouseMotion()
{
	if (raw_mouse_supported)
		glfwSetInputMode(event_handler->GetWindow()->GetGLFWWindow(), GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
	else
		NFT_ERROR(EventError, "Raw mouse motion not supported on this system!");
}

void MouseHandler::SetCursorDisabled()
{
	glfwSetInputMode(event_handler->GetWindow()->GetGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void MouseHandler::SetCursorHidden()
{
	glfwSetInputMode(event_handler->GetWindow()->GetGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

void MouseHandler::SetCursorNormal()
{
	glfwSetInputMode(event_handler->GetWindow()->GetGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

}	 // namespace nft::Event