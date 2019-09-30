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
			Terrain(const std::string name);
		private:
			static void loadFunction(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::Vertex>& vertices);
			static constexpr float maxHeight = 100.0f;
		};
	}
}
