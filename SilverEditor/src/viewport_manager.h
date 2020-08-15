#pragma once

#include "core_editor.h"

namespace sve {

	typedef bool(*DisplayFunction)();

	struct ViewportProperties {
		ui32 x = 0u, y = 0u;
		ui32 width = 1u, height = 1u;
		bool focus = false;
	};

	struct Viewport {
		DisplayFunction displayFn;
		bool show;
		ViewportProperties properties;
	};

	void				viewport_manager_add(const char* name, DisplayFunction displayFn);
	void				viewport_manager_display();
	void				viewport_manager_show(const char* name);
	void				viewport_manager_hide(const char* name);
	ViewportProperties	viewport_manager_properties_get(const char* name);
	bool&				viewport_manager_get_show(const char* name);

	bool viewport_game_display();
	bool viewport_scene_hierarchy_display();
	bool viewport_scene_entity_display();
	bool viewport_scene_editor_display();
	bool viewport_console_display();
	bool viewport_renderer2D_display();

}