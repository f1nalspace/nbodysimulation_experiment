# nbodysimulation_experiment
Multi-Threaded N-Body 2D Smoothed Particle Hydrodynamics Fluid Simulation based on paper "Particle-based Viscoelastic Fluid Simulation" by Simon Clavet, Philippe Beaudoin, and Pierre Poulin.

Version 1.3

A experiment about creating a two-way particle simulation in 4 different programming styles to see the difference in performance and maintainability.
The core math is same for all implementations, including rendering and threading.

## Demos

1. Object oriented style 1 (Naive)
2. Object oriented style 2 (Public, reserved vectors, fixed grid, no unneccesary classes or pointers)
3. Object oriented style 3 (Structs only, no virtual function calls, reserved vectors, fixed grid)
4. Data oriented style with 8/16 byte aligned structures

## How to compile:

Compile main.cpp only and link with opengl, freeglut and glew.

## Benchmark:

There is a benchmark recording and rendering built-in.

To start a benchmark hit "B" key.
To stop a benchmark hit "Escape" key.

## Notes:

* Collision detection is discrete, therefore particles may pass through bodies when they are too thin and particles too fast.

## Todo:

* Replace glew with final_opengl.hpp
* Migrate all GUI/Text rendering to imGUI
* External particle forces
* Add bar value labels on benchmark chart
* Migrate to modern opengl 3.3+

## Version History:

### 1.3:
* Migrated to FPL 0.3.3 alpha

### 1.2:
* Using command buffer instead of immediate rendering, so we render only in main.cpp
* Rendering text using stb_truetype

### 1.1:
* Improved benchmark functionality and rendering

### 1.0:
* Added integrated benchmark functionality

### 0.9:
* Initial version

## License:

MIT License
Copyright (c) 2017 Torsten Spaete