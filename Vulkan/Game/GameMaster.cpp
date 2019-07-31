 #include "GameMaster.h"
#include "../System.h"
#include "../Assets/AssetManager.h"
#include "../Graphics/GraphicsMaster.h"
#include "../Graphics/GraphicsComponent.h"
#include "../Graphics/StaticShaderParams.h"
#include "../Graphics/TerrainShaderParams.h"
#include "../Assets/Terrain.h"
#include "Camera.h"

using namespace QZL;
using namespace Game;

GameMaster::GameMaster(const SystemMasters& masters) 
	: masters_(masters)
{
}

void GameMaster::loadGame()
{
	Assets::Entity* camera = masters_.assetManager->createEntity();
	GameScriptInitialiser cameraInit;
	cameraInit.owner = camera;
	cameraInit.inputManager = masters_.inputManager;
	cameraInit.system = masters_.system;
	camera->setGameScript(new Camera(cameraInit));
	gameScripts_.push_back(camera->getGameScript());

	Assets::Entity* testEntity = masters_.assetManager->createEntity();
	testEntity->setGraphicsComponent(Graphics::RendererTypes::STATIC, new Graphics::StaticShaderParams("101", "102"), "Teapot");
	masters_.graphicsMaster->registerComponent(testEntity->getGraphicsComponent());

	//Assets::Entity* terrain = masters_.assetManager->createEntity<Assets::Terrain>();
	//masters_.graphicsMaster->registerComponent(terrain->getGraphicsComponent());
}

void GameMaster::update(float dt)
{
	for (auto script : gameScripts_) {
		script->update(dt);
	}
}
