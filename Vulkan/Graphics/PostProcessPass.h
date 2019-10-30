#pragma once
#include "RenderPass.h"
#include "RendererBase.h"

namespace QZL {
	namespace Game {
		class AtmosphereScript;
	}
	namespace Graphics {
		class ComputePipeline;

		class PostProcessPass : public RenderPass {
			friend class SwapChain;
			friend class GraphicsMaster;

			struct ComputePushConstants {
				glm::vec3 camPos;
				float padding1;
				glm::vec3 zenithDir;
				float cameraHeight;
				glm::vec3 sunDir;
				float padding;
				glm::mat4 inverseViewProj;
			};
			enum class SubPass : uint32_t {
				AERIAL_PERSPECTIVE,
				SUBPASS_COUNT
			};
			static constexpr int INVOCATION_SIZE = 8;
			static constexpr int AP_WIDTH  = 32;
			static constexpr int AP_HEIGHT = 32;
			static constexpr int AP_DEPTH  = 16;

		protected:
			PostProcessPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd);
			~PostProcessPass();
			void doFrame(const glm::mat4& viewMatrix, const uint32_t& idx, VkCommandBuffer cmdBuffer) override;
			void createRenderers() override;
			// Dependency on the general pass to produce depth and colour of the scene.
			void initRenderPassDependency(std::vector<Image*> dependencyAttachment) override;
			void attachAtmosphereScript(Game::AtmosphereScript* script) {
				atmosphereScript_ = script;
			}
		private:
			void createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);
			void createComputePipelines() {}

			RendererBase* postProcessRenderer_;

			PushConstantInfo pushConstantInfo_;
			ComputePushConstants pushConstants_;
			uint32_t computeDescriptorIdx_;
			DescriptorBuffer* paramsBuf_;
			// [0] = scattering, [1] = transmittance (transition to compute), [2] = scattering, [3] = transmittance (transition from compute)
			std::array<VkImageMemoryBarrier, 4> memoryBarriers_; 

			Image* colourBuffer_;

			Image* geometryColourBuf_;
			Image* geometryDepthBuf_;

			// Samplers for the images produced in the GeometryPass render pass.
			TextureSampler* gpColourBuffer_;
			TextureSampler* gpDepthBuffer_;
			Game::AtmosphereScript* atmosphereScript_; // Owned by the atmosphereScript a
		};
	}
}
