#include "TerrainRenderStorage.h"
#include "LogicDevice.h"
#include "TextureManager.h"
#include "TerrainShaderParams.h"
#include "TextureSampler.h"
#include "Descriptor.h"

using namespace QZL::Graphics;

TerrainRenderStorage::TerrainRenderStorage(TextureManager* textureManager, const LogicDevice* logicDevice, ElementBufferInterface* buffer)
	: RenderStorageMeshParams<TerrainParamData>(buffer), textureManager_(textureManager), logicDevice_(logicDevice)
{
}

TerrainRenderStorage::~TerrainRenderStorage()
{
	for (auto paramData : paramData_) {
		delete paramData.heightMap;
		delete paramData.diffuse;
	}
}

TerrainParamData TerrainRenderStorage::resolveParams(GraphicsComponent* instance)
{
	auto params = static_cast<const TerrainShaderParams*>(instance->getShaderParams());
	return {
		textureManager_->requestTextureSeparate(params->getHeightmapName(), VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 8),
		textureManager_->requestTextureSeparate(params->getDebugDiffuseName(), VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 8)
	};
}


