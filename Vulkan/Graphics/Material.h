// Author: Ralph Ridley
// Date: 12/10/19
// Define each material type for the renderers

#pragma once
#include "VkUtil.h"
#include "Descriptor.h"
#include "GraphicsTypes.h"
#include "TextureSampler.h"
#include <fstream>
#include <sstream>

namespace QZL {
	namespace Graphics {
		class TextureManager;
		
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

			void load(TextureManager* textureManager, Descriptor* descriptor) {
				std::vector<std::string> lines;
				readFile(lines);
				layout_ = makeLayout(descriptor);
				makeTextureSet(descriptor, loadTextures(textureManager, lines));
			}

			virtual const RendererTypes getRendererType() const = 0;
		protected:
			void readFile(std::vector<std::string>& lines) {
				// TODO validation of input
				std::ifstream file("../Data/Materials/" + materialFileName_ + ".qmat");
				ASSERT(file.is_open());
				size_t count;
				file >> count;
				lines.reserve(count + 1);
				std::string line;
				while (line != "END") {
					file >> line;
					lines.emplace_back(line);
				}
				file.close();
			}

			void makeTextureSet(Descriptor* descriptor, std::vector<TextureSampler*> samplers) {
				if (samplers.size() > 0) {
					textureSet_ = descriptor->getSet(descriptor->createSets({ layout_ }));
					std::vector<VkWriteDescriptorSet> setWrites(samplers.size());
					for (size_t i = 0; i < samplers.size(); ++i) {
						setWrites[i] = samplers[i]->descriptorWrite(textureSet_, i);
					}
					descriptor->updateDescriptorSets(setWrites);
				}
			}

			constexpr static VkDescriptorSetLayoutBinding makeLayoutBinding(uint32_t idx, VkShaderStageFlags stageFlags, VkSampler* sampler) {
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.binding = idx;
				layoutBinding.descriptorCount = 1;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				layoutBinding.pImmutableSamplers = sampler;
				layoutBinding.stageFlags = stageFlags;
				return layoutBinding;
			}

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
			~ParticleMaterial() {
				SAFE_DELETE(diffuse_);
			}

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
				: Material(materialFileName) { }

			StaticMaterial(const std::string name, VkDescriptorSet& set, VkDescriptorSetLayout& layout)
				: Material(name, set, layout) { }

			~StaticMaterial() {
				if (!isUsingDI) {
					SAFE_DELETE(diffuse_.diffuseSampler);
					SAFE_DELETE(normalMap_.normalMapSampler);
				}
			}

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

			TerrainMaterial(const std::string name, VkDescriptorSet& set, VkDescriptorSetLayout& layout)
				: Material(name, set, layout) { }

			~TerrainMaterial() {
				SAFE_DELETE(heightmap_);
				SAFE_DELETE(diffuse_);
			}

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

			~AtmosphereMaterial() {
			}

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
