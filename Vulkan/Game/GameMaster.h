#pragma once

namespace QZL {
	struct SystemMasters;
	namespace Game {
		class GameMaster final {
			friend class System;
		private:
			GameMaster(const SystemMasters& masters);
			void loadGame();

			const SystemMasters& masters_;
		};
	}
}
