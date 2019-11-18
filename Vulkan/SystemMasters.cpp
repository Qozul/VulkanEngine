#include "SystemMasters.h"
#include "Graphics/GraphicsMaster.h"

const QZL::Graphics::LogicDevice* QZL::SystemMasters::getLogicDevice() const
{
	return graphicsMaster->getLogicDevice();
}
