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
			virtual ~RenderStorage();

			// Mesh must be added with one instance
			virtual void addMesh(GraphicsComponent* instance, BasicMesh* mesh);

			//virtual void modifyInstance(DrawElementsCommand& cmd, const size_t instanceIndex, GraphicsComponent* instance);

			DrawElementsCommand* meshData();
			GraphicsComponent** instanceData();
			size_t meshCount();
			size_t instanceCount();

			ElementBuffer* buf();

		protected:
			virtual void addInstance(DrawElementsCommand& cmd, GraphicsComponent* instance, uint32_t index);

			ElementBuffer* buf_;
			std::unordered_map<std::string, size_t> dataMap_; // Mesh name to offset in to meshes_

			std::vector<DrawElementsCommand> meshes_;
			std::vector<GraphicsComponent*> instances_;
		};
	}
}
