#pragma once
#include "GameScript.h"
#include "../../Shared/Utility.h"
#include "../InputManager.h"

namespace QZL {
	namespace Game {
		// In this demo, the sun rotates around the earth. This simple script
		// moves the entity along the circumference of a circle with radius = RADIUS
		// and centre at the parent's transform, moving by DISTANCE_PER_SECOND.
		class SunScript : public GameScript {
		public:
			SunScript(const GameScriptInitialiser& initialiser);
			~SunScript();

			glm::vec3 getSunIntensity();
			glm::vec3 getSunDirection();

		protected:
			void start() override;
			void update(float dt) override;

		private:
			float angle_;

			static constexpr float TWO_PI = std::_Pi * 2.0f;
			static constexpr float SPEED = 0.00001f;
			static constexpr float DISTANCE_PER_SECOND = SPEED * TWO_PI;
			static constexpr float RADIUS = 81000.0f;
			static constexpr float DAWN_DUSK_OFFSET = -0.1f;
		};
	}
}
