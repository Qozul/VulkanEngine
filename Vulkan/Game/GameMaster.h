#pragma once
#include <vector>

namespace QZL {
	struct SystemMasters;
	class System;
	namespace Game {
		class Scene;
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
			void update(float dt);

			const SystemMasters& masters_;

			std::vector<Scene*> scenes_;
			size_t activeSceneIdx_;
		};
	}
}
