#pragma once
#include "VkUtil.h"
#include "Material.h"

namespace QZL {
	namespace Graphics {
		class Image;
		class LogicDevice;
		class TextureSampler;
		class Descriptor;
		class TextureLoader;

		class TextureManager {
		public:
			TextureManager(const LogicDevice* logicDevice, Descriptor* descriptor, uint32_t maxTextures, bool descriptorIndexing = false);
			~TextureManager();
			
			// Returns the index of the texture sampler in the texture aray descriptor
			uint32_t requestTexture(const std::string& name, VkFilter magFilter = VK_FILTER_LINEAR, VkFilter minFilter = VK_FILTER_LINEAR,
				VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, float anisotropy = 8);

			// Returns a texture sampler and passes ownership of the sampler to the caller, which is expected to destroy the resource prior to this class
			// destructor being called.
			TextureSampler* requestTextureSeparate(const std::string& name, VkFilter magFilter = VK_FILTER_LINEAR, VkFilter minFilter = VK_FILTER_LINEAR,
				VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, float anisotropy = 8);
			
			template<typename Subclass>
			Material* requestMaterial(const std::string name);

			VkDescriptorSetLayoutBinding getSetlayoutBinding() {
				return setLayoutBinding_;
			}

			void setDescriptorSetIdx(uint32_t idx) {
				descriptorSetIdx_ = idx;
			}

		private:
			VkWriteDescriptorSet makeDescriptorWrite(VkDescriptorImageInfo imageInfo, uint32_t idx, uint32_t count = 1);

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

		template<typename Subclass>
		inline Material* TextureManager::requestMaterial(const std::string name)
		{
			static_assert(std::is_base_of<Material, Subclass>::value, "Only a subclass of material is permitted.");
			if (materials_.count(name) != 0) {
				return materials_[name];
			}
			else {
				Material* material = new Subclass(name);
				material->load(this, descriptor_);
				materials_[name] = material;
				return material;
			}
		}
	}
}
