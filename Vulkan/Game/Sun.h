#pragma once
#include "GameScript.h"
#include "../../Shared/Utility.h"
#include "../InputManager.h"

namespace QZL {
	namespace Game {
		// In this demo, the sun rotates around the earth. This simple script
		// moves the entity along the circumference of a circle of radius ORBITAL_RADIUS
		// with centre at the parent's transform, moving by ORBITAL_SPEED each frame.
		class SunScript : public GameScript {
		public:
			SunScript(const GameScriptInitialiser& initialiser);
			~SunScript();
		protected:
			void start() override;
			void update(float dt) override;

		private:
			static const float ORBITAL_SPEED = 10.0f;
			static const float ORBITAL_RADIUS = 81000.0f;
		};
	}
}
