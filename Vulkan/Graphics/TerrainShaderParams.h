#pragma once
#include "VkUtil.h"

namespace QZL {
	namespace Game {
		class GameMaster;
	}
	namespace Graphics {
		class TerrainShaderParams {
			friend class GraphicsComponent;
			friend class TerrainRenderer;
			friend class Game::GameMaster;
		public:
			const std::string& getHeightmapName() const {
				return heightmapName_;
			}
			const std::string& getDebugDiffuseName() const {
				return debugDiffuseName_;
			}
			TerrainShaderParams(const std::string& heightmapName, const std::string& debugDiffuseName)
				: heightmapName_(heightmapName), debugDiffuseName_(debugDiffuseName) {}
		private:
			const std::string heightmapName_;
			const std::string debugDiffuseName_;
		};
	}
}
