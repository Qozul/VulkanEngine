#include "Entity.h"
#include "Transform.h"
#include "../Graphics/Material.h"
#include "../Game/GameScript.h"

using namespace QZL;
using namespace QZL::Assets;

void Entity::update(float dt, const glm::mat4& parentMatrix)
{
	if (gameScript_ != nullptr) {
		gameScript_->update(dt, parentMatrix);
	}
}

void Entity::start()
{
	if (gameScript_ != nullptr) {
		gameScript_->start();
	}
}

void Entity::setGraphicsComponent(const Graphics::RendererTypes rtype, Graphics::ShaderParams* perMeshParams, Graphics::ShaderParams* perInstanceParams,
	Graphics::Material* material, const std::string& meshName, Graphics::MeshLoadFunc loadFunc, bool overrideChecks)
{
	graphicsComponent_ = new Graphics::GraphicsComponent(this, rtype, perMeshParams, perInstanceParams, meshName, loadFunc, material, overrideChecks);
}

void Entity::setGraphicsComponent(const Graphics::RendererTypes rtype, Graphics::RenderObject* robject, Graphics::ShaderParams* perInstanceParams)
{
	graphicsComponent_ = new Graphics::GraphicsComponent(this, rtype, robject, perInstanceParams);
}

void Entity::setGameScript(Game::GameScript* script)
{
	gameScript_ = script;
}

bool Entity::isStatic() const
{
	return rigidBody_ == nullptr && gameScript_ == nullptr;
}

Entity::Entity(const std::string name) 
	: graphicsComponent_(nullptr), rigidBody_(nullptr), gameScript_(nullptr), transform_(new Transform()), 
	  collisionVolume_(nullptr), sceneNode_(nullptr), preserveChildrenOnDelete_(true), name_(name)
{
}

Entity::~Entity()
{
	delete transform_;
	SAFE_DELETE(graphicsComponent_);
	SAFE_DELETE(rigidBody_);
	SAFE_DELETE(gameScript_);
	SAFE_DELETE(collisionVolume_);
}
