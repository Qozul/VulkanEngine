// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class DeviceMemory;
		class TextureManager;
		class TextureSampler;

		// Draws atmospheres from within the atmosphere
		class AtmosphereRenderer : public RendererBase {
		public:
			AtmosphereRenderer(RendererCreateInfo& createInfo);
			~AtmosphereRenderer();
			void createDescriptors(const uint32_t entityCount) override;
			void recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
		};
	}
}
