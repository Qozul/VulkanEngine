#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class VertexUtility {
			friend struct Vertex;
		private:
			static VkVertexInputBindingDescription fillBindDesc(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate);
			static void fillAttribDescs(VkVertexInputAttributeDescription& attribDesc, uint32_t location, uint32_t binding, VkFormat format, uint32_t offset);
		};

		struct Vertex {
			float x, y, z;
			float u, v;
			float nx, ny, nz;

			static VkVertexInputBindingDescription getBindDesc(uint32_t binding, VkVertexInputRate inputRate)
			{
				// Describes at which rate to load data from memory (instance/vertex) and number of bytes between data entries
				return VertexUtility::fillBindDesc(binding, sizeof(Vertex), inputRate);
			}
			static std::array<VkVertexInputAttributeDescription, 3> getAttribDescs(uint32_t binding)
			{
				// Describes how to extract a vertex attribute from a chunk of vertex data
				std::array<VkVertexInputAttributeDescription, 3> attribDescs;
				VertexUtility::fillAttribDescs(attribDescs[0], 0, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, x));
				VertexUtility::fillAttribDescs(attribDescs[1], 1, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, u));
				VertexUtility::fillAttribDescs(attribDescs[2], 2, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, nx));
				return attribDescs;
			}
		};
	}
}
