#include "RenderPass.h"
#include "SwapChain.h"
#include "LogicDevice.h"
#include "Image2D.h"
#include "Descriptor.h"
#include "BasicRenderer.h"
#include "LoopRenderer.h"
#include "TexturedRenderer.h"
#include "ComputeRenderer.h"
#include "ComputeFetchRenderer.h"
#include "GraphicsMaster.h"
#include "ElementBuffer.h"
#include "MeshLoader.h"
#include "GraphicsMaster.h"
#include "../Assets/AssetManager.h"
#include "../System.h"

using namespace QZL;
using namespace QZL::Graphics;

extern EnvironmentArgs environmentArgs;

RenderPass::RenderPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
	: logicDevice_(logicDevice), swapChainDetails_(swapChainDetails), graphicsMaster_(master)
{
	createBackBuffer(logicDevice, swapChainDetails);
	auto depthFormat = createDepthBuffer(logicDevice, swapChainDetails);

	// Render pass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainDetails.surfaceFormat.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	CHECK_VKRESULT(vkCreateRenderPass(*logicDevice, &renderPassInfo, nullptr, &renderPass_));

	createFramebuffers(logicDevice, swapChainDetails);

	descriptor_ = new Descriptor(logicDevice, kMaxRenderers * swapChainDetails.imageViews.size());
	createRenderers();
}
RenderPass::~RenderPass()
{
	SAFE_DELETE(descriptor_);
	SAFE_DELETE(depthBuffer_);
	SAFE_DELETE(backBuffer_);
	for (auto framebuffer : framebuffers_) {
		vkDestroyFramebuffer(*logicDevice_, framebuffer, nullptr);
	}
	SAFE_DELETE(terrainRenderer_);
	SAFE_DELETE(texturedRenderer_);
	vkDestroyRenderPass(*logicDevice_, renderPass_, nullptr);
}

void RenderPass::doFrame(const uint32_t idx, VkCommandBuffer cmdBuffer)
{
#if defined(COMPUTE_RUN) || defined(COMPUTE_READBACK_RUN)
	CURRENT_RENDERER->recordCompute(viewMatrix_, idx, cmdBuffer);
#endif
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass_;
	renderPassInfo.framebuffer = framebuffers_[idx];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChainDetails_.extent;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	//terrainRenderer_->recordFrame(graphicsMaster_->getViewMatrix(), idx, cmdBuffer);
	texturedRenderer_->recordFrame(graphicsMaster_->getViewMatrix(), idx, cmdBuffer);

	vkCmdEndRenderPass(cmdBuffer);
}

void RenderPass::createFramebuffers(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	framebuffers_.resize(swapChainDetails.imageViews.size());
	std::array<VkImageView, 2> attachments = { nullptr, depthBuffer_->getImageView() };
	for (size_t i = 0; i < swapChainDetails.imageViews.size(); i++) {
		attachments[0] = swapChainDetails.imageViews[i];

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass_;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainDetails.extent.width;
		framebufferInfo.height = swapChainDetails.extent.height;
		framebufferInfo.layers = 1;

		CHECK_VKRESULT(vkCreateFramebuffer(*logicDevice, &framebufferInfo, nullptr, &framebuffers_[i]));
	}
}

void RenderPass::createBackBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	ImageParameters params = { VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkImageCreateInfo createInfo = Image2D::makeImageCreateInfo(swapChainDetails.extent.width, swapChainDetails.extent.height, 1,
		VK_SAMPLE_COUNT_1_BIT, swapChainDetails.surfaceFormat.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	backBuffer_ = new Image2D(logicDevice, logicDevice->getDeviceMemory(), createInfo, MemoryAllocationPattern::kRenderTarget, params);
}

VkFormat RenderPass::createDepthBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	ImageParameters params = { VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkFormat imageFormat;
	for (VkFormat format : { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(logicDevice->getPhysicalDevice(), format, &properties);
		if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			imageFormat = format;
			break;
		}
	}

	VkImageCreateInfo createInfo = Image2D::makeImageCreateInfo(swapChainDetails.extent.width, swapChainDetails.extent.height, 1,
		VK_SAMPLE_COUNT_1_BIT, imageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	depthBuffer_ = new Image2D(logicDevice, logicDevice->getDeviceMemory(), createInfo, MemoryAllocationPattern::kRenderTarget, params);
	return imageFormat;
}

void RenderPass::createRenderers()
{
	texturedRenderer_ = new TexturedRenderer(logicDevice_, graphicsMaster_->getMasters().assetManager->textureLoader, renderPass_, swapChainDetails_.extent, descriptor_, "TexturedVert", "TexturedFrag", 1);
	graphicsMaster_->setRenderer(RendererTypes::STATIC, texturedRenderer_);

	
}
