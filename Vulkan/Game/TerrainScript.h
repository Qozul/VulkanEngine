#pragma once
#include "GameScript.h"
#include "../../Shared/Utility.h"
#include "../InputManager.h"

namespace QZL {
	namespace Graphics {
		struct LogicalCamera;
	}
	namespace Game {
		class TerrainScript : public GameScript {
		public:
			TerrainScript(const SystemMasters& initialiser);
			~TerrainScript() {}
		protected:
			void start() override {}
			void update(float dt, const glm::mat4& viewProjection, const glm::mat4& parentMatrix) override;
		private:
			Graphics::LogicalCamera* mainCamera_;
		};
	}
}
