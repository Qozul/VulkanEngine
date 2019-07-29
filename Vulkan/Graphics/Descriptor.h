#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class LogicDevice;

		enum class ReservedGraphicsBindings : uint32_t {
			PER_ENTITY_DATA = 0,
			LIGHTING = 1,
			TEXTURE_0 = 2,
			TEXTURE_1 = 3,
			TEXTURE_2 = 4,
			TEXTURE_3 = 5,
			TEXTURE_4 = 6
		};

		class Descriptor {
		public:
			Descriptor(const LogicDevice* logicDevice, const uint32_t maxSets, std::vector<std::pair<VkDescriptorType, uint32_t>> types);
			~Descriptor();
			// Return the index of the first set created with the first layout in layouts, with sets allocated for each layout given.
			// Increment the index returned appropriately to match a set.
			size_t createSets(const std::vector<VkDescriptorSetLayout>& layouts);
			const VkDescriptorSet getSet(size_t idx);

			VkDescriptorSetLayout makeLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
			void updateDescriptorSets(const std::vector<VkWriteDescriptorSet>& descriptorWrites);
		private:
			VkDescriptorPool pool_;
			std::vector<VkDescriptorSet> sets_;
			std::vector<VkDescriptorSetLayout> layouts_;
			const LogicDevice* logicDevice_;
		};
	}
}
