#include "Camera.h"
#include "../System.h"
#include "../Graphics/GraphicsMaster.h"

using namespace QZL;
using namespace Game;

Camera::Camera(const GameScriptInitialiser& initialiser)
	: GameScript(initialiser), viewMatrixPtr_(initialiser.system->getMasters().graphicsMaster->getViewMatrixPtr())
{
}

void Camera::start()
{
}

void Camera::update()
{
	
}
