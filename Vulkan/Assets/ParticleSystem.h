#pragma once
#include "Entity.h"
#include "../Graphics/Vertex.h"
#include "../Graphics/Material.h"
#include <stack>

namespace QZL {
	namespace Assets {
		// Particles are a special kind of entity made only of points, and expanded to billboarded textured quads in a shader.
		// Individual particles are grouped in to a ParticleSystem which defines some behaviour and tracks all of its particles lifetime.
		// To avoid having a model matrix for every individual particle, the system will have only one, and the particles' positions (points)
		// will be defined relative to that point. Each individual particle positions is just a vertex of the particle system, in a dynamic element buffer.

		// A 'Particle' only needs to know its index inside the system's vertex range, how long its been alive for, and its velocity.
		struct Particle {
			glm::vec3 velocity;
			float lifetime;

			Particle() {
				reset();
			}
			void reset() {
				velocity = glm::vec3(0.0f);
				lifetime = 0.0f;
			}
			void swapAndReset(Particle& other) {
				lifetime = other.lifetime;
				velocity = other.velocity;
				other.reset();
			}
		};
		class ParticleSystem : public Entity {
		public:
			void update(float dt) override;
		protected:
			// Number of tiles on xy is identical for x and y, as textures must be square.
			ParticleSystem(size_t maxParticles, float updateInterval, float textureTileLength, const std::string& textureName,
				glm::vec3* billboardPoint);
			virtual ~ParticleSystem();
			virtual void particleCreation(float dt, size_t expiredCount) = 0;
			virtual void updateParticle(Particle& particle, Graphics::ParticleVertex& vertex) = 0;

			// Tiles in the texture must be arranged left to right, top to bottom.
			void nextTextureTile(glm::vec2& currentOffset);

			/* 
				When true, system update is skipped, instead just updating each particle. This optimises when
				particles have infinite lifetime and do not overlap. This is false by default.
			*/
			bool alwaysAliveAndUnordered_;
		private:
			// Allocate particle returns nullptr if there is no available particle
			Particle* allocateParticle();
			void freeParticle(size_t idx);
			void updateActiveSize() {
				currentActiveSize_ = particles_.size() - numDeadParticles_;
			}

			Graphics::MaterialParticle material_;
			// The particles are not necessarily updated every frame, but after a specified time interval in seconds. An interval of 0 will
			// cause it to update once per frame.
			float updateInterval_;
			float elapsedUpdateTime_;
			glm::vec3* billboardPoint_;

			size_t currentActiveSize_;

			// All particles are drawn, make scale 0 if it is not to be visible
			//  : Store all particles in a list, indices correspond 1-1 with vertices
			//  : Store a list of dead particles (indices) (all those with scale = 0)
			//  : When a particle reaches the end of its lifetime, add a reference to the particle in dead particles
			//  : Pass the dead particle list to the particle creator to start them again
			//  : Store particles in a deque, all lifetimes must be the same, merely add to back and remove out the front
			size_t numDeadParticles_;
			std::vector<Particle> particles_;
			std::vector<Graphics::ParticleVertex> vertices_;

			// Each update, decrease the duration on each particle, moving dead one on to the queue and pushing them to
			//   the back of the particles & vertices vector. Run update func on each particle as they are iterated over.
			// Next run the create func (which also gives initial 'update').
			// Each update, sort particles & vertices based on distance to camera using insertion sort. 
			//   Total used range is particles.size() - dead.size().
			// Note that now lifetime can be random.
		};
	}
}
