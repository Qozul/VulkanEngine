#include "RenderPass.h"
#include "SwapChain.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "TexturedRenderer.h"
#include "TerrainRenderer.h"
#include "AtmosphereRenderer.h"
#include "GraphicsMaster.h"
#include "ElementBuffer.h"
#include "MeshLoader.h"
#include "GraphicsMaster.h"
#include "StorageBuffer.h"
#include "TextureManager.h"
#include "../Assets/AssetManager.h"
#include "../System.h"

using namespace QZL;
using namespace QZL::Graphics;

extern EnvironmentArgs environmentArgs;

RenderPass::RenderPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd)
	: logicDevice_(logicDevice), swapChainDetails_(swapChainDetails), graphicsMaster_(master), globalRenderData_(grd), descriptor_(logicDevice->getPrimaryDescriptor())
{
}

RenderPass::~RenderPass()
{
	for (auto framebuffer : framebuffers_) {
		vkDestroyFramebuffer(*logicDevice_, framebuffer, nullptr);
	}
	vkDestroyRenderPass(*logicDevice_, renderPass_, nullptr);
}

void RenderPass::createRenderPass(CreateInfo& createInfo, std::vector<VkImageView>& attachmentImages, bool firstAttachmentIsSwapChainImage)
{
	// Refer to https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#synchronization-access-types-supported in case of
	// acces mask related errors
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(createInfo.attachments.size());
	renderPassInfo.pAttachments = createInfo.attachments.data();
	renderPassInfo.subpassCount = static_cast<uint32_t>(createInfo.subpasses.size());
	renderPassInfo.pSubpasses = createInfo.subpasses.data();
	renderPassInfo.dependencyCount = static_cast<uint32_t>(createInfo.dependencies.size());
	renderPassInfo.pDependencies = createInfo.dependencies.data();

	CHECK_VKRESULT(vkCreateRenderPass(*logicDevice_, &renderPassInfo, nullptr, &renderPass_));
	createFramebuffers(logicDevice_, swapChainDetails_, attachmentImages, firstAttachmentIsSwapChainImage);
}

void RenderPass::createFramebuffers(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, std::vector<VkImageView>& attachmentImages, bool firstAttachmentIsSwapChainImage)
{
	framebuffers_.resize(swapChainDetails.imageViews.size());
	for (size_t i = 0; i < swapChainDetails.imageViews.size(); i++) {
		if (firstAttachmentIsSwapChainImage) {
			attachmentImages[0] = swapChainDetails.imageViews[i];
		}

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass_;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentImages.size());
		framebufferInfo.pAttachments = attachmentImages.data();
		framebufferInfo.width = swapChainDetails.extent.width;
		framebufferInfo.height = swapChainDetails.extent.height;
		framebufferInfo.layers = 1;

		CHECK_VKRESULT(vkCreateFramebuffer(*logicDevice, &framebufferInfo, nullptr, &framebuffers_[i]));
	}
}

VkRenderPassBeginInfo RenderPass::beginInfo(const uint32_t& idx)
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass_;
	renderPassInfo.framebuffer = framebuffers_[idx];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChainDetails_.extent;
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

VkSubpassDescription RenderPass::makeSubpass(VkPipelineBindPoint pipelineType, std::vector<VkAttachmentReference>& colourReferences, VkAttachmentReference* depthReference)
{
	VkSubpassDescription atmosphereSubpass = {};
	atmosphereSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	atmosphereSubpass.colorAttachmentCount = static_cast<uint32_t>(colourReferences.size());
	atmosphereSubpass.pColorAttachments = colourReferences.data();
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
