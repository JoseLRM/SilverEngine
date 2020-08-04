#include "core.h"

#include "EditorState.h"
#include "editor.h"

namespace sve {

	EditorState::EditorState() : State()
	{
		m_Scene.Initialize();

		m_MainCamera = m_Scene.CreateEntity();
		m_Scene.AddComponent<sv::CameraComponent>(m_MainCamera, SV_REND_CAMERA_TYPE_ORTHOGRAPHIC);
		m_Scene.AddComponent<sv::NameComponent>(m_MainCamera, "Main Camera");
		sv::CameraComponent* camComp = m_Scene.GetComponent<sv::CameraComponent>(m_MainCamera);
		camComp->projection.Orthographic_SetZoom(5.f);

		// Create debug camera
		SV_ASSERT(m_DebugCamera.camera.CreateOffscreen(sv::renderer_resolution_get().x, sv::renderer_resolution_get().y));
		m_DebugCamera.camera.projection = camComp->projection;
	}

	void EditorState::Load()
	{
	}

	void EditorState::Initialize()
	{

	}

	void EditorState::Update(float dt)
	{
		sv::CameraComponent* camComp = m_Scene.GetComponent<sv::CameraComponent>(m_MainCamera);
		
		// Adjust cameras
		{
			auto props = viewport_manager_properties_get("Game");
			sv::uvec2 size = { props.width, props.height };
			camComp->Adjust(size.x, size.y);
		}

		auto props = viewport_manager_properties_get("Scene Editor");
		{
			sv::uvec2 size = { props.width, props.height };
			m_DebugCamera.camera.Adjust(size.x, size.y);
		}

		// Debug camera controller
		float zoom = m_DebugCamera.camera.projection.Orthographic_GetZoom();
		float force = 15.f * (zoom * 0.05f) * dt;
		float zoomForce = 20.f * dt * (zoom * 0.1f);

		if (props.focus) {
			if (sv::input_key('W')) {
				m_DebugCamera.position.y -= force;
			}
			if (sv::input_key('S')) {
				m_DebugCamera.position.y += force;
			}
			if (sv::input_key('A')) {
				m_DebugCamera.position.x -= force;
			}
			if (sv::input_key('D')) {
				m_DebugCamera.position.x += force;
			}

			if (sv::input_key(SV_KEY_SPACE)) {
				zoom += zoomForce;
			}
			if (sv::input_key(SV_KEY_SHIFT)) {
				zoom -= zoomForce;
			}
			m_DebugCamera.camera.projection.Orthographic_SetZoom(zoom);
		}
	}

	void EditorState::FixedUpdate()
	{
	}

	void EditorState::Render()
	{
		sv::CameraComponent* camComp = m_Scene.GetComponent<sv::CameraComponent>(m_MainCamera);
		sv::Transform trans = m_Scene.GetTransform(camComp->entity);

		sv::renderer_scene_begin();
		sv::renderer_scene_draw_scene(m_Scene);
		sv::renderer_scene_set_camera(camComp->projection, trans.GetWorldMatrix());
		sv::renderer_scene_end();

		m_DebugCamera.viewMatrix = XMMatrixTranslation(-m_DebugCamera.position.x, -m_DebugCamera.position.y, -m_DebugCamera.position.z);

		sv::renderer_present(m_DebugCamera.camera.projection, m_DebugCamera.viewMatrix, m_DebugCamera.camera.GetOffscreen());
	}

	void EditorState::Unload()
	{
	}

	void EditorState::Close()
	{
	}

}