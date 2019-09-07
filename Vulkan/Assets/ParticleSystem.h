#pragma once
#include "Entity.h"
#include <stack>

namespace QZL {
	namespace Assets {
		// Particles are a special kind of entity made only of points, and expanded to billboarded textured quads in a shader.
		// Individual particles are grouped in to a ParticleSystem which defines some behaviour and tracks all of its particles lifetime.
		// To avoid having a model matrix for every individual particle, the system will have only one, and the particles' positions (points)
		// will be defined relative to that point. Each individual particle positions is just a vertex of the particle system, in a dynamic element buffer.

		struct ParticleVertex {
			glm::vec3 position;
			float scale;
			glm::vec2 textureOffset;
		};

		struct ParticleMaterial {
			float textureTileLength;
			glm::vec4 tint;
		};

		// A 'Particle' only needs to know its index inside the system's vertex range, how long its been alive for, and its velocity.
		struct Particle {
			const size_t index;
			glm::vec3 velocity;
			float lifetime;

			Particle(const size_t idx)
				: index(idx)  {
				reset();
			}
			void reset() {
				velocity = glm::vec3(0.0f);
				lifetime = 0.0f;
			}
		};
		class ParticleSystem : public Entity {
		public:
			void update(float dt) override;
		protected:
			// Number of tiles on xy is identical for x and y, as textures must be square.
			ParticleSystem(size_t maxParticles, float particleLifetime, float updateInterval, float textureTileLength);
			virtual ~ParticleSystem();
			virtual void particleCreation(float dt, size_t expiredCount) = 0;
			virtual void updateParticle(ParticleVertex vertex) = 0;

			// Tiles in the texture must be arranged left to right, top to bottom.
			void nextTextureTile(glm::vec2& currentOffset);

			float particleLifetime_;
		private:
			// Functions to move a particle between free and active data structures
			Particle allocateParticle();
			void freeParticle();

			ParticleMaterial material_;
			// The particles are not necessarily updated every frame, but after a specified time interval in seconds. An interval of 0 will
			// cause it to update once per frame.
			float updateInterval_;
			float elapsedUpdateTime_;
			// Particles are allocated from an available space up to max particles.
			// A stack is used for free particles, so that the lowest index is always taken first, and placed back at the top when freed.
			std::stack<Particle, std::deque<Particle>> freeParticles_;
			// For simplicity, all particles must have the same lifetime and are therefore removed first in first out
			std::deque<Particle> activeParticles_;
			std::vector<ParticleVertex> vertices_;
		};
	}
}
