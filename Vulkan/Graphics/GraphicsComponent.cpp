// Author: Ralph Ridley
// Date: 01/11/19
#include "GraphicsComponent.h"
#include "ShaderParams.h"
#include "RenderObject.h"
#include "Material.h"
#include "../Assets/Entity.h"
#include "Mesh.h"

using namespace QZL;
using namespace Graphics;

GraphicsComponent::GraphicsComponent(Entity* owner, RendererTypes type, ShaderParams* perMeshParams, ShaderParams* perInstanceParams,
	const std::string& meshName, MeshLoadFunc loadFunc, Material* material, bool overrideChecks)
	: rtype_(type), owningEntity_(owner), meshParameters_(perMeshParams), instanceParameters_(perInstanceParams),
	meshName_(meshName), loadFunc_(loadFunc), material_(material)
{
}

GraphicsComponent::GraphicsComponent(Entity* owner, RendererTypes type, RenderObject* robject, ShaderParams* perInstanceParams)
	: rtype_(type), owningEntity_(owner), meshParameters_(robject->getParams()), instanceParameters_(perInstanceParams),
	meshName_(robject->getMeshName()), loadFunc_(nullptr), material_(robject->getMaterial()), mesh_(robject->getMesh())
{
}

GraphicsComponent::~GraphicsComponent()
{
	SAFE_DELETE(instanceParameters_);
}

std::string GraphicsComponent::getParamsId()
{
	return "";
}

glm::mat4 GraphicsComponent::getModelmatrix()
{
	return owningEntity_->getModelMatrix();
}
