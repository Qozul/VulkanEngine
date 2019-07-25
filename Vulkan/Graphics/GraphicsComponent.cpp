#include "GraphicsComponent.h"
#include "StaticShaderParams.h"

using namespace QZL;
using namespace Graphics;

GraphicsComponent::~GraphicsComponent()
{
	delete shaderParameters_;
}
