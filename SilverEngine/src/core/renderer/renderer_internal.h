#pragma once

#include "core/renderer.h"

namespace sv {

    constexpr u32 LIGHT_COUNT = 1u;

    struct LightData {
	v3_f32		position;
	LightType	type;
	v3_f32		color;
	f32			range;
	f32			intensity;
	f32			smoothness;
	f32 padding0;
	f32 padding1;
    };

    struct MeshData {
	XMMATRIX model_view_matrix;
	XMMATRIX inv_model_view_matrix;
    };

    struct MaterialData {
	v3_f32 diffuse_color;
	u32 flags;
	v3_f32 specular_color;
	f32 shininess;
	v3_f32 emissive_color;
    };

#define MAT_FLAG_NORMAL_MAPPING SV_BIT(0u)
#define MAT_FLAG_SPECULAR_MAPPING SV_BIT(1u)
#define MAT_FLAG_EMISSIVE_MAPPING SV_BIT(2u)

    struct CameraBuffer_GPU {
	XMMATRIX view_matrix;
	XMMATRIX projection_matrix;
	XMMATRIX view_projection_matrix;
	XMMATRIX inverse_view_matrix;
	XMMATRIX inverse_projection_matrix;
	XMMATRIX inverse_view_projection_matrix;
	f32      screen_width;
	f32      screen_height;
	f32      near;
	f32      far;
	v4_f32	 position;
	v4_f32	 rotation;
    };

    constexpr u32 TEXT_BATCH_COUNT = 1000u; // Num of letters
    constexpr u32 SPRITE_BATCH_COUNT = 1000u;

    struct TextVertex {
	v4_f32	position;
	v2_f32	texCoord;
	Color	color;
    };

    struct TextData {
	TextVertex vertices[TEXT_BATCH_COUNT * 4u];
    };

    struct SpriteVertex {
	v4_f32 position;
	v2_f32 texCoord;
	Color color;
    };

    struct SpriteData {
	SpriteVertex data[SPRITE_BATCH_COUNT * 4u];
    };

    struct DebugVertex_Solid {
	v4_f32 position;
	Color color;
    };

    struct GaussianBlurData {
	f32 intensity;
	u32 horizontal;
	v2_f32 padding;
    };

    struct EnvironmentData {
	v3_f32 ambient_light;
    };

    struct GraphicsObjects {

	GPUImage* image_white;
	GPUImage* image_aux0;
	GPUImage* image_aux1;
	Sampler* sampler_def_linear;
	Sampler* sampler_def_nearest;
	GPUBuffer* vbuffer_batch[GraphicsLimit_CommandList];
	DepthStencilState* dss_default_depth;
	RasterizerState* rs_back_culling;
	RasterizerState* rs_wireframe;
	RasterizerState* rs_wireframe_back_culling;
	RasterizerState* rs_wireframe_front_culling;
	GPUBuffer* cbuffer_camera;
	GPUBuffer* cbuffer_environment;
	RenderPass* renderpass_off;
	RenderPass* renderpass_world;
	RenderPass* renderpass_gbuffer;
	RenderPass* renderpass_ssao;
	BlendState* bs_transparent;

	// GBUFFER

	GPUImage* offscreen;
	GPUImage* gbuffer_diffuse;
	GPUImage* gbuffer_normal;
	GPUImage* gbuffer_depthstencil;
	GPUImage* gbuffer_ssao;

	// DEBUG

	Shader* vs_debug_solid_batch;
	Shader* vs_debug_mesh_wireframe;
	Shader* ps_debug_solid;
	InputLayoutState* ils_debug_solid_batch;
	GPUBuffer* cbuffer_debug_mesh;

	// TEXT

	Shader* vs_text;
	Shader* ps_text;
	GPUBuffer* ibuffer_text;
	InputLayoutState* ils_text;

	// SPRITE

	Shader* vs_sprite;
	Shader* ps_sprite;
	InputLayoutState* ils_sprite;
	GPUBuffer* ibuffer_sprite;

	// MESH

	Shader* vs_mesh_geometry;
	Shader* ps_mesh_geometry;
	Shader* ps_mesh_lighting;
	InputLayoutState* ils_mesh;
	BlendState* bs_mesh;
	GPUBuffer* cbuffer_material;
	GPUBuffer* cbuffer_mesh_instance;
	GPUBuffer* cbuffer_light_instances;

	// PP

	Shader* vs_default_postprocess;
	Shader* ps_default_postprocess;
	Shader* ps_gaussian_blur;
	GPUBuffer* cbuffer_gaussian_blur;
	GPUBuffer* cbuffer_bloom_threshold;
	Sampler* sampler_blur;
	Shader* ps_bloom_threshold;
	BlendState* bs_addition;
	Shader* ps_ssao;

	// SKY

	Shader* vs_sky;
	Shader* ps_sky;
	GPUBuffer* vbuffer_skybox;
	GPUBuffer* cbuffer_skybox;
	InputLayoutState* ils_sky;

    };

    extern GraphicsObjects gfx;
    extern u8* batch_data[GraphicsLimit_CommandList];

    extern Font font_opensans;
    extern Font font_console;

    SV_INLINE GPUBuffer* get_batch_buffer(u32 size, CommandList cmd)
    {
	// TODO: Dynamic update
	GPUBuffer*& buffer = gfx.vbuffer_batch[cmd];

	if (buffer == nullptr || graphics_buffer_info(buffer).size < size) {

	    if (buffer) {
		graphics_destroy(buffer);
		free(batch_data[cmd]);
	    }
			
	    GPUBufferDesc desc;
	    desc.bufferType = GPUBufferType_Vertex;
	    desc.usage = ResourceUsage_Default;
	    desc.CPUAccess = CPUAccess_Write;
	    desc.size = size;
	    desc.pData = nullptr;

	    graphics_buffer_create(&desc, &buffer);
	    graphics_name_set(buffer, "BatchBuffer");

	    batch_data[cmd] = (u8*)malloc(size);
	}

	return buffer;
    }

}