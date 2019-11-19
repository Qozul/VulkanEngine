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
	static_cast<TerrainShaderParams*>(owningEntity_->getGraphicsComponent()->getShaderParams())->specularExponent = 1.0f;
	static_cast<TerrainShaderParams*>(owningEntity_->getGraphicsComponent()->getShaderParams())->distanceFarMinusClose = 300.0f;
	static_cast<TerrainShaderParams*>(owningEntity_->getGraphicsComponent()->getShaderParams())->closeDistance = 50.0f;
	static_cast<TerrainShaderParams*>(owningEntity_->getGraphicsComponent()->getShaderParams())->patchRadius = 40.0f;
	static_cast<TerrainShaderParams*>(owningEntity_->getGraphicsComponent()->getShaderParams())->maxTessellationWeight = 4.0f;
	mainCamera_->calculateFrustumPlanes(viewProjection, static_cast<TerrainShaderParams*>(owningEntity_->getGraphicsComponent()->getShaderParams())->frustumPlanes);
}
