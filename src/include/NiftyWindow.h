#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace Nifty::GUI
{
class Window
{
  public:
	Window(int width, int height, std::string title): width(width), height(height), title(title) { Init(); };
	bool ShouldClose() const { return glfwWindowShouldClose(window); };

  private:
	const int	width;
	const int	height;
	std::string title;

	GLFWwindow* window;

	void Init();
};
}	 // namespace Nifty::GUI
