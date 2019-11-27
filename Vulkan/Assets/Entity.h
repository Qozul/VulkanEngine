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
	struct SceneHeirarchyNode;
	using SimpleUpdateFunction = std::function<void(float)>;
	namespace Physics {
		class RigidBody;
		class CollisionVolume;
	}
	namespace Game {
		class GameScript;
	}
	class Entity {
		friend class Game::GameScript;
		friend class Graphics::GraphicsComponent;
	public:
		virtual void update(float dt, const glm::mat4& viewProjection, const glm::mat4& parentMatrix);
		void start();

		void setSimpleUpdateFunction(SimpleUpdateFunction updateFunc) {
			updateFunc_ = updateFunc;
		}
		void setGraphicsComponent(Graphics::GraphicsComponent* component);
		void setGameScript(Game::GameScript* script);

		void setGraphicsComponent(const Graphics::RendererTypes rtype, Graphics::ShaderParams* perMeshParams, Graphics::ShaderParams* perInstanceParams,
			Graphics::Material* material, const std::string& meshName, Graphics::MeshLoadFunc loadFunc = nullptr);
		void setGraphicsComponent(Graphics::RendererTypes rtype, Graphics::ShaderParams* params, const std::string& meshName, Graphics::Material* material);
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
		void setSceneNode(SceneHeirarchyNode* node) {
			sceneNode_ = node;
		}
		SceneHeirarchyNode* getSceneNode() {
			return sceneNode_;
		}
		const std::string name() const {
			return name_;
		}
		// Only the game script and rigidbody will change the transform and graphics uniform constants. 
		// Therefore, some optimisations can be made.
		bool isStatic() const;

		Entity(const std::string name);
		virtual ~Entity();
	protected:
		SimpleUpdateFunction updateFunc_;
		Physics::CollisionVolume* collisionVolume_;
		Physics::RigidBody* rigidBody_;
		Game::GameScript* gameScript_;
		Graphics::GraphicsComponent* graphicsComponent_;
		Transform* transform_;
		SceneHeirarchyNode* sceneNode_;
		glm::mat4 modelMatrix_;
		bool preserveChildrenOnDelete_;
		const std::string name_;
	};
}
