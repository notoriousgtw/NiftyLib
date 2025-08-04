#include "NiftyApp.h"

#include "NiftyError.h"
#include "NiftyLog.h"

//#include <chrono>
//#include <thread>

namespace nft
{
App::App(std::string name)
{
	this->name = std::move(name);
	AutoShowConsole();

	ErrorHandler::Init(this->name);

	ErrorHandler::Warn("test warning");
	ErrorHandler::Error<ColorEncodingError>("test error");
}

void App::Init()
{
	PreInit();

	// Initialize GLFW
	if (!glfwInit()) throw("Failed to initialize GLFW!");

	// Set GL Version
	const char* glsl_version = "#version 330";

	// Add main window to stack
	auto window = std::make_unique<GUI::Window>(1280, 720, "Nifty App");
	main_window = window.get();
	windows.insert(std::move(window));
	//Logger::Debug("Main window created: " + main_window->GetTitle());

	// Get vulkan extension count
	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

	PostInit();
}

void App::Loop()
{
	while (!main_window->ShouldClose())
	{
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
	// glfwDestroyWindow(window);
	glfwTerminate();
}

void App::BeginFrameCore()
{
	//
}

void App::EndFrameCore() {}

void App::Render()
{
	// Render
	// glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	// glClear(GL_COLOR_BUFFER_BIT);
}
}	 // namespace nft
