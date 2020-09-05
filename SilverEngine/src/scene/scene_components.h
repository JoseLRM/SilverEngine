#pragma once

namespace sv {

	// Name component
#if SV_SCENE_NAME_COMPONENT

	struct NameComponent : public Component<NameComponent> {
	
		std::string name;

		NameComponent() = default;
		NameComponent(const char* name) : name(name) {}
		NameComponent(const std::string& name) : name(name) {}
		NameComponent(std::string&& name) : name(std::move(name)) {}

	};

#endif

	// Sprite Component

	struct SpriteComponent : public Component<SpriteComponent> {

		Sprite sprite;
		Color color = SV_COLOR_WHITE;

		SpriteComponent() {}
		SpriteComponent(Color col) : color(col) {}
		SpriteComponent(Sprite spr) : sprite(spr) {}
		SpriteComponent(Sprite spr, Color col) : sprite(spr), color(col) {}

	};

	// Camera Component

	struct CameraComponent : public Component<CameraComponent> {
	private:
		std::unique_ptr<Offscreen> m_Offscreen;
	
	public:
		CameraSettings		settings;
	
		CameraComponent();
		CameraComponent(CameraType projectionType);
	
		CameraComponent(const CameraComponent & other);
		CameraComponent(CameraComponent && other) noexcept;
		CameraComponent& operator=(const CameraComponent & other);
		CameraComponent& operator=(CameraComponent && other) noexcept;
	
		bool CreateOffscreen(ui32 width, ui32 height);
		bool HasOffscreen() const noexcept;
		Offscreen* GetOffscreen() const noexcept;
		bool DestroyOffscreen();
	
		void Adjust(float width, float height);

	};

	// Rigid body 2d component

	struct RigidBody2DComponent : public Component<RigidBody2DComponent> {
		void* pInternal;

		bool dynamic = true;
		bool fixedRotation = false;
		vec2 velocity;
		float angularVelocity = 0.f;

		RigidBody2DComponent();
		~RigidBody2DComponent();
		RigidBody2DComponent& operator=(const RigidBody2DComponent & other);
		RigidBody2DComponent& operator=(RigidBody2DComponent && other) noexcept;
	};

	// Quad Collider

	struct QuadComponent : public Component<QuadComponent> {
		void* pInternal;

		vec2 size = { 1.f, 1.f };
		vec2 offset;
		float angularOffset = 0.f;
		float density = 10.f;
		float friction = 0.3f;
		float restitution = 0.3f;

		QuadComponent();
		~QuadComponent();
		QuadComponent& operator=(const QuadComponent & other);
		QuadComponent& operator=(QuadComponent && other) noexcept;

	};

	// Mesh component

	struct MeshComponent : public Component<MeshComponent> {

		SharedRef<Mesh>		mesh;
		SharedRef<Material> material;

	};

	// Light component

	struct LightComponent : public Component<LightComponent> {

		LightType	lightType	= LightType_Point;
		float		intensity	= 1.f;
		float		range		= 5.f;
		float		smoothness	= 0.8f;
		Color3f		color		= SV_COLOR3F_WHITE;

	};

}