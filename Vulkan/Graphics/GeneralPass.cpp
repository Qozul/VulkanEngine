// Author: Ralph Ridley
// Date: 01/11/19
#include "GeneralPass.h"
#include "GraphicsMaster.h"
#include "SwapChainDetails.h"
#include "TexturedRenderer.h"
#include "TerrainRenderer.h"
#include "AtmosphereRenderer.h"
#include "Image.h"
#include "LogicDevice.h"

using namespace QZL;
using namespace QZL::Graphics;

GeometryPass::GeometryPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd)
	: RenderPass(master, logicDevice, swapChainDetails, grd)
{
	CreateInfo createInfo = {};
	createColourBuffer(logicDevice, swapChainDetails);
	auto depthFormat = createDepthBuffer(logicDevice, swapChainDetails);
	createInfo.attachments.push_back(makeAttachment(swapChainDetails.surfaceFormat.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	createInfo.attachments.push_back(makeAttachment(depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL));

	std::vector<VkAttachmentReference> colourAttachmentRefs;
	VkAttachmentReference colourRef = {};
	colourRef.attachment = 0;
	colourRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colourAttachmentRefs.push_back(colourRef);
	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Atmosphere and general subpasses
	createInfo.subpasses.push_back(makeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachmentRefs, &depthAttachmentRef));
	createInfo.subpasses.push_back(makeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachmentRefs, &depthAttachmentRef));
	
	createInfo.dependencies.push_back(makeSubpassDependency(
		VK_SUBPASS_EXTERNAL,
		(uint32_t)SubPass::ATMOSPHERE,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
	);
	createInfo.dependencies.push_back(makeSubpassDependency(
		(uint32_t)SubPass::ATMOSPHERE,
		(uint32_t)SubPass::GENERAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
	);
	createInfo.dependencies.push_back(makeSubpassDependency(
		(uint32_t)SubPass::GENERAL, 
		VK_SUBPASS_EXTERNAL, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT));

	std::vector<VkImageView> attachmentImages = { colourBuffer_->getImageView(), depthBuffer_->getImageView() };
	createRenderPass(createInfo, attachmentImages, false);
	createRenderers();
}

GeometryPass::~GeometryPass()
{
	SAFE_DELETE(depthBuffer_);
	SAFE_DELETE(colourBuffer_);
	SAFE_DELETE(terrainRenderer_);
	SAFE_DELETE(texturedRenderer_);
	SAFE_DELETE(atmosphereRenderer_);
}

void GeometryPass::doFrame(LogicalCamera& camera, const uint32_t& idx, VkCommandBuffer cmdBuffer)
{
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	auto bi = beginInfo(idx);
	bi.clearValueCount = static_cast<uint32_t>(clearValues.size());
	bi.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(cmdBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE);

	atmosphereRenderer_->recordFrame(camera, idx, cmdBuffer);

	vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

	terrainRenderer_->recordFrame(camera, idx, cmdBuffer);
	texturedRenderer_->recordFrame(camera, idx, cmdBuffer);

	vkCmdEndRenderPass(cmdBuffer);
}

void GeometryPass::createRenderers()
{
	RendererCreateInfo createInfo = {};
	createInfo.logicDevice = logicDevice_;
	createInfo.descriptor = descriptor_;
	createInfo.extent = swapChainDetails_.extent;
	createInfo.renderPass = renderPass_;
	createInfo.globalRenderData = globalRenderData_;
	createInfo.swapChainImageCount = swapChainDetails_.images.size();

	createInfo.updateRendererSpecific(1, 1, "StaticVert", logicDevice_->supportsOptionalExtension(OptionalExtensions::DESCRIPTOR_INDEXING) ? "StaticFrag_DI" : "StaticFrag");
	texturedRenderer_ = new TexturedRenderer(createInfo);

	createInfo.updateRendererSpecific(1, 1, "TerrainVert", "TerrainFrag", "", "TerrainTESC", "TerrainTESE");
	terrainRenderer_ = new TerrainRenderer(createInfo);

	createInfo.updateRendererSpecific(0, 1, "AtmosphereVert", "AtmosphereFrag");
	atmosphereRenderer_ = new AtmosphereRenderer(createInfo);

	graphicsMaster_->setRenderer(RendererTypes::STATIC, texturedRenderer_);
	graphicsMaster_->setRenderer(RendererTypes::TERRAIN, terrainRenderer_);
	graphicsMaster_->setRenderer(RendererTypes::ATMOSPHERE, atmosphereRenderer_);
}

void GeometryPass::createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	colourBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, swapChainDetails.surfaceFormat.format, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	colourBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

VkFormat GeometryPass::createDepthBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
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
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });
	depthBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	return imageFormat;
}
