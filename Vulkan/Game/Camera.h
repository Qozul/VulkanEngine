#pragma once
#include "GameScript.h"
#include "../../Shared/Utility.h"
#include "../InputManager.h"

namespace QZL {
	namespace Graphics {
		struct LogicalCamera;
	}
	namespace Game {
		class Camera : public GameScript {
			struct TrackNode {
				glm::vec3 position;
				float pitch;
				float yaw;
			};
		public:
			Camera(const SystemMasters& initialiser);
			~Camera();
		protected:
			void start() override;
			void update(float dt, const glm::mat4& viewProjection, const glm::mat4& parentMatrix) override;

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

			Graphics::LogicalCamera* mainCamera_;
			float yaw_;
			float pitch_;
			InputProfile inputProfile_;
			float speed_;

			bool isOnTrack_;
			std::vector<TrackNode> trackNodes_;
			int currentNode_;
			int maxNode_;
			float trackAmount_;

			static const int MAX_SPEED = 200;
			static const int MIN_SPEED = 1;

			static const float SPEED;
			static const float MOUSE_SENSITIVITY;
			static const float MAX_ROTATION_DT;
		};
	}
}
