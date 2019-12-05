#include "TerrainScript.h"
#include "../Assets/Entity.h"
#include "../Graphics/GraphicsComponent.h"
#include "../Graphics/ShaderParams.h"
#include "../Graphics/GraphicsMaster.h"

using namespace QZL;
using namespace QZL::Game;
using namespace QZL::Graphics;

TerrainScript::TerrainScript(const SystemMasters& initialiser)
	: GameScript(initialiser), mainCamera_(initialiser.graphicsMaster->getCamera(0))
{
}

void TerrainScript::update(float dt, const glm::mat4& viewProjection, const glm::mat4& parentMatrix)
{
	const float kSnowAccumulationRate = 0.00001f;
	static_cast<TerrainShaderParams*>(owningEntity_->getGraphicsComponent()->getShaderParams())->heights.w -= kSnowAccumulationRate;
	static_cast<TerrainShaderParams*>(owningEntity_->getGraphicsComponent()->getShaderParams())->distanceFarMinusClose = 200.0f;
	static_cast<TerrainShaderParams*>(owningEntity_->getGraphicsComponent()->getShaderParams())->closeDistance = 60.0f;
	static_cast<TerrainShaderParams*>(owningEntity_->getGraphicsComponent()->getShaderParams())->patchRadius = 40.0f;
	static_cast<TerrainShaderParams*>(owningEntity_->getGraphicsComponent()->getShaderParams())->maxTessellationWeight += dt * 1.3f;
	mainCamera_->calculateFrustumPlanes(viewProjection, static_cast<TerrainShaderParams*>(owningEntity_->getGraphicsComponent()->getShaderParams())->frustumPlanes);
}
