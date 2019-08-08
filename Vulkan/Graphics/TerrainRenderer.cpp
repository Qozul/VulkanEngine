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
#include "TerrainShaderParams.h"
#include "TerrainRenderStorage.h"
#include "SwapChain.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace Graphics;

TerrainRenderer::TerrainRenderer(LogicDevice* logicDevice, TextureManager* textureManager, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
	const std::string& vertexShader, const std::string& tessCtrlShader, const std::string& tessEvalShader, const std::string& fragmentShader, 
	const uint32_t entityCount, const GlobalRenderData* globalRenderData)
	: RendererBase(logicDevice), descriptor_(descriptor)
{
	ASSERT(entityCount > 0);
	renderStorage_ = new TerrainRenderStorage(textureManager, logicDevice, new ElementBuffer<Vertex>(logicDevice->getDeviceMemory()));

	StorageBuffer* mvpBuf = new StorageBuffer(logicDevice, MemoryAllocationPattern::kDynamicResource, (uint32_t)ReservedGraphicsBindings0::PER_ENTITY_DATA, 0,
		sizeof(ElementData) * MAX_FRAMES_IN_FLIGHT, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
	StorageBuffer* matBuf = new StorageBuffer(logicDevice, MemoryAllocationPattern::kDynamicResource, (uint32_t)ReservedGraphicsBindings0::MATERIAL_DATA, 0,
		sizeof(MaterialStatic) * MAX_FRAMES_IN_FLIGHT, VK_SHADER_STAGE_FRAGMENT_BIT);
	StorageBuffer* tessBuf = new StorageBuffer(logicDevice, MemoryAllocationPattern::kDynamicResource, (uint32_t)ReservedGraphicsBindings0::UNRESERVED, 0,
		sizeof(TessControlInfo) * MAX_FRAMES_IN_FLIGHT, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, true);
	storageBuffers_.push_back(mvpBuf);
	storageBuffers_.push_back(matBuf);
	storageBuffers_.push_back(tessBuf);
	VkDescriptorSetLayout layout;
	VkDescriptorSetLayoutBinding heightmapBinding = {};
	heightmapBinding.binding = (uint32_t)ReservedGraphicsBindings0::TEXTURE_0;
	heightmapBinding.descriptorCount = 1;
	heightmapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	heightmapBinding.pImmutableSamplers = nullptr;
	heightmapBinding.stageFlags = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

	VkDescriptorSetLayoutBinding debugDiffuseBinding = {};
	debugDiffuseBinding.binding = (uint32_t)ReservedGraphicsBindings0::TEXTURE_1;
	debugDiffuseBinding.descriptorCount = 1;
	debugDiffuseBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	debugDiffuseBinding.pImmutableSamplers = nullptr;
	debugDiffuseBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	layout = descriptor->makeLayout({ mvpBuf->getBinding(), matBuf->getBinding(), tessBuf->getBinding(), heightmapBinding, debugDiffuseBinding });

	pipelineLayouts_.push_back(layout);
	pipelineLayouts_.push_back(globalRenderData->layout);

	size_t idx = descriptor->createSets({ layout, layout, layout });
	std::vector<VkWriteDescriptorSet> descWrites;
	for (int i = 0; i < 3; ++i) {
		descriptorSets_.push_back(descriptor->getSet(idx + i));
		descriptorSets_.push_back(globalRenderData->globalDataDescriptor->getSet(globalRenderData->setIdx));
		descWrites.push_back(mvpBuf->descriptorWrite(descriptor->getSet(idx + i)));
		descWrites.push_back(matBuf->descriptorWrite(descriptor->getSet(idx + i)));
		descWrites.push_back(tessBuf->descriptorWrite(descriptor->getSet(idx + i)));
	}
	descriptor->updateDescriptorSets(descWrites);

	createPipeline(logicDevice, renderPass, swapChainExtent, RendererPipeline::makeLayoutInfo(pipelineLayouts_.size(), pipelineLayouts_.data()), vertexShader, fragmentShader, tessCtrlShader, tessEvalShader);
}

TerrainRenderer::~TerrainRenderer()
{
}

void TerrainRenderer::recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	beginFrame(cmdBuffer);
	renderStorage_->buf()->bind(cmdBuffer);

	updateBuffers(viewMatrix);

	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];

		auto srs = static_cast<TerrainRenderStorage*>(renderStorage_);
		std::vector<VkWriteDescriptorSet> descWrites;
		descWrites.push_back(srs->getParamData(i).heightMap->descriptorWrite(descriptorSets_[idx * 2]));
		descWrites.push_back(srs->getParamData(i).diffuse->descriptorWrite(descriptorSets_[idx * 2]));
		descriptor_->updateDescriptorSets(descWrites);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 2, &descriptorSets_[idx * 2], 0, nullptr);
		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.indexCount, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}

void TerrainRenderer::initialise(const glm::mat4& viewMatrix)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	ElementData* eleDataPtr = static_cast<ElementData*>(storageBuffers_[0]->bindRange());
	MaterialStatic* matDataPtr = static_cast<MaterialStatic*>(storageBuffers_[1]->bindRange());
	auto instPtr = renderStorage_->instanceData();
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		matDataPtr[i] = static_cast<TerrainShaderParams*>((*(instPtr))->getShaderParams())->getMaterial();
	}
	glm::mat4 model = (*(instPtr))->getEntity()->getTransform()->toModelMatrix();
	ElementData data = {
			model, GraphicsMaster::kProjectionMatrix * viewMatrix * model
	};
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		eleDataPtr[i] = data;
	}
	storageBuffers_[0]->unbindRange();
	storageBuffers_[1]->unbindRange();
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
		tcPtr[i].distanceFarMinusClose = 300.0f; // Implies far distance is 500.0f+
		tcPtr[i].closeDistance = 50.0f;
		tcPtr[i].patchRadius = 40.0f;
		tcPtr[i].maxTessellationWeight = 16.0f;
		tcPtr[i].frustumPlanes = calculateFrustumPlanes(mvp);
	}

	storageBuffers_[2]->unbindRange();
	storageBuffers_[0]->unbindRange();
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
