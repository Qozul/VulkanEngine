#include "RenderStorage.h"
#include "GraphicsComponent.h"
#include "DeviceMemory.h"
#include "ElementBuffer.h"
#include "DynamicVertexBuffer.h"
#include "RenderObject.h"

using namespace QZL;
using namespace QZL::Graphics;

RenderStorage::RenderStorage(BufferInterface* buffer, InstanceUsage usage)
	: buffer_(buffer), usage_(usage)
{
}

RenderStorage::~RenderStorage()
{
	for (auto& obj : renderObjects_) {
		delete obj;
	}
	SAFE_DELETE(buffer_);
}

void RenderStorage::addMesh(GraphicsComponent* instance, RenderObject* robject)
{
	if (usage_ == InstanceUsage::ONE) {
		addMeshOneInstance(instance, robject);
	}
	else {
		addMeshUnlimitedInstances(instance, robject);
	}
}

void RenderStorage::addInstance(DrawElementsCommand& cmd, GraphicsComponent* instance, uint32_t index)
{
	instances_.insert(instances_.begin() + index, instance);
	cmd.instanceCount++;
}

void RenderStorage::addMeshOneInstance(GraphicsComponent* instance, RenderObject* robject)
{
	robject = robject == nullptr 
		? new RenderObject(static_cast<ElementBufferInterface*>(buffer_), instance->getMeshName(), instance->getPerMeshShaderParams(), instance->getLoadInfo(), instance->getMaterial())
		: robject;
	renderObjects_.push_back(robject);
	auto mesh = robject->getMesh();
	drawCmds_.emplace_back(mesh->count, 1, mesh->indexOffset, mesh->vertexOffset, 0);
	instances_.push_back(instance);
}

void RenderStorage::addMeshUnlimitedInstances(GraphicsComponent* instance, RenderObject* robject)
{
	auto paramsId = instance->getPerMeshShaderParams() != nullptr ? instance->getPerMeshShaderParams()->id : "";
	auto key = instance->getMeshName() + paramsId;

	auto keyIt = dataMap_.find(key);
	if (keyIt != dataMap_.end()) {
		auto& cmd = drawCmds_[dataMap_[key]];
		addInstance(cmd, instance, cmd.baseInstance);
	}
	else {
		robject = robject == nullptr
			? new RenderObject(static_cast<ElementBufferInterface*>(buffer_), instance->getMeshName(), instance->getPerMeshShaderParams(), instance->getLoadInfo(), instance->getMaterial())
			: robject;
		auto mesh = robject->getMesh();
		dataMap_[key] = drawCmds_.size();

		ASSERT(drawCmds_.size() == renderObjects_.size());
		renderObjects_.push_back(robject);

		auto index = instances_.size();
		if (buffer_->bufferType() & BufferFlags::ELEMENT) {
			drawCmds_.emplace_back(mesh->count, 0, mesh->indexOffset, mesh->vertexOffset, index);
		}
		else if (buffer_->bufferType() & BufferFlags::VERTEX) {
			drawCmds_.emplace_back(mesh->count, 0, 0, mesh->vertexOffset, index);
		}
		else {
			ASSERT(false);
		}
		addInstance(drawCmds_[dataMap_[key]], instance, index);
	}
}
