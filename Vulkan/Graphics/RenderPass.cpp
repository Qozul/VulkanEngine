// Author: Ralph Ridley
// Date: 01/11/19
#include "RenderPass.h"
#include "SwapChainDetails.h"
#include "LogicDevice.h"

using namespace QZL;
using namespace QZL::Graphics;

RenderPass::RenderPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo)
	: logicDevice_(logicDevice), swapChainDetails_(swapChainDetails), graphicsMaster_(master), globalRenderData_(grd), descriptor_(logicDevice->getPrimaryDescriptor()), renderPass_(VK_NULL_HANDLE),
	graphicsInfo_(graphicsInfo)
{
}

RenderPass::~RenderPass()
{
	for (auto framebuffer : framebuffers_) {
		vkDestroyFramebuffer(*logicDevice_, framebuffer, nullptr);
	}
	vkDestroyRenderPass(*logicDevice_, renderPass_, nullptr);
}

void RenderPass::createRenderPass(CreateInfo& createInfo, std::vector<VkImageView>& attachmentImages, VkExtent2D extent)
{
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(createInfo.attachments.size());
	renderPassInfo.pAttachments = createInfo.attachments.data();
	renderPassInfo.subpassCount = static_cast<uint32_t>(createInfo.subpasses.size());
	renderPassInfo.pSubpasses = createInfo.subpasses.data();
	renderPassInfo.dependencyCount = static_cast<uint32_t>(createInfo.dependencies.size());
	renderPassInfo.pDependencies = createInfo.dependencies.data();

	CHECK_VKRESULT(vkCreateRenderPass(*logicDevice_, &renderPassInfo, nullptr, &renderPass_));
	if (extent.width == 0) {
		createFramebuffers(logicDevice_, swapChainDetails_, attachmentImages, swapChainDetails_.extent);
	}
	else {
		createFramebuffers(logicDevice_, swapChainDetails_, attachmentImages, extent);
	}
}

void RenderPass::createFramebuffers(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, std::vector<VkImageView>& attachmentImages,
	VkExtent2D extent)
{
	bool useSwapChainImageView = attachmentImages[0] == nullptr;
	framebuffers_.resize(swapChainDetails.imageViews.size());
	for (size_t i = 0; i < swapChainDetails.imageViews.size(); ++i) {
		if (useSwapChainImageView) {
			attachmentImages[0] = swapChainDetails.imageViews[i];
		}

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass_;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentImages.size());
		framebufferInfo.pAttachments = attachmentImages.data();
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		CHECK_VKRESULT(vkCreateFramebuffer(*logicDevice, &framebufferInfo, nullptr, &framebuffers_[i]));
	}
}

VkRenderPassBeginInfo RenderPass::beginInfo(const uint32_t& idx, VkExtent2D extent, int32_t offsetX)
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass_;
	renderPassInfo.framebuffer = framebuffers_[idx];
	renderPassInfo.renderArea.offset = { offsetX, 0 };
	renderPassInfo.renderArea.extent = extent.width == 0 ? swapChainDetails_.extent : extent;
	return renderPassInfo;
}

VkAttachmentDescription RenderPass::makeAttachment(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
	VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp, VkImageLayout initialLayout, VkImageLayout finalLayout)
{
	VkAttachmentDescription attachment = {};
	attachment.format = format;
	attachment.samples = samples;
	attachment.loadOp = loadOp;
	attachment.storeOp = storeOp;
	attachment.stencilLoadOp = stencilLoadOp;
	attachment.stencilStoreOp = stencilStoreOp;
	attachment.initialLayout = initialLayout;
	attachment.finalLayout = finalLayout;
	return attachment;
}

VkSubpassDescription RenderPass::makeSubpass(VkPipelineBindPoint pipelineType, std::vector<VkAttachmentReference>& colourReferences, VkAttachmentReference* depthReference, VkAttachmentReference* resolve)
{
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = static_cast<uint32_t>(colourReferences.size());
	subpass.pColorAttachments = colourReferences.data();
	subpass.pDepthStencilAttachment = depthReference;
	subpass.pResolveAttachments = resolve;
	return subpass;
}
VkSubpassDescription RenderPass::makeSubpass(VkPipelineBindPoint pipelineType, VkAttachmentReference* depthReference)
{
	VkSubpassDescription atmosphereSubpass = {};
	atmosphereSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	atmosphereSubpass.colorAttachmentCount = 0;
	atmosphereSubpass.pColorAttachments = nullptr;
	atmosphereSubpass.pDepthStencilAttachment = depthReference;
	return atmosphereSubpass;
}

VkSubpassDependency RenderPass::makeSubpassDependency(uint32_t srcIdx, uint32_t dstIdx, VkPipelineStageFlags srcStage, 
	VkAccessFlags srcAccess, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess)
{
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = srcIdx;
	dependency.dstSubpass = dstIdx;
	dependency.srcStageMask = srcStage;
	dependency.srcAccessMask = srcAccess;
	dependency.dstStageMask = dstStage;
	dependency.dstAccessMask = dstAccess;
	dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	return dependency;
}
