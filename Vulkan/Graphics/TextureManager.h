// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "VkUtil.h"
#include "Material.h"
#include "Image.h"

namespace QZL {
	namespace Graphics {
		class LogicDevice;
		class TextureSampler;
		class Descriptor;
		class TextureLoader;

		struct SamplerInfo {
			VkFilter magFilter = VK_FILTER_LINEAR;
			VkFilter minFilter = VK_FILTER_LINEAR;
			VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			float anisotropy = 8;
			VkShaderStageFlags stages = VK_SHADER_STAGE_FRAGMENT_BIT;
			SamplerInfo() { }
			SamplerInfo(VkShaderStageFlags stages) : stages(stages) { }
			SamplerInfo(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, float anisotropy, VkShaderStageFlags stages)
				: magFilter(magFilter), minFilter(minFilter), addressMode(addressMode), anisotropy(anisotropy), stages(stages) { }
		};

		class TextureManager {
		public:
			TextureManager(const LogicDevice* logicDevice, Descriptor* descriptor, uint32_t maxTextures, bool descriptorIndexing = false);
			~TextureManager();
			
			// Returns the index of the texture sampler in the texture aray descriptor
			uint32_t requestTexture(const std::string& name, SamplerInfo samplerInfo = {});

			// Returns a texture sampler and passes ownership of the sampler to the caller, which is expected to destroy the resource prior to this class
			// destructor being called.
			TextureSampler* requestTextureSeparate(const std::string& name, VkFilter magFilter = VK_FILTER_LINEAR, VkFilter minFilter = VK_FILTER_LINEAR,
				VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, float anisotropy = 8, VkShaderStageFlags stages = VK_SHADER_STAGE_FRAGMENT_BIT);

			uint32_t allocateTexture(const std::string& name, Image*& imgPtr, VkImageCreateInfo createInfo,
				MemoryAllocationPattern allocationPattern, ImageParameters parameters, SamplerInfo samplerInfo = {});
			uint32_t allocateTexture(const std::string& name, Image* img, SamplerInfo samplerInfo = {});

			TextureSampler* getSampler(std::string name) {
				return textureSamplersDI_[name].first;
			}
			
			Material* requestMaterial(const MaterialType type, const std::string name);

			VkDescriptorSetLayoutBinding getSetlayoutBinding() {
				return setLayoutBinding_;
			}

			void setDescriptorSetIdx(uint32_t idx) {
				descriptorSetIdx_ = idx;
			}

			const bool descriptorIndexingEnabled() {
				return descriptorIndexingActive_;
			}

			std::vector<uint32_t> materialData_;
		private:
			VkWriteDescriptorSet makeDescriptorWrite(VkDescriptorImageInfo imageInfo, uint32_t idx, uint32_t count = 1); 
			Image* allocateImage(std::string name, VkImageCreateInfo createInfo, MemoryAllocationPattern allocationPattern, ImageParameters parameters);

			const bool descriptorIndexingActive_;
			const uint32_t maxTextures_;
			const LogicDevice* logicDevice_;
			TextureLoader* textureLoader_;
			Descriptor* descriptor_;
			uint32_t descriptorSetIdx_;
			VkDescriptorSetLayoutBinding setLayoutBinding_;
			std::unordered_map<std::string, Image*> textures_;
			std::unordered_map<std::string, TextureSampler*> texturesSamplers_;
			std::unordered_map<std::string, std::pair<TextureSampler*, uint32_t>> textureSamplersDI_;
			std::unordered_map<std::string, Material*> materials_;

			std::queue<uint32_t> freeDescriptors_;
		};
	}
}
