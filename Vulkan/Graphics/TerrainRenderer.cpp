#include "TerrainRenderer.h"
#include "ElementBuffer.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "TextureSampler.h"
#include "TextureLoader.h"
#include "DeviceMemory.h"
#include "RendererPipeline.h"
#include "GraphicsComponent.h"
#include "TerrainShaderParams.h"
#include "TerrainRenderStorage.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace Graphics;

TerrainRenderer::TerrainRenderer(const LogicDevice* logicDevice, TextureLoader*& textureLoader, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
	const std::string& vertexShader, const std::string& tessCtrlShader, const std::string& tessEvalShader, const std::string& fragmentShader, 
	const uint32_t entityCount)
	: RendererBase(new TerrainRenderStorage(textureLoader, logicDevice)), descriptor_(descriptor)
{
	ASSERT(entityCount > 0);
	StorageBuffer* mvpBuf = new StorageBuffer(logicDevice, MemoryAllocationPattern::kDynamicResource, 0, 0,
		sizeof(ElementData) * entityCount, VK_SHADER_STAGE_VERTEX_BIT);

	VkDescriptorSetLayoutBinding heightmapBinding = {};
	heightmapBinding.binding = 1;
	heightmapBinding.descriptorCount = 1;
	heightmapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	heightmapBinding.pImmutableSamplers = nullptr;
	heightmapBinding.stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

	VkDescriptorSetLayoutBinding debugDiffuseBinding = {};
	debugDiffuseBinding.binding = 3;
	debugDiffuseBinding.descriptorCount = 1;
	debugDiffuseBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	debugDiffuseBinding.pImmutableSamplers = nullptr;
	debugDiffuseBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	auto layout = descriptor->makeLayout({ mvpBuf->getBinding(), heightmapBinding, debugDiffuseBinding });
	storageBuffers_.push_back(mvpBuf);
	size_t idx = descriptor->createSets({ layout, layout, layout });
	std::vector<VkWriteDescriptorSet> descWrites;
	for (int i = 0; i < 3; ++i) {
		descriptorSets_.push_back(descriptor->getSet(idx + i));
		descWrites.push_back(mvpBuf->descriptorWrite(descriptor->getSet(idx + i)));
	}
	descriptor->updateDescriptorSets(descWrites);

	createPipeline(logicDevice, renderPass, swapChainExtent, RendererPipeline::makeLayoutInfo(storageBuffers_.size(), &layout), vertexShader, fragmentShader);
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
	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];

		auto trs = static_cast<TerrainRenderStorage*>(renderStorage_);
		std::vector<VkWriteDescriptorSet> descWrites;
		descWrites.push_back(trs->getParamData(i).heightMap->descriptorWrite(descriptorSets_[idx]));
		descWrites.push_back(trs->getParamData(i).diffuse->descriptorWrite(descriptorSets_[idx]));
		descriptor_->updateDescriptorSets(descWrites);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 1, &descriptorSets_[idx], 0, nullptr);

		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.indexCount, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}

void TerrainRenderer::initialise(const glm::mat4& viewMatrix)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	ElementData* eleDataPtr = static_cast<ElementData*>(storageBuffers_[0]->bindRange());
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		glm::mat4 model = (*(instPtr + i))->getEntity()->getTransform()->toModelMatrix();
		eleDataPtr[i] = {
			model, GraphicsMaster::kProjectionMatrix * viewMatrix * model
		};
	}
	storageBuffers_[0]->unbindRange();
}
