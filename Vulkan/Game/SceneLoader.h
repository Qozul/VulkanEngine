// Author: Ralph Ridley
// Date 11/11/19
#pragma once
#include "../../Shared/Utility.h"
#include "../SystemMasters.h"

namespace QZL {
	namespace Game {
		class Scene;
		class SceneLoader {
		public:
			static Scene* loadScene(const SystemMasters& masters, const std::string name);
		};
	}
}
