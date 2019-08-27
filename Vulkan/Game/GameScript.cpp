#include "GameScript.h"
#include "../Assets/Entity.h"

QZL::Transform* QZL::Game::GameScript::transform()
{
	return owningEntity_->getTransform();
}
