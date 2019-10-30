#pragma once
#include "RenderPass.h"

namespace QZL {
	namespace Graphics {

		class GeometryPass : public RenderPass {
			friend class SwapChain;
			enum class SubPass : uint32_t {
				ATMOSPHERE,
				GENERAL,
				SUBPASS_COUNT
			};
		protected:
			GeometryPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd);
			~GeometryPass();
			void doFrame(const glm::mat4& viewMatrix, const uint32_t& idx, VkCommandBuffer cmdBuffer) override;
			void createRenderers() override;
			// No dependency
			void initRenderPassDependency(std::vector<Image*> dependencyAttachment) override { }
		private:
			void createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);
			VkFormat createDepthBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);

			RendererBase* texturedRenderer_;
			RendererBase* terrainRenderer_;
			RendererBase* particleRenderer_;
			RendererBase* atmosphereRenderer_;

			Image* colourBuffer_;
			Image* depthBuffer_;
		};
	}
}
