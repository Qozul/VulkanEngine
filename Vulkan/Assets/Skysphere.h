#pragma once
#include "Entity.h"

namespace QZL {
	namespace Assets {
		class Atmosphere;
		class Skysphere : public Entity {
		public:
			Skysphere(Atmosphere* atmosphere);
		private:
			static void loadFunction(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::Vertex>& vertices);
			static void createIscosahedron(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::Vertex>& vertices);
			static void subdivideIcosahredon(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::Vertex>& vertices);
		};
	}
}
