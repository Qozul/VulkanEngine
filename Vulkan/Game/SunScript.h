#pragma once
#include "ParticleSystem.h"
#include "../../Shared/Utility.h"
#include "../InputManager.h"
#include "../Graphics/LogicalCamera.h"

namespace QZL {
	namespace Game {
		// In this demo, the sun rotates around the earth. This simple script
		// moves the entity along the circumference of a circle with radius = RADIUS
		// and centre at the parent's transform, moving by DISTANCE_PER_SECOND.
		class SunScript : public ParticleSystem {
		public:
			SunScript(const SystemMasters& initialiser);
			~SunScript();

			glm::vec3* getSunIntensity();
			glm::vec3* getSunDirection();

		protected:
			void start() override;
			void update(float dt, const glm::mat4& viewProjection, const glm::mat4& parentMatrix) override;
			// Ignore the particle system default behaviour
			void particleCreation(float dt, size_t expiredCount) override {};
			void updateParticle(Particle& particle, Graphics::ParticleVertex& vertex, float dt) override {}

		private:
			float angle_;
			glm::vec3 direction_;
			glm::vec3 intensity_;
			Graphics::LogicalCamera* sunCamera_;

			static constexpr float TWO_PI = static_cast<float>(std::_Pi) * 2.0f;
			static constexpr float SPEED = 0.005f;
			static constexpr float DISTANCE_PER_SECOND = SPEED * TWO_PI;
			static constexpr float RADIUS = 700.0f;
			static constexpr float PI_BY_TWO = 1.57079632679f;
		};
	}
}
