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
		
		// A material is a group of textures with an associated descriptor set.
		class Material {
		public:
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

			constexpr static VkDescriptorSetLayoutBinding makeLayoutBinding(uint32_t idx, VkShaderStageFlags stageFlags, VkSampler* sampler);

			virtual std::vector<TextureSampler*> loadTextures(TextureManager* textureManager, std::vector<std::string>& lines) = 0;
			virtual VkDescriptorSetLayout makeLayout(Descriptor* descriptor) = 0;

			VkDescriptorSet textureSet_;
			VkDescriptorSetLayout layout_;
			const std::string materialFileName_;
		};

		class ParticleMaterial : public Material {
		public:
			ParticleMaterial(const std::string materialFileName)
				: Material(materialFileName), diffuse_(nullptr) { }
			~ParticleMaterial();

			static VkDescriptorSetLayout getLayout(Descriptor* descriptor);

			const RendererTypes getRendererType() const override {
				return RendererTypes::PARTICLE;
			}

		protected:
			std::vector<TextureSampler*> loadTextures(TextureManager* textureManager, std::vector<std::string>& lines) override;
			VkDescriptorSetLayout makeLayout(Descriptor* descriptor) override;
		private:
			TextureSampler* diffuse_;
		};

		class StaticMaterial : public Material {
			friend class TexturedRenderer;
		public:
			StaticMaterial(const std::string materialFileName)
				: Material(materialFileName), isUsingDI(false) {
				diffuse_.diffuseSampler = nullptr;
				normalMap_.normalMapSampler = nullptr;
			}

			~StaticMaterial();

			static VkDescriptorSetLayout getLayout(Descriptor* descriptor);

			const RendererTypes getRendererType() const override {
				return RendererTypes::STATIC;
			}
		protected:
			std::vector<TextureSampler*> loadTextures(TextureManager* textureManager, std::vector<std::string>& lines) override;
			VkDescriptorSetLayout makeLayout(Descriptor* descriptor) override;
		private:
			union {
				uint32_t diffuseTextureIndex;
				TextureSampler* diffuseSampler;
			} diffuse_;
			union {
				uint32_t normalMapIndex;
				TextureSampler* normalMapSampler;
			} normalMap_;
			bool isUsingDI;
		};

		class TerrainMaterial : public Material {
		public:
			TerrainMaterial(const std::string materialFileName)
				: Material(materialFileName), heightmap_(nullptr), diffuse_(nullptr) { }

			~TerrainMaterial();

			static VkDescriptorSetLayout getLayout(Descriptor* descriptor);

			const RendererTypes getRendererType() const override {
				return RendererTypes::TERRAIN;
			}
		protected:
			std::vector<TextureSampler*> loadTextures(TextureManager* textureManager, std::vector<std::string>& lines) override;
			VkDescriptorSetLayout makeLayout(Descriptor* descriptor) override;
		private:
			TextureSampler* heightmap_;
			TextureSampler* diffuse_;
		};

		class AtmosphereMaterial : public Material {
		public:
			AtmosphereMaterial(const std::string materialFileName)
				: Material(materialFileName), scatteringTexture_(nullptr) { }

			AtmosphereMaterial(const std::string name, VkDescriptorSet& set, VkDescriptorSetLayout& layout)
				: Material(name, set, layout), scatteringTexture_(nullptr) { }

			~AtmosphereMaterial() { }

			static VkDescriptorSetLayout getLayout(Descriptor* descriptor);

			const RendererTypes getRendererType() const override {
				return RendererTypes::ATMOSPHERE;
			}
		protected:
			std::vector<TextureSampler*> loadTextures(TextureManager* textureLoader, std::vector<std::string>& lines) override;
			VkDescriptorSetLayout makeLayout(Descriptor* descriptor) override;
		private:
			TextureSampler* scatteringTexture_;
		};
	}
}
