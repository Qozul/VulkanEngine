#include "PostProcessPass.h"
#include "SwapChain.h"
#include "GraphicsMaster.h"
#include "../SystemMasters.h"
#include "../Assets/AssetManager.h"
#include "TextureManager.h"
#include "TextureSampler.h"
#include "ComputePipeline.h"
#include "PostProcessRenderer.h"
#include "Image.h"
#include "../Assets/AtmosphereParameters.h"
#include "../Game/AtmosphereScript.h"
#include "AtmosphereRenderer.h"

using namespace QZL;
using namespace QZL::Graphics;

PostProcessPass::PostProcessPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd)
	: RenderPass(master, logicDevice, swapChainDetails, grd)
{
	// Setup the actual render pass.
	CreateInfo createInfo = {};
	createColourBuffer(logicDevice, swapChainDetails);
	createInfo.attachments.push_back(makeAttachment(swapChainDetails.surfaceFormat.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));

	std::vector<VkAttachmentReference> colourAttachmentRefs;
	VkAttachmentReference colourRef = {};
	colourRef.attachment = 0;
	colourRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colourAttachmentRefs.push_back(colourRef);

	// Subpass to mix in aerial perspective and result of geometry pass.
	createInfo.subpasses.push_back(makeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachmentRefs, nullptr));
	createInfo.dependencies.push_back(makeSubpassDependency(
		VK_SUBPASS_EXTERNAL,
		(uint32_t)SubPass::AERIAL_PERSPECTIVE,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT));
	createInfo.dependencies.push_back(makeSubpassDependency(
		(uint32_t)SubPass::AERIAL_PERSPECTIVE, 
		VK_SUBPASS_EXTERNAL, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT)
	);

	std::vector<VkImageView> attachmentImages = { nullptr };
	createRenderPass(createInfo, attachmentImages, true);
}

PostProcessPass::~PostProcessPass()
{
	SAFE_DELETE(aerialPerspectiveImageS_);
	SAFE_DELETE(aerialPerspectiveImageT_);
	SAFE_DELETE(gpColourBuffer_);
	SAFE_DELETE(gpDepthBuffer_);
	SAFE_DELETE(colourBuffer_);
	SAFE_DELETE(paramsBuf_);
	SAFE_DELETE(postProcessRenderer_);
	SAFE_DELETE(apScatteringPipeline_);
	SAFE_DELETE(apTransmittancePipeline_);
	SAFE_DELETE(atmosphereRenderer_);
}

void PostProcessPass::doFrame(const glm::mat4& viewMatrix, const uint32_t& idx, VkCommandBuffer cmdBuffer)
{	
	VkClearValue color = { 0.0f, 0.0f, 0.0f, 0.0f };
	auto bi = beginInfo(idx);
	bi.clearValueCount = 1;
	bi.pClearValues = &color;
	vkCmdBeginRenderPass(cmdBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE);

	atmosphereRenderer_->recordFrame(viewMatrix, idx, cmdBuffer);

	vkCmdEndRenderPass(cmdBuffer);
}

void PostProcessPass::initRenderPassDependency(std::vector<Image*> dependencyAttachment)
{
	ASSERT(dependencyAttachment.size() == 2);
	gpColourBuffer_ = new TextureSampler(logicDevice_, "gpColourBuffer", dependencyAttachment[0], VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);
	gpDepthBuffer_ = new TextureSampler(logicDevice_, "gpDepthBuffer", dependencyAttachment[1], VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);
	createRenderers();
}

void PostProcessPass::createRenderers()
{
	atmosphereRenderer_ = new AtmosphereRenderer(logicDevice_, graphicsMaster_->getMasters().assetManager->textureManager, renderPass_, swapChainDetails_.extent, descriptor_,
		"AtmosphereVert", "AtmosphereFrag", 1, globalRenderData_, gpColourBuffer_, gpDepthBuffer_);
	graphicsMaster_->setRenderer(RendererTypes::ATMOSPHERE, atmosphereRenderer_);
}

void PostProcessPass::createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	colourBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, swapChainDetails.surfaceFormat.format, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
}
