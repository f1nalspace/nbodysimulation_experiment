#include "demo3.h"

#ifndef DEMO3_IMPLEMENTATION
#define DEMO3_IMPLEMENTATION

#include <GL/glew.h>

#include <chrono>
#include <algorithm>
#include <cstddef>

#include "render.h"

ParticleSimulation::ParticleSimulation() :
	gravity(Vec2f(0, 0)) {
	particles.reserve(kSPHMaxParticleCount);
	cells = new Cell[kSPHGridTotalCount];
	_isMultiThreading = workerPool.GetThreadCount() > 1;
}

ParticleSimulation::~ParticleSimulation() {
	for (size_t bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex) {
		delete bodies[bodyIndex];
	}
	delete cells;
}

void ParticleSimulation::InsertParticleIntoGrid(Particle &particle, const size_t particleIndex) {
	Vec2f position = particle.curPosition;
	Vec2i cellIndex = SPHComputeCellIndex(position);

	size_t cellOffset = SPHComputeCellOffset(cellIndex.x, cellIndex.y);
	Cell *cell = &cells[cellOffset];
	assert(cell != nullptr);

	cell->indices.push_back(particleIndex);
	particle.cellIndex = cellIndex;

	size_t count = cell->indices.size();
	stats.minCellParticleCount = std::min(count, stats.minCellParticleCount);
	stats.maxCellParticleCount = std::max(count, stats.maxCellParticleCount);
}

void ParticleSimulation::RemoveParticleFromGrid(Particle & particle, const size_t particleIndex) {
	Vec2i cellIndex = particle.cellIndex;
	size_t cellOffset = SPHComputeCellOffset(cellIndex.x, cellIndex.y);

	Cell *cell = &cells[cellOffset];
	assert(cell != nullptr);

	auto result = std::find(cell->indices.begin(), cell->indices.end(), particleIndex);
	assert(result != cell->indices.end());
	cell->indices.erase(result);

	size_t count = cell->indices.size();
	stats.minCellParticleCount = std::min(count, stats.minCellParticleCount);
	stats.maxCellParticleCount = std::max(count, stats.maxCellParticleCount);
}

void ParticleSimulation::AddPlane(const Vec2f & normal, const float distance) {
	Plane *plane = new Plane(normal, distance);
	bodies.push_back(plane);
}

void ParticleSimulation::AddCircle(const Vec2f & pos, const float radius) {
	bodies.push_back(new Circle(pos, radius));
}

void ParticleSimulation::AddLineSegment(const Vec2f & a, const Vec2f & b) {
	bodies.push_back(new LineSegment(a, b));
}

void ParticleSimulation::AddPolygon(const size_t vertexCount, const Vec2f *verts) {
	std::vector<Vec2f> polyVerts;
	for (size_t i = 0; i < vertexCount; ++i) {
		polyVerts.push_back(verts[i]);
	}
	bodies.push_back(new Poly(polyVerts));
}

void ParticleSimulation::ClearBodies() {
	for (int bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex) {
		Body *body = bodies[bodyIndex];
		delete body;
	}
	bodies.clear();
}

void ParticleSimulation::ClearParticles() {
	for (size_t cellIndex = 0; cellIndex < kSPHGridTotalCount; ++cellIndex) {
		Cell *cell = &cells[cellIndex];
		assert(cell != nullptr);
		cell->indices.clear();
	}
	particles.clear();
}

void ParticleSimulation::ClearEmitters() {
	emitters.clear();
}

void ParticleSimulation::ResetStats() {
	stats = {};
}

size_t ParticleSimulation::AddParticle(const Vec2f &position, const Vec2f &force) {
	size_t particleIndex = particles.size();
	particles.push_back(Particle(position));
	Particle &particle = particles[particleIndex];
	particle.acceleration = force;
	InsertParticleIntoGrid(particle, particleIndex);
	return particleIndex;
}

void ParticleSimulation::AddVolume(const Vec2f &center, const Vec2f &force, const int countX, const int countY, const float spacing) {
	Vec2f offset = Vec2f(countX * spacing, countY * spacing) * 0.5f;
	for (int yIndex = 0; yIndex < countY; ++yIndex) {
		for (int xIndex = 0; xIndex < countX; ++xIndex) {
			Vec2f p = Vec2f((float)xIndex, (float)yIndex) * spacing;
			p += Vec2f(spacing * 0.5f);
			p += center - offset;
			Vec2f jitter = Vec2RandomDirection() * kSPHKernelHeight * kSPHVolumeParticleDistributionScale;
			p += jitter;
			AddParticle(p, force);
		}
	}
}

void ParticleSimulation::AddEmitter(const Vec2f &position, const Vec2f &direction, const float radius, const float speed, const float rate, const float duration) {
	ParticleEmitter emitter = ParticleEmitter(position, direction, radius, speed, rate, duration);
	emitters.push_back(emitter);
}

void ParticleSimulation::NeighborSearch(const size_t startIndex, const size_t endIndex, const float deltaTime) {
	for (size_t particleIndexA = startIndex; particleIndexA <= endIndex; ++particleIndexA) {
		Particle &particleA = particles[particleIndexA];
		particleA.neighbors.clear();
		Vec2i &cellIndex = particleA.cellIndex;
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) {
				int cellPosX = cellIndex.x + x;
				int cellPosY = cellIndex.y + y;
				if (SPHIsPositionInGrid(cellPosX, cellPosY)) {
					size_t cellOffset = SPHComputeCellOffset(cellPosX, cellPosY);
					Cell *cell = &cells[cellOffset];
					size_t particleCountInCell = cell->indices.size();
					if (particleCountInCell > 0) {
						for (size_t index = 0; index < particleCountInCell; ++index) {
							size_t particleIndexB = cell->indices[index];
							particleA.neighbors.push_back(particleIndexB);
						}
					}
				}
			}
		}
	}
}

void ParticleSimulation::DensityAndPressure(const size_t startIndex, const size_t endIndex, const float deltaTime) {
	for (size_t particleIndex = startIndex; particleIndex <= endIndex; ++particleIndex) {
		Particle &particle = particles[particleIndex];
		particle.density = particle.nearDensity = 0;
		size_t neighborCount = particle.neighbors.size();
		for (size_t index = 0; index < neighborCount; ++index) {
			size_t neighborIndex = particle.neighbors[index];
			Particle &neighbor = particles[neighborIndex];
			SPHComputeDensity(params, particle.curPosition, neighbor.curPosition, &particle.density);
		}
		SPHComputePressure(params, &particle.density, &particle.pressure);
	}
}

void ParticleSimulation::ViscosityForces(const size_t startIndex, const size_t endIndex, const float deltaTime) {
	for (size_t particleIndex = startIndex; particleIndex <= endIndex; ++particleIndex) {
		Particle &particle = particles[particleIndex];
		size_t neighborCount = particle.neighbors.size();
		for (size_t index = 0; index < neighborCount; ++index) {
			size_t neighborIndex = particle.neighbors[index];
			Particle &neighbor = particles[neighborIndex];
			Vec2f force = Vec2f();
			SPHComputeViscosityForce(params, particle.curPosition, neighbor.curPosition, particle.velocity, neighbor.velocity, &force);
			particle.velocity -= force * deltaTime * 0.5f;
			neighbor.velocity += force * deltaTime * 0.5f;
		}
	}
}

void ParticleSimulation::DeltaPositions(const size_t startIndex, const size_t endIndex, const float deltaTime) {
	for (size_t particleIndex = startIndex; particleIndex <= endIndex; ++particleIndex) {
		Particle &particle = particles[particleIndex];
		Vec2f accumulatedDelta = Vec2f();
		size_t neighborCount = particle.neighbors.size();
		for (size_t index = 0; index < neighborCount; ++index) {
			size_t neighborIndex = particle.neighbors[index];
			Particle &neighbor = particles[neighborIndex];
			Vec2f delta = Vec2f();
			SPHComputeDelta(params, particle.curPosition, neighbor.curPosition, &particle.pressure, deltaTime, &delta);
			neighbor.curPosition += delta * 0.5f;
			accumulatedDelta -= delta * 0.5f;
		}
		particle.curPosition += accumulatedDelta;
	}
}

void ParticleSimulation::UpdateEmitter(ParticleEmitter *emitter, const float deltaTime) {
	const float spacing = params.particleSpacing;
	const float invDeltaTime = 1.0f / deltaTime;
	if (emitter->isActive) {
		const float rate = 1.0f / emitter->rate;
		emitter->elapsed += deltaTime;
		emitter->totalElapsed += deltaTime;
		if (emitter->elapsed >= rate) {
			emitter->elapsed = 0;
			Vec2f acceleration = emitter->direction * emitter->speed * invDeltaTime;
			Vec2f dir = Vec2Cross(1.0f, emitter->direction);
			int count = (int)floor(emitter->radius / spacing);
			float halfSize = (float)count * spacing * 0.5f;
			Vec2f offset = dir * (float)count * spacing * 0.5f;
			for (int index = 0; index < count; ++index) {
				Vec2f p = dir * (float)index * spacing;
				p += dir * spacing * 0.5f;
				p += emitter->position - offset;
				Vec2f jitter = Vec2RandomDirection() * kSPHKernelHeight * kSPHVolumeParticleDistributionScale;
				p += jitter;
				AddParticle(p, acceleration);
			}
		}
		if (emitter->totalElapsed >= emitter->duration) {
			emitter->isActive = false;
		}
	}
}

void ParticleSimulation::Update(const float deltaTime) {
	const float invDt = 1.0f / deltaTime;
	const float nanosToMilliseconds = 1.0f / 1000000;
	const bool useMultiThreading = _isMultiThreading;

	// Emitters
	{
		auto startClock = std::chrono::high_resolution_clock::now();
		for (size_t emitterIndex = 0; emitterIndex < emitters.size(); ++emitterIndex) {
			ParticleEmitter *emitter = &emitters[emitterIndex];
			UpdateEmitter(emitter, deltaTime);
		}
		auto deltaClock = std::chrono::high_resolution_clock::now() - startClock;
		stats.time.emitters = std::chrono::duration_cast<std::chrono::nanoseconds>(deltaClock).count() * nanosToMilliseconds;
	}

	// Integrate forces
	{
		auto startClock = std::chrono::high_resolution_clock::now();
		for (size_t particleIndex = 0; particleIndex < particles.size(); ++particleIndex) {
			Particle &particle = particles[particleIndex];
			particle.acceleration += gravity;
			particle.velocity += particle.acceleration * deltaTime;
			particle.acceleration = Vec2f();
		}
		auto deltaClock = std::chrono::high_resolution_clock::now() - startClock;
		stats.time.integration = std::chrono::duration_cast<std::chrono::nanoseconds>(deltaClock).count() * nanosToMilliseconds;
	}

	// Viscosity force
	{
		auto startClock = std::chrono::high_resolution_clock::now();
		if (useMultiThreading) {
			workerPool.CreateTasks(particles.size(), [=](const size_t startIndex, const size_t endIndex, const float deltaTime) {
				this->ViscosityForces(startIndex, endIndex, deltaTime);
			}, deltaTime);
			workerPool.WaitUntilDone();
		} else {
			this->ViscosityForces(0, particles.size() - 1, deltaTime);
		}
		auto deltaClock = std::chrono::high_resolution_clock::now() - startClock;
		stats.time.viscosityForces = std::chrono::duration_cast<std::chrono::nanoseconds>(deltaClock).count() * nanosToMilliseconds;
	}

	// Predict
	{
		auto startClock = std::chrono::high_resolution_clock::now();
		for (size_t particleIndex = 0; particleIndex < particles.size(); ++particleIndex) {
			Particle &particle = particles[particleIndex];
			particle.prevPosition = particle.curPosition;
			particle.curPosition += particle.velocity * deltaTime;
		}
		auto deltaClock = std::chrono::high_resolution_clock::now() - startClock;
		stats.time.predict = std::chrono::duration_cast<std::chrono::nanoseconds>(deltaClock).count() * nanosToMilliseconds;
	}

	// Update grid
	{
		auto startClock = std::chrono::high_resolution_clock::now();
		for (size_t particleIndex = 0; particleIndex < particles.size(); ++particleIndex) {
			Particle &particle = particles[particleIndex];
			Vec2i newCellIndex = SPHComputeCellIndex(particle.curPosition);
			Vec2i oldCellIndex = particle.cellIndex;
			if (newCellIndex.x != oldCellIndex.x || newCellIndex.y != oldCellIndex.y) {
				RemoveParticleFromGrid(particle, particleIndex);
				InsertParticleIntoGrid(particle, particleIndex);
			}
		}
		auto deltaClock = std::chrono::high_resolution_clock::now() - startClock;
		stats.time.updateGrid = std::chrono::duration_cast<std::chrono::nanoseconds>(deltaClock).count() * nanosToMilliseconds;
	}

	// Neighbor search
	{
		auto startClock = std::chrono::high_resolution_clock::now();
		if (useMultiThreading) {
			workerPool.CreateTasks(particles.size(), [=](const size_t startIndex, const size_t endIndex, const float deltaTime) {
				this->NeighborSearch(startIndex, endIndex, deltaTime);
			}, deltaTime);
			workerPool.WaitUntilDone();
		} else {
			this->NeighborSearch(0, particles.size() - 1, deltaTime);
		}
		stats.minParticleNeighborCount = kSPHMaxParticleNeighborCount;
		stats.maxParticleNeighborCount = 0;
		for (size_t particleIndex = 0; particleIndex < particles.size(); ++particleIndex) {
			Particle &particle = particles[particleIndex];
			size_t neighborCount = particle.neighbors.size();
			stats.minParticleNeighborCount = std::min(neighborCount, stats.minParticleNeighborCount);
			stats.maxParticleNeighborCount = std::max(neighborCount, stats.maxParticleNeighborCount);
		}
		auto deltaClock = std::chrono::high_resolution_clock::now() - startClock;
		stats.time.neighborSearch = std::chrono::duration_cast<std::chrono::nanoseconds>(deltaClock).count() * nanosToMilliseconds;
	}

	// Density and pressure
	{
		auto startClock = std::chrono::high_resolution_clock::now();
		if (useMultiThreading) {
			workerPool.CreateTasks(particles.size(), [=](const size_t startIndex, const size_t endIndex, const float deltaTime) {
				this->DensityAndPressure(startIndex, endIndex, deltaTime);
			}, deltaTime);
			workerPool.WaitUntilDone();
		} else {
			this->DensityAndPressure(0, particles.size(), deltaTime);
		}
		auto deltaClock = std::chrono::high_resolution_clock::now() - startClock;
		stats.time.densityAndPressure = std::chrono::duration_cast<std::chrono::nanoseconds>(deltaClock).count() * nanosToMilliseconds;
	}

	// Calculate delta position
	{
		auto startClock = std::chrono::high_resolution_clock::now();
		if (useMultiThreading) {
			workerPool.CreateTasks(particles.size(), [=](const size_t startIndex, const size_t endIndex, const float deltaTime) {
				this->DeltaPositions(startIndex, endIndex, deltaTime);
			}, deltaTime);
			workerPool.WaitUntilDone();
		} else {
			this->DeltaPositions(0, particles.size() - 1, deltaTime);
		}
		auto deltaClock = std::chrono::high_resolution_clock::now() - startClock;
		stats.time.deltaPositions = std::chrono::duration_cast<std::chrono::nanoseconds>(deltaClock).count() * nanosToMilliseconds;
	}

	// Solve collisions
	{
		auto startClock = std::chrono::high_resolution_clock::now();
		for (size_t particleIndex = 0; particleIndex < particles.size(); ++particleIndex) {
			Particle &particle = particles[particleIndex];
			for (size_t bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex) {
				Body *body = bodies[bodyIndex];
				switch (body->type) {
					case BodyType::BodyType_Plane:
					{
						Plane *plane = static_cast<Plane *>(body);
						SPHSolvePlaneCollision(&particle.curPosition, plane->normal, plane->distance);
					} break;
					case BodyType::BodyType_Circle:
					{
						Circle *circle = static_cast<Circle *>(body);
						SPHSolveCircleCollision(&particle.curPosition, circle->pos, circle->radius);
					} break;
					case BodyType::BodyType_LineSegment:
					{
						LineSegment *lineSegment = static_cast<LineSegment *>(body);
						SPHSolveLineSegmentCollision(&particle.curPosition, lineSegment->a, lineSegment->b);
					} break;
					case BodyType::BodyType_Polygon:
					{
						Poly *polygon = static_cast<Poly *>(body);
						SPHSolvePolygonCollision(&particle.curPosition, polygon->verts.size(), &polygon->verts[0]);
					} break;
				}
			}
		}
		auto deltaClock = std::chrono::high_resolution_clock::now() - startClock;
		stats.time.collisions = std::chrono::duration_cast<std::chrono::nanoseconds>(deltaClock).count() * nanosToMilliseconds;
	}

	// Recalculate velocity for next frame
	for (size_t particleIndex = 0; particleIndex < particles.size(); ++particleIndex) {
		Particle &particle = particles[particleIndex];
		particle.velocity = (particle.curPosition - particle.prevPosition) * invDt;
	}
}

void ParticleSimulation::Render(const float worldToScreenScale) {
	// Domain
	glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(kSPHBoundaryHalfWidth, kSPHBoundaryHalfHeight);
	glVertex2f(-kSPHBoundaryHalfWidth, kSPHBoundaryHalfHeight);
	glVertex2f(-kSPHBoundaryHalfWidth, -kSPHBoundaryHalfHeight);
	glVertex2f(kSPHBoundaryHalfWidth, -kSPHBoundaryHalfHeight);
	glEnd();

	// Grid fill
	for (int yIndexInner = 0; yIndexInner < kSPHGridCountY; ++yIndexInner) {
		for (int xIndexInner = 0; xIndexInner < kSPHGridCountX; ++xIndexInner) {
			size_t cellOffset = SPHComputeCellOffset(xIndexInner, yIndexInner);
			Cell *cell = &cells[cellOffset];
			Vec2f innerP = kSPHGridOrigin + Vec2f((float)xIndexInner, (float)yIndexInner) * kSPHGridCellSize;
			Vec2f innerSize = Vec2f(kSPHGridCellSize);
			if (cell->indices.size() > 0) {
				FillRectangle(innerP, innerSize, ColorLightGray);
			}
		}
	}

	// Grid lines
	for (int yIndex = 0; yIndex < kSPHGridCountY; ++yIndex) {
		Vec2f startP = kSPHGridOrigin + Vec2f(0, (float)yIndex) * kSPHGridCellSize;
		Vec2f endP = kSPHGridOrigin + Vec2f((float)kSPHGridCountX, (float)yIndex) * kSPHGridCellSize;
		DrawLine(startP, endP, ColorDarkGray);
	}
	for (int xIndex = 0; xIndex < kSPHGridCountX; ++xIndex) {
		Vec2f startP = kSPHGridOrigin + Vec2f((float)xIndex, 0) * kSPHGridCellSize;
		Vec2f endP = kSPHGridOrigin + Vec2f((float)xIndex, (float)kSPHGridCountY) * kSPHGridCellSize;
		DrawLine(startP, endP, ColorDarkGray);
	}

	// Bodies
	for (int bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex) {
		Body *body = bodies[bodyIndex];
		switch (body->type) {
			case BodyType::BodyType_Plane:
			{
				Plane *plane = static_cast<Plane *>(body);
				plane->Render();
			}
			break;
			case BodyType::BodyType_Circle:
			{
				Circle *circle = static_cast<Circle *>(body);
				circle->Render();
			}
			break;
			case BodyType::BodyType_LineSegment:
			{
				LineSegment *lineSegment = static_cast<LineSegment *>(body);
				lineSegment->Render();
			}
			break;
			case BodyType::BodyType_Polygon:
			{
				Poly *polygon = static_cast<Poly *>(body);
				polygon->Render();
			}
			break;
		}
	}

	// Particles
	if (particles.size() > 0) {
		for (int particleIndex = 0; particleIndex < particles.size(); ++particleIndex) {
			Particle *particle = &particles[particleIndex];
			particle->color = SPHGetParticleColor(params.restDensity, particle->density, particle->pressure, particle->velocity);
		}
		float pointSize = kSPHParticleRenderRadius * 2.0f * worldToScreenScale;
		glPointSize(pointSize);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, sizeof(Particle), (void *)((uint8_t *)&particles[0] + offsetof(Particle, curPosition)));
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, sizeof(Particle), (void *)((uint8_t *)&particles[0] + offsetof(Particle, color)));
		glDrawArrays(GL_POINTS, 0, (int)particles.size());
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		glPointSize(1);
	}
}

Particle::Particle(const Vec2f & position) :
	prevPosition(position),
	curPosition(position) {
	neighbors.reserve(kSPHMaxParticleNeighborCount);
}

Cell::Cell() {
	indices.reserve(kSPHMaxCellParticleCount);
}

void Plane::Render() {
	Vec2f p = normal * distance;
	Vec2f t = Vec2f(normal.y, -normal.x);
	Vec4f color = ColorBlue;
	glColor4fv(&color.m[0]);
	glBegin(GL_LINES);
	glVertex2f(p.x + t.x * kSPHVisualPlaneLength, p.y + t.y * kSPHVisualPlaneLength);
	glVertex2f(p.x - t.x * kSPHVisualPlaneLength, p.y - t.y * kSPHVisualPlaneLength);
	glEnd();
}

void Circle::Render() {
	Vec4f color = ColorBlue;
	DrawCircle(pos, radius, color);
}

void LineSegment::Render() {
	Vec4f color = ColorBlue;
	glColor4fv(&color.m[0]);
	glBegin(GL_LINES);
	glVertex2f(a.x, a.y);
	glVertex2f(b.x, b.y);
	glEnd();
}

void Poly::Render() {
	Vec4f color = ColorBlue;
	glColor4fv(&color.m[0]);
	glBegin(GL_LINE_LOOP);
	for (size_t vertexIndex = 0; vertexIndex < verts.size(); ++vertexIndex) {
		glVertex2f(verts[vertexIndex].x, verts[vertexIndex].y);
	}
	glEnd();
}

#endif // DEMO3_IMPLEMENTATION