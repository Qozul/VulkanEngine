#include "RenderPass.h"
#include "SwapChain.h"
#include "LogicDevice.h"
#include "Image2D.h"
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

const glm::vec3 RenderPass::kAmbientColour = glm::vec3(0.2f, 0.2f, 0.2f);

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

	descriptor_ = logicDevice->getPrimaryDescriptor();

	// Create the global lighting uniform buffer
	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags = {};
	setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	setLayoutBindingFlags.bindingCount = 2;
	std::vector<VkDescriptorBindingFlagsEXT> descriptorBindingFlags = {
		0,
		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
	};
	setLayoutBindingFlags.pBindingFlags = descriptorBindingFlags.data();

	master->getMasters().assetManager->textureManager = new Graphics::TextureManager(logicDevice, descriptor_, 2, graphicsMaster_->supportsOptionalExtension(OptionalExtensions::DESCRIPTOR_INDEXING));

	globalRenderData_ = new GlobalRenderData;
	globalRenderData_->globalDataDescriptor = descriptor_;
	lightingUbo_ = new StorageBuffer(logicDevice, MemoryAllocationPattern::kDynamicResource, (uint32_t)ReservedGraphicsBindings1::LIGHTING, 0,
		sizeof(LightingData), VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, true);
	if (master->supportsOptionalExtension(OptionalExtensions::DESCRIPTOR_INDEXING)) {
		globalRenderData_->layout = descriptor_->makeLayout({ lightingUbo_->getBinding(), master->getMasters().assetManager->textureManager->getSetlayoutBinding() }, &setLayoutBindingFlags);
	}
	else {
		globalRenderData_->layout = descriptor_->makeLayout({ lightingUbo_->getBinding() });
	}
	globalRenderData_->setIdx = descriptor_->createSets({ globalRenderData_->layout }); // Only 1 set rather than 1 per frame, data is constant
	master->getMasters().assetManager->textureManager->setDescriptorSetIdx(globalRenderData_->setIdx);
	descriptor_->updateDescriptorSets({ lightingUbo_->descriptorWrite(descriptor_->getSet(globalRenderData_->setIdx)) });

	createRenderers();
}
RenderPass::~RenderPass()
{
	SAFE_DELETE(globalRenderData_);
	SAFE_DELETE(lightingUbo_);
	SAFE_DELETE(descriptor_);
	SAFE_DELETE(depthBuffer_);
	SAFE_DELETE(backBuffer_);
	for (auto framebuffer : framebuffers_) {
		vkDestroyFramebuffer(*logicDevice_, framebuffer, nullptr);
	}
	SAFE_DELETE(terrainRenderer_);
	SAFE_DELETE(texturedRenderer_);
	SAFE_DELETE(atmosphereRenderer_);
	vkDestroyRenderPass(*logicDevice_, renderPass_, nullptr);
}

void RenderPass::doFrame(const uint32_t idx, VkCommandBuffer cmdBuffer)
{
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

	updateGlobalDescriptors(idx, cmdBuffer);

	//terrainRenderer_->recordFrame(graphicsMaster_->getViewMatrix(), idx, cmdBuffer);
	//texturedRenderer_->recordFrame(graphicsMaster_->getViewMatrix(), idx, cmdBuffer);
	atmosphereRenderer_->recordFrame(graphicsMaster_->getViewMatrix(), idx, cmdBuffer);

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

	VkFormat imageFormat = VkFormat::VK_FORMAT_UNDEFINED;
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
	/*std::string fragName = logicDevice_->supportsOptionalExtension(OptionalExtensions::DESCRIPTOR_INDEXING) ? "StaticFrag_DI" : "StaticFrag";
	texturedRenderer_ = new TexturedRenderer(logicDevice_, graphicsMaster_->getMasters().assetManager->textureManager, renderPass_, swapChainDetails_.extent, descriptor_, "StaticVert", fragName, 1, globalRenderData_);
	graphicsMaster_->setRenderer(RendererTypes::STATIC, texturedRenderer_);*/

	/*terrainRenderer_ = new TerrainRenderer(logicDevice_, graphicsMaster_->getMasters().assetManager->textureManager, renderPass_, swapChainDetails_.extent, descriptor_, 
		"TerrainVert", "TerrainTESC", "TerrainTESE", "TerrainFrag", 1, globalRenderData_);
	graphicsMaster_->setRenderer(RendererTypes::TERRAIN, terrainRenderer_);*/

	atmosphereRenderer_ = new AtmosphereRenderer(logicDevice_, graphicsMaster_->getMasters().assetManager->textureManager, renderPass_, swapChainDetails_.extent, descriptor_,
		"AtmosphereVert", "AtmosphereTESC", "AtmosphereTESE", "AtmosphereFrag", 1, globalRenderData_);
	graphicsMaster_->setRenderer(RendererTypes::ATMOSPHERE, atmosphereRenderer_);
}

void RenderPass::updateGlobalDescriptors(const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	LightingData* lightingPtr = static_cast<LightingData*>(lightingUbo_->bindRange());
	*lightingPtr = {
		glm::vec4(graphicsMaster_->getCamPos(), 0.0), glm::vec4(kAmbientColour, 0.0), glm::vec4(1000.0, 500.0, -1000.0, 0.0)
	};
	lightingUbo_->unbindRange();
}
