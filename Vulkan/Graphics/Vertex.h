#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class VertexUtility {
			friend struct Vertex;
			friend struct VertexOnlyPosition;
		private:
			static VkVertexInputBindingDescription fillBindDesc(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate);
			static void fillAttribDescs(VkVertexInputAttributeDescription& attribDesc, uint32_t location, uint32_t binding, VkFormat format, uint32_t offset);
		};

		enum class VertexType {
			POSITION_ONLY,
			POSITION_UV_NORMAL
		};

		struct Vertex {
			float x, y, z;
			float u, v;
			float nx, ny, nz;

			Vertex(float x = 0.0f, float y = 0.0f, float z = 0.0f, float u = 0.0f, float v = 0.0f, float nx = 0.0f, float ny = 0.0f, float nz = 0.0f)
				: x(x), y(y), z(z), u(u), v(v), nx(nx), ny(ny), nz(nz) {}

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
#pragma pack(push, 1)
		struct VertexOnlyPosition {
			glm::vec3 pos;

			VertexOnlyPosition(float x = 0.0f, float y = 0.0f, float z = 0.0f)
				: pos(x, y, z) {}
			VertexOnlyPosition(glm::vec3 p)
				: pos(p) {}

			static VkVertexInputBindingDescription getBindDesc(uint32_t binding, VkVertexInputRate inputRate)
			{
				// Describes at which rate to load data from memory (instance/vertex) and number of bytes between data entries
				return VertexUtility::fillBindDesc(binding, sizeof(VertexOnlyPosition), inputRate);
			}
			static std::array<VkVertexInputAttributeDescription, 1> getAttribDescs(uint32_t binding)
			{
				// Describes how to extract a vertex attribute from a chunk of vertex data
				std::array<VkVertexInputAttributeDescription, 1> attribDescs;
				VertexUtility::fillAttribDescs(attribDescs[0], 0, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexOnlyPosition, pos.x));
				return attribDescs;
			}
		};
#pragma pack(pop)
		struct ParticleVertex {
			glm::vec3 position;
			float scale;
			glm::vec2 textureOffset;
		};
	}
}
