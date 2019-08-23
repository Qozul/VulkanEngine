#include "Skysphere.h"
#include "../Graphics/AtmosphereShaderParams.h"
#include "AltAtmosphere.h"
#include "../Graphics/Vertex.h"

using namespace QZL;
using namespace Assets;
using Vertex = QZL::Graphics::Vertex;

Skysphere::Skysphere(const Graphics::LogicDevice* logicDevice, Atmosphere* atmosphere)
	: atmos_(atmosphere)
{
	atmosphere->precalculateTextures(logicDevice);
	setGraphicsComponent(Graphics::RendererTypes::ATMOSPHERE, new Graphics::AtmosphereShaderParams(atmosphere->textures_, atmosphere->material_), "skysphere", loadFunction);
	if (atmosphere != nullptr) {
		//transform_->position = glm::vec3(0.0f, -10.0f, 0.0f);
		//transform_->scale = glm::vec3(32.0f, 128.0f, 1.0f);
		transform_->scale = glm::vec3(80000.0f);
		//transform_->rotation = glm::vec3(1.0f, 0.0f, 0.0f);
		//transform_->angle = 180.0f;
	}
}

Skysphere::~Skysphere()
{
	SAFE_DELETE(atmos_);
}

void Skysphere::loadFunction(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::VertexOnlyPosition>& vertices)
{
	//vertices = { Vertex(), Vertex(0.0f, 1.0f, 0.0f, 0.0f, 1.0f), Vertex(1.0f, 1.0f, 0.0, 1.0f, 1.0f), Vertex(1.0f, 0.0f, 0.0f, 1.0f, 0.0f) };
	//indices = { 0, 1, 2, 2, 3, 0 };
	// Generate a unit icosphere
	const float cullingTheta = 20.0f; // TODO All patches below this angle beneath the horizon are removed
	const int subdivisionCount = 0; // Just get a general sphere shape, don't need to much accuracy because it will be tessellated at runtime

	// Based on http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html
	createIscosahedron(indices, vertices);
	for (int i = 0; i < subdivisionCount; ++i) {
		subdivideIcosahredon(indices, vertices);
	}

	// Create a quad on the view frustum far plane
}

void Skysphere::createIscosahedron(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::VertexOnlyPosition>& vertices)
{
	auto t = (1.0f + std::sqrt(5.0f)) / 2.0f;

	addIcosahedronVertex(vertices, glm::vec3(-1,  t, 0));
	addIcosahedronVertex(vertices, glm::vec3( 1,  t, 0));
	addIcosahedronVertex(vertices, glm::vec3(-1, -t, 0));
	addIcosahedronVertex(vertices, glm::vec3( 1, -t, 0));

	addIcosahedronVertex(vertices, glm::vec3(0, -1,  t));
	addIcosahedronVertex(vertices, glm::vec3(0,  1,  t));
	addIcosahedronVertex(vertices, glm::vec3(0, -1, -t));
	addIcosahedronVertex(vertices, glm::vec3(0,  1, -t));

	addIcosahedronVertex(vertices, glm::vec3( t, 0, -1));
	addIcosahedronVertex(vertices, glm::vec3( t, 0,  1));
	addIcosahedronVertex(vertices, glm::vec3(-t, 0, -1));
	addIcosahedronVertex(vertices, glm::vec3(-t, 0,  1));

	std::vector<Graphics::IndexType> inds = { 
		0, 11, 5, 0,  5,  1,  0,  1,  7,  0, 7, 10, 0, 10, 11,
		1,  5, 9, 5, 11,  4, 11, 10,  2, 10, 7,  6, 7,  1,  8,
		3,  9, 4, 3,  4,  2,  3,  2,  6,  3, 6,  8, 3,  8,  9,
		4,  9, 5, 2,  4, 11,  6,  2, 10,  8, 6,  7, 9,  8,  1 
	};

	for (int i = 0; i < inds.size(); ++i) {
		indices.push_back(inds[i]);
	}
}

void Skysphere::subdivideIcosahredon(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::VertexOnlyPosition>& vertices)
{
	std::vector<Graphics::IndexType> newIndices;
	std::map<uint64_t, Graphics::IndexType> indexCache;
	for (int i = 0; i < indices.size(); i+=3) {
		int a = icosahedronMiddlePoint(vertices, indexCache, indices[i], indices[i+1]);
		int b = icosahedronMiddlePoint(vertices, indexCache, indices[i+1], indices[i+2]);
		int c = icosahedronMiddlePoint(vertices, indexCache, indices[i+2], indices[i]);

		newIndices.push_back(indices[i]);
		newIndices.push_back(a);
		newIndices.push_back(c);
		newIndices.push_back(indices[i+1]);
		newIndices.push_back(b);
		newIndices.push_back(a);
		newIndices.push_back(indices[i+2]);
		newIndices.push_back(c);
		newIndices.push_back(b);
		newIndices.push_back(a);
		newIndices.push_back(b);
		newIndices.push_back(c);
	}
	indices = newIndices;
}

Graphics::IndexType Skysphere::icosahedronMiddlePoint(std::vector<Graphics::VertexOnlyPosition>& vertices,
	std::map<uint64_t, Graphics::IndexType>& indexCache, Graphics::IndexType p0, Graphics::IndexType p1)
{
	std::map<uint64_t, Graphics::IndexType> middlePointIndexCache;
	bool firstIsSmaller = p0 < p1;
	uint64_t smallerIndex = firstIsSmaller ? p0 : p1;
	uint64_t greaterIndex = firstIsSmaller ? p1 : p0;
	uint64_t key = (smallerIndex << 32) + greaterIndex;
	
	if (indexCache.count(key) > 0) {
		return indexCache[key];
	}
	else {
		glm::vec3 point0 = vertices[p0].pos;
		glm::vec3 point1 = vertices[p1].pos;
		glm::vec3 middle = glm::vec3( (point0.x + point1.x) / 2.0f, (point0.y + point1.y) / 2.0f, (point0.z + point1.z) / 2.0f);
		Graphics::IndexType i = addIcosahedronVertex(vertices, middle);
		indexCache[key] = i;
		return i;
	}
}

Graphics::IndexType Skysphere::addIcosahedronVertex(std::vector<Graphics::VertexOnlyPosition>& vertices, glm::vec3 pos)
{
	auto length = pos.length();
	vertices.emplace_back(pos.x / length, pos.y / length, pos.z / length);
	return vertices.size() - 1;
}
