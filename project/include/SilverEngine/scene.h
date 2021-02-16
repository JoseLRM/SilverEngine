#pragma once

#include "SilverEngine/entity_system.h"
#include "SilverEngine/graphics.h"
#include "SilverEngine/mesh.h"

namespace sv {
    
	SV_DEFINE_COMPONENT(SpriteComponent) {

		TextureAsset texture;
		v4_f32 texcoords = { 0.f, 0.f, 1.f, 1.f };
		u32 layer = 0u;

		void serialize(ArchiveO& archive) {};
		void deserialize(ArchiveI& archive) {};

	};

	SV_DEFINE_COMPONENT(NameComponent) {

	        std::string name;

		void serialize(ArchiveO& archive) {};
		void deserialize(ArchiveI& archive) {};

	};	

	enum ProjectionType : u32 {
		ProjectionType_Clip,
		ProjectionType_Orthographic,
		ProjectionType_Perspective,
	};

	SV_DEFINE_COMPONENT(CameraComponent) {

		ProjectionType projection_type = ProjectionType_Orthographic;
		f32 near = -1000.f;
		f32 far = 1000.f;
		f32 width = 10.f;
		f32 height = 10.f;

		void serialize(ArchiveO & archive) {};
		void deserialize(ArchiveI & archive) {};

		SV_INLINE void adjust(f32 aspect)
		{
			if (width / height == aspect) return;
			v2_f32 res = { width, height };
			f32 mag = res.length();
			res.x = aspect;
			res.y = 1.f;
			res.normalize();
			res *= mag;
			width = res.x;
			height = res.y;
		}

		SV_INLINE f32 getProjectionLength()
		{
			return math_sqrt(width * width + height * height);
		}

		SV_INLINE void setProjectionLength(f32 length)
		{
			f32 currentLength = getProjectionLength();
			if (currentLength == length) return;
			width = width / currentLength * length;
			height = height / currentLength * length;
		}

	};

	SV_DEFINE_COMPONENT(MeshComponent) {
		
		MeshAsset	mesh;
		Material	material;

		void serialize(ArchiveO & archive) {};
		void deserialize(ArchiveI & archive) {};

	};

	SV_DEFINE_COMPONENT(PointLightComponent) {

		Color3f color = Color3f::White();
		f32 intensity = 1.f;
		f32 range = 5.f;
		f32 smoothness = 0.5f;

		void serialize(ArchiveO & archive) {};
		void deserialize(ArchiveI & archive) {};

	};

	SV_DEFINE_COMPONENT(DirectionLightComponent) {

		Color3f color = Color3f::White();
		f32 intensity = 1.f;
		v3_f32 direction = { 0.f, 0.f, 1.f };

		void serialize(ArchiveO & archive) {};
		void deserialize(ArchiveI & archive) {};

	};

	SV_DEFINE_COMPONENT(SpotLightComponent) {

		Color3f color = Color3f::White();
		f32 intensity = 1.f;
		v3_f32 direction = { 0.f, 0.f, 1.f };
		f32 range = 5.f;
		f32 smoothness = 0.5f;

		void serialize(ArchiveO & archive) {};
		void deserialize(ArchiveI & archive) {};

	};

}
