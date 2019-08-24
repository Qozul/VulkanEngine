#pragma once
#include "Entity.h"

namespace QZL {
	namespace Assets {
		class Atmosphere;
		class Skysphere : public Entity {
		public:
			Skysphere(const Graphics::LogicDevice* logicDevice, Atmosphere* atmosphere);
			~Skysphere();
		private:
			Atmosphere* atmos_;

			static void loadFunction(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::VertexOnlyPosition>& vertices);
		};
	}
}
