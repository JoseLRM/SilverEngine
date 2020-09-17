#pragma once

#include "renderer/objects/renderer_objects.h"

namespace sv {

	struct MeshInstance {
		XMMATRIX	modelViewMatrix;
		Mesh* pMesh;
		Material* pMaterial;

		MeshInstance() = default;
		MeshInstance(XMMATRIX mvm, Mesh* mesh, Material* mat) : modelViewMatrix(mvm), pMesh(mesh), pMaterial(mat) {}
	};

	struct ForwardRenderingDesc {
		const MeshInstance* pInstances;
		ui32* pIndices;
		ui32					count;
		bool					transparent;
		const XMMATRIX* pViewMatrix;
		const XMMATRIX* pProjectionMatrix;
		GPUImage* pRenderTarget;
		GPUImage* pDepthStencil;
		const LightInstance* lights;
		ui32					lightCount;
	};

	void renderer_mesh_forward_rendering(const ForwardRenderingDesc* desc, CommandList cmd);

}