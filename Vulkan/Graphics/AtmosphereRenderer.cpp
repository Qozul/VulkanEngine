#include "AtmosphereRenderer.h"
#include "ElementBuffer.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "TextureSampler.h"
#include "TextureManager.h"
#include "DeviceMemory.h"
#include "RendererPipeline.h"
#include "GraphicsComponent.h"
#include "SwapChain.h"
#include "AtmosphereShaderParams.h"
#include "../Assets/AltAtmosphere.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace Graphics;

using PushConstantParams = MaterialAtmosphere;

struct PushConstantExtent {
	//glm::vec2 extent;
	//glm::vec2 padding;
	glm::mat4 inverseViewProj;
	MaterialAtmosphere mat;
};

DescriptorRequirementMap AtmosphereRenderer::getDescriptorRequirements()
{
	return { {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1} };
}

AtmosphereRenderer::AtmosphereRenderer(LogicDevice* logicDevice, TextureManager* textureManager, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
	const std::string& vertexShader, const std::string& tessCtrlShader, const std::string& tessEvalShader, const std::string& fragmentShader,
	const uint32_t entityCount, const GlobalRenderData* globalRenderData)
	: RendererBase(logicDevice), descriptor_(descriptor)
{
	ASSERT(entityCount > 0);
	renderStorage_ = new RenderStorage(new ElementBuffer<VertexOnlyPosition>(logicDevice->getDeviceMemory()));

	VkDescriptorSetLayoutBinding scatteringBinding = {};
	scatteringBinding.binding = 1;
	scatteringBinding.descriptorCount = 1;
	scatteringBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	scatteringBinding.pImmutableSamplers = nullptr;
	scatteringBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayout layout = descriptor->makeLayout({ scatteringBinding });

	pipelineLayouts_.push_back(layout);

	size_t idx = descriptor->createSets({ layout, layout, layout });
	std::vector<VkWriteDescriptorSet> descWrites;
	for (int i = 0; i < 3; ++i) {
		descriptorSets_.push_back(descriptor->getSet(idx + i));
	}
	descriptor->updateDescriptorSets(descWrites);

	std::vector<VkPushConstantRange> pushConstantRanges;
	pushConstantRanges.push_back(setupPushConstantRange<PushConstantExtent>(VK_SHADER_STAGE_FRAGMENT_BIT));
	createPipeline<VertexOnlyPosition>(logicDevice, renderPass, swapChainExtent, RendererPipeline::makeLayoutInfo(pipelineLayouts_.size(), pipelineLayouts_.data(), pushConstantRanges.size(),
		pushConstantRanges.data()), vertexShader, fragmentShader, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
}

AtmosphereRenderer::~AtmosphereRenderer()
{
}

void AtmosphereRenderer::recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	beginFrame(cmdBuffer);
	renderStorage_->buf()->bind(cmdBuffer);

	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		auto params = static_cast<AtmosphereShaderParams*>((*(renderStorage_->instanceData()) + i)->getShaderParams());
		auto material = params->material;
		material.cameraPosition = glm::vec3(0.0f, 10.0f, 0.0f);
		material.sunDirection = glm::vec3(1.0f, 1.0f, 0.0f);
		material.sunIntensity = glm::vec3(10.0f);

		PushConstantExtent pce;
		//pce.extent = glm::vec2(800.0f, 600.0f);
		pce.mat = material;
		pce.inverseViewProj = glm::inverse(GraphicsMaster::kProjectionMatrix * viewMatrix);

		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 1, &descriptorSets_[idx], 0, nullptr);

		vkCmdPushConstants(cmdBuffer, pipeline_->getLayout(), pushConstantInfos_[0].stages, pushConstantInfos_[0].offset, pushConstantInfos_[0].size, &pce);
		vkCmdPipelineBarrier(cmdBuffer, pushConstantInfos_[0].stages, pushConstantInfos_[0].stages, VK_DEPENDENCY_BY_REGION_BIT, 1, &pushConstantInfos_[0].barrier, 0, nullptr, 0, nullptr);
		//vkCmdPushConstants(cmdBuffer, pipeline_->getLayout(), pushConstantInfos_[1].stages, pushConstantInfos_[1].offset, pushConstantInfos_[1].size, &material);
		//vkCmdPipelineBarrier(cmdBuffer, pushConstantInfos_[1].stages, pushConstantInfos_[1].stages, VK_DEPENDENCY_BY_REGION_BIT, 1, &pushConstantInfos_[1].barrier, 0, nullptr, 0, nullptr);
		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.indexCount, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}

void AtmosphereRenderer::initialise(const glm::mat4& viewMatrix)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	auto instPtr = renderStorage_->instanceData();
	auto params = static_cast<AtmosphereShaderParams*>((*(instPtr))->getShaderParams()); 
	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(params->textures.scatteringSum->descriptorWrite(descriptorSets_[0], 1));
	descWrites.push_back(params->textures.scatteringSum->descriptorWrite(descriptorSets_[1], 1));
	descWrites.push_back(params->textures.scatteringSum->descriptorWrite(descriptorSets_[2], 1));
	descriptor_->updateDescriptorSets(descWrites);

	// Initialise the dynamic material and matrices
	/*ElementData* eleDataPtr = static_cast<ElementData*>(storageBuffers_[0]->bindRange());
	glm::mat4 model = (*(instPtr))->getEntity()->getTransform()->toModelMatrix();
	ElementData data = {
			model, GraphicsMaster::kProjectionMatrix * viewMatrix * model
	};
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		eleDataPtr[i] = data;
	}
	storageBuffers_[0]->unbindRange();*/
}

void AtmosphereRenderer::updateBuffers(const glm::mat4& viewMatrix)
{
	auto instPtr = renderStorage_->instanceData();
	auto params = static_cast<AtmosphereShaderParams*>((*(instPtr))->getShaderParams());

	ElementData* eleDataPtr = static_cast<ElementData*>(storageBuffers_[0]->bindRange());
	glm::mat4 model = (*(instPtr))->getEntity()->getTransform()->toModelMatrix();
	ElementData data = {
			model, GraphicsMaster::kProjectionMatrix * viewMatrix * model
	};
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		eleDataPtr[i] = data;
	}
	storageBuffers_[0]->unbindRange();
}

// Based on https://github.com/SaschaWillems/Vulkan/blob/master/base/frustum.hpp
std::array<glm::vec4, 6> AtmosphereRenderer::calculateFrustumPlanes(const glm::mat4 & matrix)
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
