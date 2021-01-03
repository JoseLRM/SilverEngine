#pragma once

#include "core.h"
#include "utils/io.h"
#include "simulation/event_system.h"

#define SV_ENTITY_NULL 0u
#define SV_COMPONENT_ID_INVALID std::numeric_limits<sv::CompID>::max()

namespace sv {

	typedef u16 CompID;
	typedef u32 Entity;
	SV_DEFINE_HANDLE(ECS);

	struct BaseComponent {
		Entity entity = SV_ENTITY_NULL;
	};

	template<typename T>
	struct Component : public BaseComponent {
		static CompID ID;
		static u32 SIZE;
	};

	template<typename T>
	CompID Component<T>::ID(SV_COMPONENT_ID_INVALID);

	template<typename T>
	u32 Component<T>::SIZE;

	typedef std::function<void(BaseComponent*)>							CreateComponentFunction;
	typedef std::function<void(BaseComponent*)>							DestroyComponentFunction;
	typedef std::function<void(BaseComponent* from, BaseComponent* to)> MoveComponentFunction;
	typedef std::function<void(BaseComponent* from, BaseComponent* to)> CopyComponentFunction;
	typedef std::function<void(BaseComponent* comp, ArchiveO&)>			SerializeComponentFunction;
	typedef std::function<void(BaseComponent* comp, ArchiveI&)>			DeserializeComponentFunction;

	struct ComponentRegisterDesc {

		CompID							compID;
		CreateComponentFunction			createFn;
		DestroyComponentFunction		destroyFn;
		MoveComponentFunction			moveFn;
		CopyComponentFunction			copyFn;
		SerializeComponentFunction		serializeFn;
		DeserializeComponentFunction	deserializeFn;

	};

	class Transform {
		void* trans;
		ECS* pECS;

	public:
		Transform(Entity entity, void* transform, ECS* ecs);
		~Transform() = default;

		Transform(const Transform& other) = default;
		Transform(Transform&& other) = default;

		const Entity entity = 0;

		// getters
		const vec3f&	getLocalPosition() const noexcept;
		const vec4f&	getLocalRotation() const noexcept;
		vec3f			getLocalEulerRotation() const noexcept;
		const vec3f&	getLocalScale() const noexcept;
		XMVECTOR getLocalPositionDXV() const noexcept;
		XMVECTOR getLocalRotationDXV() const noexcept;
		XMVECTOR getLocalScaleDXV() const noexcept;
		XMMATRIX getLocalMatrix() const noexcept;

		vec3f getWorldPosition() noexcept;
		vec4f getWorldRotation() noexcept;
		vec3f getWorldEulerRotation() noexcept;
		vec3f getWorldScale() noexcept;
		XMVECTOR getWorldPositionDXV() noexcept;
		XMVECTOR getWorldRotationDXV() noexcept;
		XMVECTOR getWorldScaleDXV() noexcept;
		XMMATRIX getWorldMatrix() noexcept;

		XMMATRIX getParentMatrix() const noexcept;

		// setters
		void setPosition(const vec3f& position) noexcept;
		void setPositionX(float x) noexcept;
		void setPositionY(float y) noexcept;
		void setPositionZ(float z) noexcept;

		void setRotation(const vec4f& rotation) noexcept;
		void setEulerRotation(const vec3f& rotation) noexcept;
		void setRotationX(float x) noexcept;
		void setRotationY(float y) noexcept;
		void setRotationZ(float z) noexcept;
		void setRotationW(float w) noexcept;

		void setScale(const vec3f& scale) noexcept;
		void setScaleX(float x) noexcept;
		void setScaleY(float y) noexcept;
		void setScaleZ(float z) noexcept;

	private:
		void updateWorldMatrix();
		void notify();

	};

	void	ecs_create(ECS** ecs);
	void	ecs_destroy(ECS* ecs);
	void	ecs_clear(ECS* ecs);

	Result	ecs_serialize(ECS* ecs, ArchiveO& archive);
	Result	ecs_deserialize(ECS* ecs, ArchiveI& archive); // Must create the ECS before deserialize it

	// Component Register

	CompID ecs_component_register(const char* name, u32 compSize);

	const char* ecs_component_name(CompID ID);
	u32		ecs_component_size(CompID ID);
	CompID		ecs_component_id(const char* name);
	u32		ecs_component_register_count();

	void ecs_register(ECS* ecs, const ComponentRegisterDesc* desc);

	void		ecs_register_create(ECS* ecs,CompID ID, BaseComponent* ptr, Entity entity);
	void		ecs_register_destroy(ECS* ecs, CompID ID, BaseComponent* ptr);
	void		ecs_register_move(ECS* ecs,CompID ID, BaseComponent* from, BaseComponent* to);
	void		ecs_register_copy(ECS* ecs,CompID ID, BaseComponent* from, BaseComponent* to);
	void		ecs_register_serialize(ECS* ecs, CompID ID, BaseComponent* comp, ArchiveO& archive);
	void		ecs_register_deserialize(ECS* ecs, CompID ID, BaseComponent* comp, ArchiveI& archive);
	bool		ecs_register_exist(ECS* ecs, CompID ID);

	// Entity

	Entity		ecs_entity_create(ECS* ecs, Entity parent = SV_ENTITY_NULL);
	void		ecs_entity_destroy(ECS* ecs, Entity entity);
	void		ecs_entity_clear(ECS* ecs, Entity entity);
	Entity		ecs_entity_duplicate(ECS* ecs, Entity entity);
	bool		ecs_entity_is_empty(ECS* ecs, Entity entity);
	bool		ecs_entity_exist(ECS* ecs, Entity entity);
	u32		ecs_entity_childs_count(ECS* ecs, Entity parent);
	void		ecs_entity_childs_get(ECS* ecs, Entity parent, Entity const** childsArray);
	Entity		ecs_entity_parent_get(ECS* ecs, Entity entity);
	Transform	ecs_entity_transform_get(ECS* ecs, Entity entity);
	u32		ecs_entity_component_count(ECS* ecs, Entity entity);

	void ecs_entities_create(ECS* ecs, u32 count, Entity parent = SV_ENTITY_NULL, Entity* entities = nullptr);
	void ecs_entities_destroy(ECS* ecs, Entity const* entities, u32 count);

	u32	ecs_entity_count(ECS* ecs);
	Entity	ecs_entity_get(ECS* ecs, u32 index);

	// Components

	BaseComponent*	ecs_component_add(ECS* ecs, Entity entity, BaseComponent* comp, CompID componentID, size_t componentSize);
	BaseComponent*	ecs_component_add_by_id(ECS* ecs, Entity entity, CompID componentID);

	BaseComponent*						ecs_component_get_by_id(ECS* ecs, Entity entity, CompID componentID);
	std::pair<CompID, BaseComponent*>	ecs_component_get_by_index(ECS* ecs, Entity entity, u32 index);
	
	void ecs_component_remove_by_id(ECS* ecs, Entity entity, CompID componentID);

	u32 ecs_component_count(ECS* ecs, CompID ID);

	// Listeners

	struct ECS_CreateEntityEvent : public Event {
		ECS*	ecs;
		Entity	entity;
	};

	struct ECS_DestroyEntityEvent : public Event {
		ECS*	ecs;
		Entity	entity;
	};

	struct ECS_AddComponentEvent : public Event {
		ECS*			ecs;
		CompID			compID;
		BaseComponent*	component;
	};

	struct ECS_RemoveComponentEvent : public Event {
		ECS*			ecs;
		CompID			compID;
		BaseComponent*	component;
	};

	EventListener* ecs_listener_OnEntityCreate(ECS* ecs);
	EventListener* ecs_listener_OnEntityDestroy(ECS* ecs);
	EventListener* ecs_listener_OnComponentAdd(ECS* ecs, CompID compID = SV_COMPONENT_ID_INVALID);
	EventListener* ecs_listener_OnComponentRemove(ECS* ecs, CompID compID = SV_COMPONENT_ID_INVALID);

	// Iterators

	class ComponentIterator {
		ECS* ecs_;
		CompID compID;

		BaseComponent* it;
		u32 pool;

	public:
		ComponentIterator(ECS* ecs, CompID compID, bool end);

		BaseComponent* get_ptr();
		
		void start_begin();
		void start_end();

		bool equal(const ComponentIterator& other) const noexcept;
		void next();
		void last();

	};

	// TEMPLATES

	template<typename Component>
	void ecs_component_register(const char* name)
	{
		Component::SIZE = sizeof(Component);
		Component::ID = ecs_component_register(name, Component::SIZE);
	}

	template<typename Component>
	void ecs_register(ECS* ecs, SerializeComponentFunction serializeFn = nullptr, DeserializeComponentFunction deserializeFn = nullptr)
	{
		ComponentRegisterDesc desc;

		desc.compID = Component::ID;

		desc.createFn = [](BaseComponent* compPtr)
		{
			new(compPtr) Component();
		};

		desc.destroyFn = [](BaseComponent* compPtr)
		{
			Component* comp = reinterpret_cast<Component*>(compPtr);
			comp->~Component();
		};

		desc.moveFn = [](BaseComponent* fromB, BaseComponent* toB)
		{
			Component* from = reinterpret_cast<Component*>(fromB);
			Component* to = reinterpret_cast<Component*>(toB);
			new(to) Component(std::move(*from));
		};

		desc.copyFn = [](BaseComponent* fromB, BaseComponent* toB)
		{
			Component* from = reinterpret_cast<Component*>(fromB);
			Component* to = reinterpret_cast<Component*>(toB);
			new(to) Component(*from);
		};

		desc.serializeFn = serializeFn;
		desc.deserializeFn = deserializeFn;

		ecs_register(ecs, &desc);
	}

	template<typename Component, typename... Args>
	Component* ecs_component_add(ECS* ecs, Entity entity, Args&& ... args) {
		Component component(std::forward<Args>(args)...);
		return reinterpret_cast<Component*>(ecs_component_add(ecs, entity, (BaseComponent*)& component, Component::ID, Component::SIZE));
	}

	template<typename Component>
	Component* ecs_component_add(ECS* ecs, Entity entity) {
		return reinterpret_cast<Component*>(ecs_component_add_by_id(ecs, entity, Component::ID));
	}

	template<typename Component>
	Component* ecs_component_get(ECS* ecs, Entity entity)
	{
		return reinterpret_cast<Component*>(ecs_component_get_by_id(ecs, entity, Component::ID));
	}

	template<typename Component>
	void ecs_component_remove(ECS* ecs, Entity entity) {
		ecs_component_remove_by_id(ecs, entity, Component::ID);
	}

	template<typename Component>
	class EntityView {

		ECS* m_ECS;

	public:

		class TemplatedComponentIterator {
			ComponentIterator it;

		public:
			TemplatedComponentIterator(ECS* ecs, CompID compID, bool end)
				: it(ecs, compID, end) {}
			inline Component* operator->() { return reinterpret_cast<Component*>(it.get_ptr()); }
			inline Component& operator*() { return *reinterpret_cast<Component*>(it.get_ptr()); }

			inline bool operator==(const TemplatedComponentIterator& other) const noexcept { return it.equal(other.it); }
			inline bool operator!=(const TemplatedComponentIterator& other) const noexcept { return !it.equal(other.it); }
			inline void operator+=(u32 count) { while (count-- > 0) it.next(); }
			inline void operator-=(u32 count) { while (count-- > 0) it.last(); }
			inline void operator++() { it.next(); }
			inline void operator--() { it.last(); }
		};

	public:
		EntityView(ECS* ecs) : m_ECS(ecs) {}

		u32 size()
		{
			return ecs_component_count(m_ECS, Component::ID);
		}

		TemplatedComponentIterator begin()
		{
			TemplatedComponentIterator iterator(m_ECS, Component::ID, false);
			return iterator;
		}

		TemplatedComponentIterator end()
		{
			TemplatedComponentIterator iterator(m_ECS, Component::ID, true);
			return iterator;
		}

	};

}