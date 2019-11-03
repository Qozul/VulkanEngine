/// Author: Ralph Ridley
/// Date: 19/02/19
// Reference: https://github.com/syoyo/tinyobjloader code example for loading using tinyobj
#include "MeshLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../../Shared/tiny_obj_loader.h"

using namespace QZL;
using namespace QZL::Graphics;

const std::string MeshLoader::kPath = "../Data/Meshes/";
const std::string MeshLoader::kExt = ".obj";

BasicMesh* MeshLoader::loadMesh(const std::string& meshName, ElementBufferObject& eleBuf, MeshLoadFunc loaderFunc)
{
	ASSERT(!eleBuf.isCommitted());
	if (!eleBuf.containsMesh(meshName)) {
		if (loaderFunc == nullptr) {
			loadMeshFromFile(meshName, eleBuf);
		}
		else {
			uint32_t count;
			std::vector<char> indices;
			std::vector<char> vertices;
			loaderFunc(count, indices, vertices);
			placeMeshInBuffer(meshName, eleBuf, count, indices.data(), vertices.data(), indices.size(), vertices.size());
		}
	}
	return eleBuf.getMesh(meshName);
}

void MeshLoader::placeMeshInBuffer(const std::string& meshName, ElementBufferObject& eleBuf, uint32_t count, 
	void* indices, void* vertices, size_t indicesSize, size_t verticesSize)
{
	auto indexOffset = eleBuf.addIndices(indices, indicesSize);
	auto vertexOffset = eleBuf.addVertices(vertices, verticesSize);
	eleBuf.emplaceMesh(meshName, count, vertexOffset, indexOffset);
}

void MeshLoader::loadMeshFromFile(const std::string& meshName, ElementBufferObject& eleBuf)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	std::string warn;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, (kPath + meshName + kExt).c_str());
	if (!warn.empty())
		std::cout << warn << std::endl;
	if (!err.empty())
		std::cout << err << std::endl;

	std::vector<IndexType> indices;
	std::vector<Vertex> verts;
	// TODO: remove duplicate vertices
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex = {};
			vertex.x = attrib.vertices[3 * index.vertex_index + 0];
			vertex.y = attrib.vertices[3 * index.vertex_index + 1];
			vertex.z = attrib.vertices[3 * index.vertex_index + 2];
			vertex.u = attrib.texcoords[2 * index.texcoord_index + 0];
			vertex.v = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];
			vertex.nx = attrib.normals[3 * index.normal_index + 0];
			vertex.ny = attrib.normals[3 * index.normal_index + 1];
			vertex.nz = attrib.normals[3 * index.normal_index + 2];

			verts.push_back(vertex);
			indices.push_back(indices.size());
		}
	}
	placeMeshInBuffer(meshName, eleBuf, indices.size(), indices.data(), verts.data(), indices.size() * sizeof(IndexType), verts.size() * sizeof(Vertex));
}
