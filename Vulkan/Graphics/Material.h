// Author: Ralph Ridley
// Date: 12/10/19
// Define each material type for the renderers

#pragma once
#include "VkUtil.h"
#include "GraphicsTypes.h"

namespace QZL {
	namespace Graphics {
		class TextureManager;

		enum struct MaterialType {
			kStatic, kTerrain, kAtmosphere, kParticle, kSize
		};
		
		struct Material {
			void* data;
			size_t size;
		};

		struct Materials {
			using MaterialLoadingFunction = void(*)(TextureManager* texManager, void* data, std::vector<std::string>& lines);
		public:
			struct Static {
				uint32_t albedoIdx;
				uint32_t normalmapIdx;
			};

			struct Terrain {
				uint32_t heightmapIdx;
				uint32_t normalmapIdx;
				uint32_t albedoIdx;
			};

			struct Atmosphere {
				uint32_t scatteringIdx;
			};

			struct Particle {
				uint32_t albedoIdx;
			};
			static void loadMaterial(TextureManager* texManager, MaterialType type, std::string fileName, void* data);
			static MaterialType stringToType(std::string typeName);

			static const size_t materialTextureCountLUT[(size_t)MaterialType::kSize];
			static const size_t materialSizeLUT[(size_t)MaterialType::kSize];

		private:
			static MaterialLoadingFunction getLoadingFunction(MaterialType type);
			static void loadStaticMaterial(TextureManager* texManager, void* data, std::vector<std::string>& lines);
			static void loadTerrainMaterial(TextureManager* texManager, void* data, std::vector<std::string>& lines);
			static void loadParticleMaterial(TextureManager* texManager, void* data, std::vector<std::string>& lines);
		};
	}
}
