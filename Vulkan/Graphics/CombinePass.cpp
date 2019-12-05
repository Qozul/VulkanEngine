// Author: Ralph Ridley
// Date: 01/11/19
#include "CombinePass.h"
#include "GraphicsMaster.h"
#include "SwapChainDetails.h"
#include "FullscreenRenderer.h"
#include "Image.h"
#include "LogicDevice.h"
#include "SceneDescriptorInfo.h"
#include "GlobalRenderData.h"
#include "TextureManager.h"
#include "ElementBufferObject.h"

using namespace QZL;
using namespace QZL::Graphics;

CombinePass::CombinePass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo)
	: RenderPass(master, logicDevice, swapChainDetails, grd, graphicsInfo)
{
	CreateInfo createInfo = {};
	createColourBuffer(logicDevice, swapChainDetails);
	// Forward rendering colour and depth
	createInfo.attachments.push_back(makeAttachment(swapChainDetails.surfaceFormat.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

	std::vector<VkAttachmentReference> colourAttachmentRefs;
	VkAttachmentReference colourRef = {};
	colourRef.attachment = 0;
	colourRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colourAttachmentRefs.push_back(colourRef);

	createInfo.subpasses.push_back(makeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachmentRefs, nullptr));
	
	createInfo.dependencies.push_back(makeSubpassDependency(
		VK_SUBPASS_EXTERNAL,
		0,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
	);
	createInfo.dependencies.push_back(makeSubpassDependency(
		0, 
		VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT));

	std::vector<VkImageView> attachmentImages = { colourBuffer_->getImageView() };
	createRenderPass(createInfo, attachmentImages);
}

CombinePass::~CombinePass()
{
	SAFE_DELETE(colourBuffer_);
	SAFE_DELETE(atmosphereRenderer_);
	SAFE_DELETE(environmentRenderer_);
	SAFE_DELETE(combineRenderer_);
}

void CombinePass::doFrame(FrameInfo& frameInfo)
{
	std::array<VkClearValue, 1> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };

	auto bi = beginInfo(frameInfo.frameIdx, { 0, 0 }, 0);
	bi.clearValueCount = static_cast<uint32_t>(clearValues.size());
	bi.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(frameInfo.cmdBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport;
	viewport.height = float(swapChainDetails_.extent.height);
	viewport.width = float(swapChainDetails_.extent.width);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	viewport.x = 0;
	viewport.y = 0;
	vkCmdSetViewport(frameInfo.cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor;
	scissor.extent.width = swapChainDetails_.extent.width;
	scissor.extent.height = swapChainDetails_.extent.height;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vkCmdSetScissor(frameInfo.cmdBuffer, 0, 1, &scissor);

	const uint32_t dynamicOffsets[3] = {
		uint32_t(graphicsInfo_->mvpRange) * (frameInfo.frameIdx + (graphicsInfo_->numFrameIndices * frameInfo.mainCameraIdx)),
		uint32_t(graphicsInfo_->paramsRange) * frameInfo.frameIdx,
		uint32_t(graphicsInfo_->materialRange) * frameInfo.frameIdx
	};

	VkDescriptorSet sets[2] = { graphicsInfo_->set, globalRenderData_->getSet() };
	vkCmdBindDescriptorSets(frameInfo.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, atmosphereRenderer_->getPipelineLayout(), 0, 2, sets, 3, dynamicOffsets);

	environmentRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, nullptr);
	atmosphereRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, nullptr);
	combineRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, nullptr);
	vkCmdEndRenderPass(frameInfo.cmdBuffer);
}

void CombinePass::createRenderers()
{
	VkPushConstantRange pushConstants[2] = {
		RendererBase::setupPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(VertexPushConstants), 0),
		RendererBase::setupPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(FragmentPushConstants), sizeof(VertexPushConstants))
	};

	uint32_t offsets[1] = { graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kAtmosphere] };
	std::vector<VkSpecializationMapEntry> entries;
	VkSpecializationInfo specializationInfo = RendererBase::setupSpecConstantRanges(entries, offsets, offsets[0]);

	PipelineCreateInfo pci = {};
	pci.debugName = "Atmosphere";
	pci.enableDepthTest = VK_FALSE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = swapChainDetails_.extent;
	pci.frontFace = VK_FRONT_FACE_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	pci.subpassIndex = 0;
	pci.dynamicState = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	pci.sampleCount = VK_SAMPLE_COUNT_1_BIT;

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back("AtmosphereVert", VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back("AtmosphereFrag", VK_SHADER_STAGE_FRAGMENT_BIT, &specializationInfo);

	RendererCreateInfo2 createInfo2;
	createInfo2.shaderStages = stageInfos;
	createInfo2.pipelineCreateInfo = pci;
	createInfo2.pcRangesCount = 2;
	createInfo2.pcRanges = pushConstants;
	atmosphereRenderer_ = new FullscreenRenderer(createInfo2, logicDevice_, renderPass_, globalRenderData_, graphicsInfo_);

	stageInfos.clear();
	stageInfos.emplace_back("AtmosphereVert", VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back("EnvironmentFrag", VK_SHADER_STAGE_FRAGMENT_BIT, &specializationInfo);
	createInfo2.shaderStages = stageInfos;

	environmentRenderer_ = new FullscreenRenderer(createInfo2, logicDevice_, renderPass_, globalRenderData_, graphicsInfo_);

	uint32_t specTuple[4] = { diffuseIdx_, specularIdx_, albedoIdx_, ambientIdx_ };
	entries.clear();
	entries.push_back(RendererBase::makeSpecConstantEntry(0, 0, sizeof(uint32_t)));
	entries.push_back(RendererBase::makeSpecConstantEntry(1, sizeof(uint32_t), sizeof(uint32_t)));
	entries.push_back(RendererBase::makeSpecConstantEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t)));
	entries.push_back(RendererBase::makeSpecConstantEntry(3, sizeof(uint32_t) * 3, sizeof(uint32_t)));
	VkSpecializationInfo specializationInfo2 = RendererBase::setupSpecConstants(4, entries.data(), sizeof(uint32_t) * 4, &specTuple);
	std::vector<ShaderStageInfo> stageInfos2;
	stageInfos2.emplace_back("FullscreenVert", VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos2.emplace_back("DeferredLightingCombineFrag", VK_SHADER_STAGE_FRAGMENT_BIT, &specializationInfo2);

	pci.debugName = "Combine";
	createInfo2.shaderStages = stageInfos2;
	createInfo2.pipelineCreateInfo = pci;
	combineRenderer_ = new FullscreenRenderer(createInfo2, logicDevice_, renderPass_, globalRenderData_, graphicsInfo_);

	graphicsMaster_->setRenderer(RendererTypes::kAtmosphere, atmosphereRenderer_);
}

void CombinePass::initRenderPassDependency(std::vector<Image*> dependencyAttachment)
{
	ASSERT(dependencyAttachment.size() == 4);
	diffuseIdx_ = graphicsMaster_->getMasters().textureManager->allocateTexture("DiffuseSampler", dependencyAttachment[0],
		{ VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, VK_SHADER_STAGE_FRAGMENT_BIT, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE });
	specularIdx_ = graphicsMaster_->getMasters().textureManager->allocateTexture("SpecularSampler", dependencyAttachment[1],
		{ VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, VK_SHADER_STAGE_FRAGMENT_BIT, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE });
	albedoIdx_ = graphicsMaster_->getMasters().textureManager->allocateTexture("AlbedoSampler", dependencyAttachment[2],
		{ VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, VK_SHADER_STAGE_FRAGMENT_BIT, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE });
	ambientIdx_ = graphicsMaster_->getMasters().textureManager->allocateTexture("AmbientSampler", dependencyAttachment[3],
		{ VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, VK_SHADER_STAGE_FRAGMENT_BIT, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE });
	createRenderers();
}

void CombinePass::createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	colourBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, swapChainDetails.surfaceFormat.format, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "GeometryColourBuffer");
	colourBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}
