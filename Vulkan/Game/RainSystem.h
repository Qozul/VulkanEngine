#pragma once
#include "ParticleSystem.h"


namespace QZL {
	namespace Game {
		class RainSystem : public ParticleSystem {
		public:
			RainSystem(const SystemMasters& initialiser);
			void start() override;
		protected:
			void particleCreation(float dt, size_t expiredCount) override;
			void updateParticle(Particle& particle, Graphics::ParticleVertex& vertex, float dt) override;
		};
	}
}
