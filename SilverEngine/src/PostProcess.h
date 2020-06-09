#pragma once

#include "core.h"

namespace SV {

	class PostProcess {
		Shader m_PPVertexShader;
		Shader m_DefaultPPPixelShader;
		VertexBuffer m_PPVertexBuffer;
		InputLayout m_PPInputLayout;
		Sampler m_PPSampler;

	public:
		bool Initialize(GraphicsDevice& device);
		bool Close();

		void DefaultPP(FrameBuffer& input, FrameBuffer& output, CommandList& cmd);

	};

}