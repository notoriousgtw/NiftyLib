#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iomanip>
#include "glad/glad.h"
#include "GLFW/glfw3.h"

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
	ImGuiIO* io;
	GLFWwindow * window;

	void BeginFrameCore();
	virtual void BeginFrame() = 0;
	virtual void Update() = 0;
	virtual void EndFrame() = 0;
	void EndFrameCore();
	void Render();
};
}
