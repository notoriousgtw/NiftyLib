#pragma once

// #define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <set>

#include "NiftyLog.h"
#include "NiftyWindow.h"

#define NOMINMAX
#include <Windows.h>

namespace nft
{
class App
{
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
	Logger*		GetLogger() { return &logger; }
	std::string GetName() { return name; }
	GUI::Window* GetMainWindow() { return main_window; }

  private:
	std::string name;
	Logger		logger;
	// vk::Instance instance { nullptr };

	GUI::Window*						   main_window;
	std::set<std::unique_ptr<GUI::Window>> windows;

	// void CreateInstance();

	virtual void PreInit()	= 0;
	virtual void PostInit() = 0;
	void		 BeginFrameCore();
	virtual void BeginFrame() = 0;
	virtual void Update()	  = 0;
	virtual void EndFrame()	  = 0;
	void		 EndFrameCore();
	void		 Render();
};
}	 // namespace nft
