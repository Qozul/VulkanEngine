#include "GraphicsComponent.h"
#include "StaticShaderParams.h"

using namespace QZL;
using namespace Graphics;

GraphicsComponent::~GraphicsComponent()
{
	if (rtype_ == Graphics::RendererTypes::STATIC)
		delete shaderParameters_.ssp;
}
