#include "AtmosphereRenderer.h"
#include "ElementBuffer.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "TextureSampler.h"
#include "TextureManager.h"
#include "DeviceMemory.h"
#include "RendererPipeline.h"
#include "GraphicsComponent.h"
#include "SwapChain.h"
#include "AtmosphereShaderParams.h"
#include "../Assets/Entity.h"
#include "../Game/SunScript.h"
#include "../Game/AtmosphereScript.h"

using namespace QZL;
using namespace Graphics;

using PushConstantParams = MaterialAtmosphere;

struct PushConstantExtent {
	glm::mat4 inverseViewProj;
	MaterialAtmosphere mat;
};

AtmosphereRenderer::AtmosphereRenderer(LogicDevice* logicDevice, TextureManager* textureManager, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
	const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount, const GlobalRenderData* globalRenderData, 
	TextureSampler* geometryColourBuffer, TextureSampler* geometryDepthBuffer)
	: RendererBase(logicDevice), descriptor_(descriptor), geometryColourBuffer_(geometryColourBuffer), geometryDepthBuffer_(geometryDepthBuffer)
{
	ASSERT(entityCount > 0);
	renderStorage_ = new RenderStorageNoInstances(new ElementBuffer<VertexOnlyPosition>(logicDevice->getDeviceMemory()));

	auto scatteringBinding = TextureSampler::makeBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT);
	auto gcbBinding = TextureSampler::makeBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT);
	auto gdbBinding = TextureSampler::makeBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayout layout = descriptor->makeLayout({ scatteringBinding, gcbBinding, gdbBinding });

	pipelineLayouts_.push_back(layout);
	descriptorSets_.push_back(descriptor->getSet(descriptor->createSets({ layout })));

	std::vector<VkPushConstantRange> pushConstantRanges;
	pushConstantRanges.push_back(setupPushConstantRange<PushConstantExtent>(VK_SHADER_STAGE_FRAGMENT_BIT));

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

	PipelineCreateInfo pci = {};
	pci.enableDepthTest = VK_FALSE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = swapChainExtent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	createPipeline<VertexOnlyPosition>(logicDevice, renderPass, RendererPipeline::makeLayoutInfo(pipelineLayouts_.size(), pipelineLayouts_.data()), stageInfos, pci,
		RendererPipeline::PrimitiveType::QUADS);
}

AtmosphereRenderer::~AtmosphereRenderer()
{
}

void AtmosphereRenderer::recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;

	// Need to update each frame due to layout transfer? TODO do I really or will the write persist as long as the state is correct when I access it?
	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(geometryColourBuffer_->descriptorWrite(descriptorSets_[0], 1));
	descWrites.push_back(geometryDepthBuffer_->descriptorWrite(descriptorSets_[0], 2));
	descriptor_->updateDescriptorSets(descWrites);

	beginFrame(cmdBuffer);
	renderStorage_->buf()->bind(cmdBuffer, idx);

	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		auto params = static_cast<AtmosphereShaderParams*>((*(renderStorage_->instanceData()) + i)->getShaderParams());
		auto material = params->material;
		material.cameraPosition = glm::vec3(0.0f, 1.0f, 0.0f);
		material.sunDirection = params->sunScript->getSunDirection();
		material.sunIntensity = params->sunScript->getSunIntensity();

		PushConstantExtent pce;
		pce.mat = material;
		pce.inverseViewProj = glm::inverse(GraphicsMaster::kProjectionMatrix * viewMatrix);

		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 1, &descriptorSets_[0], 0, nullptr);

		vkCmdPushConstants(cmdBuffer, pipeline_->getLayout(), pushConstantInfos_[0].stages, pushConstantInfos_[0].offset, pushConstantInfos_[0].size, &pce);
		vkCmdPipelineBarrier(cmdBuffer, pushConstantInfos_[0].stages, pushConstantInfos_[0].stages, VK_DEPENDENCY_BY_REGION_BIT, 1, &pushConstantInfos_[0].barrier, 0, nullptr, 0, nullptr);

		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}

void AtmosphereRenderer::initialise(const glm::mat4& viewMatrix)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	auto instPtr = renderStorage_->instanceData();
	auto params = static_cast<AtmosphereShaderParams*>((*(instPtr))->getShaderParams()); 
	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(params->textures.scatteringSum->descriptorWrite(descriptorSets_[0], 0));
	descriptor_->updateDescriptorSets(descWrites);
}
