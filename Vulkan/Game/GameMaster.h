#pragma once
#include "../Graphics/VkUtil.h"

namespace QZL {
	struct SystemMasters;
	class System;
	class Scene;
	namespace Game {
		class GameMaster final {
			friend class QZL::System;
		public:
			Scene* getActiveScene() {
				return scenes_[activeSceneIdx_];
			}
			void update(glm::mat4& viewProjection, float dt, const uint32_t& frameIdx);
			void start();
		private:
			GameMaster(const SystemMasters& masters);
			~GameMaster();
			void loadGame();
			void loadDescriptors();

			const SystemMasters& masters_;

			std::vector<Scene*> scenes_;
			size_t activeSceneIdx_;
		};
	}
}
