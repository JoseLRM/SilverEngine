#pragma once

#include "SilverEngine/scene.h"

namespace sv {

	// UTILS

	constexpr Format OFFSCREEN_FORMAT = Format_R16G16B16A16_FLOAT;
	constexpr Format ZBUFFER_FORMAT = Format_D24_UNORM_S8_UINT;

	// Functions

	Result offscreen_create(u32 width, u32 height, GPUImage** pImage);
	Result depthstencil_create(u32 width, u32 height, GPUImage** pImage);

	// FONT

	struct Glyph {
		v4_f32 texCoord;
		f32 advance;
		f32 xoff, yoff;
		f32 w, h;
		f32 left_side_bearing;
	};

	enum FontFlag : u32 {
		FontFlag_None,
		FontFlag_Monospaced,
	};
	typedef u32 FontFlags;

	struct Font {

		static constexpr u32 CHAR_COUNT = u32('~') + 1u;

		Glyph* glyphs = nullptr;
		GPUImage* image = nullptr;

		Glyph* get(u32 index) { 
			if (index < CHAR_COUNT)
				return glyphs + index; 
			return nullptr;
		}

	};

	Result font_create(Font& font, const char* filepath, f32 pixelHeight, FontFlags flags);
	Result font_destroy(Font& font);

	// In Clip space
	f32 compute_text_width(const char* text, u32 count, f32 font_size, f32 aspect, Font* pFont);
	f32 compute_text_height(const char* text, u32 count, f32 font_size, f32 max_line_width, f32 aspect, Font* pFont);
	u32 compute_text_lines(const char* text, u32 count, f32 font_size, f32 max_line_width, f32 aspect, Font* pFont);

	const char* process_text_line(const char* it, u32 count, f32 max_line_width, Font& font);

	// TEXT RENDERING

	struct Font;

	enum TextAlignment : u32 {
		TextAlignment_Left,
		TextAlignment_Center,
		TextAlignment_Right,
		TextAlignment_Justified,
	};

	/*
		Clip space rendering
		Return the number of character rendered
	*/
	u32 draw_text(GPUImage* offscreen, const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, Font* pFont, Color color, CommandList cmd);

	SV_INLINE u32 draw_text(GPUImage* offscreen, const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, Font* pFont, CommandList cmd)
	{
		return draw_text(offscreen, text, text_size, x, y, max_line_width, max_lines, font_size, aspect, alignment, pFont, Color::White(), cmd);
	}

	void draw_text_area(GPUImage* offscreen, const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, u32 line_offset, bool bottom_top, Font* pFont, Color color, CommandList cmd);

	// SCENE

	void draw_scene(Scene* scene);

	void draw_sky(GPUImage* offscreen, XMMATRIX view_matrix, const XMMATRIX& projection_matrix, CommandList cmd);

	XMMATRIX camera_view_matrix(const v3_f32& position, const v4_f32 rotation, CameraComponent& camera);
	XMMATRIX camera_projection_matrix(CameraComponent& camera);
	XMMATRIX camera_view_projection_matrix(const v3_f32& position, const v4_f32 rotation, CameraComponent& camera);

	// DEBUG RENDERER

	void begin_debug_batch(CommandList cmd);
	void end_debug_batch(GPUImage* offscreen, const XMMATRIX& viewProjectionMatrix, CommandList cmd);


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

	void draw_debug_orthographic_grip(const v2_f32& position, const v2_f32& offset, const v2_f32& size, const v2_f32& gridSize, Color color, CommandList cmd);

}
