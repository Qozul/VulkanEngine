#pragma once
#include "GameScript.h"
#include "../../Shared/Utility.h"
#include "../InputManager.h"

namespace QZL {
	namespace Game {
		class Camera : public GameScript {
		public:
			Camera(const GameScriptInitialiser& initialiser);
			~Camera();
		protected:
			void start() override;
			void update(float dt, const glm::mat4& parentMatrix) override;

		private:
			void moveLeft();
			void moveRight();
			void moveForwards();
			void moveBackwards();
			void moveUp();
			void moveDown();
			void increaseSpeed();
			void decreaseSpeed();
			void updatePosition();
			void logPosition();

			glm::mat4* viewMatrixPtr_;
			glm::vec3* position_;
			glm::vec3 lookPoint_;
			float yaw_;
			float pitch_;
			InputProfile inputProfile_;
			float speed_;

			static const int MAX_SPEED = 100;
			static const int MIN_SPEED = 1;

			static const float SPEED;
			static const float MOUSE_SENSITIVITY;
			static const float MAX_ROTATION_DT;
		};
	}
}
