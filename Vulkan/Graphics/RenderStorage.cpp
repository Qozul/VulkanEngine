#include "RenderStorage.h"
#include "GraphicsComponent.h"
#include "ElementBufferObject.h"
#include "RenderObject.h"

using namespace QZL;
using namespace QZL::Graphics;

RenderStorage::RenderStorage(ElementBufferObject* buffer)
	: buffer_(buffer)
{
}

RenderStorage::~RenderStorage()
{
	SAFE_DELETE(buffer_);
}

void RenderStorage::addMesh(GraphicsComponent* instance, RenderObject* robject)
{
	addMeshUnlimitedInstances(instance, robject);
}

void RenderStorage::addMeshUnlimitedInstances(GraphicsComponent* instance, RenderObject* robject)
{
	auto key = instance->getMeshName();
	auto keyIt = dataMap_.find(key);
	if (keyIt != dataMap_.end()) {
		auto& cmd = drawCmds_[dataMap_[key]];
		cmd.instanceCount++;
	}
	else {
		auto mesh = robject == nullptr ? MeshLoader::loadMesh(key, *buffer_, instance->getLoadInfo()) : robject->getMesh();
		dataMap_[key] = drawCmds_.size();

		if (buffer_->isIndexed()) {
			drawCmds_.emplace_back(mesh->count, 1, mesh->indexOffset, mesh->vertexOffset, 0);
		}
		else {
			drawCmds_.emplace_back(mesh->count, 1, 0, mesh->vertexOffset, 0);
		}
	}
	setBaseInstances();
}

void RenderStorage::setBaseInstances()
{
	size_t baseInstance = 0;
	for (auto& cmd : drawCmds_) {
		cmd.baseInstance = baseInstance;
		baseInstance += cmd.instanceCount;
	}
}
