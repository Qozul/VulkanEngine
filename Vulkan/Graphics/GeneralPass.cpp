// Author: Ralph Ridley
// Date: 01/11/19
#include "GeneralPass.h"
#include "GraphicsMaster.h"
#include "SwapChainDetails.h"
#include "TexturedRenderer.h"
#include "TerrainRenderer.h"
#include "WaterRenderer.h"
#include "FullscreenRenderer.h"
#include "AtmosphereRenderer.h"
#include "Image.h"
#include "LogicDevice.h"
#include "SceneDescriptorInfo.h"
#include "GlobalRenderData.h"
#include "TextureManager.h"
#include "ElementBufferObject.h"

using namespace QZL;
using namespace QZL::Graphics;

CombinePass::CombinePass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo)
	: RenderPass(master, logicDevice, swapChainDetails, grd, graphicsInfo)
{
	CreateInfo createInfo = {};
	createColourBuffer(logicDevice, swapChainDetails);
	// Forward rendering colour and depth
	createInfo.attachments.push_back(makeAttachment(swapChainDetails.surfaceFormat.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

	std::vector<VkAttachmentReference> colourAttachmentRefs;
	VkAttachmentReference colourRef = {};
	colourRef.attachment = 0;
	colourRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colourAttachmentRefs.push_back(colourRef);

	createInfo.subpasses.push_back(makeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachmentRefs, nullptr));
	
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

	std::vector<VkImageView> attachmentImages = { colourBuffer_->getImageView() };
	createRenderPass(createInfo, attachmentImages);
}

CombinePass::~CombinePass()
{
	SAFE_DELETE(colourBuffer_);
	SAFE_DELETE(atmosphereRenderer_);
	SAFE_DELETE(environmentRenderer_);
	SAFE_DELETE(combineRenderer_);
}

void CombinePass::doFrame(FrameInfo& frameInfo)
{
	std::array<VkClearValue, 1> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };

	auto bi = beginInfo(frameInfo.frameIdx, { frameInfo.viewportWidth, swapChainDetails_.extent.height }, frameInfo.viewportX);
	bi.clearValueCount = static_cast<uint32_t>(clearValues.size());
	bi.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(frameInfo.cmdBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE);

	environmentRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, nullptr);
	atmosphereRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, nullptr);
	combineRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, nullptr);
	vkCmdEndRenderPass(frameInfo.cmdBuffer);
}

void CombinePass::createRenderers()
{
	RendererCreateInfo createInfo = {};
	createInfo.logicDevice = logicDevice_;
	createInfo.descriptor = descriptor_;
	createInfo.extent = swapChainDetails_.extent;
	createInfo.renderPass = renderPass_;
	createInfo.globalRenderData = globalRenderData_;
	createInfo.swapChainImageCount = swapChainDetails_.images.size();
	createInfo.graphicsInfo = graphicsInfo_;
	createInfo.subpassIndex = 0;
	createInfo.colourAttachmentCount = 1;
	createInfo.colourBlendEnables = { VK_TRUE };

	createInfo.updateRendererSpecific(0, 1, "AtmosphereVert", "AtmosphereFrag");
	atmosphereRenderer_ = new AtmosphereRenderer(createInfo);

	createInfo.updateRendererSpecific(0, 1, "AtmosphereVert", "EnvironmentFrag");
	environmentRenderer_ = new AtmosphereRenderer(createInfo);

	VkPushConstantRange pushConstants[1] = {
		RendererBase::setupPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(VertexPushConstants), 0)
	};

	struct Vals {
		uint32_t diffuseIdx;
		uint32_t specularIdx;
		uint32_t albedoIdx;
	} specConstantValues;
	specConstantValues.diffuseIdx = diffuseIdx_;
	specConstantValues.specularIdx = specularIdx_;
	specConstantValues.albedoIdx = albedoIdx_;

	std::vector<VkSpecializationMapEntry> specEntries = {
		RendererBase::makeSpecConstantEntry(0, 0, sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(1, sizeof(uint32_t), sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t))
	};

	VkSpecializationInfo specializationInfo2 = RendererBase::setupSpecConstants(3, specEntries.data(), sizeof(Vals), &specConstantValues);
	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back("FullscreenVert", VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back("DeferredLightingCombineFrag", VK_SHADER_STAGE_FRAGMENT_BIT, &specializationInfo2);

	PipelineCreateInfo pci = {};
	pci.debugName = "Combine";
	pci.enableDepthTest = VK_FALSE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	pci.subpassIndex = createInfo.subpassIndex;
	pci.dynamicState = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	pci.sampleCount = VK_SAMPLE_COUNT_1_BIT;

	RendererCreateInfo2 createInfo2;
	createInfo2.shaderStages = stageInfos;
	createInfo2.pipelineCreateInfo = pci;
	createInfo2.pcRangesCount = 1;
	createInfo2.pcRanges = pushConstants;

	createInfo.updateRendererSpecific(0, 1, "FullscreenVert", "DeferredLightingCombineFrag");
	combineRenderer_ = new FullscreenRenderer(createInfo, createInfo2);

	graphicsMaster_->setRenderer(RendererTypes::kAtmosphere, atmosphereRenderer_);
}

void CombinePass::initRenderPassDependency(std::vector<Image*> dependencyAttachment)
{
	ASSERT(dependencyAttachment.size() == 3);
	diffuseIdx_ = graphicsMaster_->getMasters().textureManager->allocateTexture("DiffuseSampler", dependencyAttachment[0],
		{ VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, VK_SHADER_STAGE_FRAGMENT_BIT, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE });
	specularIdx_ = graphicsMaster_->getMasters().textureManager->allocateTexture("SpecularSampler", dependencyAttachment[1],
		{ VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, VK_SHADER_STAGE_FRAGMENT_BIT, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE });
	albedoIdx_ = graphicsMaster_->getMasters().textureManager->allocateTexture("AlbedoSampler", dependencyAttachment[2],
		{ VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, VK_SHADER_STAGE_FRAGMENT_BIT, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE });
	createRenderers();
}

void CombinePass::createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	colourBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, swapChainDetails.surfaceFormat.format, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "GeometryColourBuffer");
	colourBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}
