/*
Author: Ralph Ridley
Date: 03/07/19

The main system class which controls all subsystems.
*/
#pragma once
#include "../Shared/Utility.h"
#include "SystemMasters.h"

int main(int argc, char** argv);

namespace QZL {
	class System final {
		friend int ::main(int argc, char** argv);
	public:
		System();
		~System();
		void loop();
		const SystemMasters& getMasters() {
			return masters_;
		}
		static float deltaTime;
		static float deltaTimeSeconds;
	private:
		SystemMasters masters_;
		InputManager* inputManager_;
	};
}
