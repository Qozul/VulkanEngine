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

using namespace QZL;
using namespace QZL::Graphics;

PostProcessPass::PostProcessPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd)
	: RenderPass(master, logicDevice, swapChainDetails, grd)
{
	// Setup aerial perspective compute.
	aerialPerspectiveImageS_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_3D, 1, 1, swapChainDetails.surfaceFormat.format, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, AP_WIDTH, AP_HEIGHT, AP_DEPTH),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED });
	aerialPerspectiveImageT_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_3D, 1, 1, swapChainDetails.surfaceFormat.format, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, AP_WIDTH, AP_HEIGHT, AP_DEPTH),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED });
	
	apScattering_ = new TextureSampler(logicDevice, "ApScatteringSampler", aerialPerspectiveImageS_, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);
	apTransmittance_ = new TextureSampler(logicDevice, "ApTransmittanceSampler", aerialPerspectiveImageT_, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

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
}

void PostProcessPass::doFrame(const glm::mat4& viewMatrix, const uint32_t& idx, VkCommandBuffer cmdBuffer)
{
	pushConstants_.inverseViewProj = glm::inverse(GraphicsMaster::kProjectionMatrix * viewMatrix);
	pushConstants_.cameraHeight = 1.0f;
	pushConstants_.sunDir = glm::vec3(1.0, 0.0, 0.0);
	pushConstants_.zenithDir = glm::vec3(0.0, 1.0, 0.0);
	pushConstants_.camPos = graphicsMaster_->getCamPos();

	vkCmdPipelineBarrier(cmdBuffer, VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_COMPUTE_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 2, &memoryBarriers_[0]);

	auto set = descriptor_->getSet(computeDescriptorIdx_);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, apScatteringPipeline_->getLayout(), 0, 1, &set, 0, nullptr);

	vkCmdPushConstants(cmdBuffer, apScatteringPipeline_->getLayout(), pushConstantInfo_.stages, pushConstantInfo_.offset, pushConstantInfo_.size, &pushConstants_);
	vkCmdPipelineBarrier(cmdBuffer, pushConstantInfo_.stages, pushConstantInfo_.stages, VK_DEPENDENCY_BY_REGION_BIT, 1, &pushConstantInfo_.barrier, 0, nullptr, 0, nullptr);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, apScatteringPipeline_->getPipeline());
	vkCmdDispatch(cmdBuffer, AP_WIDTH / INVOCATION_SIZE, AP_HEIGHT / INVOCATION_SIZE, AP_DEPTH / INVOCATION_SIZE);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, apTransmittancePipeline_->getPipeline());
	vkCmdDispatch(cmdBuffer, AP_WIDTH / INVOCATION_SIZE, AP_HEIGHT / INVOCATION_SIZE, AP_DEPTH / INVOCATION_SIZE);

	vkCmdPipelineBarrier(cmdBuffer, VK_SHADER_STAGE_COMPUTE_BIT, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 0, nullptr, 0, nullptr, 2, &memoryBarriers_[2]);
	
	VkClearValue color = { 0.0f, 0.0f, 0.0f, 1.0f };
	auto bi = beginInfo(idx);
	bi.clearValueCount = 1;
	bi.pClearValues = &color;
	vkCmdBeginRenderPass(cmdBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE);

	postProcessRenderer_->recordFrame(viewMatrix, idx, cmdBuffer);

	vkCmdEndRenderPass(cmdBuffer);
}

void PostProcessPass::createRenderers()
{
	postProcessRenderer_ = new PostProcessRenderer(logicDevice_, renderPass_, swapChainDetails_.extent, descriptor_, "PPVert", "PPFrag", aerialPerspectiveImageS_, aerialPerspectiveImageT_,
		gpColourBuffer_, gpDepthBuffer_);
}

void PostProcessPass::initRenderPassDependency(std::vector<Image*> dependencyAttachment)
{
	ASSERT(dependencyAttachment.size() == 2);
	gpColourBuffer_ = new TextureSampler(logicDevice_, "gpColourBuffer", dependencyAttachment[0], VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);
	gpDepthBuffer_ = new TextureSampler(logicDevice_, "gpDepthBuffer", dependencyAttachment[1], VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);
	createRenderers();
	postProcessRenderer_->preframeSetup(graphicsMaster_->getViewMatrix());
}

void PostProcessPass::createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	colourBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, swapChainDetails.surfaceFormat.format, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
}

void PostProcessPass::createComputePipelines()
{
	// Pipelines share layout to half the necessary descriptor binds
	paramsBuf_ = DescriptorBuffer::makeBuffer<UniformBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 0, 0,
		sizeof(Assets::AtmosphereParameters), VK_SHADER_STAGE_COMPUTE_BIT);
	auto imageBinding0 = Image::makeBinding(1, VK_SHADER_STAGE_COMPUTE_BIT);
	auto imageBinding1 = Image::makeBinding(2, VK_SHADER_STAGE_COMPUTE_BIT);
	auto texBinding = TextureSampler::makeBinding(3, VK_SHADER_STAGE_COMPUTE_BIT);
	auto layout = descriptor_->makeLayout({ imageBinding0, imageBinding1, texBinding, paramsBuf_->getBinding() });
	computeDescriptorIdx_ = descriptor_->createSets({ layout });
	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(paramsBuf_->descriptorWrite(descriptor_->getSet(computeDescriptorIdx_)));
	descWrites.push_back(apTransmittance_->descriptorWrite(descriptor_->getSet(computeDescriptorIdx_), 1));
	descWrites.push_back(apScattering_->descriptorWrite(descriptor_->getSet(computeDescriptorIdx_), 2));
	descWrites.push_back(atmosphereScript_->getTextures().gatheringSum->descriptorWrite(descriptor_->getSet(computeDescriptorIdx_), 3));
	descriptor_->updateDescriptorSets(descWrites);

	Assets::AtmosphereParameters* paramsPtr = static_cast<Assets::AtmosphereParameters*>(paramsBuf_->bindRange());
	*paramsPtr = atmosphereScript_->getParameters();
	paramsBuf_->unbindRange();

	auto pcr = RendererBase::createPushConstantRange<ComputePushConstants>(VK_SHADER_STAGE_COMPUTE_BIT, 0);
	pushConstantInfo_ = pcr.second;
	std::vector<VkPushConstantRange> pushConstantRanges;
	pushConstantRanges.push_back(pcr.first);
	apScatteringPipeline_ = new ComputePipeline(logicDevice_, ComputePipeline::makeLayoutInfo(1, &layout, pushConstantRanges), "AtmosphereAerialPerspectiveS");
	apTransmittancePipeline_ = new ComputePipeline(logicDevice_, ComputePipeline::makeLayoutInfo(1, &layout, pushConstantRanges), "AtmosphereAerialPerspectiveT");
	VkImageSubresourceRange subRange = {};
	subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subRange.baseArrayLayer = 0;
	subRange.baseMipLevel = 1;
	subRange.layerCount = 1;
	subRange.levelCount = 1;
	memoryBarriers_[0] = Image::makeMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL, logicDevice_->getFamilyIndex(QueueFamilyType::kGraphicsQueue), logicDevice_->getFamilyIndex(QueueFamilyType::kGraphicsQueue),
		aerialPerspectiveImageS_->getImage(), subRange);
	memoryBarriers_[1] = Image::makeMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL, logicDevice_->getFamilyIndex(QueueFamilyType::kGraphicsQueue), logicDevice_->getFamilyIndex(QueueFamilyType::kGraphicsQueue),
		aerialPerspectiveImageT_->getImage(), subRange);
	memoryBarriers_[2] = Image::makeMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, 
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, logicDevice_->getFamilyIndex(QueueFamilyType::kGraphicsQueue), logicDevice_->getFamilyIndex(QueueFamilyType::kGraphicsQueue),
		aerialPerspectiveImageS_->getImage(), subRange);
	memoryBarriers_[3] = Image::makeMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, 
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, logicDevice_->getFamilyIndex(QueueFamilyType::kGraphicsQueue), logicDevice_->getFamilyIndex(QueueFamilyType::kGraphicsQueue),
		aerialPerspectiveImageT_->getImage(), subRange);
}
