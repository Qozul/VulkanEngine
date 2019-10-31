#pragma once
#include "GameScript.h"
#include "../Graphics/VkUtil.h"
#include "../InputManager.h"
#include "../Assets/AtmosphereParameters.h"
#include "../Graphics/Material.h"
#include "../Graphics/ShaderParams.h"

namespace QZL {
	namespace Graphics {
		class LogicDevice;
		class TextureSampler;
		class ComputePipeline;
		class Image;
	}
	namespace Game {
		class SunScript;
		struct PrecomputedTextures {
			Graphics::Image* transmittanceImage = nullptr;
			Graphics::Image* scatteringImage = nullptr;
			Graphics::Image* gatheringImage = nullptr;
			Graphics::Image* gatheringSumImage = nullptr;
			Graphics::Image* scatteringSumImage = nullptr;
			Graphics::TextureSampler* transmittance = nullptr;
			Graphics::TextureSampler* scattering = nullptr;
			Graphics::TextureSampler* gathering = nullptr;
			Graphics::TextureSampler* gatheringSum = nullptr;
			Graphics::TextureSampler* scatteringSum = nullptr;
		};

		class AtmosphereScript : public GameScript {
			static constexpr int TRANSMITTANCE_TEXTURE_WIDTH = 64;
			static constexpr int TRANSMITTANCE_TEXTURE_HEIGHT = 256;

			static constexpr int SCATTERING_TEXTURE_WIDTH = 32;
			static constexpr int SCATTERING_TEXTURE_HEIGHT = 128;
			static constexpr int SCATTERING_TEXTURE_DEPTH = 64;

			static constexpr int GATHERING_TEXTURE_WIDTH = 32;
			static constexpr int GATHERING_TEXTURE_HEIGHT = 32;

			static constexpr int INVOCATION_SIZE = 8;
		public:
			AtmosphereScript(const GameScriptInitialiser& initialiser, SunScript* sun);
			~AtmosphereScript();
			PrecomputedTextures& getTextures() {
				return textures_;
			}
			Assets::AtmosphereParameters& getParameters() {
				return params_;
			}
			Graphics::ShaderParams* getNewShaderParameters();
			Graphics::Material* getMaterial() {
				return material_;
			}
		protected:
			void start() override;
			void update(float dt, const glm::mat4& parentMatrix) override { }
		private:
			// Creates temporary textures, returned via reference argument. Also creates the member textures.
			void initTextures(const Graphics::LogicDevice* logicDevice, PrecomputedTextures& finalTextures);
			VkDescriptorSetLayoutBinding makeLayoutBinding(const uint32_t binding, VkDescriptorType type, const VkSampler* immutableSamplers = nullptr, VkShaderStageFlags stages = VK_SHADER_STAGE_COMPUTE_BIT);

			Assets::AtmosphereParameters params_;
			Graphics::AtmosphereShaderParams shaderParams_;
			PrecomputedTextures textures_;
			Graphics::Material* material_;

			const Graphics::LogicDevice* logicDevice_;
		};
	}
}
