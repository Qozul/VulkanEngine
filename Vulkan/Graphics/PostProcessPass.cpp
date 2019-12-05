// Author: Ralph Ridley
// Date: 01/11/19
// Ping pong render passes for post processing. Two render passes alternately takes a task from the post process queue. 
// After the last effect is processed, the image is passed through a pass to the present.
#include "PostProcessPass.h"
#include "GraphicsMaster.h"
#include "SwapChainDetails.h"
#include "TextureSampler.h"
#include "FullscreenRenderer.h"
#include "Image.h"
#include "LogicDevice.h"
#include "TextureManager.h"
#include "../InputManager.h"
#include "SceneDescriptorInfo.h"
#include "GlobalRenderData.h"

using namespace QZL;
using namespace QZL::Graphics;

PostProcessPass::PostProcessPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo)
	: RenderPass(master, logicDevice, swapChainDetails, grd, graphicsInfo), input_(new InputProfile()), doFXAA_(true), doDoF_(true), colourBuffer1_(nullptr), colourBufferIdx_(0),
	depthOfFieldH_(nullptr), depthOfFieldV_(nullptr), fxaa_(nullptr), geometryColourBuf_(nullptr), geometryDepthBuf_(nullptr), gpColourBuffer_(0), gpDepthBuffer_(0), presentRenderer_(nullptr),
	renderPass2_(nullptr), renderPassPresent_(nullptr)
{
	input_->profileBindings.push_back({ { GLFW_KEY_M }, [this]() {doFXAA_ = !doFXAA_; }, 0.2f });
	input_->profileBindings.push_back({ { GLFW_KEY_N }, [this]() {doDoF_ = !doDoF_; }, 0.2f });
	master->getMasters().inputManager->addProfile("Postprocessing effects", input_);
}

PostProcessPass::~PostProcessPass()
{
	SAFE_DELETE(input_);
	SAFE_DELETE(colourBuffer1_);
	SAFE_DELETE(presentRenderer_);
	SAFE_DELETE(fxaa_);
	SAFE_DELETE(depthOfFieldH_);
	SAFE_DELETE(depthOfFieldV_);
	for (auto framebuffer : framebuffers2_) {
		vkDestroyFramebuffer(*logicDevice_, framebuffer, nullptr);
	}
	vkDestroyRenderPass(*logicDevice_, renderPass2_, nullptr);
	for (auto framebuffer : framebuffersPresent_) {
		vkDestroyFramebuffer(*logicDevice_, framebuffer, nullptr);
	}
	vkDestroyRenderPass(*logicDevice_, renderPassPresent_, nullptr);
}

void PostProcessPass::doFrame(FrameInfo& frameInfo)
{
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

	PostPushConstants vpc;
	vpc.colourIdx = colourBufferIdx_;
	vpc.depthIdx = gpDepthBuffer_;
	vpc.shadowDepthIdx = 0;
	vpc.farZ = 2500.0f;
	vpc.nearZ = 0.1f;
	vpc.screenX = float(swapChainDetails_.extent.width);
	vpc.screenY = float(swapChainDetails_.extent.height);
	vkCmdPushConstants(frameInfo.cmdBuffer, presentRenderer_->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vpc), &vpc);

	const uint32_t dynamicOffsets[3] = {
		uint32_t(graphicsInfo_->mvpRange) * frameInfo.frameIdx,
		uint32_t(graphicsInfo_->paramsRange) * frameInfo.frameIdx,
		uint32_t(graphicsInfo_->materialRange) * frameInfo.frameIdx
	};

	VkDescriptorSet sets[2] = { graphicsInfo_->set, globalRenderData_->getSet() };
	vkCmdBindDescriptorSets(frameInfo.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, presentRenderer_->getPipelineLayout(), 0, 2, sets, 3, dynamicOffsets);

	std::array<VkClearValue, 1> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };

	if (!frameInfo.splitscreenEnabled) {
		if (doDoF_) {
			// ------ DoF Horiztonal
			{
				vpc.colourIdx = gpColourBuffer_;
				vkCmdPushConstants(frameInfo.cmdBuffer, presentRenderer_->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vpc), &vpc);
				auto bi1 = beginInfo(frameInfo.frameIdx, { 0, 0 }, 0, renderPass_, framebuffers_[frameInfo.frameIdx]);
				bi1.clearValueCount = static_cast<uint32_t>(clearValues.size());
				bi1.pClearValues = clearValues.data();
				vkCmdBeginRenderPass(frameInfo.cmdBuffer, &bi1, VK_SUBPASS_CONTENTS_INLINE);
				depthOfFieldH_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, nullptr);
				vkCmdEndRenderPass(frameInfo.cmdBuffer);
			}

			// ------- DoF Vertical
			{
				vpc.colourIdx = colourBufferIdx_;
				vkCmdPushConstants(frameInfo.cmdBuffer, presentRenderer_->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vpc), &vpc);
				auto bi2 = beginInfo(frameInfo.frameIdx, { 0, 0 }, 0, renderPass2_, framebuffers2_[frameInfo.frameIdx]);
				bi2.clearValueCount = static_cast<uint32_t>(clearValues.size());
				bi2.pClearValues = clearValues.data();
				vkCmdBeginRenderPass(frameInfo.cmdBuffer, &bi2, VK_SUBPASS_CONTENTS_INLINE);
				depthOfFieldV_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, nullptr);
				vkCmdEndRenderPass(frameInfo.cmdBuffer);
			}
		}
	}
	if (doFXAA_) {
		// ------- FXAA
		{
			vpc.colourIdx = gpColourBuffer_;
			vkCmdPushConstants(frameInfo.cmdBuffer, presentRenderer_->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vpc), &vpc);
			auto bi1 = beginInfo(frameInfo.frameIdx, { 0, 0 }, 0, renderPass_, framebuffers_[frameInfo.frameIdx]);
			bi1.clearValueCount = static_cast<uint32_t>(clearValues.size());
			bi1.pClearValues = clearValues.data();
			vkCmdBeginRenderPass(frameInfo.cmdBuffer, &bi1, VK_SUBPASS_CONTENTS_INLINE);
			fxaa_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, nullptr);
			vkCmdEndRenderPass(frameInfo.cmdBuffer);
		}
	}

	// -------- Present
	auto bi2 = beginInfo(frameInfo.frameIdx, { 0, 0 }, 0, renderPassPresent_, framebuffersPresent_[frameInfo.frameIdx]);
	bi2.clearValueCount = static_cast<uint32_t>(clearValues.size());
	bi2.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(frameInfo.cmdBuffer, &bi2, VK_SUBPASS_CONTENTS_INLINE);
	vpc.colourIdx = vpc.colourIdx == gpColourBuffer_ ? colourBufferIdx_ : gpColourBuffer_;
	vkCmdPushConstants(frameInfo.cmdBuffer, presentRenderer_->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vpc), &vpc);
	presentRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, nullptr);
	vkCmdEndRenderPass(frameInfo.cmdBuffer);
}

void PostProcessPass::initRenderPassDependency(std::vector<Image*> dependencyAttachment)
{
	ASSERT(dependencyAttachment.size() == 2);
	geometryColourBuf_ = dependencyAttachment[0];
	geometryDepthBuf_ = dependencyAttachment[1];
	gpColourBuffer_ = graphicsMaster_->getMasters().textureManager->allocateTexture("gpColourBuffer", geometryColourBuf_);
	gpDepthBuffer_ = graphicsMaster_->getMasters().textureManager->allocateTexture("gpDepthBuffer", geometryDepthBuf_, 
		SamplerInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 2, VK_SHADER_STAGE_FRAGMENT_BIT));
	createPasses();
	createRenderers();
}

void PostProcessPass::createRenderers()
{
	VkPushConstantRange pushConstants[1] = {
		RendererBase::setupPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PostPushConstants), 0)
	};

	PipelineCreateInfo pci = {};
	pci.cullFace = VK_CULL_MODE_BACK_BIT;
	pci.frontFace = VK_FRONT_FACE_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	pci.enableDepthTest = VK_FALSE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = swapChainDetails_.extent;
	pci.subpassIndex = 0;
	pci.dynamicState = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	pci.sampleCount = VK_SAMPLE_COUNT_1_BIT;
	pci.colourAttachmentCount = 1;
	pci.colourBlendEnables = { VK_TRUE };

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back("FullscreenVert", VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back("PassThroughFrag", VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);
	RendererCreateInfo2 createInfo2;
	createInfo2.shaderStages = stageInfos;
	createInfo2.pipelineCreateInfo = pci;
	createInfo2.pcRangesCount = 1;
	createInfo2.pcRanges = pushConstants;
	createInfo2.ebo = nullptr;
	presentRenderer_ = new FullscreenRenderer(createInfo2, logicDevice_, renderPass2_, globalRenderData_, graphicsInfo_);
	graphicsMaster_->setRenderer(RendererTypes::kPostProcess, presentRenderer_);

	struct Vals {
		float invScreenX;
		float invScreenY;
	} specConstantValues;
	specConstantValues.invScreenX = 1.0f / swapChainDetails_.extent.width;
	specConstantValues.invScreenY = 1.0f / swapChainDetails_.extent.height;
	std::vector<VkSpecializationMapEntry> specEntries = {
		RendererBase::makeSpecConstantEntry(0, 0, sizeof(float)),
		RendererBase::makeSpecConstantEntry(1, sizeof(float), sizeof(float))
	};
	VkSpecializationInfo specializationInfo = RendererBase::setupSpecConstants(2, specEntries.data(), sizeof(Vals), &specConstantValues);
	stageInfos.clear();
	stageInfos.emplace_back("FullscreenVert", VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back("FXAA", VK_SHADER_STAGE_FRAGMENT_BIT, &specializationInfo);

	pci.debugName = "FXAA";
	createInfo2.shaderStages = stageInfos;
	createInfo2.pipelineCreateInfo = pci;
	fxaa_ = new FullscreenRenderer(createInfo2, logicDevice_, renderPass_, globalRenderData_, graphicsInfo_);

	pci.debugName = "DoFV";
	stageInfos.clear();
	stageInfos.emplace_back("FullscreenVert", VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back("DoFHorizontal", VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);
	createInfo2.shaderStages = stageInfos;
	createInfo2.pipelineCreateInfo = pci;
	depthOfFieldV_ = new FullscreenRenderer(createInfo2, logicDevice_, renderPass2_, globalRenderData_, graphicsInfo_);

	pci.debugName = "DoFH";
	stageInfos.clear();
	stageInfos.emplace_back("FullscreenVert", VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back("DoFVertical", VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);
	createInfo2.shaderStages = stageInfos;
	createInfo2.pipelineCreateInfo = pci;
	depthOfFieldH_ = new FullscreenRenderer(createInfo2, logicDevice_, renderPass_, globalRenderData_, graphicsInfo_);
}

void PostProcessPass::createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	colourBuffer1_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, swapChainDetails.surfaceFormat.format, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "PostProcessColourBuffer");
	colourBuffer1_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	colourBufferIdx_ = graphicsMaster_->getMasters().textureManager->allocateTexture("pingPongTexture1", colourBuffer1_);
}

void PostProcessPass::createPasses()
{
	CreateInfo createInfo = {};
	createColourBuffer(logicDevice_, swapChainDetails_);
	createInfo.attachments.push_back(makeAttachment(swapChainDetails_.surfaceFormat.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
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
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT));
	createInfo.dependencies.push_back(makeSubpassDependency(
		0,
		VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT)
	);

	std::vector<VkImageView> attachmentImages = { colourBuffer1_->getImageView() };
	createRenderPass(createInfo, attachmentImages, { 0, 0 }, &renderPass_, framebuffers_);

	std::vector<VkImageView> attachmentImages2 = { geometryColourBuf_->getImageView() };
	createRenderPass(createInfo, attachmentImages2, { 0, 0 }, &renderPass2_, framebuffers2_);

	CreateInfo createInfoPresent = {};
	createInfoPresent.attachments.push_back(makeAttachment(swapChainDetails_.surfaceFormat.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));

	createInfoPresent.subpasses.push_back(makeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachmentRefs, nullptr));

	createInfoPresent.dependencies.push_back(makeSubpassDependency(
		VK_SUBPASS_EXTERNAL,
		0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT));
	createInfoPresent.dependencies.push_back(makeSubpassDependency(
		0,
		VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT)
	);

	std::vector<VkImageView> attachmentImagesPresent = { nullptr };
	createRenderPass(createInfoPresent, attachmentImagesPresent, { 0, 0 }, &renderPassPresent_, framebuffersPresent_);
}
