#include "GraphicsComponent.h"
#include "StaticShaderParams.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace Graphics;

GraphicsComponent::~GraphicsComponent()
{
	SAFE_DELETE(instanceParameters_);
}

glm::mat4 GraphicsComponent::getModelmatrix()
{
	return owningEntity_->getModelMatrix();
}
