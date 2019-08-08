#pragma once
#include "RenderStorageMeshParams.h"

namespace QZL {
	namespace Graphics {
		class TextureManager;
		class TextureSampler;
		class LogicDevice;

		struct TerrainParamData {
			TextureSampler* heightMap;
			TextureSampler* diffuse;
		};
		class TerrainRenderStorage : public RenderStorageMeshParams<TerrainParamData> {
		public:
			TerrainRenderStorage(TextureManager* textureManager, const LogicDevice* logicDevice, ElementBufferInterface* buffer);
			~TerrainRenderStorage();
		protected:
			TerrainParamData resolveParams(GraphicsComponent* instance) override;
		private:
			const LogicDevice* logicDevice_;
			TextureManager* textureManager_;
		};
	}
}
