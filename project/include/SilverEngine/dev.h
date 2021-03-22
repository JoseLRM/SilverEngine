#pragma once

#ifdef SV_DEV

#include "SilverEngine/scene.h"

namespace sv {

	struct DebugCamera : public CameraComponent {
		v3_f32 position;
		v4_f32 rotation = { 0.f, 0.f, 0.f, 1.f };
		f32 velocity = 0.45f;
	};

	struct GlobalDevData {
		
		bool console_active = false;

		bool debug_draw = true;
		DebugCamera camera;

		GUI* gui;
	};

	extern GlobalDevData dev;

	// EDITOR GUI

	bool egui_begin();
	void egui_end();

	bool egui_begin_window(const char* title);
	void egui_end_window();

	void egui_header(const char* text, u64 id);
	bool egui_button(const char* text, u64 id);
	void egui_transform(Entity entity);

	bool egui_begin_component(Entity entity, CompID comp_id, bool* remove);
	void egui_end_component();

	void egui_comp_color(const char* text, u64 id, Color* pcolor);
	void egui_comp_texture(const char* text, u64 id, TextureAsset* texture);
	bool egui_comp_bool(const char* text, u64 id, bool* value);

}

#endif
