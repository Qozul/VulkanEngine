// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "RenderPass.h"

namespace QZL {
	struct InputProfile;
	namespace Game {
		class AtmosphereScript;
	}
	namespace Graphics {
		class RendererBase;

		struct PostPushConstants {
			uint32_t colourIdx;
			uint32_t depthIdx;
			uint32_t shadowDepthIdx;
			float farZ;
			float nearZ;
			float screenX;
			float screenY;
		};
		class PostProcessPass : public RenderPass {
			enum class Effects {
				kFXAA, kDOFVertical, kDOFHorizontal, kCount
			};
			friend class SwapChain;
		protected:
			PostProcessPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo);
			~PostProcessPass();
			void doFrame(FrameInfo& frameInfo) override;
			void createRenderers() override;
			// Dependency on the general pass to produce depth and colour of the scene.
			void initRenderPassDependency(std::vector<Image*> dependencyAttachment) override;
		private:
			void createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);
			void createPasses();

			VkRenderPass renderPassPresent_;
			VkRenderPass renderPass2_;
			std::vector<VkFramebuffer> framebuffers2_;
			std::vector<VkFramebuffer> framebuffersPresent_;

			RendererBase* presentRenderer_;
			RendererBase* fxaa_;
			RendererBase* depthOfFieldH_;
			RendererBase* depthOfFieldV_;

			InputProfile* input_;
			std::array<std::pair<bool, RendererBase*>, (size_t)Effects::kCount> effectStates_;

			Image* colourBuffer1_;

			// Hold information from previous pass
			Image* geometryColourBuf_;
			Image* geometryDepthBuf_;

			uint32_t colourBufferIdx_;;
			// Samplers for the images produced in the GeometryPass render pass.
			uint32_t gpColourBuffer_;
			uint32_t gpDepthBuffer_;

			bool doFXAA_;
			bool doDoF_;
		};
	}
}
