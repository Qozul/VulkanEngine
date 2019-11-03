#pragma once
#include "Entity.h"
#include "../Graphics/MeshLoader.h"

namespace QZL {
	namespace Graphics {
		class TextureManager;
	}
	namespace Assets {
		class Terrain : public Entity {
		public:
			Terrain(const std::string name, Graphics::TextureManager* textureManager);
		private:
			static void loadFunction(uint32_t& count, std::vector<char>& indices, std::vector<char>& vertices);
			static constexpr float maxHeight = 100.0f;
		};
	}
}
