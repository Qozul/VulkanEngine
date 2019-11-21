#include "RainSystem.h"

using namespace QZL;
using namespace QZL::Game;

RainSystem::RainSystem(const SystemMasters& initialiser)
	: ParticleSystem(initialiser, &initialiser.graphicsMaster->getCamera(0)->position, 20, 0.0f, (1.0f / 3.0f), "rain")
{
}

void RainSystem::start()
{
	fetchDynamicBuffer();
	particleCreation(0, 20);
	transform()->position = *billboardPoint_;
	transform()->scale = glm::vec3(1.0f);
}

void RainSystem::particleCreation(float dt, size_t expiredCount)
{
	for (size_t i = 0; i < expiredCount; ++i) {
		// Can just assume not nullptr since its within expired count
		auto p = allocateParticle();
		p->lifetime = 1.0f;
		p->velocity = glm::vec3(0.0f, -5.0f, 0.0f);// *((rand() % 100) / 100.0f);
		vertices_[currentActiveSize_ - 1].position = *billboardPoint_ + glm::vec3(10.0f) * ((rand() % 100) / 100.0f);
		vertices_[currentActiveSize_ - 1].scale = 2.0f;
		vertices_[currentActiveSize_ - 1].textureOffset = glm::vec2(0.0, 0.0);
	}
}

void RainSystem::updateParticle(Particle& particle, Graphics::ParticleVertex& vertex, float dt)
{
}
