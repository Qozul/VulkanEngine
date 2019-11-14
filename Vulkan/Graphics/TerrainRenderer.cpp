// Author: Ralph Ridley
// Date: 01/11/19
#include "TerrainRenderer.h"
#include "ElementBufferObject.h"
#include "RenderStorage.h"
#include "GlobalRenderData.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "RendererPipeline.h"
#include "GraphicsComponent.h"
#include "ShaderParams.h"
#include "RenderObject.h"
#include "Material.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace Graphics;

TerrainRenderer::TerrainRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, new RenderStorage(new ElementBufferObject(createInfo.logicDevice->getDeviceMemory(), sizeof(Vertex), 
		sizeof(uint16_t)), RenderStorage::InstanceUsage::kUnlimited))
{
	descriptorSets_.push_back(createInfo.globalRenderData->getSet());
	createDescriptors(createInfo.maxDrawnEntities);
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());
	//pipelineLayouts_.push_back(TerrainMaterial::getLayout(descriptor_));

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);
	stageInfos.emplace_back(createInfo.tessControlShader, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, nullptr);
	stageInfos.emplace_back(createInfo.tessEvalShader, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, nullptr);

	PipelineCreateInfo pci = {};
	pci.debugName = "Terrain";
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_TRUE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	pci.subpassIndex = createInfo.subpassIndex;

	createPipeline<Vertex>(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()), 
		pipelineLayouts_.data()), stageInfos, pci, RendererPipeline::PrimitiveType::kQuads);
}

TerrainRenderer::~TerrainRenderer()
{
}

void TerrainRenderer::createDescriptors(const uint32_t entityCount)
{
	DescriptorBuffer* mvpBuf = DescriptorBuffer::makeBuffer<StorageBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 0, 0,
		sizeof(ElementData) * 2, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "TerrainMVPBuffer"); // 2 = MAX_FRAMES_IN_FLIGHT
	DescriptorBuffer* matBuf = DescriptorBuffer::makeBuffer<StorageBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 1, 0,
		sizeof(StaticShaderParams::Params) * 2, VK_SHADER_STAGE_FRAGMENT_BIT, "TerrainParamsBuffer");
	DescriptorBuffer* tessBuf = DescriptorBuffer::makeBuffer<UniformBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 2, 0,
		sizeof(TessControlInfo) * 2, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "TerrainTessCtrlBuffer");
	storageBuffers_.push_back(mvpBuf);
	storageBuffers_.push_back(matBuf);
	storageBuffers_.push_back(tessBuf);
	DescriptorBuffer* diBuf = nullptr;
	diBuf = DescriptorBuffer::makeBuffer<StorageBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 3, 0,
		sizeof(uint32_t) * 3, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, "TerrainDIBuffer");
	storageBuffers_.push_back(diBuf);
	VkDescriptorSetLayout layout = descriptor_->makeLayout({ mvpBuf->getBinding(), matBuf->getBinding(), tessBuf->getBinding(), diBuf->getBinding() });

	pipelineLayouts_.push_back(layout);

	size_t idx = descriptor_->createSets({ layout, layout, layout });
	std::vector<VkWriteDescriptorSet> descWrites;
	for (int i = 0; i < 3; ++i) {
		descriptorSets_.push_back(descriptor_->getSet(idx + i));
		descWrites.push_back(mvpBuf->descriptorWrite(descriptor_->getSet(idx + i)));
		descWrites.push_back(matBuf->descriptorWrite(descriptor_->getSet(idx + i)));
		descWrites.push_back(tessBuf->descriptorWrite(descriptor_->getSet(idx + i)));
		descWrites.push_back(diBuf->descriptorWrite(descriptor_->getSet(idx + i)));
	}
	descriptor_->updateDescriptorSets(descWrites);
}

void TerrainRenderer::recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	beginFrame(cmdBuffer);
	renderStorage_->buffer()->bind(cmdBuffer, idx);

	updateBuffers(camera);
	uint32_t* dataPtr = static_cast<uint32_t*>(storageBuffers_[3]->bindRange());
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); i += 2) {
		dataPtr[i] = static_cast<TerrainMaterial*>((*(instPtr + i))->getMaterial())->heightmap_;
		dataPtr[i + 1] = static_cast<TerrainMaterial*>((*(instPtr + i))->getMaterial())->normalmap_;
		dataPtr[i + 2] = static_cast<TerrainMaterial*>((*(instPtr + i))->getMaterial())->diffuse_;
	}
	storageBuffers_[3]->unbindRange();
	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];

		VkDescriptorSet sets[2] = { descriptorSets_[1 + (size_t)idx], descriptorSets_[0] };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 2, sets, 0, nullptr);

		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}

void TerrainRenderer::updateBuffers(LogicalCamera& camera)
{
	ElementData* eleDataPtr = static_cast<ElementData*>(storageBuffers_[0]->bindRange());
	TessControlInfo* tcPtr = static_cast<TessControlInfo*>(storageBuffers_[2]->bindRange());

	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		glm::mat4 model = (*(instPtr + i))->getEntity()->getModelMatrix();
		glm::mat4 mvp = GraphicsMaster::kProjectionMatrix * camera.viewMatrix * model;
		eleDataPtr[i] = {
			model, mvp
		};
		tcPtr[i].distanceFarMinusClose = 300.0f; // Implies far distance is 350.0f+
		tcPtr[i].closeDistance = 50.0f;
		tcPtr[i].patchRadius = 40.0f;
		tcPtr[i].maxTessellationWeight = 4.0f;
		camera.calculateFrustumPlanes(mvp, tcPtr[i].frustumPlanes);
	}
	storageBuffers_[2]->unbindRange();
	storageBuffers_[0]->unbindRange();

	StaticShaderParams::Params* matDataPtr = static_cast<StaticShaderParams::Params*>(storageBuffers_[1]->bindRange());
	for (size_t i = 0; i < renderStorage_->meshCount(); ++i) {
		RenderObject* robject = renderStorage_->renderObjectData()[i];
		matDataPtr[i] = static_cast<StaticShaderParams*>(robject->getParams())->params;
	}
	storageBuffers_[1]->unbindRange();
}
