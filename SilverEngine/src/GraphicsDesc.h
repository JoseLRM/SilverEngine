#pragma once

enum SV_GFX_API {
	SV_GFX_API_INVALID,
	SV_GFX_API_DX11,
};

enum SV_GFX_USAGE : ui8 {
	SV_GFX_USAGE_DEFAULT,
	SV_GFX_USAGE_STATIC,
	SV_GFX_USAGE_DYNAMIC,
	SV_GFX_USAGE_STAGING
};

enum SV_GFX_FORMAT
{
	SV_GFX_FORMAT_UNKNOWN = 0,
	SV_GFX_FORMAT_R32G32B32A32_TYPELESS = 1,
	SV_GFX_FORMAT_R32G32B32A32_FLOAT = 2,
	SV_GFX_FORMAT_R32G32B32A32_UINT = 3,
	SV_GFX_FORMAT_R32G32B32A32_SINT = 4,
	SV_GFX_FORMAT_R32G32B32_TYPELESS = 5,
	SV_GFX_FORMAT_R32G32B32_FLOAT = 6,
	SV_GFX_FORMAT_R32G32B32_UINT = 7,
	SV_GFX_FORMAT_R32G32B32_SINT = 8,
	SV_GFX_FORMAT_R16G16B16A16_TYPELESS = 9,
	SV_GFX_FORMAT_R16G16B16A16_FLOAT = 10,
	SV_GFX_FORMAT_R16G16B16A16_UNORM = 11,
	SV_GFX_FORMAT_R16G16B16A16_UINT = 12,
	SV_GFX_FORMAT_R16G16B16A16_SNORM = 13,
	SV_GFX_FORMAT_R16G16B16A16_SINT = 14,
	SV_GFX_FORMAT_R32G32_TYPELESS = 15,
	SV_GFX_FORMAT_R32G32_FLOAT = 16,
	SV_GFX_FORMAT_R32G32_UINT = 17,
	SV_GFX_FORMAT_R32G32_SINT = 18,
	SV_GFX_FORMAT_R32G8X24_TYPELESS = 19,
	SV_GFX_FORMAT_D32_FLOAT_S8X24_UINT = 20,
	SV_GFX_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
	SV_GFX_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
	SV_GFX_FORMAT_R10G10B10A2_TYPELESS = 23,
	SV_GFX_FORMAT_R10G10B10A2_UNORM = 24,
	SV_GFX_FORMAT_R10G10B10A2_UINT = 25,
	SV_GFX_FORMAT_R11G11B10_FLOAT = 26,
	SV_GFX_FORMAT_R8G8B8A8_TYPELESS = 27,
	SV_GFX_FORMAT_R8G8B8A8_UNORM = 28,
	SV_GFX_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
	SV_GFX_FORMAT_R8G8B8A8_UINT = 30,
	SV_GFX_FORMAT_R8G8B8A8_SNORM = 31,
	SV_GFX_FORMAT_R8G8B8A8_SINT = 32,
	SV_GFX_FORMAT_R16G16_TYPELESS = 33,
	SV_GFX_FORMAT_R16G16_FLOAT = 34,
	SV_GFX_FORMAT_R16G16_UNORM = 35,
	SV_GFX_FORMAT_R16G16_UINT = 36,
	SV_GFX_FORMAT_R16G16_SNORM = 37,
	SV_GFX_FORMAT_R16G16_SINT = 38,
	SV_GFX_FORMAT_R32_TYPELESS = 39,
	SV_GFX_FORMAT_D32_FLOAT = 40,
	SV_GFX_FORMAT_R32_FLOAT = 41,
	SV_GFX_FORMAT_R32_UINT = 42,
	SV_GFX_FORMAT_R32_SINT = 43,
	SV_GFX_FORMAT_R24G8_TYPELESS = 44,
	SV_GFX_FORMAT_D24_UNORM_S8_UINT = 45,
	SV_GFX_FORMAT_R24_UNORM_X8_TYPELESS = 46,
	SV_GFX_FORMAT_X24_TYPELESS_G8_UINT = 47,
	SV_GFX_FORMAT_R8G8_TYPELESS = 48,
	SV_GFX_FORMAT_R8G8_UNORM = 49,
	SV_GFX_FORMAT_R8G8_UINT = 50,
	SV_GFX_FORMAT_R8G8_SNORM = 51,
	SV_GFX_FORMAT_R8G8_SINT = 52,
	SV_GFX_FORMAT_R16_TYPELESS = 53,
	SV_GFX_FORMAT_R16_FLOAT = 54,
	SV_GFX_FORMAT_D16_UNORM = 55,
	SV_GFX_FORMAT_R16_UNORM = 56,
	SV_GFX_FORMAT_R16_UINT = 57,
	SV_GFX_FORMAT_R16_SNORM = 58,
	SV_GFX_FORMAT_R16_SINT = 59,
	SV_GFX_FORMAT_R8_TYPELESS = 60,
	SV_GFX_FORMAT_R8_UNORM = 61,
	SV_GFX_FORMAT_R8_UINT = 62,
	SV_GFX_FORMAT_R8_SNORM = 63,
	SV_GFX_FORMAT_R8_SINT = 64,
	SV_GFX_FORMAT_A8_UNORM = 65,
	SV_GFX_FORMAT_R1_UNORM = 66,
	SV_GFX_FORMAT_R9G9B9E5_SHAREDEXP = 67,
	SV_GFX_FORMAT_R8G8_B8G8_UNORM = 68,
	SV_GFX_FORMAT_G8R8_G8B8_UNORM = 69,
	SV_GFX_FORMAT_BC1_TYPELESS = 70,
	SV_GFX_FORMAT_BC1_UNORM = 71,
	SV_GFX_FORMAT_BC1_UNORM_SRGB = 72,
	SV_GFX_FORMAT_BC2_TYPELESS = 73,
	SV_GFX_FORMAT_BC2_UNORM = 74,
	SV_GFX_FORMAT_BC2_UNORM_SRGB = 75,
	SV_GFX_FORMAT_BC3_TYPELESS = 76,
	SV_GFX_FORMAT_BC3_UNORM = 77,
	SV_GFX_FORMAT_BC3_UNORM_SRGB = 78,
	SV_GFX_FORMAT_BC4_TYPELESS = 79,
	SV_GFX_FORMAT_BC4_UNORM = 80,
	SV_GFX_FORMAT_BC4_SNORM = 81,
	SV_GFX_FORMAT_BC5_TYPELESS = 82,
	SV_GFX_FORMAT_BC5_UNORM = 83,
	SV_GFX_FORMAT_BC5_SNORM = 84,
	SV_GFX_FORMAT_B5G6R5_UNORM = 85,
	SV_GFX_FORMAT_B5G5R5A1_UNORM = 86,
	SV_GFX_FORMAT_B8G8R8A8_UNORM = 87,
	SV_GFX_FORMAT_B8G8R8X8_UNORM = 88,
	SV_GFX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
	SV_GFX_FORMAT_B8G8R8A8_TYPELESS = 90,
	SV_GFX_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
	SV_GFX_FORMAT_B8G8R8X8_TYPELESS = 92,
	SV_GFX_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
	SV_GFX_FORMAT_BC6H_TYPELESS = 94,
	SV_GFX_FORMAT_BC6H_UF16 = 95,
	SV_GFX_FORMAT_BC6H_SF16 = 96,
	SV_GFX_FORMAT_BC7_TYPELESS = 97,
	SV_GFX_FORMAT_BC7_UNORM = 98,
	SV_GFX_FORMAT_BC7_UNORM_SRGB = 99,
	SV_GFX_FORMAT_AYUV = 100,
	SV_GFX_FORMAT_Y410 = 101,
	SV_GFX_FORMAT_Y416 = 102,
	SV_GFX_FORMAT_NV12 = 103,
	SV_GFX_FORMAT_P010 = 104,
	SV_GFX_FORMAT_P016 = 105,
	SV_GFX_FORMAT_420_OPAQUE = 106,
	SV_GFX_FORMAT_YUY2 = 107,
	SV_GFX_FORMAT_Y210 = 108,
	SV_GFX_FORMAT_Y216 = 109,
	SV_GFX_FORMAT_NV11 = 110,
	SV_GFX_FORMAT_AI44 = 111,
	SV_GFX_FORMAT_IA44 = 112,
	SV_GFX_FORMAT_P8 = 113,
	SV_GFX_FORMAT_A8P8 = 114,
	SV_GFX_FORMAT_B4G4R4A4_UNORM = 115,
	
	SV_GFX_FORMAT_P208 = 130,
	SV_GFX_FORMAT_V208 = 131,
	SV_GFX_FORMAT_V408 = 132,
	
	
	SV_GFX_FORMAT_FORCE_UINT = 0xffffffff
};

namespace SV {

	constexpr ui32 GetFormatSize(SV_GFX_FORMAT format)
	{
		switch (format)
		{
		case SV_GFX_FORMAT_R32G32B32A32_TYPELESS:
		case SV_GFX_FORMAT_R32G32B32A32_FLOAT:
		case SV_GFX_FORMAT_R32G32B32A32_UINT:
		case SV_GFX_FORMAT_R32G32B32A32_SINT:
			return 16u;
		case SV_GFX_FORMAT_R32G32B32_TYPELESS:
		case SV_GFX_FORMAT_R32G32B32_FLOAT:
		case SV_GFX_FORMAT_R32G32B32_UINT:
		case SV_GFX_FORMAT_R32G32B32_SINT:
			return 12u;
		case SV_GFX_FORMAT_R16G16B16A16_TYPELESS:
		case SV_GFX_FORMAT_R16G16B16A16_FLOAT:
		case SV_GFX_FORMAT_R16G16B16A16_UNORM:
		case SV_GFX_FORMAT_R16G16B16A16_UINT:
		case SV_GFX_FORMAT_R16G16B16A16_SNORM:
		case SV_GFX_FORMAT_R16G16B16A16_SINT:
		case SV_GFX_FORMAT_R32G32_TYPELESS:
		case SV_GFX_FORMAT_R32G32_FLOAT:
		case SV_GFX_FORMAT_R32G32_UINT:
		case SV_GFX_FORMAT_R32G32_SINT:
		case SV_GFX_FORMAT_R32G8X24_TYPELESS:
		case SV_GFX_FORMAT_D32_FLOAT_S8X24_UINT:
		case SV_GFX_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case SV_GFX_FORMAT_X32_TYPELESS_G8X24_UINT:
			return 8;
		case SV_GFX_FORMAT_R10G10B10A2_TYPELESS:
		case SV_GFX_FORMAT_R10G10B10A2_UNORM:
		case SV_GFX_FORMAT_R10G10B10A2_UINT:
		case SV_GFX_FORMAT_R11G11B10_FLOAT:
		case SV_GFX_FORMAT_R8G8B8A8_TYPELESS:
		case SV_GFX_FORMAT_R8G8B8A8_UNORM:
		case SV_GFX_FORMAT_R8G8B8A8_UNORM_SRGB:
		case SV_GFX_FORMAT_R8G8B8A8_UINT:
		case SV_GFX_FORMAT_R8G8B8A8_SNORM:
		case SV_GFX_FORMAT_R8G8B8A8_SINT:
		case SV_GFX_FORMAT_R16G16_TYPELESS:
		case SV_GFX_FORMAT_R16G16_FLOAT:
		case SV_GFX_FORMAT_R16G16_UNORM:
		case SV_GFX_FORMAT_R16G16_UINT:
		case SV_GFX_FORMAT_R16G16_SNORM:
		case SV_GFX_FORMAT_R16G16_SINT:
		case SV_GFX_FORMAT_R32_TYPELESS:
		case SV_GFX_FORMAT_D32_FLOAT:
		case SV_GFX_FORMAT_R32_FLOAT:
		case SV_GFX_FORMAT_R32_UINT:
		case SV_GFX_FORMAT_R32_SINT:
		case SV_GFX_FORMAT_R24G8_TYPELESS:
		case SV_GFX_FORMAT_D24_UNORM_S8_UINT:
		case SV_GFX_FORMAT_R24_UNORM_X8_TYPELESS:
		case SV_GFX_FORMAT_X24_TYPELESS_G8_UINT:
		case SV_GFX_FORMAT_R8G8_B8G8_UNORM:
		case SV_GFX_FORMAT_G8R8_G8B8_UNORM:
		case SV_GFX_FORMAT_B8G8R8A8_UNORM:
		case SV_GFX_FORMAT_B8G8R8X8_UNORM:
		case SV_GFX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		case SV_GFX_FORMAT_B8G8R8A8_TYPELESS:
		case SV_GFX_FORMAT_B8G8R8A8_UNORM_SRGB:
		case SV_GFX_FORMAT_B8G8R8X8_TYPELESS:
		case SV_GFX_FORMAT_B8G8R8X8_UNORM_SRGB:
		case SV_GFX_FORMAT_R9G9B9E5_SHAREDEXP:
			return 4u;
		case SV_GFX_FORMAT_R8G8_TYPELESS:
		case SV_GFX_FORMAT_R8G8_UNORM:
		case SV_GFX_FORMAT_R8G8_UINT:
		case SV_GFX_FORMAT_R8G8_SNORM:
		case SV_GFX_FORMAT_R8G8_SINT:
		case SV_GFX_FORMAT_R16_TYPELESS:
		case SV_GFX_FORMAT_R16_FLOAT:
		case SV_GFX_FORMAT_D16_UNORM:
		case SV_GFX_FORMAT_R16_UNORM:
		case SV_GFX_FORMAT_R16_UINT:
		case SV_GFX_FORMAT_R16_SNORM:
		case SV_GFX_FORMAT_R16_SINT:
		case SV_GFX_FORMAT_B5G6R5_UNORM:
		case SV_GFX_FORMAT_A8P8:
		case SV_GFX_FORMAT_B4G4R4A4_UNORM:
		case SV_GFX_FORMAT_B5G5R5A1_UNORM:
			return 2u;
		case SV_GFX_FORMAT_R8_TYPELESS:
		case SV_GFX_FORMAT_R8_UNORM:
		case SV_GFX_FORMAT_R8_UINT:
		case SV_GFX_FORMAT_R8_SNORM:
		case SV_GFX_FORMAT_R8_SINT:
		case SV_GFX_FORMAT_A8_UNORM:
			return 1u;
		case SV_GFX_FORMAT_R1_UNORM:
		case SV_GFX_FORMAT_BC1_TYPELESS:
		case SV_GFX_FORMAT_BC1_UNORM:
		case SV_GFX_FORMAT_BC1_UNORM_SRGB:
		case SV_GFX_FORMAT_BC2_TYPELESS:
		case SV_GFX_FORMAT_BC2_UNORM:
		case SV_GFX_FORMAT_BC2_UNORM_SRGB:
		case SV_GFX_FORMAT_BC3_TYPELESS:
		case SV_GFX_FORMAT_BC3_UNORM:
		case SV_GFX_FORMAT_BC3_UNORM_SRGB:
		case SV_GFX_FORMAT_BC4_TYPELESS:
		case SV_GFX_FORMAT_BC4_UNORM:
		case SV_GFX_FORMAT_BC4_SNORM:
		case SV_GFX_FORMAT_BC5_TYPELESS:
		case SV_GFX_FORMAT_BC5_UNORM:
		case SV_GFX_FORMAT_BC5_SNORM:
		case SV_GFX_FORMAT_BC6H_TYPELESS:
		case SV_GFX_FORMAT_BC6H_UF16:
		case SV_GFX_FORMAT_BC6H_SF16:
		case SV_GFX_FORMAT_BC7_TYPELESS:
		case SV_GFX_FORMAT_BC7_UNORM:
		case SV_GFX_FORMAT_BC7_UNORM_SRGB:
		case SV_GFX_FORMAT_AYUV:
		case SV_GFX_FORMAT_Y410:
		case SV_GFX_FORMAT_Y416:
		case SV_GFX_FORMAT_NV12:
		case SV_GFX_FORMAT_P010:
		case SV_GFX_FORMAT_P016:
		case SV_GFX_FORMAT_420_OPAQUE:
		case SV_GFX_FORMAT_YUY2:
		case SV_GFX_FORMAT_Y210:
		case SV_GFX_FORMAT_Y216:
		case SV_GFX_FORMAT_NV11:
		case SV_GFX_FORMAT_AI44:
		case SV_GFX_FORMAT_IA44:
		case SV_GFX_FORMAT_P8:
		case SV_GFX_FORMAT_P208:
		case SV_GFX_FORMAT_V208:
		case SV_GFX_FORMAT_V408:
		case SV_GFX_FORMAT_FORCE_UINT:
		case SV_GFX_FORMAT_UNKNOWN:
		default:
			SV::LogW("Unknown Format size: '%u'", format);
			return 0u;
		}
	}
}

enum SV_GFX_TEXTURE_ADDRESS_MODE
{
	SV_GFX_TEXTURE_ADDRESS_WRAP = 1,
	SV_GFX_TEXTURE_ADDRESS_MIRROR = 2,
	SV_GFX_TEXTURE_ADDRESS_CLAMP = 3,
	SV_GFX_TEXTURE_ADDRESS_BORDER = 4,
	SV_GFX_TEXTURE_ADDRESS_MIRROR_ONCE = 5
};

enum SV_GFX_TEXTURE_FILTER
{
	SV_GFX_TEXTURE_FILTER_MIN_MAG_MIP_POINT = 0,
	SV_GFX_TEXTURE_FILTER_MIN_MAG_POINT_MIP_LINEAR = 0x1,
	SV_GFX_TEXTURE_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x4,
	SV_GFX_TEXTURE_FILTER_MIN_POINT_MAG_MIP_LINEAR = 0x5,
	SV_GFX_TEXTURE_FILTER_MIN_LINEAR_MAG_MIP_POINT = 0x10,
	SV_GFX_TEXTURE_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x11,
	SV_GFX_TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT = 0x14,
	SV_GFX_TEXTURE_FILTER_MIN_MAG_MIP_LINEAR = 0x15,
	SV_GFX_TEXTURE_FILTER_ANISOTROPIC = 0x55,
	SV_GFX_TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_POINT = 0x80,
	SV_GFX_TEXTURE_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR = 0x81,
	SV_GFX_TEXTURE_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x84,
	SV_GFX_TEXTURE_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR = 0x85,
	SV_GFX_TEXTURE_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT = 0x90,
	SV_GFX_TEXTURE_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x91,
	SV_GFX_TEXTURE_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT = 0x94,
	SV_GFX_TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR = 0x95,
	SV_GFX_TEXTURE_FILTER_COMPARISON_ANISOTROPIC = 0xd5,
	SV_GFX_TEXTURE_FILTER_MINIMUM_MIN_MAG_MIP_POINT = 0x100,
	SV_GFX_TEXTURE_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x101,
	SV_GFX_TEXTURE_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x104,
	SV_GFX_TEXTURE_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x105,
	SV_GFX_TEXTURE_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x110,
	SV_GFX_TEXTURE_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x111,
	SV_GFX_TEXTURE_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x114,
	SV_GFX_TEXTURE_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR = 0x115,
	SV_GFX_TEXTURE_FILTER_MINIMUM_ANISOTROPIC = 0x155,
	SV_GFX_TEXTURE_FILTER_MAXIMUM_MIN_MAG_MIP_POINT = 0x180,
	SV_GFX_TEXTURE_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x181,
	SV_GFX_TEXTURE_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x184,
	SV_GFX_TEXTURE_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x185,
	SV_GFX_TEXTURE_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x190,
	SV_GFX_TEXTURE_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x191,
	SV_GFX_TEXTURE_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x194,
	SV_GFX_TEXTURE_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR = 0x195,
	SV_GFX_TEXTURE_FILTER_MAXIMUM_ANISOTROPIC = 0x1d5
};

enum SV_GFX_TOPOLOGY : ui8 {
	SV_GFX_TOPOLOGY_UNDEFINED = 0,
	SV_GFX_TOPOLOGY_POINTS = 1,
	SV_GFX_TOPOLOGY_LINES = 2,
	SV_GFX_TOPOLOGY_LINESTRIP = 3,
	SV_GFX_TOPOLOGY_TRIANGLES = 4,
	SV_GFX_TOPOLOGY_TRIANGLESTRIP = 5,
};

enum SV_GFX_SHADER_TYPE : ui8 {
	SV_GFX_SHADER_TYPE_VERTEX,
	SV_GFX_SHADER_TYPE_PIXEL,
	SV_GFX_SHADER_TYPE_GEOMETRY,
	SV_GFX_SHADER_TYPE_UNDEFINED,
};

struct SV_GFX_INPUT_ELEMENT_DESC {
	const char* SemanticName;
	ui32 SemanticIndex;
	SV_GFX_FORMAT Format;
	ui32 InputSlot;
	ui32 AlignedByteOffset;
	bool InstancedData;
	ui32 InstanceDataStepRate;
};

enum SV_GFX_MODE_SCANLINE_ORDER
{
	SV_GFX_MODE_SCANLINE_ORDER_UNSPECIFIED = 0,
	SV_GFX_MODE_SCANLINE_ORDER_PROGRESSIVE = 1,
	SV_GFX_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST = 2,
	SV_GFX_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST = 3
};

enum SV_GFX_MODE_SCALING
{
	SV_GFX_MODE_SCALING_UNSPECIFIED = 0,
	SV_GFX_MODE_SCALING_CENTERED = 1,
	SV_GFX_MODE_SCALING_STRETCHED = 2
};