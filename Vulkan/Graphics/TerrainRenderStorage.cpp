#include "TerrainRenderStorage.h"
#include "LogicDevice.h"
#include "TextureLoader.h"
#include "TerrainShaderParams.h"
#include "TextureSampler.h"

using namespace QZL::Graphics;

TerrainRenderStorage::TerrainRenderStorage(TextureLoader*& textureLoader, const LogicDevice* logicDevice)
	: RenderStorageMeshParams<TerrainParamData>(logicDevice->getDeviceMemory()), textureLoader_(textureLoader), logicDevice_(logicDevice)
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
	return { new TextureSampler(logicDevice_, params->getHeightmapName(), textureLoader_->loadTexture(params->getHeightmapName()),
					VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 8, 1),
				new TextureSampler(logicDevice_, params->getDebugDiffuseName(), textureLoader_->loadTexture(params->getDebugDiffuseName()),
					VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 8, 3) };
}


