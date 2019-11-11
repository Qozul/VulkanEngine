#include "ShaderParams.h"

using namespace QZL::Graphics;

const size_t ShaderParams::shaderParamsLUT[] = { 
	sizeof(StaticShaderParams), 
	sizeof(TerrainShaderParams), 
	sizeof(AtmosphereShaderParams), 
	sizeof(ParticleShaderParams), 
	0 // Post process
};
