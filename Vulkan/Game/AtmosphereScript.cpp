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
#include "SunScript.h"

using namespace QZL;
using namespace QZL::Game;
using namespace QZL::Graphics;

AtmosphereScript::AtmosphereScript(const GameScriptInitialiser& initialiser, SunScript* sun)
	: GameScript(initialiser)
{
	logicDevice_ = initialiser.system->getMasters().graphicsMaster->getLogicDevice();

	params_.betaRay = calculateBetaRayeligh(1.0003, 2.545e25, { 6.5e-7, 5.1e-7, 4.75e-7 });// glm::vec3(6.55e-6f, 1.73e-5f, 2.30e-5f);
	params_.betaMie = 2e-6;
	params_.betaMieExt = params_.betaMie / 0.9;
	params_.planetRadius = 6371e3;
	params_.Hatm = 80000.0;
	params_.mieScaleHeight = 1200.0;
	params_.rayleighScaleHeight = 8000.0;
	params_.betaOzoneExt = glm::vec3(5.09, 7.635, 0.2545);

	shaderParams_.params.betaRay = params_.betaRay;
	shaderParams_.params.betaMie = params_.betaMie;
	shaderParams_.params.planetRadius = params_.planetRadius;
	shaderParams_.params.Hatm = params_.Hatm;
	shaderParams_.params.g = 0.76f;
	shaderParams_.params.sunDirection = sun->getSunDirection();
	shaderParams_.params.sunIntensity = sun->getSunIntensity();

	// Make the material, it only needs the scattering sampler
	auto descriptor = logicDevice_->getPrimaryDescriptor();
	VkDescriptorSetLayoutBinding scatteringBinding = makeLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayout matLayout = descriptor->makeLayout({ scatteringBinding });
	auto matSetIdx = descriptor->createSets({ matLayout });
	auto matSet = descriptor->getSet(matSetIdx);
	material_ = new AtmosphereMaterial("atmosphereMaterial", matSet, matLayout);
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
	SAFE_DELETE(material_);
}

Graphics::ShaderParams* AtmosphereScript::getNewShaderParameters() 
{
	return new Graphics::AtmosphereShaderParams(shaderParams_);
}

void AtmosphereScript::start()
{
	// See http://publications.lib.chalmers.se/records/fulltext/203057/203057.pdf for reference.
	initTextures(logicDevice_, textures_);

	// Create the uniform buffer for required parameters.
	DescriptorBuffer* buffer = DescriptorBuffer::makeBuffer<UniformBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 0, 0,
		sizeof(Assets::AtmosphereParameters), VK_SHADER_STAGE_COMPUTE_BIT, "AtmosParamsBuffer");

	// Setup descriptor set, each shader has identical descriptor even if not used.
	auto descriptor = logicDevice_->getPrimaryDescriptor();

	VkDescriptorSetLayoutBinding transmittance = makeLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // Transmittance write image
	VkDescriptorSetLayoutBinding scattering = makeLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // Scattering write image
	VkDescriptorSetLayoutBinding gatheringSampler = makeLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // Sampler gathering LUT order
	VkDescriptorSetLayoutBinding gatheringSumSampler = makeLayoutBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // Gathering Sum Sampler
	VkDescriptorSetLayoutBinding scatteringSum = makeLayoutBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // Scattering sum image write
	VkDescriptorSetLayoutBinding transmittanceSampler = makeLayoutBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // Transmittance read sampler
	VkDescriptorSetLayoutBinding gathering = makeLayoutBinding(7, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); //  gathering LUT order write image
	VkDescriptorSetLayoutBinding gatheringSum = makeLayoutBinding(8, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // Gathering Sum Sampler
	VkDescriptorSetLayoutBinding scatteringSampler = makeLayoutBinding(9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // Scattering sampler
	VkDescriptorSetLayout layout = descriptor->makeLayout({ buffer->getBinding(), transmittance, scattering, gatheringSampler, gatheringSumSampler, 
		scatteringSum, transmittanceSampler, gathering, gatheringSum, scatteringSampler });
	size_t setIdx = descriptor->createSets({ layout });
	auto set = descriptor->getSet(setIdx);

	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(buffer->descriptorWrite(descriptor->getSet(setIdx)));
	descWrites.push_back(textures_.transmittanceImage->descriptorWrite(descriptor->getSet(setIdx), 1));
	descWrites.push_back(textures_.scatteringImage->descriptorWrite(descriptor->getSet(setIdx), 2));
	descWrites.push_back(textures_.gathering->descriptorWrite(descriptor->getSet(setIdx), 3));
	descWrites.push_back(textures_.gatheringSum->descriptorWrite(descriptor->getSet(setIdx), 4));
	descWrites.push_back(textures_.scatteringSumImage->descriptorWrite(descriptor->getSet(setIdx), 5));
	descWrites.push_back(textures_.transmittance->descriptorWrite(descriptor->getSet(setIdx), 6));
	descWrites.push_back(textures_.gatheringImage->descriptorWrite(descriptor->getSet(setIdx), 7));
	descWrites.push_back(textures_.gatheringSumImage->descriptorWrite(descriptor->getSet(setIdx), 8));
	descWrites.push_back(textures_.scatteringSum->descriptorWrite(descriptor->getSet(setIdx), 9));
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
	//textures_.scatteringSumImage->changeLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	CHECK_VKRESULT(vkEndCommandBuffer(cmdBuffer));

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	CHECK_VKRESULT(vkQueueSubmit(logicDevice_->getQueueHandle(QueueFamilyType::kComputeQueue), 1, &submitInfo, VK_NULL_HANDLE));
	CHECK_VKRESULT(vkQueueWaitIdle(logicDevice_->getQueueHandle(QueueFamilyType::kComputeQueue)));

	SAFE_DELETE(buffer);

	std::vector<VkWriteDescriptorSet> materialWrites = { textures_.scatteringSum->descriptorWrite(material_->getTextureSet(), 0) };
	descriptor->updateDescriptorSets(materialWrites);
}

void AtmosphereScript::initTextures(const LogicDevice* logicDevice, PrecomputedTextures& finalTextures)
{
	finalTextures.transmittanceImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT),
		MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL }, "AtmosTransmittance");
	finalTextures.transmittance = finalTextures.transmittanceImage->createTextureSampler("AtmosTransmittance", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	finalTextures.gatheringImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, GATHERING_TEXTURE_WIDTH, GATHERING_TEXTURE_HEIGHT),
		MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL }, "AtmosGathering");
	finalTextures.gathering = finalTextures.gatheringImage->createTextureSampler("AtmosGathering", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	finalTextures.gatheringSumImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, GATHERING_TEXTURE_WIDTH, GATHERING_TEXTURE_HEIGHT),
		MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL }, "AtmosGatheringSum");
	finalTextures.gatheringSum = finalTextures.gatheringSumImage->createTextureSampler("AtmosGatheringSum", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	finalTextures.scatteringImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_3D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH),
		MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL }, "AtmosScattering");
	finalTextures.scattering = finalTextures.scatteringImage->createTextureSampler("AtmosScattering", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	finalTextures.scatteringSumImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_3D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH),
		MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL }, "AtmosScatteringSum");
	finalTextures.scatteringSum = finalTextures.scatteringSumImage->createTextureSampler("AtmosScatteringSum", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);
}

VkDescriptorSetLayoutBinding AtmosphereScript::makeLayoutBinding(const uint32_t binding, VkDescriptorType type, const VkSampler* immutableSamplers, VkShaderStageFlags stages)
{
	VkDescriptorSetLayoutBinding layoutBinding;
	layoutBinding.binding = binding;
	layoutBinding.descriptorCount = 1;
	layoutBinding.descriptorType = type;
	layoutBinding.stageFlags = stages;
	layoutBinding.pImmutableSamplers = immutableSamplers;
	return layoutBinding;
}

glm::dvec3 AtmosphereScript::calculateBetaRayeligh(double refractiveIndex, double molecularDensity, glm::dvec3 wavelength)
{
	double numerator = (refractiveIndex * refractiveIndex - 1);
	return glm::dvec3((8.0 * std::pow(std::_Pi, 3.0))) * ((numerator * numerator) / (3.0 * molecularDensity * glm::pow(wavelength, glm::dvec3(4.0))));
}
