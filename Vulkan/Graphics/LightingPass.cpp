#include "LightingPass.h"
#include "GraphicsMaster.h"
#include "SwapChainDetails.h"
#include "IndexedRenderer.h"
#include "FullscreenRenderer.h"
#include "WaterRenderer.h"
#include "ParticleRenderer.h"
#include "AtmosphereRenderer.h"
#include "Image.h"
#include "LogicDevice.h"
#include "SceneDescriptorInfo.h"
#include "GlobalRenderData.h"
#include "TextureManager.h"
#include "ElementBufferObject.h"

using namespace QZL;
using namespace QZL::Graphics;

LightingPass::LightingPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo)
	: RenderPass(master, logicDevice, swapChainDetails, grd, graphicsInfo)
{
	CreateInfo createInfo = {};
	createColourBuffer(logicDevice, swapChainDetails);
	createInfo.attachments.push_back(makeAttachment(swapChainDetails.surfaceFormat.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)); // Diffuse
	createInfo.attachments.push_back(makeAttachment(swapChainDetails.surfaceFormat.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)); // Specular
	createInfo.attachments.push_back(makeAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)); // Ambient
	
	std::vector<VkAttachmentReference> colourAttachmentRefs;
	VkAttachmentReference diffuseRef = {};
	diffuseRef.attachment = 0;
	diffuseRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colourAttachmentRefs.push_back(diffuseRef);
	VkAttachmentReference specularRef = {};
	specularRef.attachment = 1;
	specularRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colourAttachmentRefs.push_back(specularRef);
	VkAttachmentReference ambientRef = {};
	ambientRef.attachment = 2;
	ambientRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colourAttachmentRefs.push_back(ambientRef);

	createInfo.dependencies.push_back(makeSubpassDependency(
		VK_SUBPASS_EXTERNAL,
		0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
	);
	createInfo.dependencies.push_back(makeSubpassDependency(
		0,
		VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT));

	createInfo.subpasses.push_back(makeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachmentRefs, nullptr));

	std::vector<VkImageView> attachmentImages = { diffuseBuffer_->getImageView(), specularBuffer_->getImageView(), ambientBuffer_->getImageView() };
	createRenderPass(createInfo, attachmentImages);
}

LightingPass::~LightingPass()
{
	SAFE_DELETE(diffuseBuffer_);
	SAFE_DELETE(specularBuffer_);
	SAFE_DELETE(ambientBuffer_);
	SAFE_DELETE(lightingRenderer_);
	SAFE_DELETE(ssaoRenderer_);
}

void LightingPass::doFrame(FrameInfo& frameInfo)
{
	std::array<VkClearValue, 3> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[1].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[2].color = { 1.0f, 0.0f, 0.0f, 0.0f };

	auto bi = beginInfo(frameInfo.frameIdx, { frameInfo.viewportWidth, swapChainDetails_.extent.height }, frameInfo.viewportX);
	bi.clearValueCount = static_cast<uint32_t>(clearValues.size());
	bi.pClearValues = clearValues.data();

	VertexPushConstants vpc;
	vpc.cameraPosition = glm::vec4(frameInfo.cameras[frameInfo.mainCameraIdx].position, 1.0f);
	vpc.mainLightPosition = frameInfo.cameras[1].position;
	vpc.shadowTextureIdx = shadowDepthIdx_;
	vpc.shadowMatrix = frameInfo.cameras[1].viewProjection;

	vkCmdPushConstants(frameInfo.cmdBuffer, lightingRenderer_->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vpc), &vpc);

	vkCmdBeginRenderPass(frameInfo.cmdBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE);
	lightingRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, &frameInfo.commandLists[(size_t)RendererTypes::kLight]);
	ssaoRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, nullptr);
	vkCmdEndRenderPass(frameInfo.cmdBuffer);
}

void LightingPass::createRenderers()
{
	RendererCreateInfo createInfo = {};
	createInfo.logicDevice = logicDevice_;
	createInfo.descriptor = descriptor_;
	createInfo.extent = swapChainDetails_.extent;
	createInfo.renderPass = renderPass_;
	createInfo.globalRenderData = globalRenderData_;
	createInfo.swapChainImageCount = swapChainDetails_.images.size();
	createInfo.graphicsInfo = graphicsInfo_;
	createInfo.subpassIndex = 0;
	createInfo.colourAttachmentCount = 3;
	createInfo.colourBlendEnables = { VK_TRUE, VK_TRUE, VK_TRUE };

	VkPushConstantRange pushConstants[1] = {
		RendererBase::setupPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(VertexPushConstants), 0)
	};

	struct Vals {
		uint32_t positionsIdx;
		uint32_t normalsIdx;
		float invScreenX;
		float invScreenY;
		uint32_t shadowIdx;
		uint32_t depthIdx;
	} specConstantValues;
	specConstantValues.positionsIdx = positionIdx_;
	specConstantValues.normalsIdx = normalsIdx_;
	specConstantValues.depthIdx = depthIdx_;
	specConstantValues.shadowIdx = shadowDepthIdx_;
	specConstantValues.invScreenX = 1.0f / (float)swapChainDetails_.extent.width;
	specConstantValues.invScreenY = 1.0f / (float)swapChainDetails_.extent.height;

	uint32_t mvpOffset = graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kLight];

	std::vector<VkSpecializationMapEntry> specEntries = {
		RendererBase::makeSpecConstantEntry(0, 0, sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(1, sizeof(uint32_t), sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(3, sizeof(uint32_t) * 3, sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(4, sizeof(uint32_t) * 4, sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(5, sizeof(uint32_t) * 5, sizeof(uint32_t))
	};

	VkSpecializationInfo specializationInfo = RendererBase::setupSpecConstants(1, specEntries.data(), sizeof(uint32_t), &mvpOffset);
	VkSpecializationInfo specializationInfo2 = RendererBase::setupSpecConstants(6, specEntries.data(), sizeof(Vals), &specConstantValues);
	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back("DeferredLightingVert", VK_SHADER_STAGE_VERTEX_BIT, &specializationInfo);
	stageInfos.emplace_back("DeferredLightingFrag", VK_SHADER_STAGE_FRAGMENT_BIT, &specializationInfo2);

	PipelineCreateInfo pci = {};
	pci.debugName = "Lighting";
	pci.enableDepthTest = VK_FALSE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pci.subpassIndex = 0;
	pci.dynamicState = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	pci.sampleCount = VK_SAMPLE_COUNT_1_BIT;
	pci.colourAttachmentCount = 3;
	pci.colourBlendEnables = { VK_TRUE, VK_TRUE, VK_TRUE };
	pci.cullFace = VK_CULL_MODE_FRONT_BIT;

	RendererCreateInfo2 createInfo2;
	createInfo2.shaderStages = stageInfos;
	createInfo2.pipelineCreateInfo = pci;
	createInfo2.pcRangesCount = 1;
	createInfo2.pcRanges = pushConstants;
	createInfo2.ebo = new ElementBufferObject(createInfo.logicDevice->getDeviceMemory(), sizeof(Vertex), sizeof(uint16_t));

	lightingRenderer_ = new IndexedRenderer(createInfo, createInfo2);
	graphicsMaster_->setRenderer(RendererTypes::kLight, lightingRenderer_);


	std::uniform_real_distribution<float> rand(0.0f, 1.0f);
	std::default_random_engine rng;
	std::array<glm::vec4, 16U> generatedData;

	for (size_t i = 0; i < 16; ++i) {
		generatedData[i] = glm::vec4(rand(rng) * 2.0 - 1.0, rand(rng) * 2.0 - 1.0, 0.0f, 0.0f);
	}

	struct SSAOVals {
		uint32_t depthIdx;
		uint32_t normalsIdx;
		uint32_t noiseIdx;
	} ssaoVals; 
	ssaoVals.depthIdx = depthIdx_;
	ssaoVals.normalsIdx = normalsIdx_;
	ssaoVals.noiseIdx = graphicsMaster_->getMasters().textureManager->allocateGeneratedTexture("SSAONoise", generatedData.data(), 4, 4, VK_FORMAT_R16G16B16A16_SFLOAT,
		MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

	std::vector<VkSpecializationMapEntry> specEntriesSSAO = {
		RendererBase::makeSpecConstantEntry(0, 0, sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(1, sizeof(uint32_t), sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t))
	};

	VkSpecializationInfo specInfoSSAO = RendererBase::setupSpecConstants(3, specEntriesSSAO.data(), sizeof(SSAOVals), &ssaoVals.depthIdx);

	pci.subpassIndex = 0;
	pci.debugName = "SSAO";
	pci.cullFace = VK_CULL_MODE_BACK_BIT;
	pci.frontFace = VK_FRONT_FACE_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	std::vector<ShaderStageInfo> stageInfosSSAO;
	stageInfosSSAO.emplace_back("FullscreenVert", VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfosSSAO.emplace_back("SSAO", VK_SHADER_STAGE_FRAGMENT_BIT, &specInfoSSAO);
	createInfo2.shaderStages = stageInfosSSAO;
	createInfo2.pipelineCreateInfo = pci;
	createInfo2.ebo = nullptr;
	ssaoRenderer_ = new FullscreenRenderer(createInfo, createInfo2);
}

void LightingPass::initRenderPassDependency(std::vector<Image*> dependencyAttachment)
{
	ASSERT(dependencyAttachment.size() == 4);
	positionIdx_ = graphicsMaster_->getMasters().textureManager->allocateTexture("PositionSampler", dependencyAttachment[0],
		{ VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, VK_SHADER_STAGE_FRAGMENT_BIT, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE });
	normalsIdx_ = graphicsMaster_->getMasters().textureManager->allocateTexture("NormalsSampler", dependencyAttachment[1],
		{ VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, VK_SHADER_STAGE_FRAGMENT_BIT, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE });
	depthIdx_ = graphicsMaster_->getMasters().textureManager->allocateTexture("DepthSampler", dependencyAttachment[2],
		{ VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, VK_SHADER_STAGE_FRAGMENT_BIT, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE });
	shadowDepthIdx_ = graphicsMaster_->getMasters().textureManager->allocateTexture("ShadowSampler", dependencyAttachment[3],
		{ VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, VK_SHADER_STAGE_FRAGMENT_BIT, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE });
	createRenderers();
}

void LightingPass::createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	diffuseBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, swapChainDetails_.surfaceFormat.format, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "DeferredDiffuseBuffer");
	diffuseBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	specularBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, swapChainDetails_.surfaceFormat.format, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "DeferredSpecularBuffer");
	specularBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; 
	ambientBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
			MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "DeferredAmbientBuffer");
	ambientBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}
