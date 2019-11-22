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
			kShadow,
			kWater,
			kStaticDeferred,
			kNone
		};

		enum class RendererFlags : size_t {
			FULLSCREEN = 1,
			INCLUDE_MODEL = 2, 
			DESCRIPTOR_MVP = 4,
			DESCRIPTOR_PARAMS = 8, 
			DESCRIPTOR_MATERIAL = 16,
			INSTANCED = 32,
			NON_INDEXED = 64,
			DYNAMIC = 128,
			CASTS_SHADOWS = 256
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
			RendererFlags::INCLUDE_MODEL | RendererFlags::DESCRIPTOR_MVP | RendererFlags::DESCRIPTOR_PARAMS | RendererFlags::DESCRIPTOR_MATERIAL | RendererFlags::CASTS_SHADOWS,
			RendererFlags::INCLUDE_MODEL | RendererFlags::DESCRIPTOR_MVP | RendererFlags::DESCRIPTOR_PARAMS | RendererFlags::DESCRIPTOR_MATERIAL | RendererFlags::CASTS_SHADOWS,
			RendererFlags::DESCRIPTOR_PARAMS | RendererFlags::FULLSCREEN,
			RendererFlags::INCLUDE_MODEL | RendererFlags::DESCRIPTOR_MVP | RendererFlags::DESCRIPTOR_PARAMS | RendererFlags::DESCRIPTOR_MATERIAL | RendererFlags::NON_INDEXED | RendererFlags::DYNAMIC,
			RendererFlags::DESCRIPTOR_MATERIAL | RendererFlags::FULLSCREEN,
			RendererFlags::FULLSCREEN,
			RendererFlags::INCLUDE_MODEL | RendererFlags::DESCRIPTOR_MVP | RendererFlags::DESCRIPTOR_PARAMS | RendererFlags::DESCRIPTOR_MATERIAL
		};
	}
}
