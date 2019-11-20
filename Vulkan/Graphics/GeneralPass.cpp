// Author: Ralph Ridley
// Date: 01/11/19
#include "GeneralPass.h"
#include "GraphicsMaster.h"
#include "SwapChainDetails.h"
#include "TexturedRenderer.h"
#include "TerrainRenderer.h"
#include "WaterRenderer.h"
#include "ParticleRenderer.h"
#include "AtmosphereRenderer.h"
#include "Image.h"
#include "LogicDevice.h"
#include "SceneDescriptorInfo.h"
#include "GlobalRenderData.h"
#include "TextureManager.h"
#include "ElementBufferObject.h"

using namespace QZL;
using namespace QZL::Graphics;

GeometryPass::GeometryPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo)
	: RenderPass(master, logicDevice, swapChainDetails, grd, graphicsInfo)
{
	CreateInfo createInfo = {};
	createColourBuffer(logicDevice, swapChainDetails);
	auto depthFormat = createDepthBuffer(logicDevice, swapChainDetails);
	createInfo.attachments.push_back(makeAttachment(swapChainDetails.surfaceFormat.format, VK_SAMPLE_COUNT_8_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
	createInfo.attachments.push_back(makeAttachment(depthFormat, VK_SAMPLE_COUNT_8_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL));
	createInfo.attachments.push_back(makeAttachment(swapChainDetails.surfaceFormat.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

	std::vector<VkAttachmentReference> colourAttachmentRefs;
	VkAttachmentReference colourRef = {};
	colourRef.attachment = 0;
	colourRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colourAttachmentRefs.push_back(colourRef);
	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkAttachmentReference resolveRef = {};
	resolveRef.attachment = 2;
	resolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	createInfo.subpasses.push_back(makeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachmentRefs, &depthAttachmentRef, &resolveRef));
	
	createInfo.dependencies.push_back(makeSubpassDependency(
		VK_SUBPASS_EXTERNAL,
		0,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
	);
	createInfo.dependencies.push_back(makeSubpassDependency(
		0, 
		VK_SUBPASS_EXTERNAL, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT));

	std::vector<VkImageView> attachmentImages = { colourBuffer_->getImageView(), depthBuffer_->getImageView(), msaaResolveBuffer_->getImageView() };
	createRenderPass(createInfo, attachmentImages);
}

GeometryPass::~GeometryPass()
{
	SAFE_DELETE(depthBuffer_);
	SAFE_DELETE(colourBuffer_);
	SAFE_DELETE(msaaResolveBuffer_);
	SAFE_DELETE(terrainRenderer_);
	SAFE_DELETE(texturedRenderer_);
	SAFE_DELETE(atmosphereRenderer_);
	SAFE_DELETE(particleRenderer_);
	SAFE_DELETE(waterRenderer_);
	SAFE_DELETE(environmentRenderer_);
}

void GeometryPass::doFrame(FrameInfo& frameInfo)
{
	std::array<VkClearValue, 3> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };

	auto bi = beginInfo(frameInfo.frameIdx, { frameInfo.viewportWidth, swapChainDetails_.extent.height }, frameInfo.viewportX);
	bi.clearValueCount = static_cast<uint32_t>(clearValues.size());
	bi.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(frameInfo.cmdBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport;
	viewport.height = swapChainDetails_.extent.height;
	viewport.width = frameInfo.viewportWidth;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	viewport.x = frameInfo.viewportX;
	viewport.y = 0;
	vkCmdSetViewport(frameInfo.cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor;
	scissor.extent.width = frameInfo.viewportWidth;
	scissor.extent.height = swapChainDetails_.extent.height;
	scissor.offset.x = frameInfo.viewportX;
	scissor.offset.y = 0;
	vkCmdSetScissor(frameInfo.cmdBuffer, 0, 1, &scissor);

	const uint32_t dynamicOffsets[3] = {
		graphicsInfo_->mvpRange * (frameInfo.frameIdx + (graphicsInfo_->numFrameIndices * frameInfo.mainCameraIdx)),
		graphicsInfo_->paramsRange * frameInfo.frameIdx,
		graphicsInfo_->materialRange * frameInfo.frameIdx
	};

	VkDescriptorSet sets[2] = { graphicsInfo_->set, globalRenderData_->getSet() };
	vkCmdBindDescriptorSets(frameInfo.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, texturedRenderer_->getPipelineLayout(), 0, 2, sets, 3, dynamicOffsets);

	VertexPushConstants vpc;
	vpc.cameraPosition = glm::vec4(frameInfo.cameras[frameInfo.mainCameraIdx].position, 1.0f);
	vpc.mainLightPosition = frameInfo.cameras[1].position;
	vpc.shadowTextureIdx = shadowDepthTexture_;
	vpc.shadowMatrix = frameInfo.cameras[1].viewProjection;

	vkCmdPushConstants(frameInfo.cmdBuffer, texturedRenderer_->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vpc), &vpc);
	environmentRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, nullptr);
	atmosphereRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, &frameInfo.commandLists[(size_t)RendererTypes::kAtmosphere]);
	texturedRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, &frameInfo.commandLists[(size_t)RendererTypes::kStatic]);
	terrainRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, &frameInfo.commandLists[(size_t)RendererTypes::kTerrain]);
	particleRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, &frameInfo.commandLists[(size_t)RendererTypes::kParticle]);
	waterRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, &frameInfo.commandLists[(size_t)RendererTypes::kWater]);
	vkCmdEndRenderPass(frameInfo.cmdBuffer);
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
	createInfo.graphicsInfo = graphicsInfo_;

	createInfo.updateRendererSpecific(0, 1, "StaticVert", "StaticFrag");
	texturedRenderer_ = new TexturedRenderer(createInfo);

	createInfo.updateRendererSpecific(0, 1, "TerrainVert", "TerrainFrag", "", "TerrainTESC", "TerrainTESE");
	terrainRenderer_ = new TerrainRenderer(createInfo);

	createInfo.updateRendererSpecific(0, 1, "AtmosphereVert", "AtmosphereFrag");
	atmosphereRenderer_ = new AtmosphereRenderer(createInfo);

	createInfo.updateRendererSpecific(0, 2, "ParticlesVert", "ParticlesFrag", "ParticlesGeom");
	particleRenderer_ = new ParticleRenderer(createInfo);

	createInfo.updateRendererSpecific(0, 1, "WaterVert", "WaterFrag", "", "WaterTESC", "WaterTESE");
	waterRenderer_ = new WaterRenderer(createInfo);

	createInfo.updateRendererSpecific(0, 1, "AtmosphereVert", "EnvironmentFrag");
	environmentRenderer_ = new AtmosphereRenderer(createInfo);

	graphicsMaster_->setRenderer(RendererTypes::kParticle, particleRenderer_);
	graphicsMaster_->setRenderer(RendererTypes::kStatic, texturedRenderer_);
	graphicsMaster_->setRenderer(RendererTypes::kTerrain, terrainRenderer_);
	graphicsMaster_->setRenderer(RendererTypes::kAtmosphere, atmosphereRenderer_);
	graphicsMaster_->setRenderer(RendererTypes::kWater, waterRenderer_);
}

void GeometryPass::initRenderPassDependency(std::vector<Image*> dependencyAttachment)
{
	ASSERT(dependencyAttachment.size() == 1);
	shadowDepthBuf_ = dependencyAttachment[0];
	shadowDepthTexture_ = graphicsMaster_->getMasters().textureManager->allocateTexture("shadowDepth", shadowDepthBuf_, 
		{ VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, VK_SHADER_STAGE_FRAGMENT_BIT, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE });
	createRenderers();
}

void GeometryPass::createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	colourBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, swapChainDetails.surfaceFormat.format, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_8_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }, "GeometryColourBuffer");
	colourBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	msaaResolveBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, swapChainDetails.surfaceFormat.format, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }, "GeometryMSAAResolveBuffer");
	msaaResolveBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_SAMPLE_COUNT_8_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }, "GeometryDepthBuffer");
	depthBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	return imageFormat;
}
