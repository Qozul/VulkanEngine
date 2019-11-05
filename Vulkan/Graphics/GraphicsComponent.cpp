// Author: Ralph Ridley
// Date: 01/11/19
#include "GraphicsComponent.h"
#include "ShaderParams.h"
#include "RenderObject.h"
#include "Material.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace Graphics;

GraphicsComponent::GraphicsComponent(Assets::Entity* owner, RendererTypes type, ShaderParams* perMeshParams, ShaderParams* perInstanceParams,
	const std::string& meshName, MeshLoadFunc loadFunc, Material* material)
	: rtype_(type), owningEntity_(owner), meshParameters_(perMeshParams), instanceParameters_(perInstanceParams),
	meshName_(meshName), loadFunc_(loadFunc), material_(material)
{
	if (material != nullptr) {
		ASSERT(type == material->getRendererType());
	}
	if (perInstanceParams != nullptr) {
		ASSERT(type == perInstanceParams->getRendererType());
	}
	if (perMeshParams != nullptr) {
		ASSERT(type == perMeshParams->getRendererType());
	}
}

GraphicsComponent::GraphicsComponent(Assets::Entity* owner, RendererTypes type, RenderObject* robject, ShaderParams* perInstanceParams)
	: rtype_(type), owningEntity_(owner), meshParameters_(robject->getParams()), instanceParameters_(perInstanceParams),
	meshName_(robject->getMeshName()), loadFunc_(nullptr), material_(robject->getMaterial())
{
	if (robject->getMaterial() != nullptr) {
		ASSERT(type == robject->getMaterial()->getRendererType());
	}
	if (perInstanceParams != nullptr) {
		ASSERT(type == perInstanceParams->getRendererType());
	}
	if (robject->getParams() != nullptr) {
		ASSERT(type == robject->getParams()->getRendererType());
	}
}

GraphicsComponent::~GraphicsComponent()
{
	SAFE_DELETE(instanceParameters_);
}

std::string GraphicsComponent::getParamsId()
{
	return meshParameters_ == nullptr ? "" : meshParameters_->id;
}

glm::mat4 GraphicsComponent::getModelmatrix()
{
	return owningEntity_->getModelMatrix();
}
