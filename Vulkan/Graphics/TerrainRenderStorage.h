#pragma once
#include "RenderStorageMeshParams.h"

namespace QZL {
	namespace Graphics {
		class TextureLoader;
		class TextureSampler;
		class LogicDevice;

		struct TerrainParamData {
			TextureSampler* heightMap;
			TextureSampler* diffuse;
		};
		class TerrainRenderStorage : public RenderStorageMeshParams<TerrainParamData> {
		public:
			TerrainRenderStorage(TextureLoader*& textureLoader, const LogicDevice* logicDevice);
			~TerrainRenderStorage();
		protected:
			TerrainParamData resolveParams(GraphicsComponent* instance) override;
		private:
			const LogicDevice* logicDevice_;
			TextureLoader*& textureLoader_;
		};
	}
}
