#include "Camera.h"
#include "../System.h"
#include "../Graphics/GraphicsMaster.h"

using namespace QZL;
using namespace Game;

const float Camera::SPEED = 20.0f;
const float Camera::MOUSE_SENSITIVITY = 1000.0f;
const float Camera::MAX_ROTATION_DT = 5e-4f;

QZL::Game::Camera::Camera(const SystemMasters& initialiser)
	: GameScript(initialiser), mainCamera_(sysMasters_->graphicsMaster->getCamera(0)), pitch_(0.0f), yaw_(0.0f), speed_(SPEED), isOnTrack_(true), currentNode_(0), trackAmount_(0.0f)
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
	inputProfile_.profileBindings.push_back({ { GLFW_KEY_T }, [this]() { 
		isOnTrack_ = !isOnTrack_;
		currentNode_ = 0;
		trackAmount_ = 0.0f;
	}, 0.2f });
	inputManager_->addProfile("camera", &inputProfile_);
	mainCamera_->lookPoint = { 0.0f, 0.0f, 0.0f };

	trackNodes_ = std::vector<TrackNode>({ 
		{{97, 167, 9}, 232.5f, -20}, {{97, 167, 9}, 290, 88.5f}, {{138, 174, 22}, 263.5f, 243},  {{441, 112, 91}, 257.5f, -143.5f},
		{{618, 101, 229}, 264.0f, -142.0f}, {{708, 109, 346}, 291.0f, -126.5f}, {{796, 114, 403}, 256.5f, -258.5},
		{{770, 117, 413}, 263, -74}, {{552, 116, 135}, 242, 11},
		{{192, 200, 122}, 253, -137}, {{192, 200, 122}, 253, -137}
	});
	maxNode_ = int(trackNodes_.size()) - 1;
}

Camera::~Camera()
{
	inputManager_->removeProfile("camera");
}

void Camera::start()
{
}

void Camera::update(float dt, const glm::mat4& viewProjection, const glm::mat4& parentMatrix)
{
	if (isOnTrack_) {
		trackAmount_ += dt * 0.2f;
		mainCamera_->position = glm::mix(trackNodes_[currentNode_].position, trackNodes_[currentNode_ + 1].position, glm::clamp(trackAmount_, 0.0f, 1.0f));
		float phi = glm::radians(glm::mix(trackNodes_[currentNode_].yaw, trackNodes_[currentNode_ + 1].yaw, glm::clamp(trackAmount_, 0.0f, 1.0f)));
		float theta = glm::radians(glm::mix(trackNodes_[currentNode_].pitch, trackNodes_[currentNode_ + 1].pitch, glm::clamp(trackAmount_, 0.0f, 1.0f)));
		float stheta = sin(theta);
		mainCamera_->lookPoint = glm::vec3(cos(phi) * stheta, cos(theta), sin(phi) * stheta);
		if (trackAmount_ >= 1.2f) {
			++currentNode_;
			trackAmount_ = 0.0f;
		}
		if (currentNode_ == maxNode_) {
			isOnTrack_ = false;
			currentNode_ = 0;
			pitch_ = trackNodes_[currentNode_].pitch;
			yaw_ = trackNodes_[currentNode_].yaw;
		}
	}
	else {
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
	}
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
	DEBUG_LOG(vecToString(mainCamera_->position) << " Pitch: " << pitch_ << " Yaw: " << yaw_);
}
