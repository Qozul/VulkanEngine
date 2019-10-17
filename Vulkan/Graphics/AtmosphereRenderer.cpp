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
#include "ShaderParams.h"
#include "RenderObject.h"
#include "Material.h"
#include "../Assets/Entity.h"
#include "../Game/SunScript.h"
#include "../Game/AtmosphereScript.h"

using namespace QZL;
using namespace Graphics;

struct PushConstantExtent {
	glm::mat4 inverseViewProj;
	glm::vec3 betaRay;
	float betaMie;
	glm::vec3 cameraPosition;
	float planetRadius;
	glm::vec3 sunDirection;
	float Hatm;
	glm::vec3 sunIntensity;
	float g;
};

AtmosphereRenderer::AtmosphereRenderer(LogicDevice* logicDevice, TextureManager* textureManager, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
	const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount, const GlobalRenderData* globalRenderData, 
	TextureSampler* geometryColourBuffer, TextureSampler* geometryDepthBuffer)
	: RendererBase(logicDevice), descriptor_(descriptor), geometryColourBuffer_(geometryColourBuffer), geometryDepthBuffer_(geometryDepthBuffer)
{
	ASSERT(entityCount > 0);
	renderStorage_ = new RenderStorage(new ElementBuffer<VertexOnlyPosition>(logicDevice->getDeviceMemory()), RenderStorage::InstanceUsage::ONE);

	auto gcbBinding = TextureSampler::makeBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT);
	auto gdbBinding = TextureSampler::makeBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayout layout = descriptor->makeLayout({ gcbBinding, gdbBinding });

	pipelineLayouts_.push_back(layout);
	pipelineLayouts_.push_back(AtmosphereMaterial::getLayout(descriptor));

	descriptorSets_.push_back(descriptor->getSet(descriptor->createSets({ layout })));

	auto pushConstRange = setupPushConstantRange<PushConstantExtent>(VK_SHADER_STAGE_FRAGMENT_BIT);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

	PipelineCreateInfo pci = {};
	pci.enableDepthTest = VK_FALSE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = swapChainExtent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	createPipeline<VertexOnlyPosition>(logicDevice, renderPass, RendererPipeline::makeLayoutInfo(pipelineLayouts_.size(), pipelineLayouts_.data(), 1, &pushConstRange), stageInfos, pci,
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
	descWrites.push_back(geometryColourBuffer_->descriptorWrite(descriptorSets_[0], 0));
	descWrites.push_back(geometryDepthBuffer_->descriptorWrite(descriptorSets_[0], 1));
	descriptor_->updateDescriptorSets(descWrites);

	beginFrame(cmdBuffer);
	renderStorage_->buf()->bind(cmdBuffer, idx);

	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];

		auto perMeshParams = static_cast<AtmosphereShaderParams*>(robject->getParams());
		AtmosphereShaderParams::Params params = perMeshParams->params;

		PushConstantExtent pce;
		pce.inverseViewProj = glm::inverse(GraphicsMaster::kProjectionMatrix * viewMatrix);
		pce.cameraPosition = glm::vec3(0.0f, 1.0f, 0.0f); // TODO actual position
		pce.sunDirection = *params.sunDirection;
		pce.sunIntensity = *params.sunIntensity;
		pce.betaMie = params.betaMie;
		pce.betaRay = params.betaRay;
		pce.g = params.g;
		pce.Hatm = params.Hatm;
		pce.planetRadius = params.planetRadius;
		
		VkDescriptorSet sets[2] = { descriptorSets_[0], robject->getMaterial()->getTextureSet() };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 2, sets, 0, nullptr);

		vkCmdPushConstants(cmdBuffer, pipeline_->getLayout(), pushConstantInfos_[0].stages, pushConstantInfos_[0].offset, pushConstantInfos_[0].size, &pce);
		vkCmdPipelineBarrier(cmdBuffer, pushConstantInfos_[0].stages, pushConstantInfos_[0].stages, VK_DEPENDENCY_BY_REGION_BIT, 1, &pushConstantInfos_[0].barrier, 0, nullptr, 0, nullptr);

		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}

void AtmosphereRenderer::initialise(const glm::mat4& viewMatrix)
{
	/*if (renderStorage_->instanceCount() == 0)
		return;
	auto instPtr = renderStorage_->instanceData();
	auto params = static_cast<AtmosphereShaderParams*>((*(instPtr))->getShaderParams()); 
	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(params->textures.scatteringSum->descriptorWrite(descriptorSets_[0], 0));
	descriptor_->updateDescriptorSets(descWrites);*/
}
