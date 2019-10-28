 #include "GameMaster.h"
#include "../System.h"
#include "../Assets/AssetManager.h"
#include "../Graphics/GraphicsMaster.h"
#include "../Graphics/GraphicsComponent.h"
#include "../Graphics/ShaderParams.h"
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
	std::vector<Assets::Entity*> entities;
	GameScriptInitialiser scriptInit;
	scriptInit.inputManager = masters_.inputManager;
	scriptInit.system = masters_.system;

	Assets::Entity* camera = masters_.assetManager->createEntity("mainCamera");
	entities.push_back(camera);
	scriptInit.owner = camera;
	camera->setGameScript(new Camera(scriptInit));

	Assets::Entity* teapot = masters_.assetManager->createEntity("Teapot");
	teapot->getTransform()->scale = glm::vec3(2.0f);
	teapot->setGraphicsComponent(Graphics::RendererTypes::STATIC, nullptr, new Graphics::StaticShaderParams(),
		masters_.assetManager->textureManager->requestMaterial<Graphics::StaticMaterial>("ExampleStatic"), "Teapot", Graphics::MeshLoadingInfo());

	Assets::Entity* terrain = masters_.assetManager->createEntity<Assets::Terrain>("terrain", masters_.assetManager->textureManager);
	entities.push_back(terrain);

	Assets::Entity* sun = masters_.assetManager->createEntity("sun");
	entities.push_back(sun);
	scriptInit.owner = sun;
	auto sunScript = new SunScript(scriptInit, masters_.graphicsMaster->getCamPosPtr(), masters_.graphicsMaster->getDynamicBuffer(Graphics::RendererTypes::PARTICLE));
	sun->setGameScript(sunScript);
	auto sunRobject = sunScript->makeRenderObject("SunSystem");
	sun->setGraphicsComponent(Graphics::RendererTypes::PARTICLE, sunRobject);

	Assets::Entity* skysphere = masters_.assetManager->createEntity<Assets::Skysphere>("sky", masters_.graphicsMaster->getLogicDevice(), sunScript, scriptInit);

	Assets::Entity* fire = masters_.assetManager->createEntity("firetest");
	entities.push_back(fire);
	scriptInit.owner = fire;
	auto fireScript = new FireSystem(scriptInit, masters_.graphicsMaster->getCamPosPtr(), masters_.graphicsMaster->getDynamicBuffer(Graphics::RendererTypes::PARTICLE));
	fire->setGameScript(fireScript);
	auto fireRobject = fireScript->makeRenderObject("FireSystem");
	fire->setGraphicsComponent(Graphics::RendererTypes::PARTICLE, fireRobject);

	auto cameraNode = scenes_[activeSceneIdx_]->addEntity(camera);
	scenes_[activeSceneIdx_]->addEntity(sun, camera, cameraNode);
	scenes_[activeSceneIdx_]->addEntity(skysphere, camera, cameraNode);
	scenes_[activeSceneIdx_]->addEntity(fire);
	scenes_[activeSceneIdx_]->addEntity(terrain);
	scenes_[activeSceneIdx_]->addEntity(teapot);
	scenes_[activeSceneIdx_]->start();

	DEBUG_LOG(scenes_[activeSceneIdx_]);

	masters_.graphicsMaster->registerComponent(terrain->getGraphicsComponent());
	masters_.graphicsMaster->registerComponent(sun->getGraphicsComponent(), sunRobject);
	masters_.graphicsMaster->registerComponent(skysphere->getGraphicsComponent());
	masters_.graphicsMaster->registerComponent(fire->getGraphicsComponent(), fireRobject);
	masters_.graphicsMaster->registerComponent(teapot->getGraphicsComponent());
}

void GameMaster::update(float dt)
{
	scenes_[activeSceneIdx_]->update(dt);
}
