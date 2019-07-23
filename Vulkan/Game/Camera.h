#pragma once
#include "GameScript.h"
#include "../../Shared/Utility.h"

namespace QZL {
	namespace Game {
		class Camera : public GameScript {
		protected:
			Camera(const GameScriptInitialiser& initialiser);
			void start() override;
			void update() override;

		private:
			glm::mat4* viewMatrixPtr_;
		};
	}
}
