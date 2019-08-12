/*
	Atmosphere, along with the renderer pipeline by the same name, are a port of Eric Bruneton and Fabrice Neyret's paper
	https://hal.inria.fr/inria-00288758/en using Bruneton's new implementation https://ebruneton.github.io/precomputed_atmospheric_scattering/.
	This is a port to Vulkan and this engine and so directly uses code (including comments) from Bruneton's implementation with small modifications.
*/
#pragma once
#include "../Graphics/VkUtil.h"

namespace QZL {
	namespace Graphics {
		class LogicDevice;
		class TextureSampler;
		class ComputePipeline;
		class Image;
	}
	namespace Assets {

#define AbstractSpectrum glm::vec3
#define DimensionlessSpectrum glm::vec3
#define PowerSpectrum glm::vec3
#define IrradianceSpectrum glm::vec3
#define RadianceSpectrum glm::vec3
#define RadianceDensitySpectrum glm::vec3
#define ScatteringSpectrum glm::vec3
#define Direction glm::vec3
#define Luminance3 glm::vec3
#define Illuminance3 glm::vec3

		constexpr int TRANSMITTANCE_TEXTURE_WIDTH = 256;
		constexpr int TRANSMITTANCE_TEXTURE_HEIGHT = 64;

		constexpr int SCATTERING_TEXTURE_R_SIZE = 32;
		constexpr int SCATTERING_TEXTURE_MU_SIZE = 128;
		constexpr int SCATTERING_TEXTURE_MU_S_SIZE = 32;
		constexpr int SCATTERING_TEXTURE_NU_SIZE = 8;

		constexpr int SCATTERING_TEXTURE_WIDTH =
			SCATTERING_TEXTURE_NU_SIZE * SCATTERING_TEXTURE_MU_S_SIZE;
		constexpr int SCATTERING_TEXTURE_HEIGHT = SCATTERING_TEXTURE_MU_SIZE;
		constexpr int SCATTERING_TEXTURE_DEPTH = SCATTERING_TEXTURE_R_SIZE;

		constexpr int IRRADIANCE_TEXTURE_WIDTH = 64;
		constexpr int IRRADIANCE_TEXTURE_HEIGHT = 16;

		// The conversion factor between watts and lumens.
		constexpr double MAX_LUMINOUS_EFFICACY = 683.0;

		// Values from "CIE (1931) 2-deg color matching functions", see
		// "http://web.archive.org/web/20081228084047/
		//    http://www.cvrl.org/database/data/cmfs/ciexyz31.txt".
		constexpr double CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[380] = {
		  360, 0.000129900000, 0.000003917000, 0.000606100000,
		  365, 0.000232100000, 0.000006965000, 0.001086000000,
		  370, 0.000414900000, 0.000012390000, 0.001946000000,
		  375, 0.000741600000, 0.000022020000, 0.003486000000,
		  380, 0.001368000000, 0.000039000000, 0.006450001000,
		  385, 0.002236000000, 0.000064000000, 0.010549990000,
		  390, 0.004243000000, 0.000120000000, 0.020050010000,
		  395, 0.007650000000, 0.000217000000, 0.036210000000,
		  400, 0.014310000000, 0.000396000000, 0.067850010000,
		  405, 0.023190000000, 0.000640000000, 0.110200000000,
		  410, 0.043510000000, 0.001210000000, 0.207400000000,
		  415, 0.077630000000, 0.002180000000, 0.371300000000,
		  420, 0.134380000000, 0.004000000000, 0.645600000000,
		  425, 0.214770000000, 0.007300000000, 1.039050100000,
		  430, 0.283900000000, 0.011600000000, 1.385600000000,
		  435, 0.328500000000, 0.016840000000, 1.622960000000,
		  440, 0.348280000000, 0.023000000000, 1.747060000000,
		  445, 0.348060000000, 0.029800000000, 1.782600000000,
		  450, 0.336200000000, 0.038000000000, 1.772110000000,
		  455, 0.318700000000, 0.048000000000, 1.744100000000,
		  460, 0.290800000000, 0.060000000000, 1.669200000000,
		  465, 0.251100000000, 0.073900000000, 1.528100000000,
		  470, 0.195360000000, 0.090980000000, 1.287640000000,
		  475, 0.142100000000, 0.112600000000, 1.041900000000,
		  480, 0.095640000000, 0.139020000000, 0.812950100000,
		  485, 0.057950010000, 0.169300000000, 0.616200000000,
		  490, 0.032010000000, 0.208020000000, 0.465180000000,
		  495, 0.014700000000, 0.258600000000, 0.353300000000,
		  500, 0.004900000000, 0.323000000000, 0.272000000000,
		  505, 0.002400000000, 0.407300000000, 0.212300000000,
		  510, 0.009300000000, 0.503000000000, 0.158200000000,
		  515, 0.029100000000, 0.608200000000, 0.111700000000,
		  520, 0.063270000000, 0.710000000000, 0.078249990000,
		  525, 0.109600000000, 0.793200000000, 0.057250010000,
		  530, 0.165500000000, 0.862000000000, 0.042160000000,
		  535, 0.225749900000, 0.914850100000, 0.029840000000,
		  540, 0.290400000000, 0.954000000000, 0.020300000000,
		  545, 0.359700000000, 0.980300000000, 0.013400000000,
		  550, 0.433449900000, 0.994950100000, 0.008749999000,
		  555, 0.512050100000, 1.000000000000, 0.005749999000,
		  560, 0.594500000000, 0.995000000000, 0.003900000000,
		  565, 0.678400000000, 0.978600000000, 0.002749999000,
		  570, 0.762100000000, 0.952000000000, 0.002100000000,
		  575, 0.842500000000, 0.915400000000, 0.001800000000,
		  580, 0.916300000000, 0.870000000000, 0.001650001000,
		  585, 0.978600000000, 0.816300000000, 0.001400000000,
		  590, 1.026300000000, 0.757000000000, 0.001100000000,
		  595, 1.056700000000, 0.694900000000, 0.001000000000,
		  600, 1.062200000000, 0.631000000000, 0.000800000000,
		  605, 1.045600000000, 0.566800000000, 0.000600000000,
		  610, 1.002600000000, 0.503000000000, 0.000340000000,
		  615, 0.938400000000, 0.441200000000, 0.000240000000,
		  620, 0.854449900000, 0.381000000000, 0.000190000000,
		  625, 0.751400000000, 0.321000000000, 0.000100000000,
		  630, 0.642400000000, 0.265000000000, 0.000049999990,
		  635, 0.541900000000, 0.217000000000, 0.000030000000,
		  640, 0.447900000000, 0.175000000000, 0.000020000000,
		  645, 0.360800000000, 0.138200000000, 0.000010000000,
		  650, 0.283500000000, 0.107000000000, 0.000000000000,
		  655, 0.218700000000, 0.081600000000, 0.000000000000,
		  660, 0.164900000000, 0.061000000000, 0.000000000000,
		  665, 0.121200000000, 0.044580000000, 0.000000000000,
		  670, 0.087400000000, 0.032000000000, 0.000000000000,
		  675, 0.063600000000, 0.023200000000, 0.000000000000,
		  680, 0.046770000000, 0.017000000000, 0.000000000000,
		  685, 0.032900000000, 0.011920000000, 0.000000000000,
		  690, 0.022700000000, 0.008210000000, 0.000000000000,
		  695, 0.015840000000, 0.005723000000, 0.000000000000,
		  700, 0.011359160000, 0.004102000000, 0.000000000000,
		  705, 0.008110916000, 0.002929000000, 0.000000000000,
		  710, 0.005790346000, 0.002091000000, 0.000000000000,
		  715, 0.004109457000, 0.001484000000, 0.000000000000,
		  720, 0.002899327000, 0.001047000000, 0.000000000000,
		  725, 0.002049190000, 0.000740000000, 0.000000000000,
		  730, 0.001439971000, 0.000520000000, 0.000000000000,
		  735, 0.000999949300, 0.000361100000, 0.000000000000,
		  740, 0.000690078600, 0.000249200000, 0.000000000000,
		  745, 0.000476021300, 0.000171900000, 0.000000000000,
		  750, 0.000332301100, 0.000120000000, 0.000000000000,
		  755, 0.000234826100, 0.000084800000, 0.000000000000,
		  760, 0.000166150500, 0.000060000000, 0.000000000000,
		  765, 0.000117413000, 0.000042400000, 0.000000000000,
		  770, 0.000083075270, 0.000030000000, 0.000000000000,
		  775, 0.000058706520, 0.000021200000, 0.000000000000,
		  780, 0.000041509940, 0.000014990000, 0.000000000000,
		  785, 0.000029353260, 0.000010600000, 0.000000000000,
		  790, 0.000020673830, 0.000007465700, 0.000000000000,
		  795, 0.000014559770, 0.000005257800, 0.000000000000,
		  800, 0.000010253980, 0.000003702900, 0.000000000000,
		  805, 0.000007221456, 0.000002607800, 0.000000000000,
		  810, 0.000005085868, 0.000001836600, 0.000000000000,
		  815, 0.000003581652, 0.000001293400, 0.000000000000,
		  820, 0.000002522525, 0.000000910930, 0.000000000000,
		  825, 0.000001776509, 0.000000641530, 0.000000000000,
		  830, 0.000001251141, 0.000000451810, 0.000000000000,
		};

		// The conversion matrix from XYZ to linear sRGB color spaces.
		// Values from https://en.wikipedia.org/wiki/SRGB.
		constexpr double XYZ_TO_SRGB[9] = {
		  +3.2406, -1.5372, -0.4986,
		  -0.9689, +1.8758, +0.0415,
		  +0.0557, -0.2040, +1.0570
		};

		class Atmosphere {
			friend class Skysphere;
		public:
			// An atmosphere layer of width 'width', and whose density is defined as
			//   'exp_term' * exp('exp_scale' * h) + 'linear_term' * h + 'constant_term',
			// clamped to [0,1], and where h is the altitude.
			struct DensityProfileLayer {
				float width;
				float exp_term;
				float exp_scale;
				float linear_term;
				float constant_term;
			};

			// An atmosphere density profile made of several layers on top of each other
			// (from bottom to top). The width of the last layer is ignored, i.e. it always
			// extend to the top atmosphere boundary. The profile values vary between 0
			// (null density) to 1 (maximum density).
			struct DensityProfile {
				DensityProfileLayer layers[2];
			};

			struct AtmosphereParameters {
				// The solar irradiance at the top of the atmosphere.
				IrradianceSpectrum solar_irradiance;
				// The sun's angular radius. Warning: the implementation uses approximations
				// that are valid only if this angle is smaller than 0.1 radians.
				float sun_angular_radius;
				// The distance between the planet center and the bottom of the atmosphere.
				float bottom_radius;
				// The distance between the planet center and the top of the atmosphere.
				float top_radius;
				// The density profile of air molecules, i.e. a function from altitude to
				// dimensionless values between 0 (null density) and 1 (maximum density).
				DensityProfile rayleigh_density;
				// The scattering coefficient of air molecules at the altitude where their
				// density is maximum (usually the bottom of the atmosphere), as a function of
				// wavelength. The scattering coefficient at altitude h is equal to
				// 'rayleigh_scattering' times 'rayleigh_density' at this altitude.
				ScatteringSpectrum rayleigh_scattering;
				// The density profile of aerosols, i.e. a function from altitude to
				// dimensionless values between 0 (null density) and 1 (maximum density).
				DensityProfile mie_density;
				// The scattering coefficient of aerosols at the altitude where their density
				// is maximum (usually the bottom of the atmosphere), as a function of
				// wavelength. The scattering coefficient at altitude h is equal to
				// 'mie_scattering' times 'mie_density' at this altitude.
				ScatteringSpectrum mie_scattering;
				// The extinction coefficient of aerosols at the altitude where their density
				// is maximum (usually the bottom of the atmosphere), as a function of
				// wavelength. The extinction coefficient at altitude h is equal to
				// 'mie_extinction' times 'mie_density' at this altitude.
				ScatteringSpectrum mie_extinction;
				// The asymetry parameter for the Cornette-Shanks phase function for the
				// aerosols.
				float mie_phase_function_g;
				// The density profile of air molecules that absorb light (e.g. ozone), i.e.
				// a function from altitude to dimensionless values between 0 (null density)
				// and 1 (maximum density).
				DensityProfile absorption_density;

				// Inserted padding to make 16-byte aligned
				float padding0, padding1;
				// The extinction coefficient of molecules that absorb light (e.g. ozone) at
				// the altitude where their density is maximum, as a function of wavelength.
				// The extinction coefficient at altitude h is equal to
				// 'absorption_extinction' times 'absorption_density' at this altitude.
				ScatteringSpectrum absorption_extinction;

				float padding2;
				// The average albedo of the ground.
				DimensionlessSpectrum ground_albedo;
				// The cosine of the maximum Sun zenith angle for which atmospheric scattering
				// must be precomputed (for maximum precision, use the smallest Sun zenith
				// angle yielding negligible sky light radiance values. For instance, for the
				// Earth case, 102 degrees is a good choice - yielding mu_s_min = -0.2).
				float mu_s_min;
			};
			struct PrecomputedTextures {
				Graphics::Image* transmittanceImage = nullptr;
				Graphics::Image* scatteringImage = nullptr;
				Graphics::Image* irradianceImage = nullptr;
				Graphics::TextureSampler* transmittance = nullptr;
				Graphics::TextureSampler* scattering = nullptr;
				Graphics::TextureSampler* irradiance = nullptr;
			};

			struct TempPrecomputationTextures {
				Graphics::Image* deltaIrradianceImage = nullptr;
				Graphics::TextureSampler* deltaIrradianceTexture = nullptr;

				Graphics::Image* deltaRayleighScatteringImage = nullptr;
				Graphics::TextureSampler* deltaRayleighScatteringTexture = nullptr;

				Graphics::Image* deltaMieScatteringImage = nullptr;
				Graphics::TextureSampler* deltaMieScatteringTexture = nullptr;

				Graphics::Image* deltaScatteringDensityImage = nullptr;
				Graphics::TextureSampler* deltaScatteringDensityTexture = nullptr;

				Graphics::Image* deltaMultipleScatteringImage = nullptr;
				Graphics::TextureSampler* deltaMultipleScatteringTexture = nullptr;
			};

		public:
			Atmosphere(float radius) : radius_(radius) {}
			Atmosphere(AtmosphereParameters params, float radius = 1.0f)
				: parameters_(params), radius_(radius) {}
			~Atmosphere();

			void precalculateTextures(const Graphics::LogicDevice* logicDevice);
			PrecomputedTextures& getTextures() {
				return textures_;
			}
		private:
			// Creates temporary textures, returned via reference argument. Also creates the member textures.
			void initTextures(const Graphics::LogicDevice* logicDevice, TempPrecomputationTextures& tempTextures, PrecomputedTextures& finalTextures);
			VkDescriptorSetLayoutBinding makeLayoutBinding(const uint32_t binding, const VkSampler* immutableSamplers = nullptr);
			void convertSpectrumToLinearSrgb(const std::vector<double>& wavelengths, const std::vector<double>& spectrum,
				double& r, double& g, double& b);
			double cieColorMatchingFunctionTableValue(double wavelength, int column);
			double interpolate(const std::vector<double>& wavelengths, const std::vector<double>& wavelength_function, double wavelength);
			void computeSpectralRadianceToLuminanceFactors( const std::vector<double>& wavelengths, const std::vector<double>& solar_irradiance,
				double lambda_power, double& k_r, double& k_g, double& k_b);

			float radius_;
			AtmosphereParameters parameters_;
			PrecomputedTextures textures_;

			static constexpr int kLambdaMin = 360;
			static constexpr int kLambdaMax = 830;
			static constexpr double kLambdaR = 680.0;
			static constexpr double kLambdaG = 550.0;
			static constexpr double kLambdaB = 440.0;
		};
	}
}
