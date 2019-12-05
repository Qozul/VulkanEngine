#include "Entity.h"
#include "Transform.h"
#include "../Graphics/Material.h"
#include "../Game/GameScript.h"

using namespace QZL;

void Entity::update(float dt, const glm::mat4& viewProjection, const glm::mat4& parentMatrix)
{
	if (gameScript_ != nullptr) {
		gameScript_->update(dt, viewProjection, parentMatrix);
	}
	if (updateFunc_ != nullptr) {
		updateFunc_(dt);
	}
}

void Entity::start()
{
	if (gameScript_ != nullptr) {
		gameScript_->start();
	}
}

void Entity::setGraphicsComponent(Graphics::GraphicsComponent* component)
{
	graphicsComponent_ = component;
	graphicsComponent_->owningEntity_ = this;
}

void Entity::setGameScript(Game::GameScript* script)
{
	gameScript_ = script;
	gameScript_->owningEntity_ = this;
}

void Entity::setGraphicsComponent(const Graphics::RendererTypes rtype, Graphics::ShaderParams* perMeshParams, Graphics::ShaderParams* perInstanceParams,
	Graphics::Material* material, const std::string& meshName, Graphics::MeshLoadFunc loadFunc)
{
	graphicsComponent_ = new Graphics::GraphicsComponent(this, rtype, perMeshParams, perInstanceParams, meshName, loadFunc, material);
}

void Entity::setGraphicsComponent(Graphics::RendererTypes rtype, Graphics::ShaderParams* params, const std::string& meshName, Graphics::Material* material)
{
	graphicsComponent_ = new Graphics::GraphicsComponent(this, rtype, params, meshName, material);
}

bool Entity::isStatic() const
{
	return gameScript_ == nullptr;
}

Entity::Entity(const std::string name)
	: graphicsComponent_(nullptr), gameScript_(nullptr), transform_(new Transform()), 
	  sceneNode_(nullptr), preserveChildrenOnDelete_(true), name_(name), updateFunc_(nullptr)
{
}

Entity::~Entity()
{
	delete transform_;
	SAFE_DELETE(graphicsComponent_);
	SAFE_DELETE(gameScript_);
}
