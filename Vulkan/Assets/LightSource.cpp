#include "LightSource.h"
#include "Transform.h"

using namespace QZL;

LightSource::LightSource(const std::string name, glm::vec3 colour, float radius, float attenFactor)
	: Entity(name), light_({ glm::vec3(0.0), radius, colour, attenFactor })
{
	setGraphicsComponent(Graphics::RendererTypes::kLight, nullptr, nullptr, nullptr, "ico");
}

void LightSource::update(float dt, const glm::mat4& viewProjection, const glm::mat4& parentMatrix)
{
	auto ctm = (parentMatrix * transform_->toModelMatrix());
	light_.position = glm::vec3(ctm[3][0], ctm[3][1], ctm[3][2]);
}
