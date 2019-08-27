 #include "GameMaster.h"
#include "../System.h"
#include "../Assets/AssetManager.h"
#include "../Graphics/GraphicsMaster.h"
#include "../Graphics/GraphicsComponent.h"
#include "../Graphics/StaticShaderParams.h"
#include "../Graphics/TerrainShaderParams.h"
#include "../Assets/Terrain.h"
#include "../Assets/Skysphere.h"
#include "../Graphics/Material.h"
#include "../Graphics/TextureManager.h"
#include "../Assets/AltAtmosphere.h"
#include "Camera.h"
#include "SunScript.h"
#include "Scene.h"

using namespace QZL;
using namespace Game;

GameMaster::GameMaster(const SystemMasters& masters) 
	: masters_(masters), activeSceneIdx_(0)
{
	scenes_.push_back(new Scene()); // Default scene
}

GameMaster::~GameMaster()
{
	for (auto i = 0; i < scenes_.size(); ++i) {
		SAFE_DELETE(scenes_[i]);
	}
}

void GameMaster::loadGame()
{
	Assets::Entity* camera = masters_.assetManager->createEntity();
	GameScriptInitialiser scriptInit;
	scriptInit.owner = camera;
	scriptInit.inputManager = masters_.inputManager;
	scriptInit.system = masters_.system;
	camera->setGameScript(new Camera(scriptInit));
	auto cameraNode = scenes_[activeSceneIdx_]->addEntity(camera);

	/*Assets::Entity* testEntity = masters_.assetManager->createEntity();
	if (masters_.graphicsMaster->supportsOptionalExtension(Graphics::OptionalExtensions::DESCRIPTOR_INDEXING)) {
		testEntity->setGraphicsComponent(Graphics::RendererTypes::STATIC, new Graphics::StaticShaderParams(
			"101", "102", 
			Graphics::MaterialStatic(glm::vec3(1.0f), glm::vec3(0.8f), 1.0f, 10.0f, 
				masters_.assetManager->textureManager->requestTexture("101"), 
				masters_.assetManager->textureManager->requestTexture("102"))), 
			"Teapot"
		);
	}
	else {
		testEntity->setGraphicsComponent(Graphics::RendererTypes::STATIC, new Graphics::StaticShaderParams(
			"101", "102", Graphics::MaterialStatic(glm::vec3(1.0f), glm::vec3(0.8f), 1.0f, 10.0f)), "Teapot"
		);
	}
	masters_.graphicsMaster->registerComponent(testEntity->getGraphicsComponent());*/

	Assets::Entity* terrain = masters_.assetManager->createEntity<Assets::Terrain>();
	masters_.graphicsMaster->registerComponent(terrain->getGraphicsComponent());
	scenes_[activeSceneIdx_]->addEntity(terrain);

	Assets::Entity* sun = masters_.assetManager->createEntity();
	scriptInit.owner = sun;
	auto sunScript = new SunScript(scriptInit);
	sun->setGameScript(sunScript);
	scenes_[activeSceneIdx_]->addEntity(sun);// , camera, cameraNode);

	Assets::Entity* skysphere = masters_.assetManager->createEntity<Assets::Skysphere>(masters_.graphicsMaster->getLogicDevice(), new Assets::Atmosphere(), sunScript);
	masters_.graphicsMaster->registerComponent(skysphere->getGraphicsComponent());
	scenes_[activeSceneIdx_]->addEntity(skysphere, camera, cameraNode);
}

void GameMaster::update(float dt)
{
	scenes_[activeSceneIdx_]->update(dt);
}
