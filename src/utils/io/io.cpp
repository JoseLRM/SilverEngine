#include "core.h"

#include "utils/io.h"

#define STBI_ASSERT(x) SV_ASSERT(x)
#define STBI_MALLOC(size) malloc(size)
#define STBI_REALLOC(ptr, size) realloc(ptr, size)
#define STBI_FREE(ptr) free(ptr)
#define STB_IMAGE_IMPLEMENTATION

#include "external/stbi_lib.h"

namespace sv {

	Result load_image(const char* filePath, void** pdata, u32* width, u32* height)
	{
		int w = 0, h = 0, bits = 0;

#ifdef SV_RES_PATH
		std::string filePathStr = SV_RES_PATH;
		filePathStr += filePath;
		void* data = stbi_load(filePathStr.c_str(), &w, &h, &bits, 4);
#else
		void* data = stbi_load(filePath, &w, &h, &bits, 4);
#endif

		* pdata = nullptr;
		*width = w;
		*height = h;

		if (!data) return Result_NotFound;
		*pdata = data;
		return Result_Success;
	}

	std::string bin_filepath(size_t hash)
	{
		std::string filePath = "bin/" + std::to_string(hash) + ".bin";
		return filePath;
	}

	Result bin_read(size_t hash, std::vector<u8>& data)
	{
		std::string filePath = bin_filepath(hash);
		return file_read_binary(filePath.c_str(), data);
	}

	Result bin_read(size_t hash, ArchiveI& archive)
	{
		std::string filePath = "bin/" + std::to_string(hash) + ".bin";
		svCheck(archive.open_file(filePath.c_str()));
		return Result_Success;
	}

	Result bin_write(size_t hash, const void* data, size_t size)
	{
		std::string filePath = bin_filepath(hash);
		return file_write_binary(filePath.c_str(), (u8*)data, size);
	}

	Result bin_write(size_t hash, ArchiveO& archive)
	{
		std::string filePath = "bin/" + std::to_string(hash) + ".bin";
		svCheck(archive.save_file(filePath.c_str()));
		return Result_Success;
	}

}