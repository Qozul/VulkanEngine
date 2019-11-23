// Author: Ralph Ridley
// Date: 23/11/19
#pragma once
#include "Entity.h"
#include "../Graphics/Light.h"

namespace QZL {
	namespace Graphics {
		class TextureManager;
	}
	class LightSource : public Entity {
	public:
		LightSource(const std::string name, glm::vec3 colour, float radius, float attenFactor);
		void update(float dt, const glm::mat4& viewProjection, const glm::mat4& parentMatrix) override;
	private:
		Graphics::Light light_;
	};
}
