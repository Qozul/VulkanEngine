#pragma once
#include "VkUtil.h"

namespace QZL {
	namespace Game {
		class GameMaster;
	}
	namespace Graphics {
		class TextureSampler;
		class LogicDevice;
		class TextureLoader;

		class StaticShaderParams {
			friend class GraphicsComponent;
			friend class TexturedRenderer;
			friend class Game::GameMaster;
		private:
			StaticShaderParams(TextureLoader* textureLoader, LogicDevice* logicDevice, const std::string& diffuseName, const std::string& normalMapName);
			~StaticShaderParams();

			TextureSampler* diffuse_;
			TextureSampler* normalMap_;
		};
	}
}