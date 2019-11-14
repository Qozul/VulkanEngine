// Author: Ralph Ridley
// Date: 12/10/19
// Define each material type for the renderers

#pragma once
#include "VkUtil.h"
#include "GraphicsTypes.h"

namespace QZL {
	namespace Graphics {
		class TextureManager;

		enum class MaterialType {
			kStatic, kTerrain, kAtmosphere, kParticle, kSize
		};
		
		struct Material {
			void* data;
			size_t size;
		};

		class Materials {
			using MaterialLoadingFunction = void(*)(TextureManager* texManager, void* data, std::vector<std::string>& lines);
		public:
			struct Static {
				uint32_t albedoIdx;
				uint32_t normalmapIdx;
			};

			struct Terrain {
				uint32_t heightmapIdx;
				uint32_t normalmapIdx;
				uint32_t albedoIdx;
			};

			struct Atmosphere {
				uint32_t scatteringIdx;
			};

			struct Particle {
				uint32_t albedoIdx;
			};
			static void loadMaterial(TextureManager* texManager, MaterialType type, std::string fileName, void* data);
			static MaterialType stringToType(std::string typeName);

			static const size_t materialTextureCountLUT[(size_t)MaterialType::kSize];
			static const size_t materialSizeLUT[(size_t)MaterialType::kSize];

		private:
			static MaterialLoadingFunction getLoadingFunction(MaterialType type);
			static void loadStaticMaterial(TextureManager* texManager, void* data, std::vector<std::string>& lines);
			static void loadTerrainMaterial(TextureManager* texManager, void* data, std::vector<std::string>& lines);
			static void loadParticleMaterial(TextureManager* texManager, void* data, std::vector<std::string>& lines);
		};
		/*
		// A material is a group of textures with an associated descriptor set.
		class Material {
		public:
			static const size_t materialSizeLUT[(size_t)RendererTypes::kNone];

			Material(const std::string materialFileName = "")
				: materialFileName_(materialFileName), textureSet_(VK_NULL_HANDLE), layout_(VK_NULL_HANDLE)
			{
			}

			virtual ~Material() {}

			VkDescriptorSet getTextureSet() const {
				return textureSet_;
			}

			const std::string& getName() const {
				return materialFileName_;
			}

			void load(TextureManager* textureManager, Descriptor* descriptor);

			virtual const RendererTypes getRendererType() const = 0;
		protected:
			void readFile(std::vector<std::string>& lines);

			void makeTextureSet(Descriptor* descriptor, std::vector<TextureSampler*> samplers);

			constexpr static VkDescriptorSetLayoutBinding makeLayoutBinding(uint32_t idx, VkShaderStageFlags stageFlags, VkSampler* sampler, 
				VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			virtual std::vector<TextureSampler*> loadTextures(TextureManager* textureManager, std::vector<std::string>& lines) = 0;
			virtual VkDescriptorSetLayout makeLayout(Descriptor* descriptor) = 0;

			VkDescriptorSet textureSet_;
			VkDescriptorSetLayout layout_;
			const std::string materialFileName_;
		};

		class ParticleMaterial : public Material {
			friend class ParticleRenderer;
		public:
			ParticleMaterial(const std::string materialFileName)
				: Material(materialFileName) { }
			~ParticleMaterial();

			static VkDescriptorSetLayout getLayout(Descriptor* descriptor);

			const RendererTypes getRendererType() const override {
				return RendererTypes::kParticle;
			}

		protected:
			std::vector<TextureSampler*> loadTextures(TextureManager* textureManager, std::vector<std::string>& lines) override;
			VkDescriptorSetLayout makeLayout(Descriptor* descriptor) override;
		private:
			uint32_t diffuse_;
		};

		class StaticMaterial : public Material {
			friend class TexturedRenderer;
		public:
			StaticMaterial(const std::string materialFileName)
				: Material(materialFileName) { }

			~StaticMaterial();

			static VkDescriptorSetLayout getLayout(Descriptor* descriptor);

			const RendererTypes getRendererType() const override {
				return RendererTypes::kStatic;
			}
		protected:
			std::vector<TextureSampler*> loadTextures(TextureManager* textureManager, std::vector<std::string>& lines) override;
			VkDescriptorSetLayout makeLayout(Descriptor* descriptor) override;
		private:
			uint32_t diffuseTextureIndex;
			uint32_t normalMapIndex;
		};

		class TerrainMaterial : public Material {
			friend class TerrainRenderer;
		public:
			TerrainMaterial(const std::string materialFileName)
				: Material(materialFileName) { }

			~TerrainMaterial();

			static VkDescriptorSetLayout getLayout(Descriptor* descriptor);

			const RendererTypes getRendererType() const override {
				return RendererTypes::kTerrain;
			}
		protected:
			std::vector<TextureSampler*> loadTextures(TextureManager* textureManager, std::vector<std::string>& lines) override;
			VkDescriptorSetLayout makeLayout(Descriptor* descriptor) override;
		private:
			uint32_t heightmap_;
			uint32_t diffuse_;
			uint32_t normalmap_;
		};

		class AtmosphereMaterial : public Material {
		public:
			AtmosphereMaterial(const std::string name, uint32_t idx)
				: Material(name), scatteringTexture_(idx) { }

			~AtmosphereMaterial() { }

			static VkDescriptorSetLayout getLayout(Descriptor* descriptor);

			const RendererTypes getRendererType() const override {
				return RendererTypes::kAtmosphere;
			}
			uint32_t scatteringTexture_;
		protected:
			std::vector<TextureSampler*> loadTextures(TextureManager* textureLoader, std::vector<std::string>& lines) override { return { }; };
			VkDescriptorSetLayout makeLayout(Descriptor* descriptor) override;
		};
		*/

	}
}
