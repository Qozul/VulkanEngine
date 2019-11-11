#include "Camera.h"
#include "../System.h"
#include "../Graphics/GraphicsMaster.h"

using namespace QZL;
using namespace Game;

const float Camera::SPEED = 20.0f;
const float Camera::MOUSE_SENSITIVITY = 2000.0f;
const float Camera::MAX_ROTATION_DT = 5e-4f;

QZL::Game::Camera::Camera(const SystemMasters& initialiser)
	: GameScript(initialiser), mainCamera_(sysMasters_->graphicsMaster->getMainCameraPtr()), pitch_(0.0f), yaw_(0.0f), speed_(SPEED)
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
	mainCamera_->lookPoint = { 0.0f, 0.0f, 0.0f };
}

Camera::~Camera()
{
	inputManager_->removeProfile("camera");
}

void Camera::start()
{
	mainCamera_->position = glm::vec3(136.6f, 83.3f, 20.1f);
}

void Camera::update(float dt, const glm::mat4& parentMatrix)
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
	mainCamera_->lookPoint = glm::vec3(cos(phi) * stheta, cos(theta), sin(phi) * stheta);

	updatePosition();
}

void Camera::moveLeft()
{
	mainCamera_->position += glm::cross(glm::vec3(0.0, 1.0, 0.0), mainCamera_->lookPoint) * speed_ * System::deltaTimeSeconds;
}

void Camera::moveRight()
{
	mainCamera_->position -= glm::cross(glm::vec3(0.0, 1.0, 0.0), mainCamera_->lookPoint) * speed_ * System::deltaTimeSeconds;
}

void Camera::moveForwards()
{
	mainCamera_->position += mainCamera_->lookPoint * speed_ * System::deltaTimeSeconds;
}

void Camera::moveBackwards()
{
	mainCamera_->position -= mainCamera_->lookPoint * speed_ * System::deltaTimeSeconds;
}

void Camera::moveUp()
{
	mainCamera_->position.y += speed_ * System::deltaTimeSeconds;
}

void Camera::moveDown()
{
	mainCamera_->position.y -= speed_ * System::deltaTimeSeconds;
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
	transform()->position = mainCamera_->position;
	mainCamera_->viewMatrix = glm::lookAt(mainCamera_->position, mainCamera_->lookPoint + mainCamera_->position, { 0.0f, 1.0f, 0.0f });
}

void Camera::logPosition()
{
	DEBUG_LOG(vecToString(mainCamera_->position));
}
