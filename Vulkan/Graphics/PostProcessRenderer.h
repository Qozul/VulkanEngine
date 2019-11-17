// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class DeviceMemory;
		class TextureSampler;
		struct Material;
		class GraphicsComponent;
		struct AtmosphereShaderParams;

		class PostProcessRenderer : public RendererBase {
		public:
			PostProcessRenderer(RendererCreateInfo& createInfo);
			~PostProcessRenderer() = default;
			void recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
		};
	}
}
