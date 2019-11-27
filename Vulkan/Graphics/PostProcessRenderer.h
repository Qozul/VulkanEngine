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
			PostProcessRenderer(RendererCreateInfo& createInfo, uint32_t texture);
			~PostProcessRenderer() = default;
			void recordFrame(const uint32_t frameIdx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList, bool ignoreEboBind = false) override;
		};
	}
}
