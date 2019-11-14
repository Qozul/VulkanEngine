// Author: Ralph Ridley
// Date: 12/10/19
// Define each material type for the renderers

#pragma once
#include "VkUtil.h"
#include "GraphicsTypes.h"

namespace QZL {
	namespace Graphics {
		class Descriptor;
		class TextureManager;
		class TextureSampler;
		class Image;
		
		// A material is a group of textures with an associated descriptor set.
		class Material {
		public:
			static const size_t materialSizeLUT[(size_t)RendererTypes::kNone];

			Material(const std::string materialFileName = "")
				: materialFileName_(materialFileName), textureSet_(VK_NULL_HANDLE), layout_(VK_NULL_HANDLE)
			{
			}

			// This ctor is for when a material is created by the program (such as a game script). Note that care is taken with memory management
			// and that by calling this ctor, the object and its samplers are *probably* not managed by the texture manager and must be deleted manually.
			Material(const std::string name, VkDescriptorSet& set, VkDescriptorSetLayout& layout)
				: materialFileName_(name), textureSet_(set), layout_(layout) { }

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
			AtmosphereMaterial(const std::string name, VkDescriptorSet& set, VkDescriptorSetLayout& layout)
				: Material(name, set, layout), scatteringTexture_(nullptr), transmittanceTexture_(nullptr) { }

			~AtmosphereMaterial() { }

			static VkDescriptorSetLayout getLayout(Descriptor* descriptor);

			const RendererTypes getRendererType() const override {
				return RendererTypes::kAtmosphere;
			}
		protected:
			std::vector<TextureSampler*> loadTextures(TextureManager* textureLoader, std::vector<std::string>& lines) override { return { }; };
			VkDescriptorSetLayout makeLayout(Descriptor* descriptor) override;
		private:
			TextureSampler* scatteringTexture_;
			TextureSampler* transmittanceTexture_;
		};
	}
}
