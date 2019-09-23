#pragma once
#include "ParticleSystem.h"

namespace QZL {
	namespace Game {
		class FireSystem : public ParticleSystem {
		public:
			FireSystem(const GameScriptInitialiser& initialiser, glm::vec3* billboardPoint, Graphics::DynamicBufferInterface* buf);
			void start() override;
		protected:
			void particleCreation(float dt, size_t expiredCount) override;
			void updateParticle(Particle& particle, Graphics::ParticleVertex& vertex, float dt) override;
		};
	}
}
