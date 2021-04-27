#include "SilverEngine.h"

using namespace sv;

void update_scene()
{
    SceneData& scene = *get_scene_data();
    
    if (entity_exist(scene.player)) {

	v2_f32& pos = *get_entity_position2D_ptr(scene.player);

	f64 t = timer_now();

	pos.x = f32(cos(t)) * 4.f;
	pos.y = f32(sin(t)) * 2.f;
    }
}

void show_component_info(void*, ShowComponentEvent* event)
{
    static bool test = false;
    
    egui_comp_bool("", 0u, &test);
}

SV_USER bool user_initialize(bool init)
{
    if (init) {
	set_scene("Test");
    }

    event_user_register("update_scene", update_scene);
    event_user_register("show_component_info", show_component_info);
    
    return true;
}

SV_USER bool user_close(bool close)
{
    return true;
}

SV_USER bool user_get_scene_filepath(const char* name, char* filepath)
{
    sprintf(filepath, "assets/scenes/%s.scene", name);
    return true;
}
