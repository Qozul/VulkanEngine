#pragma once
#include "Graphics/VkUtil.h"

namespace QZL {
	using InputCallback = std::function<void(void)>;
	using InputCombo = std::vector<int>;
	struct ProfileBinding {
		bool enabled;
		InputCombo keyCombo;
		InputCallback callback;
		// int is delay before press is allowed to be detected again
		const float delay;
		float nextActivationTime;
		ProfileBinding(InputCombo keyCombo, InputCallback callback, const float delay)
			: enabled(true), keyCombo(keyCombo), callback(callback), delay(delay), nextActivationTime(0.0f) {}
	};
	struct InputProfile {
		std::vector<ProfileBinding> profileBindings;
	};

	class InputManager final {
	public:
		InputManager(GLFWwindow* window) 
			: window_(window), time_(0.0f) {
			glfwGetCursorPos(window, &currentMousePos_.x, &currentMousePos_.y);
			previousMousePos_ = currentMousePos_;
		}
		void addProfile(std::string name, InputProfile* profile) {
			profiles_[name] = profile;
		}
		void removeProfile(std::string name) {
			profiles_.erase(name);
		}
		void checkInput(float dt) {
			// Mouse
			previousMousePos_ = currentMousePos_;
			glfwGetCursorPos(window_, &currentMousePos_.x, &currentMousePos_.y);
			time_ += dt;
			// Keyboard
			for (auto& it : profiles_) {
				auto& profile = it.second;
				for (auto& profileBinding : profile->profileBindings) {
					if (profileBinding.enabled || profileBinding.nextActivationTime <= time_) {
						profileBinding.enabled = true;
						bool allKeysPressed = true;
						for (int i : profileBinding.keyCombo) {
							int state = glfwGetKey(window_, i);
							allKeysPressed = allKeysPressed && state == GLFW_PRESS;
						}
						if (allKeysPressed) {
							profileBinding.callback();
							profileBinding.nextActivationTime = time_ + profileBinding.delay;
							profileBinding.enabled = false;
						}
					}
				}
			}
		}
		glm::dvec2 getRelativeMousePos() {
			return currentMousePos_ - previousMousePos_;
		}

	private:
		GLFWwindow* window_;
		std::map<std::string, InputProfile*> profiles_;
		glm::dvec2 currentMousePos_;
		glm::dvec2 previousMousePos_;
		float time_;
	};
}
