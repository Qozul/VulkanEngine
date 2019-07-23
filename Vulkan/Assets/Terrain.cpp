#include "Terrain.h"
#include "../Graphics/TerrainShaderParams.h"

QZL::Assets::Terrain::Terrain()
{
	Graphics::ShaderParams params;
	params.terrainSP = new Graphics::TerrainShaderParams("heightmaps/Windermere", "ground_04");
	setGraphicsComponent(Graphics::RendererTypes::TERRAIN, params, "terrain", loadFunction);
}

void QZL::Assets::Terrain::loadFunction(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::Vertex>& vertices)
{
}
