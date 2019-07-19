/// Author: Ralph Ridley
/// Date: 31/12/18
#pragma once

#include "ShaderPipeline.h"

namespace QZL
{
	namespace Shared
	{
		class RendererBase {
		public:
			// pipeline must by dynamically allocated
			RendererBase(ShaderPipeline* pipeline) : pipeline_(pipeline) {};
			virtual ~RendererBase() {
				SAFE_DELETE(pipeline_);
			}
			virtual void initialise() = 0;
			virtual void doFrame(const glm::mat4& viewMatrix) = 0;
		protected:
			ShaderPipeline* pipeline_;
		};
	}
}
