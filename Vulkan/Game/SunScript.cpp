#include "SunScript.h"
#include "../Assets/Entity.h"
#include "../Graphics/GraphicsMaster.h"
#include "../Graphics/LogicalCamera.h"

using namespace QZL;
using namespace QZL::Game;

SunScript::SunScript(const SystemMasters& initialiser)
	: ParticleSystem(initialiser, &initialiser.graphicsMaster->getCamera(0)->position, 2, 0.0f, 0.5f, "SunMoon"), angle_(glm::radians(45.0f)), 
	sunCamera_(initialiser.graphicsMaster->getCamera(1))
{
}

SunScript::~SunScript()
{
}

glm::vec3* SunScript::getSunIntensity()
{
	return &intensity_;
}

glm::vec3* SunScript::getSunDirection()
{
	return &direction_;
}

void SunScript::start()
{
	transform()->position = glm::vec3(512.0f, 0.0f, 512.0f);
	transform()->rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
	// Setup sun
	vertices_[0].position = glm::vec3(1.0f * RADIUS, 0.0f, 0.0f);
	vertices_[0].scale = 40.0f;
	vertices_[0].textureOffset = glm::vec2(0.0f);
	// Setup moon
	vertices_[1].position = glm::vec3(-1.0f * RADIUS, 0.0f, 0.0f);
	vertices_[1].scale = 40.0f;
	vertices_[1].textureOffset = glm::vec2(0.0f, 0.0f);

	intensity_ = glm::vec3(6.5e-7, 5.1e-7, 4.75e-7) * glm::vec3(1e7);
}

void SunScript::update(float dt, const glm::mat4& viewProjection, const glm::mat4& parentMatrix)
{
	angle_ += DISTANCE_PER_SECOND * dt;
	if (angle_ > TWO_PI) {
		angle_ = 0.0f;
	}
	transform()->rotationAngle = angle_;

	// The direction from the sun particle to the centre point in world space, which in model space is +x
	direction_ = glm::vec3(1.0, 1.0, 0.0);

	sunCamera_->position = (direction_ * RADIUS);
	auto localUp = direction_;
	float x = localUp.x;
	float y = localUp.y;
	localUp.x = x * glm::cos(PI_BY_TWO) - y * glm::sin(PI_BY_TWO);
	localUp.y = x * glm::sin(PI_BY_TWO) + y * glm::cos(PI_BY_TWO);
	sunCamera_->viewMatrix = glm::lookAt(sunCamera_->position, glm::vec3(512.0f, 0.0f, 512.0f), glm::vec3(0.0, 1.0, 0.0));
}
