 #include "GameMaster.h"
#include "../System.h"
#include "../Assets/AssetManager.h"
#include "../Graphics/GraphicsMaster.h"
#include "../Graphics/GraphicsComponent.h"
#include "../Graphics/StaticShaderParams.h"
#include "../Graphics/TerrainShaderParams.h"
#include "../Assets/Terrain.h"

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
	params.staticSP = new Graphics::StaticShaderParams("101", "102");
	testEntity->setGraphicsComponent(Graphics::RendererTypes::STATIC, params, "Teapot");
	masters_.graphicsMaster->registerComponent(testEntity->getGraphicsComponent());

	//Assets::Entity* terrain = masters_.assetManager->createEntity<Assets::Terrain>();
	//masters_.graphicsMaster->registerComponent(testEntity->getGraphicsComponent());
}
