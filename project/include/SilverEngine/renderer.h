#pragma once

#include "SilverEngine/graphics.h"

namespace sv {

	// UTILS

	constexpr Format OFFSCREEN_FORMAT = Format_R16G16B16A16_FLOAT;
	constexpr Format ZBUFFER_FORMAT = Format_D24_UNORM_S8_UINT;

	enum ProjectionType : u32 {
		ProjectionType_Clip,
		ProjectionType_Orthographic,
		ProjectionType_Perspective,
	};

	enum LightType {
		LightType_None,
		LightType_Point,
		LightType_Direction,
	};

	struct CameraProjection {

		f32 width = 1.f;
		f32 height = 1.f;
		f32 near = -1000.f;
		f32 far = 1000.f;

		ProjectionType	projection_type = ProjectionType_Orthographic;
		XMMATRIX		projection_matrix = XMMatrixIdentity();

	};

	struct CameraBuffer {

		XMMATRIX	view_matrix = XMMatrixIdentity();
		XMMATRIX	projection_matrix = XMMatrixIdentity();
		v3_f32		position;
		v4_f32		rotation;

		GPUBuffer* buffer = nullptr;

	};

	struct LightInstance {

		Color3f color;
		LightType type;

		union {
			struct {
				v3_f32		position;
				f32			range;
				f32			intensity;
				f32			smoothness;
			} point;

			struct {
				v3_f32		direction;
				f32			intensity;
			} direction;
		};

		LightInstance() : color(Color3f::White()), type(LightType_None), point({}) {}

		LightInstance(const Color3f& color, const v3_f32& position, f32 range, f32 intensity, f32 smoothness)
			: color(color), type(LightType_Point), point({ position, range, intensity, smoothness }) {}

		LightInstance(const Color3f& color, const v3_f32& direction, f32 intensity)
			: color(color), type(LightType_Direction), direction({ direction, intensity }) {}
	};

	// CONTEXT

	struct RenderingContext {

		GPUImage*		offscreen;
		GPUImage*		zbuffer;
		CameraBuffer*	camera_buffer;

	};

	extern RenderingContext render_context[GraphicsLimit_CommandList];

	// Functions

	Result offscreen_create(u32 width, u32 height, GPUImage** pImage);
	Result zbuffer_create(u32 width, u32 height, GPUImage** pImage);

	Result	camerabuffer_create(CameraBuffer* camera_buffer);
	Result	camerabuffer_destroy(CameraBuffer* camera_buffer);
	void	camerabuffer_update(CameraBuffer* camera_buffer, CommandList cmd);

	void	projection_adjust(CameraProjection& projection, f32 aspect);
	f32		projection_length_get(CameraProjection& projection);
	void	projection_length_set(CameraProjection& projection, f32 length);
	void	projection_update_matrix(CameraProjection& projection);

	// TEXT RENDERING

	struct Font;

	enum TextSpace : u32 {
		TextSpace_Clip,
		TextSpace_Normal,
		TextSpace_Offscreen,
	};

	enum TextAlignment : u32 {
		TextAlignment_Left,
		TextAlignment_Center,
		TextAlignment_Right,
		TextAlignment_Justified,
	};

	/*
		Return the number of character rendered
	*/
	u32 draw_text(const char* text, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextSpace space, TextAlignment alignment, Font* pFont, CommandList cmd);

	// SPRITE RENDERING

	struct SpriteInstance {
		XMMATRIX	tm;
		v4_f32		texcoord;
		GPUImage*	image;
		Color		color;

		SpriteInstance() = default;
		SpriteInstance(const XMMATRIX& m, const v4_f32& texcoord, GPUImage* image, Color color)
			: tm(m), texcoord(texcoord), image(image), color(color) {}
	};

	void draw_sprites(const SpriteInstance* sprites, u32 count, const XMMATRIX& view_projection_matrix, CommandList cmd);

	// MESH RENDERING

	struct Mesh;

	struct Material {
		Color4f color;
	};

	struct MeshInstance {

		XMMATRIX	transform_matrix;
		Mesh* mesh;
		Material* material;

		MeshInstance(const XMMATRIX& transform_matrix, Mesh* mesh, Material* material)
			: transform_matrix(transform_matrix), mesh(mesh), material(material) {}

	};

	void draw_meshes(const MeshInstance* meshes, u32 mesh_count, const LightInstance* lights, u32 light_count, CommandList cmd);

	// DEBUG RENDERER

	void begin_debug_batch(CommandList cmd);
	void end_debug_batch(const XMMATRIX& viewProjectionMatrix, CommandList cmd);

	void draw_debug_quad(const XMMATRIX& matrix, Color color, CommandList cmd);
	void draw_debug_line(const v3_f32& p0, const v3_f32& p1, Color color, CommandList cmd);
	void draw_debug_ellipse(const XMMATRIX& matrix, Color color, CommandList cmd);
	void draw_debug_sprite(const XMMATRIX& matrix, Color color, GPUImage* image, CommandList cmd);

	// Helper functions

	void draw_debug_quad(const v3_f32& position, const v2_f32& size, Color color, CommandList cmd);
	void draw_debug_quad(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color, CommandList cmd);
	void draw_debug_quad(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color, CommandList cmd);

	void draw_debug_ellipse(const v3_f32& position, const v2_f32& size, Color color, CommandList cmd);
	void draw_debug_ellipse(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color, CommandList cmd);
	void draw_debug_ellipse(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color, CommandList cmd);

	void draw_debug_sprite(const v3_f32& position, const v2_f32& size, Color color, GPUImage* image, CommandList cmd);
	void draw_debug_sprite(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color, GPUImage* image, CommandList cmd);
	void draw_debug_sprite(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color, GPUImage* image, CommandList cmd);

	// Attributes

	// Line rasterization width (pixels)
	void set_debug_linewidth(f32 lineWidth, CommandList cmd);
	f32	 get_debug_linewidth(CommandList cmd);

	// Quad and ellipse stroke: 1.f renders normally, 0.01f renders thin stroke around
	void set_debug_stroke(f32 stroke, CommandList cmd);
	f32	 get_debug_stroke(CommandList cmd);

	// Sprite texCoords
	void	set_debug_texcoord(const v4_f32& texCoord, CommandList cmd);
	v4_f32	get_debug_texcoord(CommandList cmd);

	// Sprite sampler
	void set_debug_sampler_default(CommandList cmd);
	void set_debug_sampler(Sampler* sampler, CommandList cmd);

	// High level draw calls

	void draw_debug_orthographic_grip(const v2_f32& position, const v2_f32& size, float gridSize, Color color, CommandList cmd);

}