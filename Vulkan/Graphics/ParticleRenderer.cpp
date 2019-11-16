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
#include "SceneDescriptorInfo.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace QZL::Graphics;

ParticleRenderer::ParticleRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, new RenderStorage(new DynamicElementBuffer(createInfo.logicDevice->getDeviceMemory(), createInfo.swapChainImageCount, sizeof(ParticleVertex)),
	  RenderStorage::InstanceUsage::kUnlimited))
{
	pipelineLayouts_.push_back(createInfo.graphicsInfo->layout);
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());
	descriptorSets_.push_back(createInfo.graphicsInfo->set);
	descriptorSets_.push_back(createInfo.globalRenderData->getSet());
	storageBuffers_.push_back(createInfo.graphicsInfo->mvpBuffer);
	storageBuffers_.push_back(createInfo.graphicsInfo->paramsBuffer);
	storageBuffers_.push_back(createInfo.graphicsInfo->materialBuffer);

	VkPushConstantRange pushConstants[2] = {
		setupPushConstantRange<CameraPushConstants>(VK_SHADER_STAGE_VERTEX_BIT),
		setupPushConstantRange<TessellationPushConstants>(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
	};

	uint32_t offsets[3] = { graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kParticle], graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kParticle], graphicsInfo_->materialOffsetSizes[(size_t)RendererTypes::kParticle] };
	std::vector<VkSpecializationMapEntry> mapEntry = {
		makeSpecConstantEntry(0, 0,	sizeof(uint32_t)),
		makeSpecConstantEntry(1, sizeof(uint32_t), sizeof(uint32_t))
	};
	auto geomSpecConstant = setupSpecConstants(2, mapEntry.data(), sizeof(uint32_t) * 2, &offsets[0]);
	auto fragSpecConstant = setupSpecConstants(2, mapEntry.data(), sizeof(uint32_t) * 2, &offsets[1]);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, &fragSpecConstant);
	stageInfos.emplace_back(createInfo.geometryShader, VK_SHADER_STAGE_GEOMETRY_BIT, &geomSpecConstant);

	PipelineCreateInfo pci = {};
	pci.debugName = "Particle";
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	pci.subpassIndex = createInfo.subpassIndex;

	createPipeline<ParticleVertex>(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()),
		pipelineLayouts_.data(), 2, pushConstants), stageInfos, pci);
}

ParticleRenderer::~ParticleRenderer()
{
}

void ParticleRenderer::createDescriptors(const uint32_t particleSystemCount)
{
	/*DescriptorBuffer* instBuf = DescriptorBuffer::makeBuffer<UniformBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 0, 0,
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
	descriptor_->updateDescriptorSets(descWrites);*/
}

void ParticleRenderer::recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	beginFrame(cmdBuffer);
	renderStorage_->buffer()->updateBuffer(cmdBuffer, idx);
	bindEBO(cmdBuffer, idx);

	const uint32_t dynamicOffsets[3] = {
		graphicsInfo_->mvpRange * idx,
		graphicsInfo_->paramsRange * idx,
		graphicsInfo_->materialRange * idx
	};

	glm::mat4* eleDataPtr = (glm::mat4*)(static_cast<char*>(storageBuffers_[0]->bindRange()) + sizeof(glm::mat4) * graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kParticle] + dynamicOffsets[0]);
	ParticleShaderParams* paramsPtr = (ParticleShaderParams*)(static_cast<char*>(storageBuffers_[1]->bindRange()) +
		sizeof(ParticleShaderParams) * graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kParticle] + dynamicOffsets[1]);
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		auto comp = (*(instPtr + i));
		auto params = static_cast<ParticleShaderParams*>(comp->getPerMeshShaderParams());
		glm::mat4 model = comp->getModelmatrix();
		eleDataPtr[i] = {
			GraphicsMaster::kProjectionMatrix * camera.viewMatrix * model
		};
		paramsPtr[i] = {
			model, params->tint
		};
	}
	storageBuffers_[1]->unbindRange();
	storageBuffers_[0]->unbindRange();
	
	uint32_t* dataPtr = (uint32_t*)((char*)(storageBuffers_[2]->bindRange()) + sizeof(Materials::Particle) * graphicsInfo_->materialOffsetSizes[(size_t)RendererTypes::kParticle] + dynamicOffsets[2]);
	dataPtr[0] = 7;
	dataPtr[1] = 9;
	storageBuffers_[2]->unbindRange();
	
	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];

		vkCmdDraw(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.baseVertex, i);
	}
}
