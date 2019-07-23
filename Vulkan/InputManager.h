#pragma once
#include "Graphics/VkUtil.h"

namespace QZL {
	using InputCallback = void(*)();
	using InputCombo = std::vector<int>;
	struct InputProfile {
		bool enabled;
		std::map<InputCombo, InputCallback> profileBindings;
	};

	class InputManager final {
	public:
		InputManager(GLFWwindow* window) : window_(window) {}
		void addProfile(std::string name, InputProfile profile) {
			profiles_[name] = profile;
		}
		void checkInput() {
			for (auto& it : profiles_) {
				auto& profile = it.second;
				if (profile.enabled) {
					for (auto& it2 : profile.profileBindings) {
						bool allKeysPressed = true;
						for (int i : it2.first) {
							int state = glfwGetKey(window_, i);
							allKeysPressed = allKeysPressed && state == GLFW_PRESS;
						}
						if (allKeysPressed) {
							(*it2.second)();
						}
					}
				}
			}
		}

	private:
		GLFWwindow* window_;
		std::map<std::string, InputProfile> profiles_;
	};
}
