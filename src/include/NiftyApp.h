#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <iostream>

#include <string>
#include <stack>

#include "NiftyGUI.h"
#include "NiftyWindow.h"

namespace Nifty
{
class App
{
  public:
	App() = delete;
	App(std::string name);

	void Loop();

	~App();

  private:
	std::string name;

	std::stack<Nifty::GUI::Window> windows;

	void		 BeginFrameCore();
	virtual void BeginFrame() = 0;
	virtual void Update()	  = 0;
	virtual void EndFrame()	  = 0;
	void		 EndFrameCore();
	void		 Render();
};
}	 // namespace Nifty
