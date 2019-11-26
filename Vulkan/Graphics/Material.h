// Author: Ralph Ridley
// Date: 12/10/19
// Define each material type for the renderers

#pragma once
#include "VkUtil.h"
#include "GraphicsTypes.h"

namespace QZL {
	namespace Graphics {
		class TextureManager;
		
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
				uint32_t normalmapIdx;
				uint32_t albedoIdx0;
				uint32_t albedoIdx1;
				uint32_t albedoIdx2;
				uint32_t grassIdx;
			};

			struct Atmosphere {
				uint32_t scatteringIdx;
			};

			struct Particle {
				uint32_t albedoIdx;
			};

			struct PostProcess {
				uint32_t colourBufferIdx;
			};

			struct Water {
				uint32_t dudv;
				uint32_t displacementMap;
				uint32_t normalMap;
			};
			static void loadMaterial(TextureManager* texManager, RendererTypes type, std::string fileName, void* data);
			static RendererTypes stringToType(std::string typeName);

			static const size_t materialTextureCountLUT[(size_t)RendererTypes::kNone];
			static const size_t materialSizeLUT[(size_t)RendererTypes::kNone];

		private:
			static MaterialLoadingFunction getLoadingFunction(RendererTypes type);
			static void loadStaticMaterial(TextureManager* texManager, void* data, std::vector<std::string>& lines);
			static void loadTerrainMaterial(TextureManager* texManager, void* data, std::vector<std::string>& lines);
			static void loadParticleMaterial(TextureManager* texManager, void* data, std::vector<std::string>& lines);
			static void loadWaterMaterial(TextureManager* texManager, void* data, std::vector<std::string>& lines);
		};
	}
}
