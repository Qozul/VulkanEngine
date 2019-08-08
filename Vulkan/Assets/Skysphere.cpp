#include "Skysphere.h"

using namespace QZL;
using namespace Assets;

void Skysphere::loadFunction(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::Vertex>& vertices)
{
	// Generate an icosphere
	const float radius = 1000.0f;
	const float cullingTheta = 20.0f; // All patches below this angle beneath the horizon are culled
	const int subdivisionCount = 1; // Just get a general sphere shape, don't need to much accuracy because it will be tessellated at runtime

	createIscosahedron(indices, vertices);
	for (int i = 0; i < subdivisionCount; ++i) {
		subdivideIcosahredon(indices, vertices);
	}
}

// Based on http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html
void Skysphere::createIscosahedron(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::Vertex>& vertices)
{
	auto t = (1.0 + std::sqrt(5.0)) / 2.0;
	/*
	vertices.emplace(new Point3D(-1, t, 0));
	addVertex(new Point3D(1, t, 0));
	addVertex(new Point3D(-1, -t, 0));
	addVertex(new Point3D(1, -t, 0));

	addVertex(new Point3D(0, -1, t));
	addVertex(new Point3D(0, 1, t));
	addVertex(new Point3D(0, -1, -t));
	addVertex(new Point3D(0, 1, -t));

	addVertex(new Point3D(t, 0, -1));
	addVertex(new Point3D(t, 0, 1));
	addVertex(new Point3D(-t, 0, -1));
	addVertex(new Point3D(-t, 0, 1));
	*/
}

void Skysphere::subdivideIcosahredon(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::Vertex>& vertices)
{
}
