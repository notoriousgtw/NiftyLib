#pragma once

#include "core/event_base.h"
#include "core/observer.h"  // Add this for Event::EventHandler
#include "vk/common.h"      // For vk::Extent2D
#include "core/glfw_common.h"
#include <memory>
#include <string>

namespace nft
{
class EventHandler;
}

namespace nft::vulkan
{
// Forward declarations
class Surface;

class Window
{
  public:
	Window(int width, int height, std::string title): width(width), height(height), title(title) { Init(); };
	Window(const Window&)			 = delete;
	Window& operator=(const Window&) = delete;
	~Window() {
		// Clear surface reference before destroying window to prevent double cleanup
		surface.reset();
		
		if (window)
		{
			glfwDestroyWindow(window);
			window = nullptr;	
		}
	};

	// void CreateWindowSurface();

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
	int			 GetWidth() const { return width; };
	int			 GetHeight() const { return height; };
	vk::Extent2D GetExtent() const { return vk::Extent2D(width, height); };

	glm::vec2 GetMousePos() const
	{
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		return glm::vec2(static_cast<float>(x), static_cast<float>(y));
	};

	void	 PollEvents() const;
	void	 SwapBuffers() const { glfwSwapBuffers(window); };
	bool	 ShouldClose() const { return glfwWindowShouldClose(window); };
	bool	 WasResized() const { return framebuffer_resized; };
	void	 ResetResizeFlag() { framebuffer_resized = false; };
	void	 ClearSurface() { surface.reset(); }
	void					 SetSurface(std::shared_ptr<Surface> new_surface) { surface = new_surface; }
	std::shared_ptr<Surface> GetSurface() const { return surface; };

  private:
	int			width;
	int			height;
	std::string title;

	GLFWwindow*				 window;
	std::shared_ptr<Surface> surface;

	std::unique_ptr<Event::EventHandler> event_handler;

	static void WindowResizeCallbackStatic(GLFWwindow* window, int width, int height);
	static void KeyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void MouseButtonCallbackStatic(GLFWwindow* window, int button, int action, int mods);
	static void MouseMoveCallbackStatic(GLFWwindow* window, double x, double y);

	void Init();
	friend class Surface;
	friend class Scene;
	bool framebuffer_resized = false;
};
}	 // namespace nft::vulkan
