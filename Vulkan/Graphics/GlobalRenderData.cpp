// Author: Ralph Ridley
// Date: 04/11/19
#include "GlobalRenderData.h"
#include "TextureManager.h"
#include "TextureSampler.h"

using namespace QZL;
using namespace QZL::Graphics;

void GlobalRenderData::updateData(uint32_t idx, Light& data)
{
	Light* lightingPtr = static_cast<Light*>(lightingUbo_->bindRange());
	lightingPtr[idx] = data;
	lightingUbo_->unbindRange();
}

void GlobalRenderData::updateCameraData(LogicalCamera& mainCamera, float screenX, float screenY)
{
	CameraInfo* camInfo = (CameraInfo*)cameraInfoUbo_->bindRange();
	camInfo->projMatrix = mainCamera.projectionMatrix;
	camInfo->inverseViewProj = glm::inverse(mainCamera.viewProjection);
	camInfo->nearPlaneZ = 0.1;
	camInfo->farPlaneZ = 2500.0;
	camInfo->position = mainCamera.position;
	camInfo->viewMatrix = mainCamera.viewMatrix;
	camInfo->screenX = screenX;
	camInfo->screenY = screenY;
	cameraInfoUbo_->unbindRange();
}

void sampleKernelGeneration(glm::vec4* data) {
	std::uniform_real_distribution<float> rand(0.0f, 1.0f);
	std::default_random_engine rng;
	std::array<glm::vec4, SSAO_KERNEL_SIZE> generatedData;
	for (size_t i = 0; i < SSAO_KERNEL_SIZE; ++i) {
		glm::vec3 sample(rand(rng) * 2.0f - 1.0f, rand(rng) * 2.0f - 1.0f, rand(rng));
		sample = glm::normalize(sample);
		sample *= rand(rng);
		float scale = (float)i / SSAO_KERNEL_SIZE;
		scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
		sample *= scale;
		generatedData[i] = glm::vec4(sample, 1.0f);
	}
	memcpy(data, generatedData.data(), sizeof(glm::vec4) * SSAO_KERNEL_SIZE);
}

void GlobalRenderData::updatePostData(float screenX, float screenY)
{
	PostProcessInfo* postInfo = (PostProcessInfo*)postProcessInfoUbo_->bindRange();
	postInfo->ssaoBias = 0.015f;
	postInfo->ssaoKernelSize = SSAO_KERNEL_SIZE;
	postInfo->ssaoNoiseScale = glm::vec2(screenX / 4.0f, screenY / 4.0f);
	postInfo->ssaoRadius = 0.5f;
	sampleKernelGeneration(postInfo->ssaoSamples);
	postProcessInfoUbo_->unbindRange();
}

GlobalRenderData::GlobalRenderData(LogicDevice* logicDevice, TextureManager* textureManager, VkDescriptorSetLayoutBinding descriptorIndexBinding)
{
	environmentTexture_ = textureManager->requestTextureSeparate({ 
		"Environments/rightImage", "Environments/leftImage", "Environments/upImage", 
		"Environments/downImage", "Environments/frontImage", "Environments/backImage"
	});
	createDescriptorSet(logicDevice, { 0, 0, 0, 0, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT }, &descriptorIndexBinding);
}

GlobalRenderData::~GlobalRenderData()
{
	SAFE_DELETE(environmentTexture_);
	SAFE_DELETE(lightingUbo_);
	SAFE_DELETE(cameraInfoUbo_);
	SAFE_DELETE(postProcessInfoUbo_);
}

void GlobalRenderData::createDescriptorSet(LogicDevice* logicDevice, std::vector<VkDescriptorBindingFlagsEXT> bindingFlags, VkDescriptorSetLayoutBinding* descriptorIndexBinding)
{
	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags = {};
	setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	setLayoutBindingFlags.bindingCount = static_cast<uint32_t>(bindingFlags.size());
	setLayoutBindingFlags.pBindingFlags = bindingFlags.data();

	lightingUbo_ = DescriptorBuffer::makeBuffer<UniformBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 0, 0,
		sizeof(Light) * MAX_LIGHTS, VK_SHADER_STAGE_ALL_GRAPHICS, "GlobalLightingBuffer");

	VkDescriptorSetLayoutBinding environmentBinding = TextureSampler::makeBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT);

	cameraInfoUbo_ = DescriptorBuffer::makeBuffer<UniformBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 2, 0,
		sizeof(CameraInfo), VK_SHADER_STAGE_ALL_GRAPHICS, "GlobalCameraInfoBuffer");

	postProcessInfoUbo_ = DescriptorBuffer::makeBuffer<UniformBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 3, 0,
		sizeof(PostProcessInfo), VK_SHADER_STAGE_FRAGMENT_BIT, "GlobalPostProcessBuffer");

	auto descriptor = logicDevice->getPrimaryDescriptor();
	layout_ = descriptor->makeLayout({ lightingUbo_->getBinding(), cameraInfoUbo_->getBinding(), 
		postProcessInfoUbo_->getBinding(), environmentBinding , *descriptorIndexBinding }, &setLayoutBindingFlags);
	auto idx = descriptor->createSets({ layout_ });
	set_ = descriptor->getSet(idx);
	descriptor->updateDescriptorSets({ lightingUbo_->descriptorWrite(set_), environmentTexture_->descriptorWrite(set_, 1), 
		cameraInfoUbo_->descriptorWrite(set_), postProcessInfoUbo_->descriptorWrite(set_) });
}
