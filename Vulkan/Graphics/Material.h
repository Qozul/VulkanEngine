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
			Material(const std::string materialFileName)
				: materialFileName_(materialFileName), textureSet_(VK_NULL_HANDLE)
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
				makeTextureSet(descriptor, loadTextures(textureManager, lines));
			}

			virtual const RendererTypes getRendererType() const = 0;
		protected:
			void readFile(std::vector<std::string>& lines) {
				// TODO validation
				std::ifstream file("../Data/Materials/" + materialFileName_ + ".qmat");
				ASSERT(file.is_open());
				int count;
				file >> count;
				lines.reserve(count);
				std::string line;
				while (line != "") {
					file >> line;
					lines.emplace_back(line);
				}
				file.close();
			}

			void makeTextureSet(Descriptor* descriptor, std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings) {
				layout_ = descriptor->makeLayout(setLayoutBindings);
				textureSet_ = descriptor->getSet(descriptor->createSets({ layout_ }));
			}

			VkDescriptorSetLayoutBinding makeLayoutBinding(uint32_t idx, VkShaderStageFlags stageFlags, VkSampler* sampler) {
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.binding = idx;
				layoutBinding.descriptorCount = 1;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				layoutBinding.pImmutableSamplers = sampler;
				layoutBinding.stageFlags = stageFlags;
				return layoutBinding;
			}

			virtual std::vector<VkDescriptorSetLayoutBinding> loadTextures(TextureManager* textureManager, std::vector<std::string>& lines) = 0;

			VkDescriptorSet textureSet_;
			VkDescriptorSetLayout layout_;
			const std::string materialFileName_;
		};

		class ParticleMaterial : public Material {
		public:
			ParticleMaterial(const std::string materialFileName)
				: Material(materialFileName), diffuse_(nullptr) { }

			const RendererTypes getRendererType() const override {
				return RendererTypes::PARTICLE;
			}
			~ParticleMaterial() {
				SAFE_DELETE(diffuse_);
			}
		protected:
			std::vector<VkDescriptorSetLayoutBinding> loadTextures(TextureManager* textureManager, std::vector<std::string>& lines) override;
		private:
			TextureSampler* diffuse_;
		};

		// TODO
		class StaticMaterial : public Material {
		public:
			StaticMaterial(const std::string materialFileName)
				: Material(materialFileName) { }

			StaticMaterial(const std::string name, VkDescriptorSet& set, VkDescriptorSetLayout& layout)
				:Material(name, set, layout) { }

			~StaticMaterial() {

			}
			const RendererTypes getRendererType() const override {
				return RendererTypes::STATIC;
			}
		protected:
			std::vector<VkDescriptorSetLayoutBinding> loadTextures(TextureManager* textureManager, std::vector<std::string>& lines) override;
		private:
			union {
				uint32_t diffuseTextureIndex;
				TextureSampler* diffuseSampler;
			} diffuse_;
			union {
				uint32_t normalMapIndex;
				TextureSampler* normalMapSampler;
			} normalMap_;
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

			const RendererTypes getRendererType() const override {
				return RendererTypes::TERRAIN;
			}
		protected:
			std::vector<VkDescriptorSetLayoutBinding> loadTextures(TextureManager* textureManager, std::vector<std::string>& lines) override;
		private:
			TextureSampler* heightmap_;
			TextureSampler* diffuse_;
		};

		class AtmosphereMaterial : public Material {
		public:
			AtmosphereMaterial(const std::string materialFileName)
				: Material(materialFileName), scatteringTexture_(nullptr) { }

			AtmosphereMaterial(const std::string name, VkDescriptorSet& set, VkDescriptorSetLayout& layout)
				: Material(name, set, layout) { }

			~AtmosphereMaterial() {
			}

			const RendererTypes getRendererType() const override {
				return RendererTypes::TERRAIN;
			}
		protected:
			std::vector<VkDescriptorSetLayoutBinding> loadTextures(TextureManager* textureLoader, std::vector<std::string>& lines) override;
		private:
			TextureSampler* scatteringTexture_;
		};

		// TODO delete

		/*struct MaterialStatic {
			// 32 bytes
			float diffuseX, diffuseY, diffuseZ; // Mixed in with texture
			float alpha; // Sets maximum alpha
			float specularX, specularY, specularZ; // Can create tinting
			float specularExponent; // The intesity of the colour
			uint32_t diffuseTextureIndex; // Only used if descriptor indexing is enabled
			uint32_t normalMapIndex; // Only used if descriptor indexing is enabled
			float padding0, padding1;
			MaterialStatic(glm::vec3 diffuseColour, glm::vec3 specularColour, float alpha, float specExponent) 
				: diffuseX(diffuseColour.x), diffuseY(diffuseColour.y), diffuseZ(diffuseColour.z), alpha(alpha),
				  specularX(specularColour.x), specularY(specularColour.y), specularZ(specularColour.z), specularExponent(specExponent),
				diffuseTextureIndex(0), normalMapIndex(0), padding0(0.0f), padding1(0.0f) { }
			MaterialStatic(glm::vec3 diffuseColour, glm::vec3 specularColour, float alpha, float specExponent, 
				uint32_t diffuseIndex, uint32_t normalMapIndex)
				: diffuseX(diffuseColour.x), diffuseY(diffuseColour.y), diffuseZ(diffuseColour.z), alpha(alpha),
				specularX(specularColour.x), specularY(specularColour.y), specularZ(specularColour.z), specularExponent(specExponent),
				diffuseTextureIndex(diffuseIndex), normalMapIndex(normalMapIndex), padding0(0.0f), padding1(0.0f) { }
		};
		struct MaterialAtmosphere {
			glm::vec3 betaRay;
			float betaMie;

			glm::vec3 cameraPosition;
			float planetRadius;

			glm::vec3 sunDirection;
			float Hatm;

			glm::vec3 sunIntensity;
			float g;
		};
		struct MaterialParticle {
			float textureTileLength;
			glm::vec4 tint;
			TextureSampler* texture;
		};*/
	}
}
