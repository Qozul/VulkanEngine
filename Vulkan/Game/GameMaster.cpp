 #include "GameMaster.h"
#include "../System.h"
#include "../Assets/AssetManager.h"
#include "../Graphics/GraphicsMaster.h"
#include "../Graphics/GraphicsComponent.h"
#include "../Graphics/StaticShaderParams.h"
#include "../Graphics/ParticleShaderParams.h"
#include "../Graphics/TerrainShaderParams.h"
#include "../Assets/Terrain.h"
#include "../Assets/Skysphere.h"
#include "../Graphics/Material.h"
#include "../Graphics/TextureManager.h"
#include "../Assets/AltAtmosphere.h"
#include "Camera.h"
#include "SunScript.h"
#include "AtmosphereScript.h"
#include "Scene.h"
#include "FireSystem.h"

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
	GameScriptInitialiser scriptInit;
	scriptInit.inputManager = masters_.inputManager;
	scriptInit.system = masters_.system;

	Assets::Entity* camera = masters_.assetManager->createEntity();
	scriptInit.owner = camera;
	camera->setGameScript(new Camera(scriptInit));

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
	scenes_[activeSceneIdx_]->addEntity(terrain);

	Assets::Entity* sun = masters_.assetManager->createEntity();
	scriptInit.owner = sun;
	auto sunScript = new SunScript(scriptInit, nullptr, nullptr);
	sun->setGameScript(sunScript);

	Assets::Entity* skysphere = masters_.assetManager->createEntity<Assets::Skysphere>(masters_.graphicsMaster->getLogicDevice(), sunScript, scriptInit);

	Assets::Entity* fire = masters_.assetManager->createEntity();
	scriptInit.owner = fire;
	auto fireScript = new FireSystem(scriptInit, masters_.graphicsMaster->getCamPosPtr(), masters_.graphicsMaster->getDynamicBuffer(Graphics::RendererTypes::PARTICLE));
	fire->setGameScript(fireScript);
	fire->setGraphicsComponent(Graphics::RendererTypes::PARTICLE, new Graphics::ParticleShaderParams(fireScript->getMaterial()), "FireSystem");

	auto cameraNode = scenes_[activeSceneIdx_]->addEntity(camera);
	scenes_[activeSceneIdx_]->addEntity(sun, camera, cameraNode);
	scenes_[activeSceneIdx_]->addEntity(skysphere, camera, cameraNode);
	scenes_[activeSceneIdx_]->addEntity(fire);
	scenes_[activeSceneIdx_]->start();

	masters_.graphicsMaster->registerComponent(terrain->getGraphicsComponent());
	masters_.graphicsMaster->registerComponent(skysphere->getGraphicsComponent());
	masters_.graphicsMaster->registerComponent(fire->getGraphicsComponent(), fireScript->makeMesh());
}

void GameMaster::update(float dt)
{
	scenes_[activeSceneIdx_]->update(dt);
}
