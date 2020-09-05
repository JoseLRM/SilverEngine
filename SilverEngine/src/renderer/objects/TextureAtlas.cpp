#include "core.h"
#include "TextureAtlas.h"

namespace sv {

	Result Texture::CreateFromFile(const char* filePath, bool linearFilter, SamplerAddressMode addressMode)
	{
		svCheck(Destroy());

		// Get file data
		void* data;
		ui32 width;
		ui32 height;
		svCheck(utils_loader_image(filePath, &data, &width, &height));

		// Create Image
		{
			GPUImageDesc desc;

			desc.pData = data;
			desc.size = width * height * 4u;
			desc.format = Format_R8G8B8A8_UNORM;
			desc.layout = GPUImageLayout_ShaderResource;
			desc.type = GPUImageType_ShaderResource;
			desc.usage = ResourceUsage_Static;
			desc.CPUAccess = CPUAccess_None;
			desc.dimension = 2u;
			desc.width = width;
			desc.height = height;
			desc.depth = 1u;
			desc.layers = 1u;

			svCheck(graphics_image_create(&desc, m_Image));
		}
		// Create Sampler
		{
			SamplerDesc desc;

			desc.addressModeU = addressMode;
			desc.addressModeV = addressMode;
			desc.addressModeW = addressMode;
			desc.minFilter = linearFilter ? SamplerFilter_Linear : SamplerFilter_Nearest;
			desc.magFilter = desc.minFilter;

			svCheck(graphics_sampler_create(&desc, m_Sampler));
		}

		return Result_Success;
	}

	Result Texture::Destroy()
	{
		svCheck(graphics_destroy(m_Image));
		svCheck(graphics_destroy(m_Sampler));
		return Result_Success;
	}

}