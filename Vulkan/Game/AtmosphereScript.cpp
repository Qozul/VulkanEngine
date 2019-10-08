#include "AtmosphereScript.h"
#include "../Graphics/GraphicsMaster.h"
#include "../Graphics/logicDevice.h"
#include "../Graphics/PhysicalDevice.h"
#include "../Graphics/TextureManager.h"
#include "../Graphics/TextureSampler.h"
#include "../Graphics/ComputePipeline.h"
#include "../Graphics/Shader.h"
#include "../Graphics/Image.h"
#include "../Graphics/DeviceMemory.h"
#include "../Graphics/StorageBuffer.h"
#include "../Graphics/Descriptor.h"
#include "../System.h"
#include "../Graphics/RenderPass.h"

using namespace QZL;
using namespace QZL::Game;
using namespace QZL::Graphics;

AtmosphereScript::AtmosphereScript(const GameScriptInitialiser& initialiser)
	: GameScript(initialiser)
{
	logicDevice_ = initialiser.system->getMasters().graphicsMaster->getLogicDevice();

	params_.betaRay = glm::vec3(6.55e-6f, 1.73e-5f, 2.30e-5f);
	params_.betaMie = 2.2e-6f;
	params_.betaMieExt = params_.betaMie / 0.9f;
	params_.planetRadius = 6371e3f;
	params_.Hatm = 80000.0f;
	params_.mieScaleHeight = 1200.0f;
	params_.rayleighScaleHeight = 8000.0f;
	params_.betaOzoneExt = glm::vec3(5.09f, 7.635f, 0.2545f);

	material_.betaRay = params_.betaRay;
	material_.betaMie = params_.betaMie;
	material_.planetRadius = params_.planetRadius;
	material_.Hatm = params_.Hatm;
	material_.g = 0.76f;
}

AtmosphereScript::~AtmosphereScript()
{
	SAFE_DELETE(textures_.scattering);
	SAFE_DELETE(textures_.scatteringImage);
	SAFE_DELETE(textures_.transmittance);
	SAFE_DELETE(textures_.transmittanceImage);
	SAFE_DELETE(textures_.gathering);
	SAFE_DELETE(textures_.gatheringImage);
	SAFE_DELETE(textures_.gatheringSum);
	SAFE_DELETE(textures_.gatheringSumImage);
	SAFE_DELETE(textures_.scatteringSum);
	SAFE_DELETE(textures_.scatteringSumImage);
}

void AtmosphereScript::start()
{
	// See http://publications.lib.chalmers.se/records/fulltext/203057/203057.pdf for reference.
	// Create the textures. Need a total of 8 textures, with 3 permanent for rendering and 5 temporary.
	initTextures(logicDevice_, textures_);

	// Create the uniform buffer for required parameters.
	DescriptorBuffer* buffer = DescriptorBuffer::makeBuffer<UniformBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 0, 0,
		sizeof(Assets::AtmosphereParameters), VK_SHADER_STAGE_COMPUTE_BIT);

	// Setup descriptor set, each shader has identical descriptor even if not used.
	auto descriptor = logicDevice_->getPrimaryDescriptor();

	VkDescriptorSetLayoutBinding transmittance = makeLayoutBinding(1);
	VkDescriptorSetLayoutBinding scattering = makeLayoutBinding(2);
	VkDescriptorSetLayoutBinding gathering = makeLayoutBinding(3);
	VkDescriptorSetLayoutBinding gatheringSum = makeLayoutBinding(4);
	VkDescriptorSetLayoutBinding scatteringSum = makeLayoutBinding(5);
	VkDescriptorSetLayout layout = descriptor->makeLayout({ buffer->getBinding(), transmittance, scattering, gathering, gatheringSum, scatteringSum });
	size_t setIdx = descriptor->createSets({ layout });
	auto set = descriptor->getSet(setIdx);

	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(buffer->descriptorWrite(descriptor->getSet(setIdx)));
	// TODO need to write using the storage images not the samplers, perhaps need both available when appropriate.
	descWrites.push_back(textures_.transmittance->descriptorWrite(descriptor->getSet(setIdx), 1));
	descWrites.push_back(textures_.scattering->descriptorWrite(descriptor->getSet(setIdx), 2));
	descWrites.push_back(textures_.gathering->descriptorWrite(descriptor->getSet(setIdx), 3));
	descWrites.push_back(textures_.gatheringSum->descriptorWrite(descriptor->getSet(setIdx), 4));
	descWrites.push_back(textures_.scatteringSum->descriptorWrite(descriptor->getSet(setIdx), 5));
	descriptor->updateDescriptorSets(descWrites);

	// Write the buffer data.
	Assets::AtmosphereParameters* precompData = static_cast<Assets::AtmosphereParameters*>(buffer->bindRange());
	*precompData = params_;
	buffer->unbindRange();

	// Create the compute pipelines.
	ComputePipeline transmittancePipeline = ComputePipeline(logicDevice_, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereAltTransmittance");
	ComputePipeline singleScatteringPipeline = ComputePipeline(logicDevice_, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereAltScattering");
	ComputePipeline gatheringPipeline = ComputePipeline(logicDevice_, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereAltGathering");
	ComputePipeline multipleScatteringPipeline = ComputePipeline(logicDevice_, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereAltMultipleScattering");

	// Record command buffer and execute on compute queue.
	// TODO optimise workgroup stuff: https://stackoverflow.com/questions/54750009/compute-shader-and-workgroup
	VkCommandBuffer cmdBuffer = logicDevice_->getComputeCommandBuffer();

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	CHECK_VKRESULT(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, transmittancePipeline.getLayout(), 0, 1, &set, 0, nullptr);

	// 1. Compute Transmittance
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, transmittancePipeline.getPipeline());
	vkCmdDispatch(cmdBuffer, TRANSMITTANCE_TEXTURE_WIDTH / INVOCATION_SIZE, TRANSMITTANCE_TEXTURE_HEIGHT / INVOCATION_SIZE, 1);

	// 2. Compute Single Scattering
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, singleScatteringPipeline.getPipeline());
	vkCmdDispatch(cmdBuffer, SCATTERING_TEXTURE_WIDTH / INVOCATION_SIZE, SCATTERING_TEXTURE_HEIGHT / INVOCATION_SIZE, SCATTERING_TEXTURE_DEPTH / INVOCATION_SIZE);

	for (unsigned int scatteringOrder = 2; scatteringOrder <= 4; ++scatteringOrder) {
		// 3. Compute gathered light
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, gatheringPipeline.getPipeline());
		vkCmdDispatch(cmdBuffer, GATHERING_TEXTURE_WIDTH / INVOCATION_SIZE, GATHERING_TEXTURE_HEIGHT / INVOCATION_SIZE, 1);

		// 4. Compute Multiple Scattering
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, multipleScatteringPipeline.getPipeline());
		vkCmdDispatch(cmdBuffer, SCATTERING_TEXTURE_WIDTH / INVOCATION_SIZE, SCATTERING_TEXTURE_HEIGHT / INVOCATION_SIZE, SCATTERING_TEXTURE_DEPTH / INVOCATION_SIZE);
	}
	CHECK_VKRESULT(vkEndCommandBuffer(cmdBuffer));

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	CHECK_VKRESULT(vkQueueSubmit(logicDevice_->getQueueHandle(QueueFamilyType::kComputeQueue), 1, &submitInfo, VK_NULL_HANDLE));
	// TODO proper sync rather than just wait idle
	CHECK_VKRESULT(vkQueueWaitIdle(logicDevice_->getQueueHandle(QueueFamilyType::kComputeQueue)));

	// TODO Transfer image memory from writeable to read only optimal, it will never need to be written again.
	/*textures_.irradianceImage->changeLayout({ VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	textures_.transmittanceImage->changeLayout({ VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	textures_.scatteringImage->changeLayout({ VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });*/

	SAFE_DELETE(buffer);

	sysMasters_->graphicsMaster->attachPostProcessScript(this);
}

void AtmosphereScript::initTextures(const LogicDevice* logicDevice, PrecomputedTextures& finalTextures)
{
	finalTextures.transmittanceImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT),
		MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL });
	finalTextures.transmittance = finalTextures.transmittanceImage->createTextureSampler("AtmosTransmittance", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	finalTextures.gatheringImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, GATHERING_TEXTURE_WIDTH, GATHERING_TEXTURE_HEIGHT),
		MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL });
	finalTextures.gathering = finalTextures.gatheringImage->createTextureSampler("AtmosGathering", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	finalTextures.gatheringSumImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, GATHERING_TEXTURE_WIDTH, GATHERING_TEXTURE_HEIGHT),
		MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL });
	finalTextures.gatheringSum = finalTextures.gatheringSumImage->createTextureSampler("AtmosGatheringSum", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	finalTextures.scatteringImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_3D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH),
		MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL });
	finalTextures.scattering = finalTextures.scatteringImage->createTextureSampler("AtmosScattering", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	finalTextures.scatteringSumImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_3D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH),
		MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL });
	finalTextures.scatteringSum = finalTextures.scatteringSumImage->createTextureSampler("AtmosScatteringSum", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);
}

VkDescriptorSetLayoutBinding AtmosphereScript::makeLayoutBinding(const uint32_t binding, const VkSampler* immutableSamplers)
{
	VkDescriptorSetLayoutBinding layoutBinding;
	layoutBinding.binding = binding;
	layoutBinding.descriptorCount = 1;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	layoutBinding.pImmutableSamplers = immutableSamplers;
	return layoutBinding;
}
