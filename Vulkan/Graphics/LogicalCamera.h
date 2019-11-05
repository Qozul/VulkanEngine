// Author: Ralph Ridley
// Date: 01/11/19
// A camera is an abstraction of the view matrix, presenting itself as an observer entity. However,
// in this engine a camera would be better placed as a game script and entity, instead this class is a logical collection
// of data to represent and interface with the actual data used by the graphics pipelines.
#pragma once
#include "VkUtil.h"

namespace QZL {
	namespace Graphics {
		struct LogicalCamera {
			glm::mat4 viewMatrix;
			glm::vec3 position;
			void calculateFrustumPlanes(const glm::mat4& mvp, std::array<glm::vec4, 6>& planes) {
				// Based on https://github.com/SaschaWillems/Vulkan/blob/master/base/frustum.hpp
				float n = 0.0f;
				for (int p = 0; p < 6; ++p) {
					for (int i = 0; i < 4; ++i) {
						planes[p][i] = mvp[i].w + mvp[i][(int)n];
					}
					n += 0.51f; // No need to worry about error over the 6 iterations used, but ensures integer cast is always right
					// Normalize the plane
					float length = std::sqrt(planes[p].x * planes[p].x + planes[p].y * planes[p].y + planes[p].z * planes[p].z);
					planes[p] /= length;
				}
			}
		};
	}
}
