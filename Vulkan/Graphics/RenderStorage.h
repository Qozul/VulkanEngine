// Author: Ralph Ridley
// Date: 12/10/19
// Restructured and suitably holds a RenderObject so that parameters on a per mesh or per instance basis can be provided.

#pragma once
#include "../../Shared/Utility.h"
#include "DrawElementsCommand.h"

namespace QZL
{
	namespace Graphics {
		class ElementBufferObject;
		class GraphicsComponent;
		class RenderObject;
		struct ShaderParams;

		class RenderStorage {
		public:
			enum class InstanceUsage {
				kOne, kUnlimited
			};
		public:
			RenderStorage(ElementBufferObject* buffer, InstanceUsage usage);
			virtual ~RenderStorage();
			virtual void addMesh(GraphicsComponent* instance, RenderObject* robject);
			
			DrawElementsCommand* meshData() {
				return drawCmds_.data();
			}
			GraphicsComponent** instanceData() {
				return instances_.data();
			}
			RenderObject** renderObjectData() {
				return renderObjects_.data();
			}
			size_t meshCount() {
				return drawCmds_.size();
			}
			size_t instanceCount() {
				return instances_.size();
			}
			size_t renderObjectCount() {
				return renderObjects_.size();
			}
			ElementBufferObject* buffer() {
				return buffer_;
			}

		protected:
			virtual void addInstance(DrawElementsCommand& cmd, GraphicsComponent* instance, uint32_t index);
			void addMeshOneInstance(GraphicsComponent* instance, RenderObject* robject);
			void addMeshUnlimitedInstances(GraphicsComponent* instance, RenderObject* robject);

			InstanceUsage usage_;
			ElementBufferObject* buffer_;

			std::unordered_map<std::string, size_t> dataMap_;

			// renderObjects_ and drawCmds_ are 1-1
			std::vector<RenderObject*> renderObjects_;
			std::vector<DrawElementsCommand> drawCmds_;
			std::vector<GraphicsComponent*> instances_;
		};
	}
}
