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
	: RendererBase(logicDevice), descriptor_(descriptor)
{
	ASSERT(entityCount > 0);
	renderStorage_ = new RenderStorage(new ElementBuffer<Vertex>(logicDevice->getDeviceMemory()), RenderStorage::InstanceUsage::UNLIMITED);

	DescriptorBuffer* mvpBuf = DescriptorBuffer::makeBuffer<StorageBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 0, 0,
		sizeof(ElementData) * MAX_FRAMES_IN_FLIGHT, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
	DescriptorBuffer* matBuf = DescriptorBuffer::makeBuffer<StorageBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 1, 0,
		sizeof(StaticShaderParams::Params) * MAX_FRAMES_IN_FLIGHT, VK_SHADER_STAGE_FRAGMENT_BIT);
	DescriptorBuffer* tessBuf = DescriptorBuffer::makeBuffer<UniformBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 2, 0,
		sizeof(TessControlInfo) * MAX_FRAMES_IN_FLIGHT, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
	storageBuffers_.push_back(mvpBuf);
	storageBuffers_.push_back(matBuf);
	storageBuffers_.push_back(tessBuf);
	VkDescriptorSetLayout layout;
	/*VkDescriptorSetLayoutBinding heightmapBinding = {};
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
	debugDiffuseBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;*/
	layout = descriptor->makeLayout({ mvpBuf->getBinding(), matBuf->getBinding(), tessBuf->getBinding() });

	pipelineLayouts_.push_back(layout);
	pipelineLayouts_.push_back(globalRenderData->getLayout());
	pipelineLayouts_.push_back(TerrainMaterial::getLayout(descriptor));

	size_t idx = descriptor->createSets({ layout, layout, layout });
	std::vector<VkWriteDescriptorSet> descWrites;
	for (int i = 0; i < 3; ++i) {
		descriptorSets_.push_back(descriptor->getSet(idx + i));
		descriptorSets_.push_back(globalRenderData->getSet());
		descWrites.push_back(mvpBuf->descriptorWrite(descriptor->getSet(idx + i)));
		descWrites.push_back(matBuf->descriptorWrite(descriptor->getSet(idx + i)));
		descWrites.push_back(tessBuf->descriptorWrite(descriptor->getSet(idx + i)));
	}
	descriptor->updateDescriptorSets(descWrites);

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

		VkDescriptorSet sets[3] = { descriptorSets_[idx * 2], descriptorSets_[idx * 2 + 1], robject->getMaterial()->getTextureSet() };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 3, sets, 0, nullptr);

		//vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 2, &descriptorSets_[idx * 2], 0, nullptr);
		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}

void TerrainRenderer::initialise(const glm::mat4& viewMatrix)
{
	/*if (renderStorage_->instanceCount() == 0)
		return;
	ElementData* eleDataPtr = static_cast<ElementData*>(storageBuffers_[0]->bindRange());
	StaticShaderParams::Params* matDataPtr = static_cast<StaticShaderParams::Params*>(storageBuffers_[1]->bindRange());
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
	storageBuffers_[1]->unbindRange();*/
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
