#include "StaticRenderStorage.h"
#include "GraphicsComponent.h"
#include "StaticShaderParams.h"
#include "TextureSampler.h"
#include "LogicDevice.h"
#include "TextureLoader.h"

using namespace QZL;
using namespace Graphics;

StaticRenderStorage::StaticRenderStorage(TextureLoader*& textureLoader, const LogicDevice* logicDevice)
	: RenderStorage(logicDevice->getDeviceMemory()), logicDevice_(logicDevice), textureLoader_(textureLoader)
{
}

void StaticRenderStorage::addMesh(GraphicsComponent* instance, BasicMesh* mesh)
{
	auto params = instance->getShaderParams().ssp;
	auto fullName = std::make_tuple(params->getDiffuseName(), params->getNormalMapName(), instance->getMeshName());
	auto keyIt = texturedDataMap_.find(fullName);
	if (keyIt != texturedDataMap_.end()) {
		// This mesh already exists in the data so just add an instance
		auto& cmd = meshes_[texturedDataMap_[fullName]];
		addInstance(cmd, instance, cmd.baseInstance);
	}
	else {
		texturedDataMap_[fullName] = meshes_.size();
		auto index = instances_.size();
		meshes_.emplace_back(mesh->indexCount, 0, mesh->indexOffset, mesh->vertexOffset, index);
		diffuseTextures_.emplace_back(new TextureSampler(logicDevice_, params->getDiffuseName(), textureLoader_->loadTexture(params->getDiffuseName()), 
			VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 8, 1));
		normalMaps_.emplace_back(new TextureSampler(logicDevice_, params->getNormalMapName(), textureLoader_->loadTexture(params->getNormalMapName()),
			VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 8, 3));
		addInstance(meshes_[texturedDataMap_[fullName]], instance, index);
	}
}

void StaticRenderStorage::addInstance(DrawElementsCommand& cmd, GraphicsComponent* instance, uint32_t index)
{
	instances_.insert(instances_.begin() + index, instance);
	cmd.instanceCount++;
}
