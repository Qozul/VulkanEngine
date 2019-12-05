// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "RenderPass.h"

namespace QZL {
	namespace Graphics {
		class RendererBase;
		class CombinePass : public RenderPass {
			friend class SwapChain;
		protected:
			CombinePass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo);
			~CombinePass();
			void doFrame(FrameInfo& frameInfo) override;
			void createRenderers() override;
			void initRenderPassDependency(std::vector<Image*> dependencyAttachment) override;
		private:
			void createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);

			RendererBase* atmosphereRenderer_;
			RendererBase* environmentRenderer_;
			RendererBase* combineRenderer_;

			Image* colourBuffer_;
			uint32_t diffuseIdx_;
			uint32_t specularIdx_;
			uint32_t albedoIdx_;
			uint32_t ambientIdx_;
		};
	}
}
