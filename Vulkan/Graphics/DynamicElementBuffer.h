// Author: Ralph Ridley
// Date: 03/11/19
#pragma once
#include "ElementBufferObject.h"

namespace QZL {
	namespace Graphics {
		class DynamicElementBuffer : public ElementBufferObject {
		public:
			DynamicElementBuffer(DeviceMemory* deviceMemory, size_t swapChainImageCount, size_t sizeOfVertices, size_t sizeOfIndices = 0);
			~DynamicElementBuffer() { }

			void commit() override;
			void bind(VkCommandBuffer cmdBuffer, const size_t idx) override;
			void updateBuffer(VkCommandBuffer& cmdBuffer, const uint32_t& idx) override;
			SubBufferRange allocateSubBufferRange(size_t count) override;
			void* getSubBufferData(size_t firstVertex) override;

		private:
			size_t swapChainImageCount_;
		};
	}
}
