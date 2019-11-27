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
		LightSource(const std::string name, glm::vec3 colour, float radius, float attenFactor, glm::vec3* masterPos = nullptr);
		void update(float dt, const glm::mat4& viewProjection, const glm::mat4& parentMatrix) override;
		Graphics::Light& getLight() {
			return light_;
		}
	private:
		Graphics::Light light_;
		glm::vec3* masterPos_;
	};
}
