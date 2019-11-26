#include "DeferredPass.h"
#include "GraphicsMaster.h"
#include "SwapChainDetails.h"
#include "IndexedRenderer.h"
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

DeferredPass::DeferredPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo)
	: RenderPass(master, logicDevice, swapChainDetails, grd, graphicsInfo)
{
	CreateInfo createInfo = {};
	createColourBuffer(logicDevice, swapChainDetails);
	auto depthFormat = createDepthBuffer(logicDevice, swapChainDetails);
	createInfo.attachments.reserve(4);

	// G-buffer: Position, normals, and albedo information
	createInfo.attachments.push_back(makeAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	createInfo.attachments.push_back(makeAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	createInfo.attachments.push_back(makeAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	createInfo.attachments.push_back(makeAttachment(depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 3;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::vector<VkAttachmentReference> deferredColourAttachmentRefs;
	VkAttachmentReference positionsRef = {};
	positionsRef.attachment = 0;
	positionsRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	deferredColourAttachmentRefs.push_back(positionsRef);
	VkAttachmentReference normalsRef = {};
	normalsRef.attachment = 1;
	normalsRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	deferredColourAttachmentRefs.push_back(normalsRef);
	VkAttachmentReference albedoRef = {};
	albedoRef.attachment = 2;
	albedoRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	deferredColourAttachmentRefs.push_back(albedoRef);

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

	createInfo.subpasses.push_back(makeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, deferredColourAttachmentRefs, &depthAttachmentRef));

	std::vector<VkImageView> attachmentImages = { positionBuffer_->getImageView(), normalsBuffer_->getImageView(), albedoBuffer_->getImageView(), depthBuffer_->getImageView() };
	createRenderPass(createInfo, attachmentImages);
	createRenderers();
}

DeferredPass::~DeferredPass()
{
	SAFE_DELETE(depthBuffer_);
	SAFE_DELETE(normalsBuffer_);
	SAFE_DELETE(positionBuffer_);
	SAFE_DELETE(albedoBuffer_);
	SAFE_DELETE(staticRenderer_);
	SAFE_DELETE(terrainRenderer_);
	SAFE_DELETE(particleRenderer_);
	SAFE_DELETE(waterRenderer_);
}

void DeferredPass::doFrame(FrameInfo& frameInfo)
{
	std::array<VkClearValue, 4> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[1].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[2].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[3].depthStencil = { 1.0f, 0 };

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
	vkCmdBindDescriptorSets(frameInfo.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, staticRenderer_->getPipelineLayout(), 0, 2, sets, 3, dynamicOffsets);

	VertexPushConstants vpc;
	vpc.cameraPosition = glm::vec4(frameInfo.cameras[frameInfo.mainCameraIdx].position, 1.0f);
	vpc.mainLightPosition = frameInfo.cameras[1].position;
	vpc.shadowTextureIdx = shadowDepthIdx_;
	vpc.shadowMatrix = frameInfo.cameras[1].viewProjection;
	vkCmdPushConstants(frameInfo.cmdBuffer, terrainRenderer_->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vpc), &vpc);

	staticRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, &frameInfo.commandLists[(size_t)RendererTypes::kStatic]);
	waterRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, &frameInfo.commandLists[(size_t)RendererTypes::kWater]);
	terrainRenderer_->recordFrame(frameInfo.frameIdx, frameInfo.cmdBuffer, &frameInfo.commandLists[(size_t)RendererTypes::kTerrain]);
	vkCmdEndRenderPass(frameInfo.cmdBuffer);
}

void DeferredPass::createRenderers()
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
	createInfo.colourAttachmentCount = 3;
	createInfo.colourBlendEnables = { VK_FALSE, VK_FALSE, VK_FALSE };
	createInfo.updateRendererSpecific(0, 1, "StaticVert", "StaticDeferredFrag");

	VkPushConstantRange pushConstants[2] = {
		RendererBase::setupPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(VertexPushConstants), 0),
		RendererBase::setupPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(FragmentPushConstants), sizeof(VertexPushConstants))
	};

	struct Vals {
		uint32_t mvpOffset;
		uint32_t paramsOffset;
		uint32_t matOffset;
	} specConstantValues;
	specConstantValues.mvpOffset = graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kStatic];
	specConstantValues.paramsOffset = graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kStatic];
	specConstantValues.matOffset = graphicsInfo_->materialOffsetSizes[(size_t)RendererTypes::kStatic];

	std::vector<VkSpecializationMapEntry> specEntries = {
		RendererBase::makeSpecConstantEntry(0, 0, sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(1, sizeof(uint32_t), sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t))
	};

	VkSpecializationInfo specializationInfo = RendererBase::setupSpecConstants(2, specEntries.data(), sizeof(uint32_t) * 2, &specConstantValues.mvpOffset);
	VkSpecializationInfo specializationInfo2 = RendererBase::setupSpecConstants(2, specEntries.data(), sizeof(uint32_t) * 2, &specConstantValues.paramsOffset);
	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back("StaticVert", VK_SHADER_STAGE_VERTEX_BIT, &specializationInfo);
	stageInfos.emplace_back("StaticDeferredFrag", VK_SHADER_STAGE_FRAGMENT_BIT, &specializationInfo2);

	PipelineCreateInfo pci = {};
	pci.debugName = "Statics";
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_TRUE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pci.subpassIndex = createInfo.subpassIndex;
	pci.dynamicState = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	pci.sampleCount = VK_SAMPLE_COUNT_1_BIT;
	pci.colourBlendEnables = { VK_FALSE, VK_FALSE, VK_TRUE };
	pci.colourAttachmentCount = 3;

	RendererCreateInfo2 createInfo2;
	createInfo2.shaderStages = stageInfos;
	createInfo2.pipelineCreateInfo = pci;
	createInfo2.pcRangesCount = 2;
	createInfo2.pcRanges = pushConstants;
	createInfo2.ebo = new ElementBufferObject(createInfo.logicDevice->getDeviceMemory(), sizeof(Vertex), sizeof(uint16_t));

	staticRenderer_ = new IndexedRenderer(createInfo, createInfo2);



	uint32_t offsets[3] = { graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kTerrain], graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kTerrain], graphicsInfo_->materialOffsetSizes[(size_t)RendererTypes::kTerrain] };
	std::vector<VkSpecializationMapEntry> mapEntryTerrain = {
		RendererBase::makeSpecConstantEntry(0, 0,	sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(1, sizeof(uint32_t), sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t))
	};
	auto vertSpecConstant = RendererBase::setupSpecConstants(1, mapEntryTerrain.data(), sizeof(uint32_t), &offsets[1]);
	auto tescSpecConstant = RendererBase::setupSpecConstants(2, mapEntryTerrain.data(), sizeof(uint32_t) * 2, &offsets[1]);
	auto teseSpecConstant = RendererBase::setupSpecConstants(3, mapEntryTerrain.data(), sizeof(uint32_t) * 3, offsets);
	auto fragSpecConstant = RendererBase::setupSpecConstants(2, mapEntryTerrain.data(), sizeof(uint32_t) * 2, &offsets[1]);
	auto geomSpecConstant = RendererBase::setupSpecConstants(2, mapEntryTerrain.data(), sizeof(uint32_t) * 2, &offsets);

	std::vector<ShaderStageInfo> stageInfosTerrain;
	stageInfosTerrain.emplace_back("TerrainVert", VK_SHADER_STAGE_VERTEX_BIT, &vertSpecConstant);
	stageInfosTerrain.emplace_back("TerrainFrag", VK_SHADER_STAGE_FRAGMENT_BIT, &fragSpecConstant);
	stageInfosTerrain.emplace_back("TerrainTESC", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, &tescSpecConstant);
	stageInfosTerrain.emplace_back("TerrainTESE", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, &teseSpecConstant);
	stageInfosTerrain.emplace_back("TerrainGeom", VK_SHADER_STAGE_GEOMETRY_BIT, &geomSpecConstant);

	pci.debugName = "Terrain";
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	createInfo2.pipelineCreateInfo = pci;
	createInfo2.ebo = new ElementBufferObject(createInfo.logicDevice->getDeviceMemory(), sizeof(Vertex), sizeof(uint16_t));
	createInfo2.shaderStages = stageInfosTerrain;
	terrainRenderer_ = new IndexedRenderer(createInfo, createInfo2);



	uint32_t offsetsWater[3] = { graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kWater], graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kWater], graphicsInfo_->materialOffsetSizes[(size_t)RendererTypes::kWater] };
	std::vector<VkSpecializationMapEntry> mapEntryWater = {
		RendererBase::makeSpecConstantEntry(0, 0,	sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(1, sizeof(uint32_t), sizeof(uint32_t)),
		RendererBase::makeSpecConstantEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t))
	};
	auto vertSpecConstantWater = RendererBase::setupSpecConstants(1, mapEntryWater.data(), sizeof(uint32_t), &offsetsWater[1]);
	auto tescSpecConstantWater = RendererBase::setupSpecConstants(1, mapEntryWater.data(), sizeof(uint32_t), &offsetsWater[1]);
	auto teseSpecConstantWater = RendererBase::setupSpecConstants(3, mapEntryWater.data(), sizeof(uint32_t) * 3, offsetsWater);
	auto fragSpecConstantWater = RendererBase::setupSpecConstants(1, mapEntryWater.data(), sizeof(uint32_t), &offsetsWater[1]);

	std::vector<ShaderStageInfo> stageInfosWater;
	stageInfosWater.emplace_back("WaterVert", VK_SHADER_STAGE_VERTEX_BIT, &vertSpecConstantWater);
	stageInfosWater.emplace_back("WaterFrag", VK_SHADER_STAGE_FRAGMENT_BIT, &fragSpecConstantWater);
	stageInfosWater.emplace_back("WaterTESC", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, &tescSpecConstantWater);
	stageInfosWater.emplace_back("WaterTESE", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, &teseSpecConstantWater);

	pci.debugName = "Water";
	createInfo2.pipelineCreateInfo = pci;
	createInfo2.ebo = new ElementBufferObject(createInfo.logicDevice->getDeviceMemory(), sizeof(Vertex), sizeof(uint16_t));
	createInfo2.shaderStages = stageInfosWater;
	waterRenderer_ = new IndexedRenderer(createInfo, createInfo2);

	graphicsMaster_->setRenderer(RendererTypes::kStatic, staticRenderer_);
	graphicsMaster_->setRenderer(RendererTypes::kParticle, nullptr);
	graphicsMaster_->setRenderer(RendererTypes::kTerrain, terrainRenderer_);
	graphicsMaster_->setRenderer(RendererTypes::kWater, waterRenderer_);
}

void DeferredPass::initRenderPassDependency(std::vector<Image*> dependencyAttachment)
{
}

void DeferredPass::createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	positionBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "DeferredPositionsBuffer");
	positionBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalsBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "DeferredNormalsBuffer");
	normalsBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedoBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "DeferredAlbedoBuffer");
	albedoBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

VkFormat DeferredPass::createDepthBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails)
{
	depthBuffer_ = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, swapChainDetails.depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, swapChainDetails.extent.width, swapChainDetails.extent.height, 1),
		MemoryAllocationPattern::kRenderTarget, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }, "GeometryDepthBuffer");
	depthBuffer_->getImageInfo().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	return swapChainDetails.depthFormat;
}
