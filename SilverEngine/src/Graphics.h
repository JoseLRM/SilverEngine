#pragma once

#include "core.h"
#include "GraphicsDesc.h"
#include "EngineDevice.h"

struct SV_GRAPHICS_INITIALIZATION_DESC {

};

namespace SV {

	class VertexBuffer;
	class IndexBuffer;
	class ConstantBuffer;
	class FrameBuffer;
	class Shader;
	class InputLayout;
	class Texture;
	class Sampler;
	class BlendState;

	// CommandList
	class Graphics;
	class CommandList {
		Graphics* m_pDevice;
		ui32 m_ID;
	public:
		CommandList(Graphics* pDevice, ui32 ID) : m_pDevice(pDevice), m_ID(ID) {}
		inline ui32 GetID() const noexcept { return m_ID; }
		inline Graphics& GetDevice() const noexcept { return *m_pDevice; }
	};

	/////////////////////////////PRIMITIVES///////////////////////
	namespace _internal {
		class VertexBuffer_internal {
		protected:
			ui32 m_LastSlot;
			SV_GFX_USAGE m_Usage;
			ui32 m_Size;

		public:
			bool Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, Graphics& device);
			void Release();

			void Update(void* data, ui32 size, CommandList& cmd);

			void Bind(ui32 slot, ui32 stride, ui32 offset, CommandList& cmd);
			void Unbind(CommandList& cmd);

		protected:
			virtual bool _Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, Graphics& device) = 0;
			virtual void _Release() = 0;
			virtual void _Update(void* data, ui32 size, CommandList& cmd) = 0;
			virtual void _Bind(ui32 slot, ui32 stride, ui32 offset, CommandList& cmd) = 0;
			virtual void _Unbind(CommandList& cmd) = 0;

		};

		class IndexBuffer_internal {
		public:
			bool Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, Graphics& device);
			void Release();
				 
			void Bind(SV_GFX_FORMAT format, ui32 offset, CommandList& cmd);
			void Unbind(CommandList& cmd);

		protected:
			virtual bool _Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, Graphics& device) = 0;
			virtual void _Release() = 0;
			virtual void _Bind(SV_GFX_FORMAT format, ui32 offset, CommandList& cmd) = 0;
			virtual void _Unbind(CommandList& cmd) = 0;

		};

		class ConstantBuffer_internal {
		protected:
			ui32 m_LastSlot[3];
			SV_GFX_USAGE m_Usage;
			ui32 m_Size;

		public:
			bool Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, Graphics& device);
			void Release();

			void Update(void* data, ui32 size, CommandList& cmd);

			void Bind(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList& cmd);
			void Unbind(SV_GFX_SHADER_TYPE type, CommandList& cmd);

		protected:
			virtual bool _Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, Graphics& device) = 0;
			virtual void _Release() = 0;

			virtual void _Update(void* data, ui32 size, CommandList& cmd) = 0;

			virtual void _Bind(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList& cmd) = 0;
			virtual void _Unbind(SV_GFX_SHADER_TYPE type, CommandList& cmd) = 0;

		};

		class FrameBuffer_internal {
		protected:
			ui32 m_Width = 0;
			ui32 m_Height = 0;
			ui32 m_LastSlot;
			SV_GFX_FORMAT m_Format;

			bool m_TextureUsage;

		public:
			bool Create(ui32 width, ui32 height, SV_GFX_FORMAT format, bool textureUsage, SV::Graphics& device);
			void Release();

			bool Resize(ui32 width, ui32 height, SV::Graphics& device);

			void Clear(SV::Color4f color, CommandList& cmd);
			void Bind(ui32 slot, CommandList& cmd);
			void Unbind(CommandList& cmd);

			void BindAsTexture(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd);
			void UnbindAsTexture(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd);

			inline ui32 GetWidth() const noexcept { return m_Width; }
			inline ui32 GetHeight() const noexcept { return m_Height; }

		protected:
			virtual bool _Create(ui32 width, ui32 height, SV_GFX_FORMAT format, bool textureUsage, SV::Graphics& device) = 0;
			virtual void _Release() = 0;
			virtual bool _Resize(ui32 width, ui32 height, SV::Graphics& device) = 0;
			virtual void _Clear(SV::Color4f color, CommandList& cmd) = 0;
			virtual void _Bind(ui32 slot, CommandList& cmd) = 0;
			virtual void _Unbind(CommandList& cmd) = 0;
			virtual void _BindAsTexture(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd) = 0;

		};

		class Shader_internal {
		protected:
			SV_GFX_SHADER_TYPE m_ShaderType;

		public:
			bool Create(SV_GFX_SHADER_TYPE type, const char* filePath, SV::Graphics& device);
			void Release();

			void Bind(CommandList& cmd);
			void Unbind(CommandList& cmd);

			inline SV_GFX_SHADER_TYPE GetType() const noexcept { return m_ShaderType; }

		protected:
			virtual bool _Create(SV_GFX_SHADER_TYPE type, const char* filePath, SV::Graphics& device) = 0;
			virtual void _Release() = 0;
			virtual void _Bind(CommandList& cmd) = 0;
			virtual void _Unbind(CommandList& cmd) = 0;
		};

		class InputLayout_internal {
		public:
			bool Create(const SV_GFX_INPUT_ELEMENT_DESC* desc, ui32 count, const Shader& vs, SV::Graphics& device);
			void Release();

			void Bind(CommandList& cmd);
			void Unbind(CommandList& cmd);

		protected:
			virtual bool _Create(const SV_GFX_INPUT_ELEMENT_DESC* desc, ui32 count, const Shader& vs, SV::Graphics& device) = 0;
			virtual void _Release() = 0;
			virtual void _Bind(CommandList& cmd) = 0;
			virtual void _Unbind(CommandList& cmd) = 0;

		};

		class Texture_internal {
		protected:
			SV_GFX_FORMAT m_Format;
			SV_GFX_USAGE m_Usage;
			// 1 -> write, 2 -> read, 3 -> write-read
			ui8 m_CPUAccess;
			ui32 m_Width;
			ui32 m_Height;
			ui32 m_Size;

		public:
			bool Create(void* data, ui32 width, ui32 height, SV_GFX_FORMAT format, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, SV::Graphics& device);
			void Release();
			bool Resize(void* data, ui32 width, ui32 height, SV::Graphics& device);

			void Update(void* data, ui32 size, CommandList& cmd);

			void Bind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd);
			void Unbind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd);

		protected:
			virtual bool _Create(void* data, ui32 width, ui32 height, SV_GFX_FORMAT format, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, SV::Graphics& device) = 0;
			virtual void _Release() = 0;
			virtual void _Update(void* data, ui32 size, CommandList& cmd) = 0;
			virtual void _Bind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd) = 0;
			virtual void _Unbind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd) = 0;

		};

		class Sampler_internal {
		protected:
		public:
			bool Create(SV_GFX_TEXTURE_ADDRESS_MODE addressMode, SV_GFX_TEXTURE_FILTER filter, Graphics& device);
			void Release();

			void Bind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd);
			void Unbind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd);

		protected:
			virtual bool _Create(SV_GFX_TEXTURE_ADDRESS_MODE addressMode, SV_GFX_TEXTURE_FILTER filter, Graphics& device) = 0;
			virtual void _Release() = 0;
			virtual void _Bind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd) = 0;
			virtual void _Unbind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd) = 0;

		};

		template<typename T>
		class Primitive_internal {
			std::unique_ptr<T> m_Allocation;
		public:
			inline T* operator->() const noexcept { return m_Allocation.get(); }
			inline bool IsValid() const noexcept { return m_Allocation.get(); }
			inline T* Get() const noexcept { return m_Allocation.get(); }
			inline void Set(std::unique_ptr<T> alloc) noexcept { m_Allocation = std::move(alloc); }

		};

		class BlendState_internal {
		protected:
			struct RENDER_TARGET_BLEND_DESC {
				bool enabled = false;
				SV_GFX_BLEND src = SV_GFX_BLEND_ONE;
				SV_GFX_BLEND srcAlpha = SV_GFX_BLEND_ONE;
				SV_GFX_BLEND dest = SV_GFX_BLEND_ZERO;
				SV_GFX_BLEND destAlpha = SV_GFX_BLEND_ZERO;
				SV_GFX_BLEND_OP op = SV_GFX_BLEND_OP_ADD;
				SV_GFX_BLEND_OP opAlpha = SV_GFX_BLEND_OP_ADD;
				ui8 writeMask = SV_GFX_COLOR_WRITE_ENABLE_ALL;
			};

			bool m_IndependentRenderTarget = false;
			RENDER_TARGET_BLEND_DESC m_BlendDesc[8];

		public:
			// CreateFunctions
			void SetIndependentRenderTarget(bool enable);
			void EnableBlending(ui8 renderTarget = 0);
			void DisableBlending(ui8 renderTarget = 0);
			void SetRenderTargetOperation(SV_GFX_BLEND src, SV_GFX_BLEND srcAlpha, SV_GFX_BLEND dest, SV_GFX_BLEND destAlpha, SV_GFX_BLEND_OP op, SV_GFX_BLEND_OP opAlpha, ui8 renderTarget = 0);
			void SetRenderTargetOperation(SV_GFX_BLEND src, SV_GFX_BLEND dest, SV_GFX_BLEND_OP op, ui8 renderTarget = 0);
			void SetWriteMask(ui8 writeMask, ui8 renderTarget = 0u);

			bool Create(SV::Graphics& graphics);
			void Release();

			void Bind(ui32 sampleMask, const float* blendFactors, CommandList& cmd);
			void Unbind(CommandList& cmd);

		protected:
			virtual bool _Create(SV::Graphics& graphics) = 0;
			virtual void _Release() = 0;

			virtual void _Bind(ui32 sampleMask, const float* blendFactors, CommandList& cmd) = 0;
			virtual void _Unbind(CommandList& cmd) = 0;

		};
	}
	//////////////////////////////////////////////////////////////

	class VertexBuffer		: public _internal::Primitive_internal<_internal::VertexBuffer_internal> {};
	class IndexBuffer		: public _internal::Primitive_internal<_internal::IndexBuffer_internal> {};
	class ConstantBuffer	: public _internal::Primitive_internal<_internal::ConstantBuffer_internal> {};
	class FrameBuffer		: public _internal::Primitive_internal<_internal::FrameBuffer_internal> {};
	class Shader			: public _internal::Primitive_internal<_internal::Shader_internal> {};
	class InputLayout		: public _internal::Primitive_internal<_internal::InputLayout_internal> {};
	class Texture			: public _internal::Primitive_internal<_internal::Texture_internal> {};
	class Sampler			: public _internal::Primitive_internal<_internal::Sampler_internal> {};
	class BlendState		: public _internal::Primitive_internal<_internal::BlendState_internal> {};

	class Graphics : public SV::EngineDevice {

		SV::Adapter m_Adapter;
		ui32 m_OutputModeID = 0u;

		bool m_Fullscreen = false;
		
		bool Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc);
		bool Close();
		virtual bool _Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc) = 0;
		virtual bool _Close() = 0;

#ifdef SV_IMGUI
		virtual void ResizeBackBuffer(ui32 width, ui32 height) = 0;
#endif

		void BeginFrame();
		void EndFrame();

	public:
		friend Engine;
		friend Window;

		Graphics();
		~Graphics();

		void SetFullscreen(bool fullscreen);
		inline bool InFullscreen() const noexcept { return m_Fullscreen; }

		inline const SV::Adapter& GetAdapter() const noexcept { return m_Adapter; }
		inline ui32 GetOutputModeID() const noexcept { return m_OutputModeID; }

		virtual void Present() = 0;
		virtual CommandList BeginCommandList() = 0;
		virtual void SetViewport(ui32 slot, float x, float y, float w, float h, float n, float f, SV::CommandList& cmd) = 0;
		virtual void SetTopology(SV_GFX_TOPOLOGY topology, CommandList& cmd) = 0;

		virtual SV::FrameBuffer& GetMainFrameBuffer() = 0;

		// Draw calls
		virtual void Draw(ui32 vertexCount, ui32 startVertex, CommandList& cmd) = 0;
		virtual void DrawIndexed(ui32 indexCount, ui32 startIndex, ui32 startVertex, CommandList& cmd) = 0;
		virtual void DrawInstanced(ui32 verticesPerInstance, ui32 instances, ui32 startVertex, ui32 startInstance, CommandList& cmd) = 0;
		virtual void DrawIndexedInstanced(ui32 indicesPerInstance, ui32 instances, ui32 startIndex, ui32 startVertex, ui32 startInstance, CommandList& cmd) = 0;

		// Primitives
		virtual void ValidateVertexBuffer(VertexBuffer* buffer) = 0;
		virtual void ValidateIndexBuffer(IndexBuffer* buffer) = 0;
		virtual void ValidateConstantBuffer(ConstantBuffer* buffer) = 0;
		virtual void ValidateFrameBuffer(FrameBuffer* buffer) = 0;
		virtual void ValidateShader(Shader* shader) = 0;
		virtual void ValidateInputLayout(InputLayout* il) = 0;
		virtual void ValidateTexture(Texture* tex) = 0;
		virtual void ValidateSampler(Sampler* sam) = 0;
		virtual void ValidateBlendState(BlendState* bs) = 0;

	private:
		virtual void EnableFullscreen() = 0;
		virtual void DisableFullscreen() = 0;

	};

}