#include "Entity.h"
#include "Transform.h"

using namespace QZL;
using namespace QZL::Assets;

void Entity::setGraphicsComponent(const Graphics::RendererTypes rtype, const std::string& meshName, Graphics::ShaderParams shaderParams)
{
	graphicsComponent_ = new Graphics::GraphicsComponent(this, rtype, meshName, shaderParams);
}

bool Entity::isStatic() const
{
	return rigidBody_ == nullptr && gameScript_ == nullptr;
}

Entity::Entity() : graphicsComponent_(nullptr), rigidBody_(nullptr),
	gameScript_(nullptr), transform_(new Transform()), collisionVolume_(nullptr)
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
