#include "ParticleSystem.h"
#include "../Graphics/RenderObject.h"
#include "../System.h"
#include "../Assets/AssetManager.h"
#include "../Graphics/TextureManager.h"

using namespace QZL;
using namespace QZL::Game;

void ParticleSystem::update(float dt, const glm::mat4& parentMatrix)
{
	elapsedUpdateTime_ += dt;
	if (elapsedUpdateTime_ >= updateInterval_) {

		if (!alwaysAliveAndUnordered_) {
			// Update or free every particle that is currently active
			size_t i = 0;
			while (i < currentActiveSize_) {
				Particle& particle = particles_[i];
				particle.lifetime -= elapsedUpdateTime_;
				if (particles_[i].lifetime <= 0.0f) {
					// i does not increment because the new particle at index i needs to be checked like any other,
					// but currentActiveSize_ does decrement, so infinite loop should not be possible.
					freeParticle(i);
				}
				else {
					// Update velocity/lifetime and move position by velocity
					Graphics::ParticleVertex& vertex = vertices_[i];
					updateParticle(particle, vertex, elapsedUpdateTime_);
					vertex.position = vertex.position + (particle.velocity * elapsedUpdateTime_);
					++i;
				}
			}

			// Create any new particles, this is defined by derived classes
			particleCreation(elapsedUpdateTime_, numDeadParticles_);

			// Sort the particles to draw closest to the camera using insertion sort
			for (size_t i = 1; i < currentActiveSize_; ++i) {
				long long int j = i - 1;
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
				Graphics::ParticleVertex& vertex = vertices_[i];
				updateParticle(particles_[i], vertex, dt);
				vertex.position = particles_[i].velocity * elapsedUpdateTime_;
			}
		}
		updateBuffer();
		elapsedUpdateTime_ = 0.0f;
	}
}

Graphics::RenderObject* ParticleSystem::makeRenderObject(std::string name)
{
	Graphics::BasicMesh* mesh = new Graphics::BasicMesh();
	mesh->count = static_cast<uint32_t>(subBufferRange_.count);
	mesh->vertexOffset = static_cast<uint32_t>(subBufferRange_.first / sizeof(Graphics::ParticleVertex));
	mesh->indexOffset = 0;
	return new Graphics::RenderObject(name, mesh, makeShaderParams(), material_);
}

ParticleSystem::ParticleSystem(const GameScriptInitialiser& initialiser, glm::vec3* billboardPoint, Graphics::ElementBufferObject* buf,
	size_t maxParticles, float updateInterval, float textureTileLength, const std::string& materialName)
	: GameScript(initialiser), updateInterval_(updateInterval), billboardPoint_(billboardPoint), elapsedUpdateTime_(0.0f), alwaysAliveAndUnordered_(false),
	numDeadParticles_(maxParticles), buffer_(buf), currentActiveSize_(0)
{
	ASSERT(billboardPoint_ != nullptr);
	ASSERT(buf != nullptr);
	particles_.resize(maxParticles);
	vertices_.resize(maxParticles);
	subBufferRange_ = buf->allocateSubBufferRange(maxParticles);

	material_ = initialiser.system->getMasters().assetManager->textureManager->requestMaterial<Graphics::ParticleMaterial>(materialName);
	tint_ = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	textureTileLength_ = textureTileLength;
}

ParticleSystem::~ParticleSystem()
{
}

void ParticleSystem::nextTextureTile(glm::vec2& tileOffset)
{
	// Move one along on the columns, if this wraps it around to the beginning then move down a row.
	tileOffset.x += textureTileLength_;
	if (tileOffset.x >= 1.0 - FLT_EPSILON) {
		tileOffset.x = 0.0f;
		tileOffset.y += textureTileLength_;
	}
	// Ensure that rows also wrap to the top and remain in the texture range.
	if (tileOffset.y >= 1.0 - FLT_EPSILON) {
		tileOffset.y = 0.0f;
	}
}

void ParticleSystem::updateBuffer()
{
	void* v = buffer_->getSubBufferData(subBufferRange_.first);
	memcpy(v, vertices_.data(), vertices_.size() * sizeof(Graphics::ParticleVertex));
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
	particles_[idx].swapAndReset(particles_[currentActiveSize_ - 1]);
	std::swap(vertices_[idx], vertices_[currentActiveSize_ - 1]);
	++numDeadParticles_;
	updateActiveSize();
}
