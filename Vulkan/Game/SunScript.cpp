#include "SunScript.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace QZL::Game;

SunScript::SunScript(const GameScriptInitialiser& initialiser, glm::vec3* billboardPoint, Graphics::DynamicBufferInterface* buf)
	: ParticleSystem(initialiser, billboardPoint, buf, 2, 0.0f, (1.0f / 3.0f), "Particles/explosion"), angle_(0.0f)
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
	// Return the direction from the sun particle to the centre point in world space, which in model space is +x
	return glm::normalize(glm::vec3(transform()->toModelMatrix() * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)));
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
	vertices_[0].scale = 20.0f;
	vertices_[0].textureOffset = glm::vec2(0.0f);
	// Setup moon
	vertices_[1].position = glm::vec3(-1.0f * RADIUS, 0.0f, 0.0f);
	vertices_[1].scale = 20.0f;
	vertices_[1].textureOffset = glm::vec2(0.0f, 0.5f);

	updateBuffer();
}

void SunScript::update(float dt) 
{
	angle_ += DISTANCE_PER_SECOND * dt;
	if (angle_ > TWO_PI) {
		angle_ = 0.0f;
	}
	transform()->rotationAngle = angle_;
}