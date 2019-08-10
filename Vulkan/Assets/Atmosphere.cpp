#include "Atmosphere.h"
#include "../Graphics/LogicDevice.h"

using namespace QZL;
using namespace Assets;
using namespace Graphics;

void Atmosphere::precalculateTextures(LogicDevice* logicDevice)
{
	// See https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/model.cc.html
	// fore reference.

	// TODO Create the compute pipelines.

	// TODO Create the textures.

	// TODO Record command buffer and execute on compute queue.

	// 1. Compute Transmittance
	// 2. Compute Direct Irradiance
	// 3. Compute Single Scattering
	// 4. Compute Scatter Density
	// 5. Compute Indirect Irradiance
	// 6. Compute Multiple Scattering
}
