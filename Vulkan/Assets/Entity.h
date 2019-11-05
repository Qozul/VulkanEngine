/*
Author: Ralph Ridley
Date: 01/07/19

Entity is the basic object type used in the engine, having only a Transform by default, but able to have other
components.
*/
#pragma once

#include "../Graphics/VkUtil.h"
#include "../Graphics/GraphicsTypes.h"
#include "../Graphics/GraphicsComponent.h"

namespace QZL {
	class Transform;
	namespace Physics {
		class RigidBody;
		class CollisionVolume;
	}
	namespace Game {
		class GameScript;
		struct SceneHeirarchyNode;
	}
	namespace Assets {
		class Entity {
			friend class AssetManager;
		public:
			virtual void update(float dt, const glm::mat4& parentMatrix);
			void start();

			void setGraphicsComponent(const Graphics::RendererTypes rtype, Graphics::ShaderParams* perMeshParams, Graphics::ShaderParams* perInstanceParams,
				Graphics::Material* material, const std::string& meshName, Graphics::MeshLoadFunc loadFunc = nullptr);
			void setGraphicsComponent(const Graphics::RendererTypes rtype, Graphics::RenderObject* robject, Graphics::ShaderParams* perInstanceParams = nullptr);
			void setGameScript(Game::GameScript* script);
			Graphics::GraphicsComponent* getGraphicsComponent() {
				return graphicsComponent_;
			}
			Physics::RigidBody* getRigidBody() {
				return rigidBody_;
			}
			Game::GameScript* getGameScript() {
				return gameScript_;
			}
			Physics::CollisionVolume* getCollisionVolume() {
				return collisionVolume_;
			}
			Transform* getTransform() {
				return transform_;
			}
			glm::mat4 getModelMatrix() {
				return modelMatrix_;
			}
			void setModelMatrix(glm::mat4& m) {
				modelMatrix_ = m;
			}
			void setSceneNode(Game::SceneHeirarchyNode* node) {
				sceneNode_ = node;
			}
			Game::SceneHeirarchyNode* getSceneNode() {
				return sceneNode_;
			}
			const std::string name() const {
				return name_;
			}
			// Only the game script and rigidbody will change the transform and graphics uniform constants. 
			// Therefore, some optimisations can be made.
			bool isStatic() const;
		protected:
			Entity(const std::string name);
			virtual ~Entity();

			Physics::CollisionVolume* collisionVolume_;
			Physics::RigidBody* rigidBody_;
			Game::GameScript* gameScript_;
			Graphics::GraphicsComponent* graphicsComponent_;
			Transform* transform_;
			Game::SceneHeirarchyNode* sceneNode_;
			glm::mat4 modelMatrix_;
			bool preserveChildrenOnDelete_;
			const std::string name_;
		};
	}
}
