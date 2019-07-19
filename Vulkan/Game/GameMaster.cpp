 #include "GameMaster.h"
#include "../System.h"
#include "../Assets/AssetManager.h"
#include "../Graphics/GraphicsMaster.h"
#include "../Graphics/GraphicsComponent.h"
#include "../Graphics/StaticShaderParams.h"

using namespace QZL;
using namespace Game;

GameMaster::GameMaster(const SystemMasters& masters) 
	: masters_(masters)
{
}

void GameMaster::loadGame()
{
	Assets::Entity* testEntity = masters_.assetManager->createEntity();
	Graphics::ShaderParams params;
	params.ssp = new Graphics::StaticShaderParams(masters_.assetManager->textureLoader, masters_.graphicsMaster->details_.logicDevice, "101", "102");
	testEntity->setGraphicsComponent(Graphics::RendererTypes::STATIC, "Teapot", params);
	masters_.graphicsMaster->registerComponent(testEntity->getGraphicsComponent());
}
