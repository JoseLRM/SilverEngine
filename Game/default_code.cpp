#include "SilverEngine.h"

using namespace sv;

struct GameMemory {
    Entity foo;
};

GameMemory* game = nullptr;

void update_scene(void*, void*);

SV_USER bool user_initialize(bool init)
{
    if (init) {
	engine.game_memory = SV_ALLOCATE_STRUCT(GameMemory);
    
	set_scene("Test");
    }

    game = reinterpret_cast<GameMemory*>(engine.game_memory);

    event_user_register("update_scene", update_scene);
    
    return true;
}

SV_USER bool user_close(bool close)
{
    if (close)
	SV_FREE_STRUCT(GameMemory, game);
    
    return true;
}

SV_USER bool user_initialize_scene(Archive* parchive)
{    
    if (parchive) {
	Archive& archive = *parchive;
	archive >> game->foo;
	return true;
    }


    // Create main camera
    Entity& cam = get_scene_data()->main_camera;
    cam = create_entity(SV_ENTITY_NULL, "Camera");
    add_component<CameraComponent>(cam);

    // Create some entity
    game->foo = create_entity(SV_ENTITY_NULL, "Foo");
    SpriteComponent* spr = add_component<SpriteComponent>(game->foo);
    spr->color = Color::Salmon();

    return true;
}

void update_scene(void*, void*)
{
    if (entity_exist(game->foo)) {

	v2_f32& pos = *get_entity_position2D_ptr(game->foo);

	f64 t = timer_now();

	pos.x = f32(cos(t)) * 4.f;
	pos.y = f32(sin(t)) * 2.f;
    }
}

SV_USER bool user_serialize_scene(Scene* scene, Archive* parchive)
{
    Archive& archive = *parchive;
    archive << game->foo;
    return true;
}


SV_USER bool user_get_scene_filepath(const char* name, char* filepath)
{
    sprintf(filepath, "assets/scenes/%s.scene", name);
    return true;
}
