#pragma once
#include "GameScript.h"
#include "../Graphics/Vertex.h"
#include "../Graphics/Material.h"
#include "../Graphics/ShaderParams.h"
#include "../Graphics/ElementBufferObject.h"

namespace QZL {
	namespace Graphics {
		class DynamicBufferInterface;
		class RenderObject;
	}
	namespace Game {
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

		class ParticleSystem : public GameScript {
		public:
			// Update is controlled in this class to ensure correct behaviour, however start should be derived to
			// initialise the particle state.
			virtual void start() = 0;
			// Update can be overriden by a derived class when needed, however this provides the basic particle system logic
			virtual void update(float dt, const glm::mat4& parentMatrix) override;
			Graphics::ParticleShaderParams* makeShaderParams() {
				return new Graphics::ParticleShaderParams(textureTileLength_, tint_);
			}
			Graphics::RenderObject* makeRenderObject(std::string name);
		protected:
			// Number of tiles on xy is identical for x and y, as textures must be square.
			ParticleSystem(const GameScriptInitialiser& initialiser, glm::vec3* billboardPoint, Graphics::ElementBufferObject* buf,
				size_t maxParticles, float updateInterval, float textureTileLength, const std::string& materialName);
			virtual ~ParticleSystem();
			virtual void particleCreation(float dt, size_t expiredCount) = 0;
			virtual void updateParticle(Particle& particle, Graphics::ParticleVertex& vertex, float dt) = 0;

			// Tiles in the texture must be arranged left to right, top to bottom.
			void nextTextureTile(glm::vec2& currentOffset);

			void updateBuffer();
			
			// When true, system update is skipped, instead just updating each particle. This optimises when
			// particles have infinite lifetime and do not overlap. This is false by default.
			bool alwaysAliveAndUnordered_;
			Particle* allocateParticle();

			Graphics::Material* material_;
			float textureTileLength_;
			glm::vec4 tint_;
			size_t currentActiveSize_;
			
			// Allocate particle returns nullptr if there is no available particle
			void freeParticle(size_t idx);
			void updateActiveSize() {
				currentActiveSize_ = particles_.size() - numDeadParticles_;
			}

			// The particles are not necessarily updated every frame, but after a specified time interval in seconds. An interval of 0 will
			// cause it to update once per frame.
			float updateInterval_;
			float elapsedUpdateTime_;
			glm::vec3* billboardPoint_;
			Graphics::SubBufferRange subBufferRange_;
			Graphics::ElementBufferObject* buffer_;

			size_t numDeadParticles_;
			std::vector<Particle> particles_;
			std::vector<Graphics::ParticleVertex> vertices_;
		};
	}
}
