// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "../../Shared/Utility.h"

namespace QZL
{
	namespace Graphics {
		struct DrawElementsCommand {
			uint32_t count;
			uint32_t instanceCount;
			uint32_t firstIndex;
			uint32_t baseVertex;
			uint32_t baseInstance;
			DrawElementsCommand(uint32_t idxCount, uint32_t instCount, uint32_t firstIdx, uint32_t baseVert, uint32_t baseInst)
				: count(idxCount), instanceCount(instCount), firstIndex(firstIdx), baseVertex(baseVert), baseInstance(baseInst)
			{
			}
		};
	}
}
