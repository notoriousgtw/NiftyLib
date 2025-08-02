#include "NiftyApp.h"

namespace Nifty
{
App::App(std::string name)
{
	this->name = name;

	// Initialize GLFW
	if (!glfwInit()) throw("Failed to initialize GLFW!");

	// Set GL Version
	const char* glsl_version = "#version 330";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	// Get vulkan extension count
	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

	std::cout << "Extension Count: " << extension_count << std::endl;

	glm::mat4 matrix;
	glm::vec4 vector;
	auto	  test = matrix * vector;
}

void App::Loop()
{
	while (/*!glfwWindowShouldClose(window)*/true)
	{
		//glfwSwapBuffers(window);
		glfwPollEvents();
		BeginFrameCore();
		BeginFrame();
		Update();
		EndFrame();
		EndFrameCore();
		Render();
	}
}

App::~App()
{
	// Cleanup
	//glfwDestroyWindow(window);
	glfwTerminate();
}

void App::BeginFrameCore()
{
	// Start ImGui Frame
}

void App::EndFrameCore() {}

void App::Render()
{
	// Render
	// glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	// glClear(GL_COLOR_BUFFER_BIT);
}
}	 // namespace Nifty
