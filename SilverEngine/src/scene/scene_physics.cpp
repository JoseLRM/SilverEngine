#include "core.h"

#include "scene_internal.h"

#include "physics_impl.h"

namespace sv {

	Result scene_physics_create(const SceneDesc* desc, Scene_internal& scene)
	{
		// BOX 2D
		{
			b2World& world = *new b2World({ desc->gravity.x, desc->gravity.y });
			scene.pWorld2D = &world;
		}
		return Result_Success;
	}

	Result scene_physics_destroy(Scene_internal& scene)
	{
		// BOX 2D
		{
			b2World* pWorld = reinterpret_cast<b2World*>(scene.pWorld2D);
			delete pWorld;
			scene.pWorld2D = nullptr;
		}
		return Result_Success;
	}

	void scene_physics2D_simulate(Scene_internal& scene, float dt)
	{
		b2World& world = *reinterpret_cast<b2World*>(scene.pWorld2D);

		// Adjust Box2D bodies
		{
			EntityView<RigidBody2DComponent> bodies(scene.ecs);
			for (RigidBody2DComponent& body : bodies) {
				
				Transform trans = ecs_entity_transform_get(scene.ecs, body.entity);

				vec3 position = trans.GetWorldPosition();
				//TODO: vec3 rotation	= trans.GetWorldRotation();
				const vec3& rotation = trans.GetLocalRotation();
				vec3 worldScale = trans.GetWorldScale();

				// Create body
				if (body.pInternal == nullptr) {
					b2BodyDef def;
					def.type = body.dynamic ? b2_dynamicBody : b2_staticBody;
					def.fixedRotation = body.fixedRotation;
					def.linearVelocity.Set(body.velocity.x, body.velocity.y);
					def.angularVelocity = body.angularVelocity;
					
					body.pInternal = world.CreateBody(&def);
				}

				b2Body* b2body = reinterpret_cast<b2Body*>(body.pInternal);

				// Create fixtures
				if (body.boxCollidersCount > 0u && body.boxColliders[body.boxCollidersCount - 1u].pInternal == nullptr) {

					Box2DCollider* end = body.boxColliders - 1u;
					Box2DCollider* it = end + body.boxCollidersCount;
					while (it != end) {

						if (it->pInternal == nullptr) {

							b2FixtureDef def;
							def.density = it->density;
							def.friction = it->friction;
							def.restitution = it->restitution;

							b2PolygonShape shape;
							vec2 scale = (vec2(worldScale.x, worldScale.y) * it->size) / 2.f;
							shape.SetAsBox(scale.x, scale.y, { it->offset.x, it->offset.y }, it->angularOffset);
							def.shape = &shape;

							it->pInternal = b2body->CreateFixture(&def);
						}
						else break;

						--it;
					}

				}

				// Update fixtures
				{
					Box2DCollider* end = body.boxColliders + body.boxCollidersCount;
					Box2DCollider* it = body.boxColliders;
					while (it != end) {

						vec2 scale = (vec2(worldScale.x, worldScale.y) * it->size) / 2.f;

						b2Fixture* fixture = reinterpret_cast<b2Fixture*>(it->pInternal);
						b2PolygonShape* shape = reinterpret_cast<b2PolygonShape*>(fixture->GetShape());
						shape->SetAsBox(scale.x, scale.y, { it->offset.x, it->offset.y }, it->angularOffset);
						fixture->SetDensity(it->density);
						fixture->SetFriction(it->friction);
						fixture->SetRestitution(it->restitution);

						++it;
					}
				}

				// Update body
				b2body->SetTransform({ position.x, position.y }, rotation.z);
				b2body->SetType(body.dynamic ? b2_dynamicBody : b2_staticBody);
				b2body->SetFixedRotation(body.fixedRotation);
				b2body->SetLinearVelocity({ body.velocity.x, body.velocity.y });
				b2body->SetAngularVelocity(body.angularVelocity);
			}
		}

		// Simulation
		world.Step(dt * scene.timeStep, 6u, 2u);

		// Get simulation result
		{
			EntityView<RigidBody2DComponent> bodies(scene.ecs);
			for (RigidBody2DComponent& body : bodies) {

				b2Body* b2body = reinterpret_cast<b2Body*>(body.pInternal);
				
				const b2Transform& transform = b2body->GetTransform();
				Transform trans = ecs_entity_transform_get(scene.ecs, body.entity);

				if (ecs_entity_parent_get(scene.ecs, body.entity) == SV_ENTITY_NULL) {
					trans.SetPosition({ transform.p.x, transform.p.y, trans.GetLocalPosition().z });
					trans.SetRotation({ trans.GetLocalRotation().x, trans.GetLocalRotation().y, transform.q.GetAngle() });
				}
				else {
					log_error("TODO: Get simulation result from a child");
				}

				const b2Vec2& linearVelocity = b2body->GetLinearVelocity();
				body.velocity = { linearVelocity.x, linearVelocity.y };
				body.angularVelocity = b2body->GetAngularVelocity();
			}
		}
	}

	void scene_physics_simulate(Scene* scene_, float dt)
	{
		parseScene();

		scene_physics2D_simulate(scene, dt);
	}

}