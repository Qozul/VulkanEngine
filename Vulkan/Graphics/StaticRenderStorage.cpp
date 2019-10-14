#include "StaticRenderStorage.h"
#include "StaticShaderParams.h"
#include "TextureSampler.h"
#include "LogicDevice.h"
#include "TextureManager.h"
#include "Descriptor.h"
/*
using namespace QZL;
using namespace Graphics;

StaticRenderStorage::StaticRenderStorage(TextureManager* textureManager, const LogicDevice* logicDevice, ElementBufferInterface* buffer)
	: RenderStorageMeshParams<StaticParamData>(buffer), logicDevice_(logicDevice), textureManager_(textureManager)
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
	return { 
		textureManager_->requestTextureSeparate(params->getDiffuseName(), VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 8),
		textureManager_->requestTextureSeparate(params->getNormalMapName(), VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 8)
	};
}*/
