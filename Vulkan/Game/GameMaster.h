#pragma once
#include "../Graphics/VkUtil.h"

namespace QZL {
	struct SystemMasters;
	class System;
	class Scene;
	namespace Graphics {
		struct SceneGraphicsInfo;
		struct LogicalCamera;
	}
	namespace Game {
		class GameMaster final {
			friend class QZL::System;
		public:
			Scene* getActiveScene() {
				return scenes_[activeSceneIdx_];
			}
			std::vector<VkDrawIndexedIndirectCommand>* update(glm::mat4& viewProjection, float dt, const uint32_t& frameIdx, Graphics::LogicalCamera& mainCamera);
			void start();
		private:
			GameMaster(const SystemMasters& masters);
			~GameMaster();
			void loadGame();
			Graphics::SceneGraphicsInfo* loadDescriptors();

			const SystemMasters& masters_;

			std::vector<Scene*> scenes_;
			size_t activeSceneIdx_;
		};
	}
}
