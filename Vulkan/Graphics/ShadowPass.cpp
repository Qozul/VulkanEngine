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
}

ShadowPass::~ShadowPass()
{
	SAFE_DELETE(shadowTerrainRenderer_);
	SAFE_DELETE(depthBuffer_);
	SAFE_DELETE(shadowRenderer_);
}

void ShadowPass::doFrame(LogicalCamera* cameras, const size_t cameraCount, const uint32_t& idx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandLists)
{
	std::array<VkClearValue, 1> clearValues = {};
	clearValues[0].depthStencil = { 1.0f, 0 };

	auto bi = beginInfo(idx, { SHADOW_DIMENSIONS, SHADOW_DIMENSIONS });
	bi.clearValueCount = static_cast<uint32_t>(clearValues.size());
	bi.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(cmdBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport;
	viewport.height = SHADOW_DIMENSIONS;
	viewport.width = SHADOW_DIMENSIONS;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	viewport.x = 0;
	viewport.y = 0;
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor;
	scissor.extent.width = SHADOW_DIMENSIONS;
	scissor.extent.height = SHADOW_DIMENSIONS;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	const uint32_t dynamicOffsets[3] = {
		graphicsInfo_->mvpRange * (idx + graphicsInfo_->numFrameIndices),
		graphicsInfo_->paramsRange * (idx + graphicsInfo_->numFrameIndices),
		graphicsInfo_->materialRange * (idx + graphicsInfo_->numFrameIndices)
	};

	VkDescriptorSet sets[2] = { graphicsInfo_->set, globalRenderData_->getSet() };
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowRenderer_->getPipelineLayout(), 0, 2, sets, 3, dynamicOffsets);

	vkCmdSetDepthBias(cmdBuffer, 1.25f, 0.0f, 1.75f);

	uint32_t mvpOffset[2] = { graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kStatic], 0 };
	vkCmdPushConstants(cmdBuffer, shadowRenderer_->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t), &mvpOffset);
	graphicsInfo_->shadowCastingEBOs[(size_t)RendererTypes::kStatic]->bind(cmdBuffer, idx);
	shadowRenderer_->recordFrame(cameras[1], idx, cmdBuffer, &commandLists[(size_t)RendererTypes::kStatic]);
	mvpOffset[0] = graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kTerrain];
	mvpOffset[1] = graphicsMaster_->getMasters().textureManager->getSamplerIdx("Heightmaps/hmap2");
	//vkCmdPushConstants(cmdBuffer, shadowTerrainRenderer_->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t), &mvpOffset);
	//graphicsInfo_->shadowCastingEBOs[(size_t)RendererTypes::kTerrain]->bind(cmdBuffer, idx);
	//shadowTerrainRenderer_->recordFrame(cameras[1], idx, cmdBuffer, &commandLists[(size_t)RendererTypes::kTerrain]);

	vkCmdEndRenderPass(cmdBuffer);
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
	createInfo.updateRendererSpecific(0, 1, "ShadowTerrainVert", "", "", "ShadowterrainTESC", "ShadowTerrainTESE");
	createInfo.prims = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	shadowTerrainRenderer_ = new ShadowRenderer(createInfo);

	graphicsMaster_->setRenderer(RendererTypes::kShadow, shadowRenderer_);
}

void ShadowPass::createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	colourBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, swapChainDetails.surfaceFormat.format, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, SHADOW_DIMENSIONS, SHADOW_DIMENSIONS, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
}

VkFormat ShadowPass::createDepthBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	depthBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, SHADOW_DIMENSIONS, SHADOW_DIMENSIONS, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }, "ShadowDepthBuffer");
	depthBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	return VK_FORMAT_D32_SFLOAT;
}

