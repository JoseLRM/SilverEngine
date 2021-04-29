#pragma once

#include "platform/graphics.h"
#include "utils/string.h"

namespace sv {

    constexpr u32 MESH_NAME_SIZE = 100u;
    constexpr u32 MATERIAL_NAME_SIZE = 100u;

    typedef u32 MeshIndex;

    struct MeshVertex {
	v3_f32 position;
	v3_f32 normal;
	v3_f32 tangents;
	v3_f32 bitangents;
	v2_f32 texcoord;
    };

    struct Mesh {

	std::vector<v3_f32> positions;
	std::vector<v3_f32> normals;
	std::vector<v3_f32> tangents;
	std::vector<v3_f32> bitangents;
	std::vector<v2_f32> texcoords;

	std::vector<MeshIndex> indices;

	GPUBuffer* vbuffer = nullptr;
	GPUBuffer* ibuffer = nullptr;

	// Info about the importation of the 3D Model

	XMMATRIX model_transform_matrix = XMMatrixIdentity();
	std::string model_material_filepath;

    };

    struct Material {
	Color diffuse_color;
	Color specular_color;
	Color emissive_color;
	f32   shininess;
	TextureAsset diffuse_map;
	TextureAsset normal_map;
	TextureAsset specular_map;
	TextureAsset emissive_map;
    }; 

    SV_DEFINE_ASSET(MeshAsset, Mesh);
    SV_DEFINE_ASSET(MaterialAsset, Material);

    SV_API void mesh_apply_plane(Mesh& mesh, const XMMATRIX& transform = XMMatrixIdentity());
    SV_API void mesh_apply_cube(Mesh& mesh, const XMMATRIX& transform = XMMatrixIdentity());
    SV_API void mesh_apply_sphere(Mesh& mesh, const XMMATRIX& transform = XMMatrixIdentity());

    SV_API void mesh_set_scale(Mesh& mesh, f32 scale, bool center = false);
    SV_API void mesh_optimize(Mesh& mesh);
    SV_API void mesh_recalculate_normals(Mesh& mesh);

    SV_API bool mesh_create_buffers(Mesh& mesh, ResourceUsage usage = ResourceUsage_Static);
    SV_API bool mesh_update_buffers(Mesh& mesh, CommandList cmd);
    SV_API bool mesh_clear(Mesh& mesh);

    // Model loading

    struct MeshInfo {

	String name;

	List<v3_f32> positions;
	List<v3_f32> normals;
	List<v3_f32> tangents;
	List<v3_f32> bitangents;
	List<v2_f32> texcoords;

	List<MeshIndex> indices;

	XMMATRIX transform_matrix;
	u32 material_index = u32_max;

    };

    struct MaterialInfo {
	
	String name;
	Color diffuse_color;
	Color specular_color;
	Color emissive_color;
	f32 shininess;
	String diffuse_map_path;
	String normal_map_path;
	String specular_map_path;
	String emissive_map_path;
    };

    struct ModelInfo {
	String folderpath;
	List<MeshInfo> meshes;
	List<MaterialInfo> materials;
    };

    // Load external model format
    SV_API bool load_model(const char* filepath, ModelInfo& model_info);

    // Create asset files for the engine
    SV_API bool import_model(const char* filepath, const ModelInfo& model_info);

    SV_API bool load_mesh(const char* filepath, Mesh& mesh);
    SV_API bool load_material(const char* filepath, Material& material);

}
