#ifndef APP_H
#define APP_H

#include <string>
#include <vector>

#if DEMO == 1
#include "demo1.h"
#elif DEMO == 2
#include "demo2.h"
#elif DEMO == 3
#include "demo3.h"
#elif DEMO == 4
#include "demo4.h"
#else
#error "Not supported demo!"
#endif

const int kWindowWidth = 1280;
const int kWindowHeight = 720;
const int kMaxFrameStatisticsCount = 1000;

struct Window {
	int left, top;
	int width, height;

	Window();

	inline int GetLeft() {
		return left;
	}
	inline int GetTop() {
		return top;
	}
	inline int GetWidth() {
		return width;
	}
	inline int GetHeight() {
		return height;
	}
};

struct Application {
	Window  *window;
	std::string title;

	Application(const std::string &title);
	virtual ~Application();

	inline Window *GetWindow() {
		return window;
	}

	virtual void KeyUp(unsigned char key) = 0;

	void Resize(const int width, const int height);
	virtual void UpdateAndRender(const float frametime, const uint64_t cycles) = 0;
};

#if BENCHMARK
struct FrameStatistics {
	SPHStatistics sphStats;
	float frameTime;
};
#endif

struct DemoApplication : public Application {
	ParticleSimulation *simulation;
	size_t activeScenarioIndex;
	std::string activeScenarioName;
	bool simulationActive;
	size_t simulationFrameCount;
#if BENCHMARK
	std::vector<FrameStatistics> frameStatistics;
#endif

	DemoApplication();
	~DemoApplication();
	void UpdateAndRender(const float frameTime, const uint64_t cycles);
	void KeyUp(unsigned char key);
	void LoadScenario(size_t scenarioIndex);
#if BENCHMARK
	void SaveFrameStatistics();
#endif
};

#endif
