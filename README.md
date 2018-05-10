# nbodysimulation_experiment
Multi-Threaded N-Body 2D Smoothed Particle Hydrodynamics Fluid Simulation based on paper "Particle-based Viscoelastic Fluid Simulation" by Simon Clavet, Philippe Beaudoin, and Pierre Poulin.

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

## License:

MIT License
Copyright (c) 2017 Torsten Spaete