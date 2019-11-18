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
	//mainCamera_->calculateFrustumPlanes(viewProjection, static_cast<TerrainShaderParams*>(owningEntity_->getGraphicsComponent()->getShaderParams())->frustumPlanes);
}
