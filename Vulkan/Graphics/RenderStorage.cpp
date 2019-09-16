#include "RenderStorage.h"
#include "GraphicsComponent.h"
#include "DeviceMemory.h"
#include "ElementBuffer.h"
#include "DynamicVertexBuffer.h"

using namespace QZL;
using namespace QZL::Graphics;

RenderStorage::RenderStorage(BufferInterface* buffer)
	: buffer_(buffer)
{
}

RenderStorage::~RenderStorage()
{
	SAFE_DELETE(buffer_);
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
		if (buffer_->bufferType() & BufferFlags::ELEMENT) {
			meshes_.emplace_back(mesh->count, 0, mesh->indexOffset, mesh->vertexOffset, index);
		}
		else if (buffer_->bufferType() & BufferFlags::VERTEX) {
			meshes_.emplace_back(mesh->count, 0, 0, mesh->vertexOffset, index);
		}
		else {
			ASSERT(false);
		}
		addInstance(meshes_[dataMap_[name]], instance, index);
	}
}

void RenderStorage::addInstance(DrawElementsCommand& cmd, GraphicsComponent* instance, uint32_t index)
{
	instances_.insert(instances_.begin() + index, instance);
	cmd.instanceCount++;
}
