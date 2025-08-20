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

	//std::vector<std::unique_ptr<Event>> events;
	KeyPressEvent						key_event;

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
	void PollEvents() const;
	void SwapBuffers() const { glfwSwapBuffers(window); };
	bool ShouldClose() const { return glfwWindowShouldClose(window); };

  private:
	int			width;
	int			height;
	std::string title;

	GLFWwindow* window;

	void		HandleKeyEvent(int key, int scancode, int action, int mods);
	static void KeyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		Window* handler = static_cast<Window*>(glfwGetWindowUserPointer(window));
		if (handler)
		{
			if (key >= 0 && key <= GLFW_KEY_LAST)
			{
				if (action == GLFW_PRESS)
					handler->key_event.key_states[key] = true;
				else if (action == GLFW_RELEASE)
					handler->key_event.key_states[key] = false;
			}
			handler->key_event.Notify();
		}
	}

	void Init();
};
}	 // namespace nft::GUI
