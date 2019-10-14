#pragma once
#include "../Graphics/VkUtil.h"
#include "../Graphics/Material.h"
#include "AtmosphereParameters.h"
/*
namespace QZL {
	namespace Graphics {
		class LogicDevice;
		class TextureSampler;
		class ComputePipeline;
		class Image;
	}
	namespace Assets {

		constexpr int TRANSMITTANCE_TEXTURE_WIDTH = 64;
		constexpr int TRANSMITTANCE_TEXTURE_HEIGHT = 256;

		constexpr int SCATTERING_TEXTURE_WIDTH = 32;
		constexpr int SCATTERING_TEXTURE_HEIGHT = 128;
		constexpr int SCATTERING_TEXTURE_DEPTH = 64;

		constexpr int GATHERING_TEXTURE_WIDTH = 32;
		constexpr int GATHERING_TEXTURE_HEIGHT = 32;

		constexpr int INVOCATION_SIZE = 8;

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

		class Atmosphere {
			friend class Skysphere;
		public:
			Atmosphere(); // Uses earth model
			Atmosphere(AtmosphereParameters params);
			~Atmosphere();

			void precalculateTextures(const Graphics::LogicDevice* logicDevice);
			PrecomputedTextures& getTextures() {
				return textures_;
			}
			AtmosphereParameters& getParameters() {
				return params_;
			}
		private:
			// Creates temporary textures, returned via reference argument. Also creates the member textures.
			void initTextures(const Graphics::LogicDevice* logicDevice, PrecomputedTextures& finalTextures);
			VkDescriptorSetLayoutBinding makeLayoutBinding(const uint32_t binding, const VkSampler* immutableSamplers = nullptr);

			AtmosphereParameters params_;
			Graphics::MaterialAtmosphere material_;
			PrecomputedTextures textures_;
		};
	}
}
*/