#pragma once
#include "GameScript.h"
#include "../Graphics/VkUtil.h"
#include "../InputManager.h"
#include "../Assets/AtmosphereParameters.h"
#include "../Graphics/Material.h"

namespace QZL {
	namespace Graphics {
		class LogicDevice;
		class TextureSampler;
		class ComputePipeline;
		class Image;
	}
	namespace Game {
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
			AtmosphereScript(const GameScriptInitialiser& initialiser);
			~AtmosphereScript();
			PrecomputedTextures& getTextures() {
				return textures_;
			}
			Assets::AtmosphereParameters& getParameters() {
				return params_;
			}
			Graphics::MaterialAtmosphere& getMaterial() {
				return material_;
			}
		protected:
			void start() override;
			void update(float dt) override { }
		private:
			// Creates temporary textures, returned via reference argument. Also creates the member textures.
			void initTextures(const Graphics::LogicDevice* logicDevice, PrecomputedTextures& finalTextures);
			VkDescriptorSetLayoutBinding makeLayoutBinding(const uint32_t binding, const VkSampler* immutableSamplers = nullptr);

			Assets::AtmosphereParameters params_;
			Graphics::MaterialAtmosphere material_;
			PrecomputedTextures textures_;

			const Graphics::LogicDevice* logicDevice_;
		};
	}
}
