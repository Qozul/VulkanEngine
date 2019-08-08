#pragma once
#include "../../Shared/Utility.h"
#include "DrawElementsCommand.h"
#include "Mesh.h"

namespace QZL
{
	namespace Graphics {
		class ElementBufferInterface;
		class GraphicsComponent;

		class RenderStorage {
		public:
			RenderStorage(ElementBufferInterface* buffer);
			virtual ~RenderStorage();
			// Mesh must be added with one instance
			virtual void addMesh(GraphicsComponent* instance, BasicMesh* mesh);
			
			DrawElementsCommand* meshData() {
				return meshes_.data();
			}
			GraphicsComponent** instanceData() {
				return instances_.data();
			}
			size_t meshCount() {
				return meshes_.size();
			}
			size_t instanceCount() {
				return instances_.size();
			}
			ElementBufferInterface* buf() {
				return buf_;
			}

		protected:
			virtual void addInstance(DrawElementsCommand& cmd, GraphicsComponent* instance, uint32_t index);

			ElementBufferInterface* buf_;
			std::unordered_map<std::string, size_t> dataMap_; // Mesh name to offset in to meshes_

			std::vector<DrawElementsCommand> meshes_;
			std::vector<GraphicsComponent*> instances_;
		};
	}
}
