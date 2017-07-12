#include "app.h"

#ifndef APP_IMPLEMENTATION
#define APP_IMPLEMENTATION

#include <GL/glew.h>
#include <filesystem>
#include <iostream>
#include <fstream>

#include "sph.h"
#include "pseudorandom.h"

#include "demo1.cpp"
#include "demo2.cpp"
#include "demo3.cpp"
#include "demo4.cpp"

Window::Window() :
	left(0),
	top(0),
	width(kWindowWidth),
	height(kWindowHeight) {
}

Application::Application(const std::string &title) {
	window = new Window();
	this->title = title;
}

Application::~Application() {
	delete window;
}

void Application::Resize(const int width, const int height) {
	window->width = width;
	window->height = height;
}

DemoApplication::DemoApplication() :
	Application("NBody-Simulation") {

	demoStats.reserve(kDemoCount);

	activeScenarioIndex = 0;
	simulationActive = true;
	demoIndex = 0;
	demo = nullptr;
	LoadDemo(demoIndex);

	benchmarkActive = false;
	benchmarkDone = false;
	activeBenchmarkIteration = nullptr;
	benchmarkIterations.reserve(kBenchmarkIterationCount);

	StartBenchmark();
}

DemoApplication::~DemoApplication() {
	delete demo;
}

#if 0
void DemoApplication::SaveFrameStatistics() {
	auto userProfile = std::experimental::filesystem::path(getenv("USERPROFILE"));
	auto desktopFolder = userProfile /= "Desktop";
	auto frameStatFile = desktopFolder /= "nbodysim_framestatistics.csv";
	auto frameStatFilePath = frameStatFile.generic_string();
	{
		std::ofstream outFileStream(frameStatFilePath, std::ios::binary);
		outFileStream << "Num"
			<< ";" << "TotalFrametime"
			<< ";" << "Integration"
			<< ";" << "ViscosityForces"
			<< ";" << "UpdateGrid"
			<< ";" << "NeighborSearch"
			<< ";" << "DensityAndPressure"
			<< ";" << "DeltaPositions"
			<< ";" << "Collisions"
			<< std::endl;
		for (size_t index = 0; index < frameStatistics.size(); ++index) {
			FrameStatistics *frameStats = &frameStatistics[index];
			outFileStream << index
				<< ";" << frameStats->frameTime
				<< ";" << frameStats->sphStats.time.integration
				<< ";" << frameStats->sphStats.time.viscosityForces
				<< ";" << frameStats->sphStats.time.updateGrid
				<< ";" << frameStats->sphStats.time.neighborSearch
				<< ";" << frameStats->sphStats.time.densityAndPressure
				<< ";" << frameStats->sphStats.time.deltaPositions
				<< ";" << frameStats->sphStats.time.collisions
				<< std::endl;
		}
	}
	printf("Saved: %s\n", frameStatFilePath.c_str());
}
#endif

template <typename T>
static void UpdateMin(T &value, T a) {
	value = std::min(value, a);
}

template <typename T>
static void UpdateMax(T &value, T a) {
	value = std::max(value, a);
}

template <typename T>
static void Accumulate(T &value, T a) {
	value += a;
}

void DemoApplication::PushDemoStatistics() {
	DemoStatistics demoStat = DemoStatistics();

	size_t avgCount = 0;
	demoStat.min.frameTime = FLT_MAX;
	demoStat.max.frameTime = 0.0f;
	demoStat.avg.frameTime = 0.0f;

	size_t maxFrameCount = 0;
	const size_t iterationCount = benchmarkIterations.size();
	for (size_t iterationIndex = 0; iterationIndex < iterationCount; ++iterationIndex) {
		BenchmarkIteration *iteration = &benchmarkIterations[iterationIndex];

		size_t frameCount = iteration->frames.size();
		maxFrameCount = std::max(maxFrameCount, frameCount);

		for (size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
			FrameStatistics *frameStat = &iteration->frames[frameIndex];

			UpdateMin(demoStat.min.frameTime, frameStat->frameTime);
			UpdateMin(demoStat.min.stats.time.collisions, frameStat->stats.time.collisions);
			UpdateMin(demoStat.min.stats.time.deltaPositions, frameStat->stats.time.deltaPositions);
			UpdateMin(demoStat.min.stats.time.densityAndPressure, frameStat->stats.time.densityAndPressure);
			UpdateMin(demoStat.min.stats.time.emitters, frameStat->stats.time.emitters);
			UpdateMin(demoStat.min.stats.time.integration, frameStat->stats.time.integration);
			UpdateMin(demoStat.min.stats.time.neighborSearch, frameStat->stats.time.neighborSearch);
			UpdateMin(demoStat.min.stats.time.predict, frameStat->stats.time.predict);
			UpdateMin(demoStat.min.stats.time.updateGrid, frameStat->stats.time.updateGrid);
			UpdateMin(demoStat.min.stats.time.viscosityForces, frameStat->stats.time.viscosityForces);

			UpdateMax(demoStat.max.frameTime, frameStat->frameTime);
			UpdateMax(demoStat.max.stats.time.collisions, frameStat->stats.time.collisions);
			UpdateMax(demoStat.max.stats.time.deltaPositions, frameStat->stats.time.deltaPositions);
			UpdateMax(demoStat.max.stats.time.densityAndPressure, frameStat->stats.time.densityAndPressure);
			UpdateMax(demoStat.max.stats.time.emitters, frameStat->stats.time.emitters);
			UpdateMax(demoStat.max.stats.time.integration, frameStat->stats.time.integration);
			UpdateMax(demoStat.max.stats.time.neighborSearch, frameStat->stats.time.neighborSearch);
			UpdateMax(demoStat.max.stats.time.predict, frameStat->stats.time.predict);
			UpdateMax(demoStat.max.stats.time.updateGrid, frameStat->stats.time.updateGrid);
			UpdateMax(demoStat.max.stats.time.viscosityForces, frameStat->stats.time.viscosityForces);

			Accumulate(demoStat.avg.frameTime, frameStat->frameTime);
			Accumulate(demoStat.avg.stats.time.collisions, frameStat->stats.time.collisions);
			Accumulate(demoStat.avg.stats.time.deltaPositions, frameStat->stats.time.deltaPositions);
			Accumulate(demoStat.avg.stats.time.densityAndPressure, frameStat->stats.time.densityAndPressure);
			Accumulate(demoStat.avg.stats.time.emitters, frameStat->stats.time.emitters);
			Accumulate(demoStat.avg.stats.time.integration, frameStat->stats.time.integration);
			Accumulate(demoStat.avg.stats.time.neighborSearch, frameStat->stats.time.neighborSearch);
			Accumulate(demoStat.avg.stats.time.predict, frameStat->stats.time.predict);
			Accumulate(demoStat.avg.stats.time.updateGrid, frameStat->stats.time.updateGrid);
			Accumulate(demoStat.avg.stats.time.viscosityForces, frameStat->stats.time.viscosityForces);

			++avgCount;
		}
	}

	if (avgCount > 0) {
		const float avg = 1.0f / (float)avgCount;
		demoStat.avg.frameTime *= avg;
		demoStat.avg.stats.time.collisions *= avg;
		demoStat.avg.stats.time.deltaPositions *= avg;
		demoStat.avg.stats.time.densityAndPressure *= avg;
		demoStat.avg.stats.time.emitters *= avg;
		demoStat.avg.stats.time.integration *= avg;
		demoStat.avg.stats.time.neighborSearch *= avg;
		demoStat.avg.stats.time.predict *= avg;
		demoStat.avg.stats.time.updateGrid *= avg;
		demoStat.avg.stats.time.viscosityForces *= avg;
	}

	demoStat.frameCount = maxFrameCount;
	demoStats.push_back(demoStat);
}

void DemoApplication::RenderBenchmark(OSDState *osdState, const float width, const float height) {
	char osdBuffer[256];
	DemoStatistics *firstDemoStat = &demoStats[0];
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Benchmark done, Scenario: %llu, Frames: %llu", (firstDemoStat->scenarioIndex + 1), firstDemoStat->frameCount);
	DrawOSDLine(osdState, osdBuffer);

#if 0
	for (size_t demoStatIndex = 0; demoStatIndex < demoStats.size(); ++demoStatIndex) {
		DemoStatistics *demoStat = &demoStats[demoStatIndex];
		sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Demo %llu", (demoStatIndex + 1));
		DrawOSDLine(osdState, osdBuffer);
		sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tFrametime min: %f, max: %f, avg: %f", demoStat->min.frameTime, demoStat->max.frameTime, demoStat->avg.frameTime);
		DrawOSDLine(osdState, osdBuffer);
		sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tIntegration min: %f, max: %f, avg: %f", demoStat->min.stats.time.integration, demoStat->max.stats.time.integration, demoStat->avg.stats.time.integration);
		DrawOSDLine(osdState, osdBuffer);
		sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tViscosity forces min: %f, max: %f, avg: %f", demoStat->min.stats.time.viscosityForces, demoStat->max.stats.time.viscosityForces, demoStat->avg.stats.time.viscosityForces);
		DrawOSDLine(osdState, osdBuffer);
		sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tPredict min: %f, max: %f, avg: %f", demoStat->min.stats.time.predict, demoStat->max.stats.time.predict, demoStat->avg.stats.time.predict);
		DrawOSDLine(osdState, osdBuffer);
		sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tUpdate grid min: %f, max: %f, avg: %f", demoStat->min.stats.time.updateGrid, demoStat->max.stats.time.updateGrid, demoStat->avg.stats.time.updateGrid);
		DrawOSDLine(osdState, osdBuffer);
		sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tNeighbor search: %f, max: %f, avg: %f", demoStat->min.stats.time.neighborSearch, demoStat->max.stats.time.neighborSearch, demoStat->avg.stats.time.neighborSearch);
		DrawOSDLine(osdState, osdBuffer);
		sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tDensity and pressure: %f, max: %f, avg: %f", demoStat->min.stats.time.densityAndPressure, demoStat->max.stats.time.densityAndPressure, demoStat->avg.stats.time.densityAndPressure);
		DrawOSDLine(osdState, osdBuffer);
		sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tDelta positions: %f, max: %f, avg: %f", demoStat->min.stats.time.deltaPositions, demoStat->max.stats.time.deltaPositions, demoStat->avg.stats.time.deltaPositions);
		DrawOSDLine(osdState, osdBuffer);
		sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tCollisions: %f, max: %f, avg: %f", demoStat->min.stats.time.collisions, demoStat->max.stats.time.collisions, demoStat->avg.stats.time.collisions);
		DrawOSDLine(osdState, osdBuffer);
	}
#endif

	float areaScale = 0.95f;
	float areaWidth = width * areaScale;
	float areaHeight = height * areaScale;
	float areaLeft = (width - areaWidth) / 2.0f;
	float areaBottom = (height - areaHeight) / 2.0f;

	float sampleLabelTextScale = 0.9f;
	float sampleAxisMargin = 10.0f;
	float sampleAxisHeight = ((float)osdState->fontHeight * sampleLabelTextScale) + (sampleAxisMargin * 2.0f);

	float legendLabelTextScale = 0.75f;
	float legendLabelPadding = 20.0f;
	float legendBulletPadding = 10.0f;
	float legendMargin = 0.0f;
	float legendHeight = ((float)osdState->fontHeight * legendLabelTextScale) + (legendMargin * 2.0f);
	float legendBulletSize = (float)osdState->fontHeight * legendLabelTextScale * 0.75f;

	float chartOriginX = areaLeft;
	float chartOriginY = areaBottom + sampleAxisHeight + legendHeight;
	float chartHeight = areaHeight - sampleAxisHeight - legendHeight;
	float chartWidth = areaWidth;

	const int seriesCount = kDemoCount;

	std::vector<std::string> legendLabels;
	legendLabels.push_back("Demo 1");
	legendLabels.push_back("Demo 2");
	legendLabels.push_back("Demo 3");
	legendLabels.push_back("Demo 4");
	assert(seriesCount == legendLabels.size());

	std::vector<std::string> sampleLabels;
	sampleLabels.push_back("Frametime");
	sampleLabels.push_back("Integration");
	sampleLabels.push_back("Viscosity forces");
	sampleLabels.push_back("Predict");
	sampleLabels.push_back("Update grid");
	sampleLabels.push_back("Neighbor search");
	sampleLabels.push_back("Density and pressure");
	sampleLabels.push_back("Delta positions");
	sampleLabels.push_back("Collisions");
	const size_t sampleCount = sampleLabels.size();

	RandomSeries colorRandomSeries = RandomSeed(1337);
	std::vector<Vec4f> seriesColors;
	for (int seriesIndex = 0; seriesIndex < seriesCount; ++seriesIndex) {
		seriesColors.push_back(RandomColor(&colorRandomSeries));
	}
	assert(seriesColors.size() == legendLabels.size());

	std::vector<float> values[seriesCount];
	RandomSeries valueRandomSeries = RandomSeed(1337);
	for (int seriesIndex = 0; seriesIndex < seriesCount; ++seriesIndex) {
		DemoStatistics *demoStat = &demoStats[seriesIndex];
		values[seriesIndex].reserve(sampleCount);
		values[seriesIndex].push_back(demoStat->avg.frameTime * 1000.0f);
		values[seriesIndex].push_back(demoStat->avg.stats.time.integration);
		values[seriesIndex].push_back(demoStat->avg.stats.time.viscosityForces);
		values[seriesIndex].push_back(demoStat->avg.stats.time.predict);
		values[seriesIndex].push_back(demoStat->avg.stats.time.updateGrid);
		values[seriesIndex].push_back(demoStat->avg.stats.time.neighborSearch);
		values[seriesIndex].push_back(demoStat->avg.stats.time.densityAndPressure);
		values[seriesIndex].push_back(demoStat->avg.stats.time.deltaPositions);
		values[seriesIndex].push_back(demoStat->avg.stats.time.collisions);
		assert(sampleCount == values[seriesIndex].size());
	}

	float minValue = 0;
	float maxValue = 0;
	for (int seriesIndex = 0; seriesIndex < seriesCount; ++seriesIndex) {
		for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
			minValue = std::min(minValue, values[seriesIndex][sampleIndex]);
			maxValue = std::max(maxValue, values[seriesIndex][sampleIndex]);
		}
	}
	float valueRange = maxValue - minValue;
	float invValueRange = 1.0f / valueRange;

	float sampleWidth = chartWidth / (float)sampleCount;
	float sampleMargin = 10;
	float subSampleMargin = 5;

	// Chart area
	glColor4f(0.1f, 0.1f, 0.1f, 1);
	glBegin(GL_QUADS);
	glVertex2f(areaLeft, areaBottom);
	glVertex2f(areaLeft + areaWidth, areaBottom);
	glVertex2f(areaLeft + areaWidth, areaBottom + areaHeight);
	glVertex2f(areaLeft, areaBottom + areaHeight);
	glEnd();

	// Sample lines
	glColor4f(0.25f, 0.25f, 0.25f, 1);
	glLineWidth(1);
	for (int sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex) {
		glBegin(GL_LINES);
		glVertex2f(areaLeft + (float)sampleIndex * sampleWidth, areaBottom + legendHeight);
		glVertex2f(areaLeft + (float)sampleIndex * sampleWidth, areaBottom + legendHeight + areaHeight);
		glEnd();
	}
	glLineWidth(1);

	// Axis lines
	glColor4f(0.65f, 0.65f, 0.65f, 1);
	glLineWidth(1);
	glBegin(GL_LINES);
	glVertex2f(chartOriginX, chartOriginY);
	glVertex2f(chartOriginX + chartWidth, chartOriginY);
	glVertex2f(chartOriginX, chartOriginY);
	glVertex2f(chartOriginX, chartOriginY + chartHeight);
	glEnd();
	glLineWidth(1);

	// Sample labels
	for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
		const char *sampleLabel = sampleLabels[sampleIndex].c_str();
		float textWidth = (float)GetTextWidth(sampleLabel) * sampleLabelTextScale;
		float xLeft = chartOriginX + (float)sampleIndex * sampleWidth + sampleWidth * 0.5f - textWidth * 0.5f;
		float yMiddle = chartOriginY - sampleAxisHeight * 0.5f;
		RenderText(xLeft, yMiddle, sampleLabel, sampleLabelTextScale, Vec4f(1, 1, 1, 1));
	}

	// Legend
	float legendCurLeft = areaLeft;
	float legendMiddle = areaBottom + legendHeight * 0.5f + legendMargin;
	for (int legendLabelIndex = 0; legendLabelIndex < legendLabels.size(); ++legendLabelIndex) {
		Vec4f legendColor = seriesColors[legendLabelIndex];
		FillRectangle(Vec2f(legendCurLeft, legendMiddle), Vec2f(legendBulletSize, legendBulletSize), legendColor);
		legendCurLeft += legendBulletSize + legendBulletPadding;

		const char *legendLabel = legendLabels[legendLabelIndex].c_str();
		float labelWidth = GetTextWidth(legendLabel) * legendLabelTextScale;
		RenderText(legendCurLeft, legendMiddle, legendLabel, legendLabelTextScale, Vec4f(1, 1, 1, 1));
		legendCurLeft += labelWidth + legendLabelPadding;
	}

	// Bars
	float barWidth = sampleWidth - (sampleMargin * 2.0f);
	float seriesBarWidth = (barWidth - (subSampleMargin * (float)(seriesCount - 1))) / (float)seriesCount;
	for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
		for (int seriesIndex = 0; seriesIndex < seriesCount; ++seriesIndex) {
			Vec4f seriesColor = seriesColors[seriesIndex];
			float value = values[seriesIndex][sampleIndex];
			float sampleHeight = (value * invValueRange) * chartHeight;
			float sampleLeft = chartOriginX + (float)sampleIndex * sampleWidth + sampleMargin + ((float)seriesIndex * seriesBarWidth) + ((float)seriesIndex * subSampleMargin);
			float sampleRight = sampleLeft + seriesBarWidth;
			float sampleBottom = chartOriginY;
			float sampleTop = chartOriginY + sampleHeight;
			FillRectangle(Vec2f(sampleLeft, sampleBottom), Vec2f(abs(sampleRight - sampleLeft), abs(sampleBottom - sampleTop)), seriesColor);
		}
	}
}

void DemoApplication::UpdateAndRender(const float frameTime, const uint64_t cycles) {
	if (simulationActive) {
		for (int step = 0; step < kSPHSubsteps; ++step) {
			demo->Update(kSPHSubstepDeltaTime);
		}

		if (benchmarkActive) {
			assert(activeBenchmarkIteration != nullptr);
			activeBenchmarkIteration->frames.push_back(FrameStatistics(demo->GetStats(), frameTime));

			if (activeBenchmarkIteration->frames.size() == kBenchmarkFrameCount) {
				// Iteration complete
				if (benchmarkIterations.size() == kBenchmarkIterationCount) {

					// Calculate and add demo statistics
					PushDemoStatistics();

					// Demo complete
					if (demoIndex == (kDemoCount - 1)) {
						// Benchmark complete
						simulationActive = false;
						benchmarkDone = true;
						benchmarkActive = false;
						activeBenchmarkIteration = nullptr;
					} else {
						// Next demo
						demoIndex++;
						LoadDemo(demoIndex);

						benchmarkIterations.clear();
						benchmarkIterations.push_back(BenchmarkIteration(kBenchmarkFrameCount));
						activeBenchmarkIteration = &benchmarkIterations[0];
					}
				} else {
					// Next iteration
					benchmarkIterations.push_back(BenchmarkIteration(kBenchmarkFrameCount));
					activeBenchmarkIteration = &benchmarkIterations[benchmarkIterations.size() - 1];
					LoadScenario(activeScenarioIndex);
				}
			}
		}
	}

	float left = -kSPHBoundaryHalfWidth;
	float right = kSPHBoundaryHalfWidth;
	float top = kSPHBoundaryHalfHeight;
	float bottom = -kSPHBoundaryHalfHeight;

	int w = window->width;
	int h = window->height;
	glViewport(0, 0, w, h);

	float worldToScreenScale = (float)w / kSPHBoundaryWidth;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(left, right, bottom, top, 0.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!benchmarkDone && !benchmarkActive) {
		demo->Render(worldToScreenScale);
	}

	char osdBuffer[256];
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, 0, h, 0.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// OSD
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	OSDState osdState = CreateOSD();
	osdState.charY = h - osdState.fontHeight;

#if 0
	RenderBenchmark(&osdState, (float)w, (float)h);
#else
	if (!benchmarkActive) {
		if (benchmarkDone && (demoStats.size() > 0)) {
			RenderBenchmark(&osdState, (float)w, (float)h);
		} else {
			size_t scenarioCount = ArrayCount(SPHScenarios);
			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "%s - [%llu / %llu] %s (Space)", title.c_str(), (activeScenarioIndex + 1), scenarioCount, activeScenarioName.c_str());
			DrawOSDLine(&osdState, osdBuffer);
			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Simulation: %s (P)", (simulationActive ? "yes" : "no"));
			DrawOSDLine(&osdState, osdBuffer);
			if (demo->IsMultiThreadingSupported()) {
				sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Multithreading: %s, %llu threads (T)", (demo->IsMultiThreading() ? "yes" : "no"), demo->GetWorkerThreadCount());
			} else {
				sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Multithreading: not supported");
			}
			DrawOSDLine(&osdState, osdBuffer);
			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Reset (R)");
			DrawOSDLine(&osdState, osdBuffer);
			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Frame time: %f ms, Cycles: %llu", (frameTime * 1000.0f), cycles);
			DrawOSDLine(&osdState, osdBuffer);
			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Particles: %llu", demo->GetParticleCount());
			DrawOSDLine(&osdState, osdBuffer);

			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Stats:");
			DrawOSDLine(&osdState, osdBuffer);
			const SPHStatistics &stats = demo->GetStats();
			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tMin/Max cell particle count: %llu / %llu", stats.minCellParticleCount, stats.maxCellParticleCount);
			DrawOSDLine(&osdState, osdBuffer);
			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tMin/Max particle neighbor count: %llu / %llu", stats.minParticleNeighborCount, stats.maxParticleNeighborCount);
			DrawOSDLine(&osdState, osdBuffer);
			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tTime integration: %f ms", stats.time.integration);
			DrawOSDLine(&osdState, osdBuffer);
			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tTime viscosity forces: %f ms", stats.time.viscosityForces);
			DrawOSDLine(&osdState, osdBuffer);
			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tTime predict: %f ms", stats.time.predict);
			DrawOSDLine(&osdState, osdBuffer);
			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tTime update grid: %f ms", stats.time.updateGrid);
			DrawOSDLine(&osdState, osdBuffer);
			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tTime neighbor search: %f ms", stats.time.neighborSearch);
			DrawOSDLine(&osdState, osdBuffer);
			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tTime density and pressure: %f ms", stats.time.densityAndPressure);
			DrawOSDLine(&osdState, osdBuffer);
			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tTime delta positions: %f ms", stats.time.deltaPositions);
			DrawOSDLine(&osdState, osdBuffer);
			sprintf_s(osdBuffer, ArrayCount(osdBuffer), "\tTime collisions: %f ms", stats.time.collisions);
			DrawOSDLine(&osdState, osdBuffer);
		}
	} else {
		sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Benchmarking - Demo %llu of %llu, Scenario: %llu (Escape)", demoIndex + 1, (size_t)4, (activeScenarioIndex + 1));
		DrawOSDLine(&osdState, osdBuffer);
		sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Iteration %llu of %llu", benchmarkIterations.size(), kBenchmarkIterationCount);
		DrawOSDLine(&osdState, osdBuffer);
		assert(activeBenchmarkIteration != nullptr);
		sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Frame %llu of %llu", activeBenchmarkIteration->frames.size() + 1, kBenchmarkFrameCount);
		DrawOSDLine(&osdState, osdBuffer);
}
#endif
}

void DemoApplication::LoadDemo(const size_t demoIndex) {
	if (demo != nullptr) {
		delete demo;
	}
	switch (demoIndex) {
		case 0:
		{
			demo = new Demo1::ParticleSimulation();
			title = Demo1::kDemoName;
		} break;
		case 1:
		{
			demo = new Demo2::ParticleSimulation();
			title = Demo2::kDemoName;
		} break;
		case 2:
		{
			demo = new Demo3::ParticleSimulation();
			title = Demo3::kDemoName;
		} break;
		case 3:
		{
			demo = new Demo4::ParticleSimulation();
			title = Demo4::kDemoName;
		} break;
		default:
			assert(false);
	}
	LoadScenario(activeScenarioIndex);
}

void DemoApplication::StartBenchmark() {
	benchmarkActive = true;
	benchmarkDone = false;

	benchmarkIterations.clear();
	benchmarkIterations.push_back(BenchmarkIteration(kBenchmarkFrameCount));
	activeBenchmarkIteration = &benchmarkIterations[0];

	demoStats.clear();

	simulationActive = true;
	demoIndex = 0;
	LoadDemo(demoIndex);
}

void DemoApplication::StopBenchmark() {
	simulationActive = false;
	benchmarkActive = false;
	benchmarkDone = true;
	activeBenchmarkIteration = nullptr;
}

void DemoApplication::KeyUp(unsigned char key) {
	if (!benchmarkActive) {
		if (key == ' ') {
			activeScenarioIndex = (activeScenarioIndex + 1) % ArrayCount(SPHScenarios);
			LoadScenario(activeScenarioIndex);
		} else if (key == 'p') {
			simulationActive = !simulationActive;
		} else if (key == 'd') {
			demoIndex = (demoIndex + 1) % 4;
			LoadDemo(demoIndex);
		} else if (key == 'r') {
			LoadScenario(activeScenarioIndex);
		} else if (key == 't' && demo->IsMultiThreadingSupported()) {
			demo->ToggleMultiThreading();
		} else if (key == 'b') {
			StartBenchmark();
		}
	} else {
		if (key == 27) {
			StopBenchmark();
		}
	}
}

void DemoApplication::LoadScenario(size_t scenarioIndex) {
	SPHScenario *scenario = &SPHScenarios[scenarioIndex];
	activeScenarioName = scenario->name;
	demo->ResetStats();
	demo->ClearBodies();
	demo->ClearParticles();
	demo->ClearEmitters();
	demo->SetGravity(scenario->gravity);
	demo->SetParams(scenario->parameters);

	// Bodies
	for (size_t bodyIndex = 0; bodyIndex < scenario->bodyCount; ++bodyIndex) {
		SPHScenarioBody *body = &scenario->bodies[bodyIndex];
		switch (body->type) {
			case SPHScenarioBodyType::SPHScenarioBodyType_Plane:
			{
				float distance = Vec2Dot(body->orientation.col1, body->position);
				demo->AddPlane(body->orientation.col1, distance);
			} break;
			case SPHScenarioBodyType::SPHScenarioBodyType_Circle:
			{
				demo->AddCircle(body->position, body->radius);
			} break;
			case SPHScenarioBodyType::SPHScenarioBodyType_LineSegment:
			{
				assert(body->vertexCount == 2);
				Vec2f a = Vec2MultMat2(body->orientation, body->localVerts[0]) + body->position;
				Vec2f b = Vec2MultMat2(body->orientation, body->localVerts[1]) + body->position;
				demo->AddLineSegment(a, b);
			} break;
			case SPHScenarioBodyType::SPHScenarioBodyType_Polygon:
			{
				assert(body->vertexCount >= 3);
				Vec2f verts[kMaxScenarioPolygonCount];
				for (size_t vertexIndex = 0; vertexIndex < body->vertexCount; ++vertexIndex) {
					verts[vertexIndex] = Vec2MultMat2(body->orientation, body->localVerts[vertexIndex]) + body->position;
				}
				demo->AddPolygon(body->vertexCount, verts);
			} break;
		}
	}

	// Volumes
	const SPHParameters &params = demo->GetParams();
	const float spacing = params.particleSpacing;
	for (size_t volumeIndex = 0; volumeIndex < scenario->bodyCount; ++volumeIndex) {
		SPHScenarioVolume *volume = &scenario->volumes[volumeIndex];
		int numX = (int)floor((volume->size.w / spacing));
		int numY = (int)floor((volume->size.h / spacing));
		demo->AddVolume(volume->position, volume->force, numX, numY, spacing);
	}

	// Emitters
	for (size_t emitterIndex = 0; emitterIndex < scenario->emitterCount; ++emitterIndex) {
		SPHScenarioEmitter *emitter = &scenario->emitters[emitterIndex];
		demo->AddEmitter(emitter->position, emitter->direction, emitter->radius, emitter->speed, emitter->rate, emitter->duration);
	}
}

#endif