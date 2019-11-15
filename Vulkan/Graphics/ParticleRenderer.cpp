// Author: Ralph Ridley
// Date: 01/11/19

#include "ParticleRenderer.h"
#include "DynamicElementBuffer.h"
#include "StorageBuffer.h"
#include "RenderStorage.h"
#include "GlobalRenderData.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "ShaderParams.h"
#include "RenderObject.h"
#include "Material.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace QZL::Graphics;

struct PushConstantGeometry {
	glm::vec3 billboardPoint;
	float tileLength = 0.0f;
};

struct PerInstanceParams {
	glm::mat4 model;
	glm::mat4 mvp;
	glm::vec4 tint;
};

ParticleRenderer::ParticleRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, new RenderStorage(new DynamicElementBuffer(createInfo.logicDevice->getDeviceMemory(), createInfo.swapChainImageCount, sizeof(ParticleVertex)),
	  RenderStorage::InstanceUsage::kUnlimited))
{
	descriptorSets_.push_back(createInfo.globalRenderData->getSet());
	createDescriptors(createInfo.maxDrawnEntities);
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());

	auto pushConstRange = setupPushConstantRange<PushConstantGeometry>(VK_SHADER_STAGE_GEOMETRY_BIT);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);
	stageInfos.emplace_back(createInfo.geometryShader, VK_SHADER_STAGE_GEOMETRY_BIT, nullptr);

	PipelineCreateInfo pci = {};
	pci.debugName = "Particle";
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	pci.subpassIndex = createInfo.subpassIndex;

	createPipeline<ParticleVertex>(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()),
		pipelineLayouts_.data(), 1, &pushConstRange), stageInfos, pci);
}

ParticleRenderer::~ParticleRenderer()
{
}

void ParticleRenderer::createDescriptors(const uint32_t particleSystemCount)
{
	DescriptorBuffer* instBuf = DescriptorBuffer::makeBuffer<UniformBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 0, 0,
		sizeof(PerInstanceParams) * particleSystemCount, VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, "ParticleInstancesBuffer");
	storageBuffers_.push_back(instBuf);
	DescriptorBuffer* diBuf = DescriptorBuffer::makeBuffer<StorageBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 1, 0,
		sizeof(uint32_t) * particleSystemCount, VK_SHADER_STAGE_FRAGMENT_BIT, "ParticleDIBuffer");
	storageBuffers_.push_back(diBuf);

	VkDescriptorSetLayout layout = descriptor_->makeLayout({ instBuf->getBinding(), diBuf->getBinding() });

	pipelineLayouts_.push_back(layout);

	auto setIdx = descriptor_->createSets({ layout, layout, layout });
	std::vector<VkWriteDescriptorSet> descWrites;
	for (int i = 0; i < 3; ++i) {
		descriptorSets_.push_back(descriptor_->getSet(setIdx + i));
		descWrites.push_back(instBuf->descriptorWrite(descriptor_->getSet(setIdx + i)));
		descWrites.push_back(diBuf->descriptorWrite(descriptor_->getSet(setIdx + i)));
	}
	descriptor_->updateDescriptorSets(descWrites);
}

void ParticleRenderer::recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	beginFrame(cmdBuffer);
	renderStorage_->buffer()->updateBuffer(cmdBuffer, idx);
	bindEBO(cmdBuffer, idx);

	PerInstanceParams* eleDataPtr = static_cast<PerInstanceParams*>(storageBuffers_[0]->bindRange());
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		auto comp = (*(instPtr + i));
		auto params = static_cast<ParticleShaderParams*>(comp->getPerMeshShaderParams());
		glm::mat4 model = comp->getModelmatrix();
		eleDataPtr[i] = {
			model, GraphicsMaster::kProjectionMatrix * camera.viewMatrix * model, params->tint
		};
	}
	storageBuffers_[0]->unbindRange();

	uint32_t* dataPtr = static_cast<uint32_t*>(storageBuffers_[1]->bindRange());
	dataPtr[0] = 7;
	dataPtr[1] = 9;
	storageBuffers_[1]->unbindRange();

	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];

		auto params = static_cast<ParticleShaderParams*>(robject->getParams());
		PushConstantGeometry pcg;
		pcg.billboardPoint = camera.position;
		pcg.tileLength = params->textureTileLength;
		
		VkDescriptorSet sets[2] = { descriptorSets_[1 + (size_t)idx], descriptorSets_[0] };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 2, sets, 0, nullptr);

		vkCmdPushConstants(cmdBuffer, pipeline_->getLayout(), VK_SHADER_STAGE_GEOMETRY_BIT, pushConstantInfos_[0].offset, pushConstantInfos_[0].size, &pcg);

		vkCmdDraw(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}
