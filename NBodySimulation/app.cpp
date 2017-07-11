#include "app.h"

#ifndef APP_IMPLEMENTATION
#define APP_IMPLEMENTATION

#include <GL/glew.h>
#include <filesystem>
#include <iostream>
#include <fstream>

#include "sph.h"

#if DEMO == 1
#include "demo1.cpp"
#elif DEMO == 2
#include "demo2.cpp"
#elif DEMO == 3
#include "demo3.cpp"
#elif DEMO == 4
#include "demo4.cpp"
#else
#error "Not supported demo!"
#endif

#define ArrayCount(arr) (sizeof(arr) / sizeof((arr)[0]))

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
	Application(std::string(kDemoName)),
	simulationFrameCount(0),
	simulationActive(true) {
	simulation = new ParticleSimulation();
	LoadScenario(activeScenarioIndex = 0);
#if STORE_FRAMESTATS
	frameStatistics.reserve(kMaxFrameStatisticsCount);
#endif
}

DemoApplication::~DemoApplication() {
	delete simulation;
}

struct OSDState {
	int charY;
};

static void DrawOSDLine(OSDState *osdState, const char *text) {
	const int charHeight = 12;
	glRasterPos2i(0, osdState->charY);
	glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char *)text);
	osdState->charY -= charHeight;
}

#if BENCHMARK
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

void DemoApplication::UpdateAndRender(const float frameTime, const uint64_t cycles) {
	if (simulationActive) {
		for (int step = 0; step < kSPHSubsteps; ++step) {
			simulation->Update(kSPHSubstepDeltaTime);
		}
		++simulationFrameCount;
#if BENCHMARK
		if (frameStatistics.size() < kMaxFrameStatisticsCount) {
			FrameStatistics newStats;
			newStats.frameTime = frameTime;
			newStats.sphStats = simulation->GetStats();
			frameStatistics.push_back(newStats);
		} else {
			SaveFrameStatistics();
			simulationActive = false;
		}
#endif
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

#if !BENCHMARK
	simulation->Render(worldToScreenScale);
#endif

	char osdBuffer[256];
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, 0, h, 0.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// OSD
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	OSDState osdState = {};
	osdState.charY = h - 12;

	size_t scenarioCount = ArrayCount(SPHScenarios);
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "%s - [%llu / %llu] %s (Space)", title.c_str(), (activeScenarioIndex + 1), scenarioCount, activeScenarioName.c_str());
	DrawOSDLine(&osdState, osdBuffer);
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Simulation: %s, frames: %llu (P)", (simulationActive ? "yes" : "no"), simulationFrameCount);
	DrawOSDLine(&osdState, osdBuffer);
#if !BENCHMARK
	if (simulation->IsMultiThreadingSupported()) {
		sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Multithreading: %s, %llu threads (T)", (simulation->IsMultiThreading() ? "yes" : "no"), simulation->GetWorkerThreadCount());
	} else {
		sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Multithreading: not supported");
	}
	DrawOSDLine(&osdState, osdBuffer);
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Reset (R)");
	DrawOSDLine(&osdState, osdBuffer);
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Frame time: %f ms, Cycles: %llu", (frameTime * 1000.0f), cycles);
	DrawOSDLine(&osdState, osdBuffer);
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Particles: %llu", simulation->GetParticleCount());
	DrawOSDLine(&osdState, osdBuffer);

	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Stats:");
	DrawOSDLine(&osdState, osdBuffer);
	const SPHStatistics &stats = simulation->GetStats();
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Min/Max cell particle count: %llu / %llu", stats.minCellParticleCount, stats.maxCellParticleCount);
	DrawOSDLine(&osdState, osdBuffer);
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Min/Max particle neighbor count: %llu / %llu", stats.minParticleNeighborCount, stats.maxParticleNeighborCount);
	DrawOSDLine(&osdState, osdBuffer);
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Time integration: %f ms", stats.time.integration);
	DrawOSDLine(&osdState, osdBuffer);
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Time viscosity forces: %f ms", stats.time.viscosityForces);
	DrawOSDLine(&osdState, osdBuffer);
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Time predict: %f ms", stats.time.predict);
	DrawOSDLine(&osdState, osdBuffer);
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Time update grid: %f ms", stats.time.updateGrid);
	DrawOSDLine(&osdState, osdBuffer);
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Time neighbor search: %f ms", stats.time.neighborSearch);
	DrawOSDLine(&osdState, osdBuffer);
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Time density and pressure: %f ms", stats.time.densityAndPressure);
	DrawOSDLine(&osdState, osdBuffer);
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Time delta positions: %f ms", stats.time.deltaPositions);
	DrawOSDLine(&osdState, osdBuffer);
	sprintf_s(osdBuffer, ArrayCount(osdBuffer), "Time collisions: %f ms", stats.time.collisions);
	DrawOSDLine(&osdState, osdBuffer);
#endif
}

void DemoApplication::KeyUp(unsigned char key) {
	if (key == ' ') {
		activeScenarioIndex = (activeScenarioIndex + 1) % ArrayCount(SPHScenarios);
		LoadScenario(activeScenarioIndex);
	} else if (key == 'p') {
		simulationActive = !simulationActive;
	} else if (key == 'r') {
		simulationFrameCount = 0;
#if STORE_FRAMESTATS
		frameStatistics.clear();
#endif
		LoadScenario(activeScenarioIndex);
	} else if (key == 't' && simulation->IsMultiThreadingSupported()) {
		simulation->ToggleMultiThreading();
	}
}

void DemoApplication::LoadScenario(size_t scenarioIndex) {
	SPHScenario *scenario = &SPHScenarios[scenarioIndex];
	activeScenarioName = scenario->name;
	simulation->ResetStats();
	simulation->ClearBodies();
	simulation->ClearParticles();
	simulation->ClearEmitters();
	simulation->SetGravity(scenario->gravity);
	simulation->SetParams(scenario->parameters);

	// Bodies
	for (size_t bodyIndex = 0; bodyIndex < scenario->bodyCount; ++bodyIndex) {
		SPHScenarioBody *body = &scenario->bodies[bodyIndex];
		switch (body->type) {
			case SPHScenarioBodyType::SPHScenarioBodyType_Plane:
			{
				float distance = Vec2Dot(body->orientation.col1, body->position);
				simulation->AddPlane(body->orientation.col1, distance);
			} break;
			case SPHScenarioBodyType::SPHScenarioBodyType_Circle:
			{
				simulation->AddCircle(body->position, body->radius);
			} break;
			case SPHScenarioBodyType::SPHScenarioBodyType_LineSegment:
			{
				assert(body->vertexCount == 2);
				Vec2f a = Vec2MultMat2(body->orientation, body->localVerts[0]) + body->position;
				Vec2f b = Vec2MultMat2(body->orientation, body->localVerts[1]) + body->position;
				simulation->AddLineSegment(a, b);
			} break;
			case SPHScenarioBodyType::SPHScenarioBodyType_Polygon:
			{
				assert(body->vertexCount >= 3);
				Vec2f verts[kMaxScenarioPolygonCount];
				for (size_t vertexIndex = 0; vertexIndex < body->vertexCount; ++vertexIndex) {
					verts[vertexIndex] = Vec2MultMat2(body->orientation, body->localVerts[vertexIndex]) + body->position;
				}
				simulation->AddPolygon(body->vertexCount, verts);
			} break;
		}
	}

	// Volumes
	const SPHParameters &params = simulation->GetParams();
	const float spacing = params.particleSpacing;
	for (size_t volumeIndex = 0; volumeIndex < scenario->bodyCount; ++volumeIndex) {
		SPHScenarioVolume *volume = &scenario->volumes[volumeIndex];
		int numX = (int)floor((volume->size.w / spacing));
		int numY = (int)floor((volume->size.h / spacing));
		simulation->AddVolume(volume->position, volume->force, numX, numY, spacing);
	}

	// Emitters
	for (size_t emitterIndex = 0; emitterIndex < scenario->emitterCount; ++emitterIndex) {
		SPHScenarioEmitter *emitter = &scenario->emitters[emitterIndex];
		simulation->AddEmitter(emitter->position, emitter->direction, emitter->radius, emitter->speed, emitter->rate, emitter->duration);
	}
}

#endif