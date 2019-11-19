#pragma once
#include "Entity.h"

namespace QZL {
	namespace Graphics {
		class TextureManager;
	}
	class Water : public Entity {
	public:
		Water(const std::string name, Graphics::TextureManager* textureManager);
	private:
		static void loadFunction(uint32_t& count, std::vector<char>& indices, std::vector<char>& vertices);
	};
}
