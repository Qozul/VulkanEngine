#pragma once
#include "Entity.h"

namespace QZL {
	namespace Graphics {
		class TextureManager;
	}
	class Terrain : public Entity {
	public:
		Terrain(const std::string name, Graphics::TextureManager* textureManager);
	private:
		static void loadFunction(uint32_t& count, std::vector<char>& indices, std::vector<char>& vertices);
		static constexpr float maxHeight = 100.0f;
	};
}
