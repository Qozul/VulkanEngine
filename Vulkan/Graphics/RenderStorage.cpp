#include "RenderStorage.h"
#include "ElementBuffer.h"
#include "GraphicsComponent.h"
#include "DeviceMemory.h"

#define NO_INSTANCES_INDEX std::numeric_limits<GLuint>::max()

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

void RenderStorage::addMesh(const std::string& name, BasicMesh* mesh)
{
	auto keyIt = dataMap_.find(name);
	if (keyIt != dataMap_.end()) {
		// This mesh already exists in the data so just update its data
		auto cmd = meshes_[dataMap_[name]];
		cmd.indexCount = mesh->indexCount;
		cmd.firstIndex = mesh->indexOffset;
		cmd.baseVertex = mesh->vertexOffset;
	}
	else {
		dataMap_[name] = meshes_.size();
		// An index is assigned when an instance has been added
		meshes_.emplace_back(mesh->indexCount, 0, mesh->indexOffset, mesh->vertexOffset, NO_INSTANCES_INDEX);
	}
}

void RenderStorage::addInstance(const std::string& name, GraphicsComponent* instance)
{
	auto& index = meshes_[dataMap_[name]].baseInstance;
	if (index == NO_INSTANCES_INDEX)
		index = instances_.size();

	instances_.insert(instances_.begin() + index, instance);
	meshes_[dataMap_[name]].instanceCount++;
}

void RenderStorage::modifyInstance(const std::string& name, const size_t instanceIndex, GraphicsComponent* instance)
{
	instances_[meshes_[dataMap_[name]].baseInstance + instanceIndex] = instance;
}

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
