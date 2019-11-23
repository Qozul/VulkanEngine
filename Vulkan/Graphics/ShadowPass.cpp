// Author: Ralph Ridley
// Date: 18/11/19
#include "ShadowPass.h"
#include "GraphicsMaster.h"
#include "SwapChainDetails.h"
#include "ShadowRenderer.h"
#include "Image.h"
#include "LogicDevice.h"
#include "SceneDescriptorInfo.h"
#include "GlobalRenderData.h"
#include "ElementBufferObject.h"
#include "TextureManager.h"

using namespace QZL;
using namespace QZL::Graphics;

ShadowPass::ShadowPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo)
	: RenderPass(master, logicDevice, swapChainDetails, grd, graphicsInfo)
{
	CreateInfo createInfo = {};
	auto depthFormat = createDepthBuffer(logicDevice, swapChainDetails);
	createInfo.attachments.push_back(makeAttachment(depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 0;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	createInfo.subpasses.push_back(makeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, &depthAttachmentRef));
	
	createInfo.dependencies.push_back(makeSubpassDependency(
		VK_SUBPASS_EXTERNAL,
		0,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
	);
	createInfo.dependencies.push_back(makeSubpassDependency(
		0,
		VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT));

	std::vector<VkImageView> attachmentImages = { depthBuffer_->getImageView() };
	createRenderPass(createInfo, attachmentImages, { SHADOW_DIMENSIONS, SHADOW_DIMENSIONS });
	createRenderers();
	terrainHeightmapIdx_ = graphicsMaster_->getMasters().textureManager->requestTexture("Heightmaps/hmap2");
}

ShadowPass::~ShadowPass()
{
	SAFE_DELETE(shadowTerrainRenderer_);
	SAFE_DELETE(depthBuffer_);
	SAFE_DELETE(shadowRenderer_);
}

void ShadowPass::doFrame(FrameInfo& frameInfo)
{

	std::array<VkClearValue, 1> clearValues = {};
	clearValues[0].depthStencil = { 1.0f, 0 };

	auto bi = beginInfo(frameInfo.frameIdx, { SHADOW_DIMENSIONS, SHADOW_DIMENSIONS });
	bi.clearValueCount = static_cast<uint32_t>(clearValues.size());
	bi.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(frameInfo.cmdBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE);
	VkViewport viewport;
	viewport.height = SHADOW_DIMENSIONS;
	viewport.width = SHADOW_DIMENSIONS;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	viewport.x = 0;
	viewport.y = 0;
	vkCmdSetViewport(frameInfo.cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor;
	scissor.extent.width = SHADOW_DIMENSIONS;
	scissor.extent.height = SHADOW_DIMENSIONS;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vkCmdSetScissor(frameInfo.cmdBuffer, 0, 1, &scissor);

	const uint32_t dynamicOffsets[3] = {
		graphicsInfo_->mvpRange * (frameInfo.frameIdx + (graphicsInfo_->numFrameIndices * frameInfo.mainCameraIdx)),
		graphicsInfo_->paramsRange * frameInfo.frameIdx,
		graphicsInfo_->materialRange * frameInfo.frameIdx
	};

	VkDescriptorSet sets[2] = { graphicsInfo_->set, globalRenderData_->getSet() };
	vkCmdBindDescriptorSets(frameInfo.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowRenderer_->getPipelineLayout(), 0, 2, sets, 3, dynamicOffsets);

	vkCmdSetDepthBias(frameInfo.cmdBuffer, 1.25f, 0.0f, 1.75f);

	uint32_t mvpOffset[2] = { graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kStatic], 0 };
	vkCmdPushConstants(frameInfo.cmdBuffer, shadowRenderer_->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t) * 2, &mvpOffset);
	graphicsInfo_->shadowCastingEBOs[(size_t)RendererTypes::kStatic]->bind(frameInfo.cmdBuffer, frameInfo.frameIdx);
	shadowRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, &frameInfo.commandLists[(size_t)RendererTypes::kStatic]);
	mvpOffset[0] = graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kTerrain];
	mvpOffset[1] = terrainHeightmapIdx_;
	vkCmdPushConstants(frameInfo.cmdBuffer, shadowTerrainRenderer_->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t) * 2, &mvpOffset);
	graphicsInfo_->shadowCastingEBOs[(size_t)RendererTypes::kTerrain]->bind(frameInfo.cmdBuffer, frameInfo.frameIdx);
	shadowTerrainRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, &frameInfo.commandLists[(size_t)RendererTypes::kTerrain]);
	vkCmdEndRenderPass(frameInfo.cmdBuffer);
}

void ShadowPass::createRenderers()
{
	RendererCreateInfo createInfo = {};
	createInfo.logicDevice = logicDevice_;
	createInfo.descriptor = descriptor_;
	createInfo.extent = { SHADOW_DIMENSIONS, SHADOW_DIMENSIONS };
	createInfo.renderPass = renderPass_;
	createInfo.globalRenderData = globalRenderData_;
	createInfo.swapChainImageCount = swapChainDetails_.images.size();
	createInfo.graphicsInfo = graphicsInfo_;
	createInfo.prims = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	createInfo.updateRendererSpecific(0, 1, "ShadowVert",  "");
	shadowRenderer_ = new ShadowRenderer(createInfo);
	createInfo.updateRendererSpecific(0, 1, "ShadowTerrainVert", "", "", "ShadowTerrainTESC", "ShadowTerrainTESE");
	createInfo.prims = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	shadowTerrainRenderer_ = new ShadowRenderer(createInfo);

	graphicsMaster_->setRenderer(RendererTypes::kShadow, shadowRenderer_);
}

void ShadowPass::createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
}

VkFormat ShadowPass::createDepthBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	depthBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, swapChainDetails.depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, SHADOW_DIMENSIONS, SHADOW_DIMENSIONS, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }, "ShadowDepthBuffer");
	depthBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	return  swapChainDetails.depthFormat;
}

