#pragma once
#include "../../Shared/Utility.h"
#include "DrawElementsCommand.h"
#include "Mesh.h"

namespace QZL
{
	namespace Graphics {
		class ElementBufferInterface;
		class GraphicsComponent;
		class ShaderParams;

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

		// Simplifies RenderStorage to only care about single meshes without instances.
		// This should be used for when there are not expected to be many entities in the renderer.
		class RenderStorageNoInstances : public RenderStorage {
		public:
			RenderStorageNoInstances(ElementBufferInterface* buffer)
				: RenderStorage(buffer) { }
			virtual ~RenderStorageNoInstances() { }
			// Mesh must be added with one instance
			virtual void addMesh(GraphicsComponent* instance, BasicMesh* mesh) override {
				meshes_.emplace_back(mesh->indexCount, 1, mesh->indexOffset, mesh->vertexOffset, 0);
				instances_.push_back(instance);
			}
		};
	}
}
