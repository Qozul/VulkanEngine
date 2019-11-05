// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class TerrainRenderer : public RendererBase {
			struct TessControlInfo {
				float distanceFarMinusClose = 0.0f;
				float closeDistance = 0.0f;
				float patchRadius = 0.0f;
				float maxTessellationWeight = 0.0f;
				std::array<glm::vec4, 6> frustumPlanes;
			};
		public:
			TerrainRenderer(RendererCreateInfo& createInfo);
			~TerrainRenderer();
			void createDescriptors(const uint32_t entityCount) override;
			void recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
		private:
			void updateBuffers(LogicalCamera& camera);

			TessControlInfo tessCtrlInfo_;
		};
	}
}
