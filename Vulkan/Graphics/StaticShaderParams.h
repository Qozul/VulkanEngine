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
			friend class StaticRenderStorage;
		public:
			const std::string& getDiffuseName() const {
				return diffuse_;
			}
			const std::string& getNormalMapName() const {
				return normalMap_;
			}
		private:
			StaticShaderParams(TextureLoader* textureLoader, LogicDevice* logicDevice, const std::string& diffuseName, const std::string& normalMapName)
				: diffuse_(diffuseName), normalMap_(normalMapName) {}

			const std::string diffuse_;
			const std::string normalMap_;
		};
	}
}