#include "Camera.h"
#include "../System.h"
#include "../Graphics/GraphicsMaster.h"

using namespace QZL;
using namespace Game;

const float Camera::SPEED = 1.0f;
const float Camera::MOUSE_SENSITIVITY = 2.0f;

Camera::Camera(const GameScriptInitialiser& initialiser)
	: GameScript(initialiser), viewMatrixPtr_(initialiser.system->getMasters().graphicsMaster->getViewMatrixPtr()),
	position_(initialiser.system->getMasters().graphicsMaster->getCamPosPtr()), pitch_(0.0f), yaw_(0.0f)
{
	inputProfile_.enabled = true;
	inputProfile_.profileBindings[{ GLFW_KEY_W }] = std::bind(&Camera::moveForwards, this);
	inputProfile_.profileBindings[{ GLFW_KEY_A }] = std::bind(&Camera::moveLeft, this);
	inputProfile_.profileBindings[{ GLFW_KEY_S }] = std::bind(&Camera::moveBackwards, this);
	inputProfile_.profileBindings[{ GLFW_KEY_D }] = std::bind(&Camera::moveRight, this);
	inputManager_->addProfile("camera", &inputProfile_);
	lookPoint_ = { 0.0f, 0.0f, 0.0f };
}

void Camera::start()
{
}

void Camera::update(float dt)
{
	yaw_ += inputManager_->getRelativeMousePos().x * MOUSE_SENSITIVITY * dt; // movement around y axis
	pitch_ += inputManager_->getRelativeMousePos().y * MOUSE_SENSITIVITY * dt * -1.0f; // movement around x axis
	// clamp pitch to avoid inversion
	pitch_ = glm::clamp(pitch_, 181.0f, 359.0f);
	// Calc point on sphere with these (i.e. the two circle's intersection)
	float phi = glm::radians(yaw_);
	float theta = glm::radians(pitch_);
	float stheta = sin(theta);
	lookPoint_ = glm::vec3(cos(phi) * stheta, cos(theta), sin(phi) * stheta);

	updatePosition();
}

void Camera::moveLeft()
{
	*position_ += glm::cross(glm::vec3(0.0, 1.0, 0.0), lookPoint_) * SPEED * System::deltaTime;
}

void Camera::moveRight()
{
	*position_ -= glm::cross(glm::vec3(0.0, 1.0, 0.0), lookPoint_) * SPEED * System::deltaTime;
}

void Camera::moveForwards()
{
	*position_ += lookPoint_ * SPEED * System::deltaTime;
}

void Camera::moveBackwards()
{
	*position_ -= lookPoint_ * SPEED * System::deltaTime;
}


void Camera::updatePosition()
{
	*viewMatrixPtr_ = glm::lookAt(*position_, lookPoint_ + *position_, { 0.0f, 1.0f, 0.0f });
}
