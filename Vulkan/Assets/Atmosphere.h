#pragma once

namespace QZL {
	namespace Assets {
		class Atmosphere {
			friend class Skysphere;
		public: 
			Atmosphere(float radius = 1.0f)
				: radius_(radius) {}

		private:
			float radius_;
		};
	}
}
