#pragma once
#include "GameScript.h"
#include "../../Shared/Utility.h"
#include "../InputManager.h"

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

			constexpr int TRANSMITTANCE_TEXTURE_WIDTH = 64;
			constexpr int TRANSMITTANCE_TEXTURE_HEIGHT = 256;

			constexpr int SCATTERING_TEXTURE_WIDTH = 32;
			constexpr int SCATTERING_TEXTURE_HEIGHT = 128;
			constexpr int SCATTERING_TEXTURE_DEPTH = 64;

			constexpr int GATHERING_TEXTURE_WIDTH = 32;
			constexpr int GATHERING_TEXTURE_HEIGHT = 32;

			constexpr int INVOCATION_SIZE = 8;
		public:
			AtmosphereScript(const GameScriptInitialiser& initialiser, AtmosphereParameters params);
			~AtmosphereScript();
		protected:
			void start() override;
			void update(float dt) override;
		};
	}
}
