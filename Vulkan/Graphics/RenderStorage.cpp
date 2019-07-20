#include "RenderStorage.h"
#include "ElementBuffer.h"
#include "GraphicsComponent.h"
#include "DeviceMemory.h"

using namespace QZL;
using namespace QZL::Graphics;

RenderStorage::RenderStorage(DeviceMemory* deviceMemory) 
	: buf_(new ElementBuffer(deviceMemory))
{
}

RenderStorage::~RenderStorage()
{
	SAFE_DELETE(buf_);
}

void RenderStorage::addMesh(GraphicsComponent* instance, BasicMesh* mesh)
{
	auto name = instance->getMeshName();
	auto keyIt = dataMap_.find(name);
	if (keyIt != dataMap_.end()) {
		// This mesh already exists in the data so just add an instance
		auto& cmd = meshes_[dataMap_[name]];
		addInstance(cmd, instance, cmd.baseInstance);
	}
	else {
		dataMap_[name] = meshes_.size();
		auto index = instances_.size();
		meshes_.emplace_back(mesh->indexCount, 0, mesh->indexOffset, mesh->vertexOffset, index);
		addInstance(meshes_[dataMap_[name]], instance, index);
	}
}

void RenderStorage::addInstance(DrawElementsCommand& cmd, GraphicsComponent* instance, uint32_t index)
{
	instances_.insert(instances_.begin() + index, instance);
	cmd.instanceCount++;
}

/*void RenderStorage::modifyInstance(DrawElementsCommand& cmd, const size_t instanceIndex, GraphicsComponent* instance)
{
	instances_[cmd.baseInstance + instanceIndex] = instance;
}*/

DrawElementsCommand* RenderStorage::meshData()
{
	return meshes_.data();
}
GraphicsComponent** RenderStorage::instanceData()
{
	return instances_.data();
}

size_t RenderStorage::meshCount()
{
	return meshes_.size();
}

size_t RenderStorage::instanceCount()
{
	return instances_.size();
}

ElementBuffer* RenderStorage::buf()
{
	return buf_;
}
