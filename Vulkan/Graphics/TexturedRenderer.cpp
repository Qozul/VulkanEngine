#include "TexturedRenderer.h"
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
#include "Material.h"
#include "RenderObject.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace QZL::Graphics;

TexturedRenderer::TexturedRenderer(LogicDevice* logicDevice, TextureManager* textureManager, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
	const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount, const GlobalRenderData* globalRenderData)
	: RendererBase(logicDevice), descriptor_(descriptor)
{
	ASSERT(entityCount > 0);
	renderStorage_ = new RenderStorage(new ElementBuffer<Vertex>(logicDevice->getDeviceMemory()), RenderStorage::InstanceUsage::UNLIMITED);

	DescriptorBuffer* mvpBuf = DescriptorBuffer::makeBuffer<StorageBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 0, 0,
		sizeof(ElementData) * entityCount, VK_SHADER_STAGE_VERTEX_BIT);
	DescriptorBuffer* matBuf = DescriptorBuffer::makeBuffer<StorageBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 1, 0,
		sizeof(StaticShaderParams::Params) * entityCount, VK_SHADER_STAGE_FRAGMENT_BIT);
	storageBuffers_.push_back(mvpBuf);
	storageBuffers_.push_back(matBuf);

	VkDescriptorSetLayout layout;
	if (!logicDevice->supportsOptionalExtension(OptionalExtensions::DESCRIPTOR_INDEXING)) {
		VkDescriptorSetLayoutBinding diffuseBinding = {};
		diffuseBinding.binding = 2;
		diffuseBinding.descriptorCount = 1;
		diffuseBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		diffuseBinding.pImmutableSamplers = nullptr;
		diffuseBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding normalMapBinding = {};
		normalMapBinding.binding = 3;
		normalMapBinding.descriptorCount = 1;
		normalMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		normalMapBinding.pImmutableSamplers = nullptr;
		normalMapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layout = descriptor->makeLayout({ mvpBuf->getBinding(), matBuf->getBinding(), diffuseBinding, normalMapBinding });
	}
	else {
		layout = descriptor->makeLayout({ mvpBuf->getBinding(), matBuf->getBinding() });
	}

	pipelineLayouts_.push_back(layout);
	pipelineLayouts_.push_back(globalRenderData->getLayout());

	size_t idx = descriptor->createSets({ layout, layout, layout });
	std::vector<VkWriteDescriptorSet> descWrites;
	for (int i = 0; i < 3; ++i) {
		descriptorSets_.push_back(descriptor->getSet(idx + i));
		descriptorSets_.push_back(globalRenderData->getSet());
		descWrites.push_back(mvpBuf->descriptorWrite(descriptor->getSet(idx + i)));
		descWrites.push_back(matBuf->descriptorWrite(descriptor->getSet(idx + i)));
	}
	descriptor->updateDescriptorSets(descWrites);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

	PipelineCreateInfo pci = {};
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_TRUE;
	pci.extent = swapChainExtent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	createPipeline<Vertex>(logicDevice, renderPass, RendererPipeline::makeLayoutInfo(pipelineLayouts_.size(), pipelineLayouts_.data()), stageInfos, pci);
}

TexturedRenderer::~TexturedRenderer()
{
}

void TexturedRenderer::initialise(const glm::mat4& viewMatrix)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	StaticShaderParams::Params* matDataPtr = static_cast<StaticShaderParams::Params*>(storageBuffers_[1]->bindRange());
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		matDataPtr[i] = static_cast<StaticShaderParams*>((*(instPtr + i))->getShaderParams())->params;
	}
	storageBuffers_[1]->unbindRange();
}

void TexturedRenderer::recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	beginFrame(cmdBuffer);
	static_cast<ElementBufferInterface*>(renderStorage_->buf())->bind(cmdBuffer, idx);

	ElementData* eleDataPtr = static_cast<ElementData*>(storageBuffers_[0]->bindRange());
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		glm::mat4 model = (*(instPtr + i))->getEntity()->getTransform()->toModelMatrix();
		eleDataPtr[i] = {
			model, GraphicsMaster::kProjectionMatrix * viewMatrix * model
		};
	}
	storageBuffers_[0]->unbindRange();

	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];

		if (!logicDevice_->supportsOptionalExtension(OptionalExtensions::DESCRIPTOR_INDEXING)) {
			// TODO, the choice here is to get the material from the robject (DI not supported) or from the instance.
			// it may be best to have 2 cmd loop functions because they may change the flow notably.

			VkDescriptorSet sets[3] = { descriptorSets_[idx * 2], descriptorSets_[idx * 2 + 1], robject->getMaterial()->getTextureSet() };
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 3, sets, 0, nullptr);
		}
		else {
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 2, &descriptorSets_[static_cast<size_t>(idx) * 2], 0, nullptr);
		}
		
		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}
