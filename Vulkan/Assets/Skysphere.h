#pragma once
#include "Entity.h"

namespace QZL {
	namespace Assets {
		class Atmosphere;
		class Skysphere : public Entity {
		public:
			Skysphere(Atmosphere* atmosphere);
			~Skysphere();
		private:
			Atmosphere* atmos_;

			static void loadFunction(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::VertexOnlyPosition>& vertices);
			static void createIscosahedron(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::VertexOnlyPosition>& vertices);
			static void subdivideIcosahredon(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::VertexOnlyPosition>& vertices);
			static Graphics::IndexType icosahedronMiddlePoint(std::vector<Graphics::VertexOnlyPosition>& vertices,
				std::map<uint64_t, Graphics::IndexType>& indexCache, Graphics::IndexType p0, Graphics::IndexType p1);
			static Graphics::IndexType addIcosahedronVertex(std::vector<Graphics::VertexOnlyPosition>& vertices, glm::vec3 pos);
		};
	}
}
