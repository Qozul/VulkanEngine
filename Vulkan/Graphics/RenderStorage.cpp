#include "RenderStorage.h"
#include "GraphicsComponent.h"
#include "DeviceMemory.h"
#include "ElementBuffer.h"
#include "DynamicVertexBuffer.h"

using namespace QZL;
using namespace QZL::Graphics;

RenderStorage::RenderStorage(ElementBufferInterface* buffer)
	: bufferType_(buffer->bufferType())
{
	buf_.elementBuffer = buffer;
}

RenderStorage::RenderStorage(VertexBufferInterface* buffer)
	: bufferType_(buffer->bufferType())
{
	buf_.vertexBuffer = buffer;
}

RenderStorage::~RenderStorage()
{
	switch (bufferType_) {
	case BufferType::ELEMENT:
		SAFE_DELETE(buf_.elementBuffer);
		break;
	case BufferType::VERTEX:
		SAFE_DELETE(buf_.vertexBuffer);
		break;
	}
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
		if (bufferType_ == BufferType::ELEMENT) {
			meshes_.emplace_back(mesh->count, 0, mesh->indexOffset, mesh->vertexOffset, index);
		}
		else {
			meshes_.emplace_back(mesh->count, 0, 0, mesh->vertexOffset, index);
		}
		addInstance(meshes_[dataMap_[name]], instance, index);
	}
}

void RenderStorage::addInstance(DrawElementsCommand& cmd, GraphicsComponent* instance, uint32_t index)
{
	instances_.insert(instances_.begin() + index, instance);
	cmd.instanceCount++;
}
