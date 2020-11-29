#include "core.h"

#include "material_system_internal.h"

namespace sv {

	std::unordered_map<std::string, ShaderLibraryType_internal*> g_ShaderLibraryTypeNames;

	Result matsys_shaderlibrary_type_register(const ShaderLibraryTypeDesc* desc)
	{
		{
			auto it = g_ShaderLibraryTypeNames.find(desc->name);
			if (it != g_ShaderLibraryTypeNames.end()) {
				SV_LOG_ERROR("Duplicated Renderer Type '%s'", desc->name);
				return Result_Duplicated;
			}
		}
		SV_ASSERT(desc->name);

		if (desc->subShaderCount > MAX_SUBSHADERS) {
			SV_LOG_ERROR("Can't create a shader library with more than %u subshaders", MAX_SUBSHADERS);
			return Result_InvalidUsage;
		}

		ShaderLibraryType_internal& type = g_ShaderLibrariesTypes.create();

		type.name = desc->name;
		type.subShaderCount = desc->subShaderCount;

		// Store Names
		for (ui32 i = 0u; i < type.subShaderCount; ++i) {
			SV_ASSERT(desc->name[i]);
			type.subShaderRegisters[i].name = desc->pSubShaderNames[i];
		}
		// TODO: Should check if has duplicated subshader names??

		// Store LibNames
		if (desc->pSubShaderPreLibName) {
			for (ui32 i = 0u; i < type.subShaderCount; ++i) {
				if (desc->pSubShaderPreLibName[i])
					type.subShaderRegisters[i].preLibName = desc->pSubShaderPreLibName[i];
			}
		}
		if (desc->pSubShaderPostLibName) {
			for (ui32 i = 0u; i < type.subShaderCount; ++i) {
				if (desc->pSubShaderPostLibName[i])
					type.subShaderRegisters[i].postLibName = desc->pSubShaderPostLibName[i];
			}
		}

		// Store Types
		for (ui32 i = 0u; i < type.subShaderCount; ++i) {
			if (desc->pSubShaderTypes[i])
				type.subShaderRegisters[i].type = desc->pSubShaderTypes[i];
		}

		// TODO: Default shaders
		// Store default shaders
		for (ui32 i = 0u; i < type.subShaderCount; ++i) {
			type.subShaderRegisters[i].defaultShader = nullptr;
		}
		
		// Save the renderer name
		g_ShaderLibraryTypeNames[type.name] = &type;

		return Result_Success;
	}

	SubShaderID sv::ShaderLibraryType_internal::findSubShaderID(const char* name)
	{
		for (ui32 i = 0u; i < subShaderCount; ++i) {

			SubShaderRegister& reg = subShaderRegisters[i];

			if (strcmp(reg.name.c_str(), name) == 0u) {
				return i;
			}
		}

		return ui32_max;
	}

	SubShaderID matsys_subshader_get(const char* typeName, const char* name)
	{
		auto it = g_ShaderLibraryTypeNames.find(typeName);
		if (it == g_ShaderLibraryTypeNames.end()) return ui32_max;

		return it->second->findSubShaderID(name);
	}

}