#include "Atmosphere.h"
#include "../Graphics/LogicDevice.h"
#include "../Graphics/TextureManager.h"
#include "../Graphics/TextureSampler.h"
#include "../Graphics/ComputePipeline.h"
#include "../Graphics/Shader.h"
#include "../Graphics/Image.h"
#include "../Graphics/DeviceMemory.h"
#include "../Graphics/StorageBuffer.h"
#include "../Graphics/Descriptor.h"

using namespace QZL;
using namespace Assets;
using namespace Graphics;

/*
TODO:
	- Check over textures to ensure they are correct.
	- Check data alignment of PrecomputationData.
	- Update all shaders to have correct bindings for different textures.
	- Write command buffer record and execute.
	- Write simple Quad entity (Static) to draw textures on to for testing.
	- Debug hell.
	- 100% confirm all resources neatly cleaned up appropriately.
*/

struct PrecomputationData {
	Atmosphere::AtmosphereParameters atmosphere;
	glm::mat3 luminance_from_radiance;
	int scattering_order;
};

Atmosphere::~Atmosphere()
{
	SAFE_DELETE(textures_.irradiance);
	SAFE_DELETE(textures_.irradianceImage);
	SAFE_DELETE(textures_.scattering);
	SAFE_DELETE(textures_.scatteringImage);
	SAFE_DELETE(textures_.transmittance);
	SAFE_DELETE(textures_.transmittanceImage);
}

void Atmosphere::precalculateTextures(LogicDevice* logicDevice, Descriptor* descriptor)
{
	// See https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/model.cc.html
	// for reference.
	// Create the textures. Need a total of 8 textures, with 3 permanent for rendering and 5 temporary.
	TempPrecomputationTextures tempTextures;
	initTextures(logicDevice, tempTextures, textures_);

	// Setup descriptor set, each shader has identical descriptor even if not used.
	StorageBuffer buffer = StorageBuffer(logicDevice, MemoryAllocationPattern::kDynamicResource, 1, 0,
		sizeof(PrecomputationData), VK_SHADER_STAGE_COMPUTE_BIT, true);

	VkDescriptorSetLayoutBinding transmittance = makeLayoutBinding(0);
	VkDescriptorSetLayoutBinding irradiance = makeLayoutBinding(2);
	VkDescriptorSetLayoutBinding scattering = makeLayoutBinding(3);
	VkDescriptorSetLayoutBinding directIrradiance = makeLayoutBinding(4);
	VkDescriptorSetLayoutBinding deltaRayleighScattering = makeLayoutBinding(5);
	VkDescriptorSetLayoutBinding deltaMieScattering = makeLayoutBinding(6);
	VkDescriptorSetLayoutBinding deltaScatteringDensity = makeLayoutBinding(7);
	VkDescriptorSetLayoutBinding deltaMultipleScatteringDensity = makeLayoutBinding(8);
	VkDescriptorSetLayout layout = descriptor->makeLayout({ buffer.getBinding(), transmittance, irradiance, scattering, directIrradiance,
		deltaRayleighScattering, deltaMieScattering, deltaScatteringDensity, deltaMultipleScatteringDensity });
	size_t setIdx = descriptor->createSets({ layout });

	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(textures_.transmittance->descriptorWrite(descriptor->getSet(setIdx), 0));
	descWrites.push_back(buffer.descriptorWrite(descriptor->getSet(setIdx)));
	descWrites.push_back(textures_.irradiance->descriptorWrite(descriptor->getSet(setIdx), 2));
	descWrites.push_back(textures_.scattering->descriptorWrite(descriptor->getSet(setIdx), 3));
	descWrites.push_back(tempTextures.directIrradianceTexture->descriptorWrite(descriptor->getSet(setIdx), 4));
	descWrites.push_back(tempTextures.deltaRayleighScatteringTexture->descriptorWrite(descriptor->getSet(setIdx), 5));
	descWrites.push_back(tempTextures.deltaMieScatteringTexture->descriptorWrite(descriptor->getSet(setIdx), 6));
	descWrites.push_back(tempTextures.deltaScatteringDensityTexture->descriptorWrite(descriptor->getSet(setIdx), 7));
	descWrites.push_back(tempTextures.deltaMultipleScatteringTexture->descriptorWrite(descriptor->getSet(setIdx), 8));
	descriptor->updateDescriptorSets(descWrites);

	// Create the compute pipelines.
	ComputePipeline transmittancePipeline = ComputePipeline(logicDevice, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereTransmittance");
	ComputePipeline directIrradiancePipeline = ComputePipeline(logicDevice, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereDirectIrradiance");
	ComputePipeline singleScatteringPipeline = ComputePipeline(logicDevice, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereSingleScattering");
	ComputePipeline scatteringDensityPipeline = ComputePipeline(logicDevice, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereScatteringDensity");
	ComputePipeline indirectIrradiancePipeline = ComputePipeline(logicDevice, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereIndirectirradiance");
	ComputePipeline multipleScatteringPipeline = ComputePipeline(logicDevice, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereMultipleScattering");

	// TODO Record command buffer and execute on compute queue.

	// 1. Compute Transmittance
	// 2. Compute Direct Irradiance
	// 3. Compute Single Scattering
	// 4. Compute Scatter Density
	// 5. Compute Indirect Irradiance
	// 6. Compute Multiple Scattering

	// Transfer image memory from writeable to read only optimal, it will never need to be written again.
	textures_.irradianceImage->changeLayout({ VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	textures_.transmittanceImage->changeLayout({ VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	textures_.scatteringImage->changeLayout({ VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

	// Clean up resources if needed, otherwise scope cleans up. Only persisting resources are those in the PrecomputedTextures member.
	SAFE_DELETE(tempTextures.deltaMieScatteringTexture);
	SAFE_DELETE(tempTextures.deltaMieScatteringImage);
	SAFE_DELETE(tempTextures.deltaMultipleScatteringTexture);
	SAFE_DELETE(tempTextures.deltaMultipleScatteringImage);
	SAFE_DELETE(tempTextures.deltaRayleighScatteringTexture);
	SAFE_DELETE(tempTextures.deltaRayleighScatteringImage);
	SAFE_DELETE(tempTextures.deltaScatteringDensityTexture);
	SAFE_DELETE(tempTextures.deltaScatteringDensityImage);
	SAFE_DELETE(tempTextures.directIrradianceTexture);
	SAFE_DELETE(tempTextures.directIrradianceImage);
}

void Atmosphere::initTextures(LogicDevice* logicDevice, TempPrecomputationTextures& tempTextures, PrecomputedTextures& finalTextures)
{
	tempTextures.directIrradianceImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT), MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_2D });
	tempTextures.directIrradianceTexture = tempTextures.directIrradianceImage->createTextureSampler("AtmosDirectIrradiance", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	tempTextures.deltaMultipleScatteringImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_3D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH), MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_3D });
	tempTextures.deltaMultipleScatteringTexture = tempTextures.deltaMultipleScatteringImage->createTextureSampler("AtmosDeltaMultipleScattering", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	tempTextures.deltaRayleighScatteringImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_3D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH), MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_3D });
	tempTextures.deltaRayleighScatteringTexture = tempTextures.deltaRayleighScatteringImage->createTextureSampler("AtmosRayleighScattering", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	tempTextures.deltaScatteringDensityImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_3D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH), MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_3D });
	tempTextures.deltaScatteringDensityTexture = tempTextures.deltaScatteringDensityImage->createTextureSampler("AtmosScatteringDensity", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	tempTextures.deltaMieScatteringImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_3D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH), MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_3D });
	tempTextures.deltaMieScatteringTexture = tempTextures.deltaMieScatteringImage->createTextureSampler("AtmosMieScattering", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	finalTextures.transmittanceImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_WIDTH), MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_2D });
	finalTextures.transmittance = finalTextures.transmittanceImage->createTextureSampler("AtmosTransmittance", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	finalTextures.irradianceImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT), MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_2D });
	finalTextures.irradiance = finalTextures.irradianceImage->createTextureSampler("AtmosIrradiance", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	finalTextures.scatteringImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_3D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH), MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_3D });
	finalTextures.scattering = finalTextures.scatteringImage->createTextureSampler("AtmosScattering", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);
}

VkDescriptorSetLayoutBinding Atmosphere::makeLayoutBinding(const uint32_t binding, const VkSampler* immutableSamplers)
{
	VkDescriptorSetLayoutBinding layoutBinding;
	layoutBinding.binding = binding;
	layoutBinding.descriptorCount = 1;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	layoutBinding.pImmutableSamplers = immutableSamplers;
	return layoutBinding;
}
