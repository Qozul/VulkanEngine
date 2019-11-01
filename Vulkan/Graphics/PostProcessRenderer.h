// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class DeviceMemory;
		class TextureSampler;

		class PostProcessRenderer : public RendererBase {
		public:
			PostProcessRenderer(RendererCreateInfo& createInfo, TextureSampler* geometryColourBuffer, TextureSampler* geometryDepthBuffer);
			~PostProcessRenderer();
			void createDescriptors(const uint32_t entityCount) override;
			void recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
		private:
			TextureSampler* geometryColourBuffer_;
			TextureSampler* geometryDepthBuffer_;
		};
	}
}
