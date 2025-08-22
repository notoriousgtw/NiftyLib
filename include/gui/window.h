#pragma once

#include "core/event_base.h"

#include "core/glfw_common.h"
#include <string>
#include <memory>

namespace nft
{
class EventHandler;
}

namespace nft::vulkan
{
class Window
{
  public:
	Window(int width, int height, std::string title): width(width), height(height), title(title) { Init(); };
	Window(const Window&)			 = delete;
	Window& operator=(const Window&) = delete;

	// std::vector<std::unique_ptr<Event>> events;
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

	glm::vec2 GetMousePos() const
	{
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		return glm::vec2(static_cast<float>(x), static_cast<float>(y));
	};

	void PollEvents() const;
	void SwapBuffers() const { glfwSwapBuffers(window); };
	bool ShouldClose() const { return glfwWindowShouldClose(window); };

  private:
	int			width;
	int			height;
	std::string title;

	GLFWwindow*	 window;
	std::unique_ptr<EventHandler> event_handler;

	static void KeyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void MouseButtonCallbackStatic(GLFWwindow* window, int button, int action, int mods);
	static void MouseMoveCallbackStatic(GLFWwindow* window, double x, double y);

	void Init();
	friend class Surface;
	friend class Scene;
};
}	 // namespace nft::GUI
