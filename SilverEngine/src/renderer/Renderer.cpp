#include "core.h"

#include "renderer_internal.h"
#include "graphics/graphics_internal.h"

namespace sv {

	// MAIN FUNCTIONS

	Result renderer_initialize(const InitializationRendererDesc& desc)
	{
		svCheck(renderer_postprocessing_initialize(desc));
		svCheck(renderer_sprite_initialize(desc));
		svCheck(renderer_mesh_initialize(desc));

		return Result_Success;
	}

	Result renderer_close()
	{
		svCheck(renderer_mesh_close());
		svCheck(renderer_postprocessing_close());
		svCheck(renderer_sprite_close());

		return Result_Success;
	}

	void renderer_frame_begin()
	{
		graphics_begin();
	}
	void renderer_frame_end()
	{
		graphics_commandlist_submit();
		graphics_present();
	}

	void renderer_present(GPUImage& image, const GPUImageRegion& region, GPUImageLayout layout, CommandList cmd)
	{
		GPUImage& backBuffer = graphics_swapchain_acquire_image();

		GPUImageBlit blit;
		blit.srcRegion = region;
		blit.dstRegion.size = { graphics_image_get_width(backBuffer), graphics_image_get_height(backBuffer), 1u };

		graphics_image_blit(image, backBuffer, layout, GPUImageLayout_Present, 1u, &blit, SamplerFilter_Nearest, cmd);
	}

	// OFFSCREEN

	Result renderer_offscreen_create(ui32 width, ui32 height, Offscreen& offscreen)
	{
		// Create Render Target
		GPUImageDesc imageDesc;
		imageDesc.format = SV_REND_OFFSCREEN_FORMAT;
		imageDesc.layout = GPUImageLayout_RenderTarget;
		imageDesc.dimension = 2u;
		imageDesc.width = width;
		imageDesc.height = height;
		imageDesc.depth = 1u;
		imageDesc.layers = 1u;
		imageDesc.CPUAccess = 0u;
		imageDesc.usage = ResourceUsage_Static;
		imageDesc.pData = nullptr;
		imageDesc.type = GPUImageType_RenderTarget | GPUImageType_ShaderResource;

		svCheck(graphics_image_create(&imageDesc, offscreen.renderTarget));

		// Create Depth Stencil
		imageDesc.format = Format_D24_UNORM_S8_UINT;
		imageDesc.layout = GPUImageLayout_DepthStencil;
		imageDesc.type = GPUImageType_DepthStencil;

		svCheck(graphics_image_create(&imageDesc, offscreen.depthStencil));

		return Result_Success;
	}

	Result renderer_offscreen_destroy(Offscreen& offscreen)
	{
		svCheck(graphics_destroy(offscreen.renderTarget));
		svCheck(graphics_destroy(offscreen.depthStencil));
		return Result_Success;
	}

}