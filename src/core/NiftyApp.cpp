#include "core/NiftyApp.h"

#include "core/NiftyError.h"
#include "core/NiftyEvent.h"
#include "core/NiftyLog.h"
#include "vk/NiftyVK.h"

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
	// EventHandler::Init(this);
}

void App::Init()
{
	PreInit();

	// Initialize GLFW
	if (!glfwInit())
		NFT_ERROR(GLFWFatal, "Failed to initialize GLFW!");

	// Set GL Version
	const char* glsl_version = "#version 330";

	// Initialize Vulkan
	vulkan::VulkanHandler::Init(this);

	// Add main window to stack
	auto window = std::make_unique<GUI::Window>(1280, 960, "Nifty App");
	main_window = window.get();
	windows.insert(std::move(window));


	PostInit();
}

void App::Loop()
{
	while (!main_window->ShouldClose())
	{
		glfwPollEvents();
		CalcFrameTime();
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
	vulkan::VulkanHandler::ShutDown();
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
	vulkan::VulkanHandler::Render();
}

void App::CalcFrameTime()
{
	// Get the current time in seconds since the application started
	current_time = glfwGetTime();

	// Calculate the time difference between the current and last frame
	frame_time = static_cast<float>(current_time - last_time);

	// Update the last frame time
	last_time = current_time;

	// Increment the frame count
	num_frames++;

	// Calculate frame rate every second
	if (current_time - static_cast<int>(current_time) < frame_time)
	{
		frame_rate = num_frames;
		num_frames = 0;
	}

	// Update the window title with the frame rate
	if (main_window)
	{
		std::string new_title = name + " - FPS: " + std::to_string(frame_rate);
		main_window->SetTitle(new_title);
	}
	// Log the frame rate for debugging purposes (optional)
#ifdef _DEBUG
	// logger.Debug("Frame Time: " + std::to_string(frame_time) + "s, FPS: " + std::to_string(frame_rate));
#endif
}

}	 // namespace nft
