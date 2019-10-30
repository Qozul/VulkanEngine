#include "ParticleRenderer.h"
#include "DynamicVertexBuffer.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "TextureSampler.h"
#include "ShaderParams.h"
#include "SwapChain.h"
#include "RenderObject.h"
#include "Material.h"
#include "../Assets/Entity.h"
#include "../Assets/Transform.h"

using namespace QZL;
using namespace QZL::Graphics;

struct PushConstantGeometry {
	glm::vec3 billboardPoint;
	float tileLength;
};

struct PerInstanceParams {
	glm::mat4 model;
	glm::mat4 mvp;
	glm::vec4 tint;
};

ParticleRenderer::ParticleRenderer(LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor, 
	const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader, const uint32_t particleSystemCount, const GlobalRenderData* globalRenderData,
	glm::vec3* billboardPoint)
	: RendererBase(logicDevice, new RenderStorage(new DynamicVertexBuffer<ParticleVertex>(logicDevice->getDeviceMemory(), 12, SwapChain::numSwapChainImages), RenderStorage::InstanceUsage::UNLIMITED)),
	  descriptor_(descriptor), billboardPoint_(billboardPoint)
{
	ASSERT(particleSystemCount > 0);
	createDescriptors(particleSystemCount);

	auto pushConstRange = setupPushConstantRange<PushConstantGeometry>(VK_SHADER_STAGE_GEOMETRY_BIT);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);
	stageInfos.emplace_back(geometryShader, VK_SHADER_STAGE_GEOMETRY_BIT, nullptr);

	PipelineCreateInfo pci = {};
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = swapChainExtent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	pci.subpassIndex = 1;

	createPipeline<ParticleVertex>(logicDevice, renderPass, RendererPipeline::makeLayoutInfo(pipelineLayouts_.size(), pipelineLayouts_.data(), 1, &pushConstRange), stageInfos, pci);
}

ParticleRenderer::~ParticleRenderer()
{
}

void ParticleRenderer::createDescriptors(const uint32_t particleSystemCount)
{
	DescriptorBuffer* instBuf = DescriptorBuffer::makeBuffer<UniformBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 0, 0,
		sizeof(PerInstanceParams) * particleSystemCount, VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	storageBuffers_.push_back(instBuf);

	VkDescriptorSetLayout layout = descriptor_->makeLayout({ instBuf->getBinding() });

	pipelineLayouts_.push_back(layout);
	pipelineLayouts_.push_back(ParticleMaterial::getLayout(descriptor_));

	descriptorSets_.push_back(descriptor_->getSet(descriptor_->createSets({ layout })));
	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(instBuf->descriptorWrite(descriptorSets_[0]));
	descriptor_->updateDescriptorSets(descWrites);
}

void ParticleRenderer::recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	beginFrame(cmdBuffer);
	dynamic_cast<DynamicBufferInterface*>(renderStorage_->buf())->updateBuffer(cmdBuffer, idx);
	dynamic_cast<VertexBufferInterface*>(renderStorage_->buf())->bind(cmdBuffer, idx);

	PerInstanceParams* eleDataPtr = static_cast<PerInstanceParams*>(storageBuffers_[0]->bindRange());
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		auto comp = (*(instPtr + i));
		auto params = static_cast<ParticleShaderParams*>(comp->getPerMeshShaderParams());
		glm::mat4 model = comp->getModelmatrix();
		eleDataPtr[i] = {
			model, GraphicsMaster::kProjectionMatrix * viewMatrix * model, params->params.tint
		};
	}
	storageBuffers_[0]->unbindRange();

	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];

		auto params = static_cast<ParticleShaderParams*>(robject->getParams());
		PushConstantGeometry pcg;
		pcg.billboardPoint = *billboardPoint_;
		pcg.tileLength = params->params.textureTileLength;
		
		VkDescriptorSet sets[2] = { descriptorSets_[0], robject->getMaterial()->getTextureSet() };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 2, sets, 0, nullptr);

		vkCmdPushConstants(cmdBuffer, pipeline_->getLayout(), pushConstantInfos_[0].stages, pushConstantInfos_[0].offset, pushConstantInfos_[0].size, &pcg);
		//vkCmdPipelineBarrier(cmdBuffer, pushConstantInfos_[0].stages, pushConstantInfos_[0].stages, VK_DEPENDENCY_BY_REGION_BIT, 1, &pushConstantInfos_[0].barrier, 0, nullptr, 0, nullptr);

		vkCmdDraw(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}
