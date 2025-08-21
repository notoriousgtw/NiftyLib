#pragma once

#include "core/log.h"
//#include "core/event.h"
#include "gui/window.h"
#include "vk/scene.h"

#include <set>
#include <vector>

namespace nft
{
class App
{
	using Window = vulkan::Window;
  public:
	App() = delete;
	App(std::string name);

	void Init();
	void Loop();
	void ShowConsole(bool show)
	{
		HWND console_window = GetConsoleWindow();
		if (show)
			ShowWindow(console_window, SW_SHOW);
		else
			ShowWindow(console_window, SW_HIDE);
	}
	void AutoShowConsole()
	{
		HWND console_window = GetConsoleWindow();
#ifdef _DEBUG
		ShowWindow(console_window, SW_SHOW);
#else
		ShowWindow(console_window, SW_HIDE);
#endif
	}

	~App();
	Logger*		 GetLogger() { return &logger; }
	std::string	 GetName() { return name; }
	Window* GetMainWindow() { return main_window; }

  private:
	std::string name;
	Logger		logger;
	// vk::Instance instance { nullptr };

	Window*						   main_window;
	std::set<std::unique_ptr<Window>> windows;

	// void CreateInstance();

	// virtual void RegisterErrors() = 0;
	// virtual void RegisterEvents() = 0;
	virtual void PreInit()	= 0;
	virtual void PostInit() = 0;
	void		 BeginFrameCore();
	virtual void BeginFrame() = 0;
	virtual void Update()	  = 0;
	virtual void EndFrame()	  = 0;
	void		 EndFrameCore();
	void		 Render();

	double last_time = 0;
	double current_time = 0;
	int	   num_frames = 0;
	int	   frame_rate = 0;
	float  frame_time = 0;

	void CalcFrameTime();
};
}	 // namespace nft
