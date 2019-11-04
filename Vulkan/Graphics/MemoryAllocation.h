// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		using AllocationID = uint64_t;

		enum class MemoryAllocationPattern {
			// Resources frequently read/write on GPU. Should use transfer access with this.
			kRenderTarget,
			// Resources filled once/rarely on CPU and read frequently on GPU. Should use transfer access with this.
			kStaticResource,
			// Change frequently (e.g. per frame/draw call)
			kDynamicResource,
			// Written by GPU, read by CPU
			kReadback,
			// CPU side for transfer, memory access type is irrelvant when using this pattern
			kStaging
		};

		// If direct access is to be used it must be HOST_VISIBLE
		enum class MemoryAccessType {
			// Transfer needs a staging buffer on CPU and copies accross buffer to buffer
			kTransfer,
			// Direct access using map and unmap
			kDirect,
			// Persistant mapping
			kPersistant
		};
#pragma warning (push)
#pragma warning (disable : 26595)
		struct MemoryAllocationDetails {
			union {
				VkBuffer buffer;
				VkImage image;
			};
			AllocationID id = 0;
			MemoryAccessType access;
			void* mappedData = nullptr; // Only used if access is kPersistant
			VkDeviceSize size = 0;
		};
#pragma warning (pop)
	}
}
