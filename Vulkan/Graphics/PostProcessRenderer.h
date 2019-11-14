// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class DeviceMemory;
		class TextureSampler;
		class Material;
		class GraphicsComponent;
		struct AtmosphereShaderParams;

		class PostProcessRenderer : public RendererBase {
		public:
			PostProcessRenderer(RendererCreateInfo& createInfo, uint32_t geometryColourBuffer, uint32_t geometryDepthBuffer);
			~PostProcessRenderer();
			void createDescriptors(const uint32_t entityCount) override;
			void recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
			void registerComponent(GraphicsComponent* component, RenderObject* robject) override;
		private:
			uint32_t geometryColourBuffer_;
			uint32_t geometryDepthBuffer_;
		};
	}
}
