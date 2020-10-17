#include "core.h"

#include "utils/helper.h"

#include "platform/platform_impl.h"

namespace sv {

	std::wstring parse_wstring(const char* c)
	{
		std::wstring str;
		str.resize(strlen(c));
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, c, str.size(), &str[0], str.size());
		return str;
	}

	std::string parse_string(const wchar* c)
	{
		size_t size = wcslen(c) + 1u;
		std::string str;
		str.resize(size);
		size_t i;
		wcstombs_s(&i, str.data(), size, c, size);
		return str;
	}

	size_t hash_string(const char* str)
	{
		// TEMPORAL
		size_t res = 0u;
		size_t strLength = strlen(str);
		hash_combine(res, strLength);
		while (*str != '\0') {
			hash_combine(res, (const size_t)* str);
			++str;
		}
		return res;
	}

}