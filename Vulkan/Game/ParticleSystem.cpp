#include "ParticleSystem.h"

using namespace QZL;
using namespace QZL::Game;

void ParticleSystem::update(float dt)
{
	elapsedUpdateTime_ += dt;
	if (elapsedUpdateTime_ >= updateInterval_) {
		elapsedUpdateTime_ = 0.0f;

		if (!alwaysAliveAndUnordered_) {
			// Update or free every particle that is currently active
			size_t i = 0;
			while (i < currentActiveSize_) {
				particles_[i].lifetime -= elapsedUpdateTime_;
				if (particles_[i].lifetime <= 0.0f) {
					// i does not increment because the new particle at index i needs to be checked like any other,
					// but currentActiveSize_ does decrement, so infinite loop should not be possible.
					freeParticle(i);
				}
				else {
					updateParticle(particles_[i], vertices_[i]);
					++i;
				}
			}

			// Create any new particles, this is defined by derived classes
			particleCreation(dt, numDeadParticles_);

			// Sort the particles to draw closest to the camera using insertion sort
			for (int i = 1; i < currentActiveSize_; ++i) {
				int j = i - 1;
				auto iVertVal = vertices_[i];
				auto iPartVal = particles_[i];
				float iKey = glm::distance(vertices_[i].position, *billboardPoint_);
				float jKey = glm::distance(vertices_[j].position, *billboardPoint_);
				while (j >= 0 && jKey > iKey) {
					vertices_[j + 1] = vertices_[j];
					particles_[j + 1] = particles_[j];
					--j;
				}
				vertices_[j + 1] = iVertVal;
				particles_[j + 1] = iPartVal;
			}
		}
		else {
			for (size_t i = 0; i < currentActiveSize_; ++i) {
				updateParticle(particles_[i], vertices_[i]);
			}
		}
		void* v = buffer_->getSubBufferData(subBufferRange_.first);
		memcpy(v, vertices_.data(), vertices_.size() * sizeof(Graphics::ParticleVertex));
	}
}

ParticleSystem::ParticleSystem(const GameScriptInitialiser& initialiser, size_t maxParticles, float updateInterval, float textureTileLength, const std::string& textureName,
	glm::vec3* billboardPoint, Graphics::DynamicBufferInterface* buf)
	: GameScript(initialiser), updateInterval_(updateInterval), billboardPoint_(billboardPoint), elapsedUpdateTime_(0.0f), alwaysAliveAndUnordered_(false),
	numDeadParticles_(0.0f), buffer_(buf), particles_(maxParticles)
{
	ASSERT(billboardPoint_ != nullptr);
	ASSERT(buf != nullptr);
	subBufferRange_ = buf->allocateSubBufferRange(maxParticles);
	// TODO: load texture with textureName from manager

	material_.textureTileLength = textureTileLength;
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

Particle* ParticleSystem::allocateParticle()
{
	Particle* p = nullptr;
	if (numDeadParticles_ > 0) {
		p = &particles_[currentActiveSize_];
		--numDeadParticles_;
		updateActiveSize();
	}
	return p;
}

void ParticleSystem::freeParticle(size_t idx)
{
	particles_[idx].swapAndReset(particles_[currentActiveSize_]);
	std::swap(vertices_[idx], vertices_[currentActiveSize_]);
	++numDeadParticles_;
	updateActiveSize();
}
