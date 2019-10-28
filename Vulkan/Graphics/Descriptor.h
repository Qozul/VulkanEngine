#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class LogicDevice;

		class Descriptor {
		public:
			Descriptor(const LogicDevice* logicDevice, const uint32_t maxSets, std::vector<std::pair<VkDescriptorType, uint32_t>> types);
			~Descriptor();
			// Return the index of the first set created with the first layout in layouts, with sets allocated for each layout given.
			// Increment the index returned appropriately to match a set.
			size_t createSets(const std::vector<VkDescriptorSetLayout>& layouts);
			const VkDescriptorSet getSet(size_t idx);

			VkDescriptorSetLayout makeLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings, const void* pNext = nullptr);
			void updateDescriptorSets(const std::vector<VkWriteDescriptorSet>& descriptorWrites);
		private:
			VkDescriptorPool pool_;
			std::vector<VkDescriptorSet> sets_;
			std::vector<VkDescriptorSetLayout> layouts_;
			const LogicDevice* logicDevice_;
		};
	}
}
