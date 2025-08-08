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
	Window(const Window&)			 = delete;
	Window& operator=(const Window&) = delete;

	std::string GetTitle() const { return title; };
	GLFWwindow* GetGLFWWindow() const { return window; };
	void		SetTitle(const std::string& new_title)
	{
		title = new_title;
		glfwSetWindowTitle(window, title.c_str());
	};
	void SetSize(int new_width, int new_height)
	{
		width  = new_width;
		height = new_height;
		glfwSetWindowSize(window, width, height);
	};
	int	 GetWidth() const { return width; };
	int	 GetHeight() const { return height; };
	void PollEvents() const { glfwPollEvents(); };
	void SwapBuffers() const { glfwSwapBuffers(window); };
	bool ShouldClose() const { return glfwWindowShouldClose(window); };

  private:
	int	width;
	int	height;
	std::string title;

	GLFWwindow* window;

	void Init();
};
}	 // namespace nft::GUI
