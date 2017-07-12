/*
-------------------------------------------------------------------------------------------------------------------
Multi-Threaded N-Body 2D Smoothed Particle Hydrodynamics Fluid Simulation based on paper "Particle-based Viscoelastic Fluid Simulation" by Simon Clavet, Philippe Beaudoin, and Pierre Poulin.

Version 1.0

A experiment about creating a two-way particle simulation in 4 different programming styles to see the difference in performance and maintainability.
The core math is same for all implementations, including rendering and threading.

Demos:

1. Object oriented style 1 (Naive)
2. Object oriented style 2 (Public, reserved vectors, fixed grid, no unneccesary classes or pointers)
3. Object oriented style 3 (Structs only, no virtual function calls, reserved vectors, fixed grid)
4. Data oriented style with 8/16 byte aligned structures

How to compile:

Compile main.cpp only and link with opengl, freeglut and glew.

Benchmark:

There is a benchmark recording and rendering built-in.

To start a benchmark hit "B" key.
To stop a benchmark hit "Escape" key.

Notes:

- Collision detection is discrete, therefore particles may pass through bodies when they are too thin and particles too fast.

Todo:

- External particle forces
- Add tick labels and value labels on benchmark chart
- Migrate all GUI/Text rendering to imGUI
- Migrate to modern opengl 3.3+

Version History:

1.0:
- Added integrated benchmark functionality

0.9:
- Initial version

License:

MIT License
Copyright (c) 2017 Torsten Spaete
-------------------------------------------------------------------------------------------------------------------
*/
#include <GL/glew.h>
#if _WIN32
// @NOTE(final): windef.h defines min/max macros defined in lowerspace, this will break std::min/max so we have to tell the header we dont want that macros!
#define NOMINMAX
#include <GL/wglew.h>
#endif
#include <GL/freeglut.h>
#include <chrono>

#include "app.cpp"

static Application *globalApp = nullptr;
static float lastFrameTime = 0.0f;
static uint64_t lastFrameCycles = 0;
static uint64_t lastCycles = 0;
static std::chrono::time_point<std::chrono::steady_clock> lastFrameClock;

static void ResizeFromGLUT(int width, int height) {
	globalApp->Resize(width, height);
}
static void DisplayFromGLUT() {
	globalApp->UpdateAndRender(lastFrameTime, lastFrameCycles);

	glutSwapBuffers();

	auto endFrameClock = std::chrono::high_resolution_clock::now();
	auto durationClock = endFrameClock - lastFrameClock;
	lastFrameClock = endFrameClock;
	uint64_t frameTimeInNanos = std::chrono::duration_cast<std::chrono::nanoseconds>(durationClock).count();
	lastFrameTime = frameTimeInNanos / (float)1000000000;

	uint64_t endCycles = __rdtsc();
	lastFrameCycles = endCycles - lastCycles;
	lastCycles = endCycles;
}
static void KeyPressedFromGLUT(unsigned char key, int a, int b) {
	globalApp->KeyUp(key);
}

static void SpecialKeyPressedFromGLUT(int key, int a, int b) {
	globalApp->KeyUp(key);
}

int main(int argc, char **args) {
	Application *app = globalApp = new DemoApplication();
	Window *window = app->GetWindow();

	glutInit(&argc, args);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(window->GetLeft(), window->GetTop());
	glutInitWindowSize(window->GetWidth(), window->GetHeight());

	std::string windowTitle = std::string("C++ NBody Simulation V") + std::string(kAppVersion);
	glutCreateWindow(windowTitle.c_str());

	glewInit();

#if _WIN32
	if (WGL_EXT_swap_control) {
		wglSwapIntervalEXT(0);
	}
#endif

	glutDisplayFunc(DisplayFromGLUT);
	glutReshapeFunc(ResizeFromGLUT);
	glutIdleFunc(DisplayFromGLUT);
	glutKeyboardUpFunc(KeyPressedFromGLUT);
	glutSpecialUpFunc(SpecialKeyPressedFromGLUT);

	lastFrameClock = std::chrono::high_resolution_clock::now();
	lastFrameTime = 0.0f;
	lastCycles = __rdtsc();

	glutMainLoop();

	delete app;

	return 0;
}