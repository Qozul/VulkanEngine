#include "RenderStorage.h"
#include "GraphicsComponent.h"
#include "ElementBufferObject.h"
#include "RenderObject.h"

using namespace QZL;
using namespace QZL::Graphics;

RenderStorage::RenderStorage(ElementBufferObject* buffer, InstanceUsage usage)
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
		? new RenderObject(buffer_, instance->getMeshName(), instance->getPerMeshShaderParams(), instance->getLoadInfo(), instance->getMaterial())
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
			? new RenderObject(buffer_, instance->getMeshName(), instance->getPerMeshShaderParams(), instance->getLoadInfo(), instance->getMaterial())
			: robject;
		auto mesh = robject->getMesh();
		dataMap_[key] = drawCmds_.size();

		ASSERT(drawCmds_.size() == renderObjects_.size());
		renderObjects_.push_back(robject);

		uint32_t index = static_cast<uint32_t>(instances_.size());
		if (buffer_->isIndexed()) {
			drawCmds_.emplace_back(mesh->count, 0, mesh->indexOffset, mesh->vertexOffset, index);
		}
		else {
			drawCmds_.emplace_back(mesh->count, 0, 0, mesh->vertexOffset, index);
		}
		addInstance(drawCmds_[dataMap_[key]], instance, index);
	}
}
