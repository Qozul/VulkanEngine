#include "PostProcessRenderer.h"
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

PostProcessRenderer::PostProcessRenderer(LogicDevice* logicDevice, TextureManager* textureManager, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
	const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount, const GlobalRenderData* globalRenderData,
	TextureSampler* geometryColourBuffer, TextureSampler* geometryDepthBuffer)
	: RendererBase(logicDevice, new RenderStorage(new ElementBuffer<VertexOnlyPosition>(logicDevice->getDeviceMemory()), RenderStorage::InstanceUsage::ONE)),
	descriptor_(descriptor), geometryColourBuffer_(geometryColourBuffer), geometryDepthBuffer_(geometryDepthBuffer)
{
	ASSERT(entityCount > 0);
	createDescriptors(entityCount);

	std::vector<Graphics::VertexOnlyPosition> vertices = { glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, -1.0f, 0.0f) };
	std::vector<uint16_t> indices = { 0, 1, 3, 2 };
	auto buf = static_cast<ElementBufferInterface*>(renderStorage_->buf());
	auto voffset = buf->addVertices(vertices.data(), vertices.size());
	auto ioffset = buf->addIndices(indices.data(), indices.size());
	buf->emplaceMesh("FullscreenQuad", indices.size(), ioffset, voffset);

	//auto pushConstRange = setupPushConstantRange<PushConstantExtent>(VK_SHADER_STAGE_FRAGMENT_BIT);

	std::array<VkSpecializationMapEntry, 2> specConstantEntries;
	specConstantEntries[0].constantID = 0;
	specConstantEntries[0].size = sizeof(float);
	specConstantEntries[0].offset = 0;
	specConstantEntries[1].constantID = 1;
	specConstantEntries[1].size = sizeof(float);
	specConstantEntries[1].offset = sizeof(float);
	std::array<float, 2> specConstantValues;
	specConstantValues[0] = GraphicsMaster::NEAR_PLANE_Z;
	specConstantValues[1] = GraphicsMaster::FAR_PLANE_Z;

	std::array<VkSpecializationInfo, 2> specConstants;
	specConstants[0].mapEntryCount = 0;
	specConstants[0].dataSize = 0;
	specConstants[1].mapEntryCount = static_cast<uint32_t>(specConstantEntries.size());
	specConstants[1].pMapEntries = specConstantEntries.data();
	specConstants[1].dataSize = sizeof(specConstantValues);
	specConstants[1].pData = specConstantValues.data();

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(vertexShader, VK_SHADER_STAGE_VERTEX_BIT, &specConstants[0]);
	stageInfos.emplace_back(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, &specConstants[1]);

	PipelineCreateInfo pci = {};
	pci.enableDepthTest = VK_FALSE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = swapChainExtent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	pci.subpassIndex = 0;

	createPipeline<VertexOnlyPosition>(logicDevice, renderPass, RendererPipeline::makeLayoutInfo(pipelineLayouts_.size(), pipelineLayouts_.data(), 0, nullptr), stageInfos, pci,
		RendererPipeline::PrimitiveType::QUADS);
}

PostProcessRenderer::~PostProcessRenderer()
{
}

void PostProcessRenderer::createDescriptors(const uint32_t entityCount)
{
	VkDescriptorSetLayoutBinding gcbBinding = TextureSampler::makeBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding gdbBinding = TextureSampler::makeBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayout layout = descriptor_->makeLayout({ gcbBinding, gdbBinding });

	pipelineLayouts_.push_back(layout);

	descriptorSets_.push_back(descriptor_->getSet(descriptor_->createSets({ layout })));

	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(geometryColourBuffer_->descriptorWrite(descriptorSets_[0], 0));
	descWrites.push_back(geometryDepthBuffer_->descriptorWrite(descriptorSets_[0], 1));
	descriptor_->updateDescriptorSets(descWrites);
}

void PostProcessRenderer::recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	beginFrame(cmdBuffer);
	renderStorage_->buf()->bind(cmdBuffer, idx);

	VkDescriptorSet sets[1] = { descriptorSets_[0] };
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 1, sets, 0, nullptr);

	vkCmdDrawIndexed(cmdBuffer, 4, 1, 0, 0, 0);
}
