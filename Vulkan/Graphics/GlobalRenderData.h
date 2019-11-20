// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "VkUtil.h"
#include "StorageBuffer.h"
#include "Descriptor.h"

namespace QZL {
	namespace Graphics {
		class TextureSampler;
		static const size_t kMaxLights = 1;
		struct LightingData {
			glm::vec4 cameraPosition;
			glm::vec4 ambientColour;
			std::array<glm::vec4, kMaxLights> lightPositions;
		};
		struct Light {
			glm::vec3 position;
			float radius;
			glm::vec3 colour;
			float attenuationFactor;
			glm::vec4 direction;
		};
		enum class GlobalRenderDataBindings : uint32_t {
			kLighting = 0,
			kEnvironmentMap = 1,
			kTextureArray = 2
		};
		class LogicDevice;
		class GlobalRenderData {
			friend class SwapChain;
		public:
			VkDescriptorSet getSet() const {
				return set_;
			}
			VkDescriptorSetLayout getLayout() const {
				return layout_;
			}
			void updateData(uint32_t idx, LightingData& data);
		private:
			GlobalRenderData(LogicDevice* logicDevice, TextureManager* textureManager, VkDescriptorSetLayoutBinding descriptorIndexBinding);
			~GlobalRenderData();
			void createDescriptorSet(LogicDevice* logicDevice, std::vector<VkDescriptorBindingFlagsEXT> bindingFlags, VkDescriptorSetLayoutBinding* descriptorIndexBinding = nullptr);

			VkDescriptorSet set_;
			VkDescriptorSetLayout layout_;
			DescriptorBuffer* lightingUbo_;
			TextureSampler* environmentTexture_;
		};
	}
}
