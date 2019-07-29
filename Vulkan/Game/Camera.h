#pragma once
#include "GameScript.h"
#include "../../Shared/Utility.h"
#include "../InputManager.h"

namespace QZL {
	namespace Game {
		class Camera : public GameScript {
		public:
			Camera(const GameScriptInitialiser& initialiser);
		protected:
			void start() override;
			void update(float dt) override;

		private:
			void moveLeft();
			void moveRight();
			void moveForwards();
			void moveBackwards();
			void updatePosition();

			glm::mat4* viewMatrixPtr_;
			glm::vec3* position_;
			glm::vec3 lookPoint_;
			float yaw_;
			float pitch_;
			InputProfile inputProfile_;

			static const float SPEED;
			static const float MOUSE_SENSITIVITY;
		};
	}
}
