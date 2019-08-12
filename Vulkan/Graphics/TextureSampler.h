#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class LogicDevice;
		class Image;

		class TextureSampler {
		public:
			TextureSampler(const LogicDevice* logicDevice, const std::string& name, Image* texture, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, 
				float anisotropy);
			~TextureSampler();
			operator VkSampler() {
				return sampler_;
			}
			VkWriteDescriptorSet descriptorWrite(VkDescriptorSet set, uint32_t binding);
			const std::string& getName() const {
				return name_;
			}
			VkDescriptorImageInfo getImageInfo();
		private:
			const LogicDevice* logicDevice_;
			Image* texture_;
			VkSampler sampler_;
			VkDescriptorImageInfo imageInfo_;
			const std::string name_;
		};
	}
}
