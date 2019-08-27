#include "SunScript.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace QZL::Game;

SunScript::SunScript(const GameScriptInitialiser& initialiser)
	: GameScript(initialiser), angle_(0.0f)
{
}

SunScript::~SunScript()
{
}

glm::vec3 SunScript::getSunIntensity()
{
	auto sinTheta = glm::sin(angle_) + DAWN_DUSK_OFFSET;
	auto sign = glm::sign(sinTheta);
	auto factor = sign * glm::pow((sinTheta), 2.0f);
	return glm::max(glm::vec3(10.0f) * factor, glm::vec3(0.0f));
}

  glm::vec3 SunScript::getSunDirection()
{
	  return glm::normalize(glm::vec3(owningEntity_->getModelMatrix()[3][0], owningEntity_->getModelMatrix()[3][1], owningEntity_->getModelMatrix()[3][2]));
}

void SunScript::start()
{
	transform()->position = glm::vec3(0.0, 1.0, 0.0) * RADIUS;
}

void SunScript::update(float dt)
{
	angle_ += DISTANCE_PER_SECOND * dt;
	if (angle_ > TWO_PI) {
		angle_ = 0.0f;
	}
	transform()->position = glm::vec3(std::cos(angle_) * RADIUS, glm::sin(angle_) * RADIUS, 0.0f);
}
