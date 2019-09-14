#include "ParticleRenderer.h"
#include "DynamicVertexBuffer.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "TextureSampler.h"
#include "ParticleShaderParams.h"

using namespace QZL;
using namespace QZL::Graphics;

struct PushConstantGeometry {
	glm::mat4 mvp;
	glm::vec3 billboardPoint;
	float tileLength;
};

struct PushConstantFragment {
	glm::vec4 tint;
};

ParticleRenderer::ParticleRenderer(LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor, 
	const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader, const uint32_t particleSystemCount, const GlobalRenderData* globalRenderData,
	glm::vec3* billboardPoint)
	: RendererBase(logicDevice), descriptor_(descriptor), billboardPoint_(billboardPoint)
{
	ASSERT(particleSystemCount > 0);
	renderStorage_ = new RenderStorage(new ElementBuffer<ParticleVertex>(logicDevice->getDeviceMemory()));

	auto texBinding = TextureSampler::makeBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayout layout = descriptor->makeLayout({ texBinding });

	pipelineLayouts_.push_back(layout);
	descriptorSets_.push_back(descriptor->getSet(descriptor->createSets({ layout })));

	std::vector<VkPushConstantRange> pushConstantRanges;
	pushConstantRanges.push_back(setupPushConstantRange<PushConstantGeometry>(VK_SHADER_STAGE_GEOMETRY_BIT));
	pushConstantRanges.push_back(setupPushConstantRange<PushConstantFragment>(VK_SHADER_STAGE_FRAGMENT_BIT));
	createPipeline<VertexOnlyPosition>(logicDevice, renderPass, swapChainExtent, RendererPipeline::makeLayoutInfo(pipelineLayouts_.size(), pipelineLayouts_.data(), pushConstantRanges.size(),
		pushConstantRanges.data()), vertexShader, fragmentShader, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, false);
}

ParticleRenderer::~ParticleRenderer()
{
}

void ParticleRenderer::recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	beginFrame(cmdBuffer);
	static_cast<VertexBufferInterface*>(renderStorage_->buf())->bind(cmdBuffer);

	// TODO move mvp matrix to uniform buffer so that instances can be used
	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		auto component = (*(renderStorage_->instanceData()) + i);
		auto params = static_cast<ParticleShaderParams*>(component->getShaderParams());
		auto material = params->material;

		PushConstantGeometry pcg;
		pcg.mvp = GraphicsMaster::kProjectionMatrix * viewMatrix * component->getModelmatrix();
		pcg.billboardPoint = *billboardPoint_;
		pcg.tileLength = material.textureTileLength;

		PushConstantFragment pcf;
		pcf.tint = material.tint;

		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 1, &descriptorSets_[0], 0, nullptr);

		vkCmdPushConstants(cmdBuffer, pipeline_->getLayout(), pushConstantInfos_[0].stages, pushConstantInfos_[0].offset, pushConstantInfos_[0].size, &pcg);
		vkCmdPipelineBarrier(cmdBuffer, pushConstantInfos_[0].stages, pushConstantInfos_[0].stages, VK_DEPENDENCY_BY_REGION_BIT, 1, &pushConstantInfos_[0].barrier, 0, nullptr, 0, nullptr);

		vkCmdDraw(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}

	/* TODO: 
		Render storage using a persistently mapped vertex buffer (rather than element buffer)
		- Need a new type of "ElementBuffer": VertexBuffer to deal with the lack of indices
		- Render Storafe Vertex buffer needs a pointer to each particle system's vertex buffer
		- The render storage vertex buffer must be double/triple buffered

		- Need to modify deviceMemory stuff to allow for persistent mapping, also need to check that the feature is enabled/supported
		- Need to modify swapchain to provide the number of swap chain images created (rather than just using '3' hard coded)

		Reference:
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			particles.size,
			&particles.buffer,
			&particles.memory,
			particleBuffer.data()));

		// Map the memory and store the pointer for reuse
		VK_CHECK_RESULT(vkMapMemory(device, particles.memory, 0, particles.size, 0, &particles.mappedMemory));
	
	*/
}

void ParticleRenderer::initialise(const glm::mat4& viewMatrix)
{
	if (renderStorage_->instanceCount() == 0)
		return;
}
