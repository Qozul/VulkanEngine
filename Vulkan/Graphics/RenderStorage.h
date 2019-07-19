#pragma once
#include "../../Shared/Utility.h"
#include "DrawElementsCommand.h"
#include "Mesh.h"

namespace QZL
{
	namespace Graphics {
		class ElementBuffer;
		class GraphicsComponent;
		class DeviceMemory;

		class RenderStorage {
		public:
			RenderStorage(DeviceMemory* deviceMemory);
			~RenderStorage();

			void addMesh(const std::string& name, BasicMesh* mesh);
			void addInstance(const std::string& name, GraphicsComponent* instance);
			//void addInstance(const std::string& name, TexturedMeshInstance* instance);

			void modifyInstance(const std::string& name, const size_t instanceIndex, GraphicsComponent* instance);
			//void modifyInstance(const std::string& name, const size_t instanceIndex, TexturedMeshInstance* instance);

			DrawElementsCommand* meshData();
			GraphicsComponent** instanceData();
			size_t meshCount();
			size_t instanceCount();

			ElementBuffer* buf();

		private:
			ElementBuffer* buf_;
			std::unordered_map<std::string, size_t> dataMap_;

			std::vector<DrawElementsCommand> meshes_;
			// instances_ and textureData_ should always be an equal size IF textures are being used
			std::vector<GraphicsComponent*> instances_;
		};
	}
}
