#pragma once

#include "..//config.h"

#ifndef SV_DEBUG
#define NDEBUG
#endif

#ifdef SV_SRC_PATH
#define SV_SRC_PATH_W L"" SV_SRC_PATH
#endif

// std includes
#include <math.h>
#include <DirectXMath.h>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include <array>
#include <stdint.h>
#include <functional>
#include <mutex>
#include <atomic>
#include <fstream>
#include <filesystem>

using namespace DirectX;

// types
typedef uint8_t		ui8;
typedef uint16_t	ui16;
typedef uint32_t	ui32;
typedef uint64_t	ui64;

typedef int8_t		i8;
typedef int16_t		i16;
typedef int32_t		i32;
typedef int64_t		i64;

typedef wchar_t		wchar;

typedef void*		WindowHandle;

typedef int			BOOL;
#define FALSE		0
#define TRUE		1

namespace sv {

	// Result

	enum Result : ui32 {
		Result_Success,
		Result_CloseRequest,
		Result_UnknownError,
		Result_PlatformError,
		Result_NotFound,
		Result_InvalidFormat,
		Result_InvalidUsage,
	};
	
	// Exception
	
	struct Exception {
		std::string type, desc, file;
		ui32 line;
		Exception(const char* type, const char* desc, const char* file, ui32 line)
			: type(type), desc(desc), file(file), line(line) {}
	};
#define SV_THROW(type, desc) throw sv::Exception(type, desc, __FILE__, __LINE__)

}

// macros

#ifdef SV_DEBUG
#define SV_ASSERT(x) do{ if((x) == false) SV_THROW("Assertion Failed!!", #x); }while(0)
#else
#define SV_ASSERT(x) x
#endif

#define svCheck(x) do{ sv::Result __res__ = (x); if(__res__ != sv::Result_Success) { sv::log_error(#x" (error code: %u)", __res__); return __res__; } }while(0)
#define svZeroMemory(dest, size) memset(dest, 0, size)
#define SV_BIT(x) 1ULL << x 

// SilverEngine Includes

#include "utils.h"
#include "console.h"