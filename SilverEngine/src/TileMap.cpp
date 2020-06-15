#include "core.h"
#include "TileMap.h"

namespace SV {

	Shader TileMap::s_VertexShader;
	Shader TileMap::s_PixelShader;
	InputLayout TileMap::s_InputLayout;
	Sampler TileMap::s_NullSampler;
	std::mutex TileMap::s_CreateMutex;

	TileMap::TileMap()
	{
		svZeroMemory(m_Textures, MAX_TEXTURES * sizeof(TextureAtlas*));
	}
	TileMap::~TileMap()
	{
		Deallocate();
	}
	TileMap::TileMap(TileMap&& tileMap) noexcept
	{
		this->operator=(std::move(tileMap));
	}
	TileMap& TileMap::operator=(TileMap&& other) noexcept
	{
		m_Sprites = std::move(other.m_Sprites);
		m_Tiles = other.m_Tiles;
		m_Width = other.m_Width;
		m_Height = other.m_Height;
		m_VertexBuffer = std::move(other.m_VertexBuffer);
		m_IndexBuffer = std::move(other.m_IndexBuffer);
		m_ConstantBuffer = std::move(other.m_ConstantBuffer);
		m_Modified = other.m_Modified;
		other.m_Tiles = nullptr;
		other.m_Width = 0u;
		other.m_Height = 0u;
		other.m_Modified = true;

		return *this;
	} 
	void TileMap::CreateGrid(ui32 width, ui32 height)
	{
		Allocate(width, height);
		m_Width = width;
		m_Height = height;
	}
	Tile TileMap::CreateTile(const SV::Sprite& sprite)
	{
		if (sprite.pTextureAtlas == nullptr) return SV_NULL_TILE;

		ui32 texID = MAX_TEXTURES;

		TextureAtlas* texture = sprite.pTextureAtlas;
		for (ui32 i = 0; i < MAX_TEXTURES; ++i) {
			if (m_Textures[i] == nullptr) {
				m_Textures[i] = texture;
				texID = i;
				break;
			}
			else if (m_Textures[i] == texture) {
				texID = i;
				break;
			}
		}

		SV_ASSERT(texID != MAX_TEXTURES);

		m_Sprites.emplace_back(sprite, texID);
		return m_Sprites.size();
	}

	void TileMap::PutTile(i32 x, i32 y, Tile tile) noexcept
	{
		SV_ASSERT(InBounds(x, y));
		m_Modified = true;
		m_Tiles[x + y * m_Width] = tile;
	}

	void TileMap::Bind(XMMATRIX matrix, CommandList& cmd)
	{
		cmd.GetDevice().SetTopology(SV_GFX_TOPOLOGY_TRIANGLES, cmd);

		if (m_Modified) {
			if (!CreateBuffers(cmd.GetDevice())) return;
		}

		// Bind textures
		for (ui32 i = 0; i < MAX_TEXTURES; ++i) {
			TextureAtlas* texture = m_Textures[i];
			if (texture == nullptr) {
				s_NullSampler->Bind(SV_GFX_SHADER_TYPE_PIXEL, i, cmd);
			}
			else texture->Bind(SV_GFX_SHADER_TYPE_PIXEL, i, cmd);
		}

		// Bind buffers
		m_VertexBuffer->Bind(0u, sizeof(Vertex), 0u, cmd);
		m_IndexBuffer->Bind(SV_GFX_FORMAT_R32_UINT, 0u, cmd);
		m_ConstantBuffer->Bind(0u, SV_GFX_SHADER_TYPE_VERTEX, cmd);

		m_ConstantBuffer->Update(&matrix, sizeof(XMMATRIX), cmd);
	}
	void TileMap::BindShaders(CommandList& cmd)
	{
		s_VertexShader->Bind(cmd);
		s_PixelShader->Bind(cmd);
		s_InputLayout->Bind(cmd);
	}
	void TileMap::Draw(CommandList& cmd)
	{
		cmd.GetDevice().DrawIndexed(m_IndexCount, 0u, 0u, cmd);
	}
	void TileMap::Unbind(CommandList& cmd)
	{
		if (m_Modified) return;

		// Unbind textures
		for (ui32 i = 0; i < MAX_TEXTURES; ++i) {
			TextureAtlas* texture = m_Textures[i];
			if (texture == nullptr) {
				s_NullSampler->Unbind(SV_GFX_SHADER_TYPE_PIXEL, i, cmd);
			}
			else texture->Unbind(SV_GFX_SHADER_TYPE_PIXEL, i, cmd);
		}

		// Unbind buffers
		m_VertexBuffer->Unbind(cmd);
		m_IndexBuffer->Unbind(cmd);
		m_ConstantBuffer->Unbind(SV_GFX_SHADER_TYPE_VERTEX, cmd);
	}
	void TileMap::UnbindShaders(CommandList& cmd)
	{
		s_VertexShader->Unbind(cmd);
		s_PixelShader->Unbind(cmd);
		s_InputLayout->Unbind(cmd);
	}

	void TileMap::Allocate(ui32 width, ui32 height)
	{
		// TODO: Reserve last tiles
		Deallocate();
		size_t size = size_t(width) * size_t(height);
		m_Tiles = new Tile[size];
		svZeroMemory(m_Tiles, size * sizeof(Tile));
	}

	void TileMap::Deallocate()
	{
		m_Modified = true;
		if (m_Tiles) {
			delete[] m_Tiles;
			m_Tiles = nullptr;
		}
	}

	bool TileMap::CreateBuffers(Graphics& graphics)
	{
		if (m_VertexBuffer.IsValid()) m_VertexBuffer->Release();
		if (m_IndexBuffer.IsValid()) m_IndexBuffer->Release();

		graphics.ValidateVertexBuffer(&m_VertexBuffer);
		graphics.ValidateIndexBuffer(&m_IndexBuffer);

		ui32 tileCount = m_Width * m_Height;

		std::vector<Vertex> vertexData;
		vertexData.reserve(size_t(tileCount) * 4u);
		std::vector<ui32> indexData;
		indexData.reserve(size_t(tileCount) * 6u);

		SV::vec2 half = SV::vec2(float(m_Width) / 2.f, float(m_Height) / 2.f);

		for (ui32 y = 0; y < m_Height; ++y) {
			ui32 offset = y * m_Width;

			for (ui32 x = 0; x < m_Width; ++x) {

				Tile tile = m_Tiles[size_t(x) + size_t(offset)];

				if (tile != SV_NULL_TILE) {
					SV::vec2 position = SV::vec2(x, y) - half;
					ui32 texID = 0u;

					const TileData& tileData = m_Sprites[tile - 1];
					SV::vec4 texCoord = tileData.sprite.pTextureAtlas->GetSpriteTexCoords(tileData.sprite.ID);

					SV::vec2 t0 = SV::vec2(texCoord.x, texCoord.y);
					SV::vec2 t1 = SV::vec2(texCoord.x, texCoord.y) + SV::vec2(texCoord.z, texCoord.w);

					ui32 index = ui32(vertexData.size());

					vertexData.emplace_back(position + SV::vec2(0.f, 1.f), t0, tileData.sprite.ID);
					vertexData.emplace_back(position + SV::vec2(1.f), SV::vec2(t1.x, t0.y), tileData.sprite.ID);
					vertexData.emplace_back(position, SV::vec2(t0.x, t1.y), tileData.sprite.ID);
					vertexData.emplace_back(position + SV::vec2(1.f, 0.f), t1, tileData.sprite.ID);

					indexData.emplace_back(index + 0);
					indexData.emplace_back(index + 1);
					indexData.emplace_back(index + 2);

					indexData.emplace_back(index + 2);
					indexData.emplace_back(index + 1);
					indexData.emplace_back(index + 3);
				}
			}
		}

		m_IndexCount = ui32(indexData.size());

		if (m_IndexCount == 0) return false;
		
		// Create Buffers
		if(!m_VertexBuffer->Create(vertexData.size() * sizeof(Vertex), SV_GFX_USAGE_DEFAULT, false, false, &vertexData[0], graphics)) return false;
		if(!m_IndexBuffer->Create(indexData.size() * sizeof(ui32), SV_GFX_USAGE_STATIC, false, false, &indexData[0], graphics)) return false;

		if (!m_ConstantBuffer.IsValid()) {
			graphics.ValidateConstantBuffer(&m_ConstantBuffer);
			if (!m_ConstantBuffer->Create(sizeof(XMMATRIX), SV_GFX_USAGE_DEFAULT, false, false, nullptr, graphics)) return false;
		}

		m_Modified = false;

		return true;
	}

	bool TileMap::CreateShaders(Graphics& graphics)
	{
		std::lock_guard<std::mutex> lock(s_CreateMutex);

		graphics.ValidateShader(&s_VertexShader);
		graphics.ValidateShader(&s_PixelShader);
		graphics.ValidateInputLayout(&s_InputLayout);
		graphics.ValidateSampler(&s_NullSampler);

		// Create static Shaders
		{
			if (!s_VertexShader->Create(SV_GFX_SHADER_TYPE_VERTEX, "shaders/TileMapVertex.cso", graphics)) {
				SV::LogE("TileMap VertexShader not found");
				return false;
			}

			if (!s_PixelShader->Create(SV_GFX_SHADER_TYPE_PIXEL, "shaders/TileMapPixel.cso", graphics)) {
				SV::LogE("TileMap PixelShader not found");
				return false;
			}
		}

		// Create static input layout
		{
			SV_GFX_INPUT_ELEMENT_DESC desc[] = {
				{"Position", 0u, SV_GFX_FORMAT_R32G32_FLOAT, 0u, 0u, false, 0u},
				{"TexCoord", 0u, SV_GFX_FORMAT_R32G32_FLOAT, 0u, 2 * sizeof(float), false, 0u},
				{"TextureID", 0u, SV_GFX_FORMAT_R32_UINT, 0u, 4 * sizeof(float), false, 0u}
			};
			if (!s_InputLayout->Create(desc, 3, s_VertexShader, graphics)) {
				SV::LogE("Can't create TileMap InputLayout");
				return false;
			}
		}

		// Create Null Sampler
		s_NullSampler->Create(SV_GFX_TEXTURE_ADDRESS_WRAP, SV_GFX_TEXTURE_FILTER_MIN_MAG_MIP_LINEAR, graphics);

		return true;
	}

}