#include "core.h"

#include "entity_system_internal.h"

#define parse() sv::EntityTransform* t = reinterpret_cast<sv::EntityTransform*>(trans)

namespace sv {

	Transform::Transform(Entity entity, void* transform, ECS* ecs)
		: entity(entity), trans(transform), pECS(ecs) {}

	const vec3f& Transform::getLocalPosition() const noexcept 
	{ 
		parse();
		return *(vec3f*)& t->localPosition;
	}

	const vec4f& Transform::getLocalRotation() const noexcept 
	{ 
		parse();
		return *(vec4f*)& t->localRotation;
	}

	vec3f Transform::getLocalEulerRotation() const noexcept
	{
		parse();
		vec3f euler;

		// roll (x-axis rotation)

		XMFLOAT4 q = t->localRotation;
		float sinr_cosp = 2.f * (q.w * q.x + q.y * q.z);
		float cosr_cosp = 1.f - 2.f * (q.x * q.x + q.y * q.y);
		euler.x = std::atan2(sinr_cosp, cosr_cosp);

		// pitch (y-axis rotation)
		float sinp = 2.f * (q.w * q.y - q.z * q.x);
		if (std::abs(sinp) >= 1.f)
			euler.y = std::copysign(PI / 2.f, sinp); // use 90 degrees if out of range
		else
			euler.y = std::asin(sinp);

		// yaw (z-axis rotation)
		float siny_cosp = 2 * (q.w * q.z + q.x * q.y);
		float cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
		euler.z = std::atan2(siny_cosp, cosy_cosp);

		if (euler.x < 0.f) {
			euler.x = 2.f * PI + euler.x;
		}
		if (euler.y < 0.f) {
			euler.y = 2.f * PI + euler.y;
		}
		if (euler.z < 0.f) {
			euler.z = 2.f * PI + euler.z;
		}

		return euler;
	}

	const vec3f& Transform::getLocalScale() const noexcept 
	{ 
		parse();
		return *(vec3f*)& t->localScale;
	}

	XMVECTOR Transform::getLocalPositionDXV() const noexcept 
	{ 
		parse();
		return XMLoadFloat3(&t->localPosition);
	}

	XMVECTOR Transform::getLocalRotationDXV() const noexcept 
	{ 
		parse();
		return XMLoadFloat4(&t->localRotation);
	}

	XMVECTOR Transform::getLocalScaleDXV() const noexcept 
	{ 
		parse();
		return XMLoadFloat3(&t->localScale);
	}

	XMMATRIX Transform::getLocalMatrix() const noexcept
	{
		parse();
		return XMMatrixScalingFromVector(getLocalScaleDXV()) * XMMatrixRotationQuaternion(XMLoadFloat4(&t->localRotation))
			* XMMatrixTranslation(t->localPosition.x, t->localPosition.y, t->localPosition.z);
	}

	vec3f Transform::getWorldPosition() noexcept
	{
		parse();
		if (t->modified) updateWorldMatrix();
		return *(vec3f*)& t->worldMatrix._41;
	}

	vec4f Transform::getWorldRotation() noexcept
	{
		parse();
		if (t->modified) updateWorldMatrix();
		return *(vec4f*)& getWorldRotationDXV();
	}

	vec3f Transform::getWorldEulerRotation() noexcept
	{
		parse();
		XMFLOAT4X4 rm;
		XMStoreFloat4x4(&rm, XMMatrixTranspose(XMMatrixRotationQuaternion(getWorldRotationDXV())));

		vec3f euler;

		if (rm._13 < 1.f) {
			if (rm._13 > -1.f) {
				euler.y = asin(rm._13);
				euler.x = atan2(-rm._23, rm._33);
				euler.z = atan2(-rm._12, rm._11);
			}
			else {
				euler.y = -PI / 2.f;
				euler.x = -atan2(rm._21, rm._22);
				euler.z = 0.f;
			}
		}
		else {
			euler.y = PI / 2.f;
			euler.x = atan2(rm._21, rm._22);
			euler.z = 0.f;
		}

		if (euler.x < 0.f) {
			euler.x = 2 * PI + euler.x;
		}
		if (euler.z < 0.f) {
			euler.z = 2 * PI + euler.z;
		}

		return euler;
	}

	vec3f Transform::getWorldScale() noexcept
	{
		parse();
		if (t->modified) updateWorldMatrix();
		return { (*(vec3f*)& t->worldMatrix._11).length(), (*(vec3f*)& t->worldMatrix._21).length(), (*(vec3f*)& t->worldMatrix._31).length() };
	}

	XMVECTOR Transform::getWorldPositionDXV() noexcept
	{
		parse();
		if (t->modified) updateWorldMatrix();

		vec3f position = getWorldPosition();
		return XMVectorSet(position.x, position.y, position.z, 0.f);
	}

	XMVECTOR Transform::getWorldRotationDXV() noexcept
	{
		parse();
		if (t->modified) updateWorldMatrix();
		XMVECTOR scale;
		XMVECTOR rotation;
		XMVECTOR position;

		XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&t->worldMatrix));

		return rotation;
	}

	XMVECTOR Transform::getWorldScaleDXV() noexcept
	{
		parse();

		if (t->modified) updateWorldMatrix();
		XMVECTOR scale;
		XMVECTOR rotation;
		XMVECTOR position;

		XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&t->worldMatrix));

		return XMVectorAbs(scale);
	}

	XMMATRIX Transform::getWorldMatrix() noexcept
	{
		parse();

		if (t->modified) updateWorldMatrix();
		return XMLoadFloat4x4(&t->worldMatrix);
	}

	XMMATRIX Transform::getParentMatrix() const noexcept
	{
		parse();
		ECS_internal& ecs = *reinterpret_cast<ECS_internal*>(pECS);

		EntityData& entityData = ecs.entityData[entity];
		Entity parent = entityData.parent;

		if (parent) {
			Transform parentTransform(parent, &ecs.entityData.get_transform(parent), pECS);
			return parentTransform.getWorldMatrix();
		}
		else return XMMatrixIdentity();
	}

	void Transform::setPosition(const vec3f& position) noexcept
	{
		notify();

		parse();
		t->localPosition = *(XMFLOAT3*)& position;
	}

	void Transform::setPositionX(float x) noexcept
	{
		notify();

		parse();
		t->localPosition.x = x;
	}

	void Transform::setPositionY(float y) noexcept
	{
		notify();

		parse();
		t->localPosition.y = y;
	}

	void Transform::setPositionZ(float z) noexcept
	{
		notify();

		parse();
		t->localPosition.z = z;
	}

	void Transform::setRotation(const vec4f& rotation) noexcept
	{
		notify();

		parse();
		t->localRotation = *(XMFLOAT4*)& rotation;
	}

	void Transform::setEulerRotation(const vec3f& r) noexcept
	{
		notify();

		float cy = cos(r.z * 0.5f);
		float sy = sin(r.z * 0.5f);
		float cp = cos(r.y * 0.5f);
		float sp = sin(r.y * 0.5f);
		float cr = cos(r.x * 0.5f);
		float sr = sin(r.x * 0.5f);

		vec4f q;
		q.w = cr * cp * cy + sr * sp * sy;
		q.x = sr * cp * cy - cr * sp * sy;
		q.y = cr * sp * cy + sr * cp * sy;
		q.z = cr * cp * sy - sr * sp * cy;

		parse();
		XMStoreFloat4(&t->localRotation, XMVectorSet(q.x, q.y, q.z, q.w));
	}

	void Transform::setRotationX(float x) noexcept
	{
		notify();

		parse();
		t->localRotation.x = x;
	}

	void Transform::setRotationY(float y) noexcept
	{
		notify();

		parse();
		t->localRotation.y = y;
	}

	void Transform::setRotationZ(float z) noexcept
	{
		notify();

		parse();
		t->localRotation.z = z;
	}

	void Transform::setRotationW(float w) noexcept
	{
		notify();

		parse();
		t->localRotation.w = w;
	}

	void Transform::setScale(const vec3f& scale) noexcept
	{
		parse();

		notify();
		t->localScale = *(XMFLOAT3*)& scale;
	}

	void Transform::setScaleX(float x) noexcept
	{
		notify();

		parse();
		t->localScale.x = x;
	}

	void Transform::setScaleY(float y) noexcept
	{
		notify();

		parse();
		t->localScale.y = y;
	}

	void Transform::setScaleZ(float z) noexcept
	{
		notify();

		parse();
		t->localScale.z = z;
	}

	void Transform::updateWorldMatrix()
	{
		parse();
		ECS_internal& ecs = *reinterpret_cast<ECS_internal*>(pECS);

		t->modified = false;

		XMMATRIX m = getLocalMatrix();

		EntityData& entityData = ecs.entityData[entity];
		Entity parent = entityData.parent;

		if (parent != SV_ENTITY_NULL) {
			Transform parentTransform(parent, &ecs.entityData.get_transform(parent), pECS);
			XMMATRIX mp = parentTransform.getWorldMatrix();
			m = m * mp;
		}
		XMStoreFloat4x4(&t->worldMatrix, m);
	}

	void Transform::notify()
	{
		parse();

		if (!t->modified) {

			t->modified = true;

			ECS_internal& ecs = *reinterpret_cast<ECS_internal*>(pECS);
			auto& list = ecs.entityData;
			EntityData& entityData = list[entity];

			if (entityData.childsCount == 0) return;

			auto& entities = ecs.entities;
			for (u32 i = 0; i < entityData.childsCount; ++i) {
				EntityTransform& et = list.get_transform(entities[entityData.handleIndex + 1 + i]);
				et.modified = true;
			}

		}
	}

}