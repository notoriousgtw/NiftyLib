#include "NiftyApp.h"

#include "NiftyError.h"
#include "NiftyEvent.h"
#include "NiftyLog.h"
#include "NiftyVK.h"

// #include <chrono>
// #include <thread>

namespace nft
{
App::App(std::string name)
{
	this->name = std::move(name);
	AutoShowConsole();
	logger = Logger(this->name);
#ifdef _DEBUG
	logger.SetVerbose(true);
#endif

	ErrorHandler::Init(this);
	//EventHandler::Init(this);
}

void App::Init()
{
	PreInit();

	// Initialize GLFW
	if (!glfwInit()) NFT_ERROR(GLFWFatal, "Failed to initialize GLFW!");

	// Set GL Version
	const char* glsl_version = "#version 330";

	// Add main window to stack
	auto window = std::make_unique<GUI::Window>(1280, 720, "Nifty App");
	main_window = window.get();
	windows.insert(std::move(window));

	// Initialize Vulkan
	Vulkan::VulkanHandler::Init(this);

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
	Vulkan::VulkanHandler::ShutDown();
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
