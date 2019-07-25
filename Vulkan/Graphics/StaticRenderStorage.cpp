#include "StaticRenderStorage.h"
#include "StaticShaderParams.h"
#include "TextureSampler.h"
#include "LogicDevice.h"
#include "TextureLoader.h"

using namespace QZL;
using namespace Graphics;

StaticRenderStorage::StaticRenderStorage(TextureLoader*& textureLoader, const LogicDevice* logicDevice)
	: RenderStorageMeshParams<StaticParamData>(logicDevice->getDeviceMemory()), logicDevice_(logicDevice), textureLoader_(textureLoader)
{
}

StaticRenderStorage::~StaticRenderStorage()
{
	for (auto paramData : paramData_) {
		delete paramData.diffuse;
		delete paramData.normalMap;
	}
}

StaticParamData StaticRenderStorage::resolveParams(GraphicsComponent* instance)
{
	auto params = static_cast<const StaticShaderParams*>(instance->getShaderParams());
	return { new TextureSampler(logicDevice_, params->getDiffuseName(), textureLoader_->loadTexture(params->getDiffuseName()),
					VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 8, 1),
				new TextureSampler(logicDevice_, params->getNormalMapName(), textureLoader_->loadTexture(params->getNormalMapName()),
					VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 8, 3) };
}
