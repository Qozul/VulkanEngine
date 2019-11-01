// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "VkUtil.h"
#include "StorageBuffer.h"
#include "Descriptor.h"

namespace QZL {
	namespace Graphics {
		static const size_t kMaxLights = 1;
		struct LightingData {
			glm::vec4 cameraPosition;
			glm::vec4 ambientColour;
			std::array<glm::vec4, kMaxLights> lightPositions;
		};
		enum class GlobalRenderDataBindings : uint32_t {
			LIGHTING = 0,
			TEXTURE_ARRAY_BINDING = 1
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
			void updateData(uint32_t idx, LightingData& data) {
				LightingData* lightingPtr = static_cast<LightingData*>(lightingUbo_->bindRange());
				*lightingPtr = data;				
				lightingUbo_->unbindRange();
			}
		private:
			GlobalRenderData(LogicDevice* logicDevice) {
				VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags = {};
				setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
				setLayoutBindingFlags.bindingCount = 2;
				std::vector<VkDescriptorBindingFlagsEXT> descriptorBindingFlags = {
					0,
					0
				};

				lightingUbo_ = DescriptorBuffer::makeBuffer<UniformBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, (uint32_t)GlobalRenderDataBindings::LIGHTING, 0,
					sizeof(LightingData), VK_SHADER_STAGE_ALL);
				
				auto descriptor = logicDevice->getPrimaryDescriptor();
				layout_ = descriptor->makeLayout({ lightingUbo_->getBinding() });
				auto idx = descriptor->createSets({ layout_ });
				set_ = descriptor->getSet(idx);
				descriptor->updateDescriptorSets({ lightingUbo_->descriptorWrite(set_) });
			}

			GlobalRenderData(LogicDevice* logicDevice, VkDescriptorSetLayoutBinding descriptorIndexBinding) {
				VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags = {};
				setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
				setLayoutBindingFlags.bindingCount = 2;
				std::vector<VkDescriptorBindingFlagsEXT> descriptorBindingFlags = {
					0,
					VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
				};
				setLayoutBindingFlags.pBindingFlags = descriptorBindingFlags.data();

				lightingUbo_ = DescriptorBuffer::makeBuffer<UniformBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, (uint32_t)GlobalRenderDataBindings::LIGHTING, 0,
					sizeof(LightingData), VK_SHADER_STAGE_ALL);

				auto descriptor = logicDevice->getPrimaryDescriptor();
				layout_ = descriptor->makeLayout({ lightingUbo_->getBinding(), descriptorIndexBinding }, &setLayoutBindingFlags);
				auto idx = descriptor->createSets({ layout_ });
				set_ = descriptor->getSet(idx);
				descriptor->updateDescriptorSets({ lightingUbo_->descriptorWrite(set_) });
			}
			~GlobalRenderData() {
				SAFE_DELETE(lightingUbo_);
			}


			VkDescriptorSet set_;
			VkDescriptorSetLayout layout_;
			DescriptorBuffer* lightingUbo_;
		};
	}
}
