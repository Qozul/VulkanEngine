// Author: Ralph Ridley
// Date: 01/11/19
#include "PostProcessPass.h"
#include "GraphicsMaster.h"
#include "SwapChainDetails.h"
#include "TextureSampler.h"
#include "PostProcessRenderer.h"
#include "ParticleRenderer.h"
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
	auto depthFormat = createDepthBuffer(logicDevice, swapChainDetails);
	createInfo.attachments.push_back(makeAttachment(swapChainDetails.surfaceFormat.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
	createInfo.attachments.push_back(makeAttachment(depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));

	std::vector<VkAttachmentReference> colourAttachmentRefs;
	VkAttachmentReference colourRef = {};
	colourRef.attachment = 0;
	colourRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colourAttachmentRefs.push_back(colourRef);
	VkAttachmentReference depthRef = {};
	depthRef.attachment = 1;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Subpass to mix in aerial perspective and result of geometry pass.
	createInfo.subpasses.push_back(makeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachmentRefs, &depthRef));

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

	std::vector<VkImageView> attachmentImages = { nullptr, depthBuffer_->getImageView() };
	createRenderPass(createInfo, attachmentImages, true);
}

PostProcessPass::~PostProcessPass()
{
	SAFE_DELETE(colourBuffer_);
	SAFE_DELETE(depthBuffer_);
	SAFE_DELETE(postProcessRenderer_);
	SAFE_DELETE(particleRenderer_);
}

void PostProcessPass::doFrame(LogicalCamera* cameras, const size_t cameraCount, const uint32_t& idx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandLists)
{
	VkOffset3D imageOffset = {};
	imageOffset.x = 0;
	imageOffset.y = 0;
	imageOffset.z = 0;

	VkExtent3D imageCopyExtent = {};
	imageCopyExtent.width = swapChainDetails_.extent.width;
	imageCopyExtent.height = swapChainDetails_.extent.height;
	imageCopyExtent.depth = 0;

	VkImageSubresourceLayers srcSubresource = {};
	srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	srcSubresource.mipLevel = 0;
	srcSubresource.baseArrayLayer = 0;
	srcSubresource.layerCount = 1;

	VkImageSubresourceLayers dstSubresource = {};
	dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	dstSubresource.mipLevel = 0;
	dstSubresource.baseArrayLayer = 0;
	dstSubresource.layerCount = 1;

	VkImageCopy imageCopy = {};
	imageCopy.srcOffset = imageOffset;
	imageCopy.dstOffset = imageOffset;
	imageCopy.extent = imageCopyExtent;
	imageCopy.srcSubresource = srcSubresource;
	imageCopy.dstSubresource = dstSubresource;

	vkCmdCopyImage(cmdBuffer, geometryDepthBuf_->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, depthBuffer_->getImage(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

	geometryDepthBuf_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	geometryDepthBuf_->changeLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	geometryDepthBuf_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	auto bi = beginInfo(idx);
	bi.clearValueCount = static_cast<uint32_t>(clearValues.size());
	bi.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(cmdBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE);

	postProcessRenderer_->recordFrame(cameras[0], idx, cmdBuffer, &commandLists[(size_t)RendererTypes::kPostProcess]);

	particleRenderer_->recordFrame(cameras[0], idx, cmdBuffer, &commandLists[(size_t)RendererTypes::kParticle]);

	vkCmdEndRenderPass(cmdBuffer);
}

void PostProcessPass::initRenderPassDependency(std::vector<Image*> dependencyAttachment)
{
	ASSERT(dependencyAttachment.size() == 2);
	geometryColourBuf_ = dependencyAttachment[0];
	geometryDepthBuf_ = dependencyAttachment[1];
	gpColourBuffer_ = graphicsMaster_->getMasters().textureManager->allocateTexture("gpColourBuffer", geometryColourBuf_);
	gpDepthBuffer_ = graphicsMaster_->getMasters().textureManager->allocateTexture("gpDepthBuffer", geometryDepthBuf_);
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

	createInfo.updateRendererSpecific(0, 1, "PPVert", "PPFrag");
	postProcessRenderer_ = new PostProcessRenderer(createInfo, gpColourBuffer_, gpDepthBuffer_);

	createInfo.updateRendererSpecific(0, 2, "ParticlesVert", "ParticlesFrag", "ParticlesGeom");
	particleRenderer_ = new ParticleRenderer(createInfo);

	graphicsMaster_->setRenderer(RendererTypes::kParticle, particleRenderer_);
	graphicsMaster_->setRenderer(RendererTypes::kPostProcess, postProcessRenderer_);
}

void PostProcessPass::createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	colourBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, swapChainDetails.surfaceFormat.format, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
}

VkFormat PostProcessPass::createDepthBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
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
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL });
	depthBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	return imageFormat;
}
