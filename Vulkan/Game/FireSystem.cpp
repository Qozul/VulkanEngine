#include "FireSystem.h"

using namespace QZL;
using namespace Game;

FireSystem::FireSystem(const GameScriptInitialiser& initialiser, glm::vec3* billboardPoint, Graphics::ElementBufferObject* buf)
	: ParticleSystem(initialiser, billboardPoint, buf, 10, 0.0f, (1.0f / 3.0f), "Fire")
{
}

void FireSystem::start()
{
	particleCreation(0, 10);
	transform()->position = *billboardPoint_;
	transform()->scale = glm::vec3(1.0f);
}

void FireSystem::particleCreation(float dt, size_t expiredCount)
{
	for (size_t i = 0; i < expiredCount; ++i) {
		// Can just assume not nullptr since its within expired count
		auto p = allocateParticle();
		p->lifetime = 50.0f;
		p->velocity = glm::vec3(0.0f, 1.0f, 0.0f);// *((rand() % 100) / 100.0f);
		vertices_[currentActiveSize_ - 1].position = glm::vec3(i * 10.0f, 0.0f, 0.0f);
		vertices_[currentActiveSize_ - 1].scale = 1.0f;
		vertices_[currentActiveSize_ - 1].textureOffset = glm::vec2(0.0, 0.0);
	}
}

void FireSystem::updateParticle(Particle& particle, Graphics::ParticleVertex& vertex, float dt)
{
}
