#include "StaticShaderParams.h"
#include "TextureSampler.h"
#include "TextureLoader.h"

using namespace QZL;
using namespace QZL::Graphics;

StaticShaderParams::StaticShaderParams(TextureLoader* textureLoader, LogicDevice* logicDevice, const std::string& diffuseName, const std::string& normalMapName)
{
	diffuse_ = new TextureSampler(logicDevice, textureLoader->loadTexture(diffuseName), VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 8, 1);
	normalMap_ = new TextureSampler(logicDevice, textureLoader->loadTexture(normalMapName), VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 8, 3);
}

StaticShaderParams::~StaticShaderParams()
{
	SAFE_DELETE(diffuse_);
	SAFE_DELETE(normalMap_);
}
