#pragma once
#include "RenderStorageMeshParams.h"

namespace QZL {
	namespace Graphics {
		class TextureLoader;
		class TextureSampler;
		class LogicDevice;
		
		struct StaticParamData {
			TextureSampler* diffuse;
			TextureSampler* normalMap;
		};
		class StaticRenderStorage : public RenderStorageMeshParams<StaticParamData> {
		public:
			StaticRenderStorage(TextureLoader*& textureLoader, const LogicDevice* logicDevice);
			~StaticRenderStorage();
		protected:
			StaticParamData resolveParams(GraphicsComponent* instance) override;
		private:
			const LogicDevice* logicDevice_;
			TextureLoader*& textureLoader_;
		};
	}
}
