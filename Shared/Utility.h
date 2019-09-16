/// This file can be included everywhere and defines universal includes, definitions, and functions
/// Author: Ralph Ridley
/// Date: 11/10/18
#pragma once

#include <iostream>
#include <stdexcept>

#include <set>
#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include <tuple>

#include <string>
#include <array>
#include <optional>
#include <functional>
#include <algorithm>
#include <random>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_CTOR_INIT
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "PerfMeasurer.h"

using uint = uint64_t;

// For calling delete, ensures it is not erroneous to do so
#define SAFE_DELETE(b) if (b != nullptr) { delete b; b = nullptr; }
// To mimic the idea of c# out keyword, but with no enforced rule
#define OUT 
#define ASSERT(post) if (!(post)) throw std::runtime_error("Assertion failed.")

// Debug/Release related code
#ifdef _DEBUG
#define DEBUG_LOG(m) std::cout << m << std::endl
#define DEBUG_ERR(m) std::cout << "Error in " << __FILE__ << " at line " << __LINE__ << " " << m << std::endl
#define EXPECTS(pre) if (!(pre)) throw std::runtime_error("Precondition failed.")
#define ASSERT_DEBUG(post) ASSERT(post);
#else
#define DEBUG_LOG(m)
#define DEBUG_ERR(m)
#define ASSERT_DEBUG(post) post;
#define EXPECTS(pre) pre;
#endif

namespace QZL
{
	namespace Shared
	{
		constexpr int kDefaultWidth = 800;
		constexpr int kDefaultHeight = 600;

		constexpr float kFoV = 45.0f;

		static std::vector<std::string> kMeshNames = { "Teapot", "Teapot2", "Teapot3" };
		static std::vector<std::pair<std::string, std::string>> kTextureNames = { 
			{ "101", "102" }, { "grass_01b", "grass_01_rough" }, { "ground_04", "ground_04_nrm" },
			{ "rock_05_col", "rock_05_roughness" }, { "T_crate1_D", "T_crate1_S" }
		};
		static std::random_device kRandDevice;
		static std::mt19937 kRng(kRandDevice());
		static std::uniform_int_distribution<size_t> kMeshDist(0, kMeshNames.size() - 1);
		static std::uniform_int_distribution<size_t> kTextDist(0, kTextureNames.size() - 1);

		inline void checkGLError() {
			GLenum err = glGetError();
			if (err != GL_NO_ERROR)
				DEBUG_LOG("OpenGL Error " << std::hex << "0x" << err);
		}
	}
}
