/*
-------------------------------------------------------------------------------------------------------------------
Multi-Threaded N-Body 2D Smoothed Particle Hydrodynamics Fluid Simulation
Based on paper "Particle-based Viscoelastic Fluid Simulation" by Simon Clavet, Philippe Beaudoin, and Pierre Poulin

A experiment about creating a two-way particle simulation in 4 different programming styles to see the difference in performance and maintainability.
The core math is same for all implementations, including rendering and threading.

MIT License
Copyright (c) 2017 Torsten Spaete
-------------------------------------------------------------------------------------------------------------------

Demo 1-4

1.) Object oriented style 1 (Naive)
2.) Object oriented style 2 (Public, reserved vectors, fixed grid, no unneccesary classes or pointers)
3.) Object oriented style 3 (Structs only, no virtual function calls, reserved vectors, fixed grid)
4.) Data oriented style with 8/16 byte aligned structures

Usage:

Compile main.cpp only and link with opengl and freeglut. Thats it.

Code/Demo type can be changed from source only, just change the DEMO define below.
To create a benchmarking csv, just set the BENCHMARK define to 1.

Notes:

- Collision detection is discrete, therefore particles may pass through bodies when they are too thin and particles too fast.

Todo:

- Simple GUI using imgui
- Migrate to modern opengl 3.3+
- Introduce mass term in sph formulas, so we can use this for rigidbody interaction.
- Rigidbody physics and two-way fluid interaction: Block and impulse

*/
#define DEMO 1
#define BENCHMARK 0

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

int main(int argc, char **args) {
	Application *app = globalApp = new DemoApplication();
	Window *window = app->GetWindow();

	glutInit(&argc, args);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(window->GetLeft(), window->GetTop());
	glutInitWindowSize(window->GetWidth(), window->GetHeight());
	glutCreateWindow("C++ NBody Simulation");

	glewInit();
	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

#if _WIN32
	if (WGL_EXT_swap_control) {
		wglSwapIntervalEXT(0);
	}
#endif

	glutDisplayFunc(DisplayFromGLUT);
	glutReshapeFunc(ResizeFromGLUT);
	glutIdleFunc(DisplayFromGLUT);
	glutKeyboardUpFunc(KeyPressedFromGLUT);

	lastFrameClock = std::chrono::high_resolution_clock::now();
	lastFrameTime = 0.0f;
	lastCycles = __rdtsc();

	glutMainLoop();

	delete app;

	return 0;
}