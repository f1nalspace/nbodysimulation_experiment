/* Demo 1 - Object oriented style 1 (Naive) */

#ifndef DEMO1_H
#define DEMO1_H

#include <assert.h>
#include <vector>
#include <random>

#include "vecmath.h"
#include "sph.h"
#include "threadpool.h"
#include "base.h"

namespace Demo1 {
	const char *kDemoName = "Demo 1";

	class Grid;

	class Particle {
	private:
		Vec2f _acceleration;
		Vec2f _velocity;
		Vec2f _prevPosition;
		Vec2f _curPosition;
		Vec2i _cellIndex;
		float _density;
		float _nearDensity;
		float _pressure;
		float _nearPressure;
		std::vector<Particle *> _neighbors;
	public:
		Particle(const Vec2f &position);

		void IntegrateForces(const float deltaTime);
		void Predict(const float deltaTime);
		void UpdateVelocity(const float invDeltaTime);
		void UpdateNeighbors(Grid *grid);
		void ComputeDensityAndPressure(const SPHParameters &params, SPHStatistics &stats);
		void ComputeDeltaPosition(const SPHParameters &params, const float deltaTime, SPHStatistics &stats);
		void ComputeViscosityForces(const SPHParameters &params, const float deltaTime, SPHStatistics &stats);
		void ClearDensity();

		void SetVelocity(const Vec2f &v);
		const Vec2f &GetVelocity();

		const Vec2f &GetPosition();
		void SetPosition(const Vec2f &position);

		const Vec2f &GetPrevPosition();
		void SetPrevPosition(const Vec2f &prevPosition);

		void SetAcceleration(const Vec2f &acceleration);
		const Vec2f &GetAcceleration();

		const Vec2i &GetCellIndex();
		void SetCellIndex(const Vec2i &cellIndex);

		Particle *GetNeighbor(size_t index);
		size_t GetNeighborCount();

		float GetDensity();
		float GetNearDensity();

		void SetDensity(const float density);
		void SetNearDensity(const float nearDensity);

		void SetPressure(const float pressure);
		float GetPressure();

		void SetNearPressure(const float nearPressure);
		float GetNearPressure();

	};

	class Cell {
	private:
		std::vector<Particle *> _particles;
	public:
		Cell();

		void Add(Particle *particle);
		void Remove(Particle *particle);
		void Clear();

		Particle *GetParticle(size_t index);
		size_t GetCount();
	};

	class Grid {
	private:
		std::vector<Cell *> _cells;
	public:
		Grid(const size_t maxCellCount);
		~Grid();

		Cell *GetCell(const size_t index);
		void Clear();
		void InsertParticleIntoGrid(Particle *particle, SPHStatistics &stats);
		void RemoveParticleFromGrid(Particle *particle, SPHStatistics &stats);
		Cell *EnforceCell(const size_t index);
	};

	enum class BodyType {
		Plane,
		Circle,
		LineSegment,
		Polygon,
	};

	class Body {
	private:
		BodyType _type;
	public:
		Body(const BodyType type);
		virtual ~Body();
		BodyType GetType();
		virtual void SolveCollision(Particle *particle);
		virtual void Render();
	};

	class Plane : public Body {
	private:
		Vec2f _normal;
		float _distance;
	public:
		Plane(const Vec2f &normal, const float distance);
		const Vec2f &GetNormal();
		float GetDistance();
		void SolveCollision(Particle *particle);
		void Render();
	};

	class Circle : public Body {
	private:
		Vec2f _pos;
		float _radius;
	public:
		Circle(const Vec2f &pos, const float radius);
		const Vec2f &GetPosition();
		float GetRadius();
		void SolveCollision(Particle *particle);
		void Render();
	};

	class LineSegment : public Body {
	private:
		Vec2f _a, _b;
	public:
		LineSegment(const Vec2f &a, const Vec2f &b);
		const Vec2f &GetA();
		const Vec2f &GetB();
		void SolveCollision(Particle *particle);
		void Render();
	};

	class Poly : public Body {
	private:
		std::vector<Vec2f> _verts;
	public:
		Poly(const std::vector<Vec2f> &verts);
		const size_t GetVertexCount();
		const Vec2f &GetVertex(size_t index);
		void SolveCollision(Particle *particle);
		void Render();
	};

	class ParticleEmitter {
	private:
		Vec2f _position;
		Vec2f _direction;
		float _radius;
		float _speed;
		float _rate;
		float _duration;
		float _elapsed;
		float _totalElapsed;
		bool _isActive;
	public:
		ParticleEmitter(const Vec2f &position, const Vec2f &direction, const float radius, const float speed, const float rate, const float duration);
		const Vec2f &GetPosition();
		const Vec2f &GetDirection();
		float GetRadius();
		float GetSpeed();
		float GetRate();
		float GetDuration();
		float GetElapsed();
		void SetElapsed(const float elapsed);
		float GetTotalElapsed();
		void SetTotalElapsed(const float totalElapsed);
		bool GetIsActive();
		void SetIsActive(const bool isActive);
		void Render();
	};

	struct ParticleRenderObject {
		Vec2f pos;
		Vec4f color;
	};

	class ParticleSimulation : public BaseSimulation {
	private:
		SPHParameters _params;
		SPHStatistics _stats;

		Vec2f _gravity;

		std::vector<Particle *> _particles;
		std::vector<ParticleRenderObject> _particleRenderObjects;

		std::vector<Body *> _bodies;

		std::vector<ParticleEmitter *> _emitters;

		Grid *_grid;

		bool _isMultiThreading;
		ThreadPool *_workerPool;
	private:
		void UpdateEmitter(ParticleEmitter *emitter, const float deltaTime);
		void ViscosityForces(const size_t startIndex, const size_t endIndex, const float deltaTime);
		void NeighborSearch(const size_t startIndex, const size_t endIndex, const float deltaTime);
		void DensityAndPressure(const size_t startIndex, const size_t endIndex, const float deltaTime);
		void DeltaPositions(const size_t startIndex, const size_t endIndex, const float deltaTime);
	public:
		ParticleSimulation();
		~ParticleSimulation();

		void ResetStats();
		void ClearBodies();
		void ClearParticles();
		void ClearEmitters();

		void AddPlane(const Vec2f &normal, const float distance);
		void AddCircle(const Vec2f &pos, const float radius);
		void AddLineSegment(const Vec2f &a, const Vec2f &b);
		void AddPolygon(const size_t vertexCount, const Vec2f *verts);

		size_t AddParticle(const Vec2f &position, const Vec2f &force);
		void AddVolume(const Vec2f &center, const Vec2f &force, const int countX, const int countY, const float spacing);
		void AddEmitter(const Vec2f &position, const Vec2f &direction, const float radius, const float speed, const float rate, const float duration);

		void Update(const float deltaTime);
		void Render(const float worldToScreenScale);

		inline size_t GetParticleCount() {
			return _particles.size();
		}
		inline void SetGravity(const Vec2f &gravity) {
			_gravity = gravity;
		}
		inline const SPHParameters &GetParams() {
			return _params;
		}
		inline const SPHStatistics &GetStats() {
			return _stats;
		}
		inline void SetParams(const SPHParameters &params) {
			_params = params;
		}
		inline void ToggleMultiThreading() {
			_isMultiThreading = !_isMultiThreading;
		}
		inline bool IsMultiThreadingSupported() {
			return true;
		}
		inline bool IsMultiThreading() {
			return _isMultiThreading;
		}
		inline size_t GetWorkerThreadCount() {
			return _workerPool->GetThreadCount();
		}
	};
};

#endif // DEMO1_H