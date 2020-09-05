#pragma once

#include "graphics.h"

namespace sv {

	class Texture;

	struct Sprite {
		sv::SharedRef<Texture>	texture;
		vec4					texCoord;
	};

	class Texture {
		GPUImage			m_Image;
		Sampler				m_Sampler;

	public:
		Result CreateFromFile(const char* filePath, bool linearFilter, SamplerAddressMode addressMode);
		Result Destroy();

		inline GPUImage& GetImage() { return m_Image; }
		inline Sampler& GetSampler() { return m_Sampler; }

	};

}