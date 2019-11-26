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
			void start();
		private:
			GameMaster(const SystemMasters& masters);
			~GameMaster();
			void loadGame();
			Graphics::SceneGraphicsInfo* loadDescriptors();
			void loadStatics(Scene* scene);
			const SystemMasters& masters_;

			std::vector<Scene*> scenes_;
			size_t activeSceneIdx_;
		};
	}
}
