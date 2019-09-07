#include "ParticleSystem.h"

using namespace QZL;
using namespace QZL::Assets;

void ParticleSystem::update(float dt)
{
	elapsedUpdateTime_ += dt;
	if (elapsedUpdateTime_ >= updateInterval_) {
		elapsedUpdateTime_ = 0.0f;
		// Free any expired particles
		size_t expiredCount = 0;
		bool expired = false;
		do {
			auto p = activeParticles_.front();
			if (p.lifetime + elapsedUpdateTime_ >= particleLifetime_) {
				// If gaps appear in the vertex data it shouldn't draw them.
				// Gaps may appear if all the expired particles are not replaced the same update.
				vertices_[p.index].scale = 0.0f;
				activeParticles_.pop_front();
				++expiredCount;
				expired = true;
			}
		} while (expired);

		// Create any new particles, this is defined by derived classes
		particleCreation(dt, expiredCount);

		// Update each active particle
		for (size_t i = 0; i < activeParticles_.size(); ++i) {
			updateParticle(vertices_[activeParticles_[i].index]);
		}
	}
}

ParticleSystem::ParticleSystem(size_t maxParticles, float particleLifetime, float updateInterval, float textureTileLength)
	: particleLifetime_(particleLifetime), updateInterval_(updateInterval), elapsedUpdateTime_(0.0f)
{
	material_.textureTileLength = textureTileLength;
	for (size_t i = 0; i < maxParticles; ++i) {
		freeParticles_.emplace(i);
	}
}

ParticleSystem::~ParticleSystem()
{
}

void ParticleSystem::nextTextureTile(glm::vec2& tileOffset)
{
	// Move one along on the columns, if this wraps it around to the beginning then move down a row.
	tileOffset.x += material_.textureTileLength;
	if (tileOffset.x >= 1.0 - FLT_EPSILON) {
		tileOffset.x = 0.0f;
		tileOffset.y += material_.textureTileLength;
	}
	// Ensure that rows also wrap to the top and remain in the texture range.
	if (tileOffset.y >= 1.0 - FLT_EPSILON) {
		tileOffset.y = 0.0f;
	}
}

Particle ParticleSystem::allocateParticle()
{
	ASSERT(!freeParticles_.empty());
	auto p = freeParticles_.top();
	freeParticles_.pop();
	activeParticles_.push_back(p);
	p.reset();
	return p;
}

void ParticleSystem::freeParticle()
{
	if (!activeParticles_.empty()) {
		auto p = activeParticles_.front();
		activeParticles_.pop_front();
		freeParticles_.push(p);
	}
}
