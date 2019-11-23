// Author: Ralph Ridley
// Date: 23/11/19
#pragma once
#include "RenderPass.h"

namespace QZL {
	namespace Graphics {
		class RendererBase;
		class DeferredPass : public RenderPass {
			friend class SwapChain;
		protected:
			DeferredPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo);
			~DeferredPass();
			void doFrame(FrameInfo& frameInfo) override;
			void createRenderers() override;
			void initRenderPassDependency(std::vector<Image*> dependencyAttachment) override;
		private:
			void createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);
			VkFormat createDepthBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);

			RendererBase* staticRenderer_;
			RendererBase* terrainRenderer_;
			RendererBase* particleRenderer_;
			RendererBase* waterRenderer_;

			Image* depthBuffer_;
			Image* normalsBuffer_;
			Image* albedoBuffer_;
			Image* positionBuffer_;

			uint32_t shadowDepthIdx_;
		};
	}
}
