#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace nft::GUI
{
class Window
{
  public:
	Window(int width, int height, std::string title): width(width), height(height), title(title) { Init(); };
	std::string GetTitle() const { return title; };
	bool ShouldClose() const { return glfwWindowShouldClose(window); };

	Window(const Window&)			 = delete;
	Window& operator=(const Window&) = delete;

  private:
	const int	width;
	const int	height;
	std::string title;

	GLFWwindow* window;

	void Init();
};
}	 // namespace nft::GUI
