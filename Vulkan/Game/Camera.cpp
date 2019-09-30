#include "Camera.h"
#include "../System.h"
#include "../Graphics/GraphicsMaster.h"

using namespace QZL;
using namespace Game;

const float Camera::SPEED = 20.0f;
const float Camera::MOUSE_SENSITIVITY = 2000.0f;
const float Camera::MAX_ROTATION_DT = 5e-4f;

Camera::Camera(const GameScriptInitialiser& initialiser)
	: GameScript(initialiser), viewMatrixPtr_(initialiser.system->getMasters().graphicsMaster->getViewMatrixPtr()),
	position_(initialiser.system->getMasters().graphicsMaster->getCamPosPtr()), pitch_(0.0f), yaw_(0.0f), speed_(SPEED)
{
	inputProfile_.profileBindings.push_back({ { GLFW_KEY_W }, std::bind(&Camera::moveForwards, this), 0.0f });
	inputProfile_.profileBindings.push_back({ { GLFW_KEY_A }, std::bind(&Camera::moveLeft, this), 0.0f });
	inputProfile_.profileBindings.push_back({ { GLFW_KEY_S }, std::bind(&Camera::moveBackwards, this), 0.0f });
	inputProfile_.profileBindings.push_back({ { GLFW_KEY_D }, std::bind(&Camera::moveRight, this), 0.0f });
	inputProfile_.profileBindings.push_back({ { GLFW_KEY_Q }, std::bind(&Camera::moveUp, this), 0.0f });
	inputProfile_.profileBindings.push_back({ { GLFW_KEY_E }, std::bind(&Camera::moveDown, this), 0.0f });
	inputProfile_.profileBindings.push_back({ { GLFW_KEY_X }, std::bind(&Camera::increaseSpeed, this), 0.2f });
	inputProfile_.profileBindings.push_back({ { GLFW_KEY_Z }, std::bind(&Camera::decreaseSpeed, this), 0.2f });
	inputProfile_.profileBindings.push_back({ { GLFW_KEY_L }, std::bind(&Camera::logPosition, this), 0.2f });
	inputManager_->addProfile("camera", &inputProfile_);
	lookPoint_ = { 0.0f, 0.0f, 0.0f };
}

Camera::~Camera()
{
	inputManager_->removeProfile("camera");
}

void Camera::start()
{
	*position_ = glm::vec3(136.6f, 83.3f, 20.1f);
}

void Camera::update(float dt)
{
	dt = glm::min(dt, MAX_ROTATION_DT);
	yaw_ += static_cast<float>(inputManager_->getRelativeMousePos().x) * MOUSE_SENSITIVITY * dt; // movement around y axis
	pitch_ += static_cast<float>(inputManager_->getRelativeMousePos().y) * MOUSE_SENSITIVITY * dt * -1.0f; // movement around x axis
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
	*position_ += glm::cross(glm::vec3(0.0, 1.0, 0.0), lookPoint_) * speed_ * System::deltaTimeSeconds;
}

void Camera::moveRight()
{
	*position_ -= glm::cross(glm::vec3(0.0, 1.0, 0.0), lookPoint_) * speed_ * System::deltaTimeSeconds;
}

void Camera::moveForwards()
{
	*position_ += lookPoint_ * speed_ * System::deltaTimeSeconds;
}

void Camera::moveBackwards()
{
	*position_ -= lookPoint_ * speed_ * System::deltaTimeSeconds;
}

void Camera::moveUp()
{
	position_->y += speed_ * System::deltaTimeSeconds;
}

void Camera::moveDown()
{
	position_->y -= speed_ * System::deltaTimeSeconds;
}

void Camera::increaseSpeed()
{
	speed_ += 1.0f;
	speed_ = glm::clamp(speed_, (float)MIN_SPEED, (float)MAX_SPEED);
}

void Camera::decreaseSpeed()
{
	speed_ -= 1.0f;
	speed_ = glm::clamp(speed_, (float)MIN_SPEED, (float)MAX_SPEED);
}


void Camera::updatePosition()
{
	transform()->position = *position_;
	*viewMatrixPtr_ = glm::lookAt(*position_, lookPoint_ + *position_, { 0.0f, 1.0f, 0.0f });
}

void Camera::logPosition()
{
	DEBUG_LOG(vecToString(*position_));
}
