#include "PostProcessRenderer.h"
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
#include "TextureManager.h"
#include "../Assets/Entity.h"
#include "../Game/SunScript.h"
#include "../Game/AtmosphereScript.h"

using namespace QZL;
using namespace QZL::Graphics;

PostProcessRenderer::PostProcessRenderer(LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent,
	Descriptor* descriptor, const std::string& vertexShader, const std::string& fragmentShader, Image* apScatteringTex,
	Image* apTransmittanceTex, TextureSampler* colourTex, TextureSampler* depthTex)
	: RendererBase(logicDevice, new RenderStorage(new ElementBuffer<VertexOnlyPosition>(logicDevice->getDeviceMemory()), RenderStorage::InstanceUsage::ONE)), descriptor_(descriptor)
{
	apScatteringTexture_ = new TextureSampler(logicDevice, "ApScatteringSampler", apScatteringTex, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);
	apTransmittanceTexture_ = new TextureSampler(logicDevice, "ApTransmittanceSampler", apTransmittanceTex, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	auto scatteringbinding = TextureSampler::makeBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT);
	auto transmittancebinding = TextureSampler::makeBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT);
	auto colourbinding = TextureSampler::makeBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT);
	auto depthbinding = TextureSampler::makeBinding(3, VK_SHADER_STAGE_FRAGMENT_BIT);

	auto layout = descriptor->makeLayout({ scatteringbinding, transmittancebinding, colourbinding, depthbinding });
	pipelineLayouts_.push_back(layout);

	auto setIdx = descriptor->createSets({ layout });
	descriptorSets_.push_back(descriptor->getSet(setIdx));
	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(apScatteringTexture_->descriptorWrite(descriptorSets_[0], 0));
	descWrites.push_back(apTransmittanceTexture_->descriptorWrite(descriptorSets_[0], 1));
	descWrites.push_back(colourTex->descriptorWrite(descriptorSets_[0], 2));
	descWrites.push_back(depthTex->descriptorWrite(descriptorSets_[0], 3));
	descriptor_->updateDescriptorSets(descWrites);
	
	std::vector<Graphics::VertexOnlyPosition> vertices = { glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, -1.0f, 0.0f) };
	std::vector<uint16_t> indices = { 0, 1, 3, 2 };
	auto buf = static_cast<ElementBufferInterface*>(renderStorage_->buf());
	auto voffset = buf->addVertices(vertices.data(), vertices.size());
	auto ioffset = buf->addIndices(indices.data(), indices.size());
	buf->emplaceMesh("FullscreenQuad", indices.size(), ioffset, voffset);
	renderStorage_->addMesh(nullptr, new RenderObject("FullscreenQuad", buf->getMesh("FullscreenQuad"), nullptr, nullptr, false));

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
	pci.extent = swapChainExtent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	createPipeline<VertexOnlyPosition>(logicDevice, renderPass, RendererPipeline::makeLayoutInfo(pipelineLayouts_.size(), pipelineLayouts_.data()), stageInfos, pci);
}

PostProcessRenderer::~PostProcessRenderer()
{
	SAFE_DELETE(apScatteringTexture_);
	SAFE_DELETE(apTransmittanceTexture_);
}

void PostProcessRenderer::recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	ASSERT_DEBUG(renderStorage_->meshCount() > 0);
	beginFrame(cmdBuffer);
	static_cast<ElementBufferInterface*>(renderStorage_->buf())->bind(cmdBuffer, idx);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 1, &descriptorSets_[0], 0, nullptr);
	auto cmd = *renderStorage_->meshData();
	vkCmdDrawIndexed(cmdBuffer, cmd.count, cmd.instanceCount, cmd.firstIndex, cmd.baseVertex, cmd.baseInstance);
}

void PostProcessRenderer::initialise(const glm::mat4& viewMatrix)
{
}
