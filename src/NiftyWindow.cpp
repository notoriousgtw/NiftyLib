#include "NiftyWindow.h"

namespace nft::GUI
{
void Window::Init()
{
	// Initialize GLFW window
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
	if (window == NULL) throw("Failed to create window!");
}
}	 // namespace nft::GUI