#pragma once
#include "RenderStorage.h"

namespace QZL {
	namespace Graphics {
		class TextureLoader;
		class TextureSampler;
		class LogicDevice;
		// Fundamentally the base class behaviour is unchanged, however for the purposes of separating entities based on texture and meshes
		// where the same mesh can have different textures (determined by a given instance) then in its simplest form the key essentially becomes 
		// a tuple of the texture names and the mesh name
		class StaticRenderStorage : public RenderStorage {
		public:
			StaticRenderStorage(TextureLoader*& textureLoader, const LogicDevice* logicDevice);

			void addMesh(GraphicsComponent* instance, BasicMesh* mesh) override;

			TextureSampler* getDiffuseTexture(size_t idx) {
				return diffuseTextures_[idx];
			}
			TextureSampler* getNormalMap(size_t idx) {
				return normalMaps_[idx];
			}
		protected:
			void addInstance(DrawElementsCommand& cmd, GraphicsComponent* instance, uint32_t index) override;
		private:
			const LogicDevice* logicDevice_;
			TextureLoader*& textureLoader_;

			std::map<std::tuple<std::string, std::string, std::string>, size_t> texturedDataMap_;
			// Textures are 1-1 with the draw element commands (meshes)
			std::vector<TextureSampler*> diffuseTextures_;
			std::vector<TextureSampler*> normalMaps_;
		};
	}
}
