#include "SystemMasters.h"
#include "Assets/AssetManager.h"
#include "Graphics/GraphicsMaster.h"

const QZL::Graphics::LogicDevice* QZL::SystemMasters::getLogicDevice() const
{
	return graphicsMaster->getLogicDevice();
}

QZL::Graphics::TextureManager* QZL::SystemMasters::getTextureManager() const
{
	return assetManager->textureManager;
}
