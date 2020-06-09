#include "core.h"

#include "Renderer.h"

namespace SV {

	Renderer::Renderer()
	{
	}

	Renderer::~Renderer()
	{
	}

	bool Renderer::Initialize(SV_RENDERER_INITIALIZATION_DESC& desc)
	{
		GraphicsDevice& device = GetEngine().GetGraphics().GetDevice();

		if (!m_PostProcess.Initialize(device)) {
			SV::LogE("Can't initialize Post Process");
			return false;
		}

		if (!m_Renderer2D.Initialize(device)) {
			SV::LogE("Can't initialize Renderer2D");
			return false;
		}

		device.ValidateFrameBuffer(&m_Offscreen);

		if (!m_Offscreen->Create(1280, 720, SV_GFX_FORMAT_R8G8B8A8_UNORM, true, device)) {
			SV::LogE("Can't create offscreen framebuffer");
			return false;
		}

		return true;
	}

	bool Renderer::Close()
	{
		if (!m_Renderer2D.Close()) {
			SV::LogE("Can't close Renderer2D");
			return false;
		}

		if (!m_PostProcess.Close()) {
			SV::LogE("Can't close Post Process");
			return false;
		}

		return true;
	}

	void Renderer::BeginFrame()
	{
		if (m_pCamera == nullptr) return;

		m_RenderQueue2D.Begin(m_pCamera);
	}
	void Renderer::Render()
	{
		if (m_pCamera == nullptr) {
			SV::LogW("Where are the camera ;)");
			return;
		}

		m_RenderQueue2D.End();

		GraphicsDevice& device = GetEngine().GetGraphics().GetDevice();

		CommandList cmd = device.BeginCommandList();

		m_Offscreen->Clear(SVColor4f::BLACK, cmd);

		m_Offscreen->Bind(0u, cmd);
		device.SetViewport(0u, 0.f, 0.f, 1280.f, 720.f, 0.f, 1.f, cmd);

		for (ui32 i = 0; i < m_RenderQueue2D.m_pLayers.size(); ++i) {
			m_Renderer2D.DrawQuads(m_RenderQueue2D.m_pLayers[i]->quadInstances, cmd);
			m_Renderer2D.DrawSprites(m_RenderQueue2D.m_pLayers[i]->spriteInstances, cmd);
			m_Renderer2D.DrawEllipses(m_RenderQueue2D.m_pLayers[i]->ellipseInstances, cmd);
			m_Renderer2D.DrawPoints(m_RenderQueue2D.m_pLayers[i]->pointInstances, cmd);
			m_Renderer2D.DrawLines(m_RenderQueue2D.m_pLayers[i]->lineInstances, cmd);
		}

		m_PostProcess.DefaultPP(m_Offscreen, device.GetMainFrameBuffer(), cmd);
	}
	void Renderer::EndFrame()
	{
	}

}