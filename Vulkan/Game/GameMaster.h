#pragma once
#include <vector>

namespace QZL {
	struct SystemMasters;
	class System;
	namespace Game {
		class GameScript;
		class GameMaster final {
			friend class QZL::System;
		private:
			GameMaster(const SystemMasters& masters);
			void loadGame();
			void update(float dt);

			const SystemMasters& masters_;

			std::vector<GameScript*> gameScripts_;
		};
	}
}
