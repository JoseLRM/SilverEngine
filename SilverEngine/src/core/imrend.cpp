#include "core/renderer/renderer_internal.h"

namespace sv {

    struct ImRendVertex {
		v4_f32 position;
		v2_f32 texcoord;
		Color color;
		u32 padding;
    };

    struct ImRendScissor {
		v4_f32 bounds;
		bool additive;
    };

    struct ImRendState {
		RawList buffer;
	
		List<XMMATRIX> matrix_stack;
		List<ImRendScissor> scissor_stack;
	
		XMMATRIX current_matrix;

		struct {
			ImRendCamera current;
			bool is_custom;
			XMMATRIX view_matrix;
			XMMATRIX projection_matrix;
		} camera;
		

		struct {

			GPUBuffer* cbuffer_primitive;
			GPUBuffer* cbuffer_mesh;
	    
		} gfx;
    };

    struct ImRendGfx {

		Shader* vs_primitive;
		Shader* ps_primitive;
		Shader* vs_mesh_wireframe;
	
    };

    struct ImRendGlobalState {
		ImRendState state[GraphicsLimit_CommandList];
		ImRendGfx gfx;
    };

    static ImRendGlobalState* imrend_state = 0;
    
#define SV_IMREND() auto& state = imrend_state->state[cmd]

    enum ImRendHeader : u32 {
		ImRendHeader_PushMatrix,
		ImRendHeader_PopMatrix,
		ImRendHeader_PushScissor,
		ImRendHeader_PopScissor,

		ImRendHeader_Camera,
		ImRendHeader_CustomCamera,

		ImRendHeader_DrawCall,
    };

    enum ImRendDrawCall : u32 {
		ImRendDrawCall_Quad,
		ImRendDrawCall_Sprite,
		ImRendDrawCall_Triangle,
		ImRendDrawCall_Line,
	
		ImRendDrawCall_MeshWireframe,
	
		ImRendDrawCall_Text,
		ImRendDrawCall_TextArea,
    };

#undef COMPILE_SHADER
#undef COMPILE_VS
#undef COMPILE_PS
#undef COMPILE_VS_
#undef COMPILE_PS_
    
#define COMPILE_SHADER(type, shader, path, alwais_compile) SV_CHECK(graphics_shader_compile_fastbin_from_file(path, type, &shader, "$system/shaders/immediate_mode/" path, alwais_compile))
#define COMPILE_VS(shader, path) COMPILE_SHADER(ShaderType_Vertex, shader, path, false)
#define COMPILE_PS(shader, path) COMPILE_SHADER(ShaderType_Pixel, shader, path, false)

#define COMPILE_VS_(shader, path) COMPILE_SHADER(ShaderType_Vertex, shader, path, true)
#define COMPILE_PS_(shader, path) COMPILE_SHADER(ShaderType_Pixel, shader, path, true)

    bool _imrend_initialize()
    {
		imrend_state = SV_ALLOCATE_STRUCT(ImRendGlobalState, "Renderer");
	
		auto& imgfx = imrend_state->gfx;
	
		COMPILE_VS(imgfx.vs_primitive, "primitive_shader.hlsl");
		COMPILE_PS(imgfx.ps_primitive, "primitive_shader.hlsl");
		COMPILE_VS(imgfx.vs_mesh_wireframe, "mesh_wireframe.hlsl");

		foreach(i, GraphicsLimit_CommandList) {

			auto& gfx = imrend_state->state[i].gfx;

			// Buffers
			{
				GPUBufferDesc desc;

				desc.buffer_type = GPUBufferType_Constant;
				desc.usage = ResourceUsage_Dynamic;
				desc.cpu_access = CPUAccess_Write;
				desc.size = sizeof(ImRendVertex) * 4u;
				desc.data = nullptr;

				SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_primitive));

				desc.buffer_type = GPUBufferType_Constant;
				desc.usage = ResourceUsage_Dynamic;
				desc.cpu_access = CPUAccess_Write;
				desc.size = sizeof(XMMATRIX) + sizeof(v4_f32);
				desc.data = nullptr;

				SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_mesh));
			}
		}

		return true;
    }

    void _imrend_close()
    {
		if (imrend_state) {
	    
			auto& imgfx = imrend_state->gfx;

			// Free gfx objects
			{
				graphics_destroy_struct(&imgfx, sizeof(imgfx));

				foreach(i, GraphicsLimit_CommandList) {

					auto& state = imrend_state->state[i];
	    
					graphics_destroy_struct(&state.gfx, sizeof(state.gfx));
				}
			}

			SV_FREE_STRUCT(imrend_state);
		}
    }

    template<typename T>
    SV_AUX void imrend_write(ImRendState& state, const T& t)
    {
		state.buffer.write_back(&t, sizeof(T));
    }

    SV_AUX void imrend_write_text(ImRendState& state, const char* text)
    {
		state.buffer.write_back(text, string_size(string_validate(text)) + 1u);
    }

    template<typename T>
    SV_AUX T imrend_read(u8*& it)
    {
		T t;
		memcpy(&t, it, sizeof(T));
		it += sizeof(T);
		return t;
    }

    SV_AUX void update_current_matrix(ImRendState& state)
    {
		XMMATRIX matrix = XMMatrixIdentity();

		for (const XMMATRIX& m : state.matrix_stack) {

			matrix = XMMatrixMultiply(matrix, m);
		}

		XMMATRIX vpm;

		if (state.camera.is_custom) {

			vpm = state.camera.view_matrix * state.camera.projection_matrix;
		}
		else {
			
			switch (state.camera.current)
			{
			case ImRendCamera_Normal:
				vpm = XMMatrixScaling(2.f, 2.f, 1.f) * XMMatrixTranslation(-1.f, -1.f, 0.f);
				break;

#if SV_EDITOR
			case ImRendCamera_Editor:
				vpm = dev.camera.view_projection_matrix;
				break;
#endif

			case ImRendCamera_Clip:
			default:
				vpm = XMMatrixIdentity();
	    
			}
		}

		state.current_matrix = matrix * vpm;
    }

    SV_AUX void update_current_scissor(ImRendState& state, CommandList cmd)
    {
		v4_f32 s0 = { 0.5f, 0.5f, 1.f, 1.f };

		u32 begin_index = 0u;

		if (state.scissor_stack.size()) {
	    
			for (i32 i = (i32)state.scissor_stack.size() - 1u; i >= 0; --i) {

				if (!state.scissor_stack[i].additive) {
					begin_index = i;
					break;
				}
			}

			for (u32 i = begin_index; i < (u32)state.scissor_stack.size(); ++i) {

				const v4_f32& s1 = state.scissor_stack[i].bounds;

				f32 min0 = s0.x - s0.z * 0.5f;
				f32 max0 = s0.x + s0.z * 0.5f;
	    
				f32 min1 = s1.x - s1.z * 0.5f;
				f32 max1 = s1.x + s1.z * 0.5f;
	    
				f32 min = SV_MAX(min0, min1);
				f32 max = SV_MIN(max0, max1);
		
				if (min >= max) {
					s0 = {};
					break;
				}
	    
				s0.z = max - min;
				s0.x = min + s0.z * 0.5f;
	    
				min0 = s0.y - s0.w * 0.5f;
				max0 = s0.y + s0.w * 0.5f;
	    
				min1 = s1.y - s1.w * 0.5f;
				max1 = s1.y + s1.w * 0.5f;

				min = SV_MAX(min0, min1);
				max = SV_MIN(max0, max1);

				if (min >= max) {
					s0 = {};
					break;
				}

				s0.w = max - min;
				s0.y = min + s0.w * 0.5f;
			}
		}

		const GPUImageInfo& info = graphics_image_info(renderer->gfx.offscreen);
	
		Scissor s;
		s.width = u32(s0.z * f32(info.width));
		s.height = u32(s0.w * f32(info.height));
		s.x = u32(s0.x * f32(info.width) - s0.z * f32(info.width) * 0.5f);
		s.y = u32(s0.y * f32(info.height) - s0.w * f32(info.height) * 0.5f);

		graphics_scissor_set(s, 0u, cmd);
    }
    
    void imrend_begin_batch(CommandList cmd)
    {
		SV_IMREND();

		state.buffer.reset();
    }
    
    void imrend_flush(CommandList cmd)
    {
		SV_IMREND();
		auto& imgfx = imrend_state->gfx;

		graphics_event_begin("Immediate Rendering", cmd);

		auto& gfx = renderer->gfx;

		graphics_viewport_set(gfx.offscreen, 0u, cmd);
		graphics_scissor_set(gfx.offscreen, 0u, cmd);
	
		GPUImage* att[1];
		att[0] = gfx.offscreen;

		graphics_renderpass_begin(gfx.renderpass_off, att, cmd);

		state.current_matrix = XMMatrixIdentity();
		state.matrix_stack.reset();
		state.scissor_stack.reset();
		state.camera.current = ImRendCamera_Clip;
		state.camera.is_custom = false;

		u8* it = (u8*)state.buffer.data();
		u8* end = (u8*)state.buffer.data() + state.buffer.size();

		while (it != end)
		{
			ImRendHeader header = imrend_read<ImRendHeader>(it);
	    
			switch (header) {

			case ImRendHeader_PushMatrix:
			{
				XMMATRIX m = imrend_read<XMMATRIX>(it);
				state.matrix_stack.push_back(m);
				update_current_matrix(state);
			}
			break;
		
			case ImRendHeader_PopMatrix:
			{
				state.matrix_stack.pop_back();
				update_current_matrix(state);
			}
			break;

			case ImRendHeader_PushScissor:
			{
				ImRendScissor s;
				s.bounds = imrend_read<v4_f32>(it);
				s.additive = imrend_read<bool>(it);
		
				state.scissor_stack.push_back(s);
				update_current_scissor(state, cmd);
			}
			break;
	    
			case ImRendHeader_PopScissor:
			{
				state.scissor_stack.pop_back();
				update_current_scissor(state, cmd);
			}
			break;

			case ImRendHeader_Camera:
			{
				state.camera.current = imrend_read<ImRendCamera>(it);
				state.camera.is_custom = false;
				update_current_matrix(state);
			}
			break;

			case ImRendHeader_CustomCamera:
			{
				state.camera.view_matrix = imrend_read<XMMATRIX>(it);
				state.camera.projection_matrix = imrend_read<XMMATRIX>(it);
				state.camera.is_custom = true;
				update_current_matrix(state);
			}
			break;
		
			case ImRendHeader_DrawCall:
			{
				ImRendDrawCall draw_call = imrend_read<ImRendDrawCall>(it);

				switch (draw_call) {

				case ImRendDrawCall_Quad:
				case ImRendDrawCall_Sprite:
				case ImRendDrawCall_Triangle:
				case ImRendDrawCall_Line:
				{
					graphics_constant_buffer_bind(state.gfx.cbuffer_primitive, 0u, ShaderType_Vertex, cmd);

					graphics_blendstate_bind(gfx.bs_transparent, cmd);
					graphics_depthstencilstate_unbind(cmd);
					graphics_inputlayoutstate_unbind(cmd);
					graphics_rasterizerstate_unbind(cmd);

					graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);
		    
					graphics_shader_bind(imgfx.vs_primitive, cmd);
					graphics_shader_bind(imgfx.ps_primitive, cmd);

					if (draw_call == ImRendDrawCall_Quad || draw_call == ImRendDrawCall_Sprite) {

						graphics_topology_set(GraphicsTopology_TriangleStrip, cmd);

						GPUImageLayout layout = GPUImageLayout_ShaderResource;
						v3_f32 position = imrend_read<v3_f32>(it);
						v2_f32 size = imrend_read<v2_f32>(it);
						Color color = imrend_read<Color>(it);
						GPUImage* image = gfx.image_white;
						v4_f32 tc = { 0.f, 0.f, 1.f, 1.f };

						if (draw_call == ImRendDrawCall_Sprite) {
							image = imrend_read<GPUImage*>(it);
							layout = imrend_read<GPUImageLayout>(it);
							tc = imrend_read<v4_f32>(it);

							if (image == NULL)
								image = gfx.image_white;
							
							if (layout != GPUImageLayout_ShaderResource && layout != GPUImageLayout_DepthStencilReadOnly) {
								
								// TEMP
								graphics_renderpass_end(cmd);
								
								GPUBarrier barrier = GPUBarrier::Image(image, layout, (layout == GPUImageLayout_DepthStencil) ? GPUImageLayout_DepthStencilReadOnly : GPUImageLayout_ShaderResource);
								graphics_barrier(&barrier, 1u, cmd);
								
								graphics_renderpass_begin(gfx.renderpass_off, att, cmd);
							}
						}

						XMMATRIX m = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixTranslation(position.x, position.y, position.z);

						m *= state.current_matrix;

						XMVECTOR v0 = XMVector4Transform(XMVectorSet(-0.5f, 0.5f, 0.f, 1.f), m);
						XMVECTOR v1 = XMVector4Transform(XMVectorSet(0.5f, 0.5f, 0.f, 1.f), m);
						XMVECTOR v2 = XMVector4Transform(XMVectorSet(-0.5f, -0.5f, 0.f, 1.f), m);
						XMVECTOR v3 = XMVector4Transform(XMVectorSet(0.5f, -0.5f, 0.f, 1.f), m);
		    
						ImRendVertex vertices[4u];
						vertices[0u] = { v4_f32(v0), v2_f32{tc.x, tc.y}, color };
						vertices[1u] = { v4_f32(v1), v2_f32{tc.z, tc.y}, color };
						vertices[2u] = { v4_f32(v2), v2_f32{tc.x, tc.w}, color };
						vertices[3u] = { v4_f32(v3), v2_f32{tc.z, tc.w}, color };

						graphics_shader_resource_bind(image, 0u, ShaderType_Pixel, cmd);

						graphics_buffer_update(state.gfx.cbuffer_primitive, GPUBufferState_Constant, vertices, sizeof(ImRendVertex) * 4u, 0u, cmd);

						graphics_draw(4u, 1u, 0u, 0u, cmd);

						if (draw_call == ImRendDrawCall_Sprite && layout != GPUImageLayout_ShaderResource && layout != GPUImageLayout_DepthStencilReadOnly) {
							GPUBarrier barrier = GPUBarrier::Image(image, (layout == GPUImageLayout_DepthStencil) ? GPUImageLayout_DepthStencilReadOnly : GPUImageLayout_ShaderResource, layout);
							// TEMP
							graphics_renderpass_end(cmd);
							graphics_barrier(&barrier, 1u, cmd);
							graphics_renderpass_begin(gfx.renderpass_off, att, cmd);
						}
					}
					else if (draw_call == ImRendDrawCall_Triangle) {

						graphics_topology_set(GraphicsTopology_TriangleStrip, cmd);

						v3_f32 p0 = imrend_read<v3_f32>(it);
						v3_f32 p1 = imrend_read<v3_f32>(it);
						v3_f32 p2 = imrend_read<v3_f32>(it);
						Color color = imrend_read<Color>(it);

						XMMATRIX m = state.current_matrix;

						XMVECTOR v0 = XMVector4Transform(vec3_to_dx(p0, 1.f), m);
						XMVECTOR v1 = XMVector4Transform(vec3_to_dx(p1, 1.f), m);
						XMVECTOR v2 = XMVector4Transform(vec3_to_dx(p2, 1.f), m);
		    
						ImRendVertex vertices[3u];
						vertices[0u] = { v4_f32(v0), v2_f32{}, color };
						vertices[1u] = { v4_f32(v1), v2_f32{}, color };
						vertices[2u] = { v4_f32(v2), v2_f32{}, color };

						graphics_shader_resource_bind(gfx.image_white, 0u, ShaderType_Pixel, cmd);

						graphics_buffer_update(state.gfx.cbuffer_primitive, GPUBufferState_Constant, vertices, sizeof(ImRendVertex) * 3u, 0u, cmd);

						graphics_draw(3u, 1u, 0u, 0u, cmd);
					}
					else if (draw_call == ImRendDrawCall_Line) {

						graphics_topology_set(GraphicsTopology_Lines, cmd);

						v3_f32 p0 = imrend_read<v3_f32>(it);
						v3_f32 p1 = imrend_read<v3_f32>(it);
						Color color = imrend_read<Color>(it);

						XMMATRIX m = state.current_matrix;

						XMVECTOR v0 = XMVector4Transform(vec3_to_dx(p0, 1.f), m);
						XMVECTOR v1 = XMVector4Transform(vec3_to_dx(p1, 1.f), m);
		    
						ImRendVertex vertices[2u];
						vertices[0u] = { v4_f32(v0), v2_f32{}, color };
						vertices[1u] = { v4_f32(v1), v2_f32{}, color };

						graphics_shader_resource_bind(gfx.image_white, 0u, ShaderType_Pixel, cmd);

						graphics_buffer_update(state.gfx.cbuffer_primitive, GPUBufferState_Constant, vertices, sizeof(ImRendVertex) * 2u, 0u, cmd);

						graphics_draw(3u, 1u, 0u, 0u, cmd);
					}
		    
				}break;

				case ImRendDrawCall_MeshWireframe:
				{
					graphics_shader_resource_bind(gfx.image_white, 0u, ShaderType_Pixel, cmd);
					graphics_inputlayoutstate_bind(gfx.ils_mesh, cmd);
					graphics_shader_bind(imgfx.vs_mesh_wireframe, cmd);
					graphics_shader_bind(imgfx.ps_primitive, cmd);
					graphics_topology_set(GraphicsTopology_Triangles, cmd);
					graphics_constant_buffer_bind(state.gfx.cbuffer_mesh, 0u, ShaderType_Vertex, cmd);
					graphics_rasterizerstate_bind(gfx.rs_wireframe, cmd);
					graphics_blendstate_bind(gfx.bs_transparent, cmd);

					Mesh* mesh = imrend_read<Mesh*>(it);
					Color color = imrend_read<Color>(it);

					graphics_vertex_buffer_bind(mesh->vbuffer, 0u, 0u, cmd);
					graphics_index_buffer_bind(mesh->ibuffer, 0u, cmd);

					struct {
						XMMATRIX matrix;
						v4_f32 color;
					} data;

					data.matrix = state.current_matrix;
					data.color = color_to_vec4(color);

					graphics_buffer_update(state.gfx.cbuffer_mesh, GPUBufferState_Constant, &data, sizeof(data), 0u, cmd);
					graphics_draw_indexed((u32)mesh->indices.size(), 1u, 0u, 0u, 0u, cmd);
				}
				break;
		
				case ImRendDrawCall_TextArea:
				{
					DrawTextAreaDesc desc;
					desc.text = (const char*)it;
					it += string_size(desc.text) + 1u;
					desc.max_width = imrend_read<f32>(it);
					desc.max_lines = imrend_read<u32>(it);
					desc.font_size = imrend_read<f32>(it);
					desc.aspect = imrend_read<f32>(it);
					desc.alignment = imrend_read<TextAlignment>(it);
					desc.line_offset = imrend_read<u32>(it);
					desc.bottom_top = imrend_read<bool>(it);
					desc.font = imrend_read<Font*>(it);
					desc.color = imrend_read<Color>(it);
					desc.transform_matrix = state.current_matrix;

					graphics_renderpass_end(cmd);
					draw_text_area(desc, cmd);
					graphics_renderpass_begin(gfx.renderpass_off, att, cmd);
		    
				}break;

				case ImRendDrawCall_Text:
				{
					DrawTextDesc desc;
					desc.text = (const char*)it;
					it += string_size(desc.text) + 1u;
					desc.max_width = imrend_read<f32>(it);
					desc.max_lines = imrend_read<u32>(it);
					desc.font_size = imrend_read<f32>(it);
					desc.aspect = imrend_read<f32>(it);
					desc.alignment = imrend_read<TextAlignment>(it);
					desc.font = imrend_read<Font*>(it);
					desc.color = imrend_read<Color>(it);
					desc.transform_matrix = state.current_matrix;
					
					graphics_renderpass_end(cmd);
					draw_text(desc, cmd);
					graphics_renderpass_begin(gfx.renderpass_off, att, cmd);
		    
				}break;
		    
				}
			}
			break;
		
		
		
			}
		}

		SV_ASSERT(state.matrix_stack.empty());
		SV_ASSERT(state.scissor_stack.empty());

		graphics_renderpass_end(cmd);

		graphics_event_end(cmd);
    }

    void imrend_push_matrix(const XMMATRIX& matrix, CommandList cmd)
    {
		SV_IMREND();

		imrend_write(state, ImRendHeader_PushMatrix);
		imrend_write(state, matrix);
    }

    void imrend_pop_matrix(CommandList cmd)
    {
		SV_IMREND();
	
		imrend_write(state, ImRendHeader_PopMatrix);
    }

    void imrend_push_scissor(f32 x, f32 y, f32 width, f32 height, bool additive, CommandList cmd)
    {
		SV_IMREND();
	
		imrend_write(state, ImRendHeader_PushScissor);
		imrend_write(state, v4_f32(x, y, width, height));
		imrend_write(state, additive);
    }
    
    void imrend_pop_scissor(CommandList cmd)
    {
		SV_IMREND();
	
		imrend_write(state, ImRendHeader_PopScissor);
    }

    void imrend_camera(ImRendCamera camera, CommandList cmd)
    {
		SV_IMREND();
	
		imrend_write(state, ImRendHeader_Camera);
		imrend_write(state, camera);
    }

	void imrend_camera(const XMMATRIX& view_matrix, const XMMATRIX& projection_matrix, CommandList cmd)
	{
		SV_IMREND();
	
		imrend_write(state, ImRendHeader_CustomCamera);
		imrend_write(state, view_matrix);
		imrend_write(state, projection_matrix);
	}

    void imrend_draw_quad(const v3_f32& position, const v2_f32& size, Color color, CommandList cmd)
    {
		SV_IMREND();
	
		imrend_write(state, ImRendHeader_DrawCall);
		imrend_write(state, ImRendDrawCall_Quad);

		imrend_write(state, position);
		imrend_write(state, size);
		imrend_write(state, color);
    }

    void imrend_draw_line(const v3_f32& p0, const v3_f32& p1, Color color, CommandList cmd)
    {
		SV_IMREND();
	
		imrend_write(state, ImRendHeader_DrawCall);
		imrend_write(state, ImRendDrawCall_Line);

		imrend_write(state, p0);
		imrend_write(state, p1);
		imrend_write(state, color);
    }

    void imrend_draw_triangle(const v3_f32& p0, const v3_f32& p1, const v3_f32& p2, Color color, CommandList cmd)
    {
		SV_IMREND();

		imrend_write(state, ImRendHeader_DrawCall);
		imrend_write(state, ImRendDrawCall_Triangle);
	
		imrend_write(state, p0);
		imrend_write(state, p1);
		imrend_write(state, p2);
		imrend_write(state, color);
    }

    void imrend_draw_sprite(const v3_f32& position, const v2_f32& size, Color color, GPUImage* image, GPUImageLayout layout, const v4_f32& texcoord, CommandList cmd)
    {
		SV_IMREND();

		imrend_write(state, ImRendHeader_DrawCall);
		imrend_write(state, ImRendDrawCall_Sprite);

		imrend_write(state, position);
		imrend_write(state, size);
		imrend_write(state, color);
		imrend_write(state, image);
		imrend_write(state, layout);
		imrend_write(state, texcoord);
    }

    void imrend_draw_mesh_wireframe(Mesh* mesh, Color color, CommandList cmd)
    {
		if (mesh == NULL || mesh->vbuffer == NULL || mesh->ibuffer == NULL)
			return;
		
		SV_IMREND();

		imrend_write(state, ImRendHeader_DrawCall);
		imrend_write(state, ImRendDrawCall_MeshWireframe);
	
		imrend_write(state, mesh);
		imrend_write(state, color);
    }

	SV_API void imrend_draw_text(const char* text, f32 font_size, f32 aspect, Font* font, Color color, CommandList cmd)
	{
		SV_IMREND();

		imrend_write(state, ImRendHeader_DrawCall);
		imrend_write(state, ImRendDrawCall_Text);

		imrend_write_text(state, text);
		imrend_write(state, f32_max);
		imrend_write(state, u32_max);
		imrend_write(state, font_size);
		imrend_write(state, aspect);
		imrend_write(state, TextAlignment_Center);
		imrend_write(state, font);
		imrend_write(state, color);
	}
	
	SV_API void imrend_draw_text_bounds(const char* text, f32 max_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, Font* font, Color color, CommandList cmd)
	{
		SV_IMREND();

		imrend_write(state, ImRendHeader_DrawCall);
		imrend_write(state, ImRendDrawCall_Text);

		imrend_write_text(state, text);
		imrend_write(state, max_width);
		imrend_write(state, max_lines);
		imrend_write(state, font_size);
		imrend_write(state, aspect);
		imrend_write(state, alignment);
		imrend_write(state, font);
		imrend_write(state, color);
	}

	void imrend_draw_text_area(const char* text, size_t text_size, f32 max_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, u32 line_offset, bool bottom_top, Font* font, Color color, CommandList cmd)
    {
		SV_IMREND();
		
		imrend_write(state, ImRendHeader_DrawCall);
		imrend_write(state, ImRendDrawCall_TextArea);

		state.buffer.write_back(text, text_size);
		imrend_write(state, '\0');
		
		imrend_write(state, max_width);
		imrend_write(state, max_lines);
		imrend_write(state, font_size);
		imrend_write(state, aspect);
		imrend_write(state, alignment);
		imrend_write(state, line_offset);
		imrend_write(state, bottom_top);
		imrend_write(state, font);
		imrend_write(state, color);
    }

	void imrend_draw_cube_wireframe(Color color, CommandList cmd)
	{
		v3_f32 v0 = { -0.5f,  0.5f,  0.5f };
		v3_f32 v1 = {  0.5f,  0.5f,  0.5f };
		v3_f32 v2 = { -0.5f, -0.5f,  0.5f };
		v3_f32 v3 = {  0.5f, -0.5f,  0.5f };
		v3_f32 v4 = { -0.5f,  0.5f, -0.5f };
		v3_f32 v5 = {  0.5f,  0.5f, -0.5f };
		v3_f32 v6 = { -0.5f, -0.5f, -0.5f };
		v3_f32 v7 = {  0.5f, -0.5f, -0.5f };
			
		imrend_draw_line(v0, v1, color, cmd);
		imrend_draw_line(v1, v3, color, cmd);
		imrend_draw_line(v3, v2, color, cmd);
		imrend_draw_line(v0, v2, color, cmd);

		imrend_draw_line(v4, v5, color, cmd);
		imrend_draw_line(v5, v7, color, cmd);
		imrend_draw_line(v7, v6, color, cmd);
		imrend_draw_line(v4, v6, color, cmd);

		imrend_draw_line(v0, v4, color, cmd);
		imrend_draw_line(v1, v5, color, cmd);
		imrend_draw_line(v2, v6, color, cmd);
		imrend_draw_line(v3, v7, color, cmd);
	}

	void imrend_draw_sphere_wireframe(u32 vertical_segments, u32 horizontal_segments, Color color, CommandList cmd)
	{
		foreach(x, horizontal_segments) {

			f32 a0 = (x + 1) * PI / (horizontal_segments + 1u);
			f32 a1 = (x + 2) * PI / (horizontal_segments + 1u);
			
			foreach(y, vertical_segments) {

				f32 b0 = (y + 0) * TAU / vertical_segments;
				f32 b1 = (y + 1) * TAU / vertical_segments;

				f32 sin_a0 = sinf(a0);
				f32 sin_a1 = sinf(a1);
				f32 sin_b0 = sinf(b0);
				f32 sin_b1 = sinf(b1);
				f32 cos_a0 = cosf(a0);
				f32 cos_a1 = cosf(a1);
				f32 cos_b0 = cosf(b0);
				f32 cos_b1 = cosf(b1);

				f32 x0 = sin_a0 * cos_b0 * 0.5f;
				f32 y0 = cos_a0 * 0.5f;
				f32 z0 = sin_a0 * sin_b0 * 0.5f;

				f32 x1 = sin_a1 * cos_b0 * 0.5f;
				f32 y1 = cos_a1 * 0.5f;
				f32 z1 = sin_a1 * sin_b0 * 0.5f;

				f32 x2 = sin_a0 * cos_b1 * 0.5f;
				f32 y2 = cos_a0 * 0.5f;
				f32 z2 = sin_a0 * sin_b1 * 0.5f;

				f32 x3 = sin_a1 * cos_b1 * 0.5f;
				f32 y3 = cos_a1 * 0.5f;
				f32 z3 = sin_a1 * sin_b1 * 0.5f;

				imrend_draw_line({ x0, y0, z0 }, { x1, y1, z1 }, color, cmd);
				imrend_draw_line({ x0, y0, z0 }, { x2, y2, z2 }, color, cmd);
				imrend_draw_line({ x0, y0, z0 }, { x3, y3, z3 }, color, cmd);
			}
		}
	}
    
    void imrend_draw_orthographic_grip(const v2_f32& position, const v2_f32& offset, const v2_f32& size, const v2_f32& gridSize, Color color, CommandList cmd)
    {
		v2_f32 begin = position - offset - size / 2.f;
		v2_f32 end = begin + size;

		for (f32 y = i32(begin.y / gridSize.y) * gridSize.y; y < end.y; y += gridSize.y) {
			imrend_draw_line({ begin.x + offset.x, y + offset.y, 0.f }, { end.x + offset.x, y + offset.y, 0.f }, color, cmd);
		}
		for (f32 x = i32(begin.x / gridSize.x) * gridSize.x; x < end.x; x += gridSize.x) {
			imrend_draw_line({ x + offset.x, begin.y + offset.y, 0.f }, { x + offset.x, end.y + offset.y, 0.f }, color, cmd);
		}
    }
}
