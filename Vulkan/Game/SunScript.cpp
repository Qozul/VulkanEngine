#include "SunScript.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace QZL::Game;

SunScript::SunScript(const GameScriptInitialiser& initialiser, glm::vec3* billboardPoint, Graphics::DynamicBufferInterface* buf)
	: GameScript(initialiser), angle_(0.0f)//ParticleSystem(initialiser, billboardPoint, buf, 2, 0.0f, (1.0f / 3.0f), "Particles/explosion"), angle_(0.0f)
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
	// TODO: return the direction from the sun particle to the centre point
	return glm::normalize(glm::vec3(owningEntity_->getModelMatrix()[3][0], owningEntity_->getModelMatrix()[3][1], owningEntity_->getModelMatrix()[3][2]));
}

void SunScript::start()
{
	// Ensure both sun and moon particles have zero velocity
	//particles_[0].reset();
	//particles_[1].reset();
	transform()->position = glm::vec3(0.0, 1.0, 0.0) * RADIUS;
}

void SunScript::particleCreation(float dt, size_t expiredCount)
{
}

void SunScript::update(float dt) 
{
	angle_ += DISTANCE_PER_SECOND * dt;
	if (angle_ > TWO_PI) {
		angle_ = 0.0f;
	}
	transform()->position = glm::vec3(std::cos(angle_) * RADIUS, glm::sin(angle_) * RADIUS, 0.0f);
}

/*
void SunScript::updateParticle(Particle& particle, Graphics::ParticleVertex& vertex, float dt)
{
	// TODO Move particles around the centre point y rotating the axis.
	// TODO remember to modify shader to counter rotate world position
	// - just make the rotation angle_ around x axis
	// - need uniform buffer for system instances model, mvp matrices, and tint
	// - push constant billboard point, tilelength
	angle_ += DISTANCE_PER_SECOND * dt;
	if (angle_ > TWO_PI) {
		angle_ = 0.0f;
	}
	transform()->position = glm::vec3(std::cos(angle_) * RADIUS, glm::sin(angle_) * RADIUS, 0.0f);
}
*/
