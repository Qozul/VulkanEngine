#include "TerrainRenderer.h"
#include "ElementBuffer.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "TextureSampler.h"
#include "TextureManager.h"
#include "DeviceMemory.h"
#include "RendererPipeline.h"
#include "GraphicsComponent.h"
#include "ShaderParams.h"
#include "RenderObject.h"
#include "Material.h"
#include "SwapChain.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace Graphics;

TerrainRenderer::TerrainRenderer(LogicDevice* logicDevice, TextureManager* textureManager, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
	const std::string& vertexShader, const std::string& tessCtrlShader, const std::string& tessEvalShader, const std::string& fragmentShader, 
	const uint32_t entityCount, const GlobalRenderData* globalRenderData)
	: RendererBase(logicDevice, new RenderStorage(new ElementBuffer<Vertex>(logicDevice->getDeviceMemory()), RenderStorage::InstanceUsage::UNLIMITED)), descriptor_(descriptor)
{
	ASSERT(entityCount > 0);

	descriptorSets_.push_back(globalRenderData->getSet());
	createDescriptors(entityCount);
	pipelineLayouts_.push_back(globalRenderData->getLayout());
	pipelineLayouts_.push_back(TerrainMaterial::getLayout(descriptor_));

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);
	stageInfos.emplace_back(tessCtrlShader, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, nullptr);
	stageInfos.emplace_back(tessEvalShader, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, nullptr);

	PipelineCreateInfo pci = {};
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_TRUE;
	pci.extent = swapChainExtent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

	createPipeline<Vertex>(logicDevice, renderPass, RendererPipeline::makeLayoutInfo(pipelineLayouts_.size(), pipelineLayouts_.data()), stageInfos, pci,
		RendererPipeline::PrimitiveType::QUADS);
}

TerrainRenderer::~TerrainRenderer()
{
}

void TerrainRenderer::createDescriptors(const uint32_t entityCount)
{
	DescriptorBuffer* mvpBuf = DescriptorBuffer::makeBuffer<StorageBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 0, 0,
		sizeof(ElementData) * MAX_FRAMES_IN_FLIGHT, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
	DescriptorBuffer* matBuf = DescriptorBuffer::makeBuffer<StorageBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 1, 0,
		sizeof(StaticShaderParams::Params) * MAX_FRAMES_IN_FLIGHT, VK_SHADER_STAGE_FRAGMENT_BIT);
	DescriptorBuffer* tessBuf = DescriptorBuffer::makeBuffer<UniformBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 2, 0,
		sizeof(TessControlInfo) * MAX_FRAMES_IN_FLIGHT, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
	storageBuffers_.push_back(mvpBuf);
	storageBuffers_.push_back(matBuf);
	storageBuffers_.push_back(tessBuf);
	VkDescriptorSetLayout layout;
	layout = descriptor_->makeLayout({ mvpBuf->getBinding(), matBuf->getBinding(), tessBuf->getBinding() });

	pipelineLayouts_.push_back(layout);

	size_t idx = descriptor_->createSets({ layout, layout, layout });
	std::vector<VkWriteDescriptorSet> descWrites;
	for (int i = 0; i < 3; ++i) {
		descriptorSets_.push_back(descriptor_->getSet(idx + i));
		descWrites.push_back(mvpBuf->descriptorWrite(descriptor_->getSet(idx + i)));
		descWrites.push_back(matBuf->descriptorWrite(descriptor_->getSet(idx + i)));
		descWrites.push_back(tessBuf->descriptorWrite(descriptor_->getSet(idx + i)));
	}
	descriptor_->updateDescriptorSets(descWrites);
}

void TerrainRenderer::recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	beginFrame(cmdBuffer);
	static_cast<ElementBufferInterface*>(renderStorage_->buf())->bind(cmdBuffer, idx);

	updateBuffers(viewMatrix);

	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];

		VkDescriptorSet sets[3] = { descriptorSets_[1 + idx], descriptorSets_[0], robject->getMaterial()->getTextureSet() };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 3, sets, 0, nullptr);

		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}

void TerrainRenderer::updateBuffers(const glm::mat4& viewMatrix)
{
	ElementData* eleDataPtr = static_cast<ElementData*>(storageBuffers_[0]->bindRange());
	TessControlInfo* tcPtr = static_cast<TessControlInfo*>(storageBuffers_[2]->bindRange());

	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		glm::mat4 model = (*(instPtr + i))->getEntity()->getTransform()->toModelMatrix();
		glm::mat4 mvp = GraphicsMaster::kProjectionMatrix * viewMatrix * model;
		eleDataPtr[i] = {
			model, mvp
		};
		tcPtr[i].distanceFarMinusClose = 300.0f; // Implies far distance is 350.0f+
		tcPtr[i].closeDistance = 50.0f;
		tcPtr[i].patchRadius = 40.0f;
		tcPtr[i].maxTessellationWeight = 4.0f;
		tcPtr[i].frustumPlanes = calculateFrustumPlanes(mvp);
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

// Based on https://github.com/SaschaWillems/Vulkan/blob/master/base/frustum.hpp
std::array<glm::vec4, 6> TerrainRenderer::calculateFrustumPlanes(const glm::mat4& matrix)
{
	std::array<glm::vec4, 6> planes;
	float n = 0.0f;
	for (int p = 0; p < 6; ++p) {
		for (int i = 0; i < 4; ++i) {
			planes[p][i] = matrix[i].w + matrix[i][(int)n];
		}
		n += 0.51f; // No need to worry about error over the 6 iterations used, but ensures integer cast is always right
		// Normalize the plane
		float length = std::sqrt(planes[p].x * planes[p].x + planes[p].y * planes[p].y + planes[p].z * planes[p].z);
		planes[p] /= length;
	}
	return planes;
}
