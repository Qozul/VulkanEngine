#pragma once
#include "ShaderParams.h"
/*
namespace QZL {
	namespace Game {
		class GameMaster;
	}
	namespace Graphics {
		class TerrainShaderParams : public ShaderParams {
			friend class GraphicsComponent;
			friend class TerrainRenderer;
			friend class Game::GameMaster;
		public:
			const RendererTypes getRendererType() const override {
				return RendererTypes::TERRAIN;
			}
			const std::string getParamsId() const override {
				return heightmapName_ + "." + debugDiffuseName_;
			}
			const std::string& getHeightmapName() const {
				return heightmapName_;
			}
			const std::string& getDebugDiffuseName() const {
				return debugDiffuseName_;
			}
			MaterialStatic& getMaterial() {
				return material_;
			}
			TerrainShaderParams(const std::string& heightmapName, const std::string& debugDiffuseName, MaterialStatic material)
				: heightmapName_(heightmapName), debugDiffuseName_(debugDiffuseName), material_(material) {}
		private:
			const std::string heightmapName_;
			const std::string debugDiffuseName_;
			MaterialStatic material_;
		};
	}
}*/
