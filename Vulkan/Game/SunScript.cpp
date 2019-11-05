#include "SunScript.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace QZL::Game;

SunScript::SunScript(const GameScriptInitialiser& initialiser, glm::vec3* billboardPoint, Graphics::ElementBufferObject* buf)
	: ParticleSystem(initialiser, billboardPoint, buf, 2, 0.0f, (1.0f / 1.0f), "SunMoon"), angle_(glm::radians(150.0f))
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
	// Ensure both sun and moon particles have zero velocity
	particles_[0].reset();
	particles_[1].reset();
	transform()->position = glm::vec3(0.0f);
	transform()->rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
	// Setup sun
	vertices_[0].position = glm::vec3(1.0f * RADIUS, 0.0f, 0.0f);
	vertices_[0].scale = 40.0f;
	vertices_[0].textureOffset = glm::vec2(0.0f);
	// Setup moon
	vertices_[1].position = glm::vec3(-1.0f * RADIUS, 0.0f, 0.0f);
	vertices_[1].scale = 40.0f;
	vertices_[1].textureOffset = glm::vec2(0.0f, 0.0f);

	updateBuffer();

	intensity_ = glm::vec3(6.5e-7, 5.1e-7, 4.75e-7) * glm::vec3(1e7);
}

void SunScript::update(float dt, const glm::mat4& parentMatrix)
{
	angle_ += DISTANCE_PER_SECOND * dt;
	if (angle_ > TWO_PI) {
		angle_ = 0.0f;
	}
	transform()->rotationAngle = angle_;

	// The direction from the sun particle to the centre point in world space, which in model space is +x
	direction_ = glm::vec3(parentMatrix * (transform()->toModelMatrix() * glm::vec4(vertices_[0].position, 1.0)));

	static_cast<Graphics::ParticleShaderParams*>(owningEntity_->getGraphicsComponent()->getPerMeshShaderParams())->params.tint = glm::abs(glm::cos(angle_)) * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
}
