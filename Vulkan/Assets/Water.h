#pragma once
#include "Entity.h"

namespace QZL {
	namespace Graphics {
		class TextureManager;
	}
	class Water : public Entity {
	public:
		Water(const std::string name, Graphics::TextureManager* textureManager);
		void update(float dt, const glm::mat4& viewProjection, const glm::mat4& parentMatrix) override;
	private:
		static void loadFunction(uint32_t& count, std::vector<char>& indices, std::vector<char>& vertices);
	};
}
