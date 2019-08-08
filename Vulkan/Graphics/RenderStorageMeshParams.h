#pragma once
#include "RenderStorage.h"
#include "ShaderParams.h"
#include "GraphicsComponent.h"

namespace QZL {
	namespace Graphics {
		// Adds shader parameter data 1-1 with the meshes, where differences in param id even with the same mesh are
		// considered different for the purposes of rendering (to correctly push descriptor updates before draw cmd).
		template <typename ParamData>
		class RenderStorageMeshParams : public RenderStorage {
		public:
			RenderStorageMeshParams(ElementBufferInterface* buffer);

			virtual void addMesh(GraphicsComponent* instance, BasicMesh* mesh) override;

			const ParamData& getParamData(size_t idx) {
				return paramData_[idx];
			}
		protected:
			virtual ~RenderStorageMeshParams();
			// Parameters from the ShaderParams of the component need to be resolved to the ParamData struct given
			virtual ParamData resolveParams(GraphicsComponent* instance) = 0;
			std::vector<ParamData> paramData_;
		};
		template<typename ParamData>
		inline RenderStorageMeshParams<ParamData>::RenderStorageMeshParams(ElementBufferInterface* buffer)
			: RenderStorage(buffer)
		{
		}
		template<typename ParamData>
		inline void RenderStorageMeshParams<ParamData>::addMesh(GraphicsComponent* instance, BasicMesh* mesh)
		{
			auto params = instance->getShaderParams();
			auto fullName = params->getParamsId() + "." + instance->getMeshName();
			auto keyIt = dataMap_.find(fullName);
			if (keyIt != dataMap_.end()) {
				auto& cmd = meshes_[dataMap_[fullName]];
				addInstance(cmd, instance, cmd.baseInstance);
			}
			else {
				dataMap_[fullName] = meshes_.size();
				auto index = instances_.size();
				meshes_.emplace_back(mesh->indexCount, 0, mesh->indexOffset, mesh->vertexOffset, index);
				paramData_.push_back(resolveParams(instance));
				addInstance(meshes_[dataMap_[fullName]], instance, index);
			}
		}
		template<typename ParamData>
		inline RenderStorageMeshParams<ParamData>::~RenderStorageMeshParams()
		{
		}
	}
}
