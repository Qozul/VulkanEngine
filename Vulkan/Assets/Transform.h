/// Author: Ralph Ridley
/// Date: 15/01/19
/// Purpose: Provide basic operations for transformations
#pragma once
#include "../../Shared/Utility.h"

namespace QZL
{
	/// Each transform simply has a position, a rotation using axis-angle, and a scale.
	/// Storing only these rather than a matrix reduces memory footprint per transform.
	class Transform {
	public:
		Transform()
			: position({ 0.0f, 0.0f, 0.0f }), rotation({ 0.0f, 1.0f, 0.0f }),
			angle(0.0f), scale({ 1.0f, 1.0f, 1.0f }) {}
		Transform(glm::vec3& position, glm::vec3& rotation, float angle, glm::vec3& scale)
			: position(position), rotation(rotation), angle(angle), scale(scale) {}

		void setScale(const float newScale) { 
			scale.x = newScale;
			scale.y = newScale;
			scale.z = newScale;
		};

		glm::mat4 toModelMatrix() {
			glm::mat4 model(1.0f);
			model = glm::translate(model, position);
			model = glm::rotate(model, angle, rotation);
			model = glm::scale(model, scale);
			return model; 
		}

		float* data() {
			return &rotation.x;
		}

		glm::vec3 rotation; // axis-angle
		glm::vec3 position;
		glm::vec3 scale;
		float angle;
	};
}
