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
	createRenderPass(createInfo, attachmentImages, false);
	createRenderers();
}

ShadowPass::~ShadowPass()
{
	SAFE_DELETE(depthBuffer_);
	SAFE_DELETE(shadowRenderer_);
}

void ShadowPass::doFrame(LogicalCamera& camera, const uint32_t& idx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandLists)
{
	std::array<VkClearValue, 1> clearValues = {};
	clearValues[0].depthStencil = { 1.0f, 0 };

	auto bi = beginInfo(idx);
	bi.clearValueCount = static_cast<uint32_t>(clearValues.size());
	bi.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(cmdBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE);

	const uint32_t dynamicOffsets[3] = {
		graphicsInfo_->mvpRange * (idx + 3),
		graphicsInfo_->paramsRange * (idx + 3),
		graphicsInfo_->materialRange * (idx + 3)
	};

	VkDescriptorSet sets[2] = { graphicsInfo_->set, globalRenderData_->getSet() };
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowRenderer_->getPipelineLayout(), 0, 2, sets, 3, dynamicOffsets);
	
	vkCmdSetDepthBias(cmdBuffer, 1.25f, 0.0f, 1.75f);

	uint32_t mvpOffset = graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kStatic];
	vkCmdPushConstants(cmdBuffer, shadowRenderer_->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t), &mvpOffset);
	graphicsInfo_->shadowCastingEBOs[(size_t)RendererTypes::kStatic]->bind(cmdBuffer, idx);
	shadowRenderer_->recordFrame(camera, idx, cmdBuffer, &commandLists[(size_t)RendererTypes::kStatic]);

	mvpOffset = graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kTerrain];
	vkCmdPushConstants(cmdBuffer, shadowRenderer_->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t), &mvpOffset);
	graphicsInfo_->shadowCastingEBOs[(size_t)RendererTypes::kTerrain]->bind(cmdBuffer, idx);
	shadowRenderer_->recordFrame(camera, idx, cmdBuffer, &commandLists[(size_t)RendererTypes::kTerrain]);

	vkCmdEndRenderPass(cmdBuffer);
}

void ShadowPass::createRenderers()
{
	RendererCreateInfo createInfo = {};
	createInfo.logicDevice = logicDevice_;
	createInfo.descriptor = descriptor_;
	createInfo.extent = swapChainDetails_.extent;
	createInfo.renderPass = renderPass_;
	createInfo.globalRenderData = globalRenderData_;
	createInfo.swapChainImageCount = swapChainDetails_.images.size();
	createInfo.graphicsInfo = graphicsInfo_;
	createInfo.updateRendererSpecific(0, 1, "ShadowVert",  "ShadowFrag");
	shadowRenderer_ = new ShadowRenderer(createInfo);

	graphicsMaster_->setRenderer(RendererTypes::kShadow, shadowRenderer_);
}

VkFormat ShadowPass::createDepthBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	VkFormat imageFormat = VkFormat::VK_FORMAT_UNDEFINED;
	for (VkFormat format : { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM }) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(logicDevice->getPhysicalDevice(), format, &properties);
		if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT && properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
			imageFormat = format;
			break;
		}
	}
	ASSERT(imageFormat != VkFormat::VK_FORMAT_UNDEFINED);

	depthBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, imageFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, SHADOW_DIMENSIONS, SHADOW_DIMENSIONS, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });
	depthBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	return imageFormat;
}

