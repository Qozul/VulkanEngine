#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class LogicDevice;
		class Image2D;

		class TextureSampler {
		public:
			TextureSampler(const LogicDevice* logicDevice, const std::string& name, Image2D* texture, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode,
				float anisotropy, uint32_t binding);
			~TextureSampler();
			operator VkSampler() {
				return sampler_;
			}
			VkWriteDescriptorSet descriptorWrite(VkDescriptorSet set);
			const std::string& getName() const {
				return name_;
			}
			VkDescriptorImageInfo getImageInfo();
		private:
			const LogicDevice* logicDevice_;
			Image2D* texture_;
			VkSampler sampler_;
			VkDescriptorImageInfo imageInfo_;
			uint32_t bindingIdx_;
			const std::string name_;
		};
	}
}
