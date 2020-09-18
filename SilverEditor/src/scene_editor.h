#pragma once

#include "core_editor.h"
#include "scene.h"

namespace sve {

	struct DebugCamera {
		sv::CameraSettings		settings;
		sv::Offscreen			offscreen;
		sv::vec3f				position;
		sv::vec3f				rotation;
	};

	enum SceneEditorMode : ui32 {
		SceneEditorMode_2D,
		SceneEditorMode_3D,
	};

	sv::Result scene_editor_initialize();
	sv::Result scene_editor_close();

	void scene_editor_update(float dt);
	void scene_editor_render();

	DebugCamera& scene_editor_camera_get();

	SceneEditorMode scene_editor_mode_get();
	void			scene_editor_mode_set(SceneEditorMode mode);

}