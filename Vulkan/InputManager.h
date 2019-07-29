#pragma once
#include "Graphics/VkUtil.h"

namespace QZL {
	using InputCallback = std::function<void(void)>;
	using InputCombo = std::vector<int>;
	struct InputProfile {
		bool enabled;
		std::map<InputCombo, InputCallback> profileBindings;
	};

	class InputManager final {
	public:
		InputManager(GLFWwindow* window) 
			: window_(window) {
			glfwGetCursorPos(window, &currentMousePos_.x, &currentMousePos_.y);
			previousMousePos_ = currentMousePos_;
		}
		void addProfile(std::string name, InputProfile* profile) {
			profiles_[name] = profile;
		}
		void checkInput() {
			// Mouse
			previousMousePos_ = currentMousePos_;
			glfwGetCursorPos(window_, &currentMousePos_.x, &currentMousePos_.y);

			// Keyboard
			for (auto& it : profiles_) {
				auto& profile = it.second;
				if (profile->enabled) {
					for (auto& it2 : profile->profileBindings) {
						bool allKeysPressed = true;
						for (int i : it2.first) {
							int state = glfwGetKey(window_, i);
							allKeysPressed = allKeysPressed && state == GLFW_PRESS;
						}
						if (allKeysPressed) {
							it2.second();
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
	};
}
