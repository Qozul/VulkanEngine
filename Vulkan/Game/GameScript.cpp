#include "GameScript.h"
#include "../Assets/Entity.h"
#include "../System.h"

QZL::Game::GameScript::GameScript(const GameScriptInitialiser& initialiser)
	: owningEntity_(initialiser.owner), inputManager_(initialiser.inputManager), sysMasters_(&initialiser.system->getMasters())
{
}

QZL::Transform* QZL::Game::GameScript::transform()
{
	return owningEntity_->getTransform();
}
