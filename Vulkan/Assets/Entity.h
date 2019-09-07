/*
Author: Ralph Ridley
Date: 01/07/19

Entity is the basic object type used in the engine, having only a Transform by default, but able to have other
components.
*/
#pragma once

#include "../../Shared/Utility.h"
#include "../Graphics/GraphicsMaster.h"
#include "../Graphics/GraphicsComponent.h"

namespace QZL {
	struct Transform;
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
			virtual void update(float dt);
			void start();

			void setGraphicsComponent(const Graphics::RendererTypes rtype, Graphics::ShaderParams* shaderParams, const std::string& meshName, 
				Graphics::MeshLoaderFunction meshLoaderFunc = nullptr);
			void setGraphicsComponent(const Graphics::RendererTypes rtype, Graphics::ShaderParams* shaderParams, const std::string& meshName,
				Graphics::MeshLoaderFunctionOnlyPos meshLoaderFunc);
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
			// Only the game script and rigidbody will change the transform and graphics uniform constants. 
			// Therefore, some optimisations can be made.
			bool isStatic() const;
		protected:
			Entity();
			virtual ~Entity();

			Physics::CollisionVolume* collisionVolume_;
			Physics::RigidBody* rigidBody_;
			Game::GameScript* gameScript_;
			Graphics::GraphicsComponent* graphicsComponent_;
			Transform* transform_;
			Game::SceneHeirarchyNode* sceneNode_;
			glm::mat4 modelMatrix_;
			bool preserveChildrenOnDelete_;
		};
	}
}
