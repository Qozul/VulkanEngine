// Author: Ralph Ridley
// Date: 01/11/19
#include "PostProcessPass.h"
#include "GraphicsMaster.h"
#include "SwapChainDetails.h"
#include "TextureSampler.h"
#include "PostProcessRenderer.h"
#include "FullscreenRenderer.h"
#include "Image.h"
#include "LogicDevice.h"
#include "TextureManager.h"

using namespace QZL;
using namespace QZL::Graphics;

PostProcessPass::PostProcessPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo)
	: RenderPass(master, logicDevice, swapChainDetails, grd, graphicsInfo)
{
	// Setup the actual render pass.
	CreateInfo createInfo = {};
	createColourBuffer(logicDevice, swapChainDetails);
	createInfo.attachments.push_back(makeAttachment(swapChainDetails.surfaceFormat.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));

	std::vector<VkAttachmentReference> colourAttachmentRefs;
	VkAttachmentReference colourRef = {};
	colourRef.attachment = 0;
	colourRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colourAttachmentRefs.push_back(colourRef);

	// Subpass to mix in aerial perspective and result of geometry pass.
	createInfo.subpasses.push_back(makeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachmentRefs, nullptr));

	createInfo.dependencies.push_back(makeSubpassDependency(
		VK_SUBPASS_EXTERNAL,
		(uint32_t)SubPass::kAerialPerspective,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT));
	createInfo.dependencies.push_back(makeSubpassDependency(
		(uint32_t)SubPass::kAerialPerspective, 
		VK_SUBPASS_EXTERNAL, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT)
	);

	std::vector<VkImageView> attachmentImages = { nullptr };
	createRenderPass(createInfo, attachmentImages);
}

PostProcessPass::~PostProcessPass()
{
	SAFE_DELETE(colourBuffer_);
	SAFE_DELETE(postProcessRenderer_);
	SAFE_DELETE(fxaa_);
}

void PostProcessPass::doFrame(FrameInfo& frameInfo)
{
	std::array<VkClearValue, 1> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };

	auto bi = beginInfo(frameInfo.frameIdx);
	bi.clearValueCount = static_cast<uint32_t>(clearValues.size());
	bi.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(frameInfo.cmdBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport;
	viewport.height = swapChainDetails_.extent.height;
	viewport.width = swapChainDetails_.extent.width;
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

	fxaa_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, &frameInfo.commandLists[(size_t)RendererTypes::kPostProcess]);

	vkCmdEndRenderPass(frameInfo.cmdBuffer);
}

void PostProcessPass::initRenderPassDependency(std::vector<Image*> dependencyAttachment)
{
	ASSERT(dependencyAttachment.size() == 2);
	geometryColourBuf_ = dependencyAttachment[0];
	geometryDepthBuf_ = dependencyAttachment[1];
	gpColourBuffer_ = graphicsMaster_->getMasters().textureManager->allocateTexture("gpColourBuffer", geometryColourBuf_);
	gpDepthBuffer_ = graphicsMaster_->getMasters().textureManager->allocateTexture("gpDepthBuffer", geometryDepthBuf_, 
		SamplerInfo(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT, 2, VK_SHADER_STAGE_FRAGMENT_BIT));
	createRenderers();
}

void PostProcessPass::createRenderers()
{
	RendererCreateInfo createInfo = {};
	createInfo.logicDevice = logicDevice_;
	createInfo.descriptor = descriptor_;
	createInfo.extent = swapChainDetails_.extent;
	createInfo.renderPass = renderPass_;
	createInfo.globalRenderData = globalRenderData_;
	createInfo.swapChainImageCount = swapChainDetails_.images.size();
	createInfo.graphicsInfo = graphicsInfo_;

	//createInfo.updateRendererSpecific(0, 1, "PPVert", "PPFrag");
	//postProcessRenderer_ = new PostProcessRenderer(createInfo, gpColourBuffer_, gpDepthBuffer_);
	//graphicsMaster_->setRenderer(RendererTypes::kPostProcess, postProcessRenderer_);

	VkPushConstantRange pushConstants[1] = {
		RendererBase::setupPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(VertexPushConstants), 0)
	};

	struct Vals {
		float nearPlane;
		float farPlane;
		uint32_t colourIdx;
		uint32_t depthIdx;
	} specConstantValues;
	specConstantValues.nearPlane = 1.0f / swapChainDetails_.extent.width;
	specConstantValues.farPlane = 1.0f / swapChainDetails_.extent.height;
	specConstantValues.colourIdx = gpColourBuffer_;
	specConstantValues.depthIdx = gpDepthBuffer_;
	std::vector<VkSpecializationMapEntry> specEntries = {
		RendererBase::makeSpecConstantEntry(0, 0, sizeof(float)),
		RendererBase::makeSpecConstantEntry(1, sizeof(float), sizeof(float)),
		RendererBase::makeSpecConstantEntry(2, sizeof(float) * 2, sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(3, sizeof(uint32_t) + sizeof(float) * 2, sizeof(uint32_t))
	};
	VkSpecializationInfo specializationInfo = RendererBase::setupSpecConstants(4, specEntries.data(), sizeof(Vals), &specConstantValues);
	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back("FullscreenVert", VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back("FXAA", VK_SHADER_STAGE_FRAGMENT_BIT, &specializationInfo);

	PipelineCreateInfo pci = {};
	pci.debugName = "FXAA";
	pci.enableDepthTest = VK_FALSE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	pci.subpassIndex = createInfo.subpassIndex;
	pci.dynamicState = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	pci.sampleCount = VK_SAMPLE_COUNT_1_BIT;

	RendererCreateInfo2 createInfo2;
	createInfo2.shaderStages = stageInfos;
	createInfo2.pipelineCreateInfo = pci;
	createInfo2.pcRangesCount = 1;
	createInfo2.pcRanges = pushConstants;
	fxaa_ = new FullscreenRenderer(createInfo, createInfo2);
}

void PostProcessPass::createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	colourBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, swapChainDetails.surfaceFormat.format, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }, "PostProcessColourBuffer");
}
