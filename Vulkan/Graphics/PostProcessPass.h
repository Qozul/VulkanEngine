// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "RenderPass.h"

namespace QZL {
	namespace Game {
		class AtmosphereScript;
	}
	namespace Graphics {
		class RendererBase;
		class PostProcessPass : public RenderPass {
			friend class SwapChain;
			enum class SubPass : uint32_t {
				kAerialPerspective,
				kSubpassCount
			};
		protected:
			PostProcessPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd);
			~PostProcessPass();
			void doFrame(LogicalCamera& camera, const uint32_t& idx, VkCommandBuffer cmdBuffer) override;
			void createRenderers() override;
			// Dependency on the general pass to produce depth and colour of the scene.
			void initRenderPassDependency(std::vector<Image*> dependencyAttachment) override;
		private:
			void createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);
			VkFormat createDepthBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);
			void createComputePipelines() {}

			RendererBase* postProcessRenderer_;
			RendererBase* particleRenderer_;

			Image* colourBuffer_;
			Image* depthBuffer_;

			Image* geometryColourBuf_;
			Image* geometryDepthBuf_;

			// Samplers for the images produced in the GeometryPass render pass.
			uint32_t gpColourBuffer_;
			uint32_t gpDepthBuffer_;
		};
	}
}
