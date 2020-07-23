#pragma once

#include "core.h"
#include "GraphicsDesc.h"

enum SV_GFX_GRAPHICS_PIPELINE_STATE : ui64 {
	SV_GFX_GRAPHICS_PIPELINE_STATE_NONE = 0,
	SV_GFX_GRAPHICS_PIPELINE_STATE_VERTEX_BUFFER		= SV_BIT(0),
	SV_GFX_GRAPHICS_PIPELINE_STATE_INDEX_BUFFER			= SV_BIT(1),

	SV_GFX_GRAPHICS_PIPELINE_STATE_CONSTANT_BUFFER		= SV_BIT(2),
	SV_GFX_GRAPHICS_PIPELINE_STATE_CONSTANT_BUFFER_VS	= SV_BIT(3),
	SV_GFX_GRAPHICS_PIPELINE_STATE_CONSTANT_BUFFER_PS	= SV_BIT(4),
	SV_GFX_GRAPHICS_PIPELINE_STATE_CONSTANT_BUFFER_GS	= SV_BIT(5),
	SV_GFX_GRAPHICS_PIPELINE_STATE_CONSTANT_BUFFER_HS	= SV_BIT(6),
	SV_GFX_GRAPHICS_PIPELINE_STATE_CONSTANT_BUFFER_DS	= SV_BIT(7),
	SV_GFX_GRAPHICS_PIPELINE_STATE_CONSTANT_BUFFER_MS	= SV_BIT(8),
	SV_GFX_GRAPHICS_PIPELINE_STATE_CONSTANT_BUFFER_TS	= SV_BIT(9),

	SV_GFX_GRAPHICS_PIPELINE_STATE_IMAGE				= SV_BIT(10),
	SV_GFX_GRAPHICS_PIPELINE_STATE_IMAGE_VS				= SV_BIT(11),
	SV_GFX_GRAPHICS_PIPELINE_STATE_IMAGE_PS				= SV_BIT(12),
	SV_GFX_GRAPHICS_PIPELINE_STATE_IMAGE_GS				= SV_BIT(13),
	SV_GFX_GRAPHICS_PIPELINE_STATE_IMAGE_HS				= SV_BIT(14),
	SV_GFX_GRAPHICS_PIPELINE_STATE_IMAGE_DS				= SV_BIT(15),
	SV_GFX_GRAPHICS_PIPELINE_STATE_IMAGE_MS				= SV_BIT(16),
	SV_GFX_GRAPHICS_PIPELINE_STATE_IMAGE_TS				= SV_BIT(17),

	SV_GFX_GRAPHICS_PIPELINE_STATE_SAMPLER				= SV_BIT(18),
	SV_GFX_GRAPHICS_PIPELINE_STATE_SAMPLER_VS			= SV_BIT(19),
	SV_GFX_GRAPHICS_PIPELINE_STATE_SAMPLER_PS			= SV_BIT(20),
	SV_GFX_GRAPHICS_PIPELINE_STATE_SAMPLER_GS			= SV_BIT(21),
	SV_GFX_GRAPHICS_PIPELINE_STATE_SAMPLER_HS			= SV_BIT(22),
	SV_GFX_GRAPHICS_PIPELINE_STATE_SAMPLER_DS			= SV_BIT(23),
	SV_GFX_GRAPHICS_PIPELINE_STATE_SAMPLER_MS			= SV_BIT(24),
	SV_GFX_GRAPHICS_PIPELINE_STATE_SAMPLER_TS			= SV_BIT(25),

	SV_GFX_GRAPHICS_PIPELINE_STATE_RENDER_PASS			= SV_BIT(26),
	SV_GFX_GRAPHICS_PIPELINE_STATE_PIPELINE				= SV_BIT(27),

	SV_GFX_GRAPHICS_PIPELINE_STATE_VIEWPORT				= SV_BIT(28),
	SV_GFX_GRAPHICS_PIPELINE_STATE_SCISSOR				= SV_BIT(29),
	SV_GFX_GRAPHICS_PIPELINE_STATE_CLEAR_COLOR			= SV_BIT(30),
	SV_GFX_GRAPHICS_PIPELINE_STATE_CLEAR_DEPTH_STENCIL	= SV_BIT(31),
};

namespace SV {
	namespace _internal {

		typedef GfxFlags64	GraphicsPipelineStateFlags;

		struct GraphicsPipelineState {
			Buffer_internal*				vertexBuffers[SV_GFX_VERTEX_BUFFER_COUNT];
			ui32							vertexBufferOffsets[SV_GFX_VERTEX_BUFFER_COUNT];
			ui32							vertexBufferStrides[SV_GFX_VERTEX_BUFFER_COUNT];
			ui32							vertexBuffersCount;

			Buffer_internal*				indexBuffer;
			ui32							indexBufferOffset;
			
			Buffer_internal*				constantBuffers[SV_GFX_CONSTANT_BUFFER_COUNT][SV_GFX_SHADER_TYPE_GFX_COUNT];
			ui32							constantBuffersCount[SV_GFX_SHADER_TYPE_GFX_COUNT];
			
			Image_internal*					images[SV_GFX_IMAGE_COUNT][SV_GFX_SHADER_TYPE_GFX_COUNT];
			ui32							imagesCount[SV_GFX_SHADER_TYPE_GFX_COUNT];
			
			Sampler_internal*				sampers[SV_GFX_SAMPLER_COUNT][SV_GFX_SHADER_TYPE_GFX_COUNT];
			ui32							samplersCount[SV_GFX_SHADER_TYPE_GFX_COUNT];
			
			RenderPass_internal*			renderPass;
			Image_internal*					attachments[SV_GFX_ATTACHMENTS_COUNT];
			GraphicsPipeline_internal*		pipeline;

			SV_GFX_VIEWPORT					viewports[SV_GFX_VIEWPORT_COUNT];
			ui32							viewportsCount;
			SV_GFX_SCISSOR					scissors[SV_GFX_SCISSOR_COUNT];
			ui32							scissorsCount;
			
			SV::vec4						clearColors[SV_GFX_ATTACHMENTS_COUNT];
			std::pair<float, ui32>			clearDepthStencil;
			
			GraphicsPipelineStateFlags	flags;
		};
		struct ComputePipelineState {

		};

		struct PipelineState {
			GraphicsPipelineState	graphics[SV_GFX_COMMAND_LIST_COUNT];
			ComputePipelineState	compute	[SV_GFX_COMMAND_LIST_COUNT];
			SV_GFX_PIPELINE_MODE	mode	[SV_GFX_COMMAND_LIST_COUNT];
		};

	}
}