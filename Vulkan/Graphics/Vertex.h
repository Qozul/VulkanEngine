// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		enum class VertexType {
			POSITION_ONLY,
			POSITION_UV_NORMAL
		};

		inline VkVertexInputBindingDescription makeVertexBindingDescription(uint32_t binding, uint32_t sizeOfVertex, VkVertexInputRate inputRate) {
			VkVertexInputBindingDescription desc = {};
			desc.binding = binding;
			desc.inputRate = inputRate;
			desc.stride = sizeOfVertex;
			return desc;
		}

		inline std::vector<VkVertexInputAttributeDescription> makeVertexAttribDescriptions(uint32_t binding, std::vector<std::pair<uint32_t, VkFormat>> attribInfo) {
			std::vector<VkVertexInputAttributeDescription> attribDescriptions(attribInfo.size());
			for (uint32_t i = 0; i < attribDescriptions.size(); ++i) {
				attribDescriptions[i].location = i;
				attribDescriptions[i].binding = binding;
				attribDescriptions[i].offset = attribInfo[i].first;
				attribDescriptions[i].format = attribInfo[i].second;
			}
			return attribDescriptions;
		}

		struct Vertex {
			float x, y, z;
			float u, v;
			float nx, ny, nz;

			Vertex(float x = 0.0f, float y = 0.0f, float z = 0.0f, float u = 0.0f, float v = 0.0f, float nx = 0.0f, float ny = 0.0f, float nz = 0.0f)
				: x(x), y(y), z(z), u(u), v(v), nx(nx), ny(ny), nz(nz) {}

			static std::vector<std::pair<uint32_t, VkFormat>> makeAttribInfo() {
				return { 
					{ static_cast<uint32_t>(offsetof(Vertex, x)), VK_FORMAT_R32G32B32_SFLOAT }, 
					{ static_cast<uint32_t>(offsetof(Vertex, u)), VK_FORMAT_R32G32_SFLOAT },
					{ static_cast<uint32_t>(offsetof(Vertex, nx)), VK_FORMAT_R32G32B32_SFLOAT }
				};
			}
		};
#pragma pack(push, 1)
		struct VertexOnlyPosition {
			glm::vec3 pos;

			VertexOnlyPosition(float x = 0.0f, float y = 0.0f, float z = 0.0f)
				: pos(x, y, z) {}
			VertexOnlyPosition(glm::vec3 p)
				: pos(p) {}

			static std::vector<std::pair<uint32_t, VkFormat>> makeAttribInfo() {
				return {
					{ offsetof(VertexOnlyPosition, pos.x), VK_FORMAT_R32G32B32_SFLOAT }
				};
			}
		};
#pragma pack(pop)
		struct ParticleVertex {
			glm::vec3 position;
			float scale = 0.0f;
			glm::vec2 textureOffset;

			static std::vector<std::pair<uint32_t, VkFormat>> makeAttribInfo() {
				return {
					{ offsetof(ParticleVertex, position.x), VK_FORMAT_R32G32B32_SFLOAT },
					{ offsetof(ParticleVertex, scale), VK_FORMAT_R32_SFLOAT },
					{ offsetof(ParticleVertex, textureOffset.x), VK_FORMAT_R32G32_SFLOAT }
				};
			}
		};
	}
}
