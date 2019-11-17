 #include "GameMaster.h"
#include "../System.h"
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
#include "../Graphics/GraphicsTypes.h"
#include "../Graphics/SceneDescriptorInfo.h"

#include "../Graphics/StorageBuffer.h"
#include "../Graphics/Descriptor.h"
#include "../Graphics/PhysicalDevice.h"
#include "../Graphics/LogicDevice.h"
#include "../Graphics/RenderObject.h"

using namespace QZL;
using namespace Game;

GameMaster::GameMaster(const SystemMasters& masters) 
	: masters_(masters), activeSceneIdx_(0)
{
	scenes_.push_back(new Scene(&masters)); // Default scene
}

GameMaster::~GameMaster()
{
	for (auto i = 0; i < scenes_.size(); ++i) {
		SAFE_DELETE(scenes_[i]);
	}
}

Graphics::SceneGraphicsInfo* GameMaster::loadDescriptors()
{
	return scenes_[activeSceneIdx_]->createDescriptors(3, masters_.graphicsMaster->details_.physicalDevice->getDeviceLimits());
}

void GameMaster::loadGame()
{
	GameScriptInitialiser scriptInit;
	scriptInit.inputManager = masters_.inputManager;
	scriptInit.system = masters_.system;

	Entity* camera = new Entity("mainCamera");
	scriptInit.owner = camera;
	camera->setGameScript(new Camera(masters_));

	Entity* teapot = new Entity("Teapot");
	teapot->getTransform()->scale = glm::vec3(2.0f);
	teapot->setGraphicsComponent(Graphics::RendererTypes::kStatic, nullptr, new Graphics::StaticShaderParams(),
		masters_.textureManager->requestMaterial(Graphics::RendererTypes::kStatic, "ExampleStatic"), "Teapot");
	
	Entity* terrain = new Terrain("terrain", masters_.textureManager);

	Entity* sun = new Entity("sun");
	scriptInit.owner = sun;
	auto sunScript = new SunScript(masters_);
	sun->setGameScript(sunScript);
	auto sunRobject = sunScript->makeRenderObject("SunSystem");
	sun->setGraphicsComponent(Graphics::RendererTypes::kParticle, sunRobject, sunRobject->getParams());

	Skysphere* skysphere = new Skysphere("sky", masters_.getLogicDevice(), sunScript, scriptInit);

	Entity* fire = new Entity("firetest");
	scriptInit.owner = fire;
	auto fireScript = new FireSystem(masters_);
	fire->setGameScript(fireScript);
	auto fireRobject = fireScript->makeRenderObject("FireSystem");
	fire->setGraphicsComponent(Graphics::RendererTypes::kParticle, fireRobject, fireRobject->getParams());

	auto cameraNode = scenes_[activeSceneIdx_]->addEntity(camera);
	scenes_[activeSceneIdx_]->addEntity(sun, camera, cameraNode);
	scenes_[activeSceneIdx_]->addEntity(skysphere, camera, cameraNode);
	scenes_[activeSceneIdx_]->addEntity(fire);
	scenes_[activeSceneIdx_]->addEntity(terrain);
	scenes_[activeSceneIdx_]->addEntity(teapot);

	DEBUG_LOG(scenes_[activeSceneIdx_]);
}

std::vector<VkDrawIndexedIndirectCommand>* GameMaster::update(glm::mat4& viewProjection, float dt, const uint32_t& frameIdx)
{
	return scenes_[activeSceneIdx_]->update(viewProjection, dt, frameIdx);
}

void GameMaster::start()
{
	scenes_[activeSceneIdx_]->start();
}
