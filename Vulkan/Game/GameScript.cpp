#include "GameScript.h"
#include "../Assets/Entity.h"
#include "../System.h"

QZL::Game::GameScript::GameScript(const GameScriptInitialiser& initialiser)
	: owningEntity_(initialiser.owner), inputManager_(initialiser.inputManager), sysMasters_(&initialiser.system->getMasters())
{
}

QZL::Game::GameScript::GameScript(const SystemMasters& initialiser)
	: sysMasters_(&initialiser), inputManager_(initialiser.inputManager)
{
}

QZL::Transform* QZL::Game::GameScript::transform()
{
	return owningEntity_->getTransform();
}
