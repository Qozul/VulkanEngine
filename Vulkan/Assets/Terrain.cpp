#include "Terrain.h"
#include "../Graphics/TerrainShaderParams.h"

QZL::Assets::Terrain::Terrain()
{
	setGraphicsComponent(Graphics::RendererTypes::TERRAIN, new Graphics::TerrainShaderParams("heightmaps/Windermere", "ground_04"), "terrain", loadFunction);
}

void QZL::Assets::Terrain::loadFunction(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::Vertex>& vertices)
{
}
