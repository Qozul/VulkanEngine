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
		(uint32_t)SubPass::kAtmosphere,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
	);
	createInfo.dependencies.push_back(makeSubpassDependency(
		(uint32_t)SubPass::kAtmosphere,
		(uint32_t)SubPass::kGeneral,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
	);
	createInfo.dependencies.push_back(makeSubpassDependency(
		(uint32_t)SubPass::kGeneral, 
		VK_SUBPASS_EXTERNAL, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT));

	std::vector<VkImageView> attachmentImages = { colourBuffer_->getImageView(), depthBuffer_->getImageView() };
	createRenderPass(createInfo, attachmentImages, false);
}

GeometryPass::~GeometryPass()
{
	SAFE_DELETE(depthBuffer_);
	SAFE_DELETE(colourBuffer_);
	SAFE_DELETE(terrainRenderer_);
	SAFE_DELETE(texturedRenderer_);
	SAFE_DELETE(atmosphereRenderer_);
}

void GeometryPass::doFrame(LogicalCamera& camera, const uint32_t& idx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandLists)
{
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	auto bi = beginInfo(idx);
	bi.clearValueCount = static_cast<uint32_t>(clearValues.size());
	bi.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(cmdBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE);

	const uint32_t dynamicOffsets[3] = {
		graphicsInfo_->mvpRange * idx,
		graphicsInfo_->paramsRange * idx,
		graphicsInfo_->materialRange * idx
	};

	VkDescriptorSet sets[2] = { graphicsInfo_->set, globalRenderData_->getSet() };
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, texturedRenderer_->getPipelineLayout(), 0, 2, sets, 3, dynamicOffsets);

	/*VertexPushConstants vpc;
	vpc.mainLightPosition = ; // TODO get mainLight position
	vpc.shadowMatrix = ; // TODO get cameras_[1].viewProjection
	vpc.shadowTextureIdx = shadowDepthTexture_;*/

	CameraPushConstants pc;
	pc.cameraPosition = glm::vec4(camera.position, 1.0f);
	vkCmdPushConstants(cmdBuffer, texturedRenderer_->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec4), &pc);

	TessellationPushConstants pcs;
	pcs.distanceFarMinusClose = 300.0f; // Implies far distance is 350.0f+
	pcs.closeDistance = 50.0f;
	pcs.patchRadius = 40.0f;
	pcs.maxTessellationWeight = 4.0f;
	camera.calculateFrustumPlanes(camera.viewProjection, pcs.frustumPlanes); // TODO calculate in terrain update and use as shader params

	vkCmdPushConstants(cmdBuffer, texturedRenderer_->getPipelineLayout(), VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, sizeof(glm::vec4), sizeof(TessellationPushConstants), &pcs);
	atmosphereRenderer_->recordFrame(camera, idx, cmdBuffer, &commandLists[(size_t)RendererTypes::kAtmosphere]);

	vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
	terrainRenderer_->recordFrame(camera, idx, cmdBuffer, &commandLists[(size_t)RendererTypes::kTerrain]);
	texturedRenderer_->recordFrame(camera, idx, cmdBuffer, &commandLists[(size_t)RendererTypes::kStatic]);

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
	createInfo.graphicsInfo = graphicsInfo_;

	createInfo.updateRendererSpecific(1, 1, "StaticVert", logicDevice_->supportsOptionalExtension(OptionalExtensions::kDescriptorIndexing) ? "StaticFrag_DI" : "StaticFrag");
	texturedRenderer_ = new TexturedRenderer(createInfo);

	createInfo.updateRendererSpecific(1, 1, "TerrainVert", "TerrainFrag", "", "TerrainTESC", "TerrainTESE");
	terrainRenderer_ = new TerrainRenderer(createInfo);

	createInfo.updateRendererSpecific(0, 1, "AtmosphereVert", "AtmosphereFrag");
	atmosphereRenderer_ = new AtmosphereRenderer(createInfo);

	graphicsMaster_->setRenderer(RendererTypes::kStatic, texturedRenderer_);
	graphicsMaster_->setRenderer(RendererTypes::kTerrain, terrainRenderer_);
	graphicsMaster_->setRenderer(RendererTypes::kAtmosphere, atmosphereRenderer_);
}

void GeometryPass::initRenderPassDependency(std::vector<Image*> dependencyAttachment)
{
	ASSERT(dependencyAttachment.size() == 1);
	shadowDepthBuf_ = dependencyAttachment[0];
	shadowDepthTexture_ = graphicsMaster_->getMasters().textureManager->allocateTexture("shadowDepth", shadowDepthBuf_);
	createRenderers();
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
