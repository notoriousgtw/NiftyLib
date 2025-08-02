#include "NiftyWindow.h"

namespace Nifty::GUI
{
void Window::Init()
{
	// Initialize GLFW window
	window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
	if (window == NULL) throw("Failed to create window!");
}
}	 // namespace Nifty::GUI