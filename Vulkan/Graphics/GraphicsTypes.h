// Author: Ralph Ridley
// Date: 01/11/19
#pragma once

namespace QZL {
	namespace Graphics {
		enum class RendererTypes : size_t {
			kStatic,
			kTerrain,
			kAtmosphere,
			kParticle,
			kPostProcess,
			kNone
		};

		enum class RendererFlags : size_t {
			FULLSCREEN = 1, 
			INCLUDE_MODEL = 2, 
			DESCRIPTOR_MVP = 4,
			DESCRIPTOR_PARAMS = 8, 
			DESCRIPTOR_MATERIAL = 16
		};
		
		inline constexpr RendererFlags operator|(RendererFlags a, RendererFlags b)
		{
			return static_cast<RendererFlags>(static_cast<size_t>(a) | static_cast<size_t>(b));
		}

		inline constexpr bool operator&(RendererFlags a, RendererFlags b)
		{
			return static_cast<size_t>(a) & static_cast<size_t>(b);
		}

		enum class RenderPassTypes : size_t {
			kComputePrePass,
			kGeometry,
			kPostProcess
		};

		constexpr RendererFlags kRendererTypeFlags[(size_t)RendererTypes::kNone] = { 
			RendererFlags::INCLUDE_MODEL | RendererFlags::DESCRIPTOR_MVP | RendererFlags::DESCRIPTOR_PARAMS | RendererFlags::DESCRIPTOR_MATERIAL,
			RendererFlags::INCLUDE_MODEL | RendererFlags::DESCRIPTOR_MVP | RendererFlags::DESCRIPTOR_PARAMS | RendererFlags::DESCRIPTOR_MATERIAL,
			RendererFlags::DESCRIPTOR_PARAMS,
			RendererFlags::INCLUDE_MODEL | RendererFlags::DESCRIPTOR_MVP | RendererFlags::DESCRIPTOR_PARAMS | RendererFlags::DESCRIPTOR_MATERIAL,
			RendererFlags::DESCRIPTOR_MATERIAL
		};
	}
}
