#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class LogicDevice;
		class Image2D;

		class TextureSampler {
		public:
			TextureSampler(const LogicDevice* logicDevice, Image2D* texture, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode,
				float anisotropy, uint32_t binding);
			~TextureSampler();
			operator VkSampler() {
				return sampler_;
			}
			const VkDescriptorSetLayoutBinding& getBinding();
			VkWriteDescriptorSet descriptorWrite(VkDescriptorSet set);
		private:
			const LogicDevice* logicDevice_;
			Image2D* texture_;
			VkSampler sampler_;
			VkDescriptorSetLayoutBinding binding_;
			VkDescriptorImageInfo imageInfo_;
			uint32_t bindingIdx_;
		};
	}
}
