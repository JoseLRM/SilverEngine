#pragma once

#include "defines.h"

namespace sv {

    struct Scene;
    struct Archive;
    
    typedef bool(*UserInitializeFn)(bool init);
    typedef void(*UserUpdateFn)();
    typedef bool(*UserCloseFn)(bool close);
    typedef bool(*UserValidateSceneFn)(const char* name);
    typedef bool(*UserGetSceneFilepathFn)(const char* name, char* filepath);
    typedef bool(*UserInitializeSceneFn)(Scene* scene, Archive* parchive);
    typedef bool(*UserCloseSceneFn)(Scene* scene);
    typedef bool(*UserSerializeSceneFn)(Scene* scene, Archive* parchive);
    
    struct UserCallbacks {
	UserInitializeFn initialize;
	UserUpdateFn update;
	UserCloseFn close;
	UserValidateSceneFn validate_scene;
	UserGetSceneFilepathFn get_scene_filepath;
	UserInitializeSceneFn initialize_scene;
	UserCloseSceneFn close_scene;
	UserSerializeSceneFn serialize_scene;
    };

    void _user_callbacks_set(const UserCallbacks& callbacks);

    bool _user_initialize(bool init_engine);
    void _user_update();
    bool _user_close(bool close_engine);
    bool _user_validate_scene(const char* name);
    bool _user_get_scene_filepath(const char* name, char* filepath);
    bool _user_initialize_scene(Scene* scene, Archive* parchive);
    bool _user_close_scene(Scene* scene);
    bool _user_serialize_scene(Scene* scene, Archive* parchive); 

}
