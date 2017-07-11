# nbodysimulation_experiment
Multi-Threaded N-Body 2D Smoothed Particle Hydrodynamics Fluid Simulation based on paper "Particle-based Viscoelastic Fluid Simulation" by Simon Clavet, Philippe Beaudoin, and Pierre Poulin

A experiment about creating a two-way particle simulation in 4 different programming styles to see the difference in performance and maintainability.
The core math is same for all implementations, including rendering and threading.

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

License:

MIT License
Copyright (c) 2017 Torsten Spaete