#pragma once
#include <vector>

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
		private:
			GameMaster(const SystemMasters& masters);
			~GameMaster();
			void loadGame();
			void loadDescriptors();
			void update(float dt);

			const SystemMasters& masters_;

			std::vector<Scene*> scenes_;
			size_t activeSceneIdx_;
		};
	}
}
