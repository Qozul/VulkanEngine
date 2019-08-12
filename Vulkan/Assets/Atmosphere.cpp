#include "Atmosphere.h"
#include "../Graphics/LogicDevice.h"
#include "../Graphics/PhysicalDevice.h"
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
	- Check over textures to ensure they are correct. X
	- Check data alignment of PrecomputationData. X
	- Update all shaders to have correct bindings for different textures. X
	- Write buffer data. X
	- Setup compute queue and get command buffer. X
	- Write command buffer record and execute.  X
	- Calculate init things still left (blend, num_scattering orders, lumaninance_to_radiance). X 
	- Scattering order as push constant X
	- Need access to VK_DESCRIPTOR_TYPE_STORAGE_IMAGE descriptors X
	- Write simple Quad entity (Static) to draw textures on to for testing.
	- Debug hell.
	- 100% confirm all resources neatly cleaned up appropriately.
*/

struct PrecomputationData {
	Atmosphere::AtmosphereParameters atmosphere;
	glm::mat3 luminance_from_radiance;
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

void Atmosphere::precalculateTextures(const LogicDevice* logicDevice)
{
	// See https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/model.cc.html for reference.
	// Create the textures. Need a total of 8 textures, with 3 permanent for rendering and 5 temporary.
	TempPrecomputationTextures tempTextures;
	initTextures(logicDevice, tempTextures, textures_);

	// Create the uniform buffer for required parameters.
	StorageBuffer buffer = StorageBuffer(logicDevice, MemoryAllocationPattern::kDynamicResource, 1, 0,
		sizeof(PrecomputationData), VK_SHADER_STAGE_COMPUTE_BIT, true);

	// Setup descriptor set, each shader has identical descriptor even if not used.
	auto descriptor = logicDevice->getPrimaryDescriptor();

	VkDescriptorSetLayoutBinding transmittance = makeLayoutBinding(0);
	VkDescriptorSetLayoutBinding irradiance = makeLayoutBinding(2);
	VkDescriptorSetLayoutBinding scattering = makeLayoutBinding(3);
	VkDescriptorSetLayoutBinding deltaIrradiance = makeLayoutBinding(4);
	VkDescriptorSetLayoutBinding deltaRayleighScattering = makeLayoutBinding(5);
	VkDescriptorSetLayoutBinding deltaMieScattering = makeLayoutBinding(6);
	VkDescriptorSetLayoutBinding deltaScatteringDensity = makeLayoutBinding(7);
	VkDescriptorSetLayoutBinding deltaMultipleScatteringDensity = makeLayoutBinding(8);
	VkDescriptorSetLayout layout = descriptor->makeLayout({ buffer.getBinding(), transmittance, irradiance, scattering, deltaIrradiance,
		deltaRayleighScattering, deltaMieScattering, deltaScatteringDensity, deltaMultipleScatteringDensity });
	size_t setIdx = descriptor->createSets({ layout });
	auto set = descriptor->getSet(setIdx);

	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(textures_.transmittance->descriptorWrite(descriptor->getSet(setIdx), 0));
	descWrites.push_back(buffer.descriptorWrite(descriptor->getSet(setIdx)));
	descWrites.push_back(textures_.irradiance->descriptorWrite(descriptor->getSet(setIdx), 2));
	descWrites.push_back(textures_.scattering->descriptorWrite(descriptor->getSet(setIdx), 3));
	descWrites.push_back(tempTextures.deltaIrradianceTexture->descriptorWrite(descriptor->getSet(setIdx), 4));
	descWrites.push_back(tempTextures.deltaRayleighScatteringTexture->descriptorWrite(descriptor->getSet(setIdx), 5));
	descWrites.push_back(tempTextures.deltaMieScatteringTexture->descriptorWrite(descriptor->getSet(setIdx), 6));
	descWrites.push_back(tempTextures.deltaScatteringDensityTexture->descriptorWrite(descriptor->getSet(setIdx), 7));
	descWrites.push_back(tempTextures.deltaMultipleScatteringTexture->descriptorWrite(descriptor->getSet(setIdx), 8));
	descriptor->updateDescriptorSets(descWrites);

	// Write the buffer data.
	PrecomputationData* precompData = static_cast<PrecomputationData*>(buffer.bindRange());
	precompData->atmosphere = parameters_;
	precompData->luminance_from_radiance = glm::mat3();
	buffer.unbindRange();

	// Create the compute pipelines.
	ComputePipeline transmittancePipeline = ComputePipeline(logicDevice, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereTransmittance");
	ComputePipeline directIrradiancePipeline = ComputePipeline(logicDevice, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereDirectIrradiance");
	ComputePipeline singleScatteringPipeline = ComputePipeline(logicDevice, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereSingleScattering");
	ComputePipeline scatteringDensityPipeline = ComputePipeline(logicDevice, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereScatteringDensity");
	ComputePipeline indirectIrradiancePipeline = ComputePipeline(logicDevice, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereIndirectirradiance");
	ComputePipeline multipleScatteringPipeline = ComputePipeline(logicDevice, ComputePipeline::makeLayoutInfo(1, &layout, {}), "AtmosphereMultipleScattering");

	VkMemoryBarrier pushConstantBarrier = {};
	pushConstantBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	pushConstantBarrier.pNext = NULL;

	// TODO Record command buffer and execute on compute queue.
	VkCommandBuffer cmdBuffer = logicDevice->getComputeCommandBuffer();

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	CHECK_VKRESULT(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, transmittancePipeline.getLayout(), 0, 2, &set, 0, nullptr);

	// 1. Compute Transmittance
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, transmittancePipeline.getPipeline());
	vkCmdDispatch(cmdBuffer, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT, 1);

	// 2. Compute Direct Irradiance
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, directIrradiancePipeline.getPipeline());
	vkCmdDispatch(cmdBuffer, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, 1);

	// 3. Compute Single Scattering
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, singleScatteringPipeline.getPipeline());
	vkCmdDispatch(cmdBuffer, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH);

	for (unsigned int scatteringOrder = 2; scatteringOrder <= 4; ++scatteringOrder) {
		// 4. Compute Scatter Density
		vkCmdPushConstants(cmdBuffer, scatteringDensityPipeline.getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(scatteringOrder), &scatteringOrder);
		vkCmdPipelineBarrier(cmdBuffer, VK_SHADER_STAGE_COMPUTE_BIT, VK_SHADER_STAGE_COMPUTE_BIT, VK_DEPENDENCY_BY_REGION_BIT, 1, &pushConstantBarrier, 0, nullptr, 0, nullptr);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, scatteringDensityPipeline.getPipeline());
		vkCmdDispatch(cmdBuffer, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH);

		// 5. Compute Indirect Irradiance
		vkCmdPushConstants(cmdBuffer, indirectIrradiancePipeline.getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(scatteringOrder), &scatteringOrder);
		vkCmdPipelineBarrier(cmdBuffer, VK_SHADER_STAGE_COMPUTE_BIT, VK_SHADER_STAGE_COMPUTE_BIT, VK_DEPENDENCY_BY_REGION_BIT, 1, &pushConstantBarrier, 0, nullptr, 0, nullptr);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, indirectIrradiancePipeline.getPipeline());
		vkCmdDispatch(cmdBuffer, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH);

		// 6. Compute Multiple Scattering
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, multipleScatteringPipeline.getPipeline());
		vkCmdDispatch(cmdBuffer, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH);

	}
	CHECK_VKRESULT(vkEndCommandBuffer(cmdBuffer));

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	vkQueueSubmit(logicDevice->getQueueHandle(QueueFamilyType::kComputeQueue), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(logicDevice->getQueueHandle(QueueFamilyType::kComputeQueue));

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
	SAFE_DELETE(tempTextures.deltaIrradianceTexture);
	SAFE_DELETE(tempTextures.deltaIrradianceImage);
}

void Atmosphere::initTextures(const LogicDevice* logicDevice, TempPrecomputationTextures& tempTextures, PrecomputedTextures& finalTextures)
{
	tempTextures.deltaIrradianceImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT), MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_2D });
	tempTextures.deltaIrradianceTexture = tempTextures.deltaIrradianceImage->createTextureSampler("AtmosDeltaIrradiance", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

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
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_WIDTH), MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_2D });
	finalTextures.transmittance = finalTextures.transmittanceImage->createTextureSampler("AtmosTransmittance", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	finalTextures.irradianceImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT), MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_2D });
	finalTextures.irradiance = finalTextures.irradianceImage->createTextureSampler("AtmosIrradiance", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	finalTextures.scatteringImage = new Image(logicDevice, Image::makeCreateInfo(VK_IMAGE_TYPE_3D, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH), MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_3D });
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

void Atmosphere::convertSpectrumToLinearSrgb(const std::vector<double>& wavelengths, const std::vector<double>& spectrum, double& r, double& g, double& b)
{
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;
	const int dlambda = 1;
	for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda) {
		double value = interpolate(wavelengths, spectrum, lambda);
		x += cieColorMatchingFunctionTableValue(lambda, 1) * value;
		y += cieColorMatchingFunctionTableValue(lambda, 2) * value;
		z += cieColorMatchingFunctionTableValue(lambda, 3) * value;
	}
	r = MAX_LUMINOUS_EFFICACY *
		(XYZ_TO_SRGB[0] * x + XYZ_TO_SRGB[1] * y + XYZ_TO_SRGB[2] * z) * dlambda;
	g = MAX_LUMINOUS_EFFICACY *
		(XYZ_TO_SRGB[3] * x + XYZ_TO_SRGB[4] * y + XYZ_TO_SRGB[5] * z) * dlambda;
	b = MAX_LUMINOUS_EFFICACY *
		(XYZ_TO_SRGB[6] * x + XYZ_TO_SRGB[7] * y + XYZ_TO_SRGB[8] * z) * dlambda;
}

double Atmosphere::cieColorMatchingFunctionTableValue(double wavelength, int column)
{
	if (wavelength <= kLambdaMin || wavelength >= kLambdaMax) {
		return 0.0;
	}
	double u = (wavelength - kLambdaMin) / 5.0;
	int row = static_cast<int>(std::floor(u));
	assert(row >= 0 && row + 1 < 95);
	assert(CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row] <= wavelength &&
		CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1)] >= wavelength);
	u -= row;
	return CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row + column] * (1.0 - u) +
		CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1) + column] * u;
}

double Atmosphere::interpolate(const std::vector<double> & wavelengths, const std::vector<double> & wavelength_function, double wavelength)
{
	assert(wavelength_function.size() == wavelengths.size());
	if (wavelength < wavelengths[0]) {
		return wavelength_function[0];
	}
	for (unsigned int i = 0; i < wavelengths.size() - 1; ++i) {
		if (wavelength < wavelengths[i + 1]) {
			double u =
				(wavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
			return
				wavelength_function[i] * (1.0 - u) + wavelength_function[i + 1] * u;
		}
	}
	return wavelength_function[wavelength_function.size() - 1];
}

void Atmosphere::computeSpectralRadianceToLuminanceFactors(const std::vector<double>& wavelengths, const std::vector<double>& solar_irradiance, 
	double lambda_power, double& k_r, double& k_g, double& k_b)
{
	k_r = 0.0;
	k_g = 0.0;
	k_b = 0.0;
	double solar_r = interpolate(wavelengths, solar_irradiance, kLambdaR);
	double solar_g = interpolate(wavelengths, solar_irradiance, kLambdaG);
	double solar_b = interpolate(wavelengths, solar_irradiance, kLambdaB);
	int dlambda = 1;
	for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda) {
		double x_bar = cieColorMatchingFunctionTableValue(lambda, 1);
		double y_bar = cieColorMatchingFunctionTableValue(lambda, 2);
		double z_bar = cieColorMatchingFunctionTableValue(lambda, 3);
		const double* xyz2srgb = XYZ_TO_SRGB;
		double r_bar =
			xyz2srgb[0] * x_bar + xyz2srgb[1] * y_bar + xyz2srgb[2] * z_bar;
		double g_bar =
			xyz2srgb[3] * x_bar + xyz2srgb[4] * y_bar + xyz2srgb[5] * z_bar;
		double b_bar =
			xyz2srgb[6] * x_bar + xyz2srgb[7] * y_bar + xyz2srgb[8] * z_bar;
		double irradiance = interpolate(wavelengths, solar_irradiance, lambda);
		k_r += r_bar * irradiance / solar_r *
			pow(lambda / kLambdaR, lambda_power);
		k_g += g_bar * irradiance / solar_g *
			pow(lambda / kLambdaG, lambda_power);
		k_b += b_bar * irradiance / solar_b *
			pow(lambda / kLambdaB, lambda_power);
	}
	k_r *= MAX_LUMINOUS_EFFICACY * dlambda;
	k_g *= MAX_LUMINOUS_EFFICACY * dlambda;
	k_b *= MAX_LUMINOUS_EFFICACY * dlambda;
}
