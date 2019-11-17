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
			RenderStorage(ElementBufferObject* buffer);
			virtual ~RenderStorage();
			virtual void addMesh(GraphicsComponent* instance, RenderObject* robject);
			
			DrawElementsCommand* meshData() {
				return drawCmds_.data();
			}
			size_t meshCount() {
				return drawCmds_.size();
			}
			ElementBufferObject* buffer() {
				return buffer_;
			}

		protected:
			void addMeshUnlimitedInstances(GraphicsComponent* instance, RenderObject* robject);
			void setBaseInstances();

			ElementBufferObject* buffer_;

			std::unordered_map<std::string, size_t> dataMap_;
			std::vector<DrawElementsCommand> drawCmds_;
		};
	}
}
