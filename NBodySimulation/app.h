#ifndef APP_H
#define APP_H

#include <string>
#include <vector>

#include "base.h"
#include "render.h"

#include "demo1.h"
#include "demo2.h"
#include "demo3.h"
#include "demo4.h"

const int kWindowWidth = 1280;
const int kWindowHeight = 720;

#define VERY_SHORT_BENCHMARK 0

#if !VERY_SHORT_BENCHMARK
const size_t kBenchmarkFrameCount = 250;
const size_t kBenchmarkIterationCount = 4;
#else
const size_t kBenchmarkFrameCount = 10;
const size_t kBenchmarkIterationCount = 2;
#endif
const size_t kDemoCount = 4;

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

struct FrameStatistics {
	SPHStatistics stats;
	float frameTime;

	FrameStatistics() {
		this->stats = SPHStatistics();
		this->frameTime = 0.0f;
	}

	FrameStatistics(const SPHStatistics &stats, const float frameTime) {
		this->stats = stats;
		this->frameTime = frameTime;
	}
};

struct BenchmarkIteration {
	std::vector<FrameStatistics> frames;

	BenchmarkIteration(const size_t maxFrames) {
		frames.reserve(maxFrames);
	}
};

struct DemoStatistics {
	size_t demoIndex;
	size_t scenarioIndex;
	size_t frameCount;
	size_t iterationCount;
	FrameStatistics min;
	FrameStatistics max;
	FrameStatistics avg;
};

struct DemoApplication : public Application {
	bool benchmarkActive;
	bool benchmarkDone;
	std::vector<BenchmarkIteration> benchmarkIterations;
	BenchmarkIteration *activeBenchmarkIteration;

	std::vector<DemoStatistics> demoStats;

	size_t demoIndex;
	BaseSimulation *demo;

	bool simulationActive;
	size_t activeScenarioIndex;
	std::string activeScenarioName;

	void LoadDemo(const size_t demoIndex);

	void PushDemoStatistics();
	void StartBenchmark();
	void StopBenchmark();

	void RenderBenchmark(OSDState *osdState, const float width, const float height);

	DemoApplication();
	~DemoApplication();
	void UpdateAndRender(const float frameTime, const uint64_t cycles);
	void KeyUp(unsigned char key);
	void LoadScenario(size_t scenarioIndex);
};

#endif
